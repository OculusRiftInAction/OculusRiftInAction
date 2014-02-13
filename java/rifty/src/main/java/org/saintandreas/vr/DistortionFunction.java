package org.saintandreas.vr;

import org.saintandreas.math.Vector2f;

public interface DistortionFunction {
  public abstract Vector2f getUndistortedTextureCoordinates(Vector2f v);
  public abstract Vector2f getDistortedMeshCoordinates(Vector2f v);
}
