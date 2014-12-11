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

protected:
  ovrTexture eyeTextures[2];
  ovrVector3f eyeOffsets[2];
  bool eyePerFrameMode{false};

private:
  ovrEyeRenderDesc eyeRenderDescs[2];
  ovrPosef eyePoses[2];
  ovrEyeType currentEye;
  glm::mat4 projections[2];
  FramebufferWrapperPtr eyeFramebuffers[2];
  unsigned int frameCount{ 0 };
  bool renderingConfigured{ false };

protected:

  virtual void onFrameStart() {
  }

  virtual void onFrameEnd() {
  }

  virtual void * getRenderWindow() = 0;

  virtual void initGl();

  virtual void renderScene() = 0;

  inline ovrEyeType getCurrentEye() const {
    return currentEye;
  }

  const ovrEyeRenderDesc & getEyeRenderDesc(ovrEyeType eye) const {
    return eyeRenderDescs[eye];
  }

  const ovrFovPort & getFov(ovrEyeType eye) const {
    return eyeRenderDescs[eye].Fov;
  }

  const glm::mat4 & getPerspectiveProjection(ovrEyeType eye) const {
    return projections[eye];
  }

  const ovrPosef & getEyePose(ovrEyeType eye) const {
    return eyePoses[eye];
  }

  const ovrPosef & getEyePose() const {
    return getEyePose(getCurrentEye());
  }

  const ovrFovPort & getFov() const {
    return getFov(getCurrentEye());
  }

  const ovrEyeRenderDesc & getEyeRenderDesc() const {
    return getEyeRenderDesc(getCurrentEye());
  }

  const glm::mat4 & getPerspectiveProjection() const {
    return getPerspectiveProjection(getCurrentEye());
  }

public:
  RiftRenderingApp();
  virtual ~RiftRenderingApp();
  bool isRenderingConfigured();
  virtual void draw();

};


