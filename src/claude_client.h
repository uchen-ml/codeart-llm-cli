#ifndef SRC_PROCESSFILE_CLAUDE_CLIENT_H_
#define SRC_PROCESSFILE_CLAUDE_CLIENT_H_

#include <string>
#include <string_view>

#include "absl/status/statusor.h"
#include "absl/types/span.h"

#include "src/client.h"
#include "src/fetch.h"

namespace processfile {

class ClaudeClient : public Client {
 public:
  explicit ClaudeClient(std::string_view model, std::string_view name,
                        std::string_view api_key, int max_tokens)
      : model_(model),
        name_(name),
        api_key_(api_key),
        max_tokens_(max_tokens) {}
  ~ClaudeClient() override = default;

  std::string_view name() const override { return name_; }

  absl::StatusOr<std::string> Query(
      const Fetch& fetch, std::string_view prompt,
      absl::Span<const std::string_view> input_contents) override;

 private:
  std::string model_;
  std::string name_;
  std::string api_key_;
  int max_tokens_;
};

}  // namespace processfile

#endif  // SRC_PROCESSFILE_CLAUDE_CLIENT_H_