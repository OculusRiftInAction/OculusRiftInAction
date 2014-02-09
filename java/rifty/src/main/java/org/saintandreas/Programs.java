package org.saintandreas;

import java.util.HashMap;
import java.util.Map;

import org.saintandreas.gl.shaders.Program;
import org.saintandreas.resources.Resource;

public class Programs {
  private static Map<String, Program> LOADED_PROGRAMS = new HashMap<>();

  private static String getKey(Resource vs, Resource fs) {
    return vs.getPath() + ":" + fs.getPath();
  }

  public static Program getProgram(Resource vs, Resource fs) {
    String key = getKey(vs, fs);
    if (!LOADED_PROGRAMS.containsKey(key)) {
      LOADED_PROGRAMS.put(key, new Program(vs.getPath(), fs.getPath()));
    }
    return LOADED_PROGRAMS.get(key);
  }
}
