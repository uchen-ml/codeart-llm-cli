#ifndef SRC_EVENT_LOOP_H
#define SRC_EVENT_LOOP_H

#include <memory>
#include <thread>
#include <vector>

#include "absl/functional/any_invocable.h"
#include "absl/log/log.h"  // IWYU pragma: keep
#include "absl/synchronization/mutex.h"

namespace uchen::chat {

class EventLoop {
 public:
  static std::shared_ptr<EventLoop> Create() {
    return std::make_shared<EventLoop>();
  }

  EventLoop() : thread_(&EventLoop::Loop, this) {}

  ~EventLoop() {
    {
      absl::MutexLock lock(&mutex_);
      stop_ = true;
    }
    thread_.join();
  }

  void Run(absl::AnyInvocable<void()> task);

 private:
  using TasksList = std::vector<absl::AnyInvocable<void()>>;
  static void Loop(EventLoop* event_loop);

  std::variant<bool, TasksList> GetTasks();

  absl::Mutex mutex_;
  TasksList tasks_ ABSL_GUARDED_BY(&mutex_);
  bool stop_ ABSL_GUARDED_BY(&mutex_) = false;
  std::thread thread_;
};

}  // namespace uchen::chat

#endif  // SRC_EVENT_LOOP_H