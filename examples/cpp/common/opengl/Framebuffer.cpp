/************************************************************************************
 
 Authors     :   Bradley Austin Davis <bdavis@saintandreas.org>
 Copyright   :   Copyright Brad Davis. All Rights reserved.
 
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
 http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 
 ************************************************************************************/

#include "Common.h"

using namespace oglplus;
struct FboState {
    FramebufferWrapper* draw{ nullptr };
    FramebufferWrapper* read{ nullptr };

    FboState() {
    }

    FboState(const FboState & oldState, Framebuffer::Target target, FramebufferWrapper * fboWrapper) {
        draw = (target == Framebuffer::Target::Draw) ? fboWrapper : oldState.draw;
        read = (target == Framebuffer::Target::Read) ? fboWrapper : oldState.read;
    }

    static void transition(const FboState & oldState, const FboState & newState) {
        if (oldState.draw != newState.draw) {
            if (newState.draw) {
                if (oldState.draw) {
                    oldState.draw->onUnbind(Framebuffer::Target::Draw);
                }
                newState.draw->fbo.Bind(Framebuffer::Target::Draw);
                newState.draw->onBind(Framebuffer::Target::Draw);
            } else {
                DefaultFramebuffer().Bind(Framebuffer::Target::Draw);
            }
        }

        if (oldState.read != newState.read) {
            if (newState.read) {
                if (oldState.read) {
                    oldState.read->onUnbind(Framebuffer::Target::Read);
                }
                newState.read->fbo.Bind(Framebuffer::Target::Read);
                newState.read->onBind(Framebuffer::Target::Read);
            } else {
                DefaultFramebuffer().Bind(Framebuffer::Target::Read);
            }
        }
    }
};

static std::stack<FboState> fbo_stack;


FramebufferStack::FramebufferStack() {
    fbo_stack.push(FboState());
}

FramebufferStack & FramebufferStack::instance() {
    static FramebufferStack stack;
    return stack;
}

void FramebufferStack::Push(oglplus::Framebuffer::Target target, FramebufferWrapper * fboWrapper) {
    FboState newState(fbo_stack.top(), target, fboWrapper);
    FboState::transition(fbo_stack.top(), newState);
    fbo_stack.push(newState);
}

void FramebufferStack::Pop() {
    assert(fbo_stack.size() > 1);
    auto result = fbo_stack.top();
    fbo_stack.pop();
    FboState::transition(result, fbo_stack.top());
    assert(fbo_stack.size() >= 1);
}
