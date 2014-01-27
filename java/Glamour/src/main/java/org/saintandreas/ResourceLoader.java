package org.saintandreas;

import java.io.IOException;
import java.io.InputStream;

public interface ResourceLoader {

  InputStream getResourceStream(String path) throws IOException;

  long getResourceLastModified(String path);
}
