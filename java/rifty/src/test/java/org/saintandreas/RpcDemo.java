package org.saintandreas;

import java.io.IOException;
import java.lang.reflect.Field;
import java.util.Arrays;
import java.util.List;

import org.codehaus.jackson.JsonNode;
import org.saintandreas.xbmc.RpcClient;
import org.saintandreas.xbmc.types.videolibrary.GetMovieDetails;
import org.saintandreas.xbmc.types.videolibrary.GetMovies;
import org.saintandreas.xbmc.types.videolibrary.Movie;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.base.Function;
import com.google.common.collect.Lists;

public class RpcDemo {

  private static final Logger LOG = LoggerFactory.getLogger(RpcClient.class);

  public static List<String> getProperties(Class<?> clazz) {
    return Lists.transform(Arrays.asList(clazz.getDeclaredFields()), new Function<Field, String>() {
      @Override
      public String apply(Field input) {
        return input.getName();
      }
    });
  }
  @SuppressWarnings("unused")
  public static void main(String[] args) throws IOException {
    RpcClient client = new RpcClient();
    String response = client.getResponse(new GetMovies());
    LOG.warn(response);
    GetMovieDetails movieDetails = new GetMovieDetails();
    movieDetails.properties.addAll(getProperties(Movie.class));
    movieDetails.properties.remove("movieid");
    movieDetails.movieid = 83;
    response = client.getResponse(movieDetails);
    JsonNode node = client.mapper.readTree(response);
    node = node.get("result").get("moviedetails");
    LOG.warn(response);
    Movie movie = client.mapper.readValue(node, Movie.class);
    LOG.warn("zzz");
  }
}
