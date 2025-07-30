#ifndef SRC_OPENAI_CLIENT_H_
#define SRC_OPENAI_CLIENT_H_

#include <string_view>
#include <unordered_map>

#include "src/client.h"

namespace uchen::chat {

std::unordered_map<std::string_view, std::unique_ptr<ClientFactory>>
OpenAIClients();

}  // namespace uchen::chat

#endif  // SRC_OPENAI_CLIENT_H_