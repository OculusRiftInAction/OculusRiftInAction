package org.saintandreas.gl;

import static org.lwjgl.opengl.GL11.*;

import org.saintandreas.gl.buffers.IndexBuffer;
import org.saintandreas.gl.buffers.VertexArray;
import org.saintandreas.gl.buffers.VertexBuffer;

public class IndexedGeometry extends Geometry {
  public final IndexBuffer ibo;

  public static class Builder extends Geometry.Builder {
    protected final IndexBuffer ibo;

    public Builder(IndexBuffer ibo, VertexBuffer vbo, int elements) {
      super(vbo, elements);
      this.ibo = ibo;
    }

    @Override
    protected VertexArray buildVertexArray() {
      VertexArray vao = super.buildVertexArray();
      vao.bind();
      ibo.bind();
      VertexArray.unbind();
      ibo.unbind();
      return vao;
    }

    @Override
    public IndexedGeometry build() {
      return new IndexedGeometry(ibo, vbo, buildVertexArray(), drawType, elements);
    }
  }

  public IndexedGeometry(IndexBuffer ibo, VertexBuffer vbo, VertexArray vao,
      int drawType, int elements) {
    super(vbo, vao, drawType, elements);
    this.ibo = ibo;
  }
  
  @Override
  public void bindVertexArray() {
    super.bindVertexArray();
    ibo.bind();
  }

  @Override
  public void draw() {
    glDrawElements(drawType, elements, GL_UNSIGNED_SHORT, 0);
  }
}
