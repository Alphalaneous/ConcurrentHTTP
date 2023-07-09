/****************************************************************************
Copyright (c) 2008-2010 Ricardo Quesada
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2011      Zynga Inc.
Copyright (c) 2013-2016 Chukong Technologies Inc.
Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.

https://axmolengine.github.io/

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/

#include "Scheduler.h"
#include "Macros.h"
#include "Director.h"
#include "CArray.h"
#include "utlist.h"


// data structures

// A list double-linked list used for "updates with priority"
typedef struct _listEntryX
{
    struct _listEntryX *prev, *next;
    ccSchedulerFunc callback;
    void* target;
    int priority;
    bool paused;
    bool markedForDeletion;  // selector will no longer be called and entry will be removed at end of the next tick
} tListEntryX;

typedef struct _hashUpdateEntryX
{
    tListEntryX** list;  // Which list does it belong to ?
    tListEntryX* entry;  // entry in the list
    void* target;
    ccSchedulerFunc callback;
    UT_hash_handle hh;
} tHashUpdateEntryX;

// Hash Element used for "selectors with interval"
typedef struct _hashSelectorEntryX
{
    ccArrayX* timers;
    void* target;
    int timerIndex;
    Timer* currentTimer;
    bool paused;
    UT_hash_handle hh;
} tHashTimerEntryX;

// implementation Timer

Timer::Timer()
    : _scheduler(nullptr)
    , _elapsed(-1)
    , _runForever(false)
    , _useDelay(false)
    , _timesExecuted(0)
    , _repeat(0)
    , _delay(0.0f)
    , _interval(0.0f)
    , _aborted(false)
{}

void Timer::setupTimerWithInterval(float seconds, unsigned int repeat, float delay)
{
    _elapsed       = -1;
    _interval      = seconds;
    _delay         = delay;
    _useDelay      = (_delay > 0.0f) ? true : false;
    _repeat        = repeat;
    _runForever    = (_repeat == AX_REPEAT_FOREVER) ? true : false;
    _timesExecuted = 0;
}

void Timer::update(float dt)
{
    if (_elapsed == -1)
    {
        _elapsed       = 0;
        _timesExecuted = 0;
        return;
    }

    // accumulate elapsed time
    _elapsed += dt;

    // deal with delay
    if (_useDelay)
    {
        if (_elapsed < _delay)
        {
            return;
        }
        _timesExecuted += 1;  // important to increment before call trigger
        trigger(_delay);
        _elapsed  = _elapsed - _delay;
        _useDelay = false;
        // after delay, the rest time should compare with interval
        if (isExhausted())
        {  // unschedule timer
            cancel();
            return;
        }
    }

    // if _interval == 0, should trigger once every frame
    float interval = (_interval > 0) ? _interval : _elapsed;
    while ((_elapsed >= interval) && !_aborted)
    {
        _timesExecuted += 1;  // important to increment before call trigger
        trigger(interval);
        _elapsed -= interval;

        if (isExhausted())
        {
            cancel();
            break;
        }

        if (_elapsed <= 0.f)
        {
            break;
        }
    }
}

bool Timer::isExhausted() const
{
    return !_runForever && _timesExecuted > _repeat;
}

// TimerTargetSelector

TimerTargetSelector::TimerTargetSelector() : _target(nullptr), _selector(nullptr) {}

bool TimerTargetSelector::initWithSelector(Scheduler* scheduler,
                                           SEL_SCHEDULEX selector,
                                           Ref* target,
                                           float seconds,
                                           unsigned int repeat,
                                           float delay)
{
    _scheduler = scheduler;
    _target    = target;
    _selector  = selector;
    setupTimerWithInterval(seconds, repeat, delay);
    return true;
}

void TimerTargetSelector::trigger(float dt)
{
    if (_target && _selector)
    {
        (_target->*_selector)(dt);
    }
}

void TimerTargetSelector::cancel()
{
    _scheduler->unschedule(_selector, _target);
}

// TimerTargetCallback

TimerTargetCallback::TimerTargetCallback() : _target(nullptr), _callback(nullptr) {}

bool TimerTargetCallback::initWithCallback(Scheduler* scheduler,
                                           const ccSchedulerFunc& callback,
                                           void* target,
                                           std::string_view key,
                                           float seconds,
                                           unsigned int repeat,
                                           float delay)
{
    _scheduler = scheduler;
    _target    = target;
    _callback  = callback;
    _key       = key;
    setupTimerWithInterval(seconds, repeat, delay);
    return true;
}

void TimerTargetCallback::trigger(float dt)
{
    if (_callback)
    {
        _callback(dt);
    }
}

void TimerTargetCallback::cancel()
{
    _scheduler->unschedule(_key, _target);
}

// implementation of Scheduler

// Priority level reserved for system services.
const int Scheduler::PRIORITY_SYSTEM = INT_MIN;

// Minimum priority level for user scheduling.
const int Scheduler::PRIORITY_NON_SYSTEM_MIN = PRIORITY_SYSTEM + 1;

Scheduler::Scheduler()
    : _timeScale(1.0f)
    , _updatesNegList(nullptr)
    , _updates0List(nullptr)
    , _updatesPosList(nullptr)
    , _hashForUpdates(nullptr)
    , _hashForTimers(nullptr)
    , _currentTarget(nullptr)
    , _currentTargetSalvaged(false)
    , _updateHashLocked(false)
{
    // I don't expect to have more than 30 functions to all per frame
    _actionsToPerform.reserve(30);
}

Scheduler::~Scheduler()
{
    unscheduleAll();
}

void Scheduler::removeHashElement(_hashSelectorEntryX* element)
{
    ccArrayFree(element->timers);
    HASH_DEL(_hashForTimers, element);
    free(element);
}

void Scheduler::schedule(const ccSchedulerFunc& callback,
                         void* target,
                         float interval,
                         bool paused,
                         std::string_view key)
{
    this->schedule(callback, target, interval, AX_REPEAT_FOREVER, 0.0f, paused, key);
}

void Scheduler::schedule(const ccSchedulerFunc& callback,
                         void* target,
                         float interval,
                         unsigned int repeat,
                         float delay,
                         bool paused,
                         std::string_view key)
{
    AXASSERT(target, "Argument target must be non-nullptr");
    AXASSERT(!key.empty(), "key should not be empty!");

    tHashTimerEntryX* element = nullptr;
    HASH_FIND_PTR(_hashForTimers, &target, element);

    if (!element)
    {
        element         = (tHashTimerEntryX*)calloc(sizeof(*element), 1);
        element->target = target;

        HASH_ADD_PTR(_hashForTimers, target, element);

        // Is this the 1st element ? Then set the pause level to all the selectors of this target
        element->paused = paused;
    }
    else
    {
        AXASSERT(element->paused == paused, "element's paused should be paused!");
    }

    if (element->timers == nullptr)
    {
        element->timers = ccArrayNew(10);
    }
    else
    {
        for (int i = 0; i < element->timers->num; ++i)
        {
            TimerTargetCallback* timer = dynamic_cast<TimerTargetCallback*>(element->timers->arr[i]);

            if (timer && !timer->isExhausted() && key == timer->getKey())
            {
                AXLOG("CCScheduler#schedule. Reiniting timer with interval %.4f, repeat %u, delay %.4f", interval,
                      repeat, delay);
                timer->setupTimerWithInterval(interval, repeat, delay);
                return;
            }
        }
        ccArrayEnsureExtraCapacity(element->timers, 1);
    }

    TimerTargetCallback* timer = new TimerTargetCallback();
    timer->initWithCallback(this, callback, target, key, interval, repeat, delay);
    ccArrayAppendObject(element->timers, timer);
    timer->release();
}

void Scheduler::unschedule(std::string_view key, void* target)
{
    // explicit handle nil arguments when removing an object
    if (target == nullptr || key.empty())
    {
        return;
    }

    // AXASSERT(target);
    // AXASSERT(selector);

    tHashTimerEntryX* element = nullptr;
    HASH_FIND_PTR(_hashForTimers, &target, element);

    if (element)
    {
        for (int i = 0; i < element->timers->num; ++i)
        {
            TimerTargetCallback* timer = dynamic_cast<TimerTargetCallback*>(element->timers->arr[i]);

            if (timer && key == timer->getKey())
            {
                if (timer == element->currentTimer && (!timer->isAborted()))
                {
                    timer->retain();
                    timer->setAborted();
                }

                ccArrayRemoveObjectAtIndex(element->timers, i, true);

                // update timerIndex in case we are in tick:, looping over the actions
                if (element->timerIndex >= i)
                {
                    element->timerIndex--;
                }

                if (element->timers->num == 0)
                {
                    if (_currentTarget == element)
                    {
                        _currentTargetSalvaged = true;
                    }
                    else
                    {
                        removeHashElement(element);
                    }
                }

                return;
            }
        }
    }
}

void Scheduler::priorityIn(_listEntryX** list, const ccSchedulerFunc& callback, void* target, int priority, bool paused)
{
    tListEntryX* listElement = new tListEntryX();

    listElement->callback = callback;
    listElement->target   = target;
    listElement->priority = priority;
    listElement->paused   = paused;
    listElement->next = listElement->prev = nullptr;
    listElement->markedForDeletion        = false;

    // empty list ?
    if (!*list)
    {
        DL_APPEND(*list, listElement);
    }
    else
    {
        bool added = false;

        for (tListEntryX* element = *list; element; element = element->next)
        {
            if (priority < element->priority)
            {
                if (element == *list)
                {
                    DL_PREPEND(*list, listElement);
                }
                else
                {
                    listElement->next = element;
                    listElement->prev = element->prev;

                    element->prev->next = listElement;
                    element->prev       = listElement;
                }

                added = true;
                break;
            }
        }

        // Not added? priority has the higher value. Append it.
        if (!added)
        {
            DL_APPEND(*list, listElement);
        }
    }

    // update hash entry for quick access
    tHashUpdateEntryX* hashElement = (tHashUpdateEntryX*)calloc(sizeof(*hashElement), 1);
    hashElement->target           = target;
    hashElement->list             = list;
    hashElement->entry            = listElement;
    memset(&hashElement->hh, 0, sizeof(hashElement->hh));
    HASH_ADD_PTR(_hashForUpdates, target, hashElement);
}

void Scheduler::appendIn(_listEntryX** list, const ccSchedulerFunc& callback, void* target, bool paused)
{
    tListEntryX* listElement = new tListEntryX();

    listElement->callback          = callback;
    listElement->target            = target;
    listElement->paused            = paused;
    listElement->priority          = 0;
    listElement->markedForDeletion = false;

    DL_APPEND(*list, listElement);

    // update hash entry for quicker access
    tHashUpdateEntryX* hashElement = (tHashUpdateEntryX*)calloc(sizeof(*hashElement), 1);
    hashElement->target           = target;
    hashElement->list             = list;
    hashElement->entry            = listElement;
    memset(&hashElement->hh, 0, sizeof(hashElement->hh));
    HASH_ADD_PTR(_hashForUpdates, target, hashElement);
}

void Scheduler::schedulePerFrame(const ccSchedulerFunc& callback, void* target, int priority, bool paused)
{
    tHashUpdateEntryX* hashElement = nullptr;
    HASH_FIND_PTR(_hashForUpdates, &target, hashElement);
    if (hashElement)
    {
        // change priority: should unschedule it first
        if (hashElement->entry->priority != priority)
        {
            unscheduleUpdate(target);
        }
        else
        {
            // don't add it again
            return;
        }
    }

    // most of the updates are going to be 0, that's way there
    // is an special list for updates with priority 0
    if (priority == 0)
    {
        appendIn(&_updates0List, callback, target, paused);
    }
    else if (priority < 0)
    {
        priorityIn(&_updatesNegList, callback, target, priority, paused);
    }
    else
    {
        // priority > 0
        priorityIn(&_updatesPosList, callback, target, priority, paused);
    }
}

bool Scheduler::isScheduled(std::string_view key, const void* target) const
{
    AXASSERT(!key.empty(), "Argument key must not be empty");
    AXASSERT(target, "Argument target must be non-nullptr");

    tHashTimerEntryX* element = nullptr;
    HASH_FIND_PTR(_hashForTimers, &target, element);

    if (!element)
    {
        return false;
    }

    if (element->timers == nullptr)
    {
        return false;
    }

    for (int i = 0; i < element->timers->num; ++i)
    {
        TimerTargetCallback* timer = dynamic_cast<TimerTargetCallback*>(element->timers->arr[i]);

        if (timer && !timer->isExhausted() && key == timer->getKey())
        {
            return true;
        }
    }

    return false;
}

void Scheduler::removeUpdateFromHash(struct _listEntryX* entry)
{
    tHashUpdateEntryX* element = nullptr;

    HASH_FIND_PTR(_hashForUpdates, &entry->target, element);
    if (element)
    {
        // list entry
        DL_DELETE(*element->list, element->entry);
        if (!_updateHashLocked)
            AX_SAFE_DELETE(element->entry);
        else
        {
            element->entry->markedForDeletion = true;
            _updateDeleteVector.emplace_back(element->entry);
        }

        // hash entry
        HASH_DEL(_hashForUpdates, element);
        free(element);
    }
}

void Scheduler::unscheduleUpdate(void* target)
{
    if (target == nullptr)
    {
        return;
    }

    tHashUpdateEntryX* element = nullptr;
    HASH_FIND_PTR(_hashForUpdates, &target, element);
    if (element)
        this->removeUpdateFromHash(element->entry);
}

void Scheduler::unscheduleAll()
{
    unscheduleAllWithMinPriority(PRIORITY_SYSTEM);
}

void Scheduler::unscheduleAllWithMinPriority(int minPriority)
{
    // Custom Selectors
    tHashTimerEntryX* element     = nullptr;
    tHashTimerEntryX* nextElement = nullptr;
    for (element = _hashForTimers; element != nullptr;)
    {
        // element may be removed in unscheduleAllSelectorsForTarget
        nextElement = (tHashTimerEntryX*)element->hh.next;
        unscheduleAllForTarget(element->target);

        element = nextElement;
    }

    // Updates selectors
    tListEntryX *entry, *tmp;
    if (minPriority < 0)
    {
        DL_FOREACH_SAFE(_updatesNegList, entry, tmp)
        {
            if (entry->priority >= minPriority)
            {
                unscheduleUpdate(entry->target);
            }
        }
    }

    if (minPriority <= 0)
    {
        DL_FOREACH_SAFE(_updates0List, entry, tmp) { unscheduleUpdate(entry->target); }
    }

    DL_FOREACH_SAFE(_updatesPosList, entry, tmp)
    {
        if (entry->priority >= minPriority)
        {
            unscheduleUpdate(entry->target);
        }
    }
}

void Scheduler::unscheduleAllForTarget(void* target)
{
    // explicit nullptr handling
    if (target == nullptr)
    {
        return;
    }

    // Custom Selectors
    tHashTimerEntryX* element = nullptr;
    HASH_FIND_PTR(_hashForTimers, &target, element);

    if (element)
    {
        if (ccArrayContainsObject(element->timers, element->currentTimer) && (!element->currentTimer->isAborted()))
        {
            element->currentTimer->retain();
            element->currentTimer->setAborted();
        }
        ccArrayRemoveAllObjects(element->timers);

        if (_currentTarget == element)
        {
            _currentTargetSalvaged = true;
        }
        else
        {
            removeHashElement(element);
        }
    }

    // update selector
    unscheduleUpdate(target);
}


void Scheduler::resumeTarget(void* target)
{
    AXASSERT(target != nullptr, "target can't be nullptr!");

    // custom selectors
    tHashTimerEntryX* element = nullptr;
    HASH_FIND_PTR(_hashForTimers, &target, element);
    if (element)
    {
        element->paused = false;
    }

    // update selector
    tHashUpdateEntryX* elementUpdate = nullptr;
    HASH_FIND_PTR(_hashForUpdates, &target, elementUpdate);
    if (elementUpdate)
    {
        AXASSERT(elementUpdate->entry != nullptr, "elementUpdate's entry can't be nullptr!");
        elementUpdate->entry->paused = false;
    }
}

void Scheduler::pauseTarget(void* target)
{
    AXASSERT(target != nullptr, "target can't be nullptr!");

    // custom selectors
    tHashTimerEntryX* element = nullptr;
    HASH_FIND_PTR(_hashForTimers, &target, element);
    if (element)
    {
        element->paused = true;
    }

    // update selector
    tHashUpdateEntryX* elementUpdate = nullptr;
    HASH_FIND_PTR(_hashForUpdates, &target, elementUpdate);
    if (elementUpdate)
    {
        AXASSERT(elementUpdate->entry != nullptr, "elementUpdate's entry can't be nullptr!");
        elementUpdate->entry->paused = true;
    }
}

bool Scheduler::isTargetPaused(void* target)
{
    AXASSERT(target != nullptr, "target must be non nil");

    // Custom selectors
    tHashTimerEntryX* element = nullptr;
    HASH_FIND_PTR(_hashForTimers, &target, element);
    if (element)
    {
        return element->paused;
    }

    // We should check update selectors if target does not have custom selectors
    tHashUpdateEntryX* elementUpdate = nullptr;
    HASH_FIND_PTR(_hashForUpdates, &target, elementUpdate);
    if (elementUpdate)
    {
        return elementUpdate->entry->paused;
    }

    return false;  // should never get here
}

std::set<void*> Scheduler::pauseAllTargets()
{
    return pauseAllTargetsWithMinPriority(PRIORITY_SYSTEM);
}

std::set<void*> Scheduler::pauseAllTargetsWithMinPriority(int minPriority)
{
    std::set<void*> idsWithSelectors;

    // Custom Selectors
    for (tHashTimerEntryX* element = _hashForTimers; element != nullptr; element = (tHashTimerEntryX*)element->hh.next)
    {
        element->paused = true;
        idsWithSelectors.insert(element->target);
    }

    // Updates selectors
    tListEntryX *entry, *tmp;
    if (minPriority < 0)
    {
        DL_FOREACH_SAFE(_updatesNegList, entry, tmp)
        {
            if (entry->priority >= minPriority)
            {
                entry->paused = true;
                idsWithSelectors.insert(entry->target);
            }
        }
    }

    if (minPriority <= 0)
    {
        DL_FOREACH_SAFE(_updates0List, entry, tmp)
        {
            entry->paused = true;
            idsWithSelectors.insert(entry->target);
        }
    }

    DL_FOREACH_SAFE(_updatesPosList, entry, tmp)
    {
        if (entry->priority >= minPriority)
        {
            entry->paused = true;
            idsWithSelectors.insert(entry->target);
        }
    }

    return idsWithSelectors;
}

void Scheduler::resumeTargets(const std::set<void*>& targetsToResume)
{
    for (const auto& obj : targetsToResume)
    {
        this->resumeTarget(obj);
    }
}

void Scheduler::runOnAxmolThread(std::function<void()> action)
{
    std::lock_guard<std::mutex> lock(_performMutex);
    _actionsToPerform.emplace_back(std::move(action));
}

void Scheduler::removeAllPendingActions()
{
    std::unique_lock<std::mutex> lock(_performMutex);
    _actionsToPerform.clear();
}

// main loop
void Scheduler::update(float dt)
{

    _updateHashLocked = true;

    if (_timeScale != 1.0f)
    {
        dt *= _timeScale;
    }

    //
    // Selector callbacks
    //

    // Iterate over all the Updates' selectors
    tListEntryX *entry, *tmp;

    // updates with priority < 0
    DL_FOREACH_SAFE(_updatesNegList, entry, tmp)
    {
        if ((!entry->paused) && (!entry->markedForDeletion))
        {
            entry->callback(dt);
        }
    }

    // updates with priority == 0
    DL_FOREACH_SAFE(_updates0List, entry, tmp)
    {
        if ((!entry->paused) && (!entry->markedForDeletion))
        {
            entry->callback(dt);
        }
    }

    // updates with priority > 0
    DL_FOREACH_SAFE(_updatesPosList, entry, tmp)
    {
        if ((!entry->paused) && (!entry->markedForDeletion))
        {
            entry->callback(dt);
        }
    }

    // Iterate over all the custom selectors
    for (tHashTimerEntryX* elt = _hashForTimers; elt != nullptr;)
    {
        _currentTarget         = elt;
        _currentTargetSalvaged = false;

        if (!_currentTarget->paused)
        {
            // The 'timers' array may change while inside this loop
            for (elt->timerIndex = 0; elt->timerIndex < elt->timers->num; ++(elt->timerIndex))
            {
                elt->currentTimer = (Timer*)(elt->timers->arr[elt->timerIndex]);
                AXASSERT(!elt->currentTimer->isAborted(), "An aborted timer should not be updated");

                elt->currentTimer->update(dt);

                if (elt->currentTimer->isAborted())
                {
                    // The currentTimer told the remove itself. To prevent the timer from
                    // accidentally deallocating itself before finishing its step, we retained
                    // it. Now that step is done, it's safe to release it.
                    elt->currentTimer->release();
                }

                elt->currentTimer = nullptr;
            }
        }

        // elt, at this moment, is still valid
        // so it is safe to ask this here (issue #490)
        elt = (tHashTimerEntryX*)elt->hh.next;

        // only delete currentTarget if no actions were scheduled during the cycle (issue #481)
        if (_currentTargetSalvaged && _currentTarget->timers->num == 0)
        {
            removeHashElement(_currentTarget);
        }
    }

    // delete all updates that are removed in update
    for (auto&& e : _updateDeleteVector)
        delete e;

    _updateDeleteVector.clear();

    _updateHashLocked = false;
    _currentTarget    = nullptr;

    //
    // Functions allocated from another thread
    //

    // Testing size is faster than locking / unlocking.
    // And almost never there will be functions scheduled to be called.
    if (!_actionsToPerform.empty())
    {
        _performMutex.lock();
        // fixed #4123: Save the callback functions, they must be invoked after '_performMutex.unlock()', otherwise if
        // new functions are added in callback, it will cause thread deadlock.
        auto temp = std::move(_actionsToPerform);
        _performMutex.unlock();

        for (const auto& function : temp)
        {
            function();
        }
    }
}

void Scheduler::schedule(SEL_SCHEDULEX selector,
                         Ref* target,
                         float interval,
                         unsigned int repeat,
                         float delay,
                         bool paused)
{
    AXASSERT(target, "Argument target must be non-nullptr");

    tHashTimerEntryX* element = nullptr;
    HASH_FIND_PTR(_hashForTimers, &target, element);

    if (!element)
    {
        element         = (tHashTimerEntryX*)calloc(sizeof(*element), 1);
        element->target = target;

        HASH_ADD_PTR(_hashForTimers, target, element);

        // Is this the 1st element ? Then set the pause level to all the selectors of this target
        element->paused = paused;
    }
    else
    {
        AXASSERT(element->paused == paused, "element's paused should be paused.");
    }

    if (element->timers == nullptr)
    {
        element->timers = ccArrayNew(10);
    }
    else
    {
        for (int i = 0; i < element->timers->num; ++i)
        {
            TimerTargetSelector* timer = dynamic_cast<TimerTargetSelector*>(element->timers->arr[i]);

            if (timer && !timer->isExhausted() && selector == timer->getSelector())
            {
                AXLOG("CCScheduler#schedule. Reiniting timer with interval %.4f, repeat %u, delay %.4f", interval,
                      repeat, delay);
                timer->setupTimerWithInterval(interval, repeat, delay);
                return;
            }
        }
        ccArrayEnsureExtraCapacity(element->timers, 1);
    }

    TimerTargetSelector* timer = new TimerTargetSelector();
    timer->initWithSelector(this, selector, target, interval, repeat, delay);
    ccArrayAppendObject(element->timers, timer);
    timer->release();
}

void Scheduler::schedule(SEL_SCHEDULEX selector, Ref* target, float interval, bool paused)
{
    this->schedule(selector, target, interval, AX_REPEAT_FOREVER, 0.0f, paused);
}

bool Scheduler::isScheduled(SEL_SCHEDULEX selector, const Ref* target) const
{
    AXASSERT(selector, "Argument selector must be non-nullptr");
    AXASSERT(target, "Argument target must be non-nullptr");

    tHashTimerEntryX* element = nullptr;
    HASH_FIND_PTR(_hashForTimers, &target, element);

    if (!element)
    {
        return false;
    }

    if (element->timers == nullptr)
    {
        return false;
    }

    for (int i = 0; i < element->timers->num; ++i)
    {
        TimerTargetSelector* timer = dynamic_cast<TimerTargetSelector*>(element->timers->arr[i]);

        if (timer && !timer->isExhausted() && selector == timer->getSelector())
        {
            return true;
        }
    }

    return false;
}

void Scheduler::unschedule(SEL_SCHEDULEX selector, Ref* target)
{
    // explicit handle nil arguments when removing an object
    if (target == nullptr || selector == nullptr)
    {
        return;
    }

    tHashTimerEntryX* element = nullptr;
    HASH_FIND_PTR(_hashForTimers, &target, element);

    if (element)
    {
        for (int i = 0; i < element->timers->num; ++i)
        {
            TimerTargetSelector* timer = dynamic_cast<TimerTargetSelector*>(element->timers->arr[i]);

            if (timer && selector == timer->getSelector())
            {
                if (timer == element->currentTimer && !timer->isAborted())
                {
                    timer->retain();
                    timer->setAborted();
                }

                ccArrayRemoveObjectAtIndex(element->timers, i, true);

                // update timerIndex in case we are in tick:, looping over the actions
                if (element->timerIndex >= i)
                {
                    element->timerIndex--;
                }

                if (element->timers->num == 0)
                {
                    if (_currentTarget == element)
                    {
                        _currentTargetSalvaged = true;
                    }
                    else
                    {
                        removeHashElement(element);
                    }
                }

                return;
            }
        }
    }
}

