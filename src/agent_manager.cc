#include "src/agent_manager.h"

#include "src/openai_client.h"

namespace codeart::llmcli {

std::string AgentManager::SendMessage(const std::string& agent,
                                      const std::string& message) {
  OpenAIClient client;
  return client.SendMessage(message).value_or("Failed to retrieve response.");
}

}  // namespace codeart::llmcli
