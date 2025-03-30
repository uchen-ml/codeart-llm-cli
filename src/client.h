#ifndef SRC_PROCESSFILE_CLIENT_H_
#define SRC_PROCESSFILE_CLIENT_H_

#include <string>
#include <string_view>

#include "absl/status/statusor.h"
#include "src/fetch.h"

namespace processfile {

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

}  // namespace processfile

#endif  // SRC_PROCESSFILE_CLIENT_H_