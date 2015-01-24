/************************************************************************************

Authors     :   Bradley Austin Davis <bdavis@saintandreas.org>
Copyright   :   Copyright Bradley Austin Davis. All Rights reserved.

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

namespace shadertoy {
  const int MAX_CHANNELS = 4;

  enum class ChannelInputType {
    TEXTURE, CUBEMAP, AUDIO, VIDEO,
  };

  const char * UNIFORM_RESOLUTION = "iResolution";
  const char * UNIFORM_GLOBALTIME = "iGlobalTime";
  const char * UNIFORM_CHANNEL_TIME = "iChannelTime";
  const char * UNIFORM_CHANNEL_RESOLUTIONS[4] = {
    "iChannelResolution[0]",
    "iChannelResolution[1]",
    "iChannelResolution[2]",
    "iChannelResolution[3]",
  };
  const char * UNIFORM_CHANNEL_RESOLUTION = "iChannelResolution";
  const char * UNIFORM_MOUSE_COORDS = "iMouse";
  const char * UNIFORM_DATE = "iDate";
  const char * UNIFORM_SAMPLE_RATE = "iSampleRate";
  const char * UNIFORM_POSITION = "iPos";
  const char * UNIFORM_CHANNELS[4] = {
    "iChannel0",
    "iChannel1",
    "iChannel2",
    "iChannel3",
  };

  const char * SHADER_HEADER = "#version 330\n"
    "uniform vec3      iResolution;           // viewport resolution (in pixels)\n"
    "uniform float     iGlobalTime;           // shader playback time (in seconds)\n"
    "uniform float     iChannelTime[4];       // channel playback time (in seconds)\n"
    "uniform vec3      iChannelResolution[4]; // channel resolution (in pixels)\n"
    "uniform vec4      iMouse;                // mouse pixel coords. xy: current (if MLB down), zw: click\n"
    "uniform vec4      iDate;                 // (year, month, day, time in seconds)\n"
    "uniform float     iSampleRate;           // sound sample rate (i.e., 44100)\n"
    "uniform vec3      iPos; // Head position\n"
    "in vec3 iDir; // Direction from viewer\n"
    "out vec4 FragColor;\n";

  const char * LINE_NUMBER_HEADER =
    "#line 1\n";

  struct Preset {
    const Resource res;
    const char * name;
    Preset(Resource res, const char * name) : res(res), name(name) {};
  };

  const QStringList PRESETS({
    ":/shaders/default.xml",
    ":/shaders/4df3DS.json",
    ":/shaders/4dfGzs.json",
    ":/shaders/4djGWR.json",
    ":/shaders/4ds3zn.json",
    ":/shaders/4dXGRM_flying_steel_cubes.xml",
    ":/shaders/4sBGD1.json",
    ":/shaders/4slGzn.json",
    ":/shaders/4sX3R2.json",
    ":/shaders/4sXGRM_oceanic.xml",
    ":/shaders/4tXGDn.json",
    //":/shaders/ld23DG_crazy.xml",
    ":/shaders/ld2GRz.json",
    ":/shaders/ldfGzr.json",
    ":/shaders/ldj3Dm.json",
    ":/shaders/ldl3zr_mobius_balls.xml",
    ":/shaders/ldSGRW.json",
    ":/shaders/lsl3W2.json",
    ":/shaders/lss3WS_relentless.xml",
    ":/shaders/lts3Wn.json",
    ":/shaders/MdX3Rr.json",
    ":/shaders/MsBGRh.json",
//    ":/shaders/MsSGD1_hand_drawn_sketch.xml",
    ":/shaders/MsXGz4.json",
    ":/shaders/MsXGzM.json",
//    ":/shaders/MtfGR8_snowglobe.xml",
    ":/shaders/XdBSzd.json",
    ":/shaders/Xlf3D8.json",
    ":/shaders/XsBSRG_morning_city.xml",
    ":/shaders/XsjXR1.json",
    ":/shaders/XslXW2.json",
    ":/shaders/XsSSRW.json"
  });

  const QStringList TEXTURES({
    "/presets/tex00.jpg",
    "/presets/tex01.jpg",
    "/presets/tex02.jpg",
    "/presets/tex03.jpg",
    "/presets/tex04.jpg",
    "/presets/tex05.jpg",
    "/presets/tex06.jpg",
    "/presets/tex07.jpg",
    "/presets/tex08.jpg",
    "/presets/tex09.jpg",
    "/presets/tex10.png",
    "/presets/tex11.png",
    "/presets/tex12.png",
    "/presets/tex14.png",
    "/presets/tex15.png",
    "/presets/tex16.png"
  });

  const QStringList CUBEMAPS({
    "/presets/cube00_%1.jpg",
    "/presets/cube01_%1.png",
    "/presets/cube02_%1.jpg",
    "/presets/cube03_%1.png",
    "/presets/cube04_%1.png",
    "/presets/cube05_%1.png",
  });

  struct Shader {
    QString id;
    QString url;
    QString name;
    QString fragmentSource;
    ChannelInputType channelTypes[MAX_CHANNELS];
    QString channelTextures[MAX_CHANNELS];
  };
}
