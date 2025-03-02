#ifndef CODEART_LLMCLI_MESSAGE_H_
#define CODEART_LLMCLI_MESSAGE_H_

#include <string>
#include <string_view>

#include "absl/strings/str_cat.h"

namespace codeart::llmcli {

class Message {
 public:
  explicit Message(std::string_view contents) : contents_(contents) {}

  const std::string& contents() const { return contents_; }

  template <typename Sink>
  friend void AbslStringify(Sink& sink, const Message& message) {
    sink.Append(absl::StrCat("Message: ", message.contents()));
  }

 private:
  std::string contents_;
};

}  // namespace codeart::llmcli

#endif  // CODEART_LLMCLI_MESSAGE_H_