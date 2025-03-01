#ifndef CODEART_LLMCLI_AGENT_MANAGER_H_
#define CODEART_LLMCLI_AGENT_MANAGER_H_

#include <string>

namespace codeart::llmcli {

class AgentManager {
 public:
  std::string SendMessage(const std::string& agent, const std::string& message);
};

}  // namespace codeart::llmcli

#endif  // CODEART_LLMCLI_AGENT_MANAGER_H_
