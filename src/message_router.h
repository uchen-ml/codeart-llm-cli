#ifndef CODEART_LLMCLI_MESSAGE_ROUTER_H_
#define CODEART_LLMCLI_MESSAGE_ROUTER_H_

#include <string>

#include "agent_manager.h"
#include "history.h"

namespace codeart::llmcli {

class MessageRouter {
 public:
  MessageRouter(AgentManager& agent_manager, History& history);

  void RouteMessage(const std::string& message);

 private:
  AgentManager& agent_manager_;
  History& history_;
};

}  // namespace codeart::llmcli

#endif  // CODEART_LLMCLI_MESSAGE_ROUTER_H_
