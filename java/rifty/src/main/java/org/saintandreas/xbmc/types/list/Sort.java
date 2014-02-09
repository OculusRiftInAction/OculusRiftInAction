package org.saintandreas.xbmc.types.list;

public class Sort {
  public enum Order { 
    Ascending, Descending
  };
  public enum Method {
    None
  }
  public Order order = Order.Ascending;
  public boolean ignorearticle = false;
  public Method method = Method.None;
}
