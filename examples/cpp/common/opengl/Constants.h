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

#pragma once

namespace oria {
  namespace Layout {
    namespace Attribute {
      enum {
        Position = 0,
        TexCoord0 = 1,
        Normal = 2,
        Color = 3,
        TexCoord1 = 4,
        InstanceTransform = 5,
      };
    }

    namespace Uniform {
      enum {
        Projection = 0,
        ModelView = 1,
        NormalMatrix = 2,
        Time = 3,
        Color = 4,
        LightAmbient = 8,
        LightCount = 9,
        ForceAlpha = 10,
        LightPosition = 16,
        LightColor = 24,
      };
    }
  }
}