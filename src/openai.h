#ifndef SRC_OPENAI_H_
#define SRC_OPENAI_H_

#include <memory>
#include <optional>
#include <string>

#include "absl/flags/declare.h"

#include "src/model.h"
#include "src/fetch.h"

ABSL_DECLARE_FLAG(std::optional<std::string>, openai_api_key);

namespace uchen::chat {

std::unique_ptr<ModelProvider> MakeOpenAIModelProvider(
    std::shared_ptr<Fetch> fetch, Parameters parameters);

}  // namespace uchen::chat

#endif  // SRC_OPENAI_H_