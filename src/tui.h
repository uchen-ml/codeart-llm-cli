#ifndef SRC_TUI_H_
#define SRC_TUI_H_

#include <iostream>
#include <thread>

#include "absl/cleanup/cleanup.h"
#include "absl/synchronization/mutex.h"

namespace uchen::chat {

template <typename T>
std::invoke_result_t<T> SpinWhile(const T& func) {
  absl::Mutex mu;
  absl::CondVar cv;
  std::optional<std::invoke_result_t<T>> result;
  std::thread spinner_thread([&]() {
    auto r = func();
    absl::MutexLock lock(&mu);
    result = std::move(r);
    cv.Signal();
  });
  absl::Cleanup cleanup = [&]() { spinner_thread.join(); };
  constexpr std::string_view kSpinChars = "|/-\\";
  absl::MutexLock lock(&mu);
  int spin = 0;
  std::cout << " ";
  while (!result.has_value()) {
    std::cout << "\r" << kSpinChars[spin++ % kSpinChars.size()];
    std::cout.flush();
    cv.WaitWithTimeout(&mu, absl::Milliseconds(200));
  }
  std::cout << "\r";
  return std::move(result).value();
}

}  // namespace uchen::chat

#endif  // SRC_TUI_H_