#ifndef SRC_CLIENT_H_
#define SRC_CLIENT_H_

#include <string>
#include <string_view>

#include "absl/status/statusor.h"

#include "src/fetch.h"

namespace uchen::chat {

// Interface for LLM clients
class Client {
 public:
  virtual ~Client() = default;

  virtual std::string_view name() const = 0;

  // Queries the LLM with a prompt and multiple input contents
  virtual absl::StatusOr<std::string> Query(
      const Fetch& fetch, std::string_view prompt,
      absl::Span<const std::string_view> input_contents) = 0;
};

struct Parameters {
  std::string model;
  std::string provider;
  std::optional<std::string> api_key;
  int max_tokens = 1024;
};

using ClientFactory =
    absl::AnyInvocable<absl::StatusOr<std::unique_ptr<Client>>(
        const Parameters&) const>;

}  // namespace uchen::chat

#endif  // SRC_CLIENT_H_