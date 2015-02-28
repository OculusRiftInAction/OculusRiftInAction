#pragma once

namespace oria {
  std::string readFile(const std::string & filename);
}

class TaskQueueWrapper {
  typedef std::mutex Mutex;
  typedef std::unique_lock<Mutex> Locker;
  typedef std::queue<Lambda> TaskQueue;

  TaskQueue queue;
  Mutex mutex;

public:
  void drainTaskQueue() {
    TaskQueue copy;
    {
      Locker lock(mutex);
      std::swap(copy, queue);
    }
    while (!copy.empty()) {
      copy.front()();
      copy.pop();
    }
  }

  void queueTask(Lambda task) {
    Locker locker(mutex);
    queue.push(task);
  }

};


class RateCounter {
  std::vector<float> times;

public:
  void reset() {
    times.clear();
  }

  float elapsed() const {
    if (times.size() < 1) {
      return 0.0f;
    }
    float elapsed = *times.rbegin() - *times.begin();
    return elapsed;
  }

  void increment() {
    times.push_back(Platform::elapsedSeconds());
  }

  float getRate() const {
    if (elapsed() == 0.0f) {
      return NAN;
    }
    return (times.size() - 1) / elapsed();
  }
};
