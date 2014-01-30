package org.saintandreas;

import java.awt.GraphicsDevice;
import java.awt.GraphicsEnvironment;
import java.awt.Rectangle;

import org.lwjgl.util.vector.ReadableVector2f;
import org.lwjgl.util.vector.Vector2f;

public class RiftDK1 extends Hmd {
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

  private static double getDistortionForCoefficients(double r, double[] K) {
    double rsq = r * r;
    return K[0] + rsq * (K[1] + rsq * (K[2] + rsq * K[3]));
  }

  public static final double DistortionK[];
  static {
    // The Rift uses input coefficients that shrink the image,
    // and then uses a scalar to re-expand the image to fill
    // the screen.  This two step process needlessly complicates 
    // rendering, so we're going to take the original Rift 
    // coefficients and replace them with pre-scaled ones
    // using the same formula to calculate the scalar that
    // the Oculus SDK
    final double fit = 1f + LensOffset;
    double k[] = new double[] { 1.0f, 0.22f, 0.24f, 0.00f };
    double fitScale = fit / getDistortionForCoefficients(fit, k);
    DistortionK = new double[] { k[0] * fitScale, k[1] * fitScale,
        k[2] * fitScale, k[3] * fitScale };
  }

  static final double EPSILON = 0.0000001;

  static boolean closeEnough(double a, double b) {
    return Math.abs(a - b) < EPSILON;
  }

  // The distortion function actualy calculates the 
  // inverse of what we want.  The correct solution
  // would be to invert the polynomial that is used
  // in the distortion function.  I don't know how to 
  // do that, but I DO know how to execute a binary 
  // search for the value.  This is much more expensive 
  // in terms of computation, but we only go through the
  // process for each point when initially setting up 
  // the per eye meshes.
  static double findDistortedRadius(double targetRadius) {
    // We know the distorted R is always greater than the input.
    // So distorting the input provides us with an easy max value
    // for the binary search
    double max = getRiftDistortion(targetRadius) + 1;
    double min = 0;

    while (true) {
      double sourceRadius = ((max - min) / 2.0) + min;
      double distortedRadius = 
          getRiftDistortion(sourceRadius) * sourceRadius;
      if (closeEnough(distortedRadius, targetRadius)) {
        return sourceRadius;
      }
      if (distortedRadius < targetRadius) {
        min = sourceRadius;
      } else {
        max = sourceRadius;
      }
    }
  }

  static double getRiftDistortion(double r) {
    return getDistortionForCoefficients(r, DistortionK);
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
    double length = rift.length();

    double distortion = findDistortedRadius(length);
    Vector2f distorted = new Vector2f(rift);
    if (distorted.lengthSquared() > 0.0f) {
      distorted.normalise();
    }
    distorted.scale((float)distortion);
    Vector2f restored = new Vector2f(distorted);
    restored.scale((float)getRiftDistortion(distorted.length()));

    distorted.y /= yscale;
    distorted.x -= xoffset;
    return distorted;
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

}
