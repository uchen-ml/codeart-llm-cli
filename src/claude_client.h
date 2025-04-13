#ifndef SRC_CLAUDE_CLIENT_H_
#define SRC_CLAUDE_CLIENT_H_

#include "src/client.h"

namespace uchen::chat {

std::unique_ptr<ModelProvider> AnthropicModelProvider();

}  // namespace uchen::chat

#endif  // SRC_CLAUDE_CLIENT_H_