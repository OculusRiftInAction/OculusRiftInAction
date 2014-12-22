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
  unsigned int count{ 0 };
  float start{ -1 };

public:
  void reset() {
    count = 0;
    start = -1;
  }

  unsigned int getCount() const {
    return count;
  }

  void startCounter() {
    if (0 == count) {
      this->start = Platform::elapsedSeconds();
    }
  }

  float elapsed() {
    if (count <= 0) {
      return 0;
    }
    return Platform::elapsedSeconds() - start;
  }

  void increment() {
    if (start < 0) {
      start = Platform::elapsedSeconds();
    } else {
      ++count;
    }
  }

  float getRate() const {
    float elapsed = Platform::elapsedSeconds() - start;
    return (float)count / elapsed;
  }
};
