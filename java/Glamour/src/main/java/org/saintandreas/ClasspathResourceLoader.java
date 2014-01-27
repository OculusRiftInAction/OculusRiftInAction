package org.saintandreas;

import static java.lang.ClassLoader.getSystemClassLoader;

import java.io.IOException;
import java.io.InputStream;
import java.net.URLClassLoader;

import com.google.common.base.Joiner;

public class ClasspathResourceLoader implements ResourceLoader {

  @Override
  public long getResourceLastModified(String path) {
    return Long.MAX_VALUE;
  }

  @Override
  public InputStream getResourceStream(String path) throws IOException {
    InputStream is = ClasspathResourceLoader.class.getResourceAsStream("/" + path);
    if (is == null) {
      throw new IOException("Resource '" + path + "' not found in classpath { "
          + Joiner.on(", ").join(((URLClassLoader) getSystemClassLoader()).getURLs())
          + " }");
    } else {
      return is;
    }
  }

}
