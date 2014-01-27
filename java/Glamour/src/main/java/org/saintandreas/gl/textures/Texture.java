package org.saintandreas.gl.textures;

import static org.lwjgl.opengl.GL11.*;

import java.awt.Color;
import java.awt.Graphics;
import java.awt.Transparency;
import java.awt.color.ColorSpace;
import java.awt.image.BufferedImage;
import java.awt.image.ColorModel;
import java.awt.image.ComponentColorModel;
import java.awt.image.DataBuffer;
import java.awt.image.DataBufferByte;
import java.awt.image.Raster;
import java.awt.image.WritableRaster;
import java.io.IOException;
import java.net.URL;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Hashtable;

import javax.imageio.ImageIO;

import org.lwjgl.BufferUtils;
import org.lwjgl.opengl.GL11;

import com.google.common.io.Resources;

public class Texture {
  public final int id;
  private final int target;

  public Texture(int target) {
    id = glGenTextures();
    this.target = target;
  }

  public Texture() {
    this(GL_TEXTURE_2D);
  }

  public void bind() {
    glBindTexture(target, id);
  }

  public void unbind() {
    unbind(target);
  }

  public static void unbind(int target) {
    glBindTexture(target, 0);
  }

  public static Texture loadImage(String resource, int textureType, int loadTarget) throws IOException {
    URL resourceUrl = Resources.getResource(resource);
    return loadImage(resourceUrl, textureType, loadTarget);
  }
  
  public static Texture loadImage(URL url, int textureType, int loadTarget)
      throws IOException {
    BufferedImage image = ImageIO.read(url);
    Texture result = new Texture(textureType);
    result.bind();
    GL11.glTexImage2D(loadTarget, 0, GL11.GL_RGBA8, image.getWidth(),
        image.getHeight(), 0, GL11.GL_RGB, GL11.GL_UNSIGNED_BYTE,
        convertImageData(image));
    result.unbind();
    return result;
  }

  public static Texture loadImage(URL url, int textureType) throws IOException {
    return loadImage(url, textureType, textureType);
  }

  public static Texture loadImage(String resource, int textureType) throws IOException {
    return loadImage(resource, textureType, textureType);
  }
  
  public static Texture loadImage(URL url) throws IOException {
    return loadImage(url, GL_TEXTURE_2D);
  }

  public static Texture loadImage(String resource) throws IOException {
    return loadImage(resource, GL_TEXTURE_2D);
  }

  /**
   * Convert the buffered image to a texture
   */
  private static ByteBuffer convertImageData(BufferedImage bufferedImage) {
    ByteBuffer imageBuffer;
    WritableRaster raster;
    BufferedImage texImage;

    ColorModel glAlphaColorModel = new ComponentColorModel(
        ColorSpace.getInstance(ColorSpace.CS_sRGB), new int[] { 8, 8, 8, 8 },
        true, false, Transparency.TRANSLUCENT, DataBuffer.TYPE_BYTE);
    raster = Raster.createInterleavedRaster(DataBuffer.TYPE_BYTE,
        bufferedImage.getWidth(), bufferedImage.getHeight(), 4, null);
    texImage = new BufferedImage(glAlphaColorModel, raster, true,
        new Hashtable<>());

    // copy the source image into the produced image
    Graphics g = texImage.getGraphics();
    g.setColor(new Color(0f, 0f, 0f, 0f));
    g.fillRect(0, 0, bufferedImage.getWidth(), bufferedImage.getHeight());
    g.drawImage(bufferedImage, 0, 0, null);

    // build a byte buffer from the temporary image
    // that be used by OpenGL to produce a texture.
    byte[] data = ((DataBufferByte) texImage.getRaster().getDataBuffer())
        .getData();

    BufferUtils.createByteBuffer(data.length);
    imageBuffer = ByteBuffer.allocateDirect(data.length);
    imageBuffer.order(ByteOrder.nativeOrder());
    imageBuffer.put(data, 0, data.length);
    imageBuffer.flip();
    return imageBuffer;
  }

}
