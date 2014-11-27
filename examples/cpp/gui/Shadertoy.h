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


  struct Preset {
    const Resource res;
    const char * name;
    Preset(Resource res, const char * name) : res(res), name(name) {};
  };

  Preset PRESETS[] {
    Preset(Resource::SHADERTOY_SHADERS_DEFAULT_FS, "Default"),
    Preset(Resource::SHADERTOY_SHADERS_4DXGRM_FLYING_STEEL_CUBES_FS, "Steel Cubes"),
    Preset(Resource::SHADERTOY_SHADERS_4DF3DS_INFINITE_CITY_FS, "Infinite City"),
    Preset(Resource::SHADERTOY_SHADERS_4DFGZS_VOXEL_EDGES_FS, "Voxel Edges"),
//    Preset(Resource::SHADERTOY_SHADERS_4DJGWR_ROUNDED_VOXELS_FS, "Rounded Voxels"),
    Preset(Resource::SHADERTOY_SHADERS_4SBGD1_FAST_BALLS_FS, "Fast Balls"),
    Preset(Resource::SHADERTOY_SHADERS_4SXGRM_OCEANIC_FS, "Oceanic"),
    Preset(Resource::SHADERTOY_SHADERS_MDX3RR_ELEVATED_FS, "Elevated"),
//    Preset(Resource::SHADERTOY_SHADERS_MSSGD1_HAND_DRAWN_SKETCH_FS, "Hand Drawn"),
    Preset(Resource::SHADERTOY_SHADERS_MSXGZM_VORONOI_ROCKS_FS, "Voronoi Rocks"),
    Preset(Resource::SHADERTOY_SHADERS_XSBSRG_MORNING_CITY_FS, "Morning City"),
    Preset(Resource::SHADERTOY_SHADERS_XSSSRW_ABANDONED_BASE_FS, "Abandoned Base"),
    Preset(Resource::SHADERTOY_SHADERS_MSXGZ4_CUBEMAP_FS, "Cubemap"),
    Preset(Resource::SHADERTOY_SHADERS_LSS3WS_RELENTLESS_FS, "Relentless"),
    Preset(Resource::NO_RESOURCE, nullptr),
  };

  const int MAX_PRESETS = 12;


  const Resource TEXTURES[] = {
    Resource::SHADERTOY_TEXTURES_TEX00_JPG,
    Resource::SHADERTOY_TEXTURES_TEX01_JPG,
    Resource::SHADERTOY_TEXTURES_TEX02_JPG,
    Resource::SHADERTOY_TEXTURES_TEX03_JPG,
    Resource::SHADERTOY_TEXTURES_TEX04_JPG,
    Resource::SHADERTOY_TEXTURES_TEX05_JPG,
    Resource::SHADERTOY_TEXTURES_TEX06_JPG,
    Resource::SHADERTOY_TEXTURES_TEX07_JPG,
    Resource::SHADERTOY_TEXTURES_TEX08_JPG,
    Resource::SHADERTOY_TEXTURES_TEX09_JPG,
    Resource::SHADERTOY_TEXTURES_TEX10_PNG,
    Resource::SHADERTOY_TEXTURES_TEX11_PNG,
    Resource::SHADERTOY_TEXTURES_TEX12_PNG,
    Resource::SHADERTOY_TEXTURES_TEX14_PNG,
    Resource::SHADERTOY_TEXTURES_TEX15_PNG,
    Resource::SHADERTOY_TEXTURES_TEX16_PNG,
    NO_RESOURCE,
  };
  const int MAX_TEXTURES = 17;

  const Resource CUBEMAPS[] = {
    Resource::SHADERTOY_CUBEMAPS_CUBE00_0_JPG,
    Resource::SHADERTOY_CUBEMAPS_CUBE01_0_PNG,
    Resource::SHADERTOY_CUBEMAPS_CUBE02_0_JPG,
    Resource::SHADERTOY_CUBEMAPS_CUBE03_0_PNG,
    Resource::SHADERTOY_CUBEMAPS_CUBE04_0_PNG,
    Resource::SHADERTOY_CUBEMAPS_CUBE05_0_PNG,
    NO_RESOURCE,
  };
  const int MAX_CUBEMAPS = 6;

  static std::string getChannelInputName(ChannelInputType type, int index) {
    switch (type) {
    case ChannelInputType::TEXTURE:
      return Platform::format("Tex%02d", index);
    case ChannelInputType::CUBEMAP:
      return Platform::format("Cube%02d", index);
    default:
      return "";
    }
  }

  static Resource getChannelInputResource(ChannelInputType type, int index) {
    switch (type) {
    case ChannelInputType::TEXTURE:
      return TEXTURES[index];
    case ChannelInputType::CUBEMAP:
      return CUBEMAPS[index];
    default:
      return NO_RESOURCE;
    }
  }
}
