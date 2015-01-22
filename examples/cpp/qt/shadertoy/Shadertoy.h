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

  Preset PRESETS[] {
    //Preset(Resource::SHADERTOY_SHADERS_DEFAULT_XML, "Default"),
    //Preset(Resource::SHADERTOY_SHADERS_LSS3WS_RELENTLESS_XML, "Relentless"),
    //Preset(Resource::SHADERTOY_SHADERS_4DFGZS_VOXEL_EDGES_XML, "Voxel Edges"),
    //Preset(Resource::SHADERTOY_SHADERS_4SBGD1_FAST_BALLS_XML, "Fast Balls"),
    //Preset(Resource::SHADERTOY_SHADERS_MDX3RR_ELEVATED_XML, "Elevated"),
    //Preset(Resource::SHADERTOY_SHADERS_MSXGZM_VORONOI_ROCKS_XML, "Voronoi Rocks"),
    //Preset(Resource::SHADERTOY_SHADERS_XSBSRG_MORNING_CITY_XML, "Morning City"),
    //Preset(Resource::SHADERTOY_SHADERS_4SX3R2_JSON, "Monster"),

//#ifdef OS_WIN
//    Preset(Resource::SHADERTOY_SHADERS_4DXGRM_FLYING_STEEL_CUBES_XML, "Steel Cubes"),
//    Preset(Resource::SHADERTOY_SHADERS_4DF3DS_INFINITE_CITY_XML, "Infinite City"),
//    Preset(Resource::SHADERTOY_SHADERS_4SXGRM_OCEANIC_XML, "Oceanic"),
//    Preset(Resource::SHADERTOY_SHADERS_MSXGZ4_CUBEMAP_XML, "Cubemap"),
//#endif

#if 0
    Preset(Resource::SHADERTOY_SHADERS_XSSSRW_ABANDONED_BASE_XML, "Abandoned Base"),
    Preset(Resource::SHADERTOY_SHADERS_4DJGWR_ROUNDED_VOXELS_XML, "Rounded Voxels"),
    Preset(Resource::SHADERTOY_SHADERS_MSSGD1_HAND_DRAWN_SKETCH_XML, "Hand Drawn"),
#endif

    Preset(Resource::NO_RESOURCE, nullptr),
  };

  template <typename T>
  class ResourceCounter {
    int count{ 0 };
  public:

    ResourceCounter(T * ts, std::function<bool(const T &)> endCondition =
        [](const T & t)->bool { return true; }
    ) {
      while (!endCondition(ts[count])) {
        ++count;
      }
    }

    int counted() {
      return count;
    }
  };

  const int MAX_PRESETS = ResourceCounter<Preset>(PRESETS, [] (const Preset & p){
    return p.res == NO_RESOURCE;
  }).counted();

  const Resource TEXTURES[] = {
    //Resource::SHADERTOY_TEXTURES_TEX00_JPG,
    //Resource::SHADERTOY_TEXTURES_TEX01_JPG,
    //Resource::SHADERTOY_TEXTURES_TEX02_JPG,
    //Resource::SHADERTOY_TEXTURES_TEX03_JPG,
    //Resource::SHADERTOY_TEXTURES_TEX04_JPG,
    //Resource::SHADERTOY_TEXTURES_TEX05_JPG,
    //Resource::SHADERTOY_TEXTURES_TEX06_JPG,
    //Resource::SHADERTOY_TEXTURES_TEX07_JPG,
    //Resource::SHADERTOY_TEXTURES_TEX08_JPG,
    //Resource::SHADERTOY_TEXTURES_TEX09_JPG,
    //Resource::SHADERTOY_TEXTURES_TEX10_PNG,
    //Resource::SHADERTOY_TEXTURES_TEX11_PNG,
    //Resource::SHADERTOY_TEXTURES_TEX12_PNG,
    //NO_RESOURCE,
    //Resource::SHADERTOY_TEXTURES_TEX14_PNG,
    //Resource::SHADERTOY_TEXTURES_TEX15_PNG,
    //Resource::SHADERTOY_TEXTURES_TEX16_PNG,
    NO_RESOURCE,
  };
  const int MAX_TEXTURES = 17;

  const Resource CUBEMAPS[] = {
    //Resource::SHADERTOY_CUBEMAPS_CUBE00_0_JPG,
    //Resource::SHADERTOY_CUBEMAPS_CUBE01_0_PNG,
    //Resource::SHADERTOY_CUBEMAPS_CUBE02_0_JPG,
    //Resource::SHADERTOY_CUBEMAPS_CUBE03_0_PNG,
    //Resource::SHADERTOY_CUBEMAPS_CUBE04_0_PNG,
    //Resource::SHADERTOY_CUBEMAPS_CUBE05_0_PNG,
    NO_RESOURCE,
  };
  const int MAX_CUBEMAPS = 6;

  struct Shader {
    bool vrSupport{ true };
    std::string id;
    std::string url;
    std::string name;
    std::string fragmentSource;
    ChannelInputType channelTypes[MAX_CHANNELS];
    std::string channelTextures[MAX_CHANNELS];
  };
}
