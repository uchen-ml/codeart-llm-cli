#ifndef CODEART_LLMCLI_OPENAI_CLIENT_H_
#define CODEART_LLMCLI_OPENAI_CLIENT_H_

#include <string>

#include "absl/status/statusor.h"

namespace codeart::llmcli {

class OpenAIClient {
 public:
  absl::StatusOr<std::string> SendMessage(const std::string& message);
};

}  // namespace codeart::llmcli

#endif  // CODEART_LLMCLI_OPENAI_CLIENT_H_
