package org.saintandreas;

import static org.lwjgl.opengl.GL11.*;

import org.saintandreas.gl.IndexedGeometry;
import org.saintandreas.gl.MatrixStack;
import org.saintandreas.gl.OpenGL;
import org.saintandreas.gl.buffers.VertexArray;
import org.saintandreas.gl.shaders.Program;
import org.saintandreas.math.Matrix4f;
import org.saintandreas.math.Quaternion;
import org.saintandreas.math.Vector3f;
import org.saintandreas.vr.oculus.RiftApp;
import org.saintandreas.vr.oculus.RiftDK1;

public class RiftDemo extends RiftApp {
  IndexedGeometry cubeGeometry;
  Program renderProgram;

  @Override
  protected void initGl() {
    super.initGl();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
//    glEnable(GL_POLYGON_SMOOTH);
//    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    renderProgram = new Program(ExampleResource.SHADERS_COLORED_VS,
        ExampleResource.SHADERS_COLORED_FS);
    renderProgram.link();
    cubeGeometry = OpenGL.makeColorCube();
  }

  @Override
  protected void update() {
    Quaternion q = sensorFusion.getPredictedOrientation();
    Matrix4f m = new Matrix4f().rotate(q);
    MatrixStack.MODELVIEW.lookat(new Vector3f(0, 0, 0.5f), Vector3f.ZERO,
        Vector3f.UNIT_Y).multiply(m.invert()); // preMultiply(m);
  }

  @Override
  public void renderScene() {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f, 0.2f, 0.2f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderProgram.use();
    MatrixStack.bindProjection(renderProgram);
    MatrixStack.MODELVIEW.push().scale(RiftDK1.LensSeparationDistance)
        .bind(renderProgram).pop();
    cubeGeometry.bindVertexArray();
    cubeGeometry.draw();
    VertexArray.unbind();
    Program.clear();
  }

  public static void main(String[] args) {
    new RiftDemo().run();
  }
}
