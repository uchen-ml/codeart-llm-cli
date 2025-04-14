#ifndef SRC_ANTHROPIC_H_
#define SRC_ANTHROPIC_H_

#include "absl/flags/declare.h"

#include "src/fetch.h"
#include "src/model.h"

ABSL_DECLARE_FLAG(std::optional<std::string>, anthropic_api_key);

namespace uchen::chat {

std::unique_ptr<ModelProvider> MakeAnthropicModelProvider(
    std::shared_ptr<Fetch> fetch, Parameters parameters);

}  // namespace uchen::chat

#endif  // SRC_ANTHROPIC_H_