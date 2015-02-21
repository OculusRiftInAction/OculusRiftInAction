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

#pragma once

namespace shadertoy {
  static const int MAX_CHANNELS = 4;

  enum class ChannelInputType {
    TEXTURE, CUBEMAP, AUDIO, VIDEO,
  };

  extern const char * UNIFORM_RESOLUTION;
  extern const char * UNIFORM_GLOBALTIME;
  extern const char * UNIFORM_CHANNEL_TIME;
  extern const char * UNIFORM_CHANNEL_RESOLUTIONS[MAX_CHANNELS];
  extern const char * UNIFORM_CHANNEL_RESOLUTION;
  extern const char * UNIFORM_MOUSE_COORDS;
  extern const char * UNIFORM_DATE;
  extern const char * UNIFORM_SAMPLE_RATE;
  extern const char * UNIFORM_POSITION;
  extern const char * UNIFORM_CHANNELS[MAX_CHANNELS];

  extern const char * SHADER_HEADER;
  extern const char * LINE_NUMBER_HEADER;

  extern const QStringList TEXTURES;
  extern const QStringList CUBEMAPS;

  struct Shader {
    QString id;
    QString url;
    QString name;
    QString fragmentSource;
    ChannelInputType channelTypes[MAX_CHANNELS];
    QString channelTextures[MAX_CHANNELS];
  };

/*
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
*/

}
