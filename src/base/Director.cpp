/****************************************************************************
Copyright (c) 2008-2010 Ricardo Quesada
Copyright (c) 2010-2013 cocos2d-x.org
Copyright (c) 2011      Zynga Inc.
Copyright (c) 2013-2016 Chukong Technologies Inc.
Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.
Copyright (c) 2021-2022 Bytedance Inc.

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

// cocos2d includes
#include "../base/Director.h"

// standard includes
#include <string>

#include "Utils.h"
#include "Scheduler.h"
#include "Macros.h"


using namespace std;

// FIXME: it should be a Director ivar. Move it there once support for multiple directors is added

// singleton stuff
static Director* s_SharedDirector = nullptr;

#define kDefaultFPS 60  // 60 frames per second

std::thread::id _axmol_thread_id;

Scheduler _scheduler;

Director* Director::getInstance()
{
    if (!s_SharedDirector)
    {
        s_SharedDirector = new Director();
        AXASSERT(s_SharedDirector, "FATAL: Not enough memory");
        s_SharedDirector->init();
    }

    return s_SharedDirector;
}

Director::Director() {}

bool Director::init()
{

    _scheduler = new Scheduler();

    return true;
}

Director::~Director()
{
    AXLOGINFO("deallocing Director: %p", this);

    
    AX_SAFE_RELEASE(_scheduler);

}



void Director::reset()
{
#
    getScheduler()->unscheduleAll();
}

Scheduler* Director::getScheduler() {
    return _scheduler;
}

void Director::setScheduler(Scheduler* scheduler)
{
    if (_scheduler != scheduler)
    {
        AX_SAFE_RETAIN(scheduler);
        AX_SAFE_RELEASE(_scheduler);
        _scheduler = scheduler;
    }
}


