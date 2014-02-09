package org.saintandreas.resources.test;

import static org.junit.Assert.*;

import java.io.IOException;

import org.junit.Test;
import org.saintandreas.resources.ClasspathResourceProvider;

public class ClasspathResourceManagerTest {

  @Test
  public void testFetch() throws IOException {
    ClasspathResourceProvider m = new ClasspathResourceProvider();
    assertEquals("<foo/>", m.getAsString(TestResource.FOO));
  }
}
