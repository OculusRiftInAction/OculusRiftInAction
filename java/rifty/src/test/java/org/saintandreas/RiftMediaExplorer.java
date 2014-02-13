package org.saintandreas;

import static org.lwjgl.opengl.GL11.*;
import static org.lwjgl.opengl.GL31.*;

import java.awt.GraphicsEnvironment;
import java.awt.Rectangle;
import java.io.IOException;

import org.saintandreas.gl.FrameBuffer;
import org.saintandreas.gl.IndexedGeometry;
import org.saintandreas.gl.MatrixStack;
import org.saintandreas.gl.OpenGL;
import org.saintandreas.gl.app.LwjglApp;
import org.saintandreas.gl.buffers.VertexArray;
import org.saintandreas.gl.shaders.Program;
import org.saintandreas.math.Vector3f;
import org.saintandreas.vr.Hmd.Eye;
import org.saintandreas.vr.oculus.RiftDK1;

import com.oculusvr.rift.RiftTracker;
import com.oculusvr.rift.fusion.SensorFusion;

public class RiftMediaExplorer extends LwjglApp {
  IndexedGeometry cubeGeometry;
  IndexedGeometry eyeMeshes[] = new IndexedGeometry[2];
  Program renderProgram;
  Program distortProgram;
  FrameBuffer frameBuffer;
  SensorFusion sensorFusion;

  @Override
  protected void initGl() {
    super.initGl();
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(Short.MAX_VALUE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH); 
    glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);    
    
    MatrixStack.MODELVIEW.lookat(new Vector3f(-2, 1, 3), Vector3f.ZERO,
        Vector3f.UNIT_Y);

    frameBuffer = new FrameBuffer(width / 2, height);
    frameBuffer.getTexture().bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    frameBuffer.getTexture().unbind();

    eyeMeshes[0] = new RiftDK1().getDistortionMesh(Eye.LEFT, 8);
    eyeMeshes[1] = new RiftDK1().getDistortionMesh(Eye.RIGHT, 8);

    distortProgram = new Program(
        ExampleResource.SHADERS_TEXTURED_VS,
        ExampleResource.SHADERS_TEXTURED_FS);
    distortProgram.link();
    renderProgram = new Program(ExampleResource.SHADERS_COLORED_VS,
        ExampleResource.SHADERS_COLORED_FS);
    renderProgram.link();
    cubeGeometry = OpenGL.makeColorCube();

    sensorFusion = new SensorFusion();
    sensorFusion.setMotionTrackingEnabled(true);
    try {
      RiftTracker.startListening(sensorFusion.createHandler());
    } catch (IOException e) {
      throw new RuntimeException(e);
    }
  }

  @Override
  protected void onResize(int width, int height) {
    super.onResize(width, height);
    MatrixStack.PROJECTION.perspective(80f, aspect / 2.0f, 0.01f, 1000.0f);
  }

  @Override
  public void drawFrame() {
    MatrixStack.MODELVIEW.push().rotate(sensorFusion.getOrientation());

    OpenGL.checkError();
    glViewport(0, 0, width, height);
    glClearColor(.3f, .3f, .3f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for (int i = 0; i < 2; ++i) {
      frameBuffer.activate();
      float offset = RiftDK1.LensOffset;
      float ipd = 0.06400f;
      float mvo = ipd / 2.0f;
      if (i == 1) {
        offset *= -1f;
        mvo *= -1f;
      }
      // new Matrix4f().translate(new Vector2f(offset, 0));
      MatrixStack.PROJECTION.push().preTranslate(offset);
      MatrixStack.MODELVIEW.push().preTranslate(mvo);
      renderScene();
      MatrixStack.MODELVIEW.pop();
      MatrixStack.PROJECTION.pop();

      frameBuffer.deactivate();

      int x = i == 0 ? 0 : (width / 2);
      glViewport(x, 0, width / 2, height);
      renderProgram.use();
      glLineWidth(3.0f);
      MatrixStack.PROJECTION.push().identity();
      MatrixStack.MODELVIEW.push().identity();
      MatrixStack.bind(renderProgram);
      MatrixStack.PROJECTION.pop();
      MatrixStack.MODELVIEW.pop();
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      //distortProgram.use();
      frameBuffer.getTexture().bind();
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      eyeMeshes[i].bindVertexArray();
      eyeMeshes[i].draw();
      VertexArray.unbind();
      Program.clear();
    }

    MatrixStack.MODELVIEW.pop();
  }

  public void renderScene() {
    glViewport(1, 1, width / 2 - 1, height - 1);
    glClearColor(.3f, .3f, .3f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderProgram.use();
    MatrixStack.bind(renderProgram);
    cubeGeometry.bindVertexArray();
    cubeGeometry.draw();
    VertexArray.unbind();
    Program.clear();
  }

  @Override
  protected void onDestroy() {
    RiftTracker.stopListening();
  }

  public static void main(String[] args) {
    System.setProperty("org.lwjgl.opengl.Window.undecorated", "true");
    new RiftMediaExplorer().run();
  }

  @Override
  protected void setupDisplay() {
    // setupDisplay(new Rectangle(100, 100, 800, 600));
    // setupDisplay(new Rectangle(100, 100, 1280, 800));
    Rectangle targetRect = RiftDK1.findRiftRect();
    if (null == targetRect) {
      targetRect = findNonPrimaryRect();
    }
    if (null == targetRect) {
      targetRect = GraphicsEnvironment.getLocalGraphicsEnvironment()
          .getDefaultScreenDevice().getDefaultConfiguration().getBounds();
    }
    setupDisplay(targetRect);
  }
}
