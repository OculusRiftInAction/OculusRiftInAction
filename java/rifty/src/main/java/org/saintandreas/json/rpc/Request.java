package org.saintandreas.json.rpc;


public class Request {
  // public static final String REQUEST = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"Files.GetDirectory\",\"params\":{\"directory\":\"Movies\",\"media\":\"video\"}}";
  public final String jsonrpc = "2.0";
  public int id = 1;
  public String method;
  public Object params;

  public static Request wrapRequest(Object object) {
    String methodName = null;
    
    Method m = object.getClass().getAnnotation(Method.class);
    if (m != null) {
      methodName = m.value();
    }
    
    if (methodName == null) {
      Class<?> clazz = object.getClass();
      Namespace namespace = clazz.getPackage().getAnnotation(Namespace.class);
      if (null != namespace) {
        methodName = namespace.value() + "." + clazz.getSimpleName(); 
      }
    }
    return wrapRequest(methodName, object);
  }

  public static Request wrapRequest(String method, Object object) {
    Request r = new Request();
    r.params = object;
    r.method = method;
    return r;
  }
}
