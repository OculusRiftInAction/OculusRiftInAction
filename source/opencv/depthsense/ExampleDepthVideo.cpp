#include "Common.h"
#include <numeric>
#include <algorithm>
#include <boost/thread.hpp>
#include <boost/thread/locks.hpp>

#include <DepthSense.hxx>
#include <opencv2/opencv.hpp>
#include <glm/gtx/vector_angle.hpp>

//#define CV_RENDER
#define MAIN_WINDOW "Display window"
#define CTL_WINDOW "Control window"

typedef std::vector<cv::Vec4i> LVec;
typedef std::vector<float> FVec;
typedef std::vector<int> IVec;
typedef std::map<int, IVec> PMap;
typedef std::pair<LVec, FVec> Pair;
typedef std::map<int, Pair> Map;


template<typename VEC, typename Function>
  void for_each_row_and_column(VEC size, Function f) {
    for (size_t row = 0; row < size.y; ++row) {
      for (size_t col = 0; col < size.x; ++col) {
        f(row, col);
      }
    }
  }

template<typename VEC, typename Function>
  void for_each_row_and_column_with_offset(VEC size, Function f) {
    for (size_t row = 0; row < size.y; ++row) {
      for (size_t col = 0; col < size.x; ++col) {
        f(row, col, (row * size.x) + col);
      }
    }
  }

template<typename A, typename B, typename Function>
  void for_each_pixel(cv::Mat a, cv::Mat b, Function f) {
    if (a.rows != b.rows || a.cols != b.cols) {
      FAIL("Size mismatch");
    }
    for_each_row_and_column(glm::uvec2(a.cols, a.rows),
        [&](size_t y, size_t x) {
          f(a.at<A>(y, x), b.at<B>(y, x));
        });
  }

template<typename A, typename B, typename Function>
  void for_each_masked_pixel(cv::Mat image, cv::Mat mask, Function f) {
    if (image.rows != mask.rows || image.cols != mask.cols) {
      FAIL("Size mismatch");
    }
    for_each_row_and_column(glm::uvec2(image.cols, mask.rows),
        [&](size_t y, size_t x) {
          B & maskPixel = mask.at<B>(y, x);
          if (!maskPixel) {
            return;
          }
          f(image.at<A>(y, x), maskPixel);
        });
  }

void zeroUnmasked(cv::Mat & image, cv::Mat & mask) {
  cv::Mat outsideMask;
  cv::bitwise_not(mask, outsideMask);
  cv::subtract(image, image, image, outsideMask);
}

void erode(int size, cv::Mat depthMask) {
  cv::Mat element = getStructuringElement(cv::MORPH_ELLIPSE,
      cv::Size(2 * size + 1, 2 * size + 1));
  cv::erode(depthMask, depthMask, element);
}

void dilate(int size, cv::Mat depthMask) {
  cv::Mat element = getStructuringElement(cv::MORPH_ELLIPSE,
      cv::Size(2 * size + 1, 2 * size + 1));
  cv::dilate(depthMask, depthMask, element);
}

template <typename V, typename Function>
PMap partitionToMap(const V & v, IVec & labels, Function f) {
  int partitionResult = cv::partition(v, labels, f);
  PMap result;
  for (int i = 0; i < v.size(); ++i) {
    result[labels[i]].push_back(i);
  }
  return result;
}

float slope(const cv::Vec4i & in){
  return atan((double)(in[3] - in[1]) / (in[2] - in[0]));
};

class CaptureHandler {
protected:
  boost::condition_variable frameChangedCondition;
  volatile bool frameChanged { false };
  volatile bool stop { false };
  // A lock to ensure only one thread is accessing
  // currentFrame / frameChanged at a time
  boost::mutex frameMutex;
  // A background thread for capturing stills from the camera
  boost::thread captureThread;

  CaptureHandler() {
  }

  void startCapture() {
    captureThread = boost::thread(boost::ref(*this));
  }
public:
  virtual void operator()() = 0;
};

class DepthSenseHelper {
public:
  template<class T>
    static T getFirstAvailableNode(DepthSense::Context context) {
      T result;
      auto devices = context.getDevices();
      // obtain the list of devices attached to the host
      std::for_each(std::begin(devices), std::end(devices),
          [&](DepthSense::Device device)
          {
            if (result.isSet()) return;

            auto nodes = device.getNodes();
            std::for_each(std::begin(nodes), std::end(nodes),
                [&](DepthSense::Node node)
                {
                  if (result.isSet()) return;

                  T resultNode = node.as<T>();
                  if (resultNode.isSet())
                  result = resultNode;
                });
          });

      return result;
    }
};

class DepthHandler : public CaptureHandler {
private:
  DepthSense::Context context;
  DepthSense::DepthNode depthNode;
  std::vector<glm::vec3> depthData;
  cv::Mat depthImage;
  static DepthHandler * INSTANCE;

  static void onNewDepthample(DepthSense::DepthNode obj,
      DepthSense::DepthNode::NewSampleReceivedData data) {
    INSTANCE->onDepthFrame(data);
  }
protected:
  void readDepthData(DepthSense::DepthNode::NewSampleReceivedData data) {
    for_each_row_and_column_with_offset(imageSize,
        [&](size_t y, size_t x, size_t offset) {
          const DepthSense::FPVertex & sourceVertex = data.verticesFloatingPoint[offset];
          const uint16_t & sourcePixel = data.depthMap[offset];
          uint16_t & targetPixel = depthImage.at<uint16_t>(y, x);
          glm::vec3 & targetVertex = depthData[offset];
          if (sourceVertex == BAD_VERTEX) {
            targetVertex = NO_DATA;
            targetPixel = 0;
          }
          else {
            targetVertex = glm::vec3(sourceVertex.x, sourceVertex.y, sourceVertex.z);
            targetPixel = sourcePixel;
          }
        });
  }

  virtual void onDepthFrame(
      DepthSense::DepthNode::NewSampleReceivedData data) {
    boost::lock_guard < boost::mutex > lock(frameMutex);
    readDepthData(data);
    frameChanged = true;
    frameChangedCondition.notify_one();
  }

public:
  glm::uvec2 imageSize;
  static const DepthSense::FPVertex BAD_VERTEX;
  static const glm::vec3 NO_DATA;

  virtual void operator()() {
    context.run();
  }

  DepthHandler() {
    INSTANCE = this;

    context = DepthSense::Context::create();
    depthNode = DepthSenseHelper::getFirstAvailableNode<
        DepthSense::DepthNode>(context);
    if (!depthNode.isSet()) {
      FAIL("no color node found");
    }
    DepthSense::DepthNode::Configuration config =
        depthNode.getConfiguration();
    config.saturation = false;
    config.frameFormat = DepthSense::FrameFormat::FRAME_FORMAT_QVGA;
    config.framerate = 60;
    context.requestControl(depthNode);
    depthNode.setConfiguration(config);
    context.releaseControl(depthNode);
    depthNode.setEnableVerticesFloatingPoint(true);
    depthNode.setEnableDepthMap(true);
    depthNode.newSampleReceivedEvent().connect(onNewDepthample);
    DepthSense::FrameFormat format =
        depthNode.getConfiguration().frameFormat;
    {
      int32_t x, y;
      FrameFormat_toResolution(format, &x, &y);
      imageSize = glm::uvec2(x, y);
    }
    depthData.resize(imageSize.x * imageSize.y);
    depthImage = cv::Mat::zeros(imageSize.y, imageSize.x, CV_16UC1);
    context.registerNode(depthNode);
    context.startNodes();
  }

  virtual void startDepthCapture() {
    captureThread = boost::thread(boost::ref(*this));
  }

  const cv::Mat & getDepthImage() const {
    return depthImage;
  }

  const std::vector<glm::vec3> & getDepthVertices() const {
    return depthData;
  }

  const glm::vec3 & getDepthVertex(size_t x, size_t y) const {
    return depthData[imageSize.x * y + x];
  }
};

DepthHandler * DepthHandler::INSTANCE;
const DepthSense::FPVertex DepthHandler::BAD_VERTEX =
    DepthSense::FPVertex(0, 0, -1);
const glm::vec3 DepthHandler::NO_DATA = glm::vec3(0, 0, 0);


class WebcamHandler : public CaptureHandler {
protected:
  cv::VideoCapture videoCapture { 0 };
  glm::uvec2 imageSize;
  cv::Mat webcamImage;

  WebcamHandler() {
    if (!videoCapture.grab()) {
      FAIL("Failed grab");
    }
    videoCapture.set(CV_CAP_PROP_FRAME_WIDTH, 1280);
    videoCapture.set(CV_CAP_PROP_FRAME_HEIGHT, 720);
    int fps = (int) videoCapture.get(CV_CAP_PROP_FPS);
    videoCapture.set(CV_CAP_PROP_FPS, 60);
    // Force minimum focus, and disable autofocus
    //    videoCapture.set(CV_CAP_PROP_FOCUS, 0);
    imageSize = glm::uvec2(videoCapture.get(CV_CAP_PROP_FRAME_WIDTH),
        videoCapture.get(CV_CAP_PROP_FRAME_HEIGHT));
    webcamImage = cv::Mat::zeros(imageSize.y, imageSize.x, CV_8UC3);
  }

  virtual void operator()() {
    int framecount = 0;
    long start = Platform::elapsedMillis();
    while (!stop) {
      cv::waitKey(1);
      if (!videoCapture.grab()) {
        FAIL("Failed grab");
      }
      // Wait for OpenGL to consume a frame before fetching a new one
      while (frameChanged) {
        Platform::sleepMillis(10);
      }
      static cv::Mat frame;
      videoCapture.retrieve(frame);
      boost::lock_guard < boost::mutex > lock(frameMutex);
      cv::flip(frame, webcamImage, 0);
      frameChanged = true;
    }
  }
};

#ifdef CV_RENDER
class DepthApp : public DepthHandler {
#else
class DepthApp : public RiftApp, public DepthHandler {
#endif
protected:
  std::vector<float> depthDelta;
  bool saveImage { false };
  cv::Mat result;
  cv::Mat roiMask;
  cv::RNG rng;

public:

  DepthApp() {
    depthDelta.resize(imageSize.x * imageSize.y);
    result = cv::Mat::zeros(imageSize.y, imageSize.x, CV_8UC3);

    cv::namedWindow(CTL_WINDOW, cv::WINDOW_AUTOSIZE);
#if CV_RENDER
    cv::moveWindow(CTL_WINDOW, 1200, -500);
#endif
    cv::createTrackbar("H P", CTL_WINDOW, &pixels, 10);
    cv::createTrackbar("H D", CTL_WINDOW, &degrees, 10);
    cv::createTrackbar("H MinL", CTL_WINDOW, &minLen, 100);
    cv::createTrackbar("H MaxG", CTL_WINDOW, &maxGap, 100);
    cv::createTrackbar("H T", CTL_WINDOW, &houghThreshold, 60);
    startCapture();
  }


  int pixels{ 1 };
  int degrees{ 1 };
  int minLen{ 50 };
  int maxGap{ 10 };
  int houghThreshold{ 41 };

  LVec detectLines() {
    using namespace cv;
    Mat depthImage;
    // Load and clean up the dpeth image
    depthImage = getDepthImage().clone();
    {
      result = Mat::zeros(depthImage.size(), CV_8UC3);
      Mat depthMask;
      depthImage.convertTo(depthMask, CV_8U);
  
      if (!roiMask.empty()) {
        zeroUnmasked(depthImage, roiMask);
      }

      // Remove speckle
      {
        Mat speckleMask = depthMask.clone();
        erode(2, speckleMask);
        dilate(3, speckleMask);
        zeroUnmasked(depthImage, speckleMask);
        // Update the depth mask
        depthImage.convertTo(depthMask, CV_8U);
      }

      // Convert to 8 bits and invert the color (closer is brighter)
      {
        normalize(depthImage, depthImage, 0, 255, CV_MINMAX, -1, depthMask);
        depthImage.convertTo(depthImage, CV_8U);
        depthImage = 255 - depthImage;
        zeroUnmasked(depthImage, depthMask);
        insertChannel(depthImage, result, 1);
      }
    }


    Mat gradient;
    {
      Mat grad_x, grad_y;
      Sobel(depthImage, grad_x, CV_16S, 1, 0, 1);
      convertScaleAbs(grad_x, grad_x);
      Sobel(depthImage, grad_y, CV_16S, 0, 1, 1);
      convertScaleAbs(grad_y, grad_y);
      addWeighted(grad_x, 0.5, grad_y, 0.5, 0, gradient);

      threshold(gradient, gradient, 0, 255, CV_THRESH_BINARY | THRESH_OTSU);
      gradient.convertTo(gradient, CV_8UC1);

      //Mat element = getStructuringElement(MORPH_ELLIPSE, Size(1, 1));
      //morphologyEx(gradient, gradient, MORPH_OPEN, element);
      //morphologyEx(gradient, gradient, MORPH_CLOSE, element);
      insertChannel(gradient, result, 2);
    }

    // Detect lines using Hough
    LVec lines;
    
    HoughLinesP(gradient, lines, pixels, degrees * CV_PI / 180, houghThreshold, minLen, maxGap);
    return lines;
  }

  typedef std::pair<glm::vec2, glm::vec2> Line;

  template <typename V>
  static Line getEndPoints(const V & v) {
    return Line(glm::vec2(v[0], v[1]), glm::vec2(v[2], v[3]));
  }

  virtual void onDepthFrame(DepthSense::DepthNode::NewSampleReceivedData data) {
    using namespace cv;
    boost::lock_guard < boost::mutex > lock(frameMutex);
    readDepthData(data);

    // Do surface normal calculation
    LVec lines = detectLines();
    if (lines.empty()) {
      return;
    }

    // Find the slopes of all the lines
    FVec slopes(lines.size());
    std::transform(lines.begin(), lines.end(), slopes.begin(), [](const Vec4i & in){
      return slope(in);
    });
    static const float SLOPE_EPSILON = PI / 72.0f;
    IVec slobeLabels;
    PMap slopePMap = partitionToMap(slopes, slobeLabels, [&](const float & a, const float & b) {
      return abs(a - b) < SLOPE_EPSILON;
    });

    IVec connectionLabels;
    static const float DISTANCE_EPSILON = 16;
    PMap connectionPMap = partitionToMap(lines, connectionLabels, [&](const Vec4i & a, const Vec4i & b) {
      Line la = getEndPoints(a);
      Line lb = getEndPoints(b);
      return ((glm::distance2(la.first, lb.first) < DISTANCE_EPSILON) ||
        (glm::distance2(la.first, lb.second) < DISTANCE_EPSILON) ||
        (glm::distance2(la.second, lb.first) < DISTANCE_EPSILON) ||
        (glm::distance2(la.second, lb.second) < DISTANCE_EPSILON)) && (abs(slope(a) - slope(b)) > PI / 3);
    });


    //std::for_each(connectionPMap.begin(), connectionPMap.end(), [&](PMap::const_reference & item){
    //  const IVec & lineIndexes = item.second;
    //  int size = lineIndexes.size();
    //  if (size < 2) {
    //    return;
    //  }


    //  Line la = getEndPoints(a);
    //  Line lb = getEndPoints(b);
    //  return ((glm::distance2(la.first, lb.first) < DISTANCE_EPSILON) ||
    //    (glm::distance2(la.first, lb.second) < DISTANCE_EPSILON) ||
    //    (glm::distance2(la.second, lb.first) < DISTANCE_EPSILON) ||
    //    (glm::distance2(la.second, lb.second) < DISTANCE_EPSILON)) && (abs(slope(a) - slope(b)) > PI / 3);


    //  if (size > 3) {
    //    fillPoly(result, )
    //  } else {
    //    Scalar color(255, (uchar)rng, 0);
    //    std::for_each(lineIndexes.begin(), lineIndexes.end(), [&](int i){
    //      const Vec4i & l = lines[i];
    //      cv::line(result, Point(l[0], l[1]), Point(l[2], l[3]), color, 2, CV_AA);
    //    });
    //  }

    //});




    // Partition the lines by slope
    //Map linesPartitioned;
    //{
    //  //template<typename _Tp, class _EqPredicate> 
    //  std::vector<int> labels;
    //  // Partition lines that are within 5 degrees of each other
    //  static const float epsilon = PI / 72.0f;
    //  int partitionResult = partition(slopes, labels, [&](const float & a, const float & b) {
    //    return abs(a - b) < epsilon;
    //  });

    //  for (size_t i = 0; i < lines.size(); i++) {
    //    Pair & pair = linesPartitioned[labels[i]];
    //    pair.first.push_back(lines[i]);
    //    pair.second.push_back(slopes[i]);
    //  }
    //}


    //RNG rng(Platform::elapsedMillis());
    //std::for_each(linesPartitioned.begin(), linesPartitioned.end(), [&](Map::const_reference & item){
    //  const LVec & lines = item.second.first;
    //  if (lines.size() < 2) {
    //    return;
    //  }

    //  //const FVec & slopes = item.second.second;
    //  //float sum = std::accumulate(slopes.begin(), slopes.end(), 0.0f);
    //  //float average = sum / slopes.size();
    //  //float minz = std::numeric_limits<float>::max(), maxz = std::numeric_limits<float>::min();
    //  //std::for_each(lines.begin(), lines.end(), [&](const Vec4i & l){
    //  //  float a, b;
    //  //  if (fabs(average) > PI / 4) {
    //  //    a = l[0];
    //  //    b = l[2];
    //  //  }
    //  //  else {
    //  //    a = l[1];
    //  //    b = l[3];
    //  //  }
    //  //  float range = fabs(b - a);
    //  //  minz = std::min(minz, range);
    //  //  maxz = std::max(maxz, range);
    //  //});

    //  //float range = maxz - minz;
    //  ////if (range < 10) {
    //  ////  return;
    //  ////}


    //  Scalar color((uchar)rng, (uchar)rng, (uchar)rng);
    //  for (size_t i = 0; i < lines.size(); i++) {
    //    const Vec4i & l = lines[i];
    //    glm::vec2 a((l[0], l[1]));
    //    glm::vec2 b((l[2], l[3]));
    //    float length = glm::length(a - b);
    //    glm::vec2 line = a - b;
    //    float slope = line.x / line.y;
    //    cv::line(result, Point(l[0], l[1]), Point(l[2], l[3]),
    //      color, 2, CV_AA);
    //  }

    //});
    frameChanged = true;
  }


  /**
   * @function on_trackbar
   * @brief Callback for trackbar
   */
//  static void on_trackbar( int v, void* p) {
//    ((DepthApp*)p)->canny_threshold = v;
//  }

#ifdef CV_RENDER

  int run() {
    gl::MatrixStack & mv = gl::Stacks::modelview();
    cv::namedWindow(MAIN_WINDOW, cv::WINDOW_AUTOSIZE);
    cv::moveWindow(MAIN_WINDOW, 800, -500);

    bool quit{ false };
    while (!quit) {
      if (frameChanged) {
        boost::lock_guard < boost::mutex > lock(frameMutex);
        cv::imshow(MAIN_WINDOW, result);
        frameChanged = false;
      }
      int key = cvWaitKey(15);
      if ('q' == (0xFF & key)) {
        quit = true;
      }
    }
    return 0;

  }

#else

  gl::GeometryPtr imageGeometry;
  gl::Texture2dPtr depthImageTexture;


  virtual void update() {
    RiftGlfwApp::update();
    CameraControl::instance().applyInteraction(player);
    gl::Stacks::modelview().top() =
      glm::inverse(player);

    if (frameChanged) {
      boost::lock_guard < boost::mutex > lock(frameMutex);
      flip(result, result, -1);
      depthImageTexture->bind();
      depthImageTexture->image2d(imageSize, result.data, 0, GL_BGR);
      gl::Texture2d::unbind();
    } // frame changed
  }

  void initGl() {
    RiftApp::initGl();
    glEnable(GL_BLEND);
    glPointSize(1.0f);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

    imageGeometry = GlUtils::getQuadGeometry(glm::aspect(imageSize), 1.0f);
    depthImageTexture = gl::Texture2dPtr(new gl::Texture2d());
    depthImageTexture->bind();
    depthImageTexture->parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    depthImageTexture->parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl::Texture2d::unbind();
  }

  virtual void renderScene() {
    glEnable(GL_DEPTH_TEST);
    gl::clearColor(glm::vec3(0.05f, 0.05f, 0.05f));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gl::MatrixStack & mv = gl::Stacks::modelview();
    gl::MatrixStack & pr = gl::Stacks::projection();
    gl::Stacks::with_push(mv, pr, [&]{
      //      GlUtils::renderSkybox(Resource::IMAGES_SKY_TRON_XNEG_PNG);
      //      GlUtils::drawColorCube();
      gl::Stacks::with_push(mv, [&]{
        mv.translate(glm::vec3(0, 1.5, 2));
        mv.scale(3.0f);
        gl::ProgramPtr program = GlUtils::getProgram(
          Resource::SHADERS_TEXTURED_VS,
          Resource::SHADERS_TEXTURED_FS);
        depthImageTexture->bind();
        GlUtils::renderGeometry(imageGeometry, program);
        gl::Texture2d::unbind();
      });
    });
  }

#endif
};

RUN_OVR_APP(DepthApp);

//template<typename VV>
//  pcl::PointCloud<pcl::PointXYZ>::Ptr cloudFromVertices(VV vv) {
//    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(
//        new pcl::PointCloud<pcl::PointXYZ>());
//    cloud->reserve(vv.size());
//    std::for_each(vv.begin(), vv.end(), [&](glm::vec3 v) {
//      if (v != DepthHandler::NO_DATA) {
//        cloud->push_back(pcl::PointXYZ(v.x, v.y, v.z));
//      }
//    });
//    return cloud;
//  }

//template<typename VV>
//  pcl::PointCloud<pcl::PointXYZ>::Ptr organizedCloudFromVertices(
//      const VV & vv, const glm::uvec2 & cloudSize) {
//
//    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(
//        new pcl::PointCloud<pcl::PointXYZ>());
//    cloud->resize(vv.size());
//    cloud->height = cloudSize.y;
//    cloud->width = cloudSize.x;
//
//    for_each_row_and_column_with_offset(cloudSize,
//        [&](size_t row, size_t col, size_t offset) {
//          const glm::vec3 & v = vv[offset];
//          pcl::PointXYZ & target = (*cloud)[offset];
//          static pcl::PointXYZ NAN_POINT(
//              std::numeric_limits<double>::quiet_NaN(),
//              std::numeric_limits<double>::quiet_NaN(),
//              std::numeric_limits<double>::quiet_NaN());
//          if (v == DepthHandler::NO_DATA) {
//            cloud->is_dense = false;
//            target = NAN_POINT;
//          }
//          else {
//            target = pcl::PointXYZ(v.x, v.y, v.z);
//          }
//        });
//    return cloud;
//  }

