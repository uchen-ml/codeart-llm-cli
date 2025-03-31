#ifndef SRC_CLAUDE_CLIENT_H_
#define SRC_CLAUDE_CLIENT_H_

#include <string_view>

#include "src/client.h"

namespace uchen::chat {

std::unordered_map<std::string_view, ClientFactory> ClaudeClients();

}  // namespace uchen::chat

#endif  // SRC_CLAUDE_CLIENT_H_