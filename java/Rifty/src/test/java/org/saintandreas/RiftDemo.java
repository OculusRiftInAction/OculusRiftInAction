package org.saintandreas;

import static org.lwjgl.opengl.GL11.*;
import static org.lwjgl.opengl.GL20.*;
import static org.lwjgl.opengl.GL31.*;

import java.awt.GraphicsEnvironment;
import java.awt.Rectangle;
import java.io.IOException;
import java.nio.ByteBuffer;

import org.lwjgl.BufferUtils;
import org.lwjgl.input.Keyboard;
import org.lwjgl.util.vector.Matrix4f;
import org.lwjgl.util.vector.Vector2f;
import org.lwjgl.util.vector.Vector3f;
import org.saintandreas.Hmd.Eye;
import org.saintandreas.gl.FrameBuffer;
import org.saintandreas.gl.IndexedGeometry;
import org.saintandreas.gl.MatrixStack;
import org.saintandreas.gl.OpenGL;
import org.saintandreas.gl.app.LwjglApp;
import org.saintandreas.gl.buffers.VertexArray;
import org.saintandreas.gl.shaders.Attribute;
import org.saintandreas.gl.shaders.Program;
import org.saintandreas.gl.textures.Texture;
import org.saintandreas.input.oculus.RiftTracker;
import org.saintandreas.input.oculus.SensorFusion;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import uk.co.caprica.vlcj.binding.LibVlc;
import uk.co.caprica.vlcj.binding.LibVlcFactory;
import uk.co.caprica.vlcj.binding.internal.libvlc_media_t;
import uk.co.caprica.vlcj.player.AudioDevice;
import uk.co.caprica.vlcj.player.AudioOutput;
import uk.co.caprica.vlcj.player.MediaPlayer;
import uk.co.caprica.vlcj.player.MediaPlayerEventListener;
import uk.co.caprica.vlcj.player.MediaPlayerFactory;
import uk.co.caprica.vlcj.player.direct.BufferFormat;
import uk.co.caprica.vlcj.player.direct.BufferFormatCallback;
import uk.co.caprica.vlcj.player.direct.DirectMediaPlayer;
import uk.co.caprica.vlcj.player.direct.RenderCallback;
import uk.co.caprica.vlcj.runtime.RuntimeUtil;

import com.sun.jna.Memory;
import com.sun.jna.Native;
import com.sun.jna.NativeLibrary;

public class RiftDemo extends LwjglApp implements BufferFormatCallback,
    RenderCallback, MediaPlayerEventListener {
  // private static final String MEDIA_URL = "http://192.168.0.4/sc2a.mp4";
  // private static final String MEDIA_URL =
  // "http://192.168.0.4/Videos/South.Park.S17E09.HDTV.x264-ASAP.%5bVTV%5d.mp4";
  //private static final String MEDIA_URL = "http://192.168.0.4/Download/Gravity.2013.1080p%203D.HDTV.x264.DTS-RARBG/Gravity.2013.1080p%203D.HDTV.x264.DTS-RARBG.mkv";
  private static final String MEDIA_URL = "http://192.168.0.4/Videos/3D/TRON%20LEGACY%203D.mkv";
  private static final Logger LOG = LoggerFactory.getLogger(RiftDemo.class);
  IndexedGeometry cubeGeometry;
  IndexedGeometry eyeMeshes[] = new IndexedGeometry[2];
  Program coloredProgram;
  Program textureProgram;
  FrameBuffer frameBuffer;
  SensorFusion sensorFusion;
  Texture videoTexture;
  ByteBuffer videoData;
  MediaPlayer player;
  volatile boolean newFrame = false;
  int videoWidth, videoHeight;
  private float videoAspect;

  public RiftDemo() {
    // Rift applications should have no window decroation
    System.setProperty("org.lwjgl.opengl.Window.undecorated", "true");
    LibVlc libvlc = LibVlcFactory.factory().log().create();
    MediaPlayerFactory playerFactory = new MediaPlayerFactory(libvlc);
    for (AudioOutput output : playerFactory.getAudioOutputs()) {
      LOG.warn("-----------");
      LOG.warn(output.getName());
      LOG.warn(output.getDescription());
      for (AudioDevice device : output.getDevices()) {
        LOG.warn("\t" + device.getDeviceId());
        LOG.warn("\t" + device.getLongName());
        LOG.warn("\t*********");
      }
    }

    player = playerFactory.newDirectMediaPlayer(this, this);
    player.addMediaPlayerEventListener(this);
    player.playMedia(MEDIA_URL);
  }

  @Override
  protected void initGl() {
    super.initGl();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_PRIMITIVE_RESTART);
    videoTexture = new Texture(GL_TEXTURE_2D);
    videoTexture.bind();
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    videoTexture.unbind();
    OpenGL.checkError();
    OpenGL.checkError();
    glPrimitiveRestartIndex(Short.MAX_VALUE);
    MatrixStack.MODELVIEW.lookat(new Vector3f(-2, 1, 3), new Vector3f(0, 0, 0),
        new Vector3f(0, 1, 0));

    frameBuffer = new FrameBuffer(width / 2, height);
    frameBuffer.getTexture().bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    frameBuffer.getTexture().unbind();
    OpenGL.checkError();

    eyeMeshes[0] = new RiftDK1().getDistortionMesh(Eye.LEFT, 128);
    eyeMeshes[1] = new RiftDK1().getDistortionMesh(Eye.RIGHT, 128);

    // glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    coloredProgram = new Program("shaders/SimpleColored.vs",
        "shaders/Simple.fs");
    textureProgram = new Program("shaders/SimpleTextured.vs",
        "shaders/Texture.fs");

    coloredProgram.link();
    textureProgram.link();

    cubeGeometry = OpenGL.makeColorCube();

    sensorFusion = new SensorFusion();
    sensorFusion.setMotionTrackingEnabled(true);
    try {
      RiftTracker.startListening(sensorFusion.createHandler());
    } catch (IOException e) {
      throw new RuntimeException(e);
    }
  }

  @Override
  protected void onResize(int width, int height) {
    super.onResize(width, height);
    MatrixStack.PROJECTION.perspective(80f, aspect / 2.0f, 0.01f, 1000.0f);
  }

  @Override
  protected void update() {
    // get event key here
    while (Keyboard.next()) {
      switch (Keyboard.getEventKey()) {
      case Keyboard.KEY_LEFT:
        player.setTime(player.getTime() - 30 * 1000);
        break;
      case Keyboard.KEY_RIGHT:
        player.setTime(player.getTime() + 30 * 1000);
        break;
      case Keyboard.KEY_UP:
        player.setTime(player.getTime() + 5 * 60 * 1000);
        break;
      case Keyboard.KEY_DOWN:
        player.setTime(player.getTime() - 5 * 60 * 1000);
        break;
      }
    }
  }

  protected void onNewFrame() {
    synchronized (videoData) {
      videoData.position(0);
      videoTexture.bind();
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, videoWidth, videoHeight, 0,
          GL_RGBA, GL_UNSIGNED_BYTE, videoData);
      videoTexture.unbind();
      newFrame = false;
    }
  }

  @Override
  public void drawFrame() {
    OpenGL.checkError();
    glViewport(0, 0, width, height);
    glClearColor(.1f, .1f, .1f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (newFrame) {
      onNewFrame();
    }

    for (int i = 0; i < 2; ++i) {
      float offset = RiftDK1.LensOffset;
      float ipd = 0.06400f;
      float mvo = ipd / 2.0f;
      if (i == 1) {
        offset *= -1f;
        mvo *= -1f;
      }
      new Matrix4f().translate(new Vector2f(offset, 0));
      MatrixStack.PROJECTION.push().preTranslate(offset);
      MatrixStack.MODELVIEW.push().preTranslate(mvo);

      frameBuffer.activate();
      renderScene(offset);
      frameBuffer.deactivate();

      MatrixStack.MODELVIEW.pop();
      MatrixStack.PROJECTION.pop();

      MatrixStack.PROJECTION.push().identity();
      MatrixStack.MODELVIEW.push().identity();
      int x = i == 0 ? 0 : (width / 2);
      glViewport(x, 0, width / 2, height);
      textureProgram.use();
      MatrixStack.bind(textureProgram);

      frameBuffer.getTexture().bind();
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      eyeMeshes[i].bindVertexArray();
      eyeMeshes[i].draw();
      VertexArray.unbind();
      Program.clear();
      MatrixStack.MODELVIEW.pop();
      MatrixStack.PROJECTION.pop();
    }
  }

  public void renderScene(float projectionOffset) {
    glViewport(1, 1, width / 2 - 1, height - 1);
    glClearColor(.3f, .3f, .3f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // coloredProgram.use();
    // MatrixStack.bind(coloredProgram);
    // cubeGeometry.bindVertexArray();
    // cubeGeometry.draw();

    // MatrixStack.PROJECTION.push().identity().preTranslate(projectionOffset);
    // MatrixStack.MODELVIEW.push().identity();
    // textureProgram.use();
    // MatrixStack.bind(textureProgram);
    // IndexedGeometry videoGeometry = null;
    // if (null == videoGeometry) {
    // Vector2f max = new Vector2f(-2f, 1f / aspect);
    // Vector2f min = new Vector2f(max);
    // min.scale(-1f);
    // Vector2f tmax = new Vector2f(1f, 1f);
    // Vector2f tmin = new Vector2f(-1f, -1f);
    // videoGeometry = OpenGL.makeTexturedQuad(min, max, tmax, tmin);
    // }
    // Program.clear();
    videoTexture.bind();
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_TRIANGLE_STRIP);
    float texOffset = 0;
    if (projectionOffset > 0) {
      texOffset = 0.5f;
    }
    float scale = 0.8f;
    float eyeAspect = aspect / 2f;
    {
      glTexCoord2f(texOffset, 1);
      glVertex2f(-1 * scale + projectionOffset, -1 * scale / (videoAspect / eyeAspect));
      glTexCoord2f(texOffset + 0.5f, 1);
      glVertex2f(1* scale + projectionOffset, -1 * scale/ (videoAspect / eyeAspect));
      glTexCoord2f(texOffset, 0);
      glVertex2f(-1* scale + projectionOffset, 1 * scale/ (videoAspect / eyeAspect));
      glTexCoord2f(texOffset + 0.5f, 0);
      glVertex2f(1* scale + projectionOffset, 1 * scale/ (videoAspect / eyeAspect));
    }
    glEnd();
    // videoGeometry.bindVertexArray();
    // videoGeometry.vbo.bind();
    // glEnableVertexAttribArray(Attribute.POSITION.location);
    // glVertexAttribPointer(Attribute.POSITION.location, 4 * 4, GL_FLOAT,
    // false, 32, 0);
    // glEnableVertexAttribArray(Attribute.TEX.location);
    // glVertexAttribPointer(Attribute.TEX.location, 4 * 4, GL_FLOAT, false, 32,
    // 16);
    // videoGeometry.ibo.bind();
    // videoGeometry.draw();
    // videoGeometry.ibo.unbind();
    // VertexArray.unbind();
    videoTexture.unbind();
    // videoGeometry.destroy();
    // MatrixStack.MODELVIEW.pop();
    // MatrixStack.PROJECTION.pop();
  }

  @Override
  protected void onDestroy() {
    RiftTracker.stopListening();
    player.stop();
    player.release();
  }

  @Override
  protected void setupDisplay() {
    Rectangle targetRect = RiftDK1.findRiftRect();
    if (null == targetRect) {
      targetRect = findNonPrimaryRect();
    }
    if (null == targetRect) {
      targetRect = GraphicsEnvironment.getLocalGraphicsEnvironment()
          .getDefaultScreenDevice().getDefaultConfiguration().getBounds();
    }
    setupDisplay(targetRect);
  }

  public static void main(String[] args) {
    NativeLibrary.addSearchPath("libvlc", "C:\\Program Files\\VideoLAN\\VLC");
    Native.loadLibrary(RuntimeUtil.getLibVlcLibraryName(), LibVlc.class);
    new RiftDemo().run();
  }

  // This callback is executed by the VLCJ library whenever new
  // frame data is available. We cannot transfer it to the OpenGL
  // texture here, because the GL calls need to be confined to the
  // main render thread.
  @Override
  public void display(DirectMediaPlayer mediaPlayer, Memory[] nativeBuffers,
      BufferFormat bufferFormat) {
    synchronized (videoData) {
      Memory m = nativeBuffers[0];
      ByteBuffer data = m.getByteBuffer(0, videoData.capacity());
      // data.position(0);
      videoData.position(0);
      videoData.put(data);
      videoData.position(0);

      newFrame = true;
    }
  }

  @Override
  public BufferFormat getBufferFormat(int sourceWidth, int sourceHeight) {
    videoData = BufferUtils.createByteBuffer(sourceWidth * sourceHeight * 4);
    videoWidth = sourceWidth;
    videoHeight = sourceHeight;
    videoAspect = (float) videoWidth / (float) videoHeight;
    return new BufferFormat("RGBA", sourceWidth, sourceHeight, //
        new int[] { sourceWidth * 4 },//
        new int[] { sourceHeight });
  }

  @Override
  public void mediaChanged(MediaPlayer mediaPlayer, libvlc_media_t media,
      String mrl) {
    LOG.warn("Media Changed");
  }

  @Override
  public void opening(MediaPlayer mediaPlayer) {
    LOG.warn("Opening");
  }

  @Override
  public void buffering(MediaPlayer mediaPlayer, float newCache) {
    LOG.warn("Buffering");
  }

  @Override
  public void playing(MediaPlayer mediaPlayer) {
    LOG.warn("Playing");
  }

  @Override
  public void paused(MediaPlayer mediaPlayer) {
    LOG.warn("Paused");
  }

  @Override
  public void stopped(MediaPlayer mediaPlayer) {
    LOG.warn("Stopped");
  }

  @Override
  public void forward(MediaPlayer mediaPlayer) {
    LOG.warn("Foreward");
  }

  @Override
  public void backward(MediaPlayer mediaPlayer) {
    LOG.warn("backward");

  }

  @Override
  public void finished(MediaPlayer mediaPlayer) {
    LOG.warn("finished");
    mediaPlayer.playMedia(MEDIA_URL);
  }

  @Override
  public void timeChanged(MediaPlayer mediaPlayer, long newTime) {
    // LOG.warn("timeChanged");
  }

  @Override
  public void positionChanged(MediaPlayer mediaPlayer, float newPosition) {
    // LOG.warn("positionChanged");
  }

  @Override
  public void seekableChanged(MediaPlayer mediaPlayer, int newSeekable) {
    LOG.warn("seekableChanged");
  }

  @Override
  public void pausableChanged(MediaPlayer mediaPlayer, int newPausable) {
    LOG.warn("pausableChanged");
  }

  @Override
  public void titleChanged(MediaPlayer mediaPlayer, int newTitle) {
    LOG.warn("titleChanged");
  }

  @Override
  public void snapshotTaken(MediaPlayer mediaPlayer, String filename) {
    LOG.warn("snapshotTaken");
  }

  @Override
  public void lengthChanged(MediaPlayer mediaPlayer, long newLength) {
    LOG.warn("lengthChanged");
  }

  @Override
  public void videoOutput(MediaPlayer mediaPlayer, int newCount) {
    LOG.warn("videoOutput");
  }

  @Override
  public void error(MediaPlayer mediaPlayer) {
    LOG.warn("error");
  }

  @Override
  public void mediaMetaChanged(MediaPlayer mediaPlayer, int metaType) {
    LOG.warn("mediaMetaChanged");
  }

  @Override
  public void mediaSubItemAdded(MediaPlayer mediaPlayer, libvlc_media_t subItem) {
    LOG.warn("mediaSubItemAdded");
  }

  @Override
  public void mediaDurationChanged(MediaPlayer mediaPlayer, long newDuration) {
    LOG.warn("mediaDurationChanged");
  }

  @Override
  public void mediaParsedChanged(MediaPlayer mediaPlayer, int newStatus) {
    LOG.warn("mediaParsedChanged");
  }

  @Override
  public void mediaFreed(MediaPlayer mediaPlayer) {
    LOG.warn("mediaFreed");
  }

  @Override
  public void mediaStateChanged(MediaPlayer mediaPlayer, int newState) {
    LOG.warn("mediaStateChanged");
  }

  @Override
  public void newMedia(MediaPlayer mediaPlayer) {
    LOG.warn("newMedia");
  }

  @Override
  public void subItemPlayed(MediaPlayer mediaPlayer, int subItemIndex) {
    LOG.warn("subItemPlayed");
  }

  @Override
  public void subItemFinished(MediaPlayer mediaPlayer, int subItemIndex) {
    LOG.warn("subItemFinished");
  }

  @Override
  public void endOfSubItems(MediaPlayer mediaPlayer) {
    LOG.warn("endOfSubItems");
  }
}
