package org.saintandreas;

import java.io.IOException;
import java.io.InputStream;

import com.google.common.base.Charsets;
import com.google.common.io.ByteStreams;

public class Resources {

  private static final ResourceLoader resourceLoader = new ClasspathResourceLoader();

  public static String fromResources(String... ress) {
    StringBuilder sb = new StringBuilder();
    for (String res : ress) {
      sb.append(fromResource(res));
    }
    return sb.toString();
  }

  public static long lastModified(String res) {
    return resourceLoader.getResourceLastModified(res);
  }

  private static String fromResource(String res) {
    try (InputStream is = resourceLoader.getResourceStream(res)) {
      return new String(ByteStreams.toByteArray(is), Charsets.UTF_8);
    } catch (IOException e) {
      throw new RuntimeException(e);
    }
  }
}
