package org.saintandreas.gl.app;


import org.lwjgl.LWJGLException;
import org.lwjgl.input.Keyboard;
import org.lwjgl.input.Mouse;
import org.lwjgl.opengl.Display;
import org.lwjgl.opengl.DisplayMode;
import org.lwjgl.opengl.GLContext;

public class LwjglApp implements Runnable {
  private GLContext glContext = new GLContext();
  protected int width = 960, height = 600;
  protected float aspect = 1.0f;

  protected void initGl() {
  }

  protected void drawFrame() {
  }

  protected void setupDisplay() throws LWJGLException {
    width = 1280;
    height = 800;
    Display.setDisplayMode(new DisplayMode(width, height));
  }
  
  @Override
  public void run() {
    try {
      System.setProperty("org.lwjgl.opengl.Window.undecorated","true");
      setupDisplay();
      Display.create();
      GLContext.useContext(glContext, false);
      Mouse.create();
      Keyboard.create();
    } catch (LWJGLException e) {
      throw new RuntimeException(e);
    }
    initGl();
    while (!Display.isCloseRequested()) {
      if (Display.wasResized()) {
        onResize(Display.getWidth(), Display.getHeight());
      }
      update();
      drawFrame();
      Display.update();
      Display.sync(60);
    }
    Display.destroy();
  }

  protected void update() {
  }

  protected void onResize(int width, int height) {
    this.width = width;
    this.height = height;
    this.aspect = (float)width / (float)height;
  }
}
