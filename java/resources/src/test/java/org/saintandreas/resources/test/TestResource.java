package org.saintandreas.resources.test;

import org.saintandreas.resources.Resource;

public enum TestResource implements Resource {
  FOO("/foo.xml");

  private String path;

  TestResource(String path) {
    this.path = path;
  }

  @Override
  public String getPath() {
    return path;
  }

}
