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

class RiftRenderingApp : public RiftManagerApp {
  ovrEyeType currentEye{ovrEye_Count};
  FramebufferWrapperPtr eyeFramebuffers[2];
  unsigned int frameCount{ 0 };

protected:
  ovrPosef eyePoses[2];
  ovrVector3f eyeOffsets[2];
  ovrTexture eyeTextures[2];
  glm::mat4 projections[2];

  bool eyePerFrameMode{ false };
  ovrEyeType lastEyeRendered{ ovrEye_Count };

  std::mutex * endFrameLock{ nullptr };

private:
  virtual void * getNativeWindow() = 0;

protected:

  inline ovrEyeType getCurrentEye() const {
    return currentEye;
  }

  const ovrPosef & getEyePose() const {
    return eyePoses[currentEye];
  }

  virtual void updateFps(float fps) { }
  virtual void initializeRiftRendering();
  virtual void drawRiftFrame() final;
  virtual void perFrameRender() {};
  virtual void perEyeRender() {};

public:
  RiftRenderingApp();
  virtual ~RiftRenderingApp();
};


