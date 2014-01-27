package org.saintandreas.gl.shaders;

import static org.lwjgl.opengl.GL11.*;
import static org.lwjgl.opengl.GL20.*;

import org.saintandreas.Resources;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class Shader {

  private static final Logger LOG = LoggerFactory.getLogger(Shader.class);
  private final String sourcePath;
  private final int type;
  private long sourceTimestamp = -1;
  int shader = -1;

  public Shader(int type, String sourcePath) {
    this.sourcePath = sourcePath;
    this.type = type;
  }

  public boolean isStale() {
    return Resources.lastModified(sourcePath) > sourceTimestamp;
  }

  public void attach(int program) {
    glAttachShader(program, shader);
  }

  public void compile() {
    try {
      sourceTimestamp = Resources.lastModified(sourcePath);
      int newShader = compile(Resources.fromResources(sourcePath), type);
      if (-1 != shader) {
        glDeleteShader(shader);
      }
      shader = newShader;
    } catch (Exception e) {
      if (shader != -1) {
        glDeleteShader(shader);
        shader = -1;
      }
      throw e;
    }
  }

  public String getLog() {
    return getLog(shader);
  }

  // printShaderInfoLog
  // From OpenGL Shading Language 3rd Edition, p215-216
  // Display (hopefully) useful error messages if shader fails to compile
  public static String getLog(int shader) {
    return glGetShaderInfoLog(shader, 8192);
  }

  public static int compile(String source, int type) {
    int newShader = glCreateShader(type);
    glShaderSource(newShader, source);
    glCompileShader(newShader);
    int compileResult = glGetShaderi(newShader, GL_COMPILE_STATUS);
    if (GL_TRUE != compileResult) {
      String log = getLog(newShader);
      LOG.warn("shader compile failed :" + log);
      glDeleteShader(newShader);
      throw new IllegalStateException("Shader compile error" + log);
    }
    return newShader;
  }

}
