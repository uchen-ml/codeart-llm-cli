#ifndef CODEART_LLMCLI_MESSAGE_H_
#define CODEART_LLMCLI_MESSAGE_H_

#include <string>
#include <string_view>

#include "absl/strings/str_cat.h"

namespace codeart::llmcli {

class Message {
 public:
  explicit Message(std::string_view contents, bool is_user)
      : contents_(contents), is_user_(is_user) {}

  const std::string& contents() const { return contents_; }
  const std::vector<Message>& context() const { return context_; }
  bool is_user() const { return is_user_; }

  template <typename Sink>
  friend void AbslStringify(Sink& sink, const Message& message) {
    sink.Append(absl::StrCat("Message: ", message.contents()));
  }

 private:
  std::string contents_;
  bool is_user_;
  std::vector<Message> context_;
};

}  // namespace codeart::llmcli

#endif  // CODEART_LLMCLI_MESSAGE_H_