package org.saintandreas;

import org.lwjgl.util.vector.ReadableVector2f;
import org.lwjgl.util.vector.Vector2f;

public class RiftDK1 extends Hmd {

  // private static final float K[] = new float[] { 0.75f, 0.12f, 0.14f, 0.05f
  // };
  public static final float K[] = new float[] { 1.0f, 0.22f, 0.24f, 0.00f };
  // hmdInfo.ChromaAbCorrection[0] = 0.99600f;
  // hmdInfo.ChromaAbCorrection[1] = -0.00400f;
  // hmdInfo.ChromaAbCorrection[2] = 1.01400f;
  // hmdInfo.ChromaAbCorrection[3] = 0;
  public static final ReadableVector2f Resolution = new Vector2f(1280f, 800f);
  public static final float Aspect = (Resolution.getX() / Resolution.getY());

  public static final ReadableVector2f Size = new Vector2f(0.14976f, 0.09360f);
  public static final float EyeToScreenDistance = 0.04100f;
  public static final float LensSeparationDistance = 0.06350f;

  public static final float LensOffset;
  static {
    float lensDistance = LensSeparationDistance / Size.getX();
    LensOffset = 1.0f - (2.0f * lensDistance);
  }

  public static final float PostScale;
  static {
    final float fit = 1f + LensOffset;
    PostScale = getDistortionRaw(fit) / fit;
  }

  static final float EPSILON = 0.0001f;
  static boolean closeEnough(float a, float b) {
    return Math.abs(a - b) < EPSILON;
  }

  // Find r such that getDistortion(r) * r == target
  static float findDistortedRadius(float sourceRadius) {
    // Binary search
    float max = 10f;
    float min = 0;
    float test = 0;
    while (true) {
      float r = ((max - min) / 2f) + min;
      test = getDistortion(r) * r / PostScale;
      if (closeEnough(test, sourceRadius)) {
        return r;
      }
      if (test < sourceRadius) {
        min = r;
      } else {
        max = r;
      }
    }
  }

  static float getDistortionRaw(float r) {
    float rsq = r * r;
    return K[0] + rsq * (K[1] + rsq * (K[2] + rsq * K[3]));
  }

  static float getDistortion(float r) {
    return getDistortionRaw(r);
  }

  public Vector2f getDistorted(Eye eye, Vector2f screen) {
    float xoffset = LensOffset * ((eye == Eye.LEFT) ? -1f : 1f);
    float yscale = 1f / 0.8f;
    // Put the incoming vertex from screen space into
    // Rift space. uniform axis scales and lens offset
    // correction
    // Correct for the per-eye aspect
    Vector2f rift = new Vector2f(screen);
    rift.y *= yscale;
    rift.x += xoffset;
    float length = rift.length();

    float distortion = findDistortedRadius(length);
    Vector2f distorted = new Vector2f(rift);
    if (distorted.lengthSquared() > 0.0f) {
      distorted.normalise();
    }
    distorted.scale(distortion);
    Vector2f restored = new Vector2f(distorted);
    restored.scale(getDistortion(distorted.length()));

    distorted.y /= yscale;
    distorted.x -= xoffset;
    return distorted;
  }

}
