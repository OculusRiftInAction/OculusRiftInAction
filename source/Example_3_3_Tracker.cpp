#include "Common.h"

typedef std::deque<glm::vec3> DequeV3;
#define HISTORY_SIZE 1000
class SensorFusionExample : public GlfwApp {
  ovrHmd hmd;
  ovrSensorState sensorState;
  DequeV3 linear{ HISTORY_SIZE };
  DequeV3 angular{ HISTORY_SIZE };

  bool renderSensors{ false };
  bool renderGraphs{ false };
public:

  SensorFusionExample() {
    hmd = ovrHmd_Create(0);
    if (!hmd) {
      FAIL("Unable to open HMD");
    }
    
    if (!ovrHmd_StartSensor(hmd, 0, 0)) {
      FAIL("Unable to locate Rift sensor device");
    }

  }

  void createRenderingTarget() {
    createWindow(glm::uvec2(1280, 800), glm::ivec2(100, 100));
  }

  void initGl() {
    GlfwApp::initGl();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    gl::Stacks::projection().top() = glm::perspective(
        PI / 3.0f, glm::aspect(windowSize),
        0.01f, 10000.0f);
    gl::Stacks::modelview().top() = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 3.5f),
        GlUtils::ORIGIN, GlUtils::UP);
  }

  virtual void onKey(int key, int scancode, int action, int mods) {
    if (GLFW_PRESS != action && GLFW_REPEAT != action) {
      return;
    }

    switch (key) {
    case GLFW_KEY_G:
      renderGraphs = !renderGraphs;
      return;

    case GLFW_KEY_S:
      if (0 == (mods & GLFW_MOD_SHIFT)) {
        renderSensors = !renderSensors;
        return;
      }
      break;

    case GLFW_KEY_R:
      ovrHmd_ResetSensor(hmd);
      return;
    }

    GlfwApp::onKey(key, scancode, action, mods);
  }

  static void smoothAdd(DequeV3 & queue, const glm::vec3 & add) {
    glm::vec3 acc;
    static int count = 4;
    int i = 0;
    for (DequeV3::const_reverse_iterator ritr = queue.rbegin(); (ritr != queue.rend()) && (++i < count); ++ritr) {
      acc += *ritr;
    }
    acc += add;
    acc /= count;
    queue.push_back(acc);
  }

  void update() {
    sensorState = ovrHmd_GetSensorState(hmd, 0);
    smoothAdd(angular, Rift::fromOvr(sensorState.Recorded.AngularAcceleration));
    while (angular.size() > HISTORY_SIZE) {
      angular.pop_front();
    }
    smoothAdd(linear, Rift::fromOvr(sensorState.Recorded.LinearAcceleration));
    while (linear.size() > HISTORY_SIZE) {
      linear.pop_front();
    }
  }


  // Draw a plot line of the sequence of values. Dynmically fit the y axis to keep the lines on the screen
  // regardless of their range.
  static void drawGraph(std::deque<glm::vec3> &hist_list) {

    // Keep track of the y bounds of the graph. These will change with time to dynamically
    // fit the full y range on the screen
    static float graph_min_y = -1, graph_max_y = 1.0;
    static const float axis_colors[3][3] = {
      { 1, 0, 0 },
      { 0, 1, 0 },
      { 0, 0, 1 }
    };

    if (hist_list.size() < 2) return;
    glLineWidth(1.0f);
    glEnable(GL_BLEND);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glBegin(GL_LINES);
    for (int axis = 0; axis<3; axis++) {
      glColor3f(axis_colors[axis][0], axis_colors[axis][1], axis_colors[axis][2]);
      for (int i = 0; i<(int)hist_list.size() - 1; i++) {
        float x_pos_0 = (float)i / (float)HISTORY_SIZE;
        float x_pos_1 = (float)(i + 1) / (float)HISTORY_SIZE;
        float y_pos_0 = (hist_list[i][axis] - graph_min_y) / (graph_max_y - graph_min_y);
        float y_pos_1 = (hist_list[i + 1][axis] - graph_min_y) / (graph_max_y - graph_min_y);
        glVertex3f(x_pos_0, y_pos_0, 0);
        glVertex3f(x_pos_1, y_pos_1, 0);
      }
    }
    glEnd();


  }

  // Draw the position, velocity or acceleration graphs, depending on the current graph_mode
  void drawGraphs() {
    gl::Stacks::withPush([&]{
      gl::Stacks::modelview().identity();
      float aspect = 1 / glm::aspect(windowSize);
      gl::Stacks::projection().top() = glm::ortho(-1.0f, 1.0f, -aspect, aspect);
      glm::vec2 cursor(0.2f, aspect * 0.95f);
      GlUtils::renderString("Angular Acceleration", cursor, 18);
      cursor = glm::vec2(0.2f, aspect * -0.05f);
      GlUtils::renderString("Linear Acceleration", cursor, 18);
    });

    // Set up ortho proj mat
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, -1, 1);

    // Clear modelview mat
    glMatrixMode(GL_MODELVIEW);

    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);

    gl::viewport(glm::uvec2(0, 0), glm::uvec2(windowSize.x, windowSize.y / 2));
    drawGraph(linear);

    gl::viewport(glm::uvec2(0, windowSize.y / 2), glm::uvec2(windowSize.x, windowSize.y / 2));
    drawGraph(angular);

    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    gl::viewport(windowSize);
  }

  void draw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl::MatrixStack & mv = gl::Stacks::modelview();
    gl::MatrixStack & pr = gl::Stacks::projection();

    // Update the modelview to reflect the orientation of the Rift
    glm::mat4 glm_mat = glm::mat4_cast(Rift::fromOvr(sensorState.Recorded.Pose.Orientation));
    mv.push().transform(glm_mat);
    GlUtils::renderRift();
    mv.pop();

    // Text display of our current sensor settings
    mv.push().identity();
    pr.push().top() = glm::ortho(
        -1.0f, 1.0f,
        -windowAspectInverse, windowAspectInverse,
        -100.0f, 100.0f);

    ovrSensorDesc desc;
    ovrHmd_GetSensorDesc(hmd, &desc);
    glm::vec2 cursor(-0.9, windowAspectInverse * 0.9);
    std::string message = Platform::format(
        "Gravity Correction: %s\n"
        "Magnetic Correction: %s\n"
        "Prediction Delta: %0.2f ms\n",
        true ? "Enabled" : "Disabled",
        true ? "Enabled" : "Disabled",
        true ? 1 * 1000.0f : 0.0f );
    GlUtils::renderString(message, cursor, 18.0f);

    // Render a set of axes and local basis to show the Rift's sensor output
    if (renderSensors) {
      mv.top() = glm::lookAt(
          glm::vec3(3.0f, 1.0f, 3.0f),
          GlUtils::ORIGIN, GlUtils::UP);
      mv.translate(glm::vec3(0.75f, -0.3f, 0.0f));
      mv.scale(0.2f);

      glm::vec3 acc = Rift::fromOvr(sensorState.Recorded.LinearAcceleration);
      glm::vec3 rot = Rift::fromOvr(sensorState.Recorded.AngularVelocity);
//      glm::vec3 mag = Rift::fromOvr(sensorState.Recorded);
      GlUtils::draw3dGrid();
      GlUtils::draw3dVector(acc, Colors::white);
      GlUtils::draw3dVector(rot, Colors::yellow);
//      GlUtils::draw3dVector(mag, Colors::cyan);
    }

    mv.pop(); 
    pr.pop();
    if (renderGraphs) {
      drawGraphs();
    }
  }
};

RUN_OVR_APP(SensorFusionExample)

