package org.saintandreas.vr.oculus;

import java.awt.GraphicsDevice;
import java.awt.GraphicsEnvironment;
import java.awt.Rectangle;

import org.saintandreas.math.Vector2f;
import org.saintandreas.math.Vector4f;
import org.saintandreas.vr.DistortionFunction;

public class RiftDK1 implements DistortionFunction  {

  public static final Vector2f Resolution = new Vector2f(1280f, 800f);
  public static final float Aspect = (Resolution.getX() / Resolution.getY());
  public static final float EyeAspect = Aspect / 2.0f;

  public static final Vector2f Size = new Vector2f(0.14976f, 0.09360f);
  public static final float EyeToScreenDistance = 0.04100f;
  public static final float LensSeparationDistance = 0.06350f;
  public static final Vector4f DistortK = new Vector4f(0.583224535f, 0.128309399f, 0.139973879f, 0);
  public static final Vector4f ChromaK = new Vector4f(0.99600f, -0.00400f, 1.01400f, 0);
  public static final float LensOffset;
  static {
    float lensDistance = LensSeparationDistance / Size.getX();
    LensOffset = 1.0f - (2.0f * lensDistance);
  }

  static double getUndistortionScaleForRadiusSquared(double rSq) {
    double factor = (DistortK.x + DistortK.y * rSq + DistortK.z * rSq * rSq + DistortK.w * rSq * rSq * rSq);
    return factor;
  }

  static double getUndistortionScaleForRadius(double r) {
    return getUndistortionScaleForRadiusSquared(r * r);
  }

  double getDistortionScaleForRadius(double rTarget) {
    double max = rTarget * 2;
    double min = 0;
    double distortionScale;
    while (true) {
      double rSource = ((max - min) / 2.0) + min;
      distortionScale = getUndistortionScaleForRadiusSquared(
          rSource * rSource);
      double rResult = distortionScale * rSource;
      if (closeEnough(rResult, rTarget)) {
        break;
      }
      if (rResult < rTarget) {
        min = rSource;
      } else {
        max = rSource;
      }
    }
    return 1.0 / distortionScale;
  }

  public static Rectangle findRiftRect() {
    GraphicsEnvironment ge = GraphicsEnvironment.getLocalGraphicsEnvironment();
    GraphicsDevice[] gs = ge.getScreenDevices();
    for (int j = 0; j < gs.length; j++) {
      GraphicsDevice gd = gs[j];
      Rectangle r = gd.getDefaultConfiguration().getBounds();
      // FIXME super weak Rift detection.
      if (r.width == 1280 && r.height == 800) {
        return r;
      }
    }
    return null;
  }

  private static Vector2f screenToTexture(Vector2f  v) {
    return v.add(1).divide(2);
  }

  private static Vector2f textureToScreen(Vector2f  v) {
    return v.mult(2).subtract(1);
  }

  private static Vector2f riftToScreen(Vector2f  v) {
    return new Vector2f(v.x + LensOffset, v.y * EyeAspect);
  }

  private static Vector2f screenToRift(Vector2f  v) {
    return new Vector2f(v.x - LensOffset, v.y / EyeAspect);
  }

  static boolean closeEnough(double a, double b, double epsilon) {
    return Math.abs(a - b) < epsilon;
  }

  private static boolean closeEnough(double a, double b) {
    return closeEnough(a, b, 1e-5);
  }

  @Override
  public Vector2f getUndistortedTextureCoordinates(Vector2f texCoords) {
    Vector2f screenCoords = textureToScreen(texCoords);
    Vector2f riftCoords = screenToRift(screenCoords); 
    float rSq = riftCoords.lengthSquared();
    float scale = (float)getUndistortionScaleForRadiusSquared(rSq);
    Vector2f resultRiftCoords = riftCoords.mult(scale);
    Vector2f resultScreenCoords = riftToScreen(resultRiftCoords);
    Vector2f resultTexCoords = screenToTexture(resultScreenCoords);
    return resultTexCoords;
  }

  @Override
  public Vector2f getDistortedMeshCoordinates(Vector2f screenCoords) {
    Vector2f rift = screenToRift(screenCoords);
    double rTarget = rift.length();
    double distortionScale = getDistortionScaleForRadius(rTarget);
    Vector2f distortedRift = rift.mult((float)distortionScale);
    double undistortionScale = getUndistortionScaleForRadiusSquared(distortedRift.lengthSquared());
    double combinedScale = undistortionScale * distortionScale;  
    assert(Math.abs(combinedScale - 1.0) < 0.0001);
    Vector2f distortedScreen = riftToScreen(distortedRift);
    return distortedScreen;
  }
}
