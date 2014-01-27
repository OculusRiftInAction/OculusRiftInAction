package org.saintandreas.gl;

import static org.lwjgl.opengl.GL11.*;

import java.nio.FloatBuffer;
import java.nio.ShortBuffer;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

import org.lwjgl.BufferUtils;
import org.lwjgl.util.vector.Matrix4f;
import org.lwjgl.util.vector.ReadableVector3f;
import org.lwjgl.util.vector.ReadableVector4f;
import org.lwjgl.util.vector.Vector2f;
import org.lwjgl.util.vector.Vector3f;
import org.lwjgl.util.vector.Vector4f;
import org.saintandreas.gl.buffers.IndexBuffer;
import org.saintandreas.gl.buffers.VertexBuffer;
import org.saintandreas.gl.shaders.Attribute;

import com.google.common.collect.Lists;

public final class OpenGL {

  private OpenGL() {
  }

  public static void checkError() {
    int error = glGetError();
    if (error != 0) {
      throw new IllegalStateException("GL error " + error);
    }
  }

  public static FloatBuffer toFloatBuffer(Matrix4f matrix) {
    FloatBuffer buffer = BufferUtils.createFloatBuffer(16);
    matrix.store(buffer);
    buffer.position(0);
    return buffer;
  }

  public static List<Vector4f> makeQuad(float size) {
    return makeQuad(new Vector2f(size, size));
  }

  public static List<Vector4f> makeQuad(Vector2f size) {
    Vector2f max = new Vector2f(size);
    max.scale(0.5f);
    Vector2f min = new Vector2f(max);
    min.scale(-1.0f);
    return makeQuad(min, max);
  }

  public static List<Vector4f> makeQuad(Vector2f min, Vector2f max) {
    List<Vector4f> result = new ArrayList<>(4);
    result.add(new Vector4f(min.x, min.y, 0, 1));
    result.add(new Vector4f(max.x, min.y, 0, 1));
    result.add(new Vector4f(min.x, max.y, 0, 1));
    result.add(new Vector4f(max.x, max.y, 0, 1));
    return result;
  }

  public static List<Vector4f> transformed(Collection<Vector4f> vs, Matrix4f m) {
    List<Vector4f> result = new ArrayList<>(vs.size());
    for (Vector4f v : vs) {
      result.add(Matrix4f.transform(m, v, new Vector4f()));
    }
    return result;
  }

  public static List<Vector4f> interleaveConstants(
      Collection<? extends ReadableVector4f> vs, ReadableVector4f... attributes) {
    List<Vector4f> result = new ArrayList<>(vs.size() * (attributes.length + 1));
    for (ReadableVector4f v : vs) {
      result.add(new Vector4f(v));
      for (ReadableVector4f a : attributes) {
        result.add(new Vector4f(a));
      }
    }
    return result;
  }

  public final static ReadableVector3f V3_X = new Vector3f(1, 0, 0);
  public final static ReadableVector3f V3_Y = new Vector3f(0, 1, 0);
  public final static ReadableVector3f V3_Z = new Vector3f(0, 0, 1);
  public final static ReadableVector3f V3_ONE = new Vector3f(1, 1, 1);
  public final static ReadableVector3f V3_ZERO = new Vector3f(0, 0, 0);

  public final static ReadableVector4f C_W = new Vector4f(1, 1, 1, 1);
  public final static ReadableVector4f C_R = new Vector4f(1, 0, 0, 1);
  public final static ReadableVector4f C_G = new Vector4f(0, 1, 0, 1);
  public final static ReadableVector4f C_B = new Vector4f(0, 0, 1, 1);
  public final static ReadableVector4f C_C = new Vector4f(0, 1, 1, 1);
  public final static ReadableVector4f C_Y = new Vector4f(1, 1, 0, 1);
  public final static ReadableVector4f C_M = new Vector4f(1, 0, 1, 1);
  public final static ReadableVector4f C_K = new Vector4f(0, 0, 0, 1);

  public static IndexedGeometry COLOR_CUBE = null;
  private static final float TAU = (float) Math.PI * 2.0f;

  public static IndexedGeometry makeColorCube() {
    if (null == COLOR_CUBE) {
      VertexBuffer vertices = toVertexBuffer(makeColorCubeVertices());
      IndexBuffer indices = toIndexBuffer(makeColorCubeIndices());
      IndexedGeometry.Builder builder = new IndexedGeometry.Builder(indices,
          vertices, 6 * 4);
      builder.withDrawType(GL_TRIANGLE_STRIP).withAttribute(Attribute.POSITION)
          .withAttribute(Attribute.COLOR);
      COLOR_CUBE = builder.build();
    }
    return COLOR_CUBE;
  }

  protected static List<Short> makeColorCubeIndices() {
    List<Short> result = new ArrayList<>();
    short offset = 0;
    for (int i = 0; i < 6; ++i) {
      if (!result.isEmpty()) {
        result.add(Short.MAX_VALUE);
      }
      result.addAll(Lists.newArrayList(Short.valueOf((short) (offset + 0)),
          Short.valueOf((short) (offset + 1)),
          Short.valueOf((short) (offset + 2)),
          Short.valueOf((short) (offset + 3))));
      offset += 4;
    }
    return result;
  }

  protected static List<Vector4f> makeColorCubeVertices() {
    List<Vector4f> result = new ArrayList<>(6 * 4 * 2);
    Matrix4f m;
    List<Vector4f> q = makeQuad(1.0f);
    // Front
    m = new Matrix4f().translate(new Vector3f(0, 0, 0.5f));
    result.addAll(interleaveConstants(transformed(q, m), C_B));

    // Back
    m = new Matrix4f().rotate(TAU / 2f, new Vector3f(V3_X)).translate(
        new Vector3f(0, 0, 0.5f));
    result.addAll(interleaveConstants(transformed(q, m), C_Y));

    // Top
    m = new Matrix4f().rotate(TAU / -4f, new Vector3f(V3_X)).translate(
        new Vector3f(0, 0, 0.5f));
    result.addAll(interleaveConstants(transformed(q, m), C_G));
    // Bottom
    m = new Matrix4f().rotate(TAU / 4f, new Vector3f(V3_X)).translate(
        new Vector3f(0, 0, 0.5f));
    result.addAll(interleaveConstants(transformed(q, m), C_M));

    // Left
    m = new Matrix4f().rotate(TAU / -4f, new Vector3f(V3_Y)).translate(
        new Vector3f(0, 0, 0.5f));
    result.addAll(interleaveConstants(transformed(q, m), C_R));

    // Right
    m = new Matrix4f().rotate(TAU / 4f, new Vector3f(V3_Y)).translate(
        new Vector3f(0, 0, 0.5f));
    result.addAll(interleaveConstants(transformed(q, m), C_C));
    return result;
  }

  public static List<Vector4f> makeTexturedQuad(Vector2f min, Vector2f max) {
    return makeTexturedQuad(min, max, new Vector2f(0, 0), new Vector2f(1, 1));
  }

  public static VertexBuffer toVertexBuffer(Collection<Vector4f> vertices) {
    FloatBuffer fb = BufferUtils.createFloatBuffer(vertices.size() * 4);
    for (Vector4f v : vertices) {
      v.store(fb);
    }
    fb.position(0);
    VertexBuffer result = new VertexBuffer();
    result.bind();
    result.setData(fb);
    result.unbind();
    return result;
  }

  public static IndexBuffer toIndexBuffer(Collection<Short> vertices) {
    ShortBuffer fb = BufferUtils.createShortBuffer(vertices.size());
    for (Short v : vertices) {
      fb.put(v);
    }
    fb.position(0);
    IndexBuffer result = new IndexBuffer();
    result.bind();
    result.setData(fb);
    result.unbind();
    return result;
  }

  public static List<Vector4f> makeTexturedQuad(Vector2f min, Vector2f max,
      Vector2f tmin, Vector2f tmax) {
    List<Vector4f> result = new ArrayList<>();
    result.add(new Vector4f(min.x, min.y, 0, 1));
    result.add(new Vector4f(tmin.x, tmin.y, 0, 0));
    result.add(new Vector4f(max.x, min.y, 0, 1));
    result.add(new Vector4f(tmax.x, tmin.y, 0, 0));
    result.add(new Vector4f(min.x, max.y, 0, 1));
    result.add(new Vector4f(tmin.x, tmax.y, 0, 0));
    result.add(new Vector4f(max.x, max.y, 0, 1));
    result.add(new Vector4f(tmax.x, tmax.y, 0, 0));
    return result;
  }

  public static Matrix4f orthographic(float left, float right, float bottom,
      float top, float near, float far) {
    Matrix4f m = new Matrix4f();
    float x_orth = 2 / (right - left);
    float y_orth = 2 / (top - bottom);
    float z_orth = -2 / (far - near);

    float tx = -(right + left) / (right - left);
    float ty = -(top + bottom) / (top - bottom);
    float tz = -(far + near) / (far - near);

    m.m00 = x_orth;
    m.m10 = 0;
    m.m20 = 0;
    m.m30 = 0;
    m.m01 = 0;
    m.m11 = y_orth;
    m.m21 = 0;
    m.m31 = 0;
    m.m02 = 0;
    m.m12 = 0;
    m.m22 = z_orth;
    m.m32 = 0;
    m.m03 = tx;
    m.m13 = ty;
    m.m23 = tz;
    m.m33 = 1;
    return m;
  }

  public static Matrix4f lookat(Vector3f eye, Vector3f center, Vector3f up) {
    Vector3f f = Vector3f.sub(center, eye, new Vector3f());
    f.normalise();
    Vector3f u = new Vector3f(up);
    u.normalise();
    Vector3f s = Vector3f.cross(f, u, new Vector3f());
    s.normalise();
    u = Vector3f.cross(s, f, u);

    Matrix4f result = new Matrix4f();
    result.m00 = s.x;
    result.m10 = s.y;
    result.m20 = s.z;
    result.m01 = u.x;
    result.m11 = u.y;
    result.m21 = u.z;
    result.m02 = -f.x;
    result.m12 = -f.y;
    result.m22 = -f.z;
    return result.translate(new Vector3f(-eye.x, -eye.y, -eye.z));
  }

  public static Matrix4f perspective(float fovy, float aspect, float zNear,
      float zFar) {
    float radians = fovy / 2f * (float) Math.PI / 180f;
    float sine = (float) Math.sin(radians);
    float cotangent = (float) Math.cos(radians) / sine;
    float deltaZ = zFar - zNear;

    if ((deltaZ == 0) || (sine == 0) || (aspect == 0)) {
      throw new IllegalStateException();
    }

    Matrix4f result = new Matrix4f();
    result.m00 = cotangent / aspect;
    result.m11 = cotangent;
    result.m22 = -(zFar + zNear) / deltaZ;
    result.m23 = -1;
    result.m32 = -2 * zNear * zFar / deltaZ;
    result.m33 = 0;
    return result;
  }

}
