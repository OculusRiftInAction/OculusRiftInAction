package org.saintandreas.xbmc.types.videolibrary;

import java.util.ArrayList;
import java.util.List;

public class Movie extends File {
  public int movieid;
  public String votes;
  public List<String> showlink = new ArrayList<>();
  public List<String> country = new ArrayList<>();
  public List<String> studio = new ArrayList<>();
  public List<String> genre = new ArrayList<>();
  public List<String> tag = new ArrayList<>();
  public List<String> writer = new ArrayList<>();
  public List<Cast> cast = new ArrayList<>();
  public String plotoutline;
  public String sorttitle;
  public int top250 = 0;
  public String trailer;
  public int year = 0;
  public String set;
  public String mpaa;
  public int setid = -1;
  public float rating;
  public String tagline = "";
  public String originaltitle;
  public String imdbnumber;
}
