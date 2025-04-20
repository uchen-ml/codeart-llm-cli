#include "src/event_loop.h"

#include <utility>
#include <vector>

namespace uchen::chat {

void EventLoop::Run(absl::AnyInvocable<void() > task) {
  absl::MutexLock lock(&mutex_);
  tasks_.push_back(std::move(task));
}

void EventLoop::Loop(EventLoop* event_loop) {
  while (true) {
    auto done_tasks = event_loop->GetTasks();
    if (std::holds_alternative<bool>(done_tasks)) {
      break;
    }
    for (auto& task : std::get<TasksList>(done_tasks)) {
      task();
    }
  }
}

std::variant<bool, std::vector<absl::AnyInvocable<void()>>>
EventLoop::GetTasks() {
  absl::MutexLock lock(&mutex_);
  absl::Condition condition(
      +[](EventLoop* event_loop)
           ABSL_EXCLUSIVE_LOCKS_REQUIRED(event_loop->mutex_) {
             return !event_loop->tasks_.empty() || event_loop->stop_;
           },
      this);
  mutex_.Await(condition);
  if (stop_) {
    return true;
  }
  return std::move(tasks_);
}

}  // namespace uchen::chat