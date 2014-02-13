package org.saintandreas.vr;

import static org.lwjgl.opengl.GL11.*;
import static org.lwjgl.opengl.GL14.*;
import static org.lwjgl.opengl.GL30.*;

import java.nio.ByteBuffer;
import java.nio.FloatBuffer;
import java.util.ArrayList;
import java.util.List;

import org.lwjgl.BufferUtils;
import org.saintandreas.gl.IndexedGeometry;
import org.saintandreas.gl.OpenGL;
import org.saintandreas.gl.buffers.IndexBuffer;
import org.saintandreas.gl.buffers.VertexBuffer;
import org.saintandreas.gl.shaders.Attribute;
import org.saintandreas.gl.textures.Texture;
import org.saintandreas.math.Vector2f;
import org.saintandreas.math.Vector4f;

/**
 * A helper class for implementing various methods of distortion.  
 * Relies on an implementation of DistortionFunction which knows
 * how to do (or delegate) the exact transformation of coordinates
 *  
 * @author bdavis
 *
 */
public class DistortionHelper {
  private DistortionFunction function;

  private Vector2f getTextureLookupValue(Vector2f texCoord, Eye eyeIndex) {
    if (eyeIndex != Eye.LEFT) {
      texCoord = new Vector2f(1.0f - texCoord.x, texCoord.y);
    }

    Vector2f result = function.getUndistortedTextureCoordinates(texCoord);

    if (eyeIndex != Eye.LEFT) {
      result = new Vector2f(1.0f - result.x, result.y);
    }
    return result;
  }

  public DistortionHelper(DistortionFunction function) {
    this.function = function;
  }

  public Texture createLookupTexture(int xres, int yres, Eye eyeIndex) {
    int size = yres * yres * 2;
    ByteBuffer buffer = BufferUtils.createByteBuffer(4 * size);
    FloatBuffer fbuffer = buffer.asFloatBuffer();
    fbuffer.position(0);
    Vector2f texSize = new Vector2f(xres, yres);
    // The texture coordinates are actually from the center of the pixel, so
    // thats what we need to use for the calculation.
    Vector2f texCenterOffset = new Vector2f(0.5f).divide(new Vector2f(xres,
        yres));
    for (int y = 0; y < yres; ++y) {
      for (int x = 0; x < xres; ++x) {
        Vector2f texCoord = new Vector2f(x, y).divide(texSize).add(
            texCenterOffset);
        Vector2f undistortedTexCoord = getTextureLookupValue(texCoord, eyeIndex);
        fbuffer.put(undistortedTexCoord.x);
        fbuffer.put(undistortedTexCoord.y);
      }
    }
    buffer.position(0);

    Texture outTexture = new Texture();
    outTexture.bind();
    outTexture.image2d(GL_RG16F, xres, yres, GL_RG, GL_FLOAT, buffer);
    outTexture.parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    outTexture.parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    outTexture.parameter(GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    outTexture.parameter(GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    return outTexture;
  }

  public IndexedGeometry createDistortionMesh(int xres, int yres, Eye eyeIndex) {
    List<Vector4f> vertices = new ArrayList<>();
    Vector2f meshSizeInverse = new Vector2f(xres - 1, yres - 1).inverse();
    for (int y = 0; y < yres; ++y) {
      for (int x = 0; x < xres; ++x) {
        Vector2f texture = new Vector2f(x, y).mult(meshSizeInverse);
        Vector2f screen = texture.mult(2).add(new Vector2f(-1, -1));
        screen = function.getDistortedMeshCoordinates(screen);
        if (eyeIndex != Eye.LEFT) {
          screen = new Vector2f(-screen.x, screen.y);
        }
        vertices.add(new Vector4f(screen.x, screen.y, 0, 1));
        vertices.add(new Vector4f(texture.x, texture.y, 0, 0));
        vertices.add(new Vector4f(texture.x, texture.y, 0, 1));
      }
    }
    List<Short> indices = new ArrayList<>();
    for (int y = 0; y < yres - 1; ++y) {
      int start = y * xres;
      int nextRowStart = start + xres;
      for (int x = 0; x < xres; ++x) {
        indices.add((short) (nextRowStart + x));
        indices.add((short) (start + x));
      }
      indices.add(Short.MAX_VALUE);
    }
    OpenGL.checkError();
    VertexBuffer vbo = OpenGL.toVertexBuffer(vertices);
    IndexBuffer ibo = OpenGL.toIndexBuffer(indices);
    OpenGL.checkError();
    IndexedGeometry.Builder b = new IndexedGeometry.Builder(ibo, vbo,
        indices.size());
    b.withDrawType(GL_TRIANGLE_STRIP);
    b.withAttribute(Attribute.POSITION);
    b.withAttribute(Attribute.TEX);
    b.withAttribute(Attribute.COLOR);
    OpenGL.checkError();
    IndexedGeometry result = b.build();
    OpenGL.checkError();
    return result;
  }
};
