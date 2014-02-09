package org.saintandreas.xbmc;

import java.io.IOException;

import org.apache.http.HttpEntity;
import org.apache.http.client.methods.CloseableHttpResponse;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.client.methods.HttpRequestBase;
import org.apache.http.client.utils.URIBuilder;
import org.apache.http.entity.StringEntity;
import org.apache.http.impl.client.CloseableHttpClient;
import org.apache.http.impl.client.HttpClients;
import org.apache.http.impl.conn.PoolingHttpClientConnectionManager;
import org.apache.http.util.EntityUtils;
import org.codehaus.jackson.map.ObjectMapper;
import org.codehaus.jackson.map.annotate.JsonSerialize.Inclusion;
import org.saintandreas.json.rpc.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.base.Charsets;
import com.google.common.io.ByteStreams;
import com.google.common.net.HttpHeaders;

public class RpcClient {
  @SuppressWarnings("unused")
  private static final Logger LOG = LoggerFactory.getLogger(RpcClient.class);

  public static final String URL = "http://192.168.0.4:8080/jsonrpc"; // ?request={%22jsonrpc%22:%222.0%22,%22id%22:1,%22method%22:%22Files.GetDirectory%22,%22params%22:{%22directory%22:%22Movies%22,%22media%22:%22video%22}}\";
  public static final String REQUEST = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"Files.GetDirectory\",\"params\":{\"directory\":\"Movies\",\"media\":\"video\"}}";
  public static final URIBuilder URI_BUILDER = new URIBuilder()
      .setScheme("http").setHost("192.168.0.4").setPort(8080)
      .setPath("/jsonrpc");

  public final ObjectMapper mapper = new ObjectMapper();
  private final PoolingHttpClientConnectionManager cm = new PoolingHttpClientConnectionManager();

  public RpcClient() {
    mapper.setSerializationInclusion(Inclusion.NON_NULL);
    cm.setMaxTotal(10);
    cm.setDefaultMaxPerRoute(10);
  }

  protected CloseableHttpClient getClient() {
    return HttpClients.custom().setConnectionManager(cm).build();
  }

  public HttpRequestBase getRequest(Object req) {
    try {
      String reqstr = mapper.writeValueAsString(Request.wrapRequest(req));
      // String requrl = URI_BUILDER.clearParameters().setParameter("request",
      // reqstr).build().toString();
      // HttpGet httpReq = new HttpGet(requrl);
      HttpPost httpReq = new HttpPost(URI_BUILDER.build().toString());
      httpReq.setEntity(new StringEntity(reqstr));
      httpReq.setHeader(HttpHeaders.CONTENT_TYPE, "application/json");
      return httpReq;
    } catch (RuntimeException e) {
      throw e;
    } catch (Exception e) {
      throw new RuntimeException(e);
    }
  }

  public String getResponse(Object request) throws IOException {
    CloseableHttpClient client = getClient();
    try (CloseableHttpResponse response = client.execute(getRequest(request))) {
      HttpEntity entity = response.getEntity();
      String responseString = new String(ByteStreams.toByteArray(entity
          .getContent()), Charsets.UTF_8);
      EntityUtils.consume(entity);
      responseString = formatJson(responseString);
      return responseString;
    }
  }

  protected String formatJson(Object in) throws IOException {
    return mapper.writerWithDefaultPrettyPrinter().writeValueAsString(in);
  }

  protected String formatJson(String in) throws IOException {
    return formatJson(mapper.readTree(in));
  }



}
