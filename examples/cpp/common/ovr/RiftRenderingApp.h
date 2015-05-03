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
  struct EyeParams {
    ovrEyeRenderDesc    renderDesc;
    ovr::SwapTexFboPtr  fbo;
    uvec2               size;
    mat4                projection;
  };

  using EyeLayers = std::vector<ovrLayer_Union>;

  EyeLayers     layers;
  uint32_t      frameCount{ 0 };
  ovrVector3f   eyeOffsets[2];
  EyeParams     eyesParams[2];
  ovr::MirrorFboPtr  mirrorFbo;
  bool          mirrorEnabled{ true };


  bool          eyePerFrameMode{ false };
  ovrEyeType    currentEye{ ovrEye_Count };
  ovrEyeType    lastEyeRendered{ ovrEye_Count };

  std::mutex *  endFrameLock{ nullptr };

protected:

  inline ovrEyeType getCurrentEye() const {
    return currentEye;
  }

  const ovrPosef & getEyePose() const {
    return layers[0].EyeFov.RenderPose[currentEye];
  }

  virtual void updateFps(float fps) { }
  virtual void initializeRiftRendering();
  virtual void drawRiftFrame() final;
  virtual void perFrameRender() {};
  virtual void perEyeRender() {};
  virtual void setupMirror(const glm::uvec2 & size);

public:
  RiftRenderingApp();
  virtual ~RiftRenderingApp();
};


