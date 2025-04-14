#ifndef SRC_ANTHROPIC_H_
#define SRC_ANTHROPIC_H_

#include "src/client.h"

namespace uchen::chat {

std::unique_ptr<ModelProvider> MakeAnthropicModelProvider(
    std::shared_ptr<Fetch> fetch, Parameters parameters);

}  // namespace uchen::chat

#endif  // SRC_ANTHROPIC_H_