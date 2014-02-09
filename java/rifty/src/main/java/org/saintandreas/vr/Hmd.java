package org.saintandreas.vr;

import static org.lwjgl.opengl.GL11.*;

import java.util.ArrayList;
import java.util.List;

import org.saintandreas.gl.IndexedGeometry;
import org.saintandreas.gl.OpenGL;
import org.saintandreas.gl.buffers.IndexBuffer;
import org.saintandreas.gl.buffers.VertexBuffer;
import org.saintandreas.gl.shaders.Attribute;
import org.saintandreas.math.Vector2f;
import org.saintandreas.math.Vector4f;

public abstract class Hmd {

  public enum Eye {
    LEFT, RIGHT
  }

  public abstract Vector2f getDistorted(Eye eye, Vector2f screen);

  public IndexedGeometry getDistortionMesh(Eye eye, int size) {
    List<Vector4f> vertices = new ArrayList<>();
    for (int y = 0; y < size; ++y) {
      for (int x = 0; x < size; ++x) {
        Vector2f texture = new Vector2f(x, y).mult(1.0f / (float)(size - 1));
        Vector2f screen = texture.mult(2).add(new Vector2f(-1, -1));
        screen = getDistorted(eye, screen);
        vertices.add(new Vector4f(screen.x, screen.y, 0, 1));
        vertices.add(new Vector4f(texture.x, texture.y, 0, 0));
        vertices.add(new Vector4f(texture.x, texture.y, 0, 1));
      }
    }
    List<Short> indices = new ArrayList<>();
    for (int y = 0; y < size - 1; ++y) {
      int start = y * size;
      int nextRowStart = start + size;
      for (int x = 0; x < size; ++x) {
        indices.add((short) (nextRowStart + x));
        indices.add((short) (start + x));
      }
      indices.add(Short.MAX_VALUE);
    }
    OpenGL.checkError();
    VertexBuffer vbo = OpenGL.toVertexBuffer(vertices);
    IndexBuffer ibo = OpenGL.toIndexBuffer(indices);
    OpenGL.checkError();
    IndexedGeometry.Builder b = new IndexedGeometry.Builder(ibo, vbo, indices.size());
    b.withDrawType(GL_TRIANGLE_STRIP);
    b.withAttribute(Attribute.POSITION);
    b.withAttribute(Attribute.TEX);
    b.withAttribute(Attribute.COLOR);
    OpenGL.checkError();
    IndexedGeometry result = b.build();
    OpenGL.checkError();
    return result;
  }

}
