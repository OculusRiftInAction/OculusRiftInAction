package org.saintandreas.resources.test;

import static org.junit.Assert.*;

import java.io.IOException;

import org.junit.Test;
import org.saintandreas.resources.FilesystemResourceProvider;

public class FilesystemResourceManagerTest {

  @Test
  public void testFetch() throws IOException {
    FilesystemResourceProvider m = new FilesystemResourceProvider("src/test/resources");
    assertEquals("<foo/>", m.getAsString(TestResource.FOO));
  }
}
