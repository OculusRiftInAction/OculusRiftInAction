package org.saintandreas.gl.app;


import java.awt.GraphicsDevice;
import java.awt.GraphicsEnvironment;
import java.awt.Rectangle;

import org.lwjgl.LWJGLException;
import org.lwjgl.input.Keyboard;
import org.lwjgl.input.Mouse;
import org.lwjgl.opengl.Display;
import org.lwjgl.opengl.DisplayMode;
import org.lwjgl.opengl.GLContext;

public abstract class LwjglApp implements Runnable {
  private GLContext glContext = new GLContext();
  protected int width, height;
  protected float aspect = 1.0f;

  protected void initGl() {
  }

  protected void drawFrame() {
  }

  protected abstract void setupDisplay();

  protected void setupDisplay(Rectangle r) {
    setupDisplay(r.x, r.y, r.width, r.height);
  }
  
  protected void setupDisplay(int left, int top, int width, int height) {
    try {
      Display.setDisplayMode(new DisplayMode(width, height));
    } catch (LWJGLException e) {
      throw new RuntimeException(e);
    }
    Display.setLocation(left, top);
    onResize(width, height);
  }

  @Override
  public void run() {
    try {
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
    onDestroy();
    Display.destroy();
  }

  protected void update() {
  }

  protected void onResize(int width, int height) {
    this.width = width;
    this.height = height;
    this.aspect = (float)width / (float)height;
  }

  protected void onDestroy() {
  }

  public static Rectangle findNonPrimaryRect() {
    GraphicsEnvironment ge = GraphicsEnvironment.getLocalGraphicsEnvironment();
    GraphicsDevice[] gs = ge.getScreenDevices();
    for (int j = 0; j < gs.length; j++) {
      GraphicsDevice gd = gs[j];
      if (gd == ge.getDefaultScreenDevice()) {
        continue;
      }
      return gd.getDefaultConfiguration().getBounds();
    }
    return null;
  }

}
