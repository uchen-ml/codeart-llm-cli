#ifndef SRC_CLIENT_H_
#define SRC_CLIENT_H_

#include <memory>
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
  int max_tokens = 1024;
};

using ModelHandle = std::unique_ptr<Client>;

class ModelProvider {
 public:
  virtual ~ModelProvider() = default;
  virtual std::string_view name() const = 0;
  virtual absl::StatusOr<ModelHandle> ConnectToModel() const = 0;
  virtual std::vector<std::string> ListModels() const = 0;
};

}  // namespace uchen::chat

#endif  // SRC_CLIENT_H_