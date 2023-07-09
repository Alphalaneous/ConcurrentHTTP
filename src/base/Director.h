/****************************************************************************
 Copyright (c) 2008-2010 Ricardo Quesada
 Copyright (c) 2010-2013 cocos2d-x.org
 Copyright (c) 2011      Zynga Inc.
 Copyright (c) 2013-2016 Chukong Technologies Inc.
 Copyright (c) 2017-2019 Xiamen Yaji Software Co., Ltd.
 Copyright (c) 2021 Bytedance Inc.

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
#pragma once

#include <stack>
#include <thread>
#include <chrono>

#include "../platform/PlatformMacros.h"
#include "Ref.h"
#include "concurrentqueue.h"
#include "Scheduler.h"

/**
 * @addtogroup base
 * @{
 */

/* Forward declarations. */
class LabelAtlas;
// class GLView;
class DirectorDelegate;
class Node;
class ActionManager;
class EventDispatcher;
class EventCustom;
class EventListenerCustom;
class TextureCache;
class Renderer;
class Camera;
class Console;

/**
 @brief Class that creates and handles the main Window and manages how
 and when to execute the Scenes.

 The Director is also responsible for:
 - initializing the OpenGL context
 - setting the OpenGL buffer depth (default one is 0-bit)
 - setting the projection (default one is 3D)

 Since the Director is a singleton, the standard way to use it is by calling:
 _ Director::getInstance()->methodName();
 */
class Director : public Ref
{
public:
    
    static Director* getInstance();

    
    Director();

    ~Director();
    bool init();
    
    void setScheduler(Scheduler* scheduler);
    Scheduler* Director::getScheduler();
    const std::thread::id& getAxmolThreadId() const { return _axmol_thread_id; }

protected:
    void reset();

    Scheduler* _scheduler = nullptr;

    std::thread::id _axmol_thread_id;
};

// end of base group
/** @} */

