package org.saintandreas.gl;

import java.util.Stack;

import org.lwjgl.util.vector.Matrix4f;
import org.lwjgl.util.vector.Vector2f;
import org.lwjgl.util.vector.Vector3f;
import org.saintandreas.gl.shaders.Program;

@SuppressWarnings("serial")
public class MatrixStack extends Stack<Matrix4f> {
  public static final MatrixStack MODELVIEW = new MatrixStack();
  public static final MatrixStack PROJECTION = new MatrixStack();

  MatrixStack() {
    push(new Matrix4f());
  }

  public MatrixStack push() {
    push(new Matrix4f(peek()));
    return this;
  }

  public MatrixStack transpose() {
    peek().transpose();
    return this;
  }

  public MatrixStack translate(Vector2f vec) {
    peek().translate(vec);
    return this;
  }

  public MatrixStack translate(Vector3f vec) {
    peek().translate(vec);
    return this;
  }

  public MatrixStack scale(Vector3f vec) {
    peek().scale(vec);
    return this;
  }

  public MatrixStack rotate(float angle, Vector3f axis) {
    peek().rotate(angle, axis);
    return this;
  }

  public MatrixStack orthographic(float left, float right, float bottom,
      float top, float near, float far) {
    peek().load(OpenGL.orthographic(left, right, bottom, top, near, far));
    return this;
  }

  public MatrixStack lookat(Vector3f eye, Vector3f center, Vector3f up) {
    peek().load(OpenGL.lookat(eye, center, up));
    return this;
  }

  public MatrixStack perspective(float fovy, float aspect, float zNear,
      float zFar) {
    peek().load(OpenGL.perspective(fovy, aspect, zNear, zFar));
    return this;
  }

  public static void bindProjection(Program program) {
    program.setUniform("Projection", MatrixStack.PROJECTION.peek());
  }

  public static void bindModelview(Program program) {
    program.setUniform("ModelView", MatrixStack.MODELVIEW.peek());
  }
  
  public static void bind(Program program) {
    bindProjection(program);
    bindModelview(program);
  }
  
  public MatrixStack preMultiply(Matrix4f m) {
    peek().load(Matrix4f.mul(m, peek(), new Matrix4f()));
    return this;
  }

  public MatrixStack preTranslate(float x) {
    return preTranslate(new Vector2f(x, 0));
  }

  public MatrixStack preTranslate(Vector2f v) {
    return preMultiply(new Matrix4f().translate(v));
  }

  public MatrixStack preTranslate(Vector3f v) {
    return preMultiply(new Matrix4f().translate(v));
  }
}
