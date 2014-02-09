package org.saintandreas.xbmc.types.videolibrary;

import java.util.ArrayList;
import java.util.List;

public class File extends Item {
  public Streams streamdetails;
  public List<String> director = new ArrayList<String>();
  public Resume resume;
  public int runtime = 0;
}
