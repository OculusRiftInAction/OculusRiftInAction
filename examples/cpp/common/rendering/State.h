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

#include "Lights.h"
#include "MatrixStack.h"

class Stacks {
public:
  static MatrixStack & projection() {
    static MatrixStack projection;
    return projection;
  }

  static MatrixStack & modelview() {
    static MatrixStack modelview;
    return modelview;
  }

  template <typename Function>
  static void withPush(MatrixStack & stack, Function f) {
    stack.withPush(f);
  }

  template <typename Function>
  static void withPush(MatrixStack & stack1, MatrixStack & stack2, Function f) {
    stack1.withPush([&]{
      stack2.withPush(f);
    });
  }

  template <typename Function>
  static void withPush(Function f) {
    withPush(projection(), modelview(), f);
  }

  template <typename Function>
  static void withIdentity(Function f) {
    withPush(projection(), modelview(), [=] {
      projection().identity();
      modelview().identity();
      f();
    });
  }

  static Lights & lights() {
    static Lights lights;
    return lights;
  }
};
