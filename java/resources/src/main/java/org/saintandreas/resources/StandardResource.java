package org.saintandreas.resources;

public enum StandardResource implements Resource {
  MESHES_BUN_ZIPPER_CTM("meshes/bun_zipper.ctm"), 
  MESHES_BUNNY2_CTM("meshes/bunny2.ctm"), 
  MESHES_CONE_CTM("meshes/cone.ctm"), 
  MESHES_CYLINDER_CTM("meshes/cylinder.ctm"), 
  MESHES_FRUSTUM_CTM("meshes/frustum.ctm"), 
  MESHES_HEMI_CTM("meshes/hemi.ctm"), 
  MESHES_ICO_SPHERE_CTM("meshes/ico-sphere.ctm"), 
  MESHES_ICO_CTM("meshes/ico.ctm"), 
  MESHES_RIFT_CTM("meshes/rift.ctm"), 
  MESHES_SPHERE_CTM("meshes/sphere.ctm"), 
  MESHES_TORUS_CTM("meshes/torus.ctm"), 
  FONTS_BUBBLEGUM_SANS_REGULAR_SDFF("fonts/Bubblegum Sans Regular.sdff"), 
  FONTS_INCONSOLATA_MEDIUM_SDFF("fonts/Inconsolata Medium.sdff"), 
  FONTS_WALTER_TURNCOAT_REGULAR_SDFF("fonts/Walter Turncoat Regular.sdff"), 
  IMAGES_SHOULDER_CAT_PNG("images/shoulder_cat.png"), 
  IMAGES_SKY_CITY_XNEG_PNG("images/sky/city/xneg.png"), 
  IMAGES_SKY_CITY_XPOS_PNG("images/sky/city/xpos.png"), 
  IMAGES_SKY_CITY_YNEG_PNG("images/sky/city/yneg.png"), 
  IMAGES_SKY_CITY_YPOS_PNG("images/sky/city/ypos.png"), 
  IMAGES_SKY_CITY_ZNEG_PNG("images/sky/city/zneg.png"), 
  IMAGES_SKY_CITY_ZPOS_PNG("images/sky/city/zpos.png"), 
  IMAGES_SKY_TRON_XNEG_PNG("images/sky/tron/xneg.png"), 
  IMAGES_SKY_TRON_XPOS_PNG("images/sky/tron/xpos.png"), 
  IMAGES_SKY_TRON_YNEG_PNG("images/sky/tron/yneg.png"), 
  IMAGES_SKY_TRON_YPOS_PNG("images/sky/tron/ypos.png"), 
  IMAGES_SKY_TRON_ZNEG_PNG("images/sky/tron/zneg.png"), 
  IMAGES_SKY_TRON_ZPOS_PNG("images/sky/tron/zpos.png"), 
  SHADERS_CUBEMAP_FS("shaders/CubeMap.fs"), 
  SHADERS_CUBEMAP_VS("shaders/CubeMap.vs"), 
  SHADERS_LIT_FS("shaders/Lit.fs"), 
  SHADERS_LIT_VS("shaders/Lit.vs"), 
  SHADERS_LITCOLORED_FS("shaders/LitColored.fs"), 
  SHADERS_LITCOLORED_VS("shaders/LitColored.vs"), 
  SHADERS_SIMPLECOLORED_VS("shaders/SimpleColored.vs"), 
  SHADERS_SIMPLETEXTURED_VS("shaders/SimpleTextured.vs"), 
  SHADERS_SIMPLE_VS("shaders/Simple.vs"), 
  SHADERS_SIMPLE_FS("shaders/Simple.fs"), 
  SHADERS_TEXT_FS("shaders/Text.fs"), 
  SHADERS_TEXT_VS("shaders/Text.vs"), 
  SHADERS_TEXTURE_FS("shaders/Texture.fs"), 

  SHADERS_NOISE_CELLULAR2_GLSL("shaders/noise/cellular2.glsl"), 
  SHADERS_NOISE_CELLULAR2X2_GLSL("shaders/noise/cellular2x2.glsl"), 
  SHADERS_NOISE_CELLULAR2X2X2_GLSL("shaders/noise/cellular2x2x2.glsl"), 
  SHADERS_NOISE_CELLULAR3_GLSL("shaders/noise/cellular3.glsl"), 
  SHADERS_NOISE_CNOISE2_GLSL("shaders/noise/cnoise2.glsl"), 
  SHADERS_NOISE_CNOISE3_GLSL("shaders/noise/cnoise3.glsl"), 
  SHADERS_NOISE_CNOISE4_GLSL("shaders/noise/cnoise4.glsl"), 
  SHADERS_NOISE_SNOISE2_GLSL("shaders/noise/snoise2.glsl"), 
  SHADERS_NOISE_SNOISE3_GLSL("shaders/noise/snoise3.glsl"), 
  SHADERS_NOISE_SNOISE4_GLSL("shaders/noise/snoise4.glsl"), 
  SHADERS_NOISE_SRDNOISE2_GLSL("shaders/noise/srdnoise2.glsl"), 
  NO_RESOURCE("");

  public final String path;
  StandardResource(String path) {
    this.path = path;
  }
  
  @Override
  public String getPath() {
    return path;
  }
}
