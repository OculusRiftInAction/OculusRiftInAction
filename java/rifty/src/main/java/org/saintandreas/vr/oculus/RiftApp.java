package org.saintandreas.vr.oculus;

import static org.lwjgl.opengl.GL11.*;
import static org.lwjgl.opengl.GL13.*;

import java.awt.GraphicsEnvironment;
import java.awt.Rectangle;
import java.io.IOException;

import org.saintandreas.ExampleResource;
import org.saintandreas.gl.FrameBuffer;
import org.saintandreas.gl.IndexedGeometry;
import org.saintandreas.gl.MatrixStack;
import org.saintandreas.gl.OpenGL;
import org.saintandreas.gl.app.LwjglApp;
import org.saintandreas.gl.buffers.IndexBuffer;
import org.saintandreas.gl.buffers.VertexArray;
import org.saintandreas.gl.shaders.Program;
import org.saintandreas.gl.textures.Texture;
import org.saintandreas.vr.DistortionHelper;
import org.saintandreas.vr.Eye;

import com.oculusvr.rift.RiftTracker;
import com.oculusvr.rift.fusion.SensorFusion;

public abstract class RiftApp extends LwjglApp {
  private Texture lookupTextures[] = new Texture[2];
  private Program distortProgram;
  private IndexedGeometry distortionQuad;
  private FrameBuffer frameBuffer;
  private float eyeAspect;
  protected SensorFusion sensorFusion;

  @Override
  protected void initGl() {
    super.initGl();
    glClearColor(.3f, .3f, .3f, 0.0f);
    distortionQuad = OpenGL.makeTexturedQuad();

    DistortionHelper helper = new DistortionHelper(new RiftDK1());
    lookupTextures[0] = helper.createLookupTexture(128, 128, Eye.LEFT);
    lookupTextures[1] = helper.createLookupTexture(128, 128, Eye.RIGHT);
    distortProgram = new Program(ExampleResource.SHADERS_TEXTURED_VS,
        ExampleResource.SHADERS_RIFTWARP_FS);
    distortProgram.link();
    distortProgram.use();
    distortProgram.setUniform("OffsetMap", 0);
    distortProgram.setUniform("Scene", 1);

    frameBuffer = new FrameBuffer(width / 2, height);
    frameBuffer.getTexture().bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    frameBuffer.getTexture().unbind();
    sensorFusion = new SensorFusion();
    sensorFusion.setMotionTrackingEnabled(true);
    try {
      RiftTracker.startListening(sensorFusion.createHandler());
    } catch (IOException e) {
      throw new RuntimeException(e);
    }
  }

  @Override
  protected final void onResize(int width, int height) {
    super.onResize(width, height);
    eyeAspect = aspect / 2.0f;
    MatrixStack.PROJECTION.perspective(80f, eyeAspect, 0.01f, 1000.0f);
  }

  @Override
  public final void drawFrame() {
    glViewport(0, 0, width, height);
    glClearColor(0, 1, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    for (int i = 0; i < 2; ++i) {
      float offset = RiftDK1.LensOffset;
      float ipd = 0.06400f;
      float mvo = ipd / 2.0f;
      if (i == 1) {
        offset *= -1f;
        mvo *= -1f;
      }
      glEnable(GL_DEPTH_TEST);
      MatrixStack.PROJECTION.push().preTranslate(offset);
      MatrixStack.MODELVIEW.push().preTranslate(mvo);
      frameBuffer.activate();
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      renderScene();
      frameBuffer.deactivate();
      MatrixStack.MODELVIEW.pop();
      MatrixStack.PROJECTION.pop();

      glDisable(GL_DEPTH_TEST);
      int x = i == 0 ? 0 : (width / 2);
      glViewport(x, 0, width / 2, height);

      distortProgram.use();
      distortProgram.setUniform("OffsetMap", 0);
      distortProgram.setUniform("Scene", 1);
      glActiveTexture(GL_TEXTURE1);
      frameBuffer.getTexture().bind();
      glActiveTexture(GL_TEXTURE0);
      lookupTextures[i].bind();

      distortionQuad.bindVertexArray();
      distortionQuad.ibo.bind();
      distortionQuad.draw();

      VertexArray.unbind();
      IndexBuffer.unbind();
      Program.clear();
    }
  }

  @Override
  protected void onDestroy() {
    RiftTracker.stopListening();
  }

  @Override
  protected void setupDisplay() {
    System.setProperty("org.lwjgl.opengl.Window.undecorated", "true");
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

  protected abstract void renderScene();
}
