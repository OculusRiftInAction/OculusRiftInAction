package org.saintandreas;

import static org.lwjgl.opengl.GL11.*;
import static org.lwjgl.opengl.GL31.*;

import java.awt.Rectangle;

import org.lwjgl.input.Keyboard;
import org.saintandreas.gl.IndexedGeometry;
import org.saintandreas.gl.MatrixStack;
import org.saintandreas.gl.OpenGL;
import org.saintandreas.gl.app.LwjglApp;
import org.saintandreas.gl.buffers.VertexArray;
import org.saintandreas.gl.shaders.Program;
import org.saintandreas.math.Vector3f;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import uk.co.caprica.vlcj.binding.LibVlc;
import uk.co.caprica.vlcj.runtime.RuntimeUtil;

import com.sun.jna.Native;
import com.sun.jna.NativeLibrary;

public class XbmcDemo extends LwjglApp {
  @SuppressWarnings("unused")
  private static final Logger LOG = LoggerFactory.getLogger(XbmcDemo.class);
  IndexedGeometry cubeGeometry;

  @Override
  protected void initGl() {
    super.initGl();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(Short.MAX_VALUE);
    MatrixStack.MODELVIEW.lookat(
        new Vector3f(-2, 1, 3), 
        new Vector3f(0, 0, 0),
        new Vector3f(0, 1, 0));

    cubeGeometry = OpenGL.makeColorCube();
  }

  @Override
  protected void onResize(int width, int height) {
    super.onResize(width, height);
    MatrixStack.PROJECTION.perspective(80f, aspect, 0.01f, 1000.0f);
  }

  @Override
  protected void update() {
    // get event key here
    while (Keyboard.next()) {
      switch (Keyboard.getEventKey()) {
      }
    }
  }

  @Override
  public void drawFrame() {
    OpenGL.checkError();
    glViewport(0, 0, width, height);
    glClearColor(.1f, .1f, .1f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    Program program = Programs.getProgram(
        ExampleResource.SHADERS_COLORED_VS, 
        ExampleResource.SHADERS_COLORED_FS);
    program.link();
    program.use();
    MatrixStack.bind(program);

    cubeGeometry.ibo.bind();
    cubeGeometry.bindVertexArray();
    cubeGeometry.draw();

    VertexArray.unbind();
    cubeGeometry.ibo.unbind();
    Program.clear();
  }

  @Override
  protected void onDestroy() {
  }

  @Override
  protected void setupDisplay() {
    setupDisplay(new Rectangle(100, -700, 200, 160));
  }

  public static void main(String[] args) {
    NativeLibrary.addSearchPath("libvlc", "C:\\Program Files\\VideoLAN\\VLC");
    Native.loadLibrary(RuntimeUtil.getLibVlcLibraryName(), LibVlc.class);
    new XbmcDemo().run();
  }

}
