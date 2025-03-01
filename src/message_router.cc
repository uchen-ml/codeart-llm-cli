#include "message_router.h"

#include <iostream>
#include <regex>

namespace codeart::llmcli {

MessageRouter::MessageRouter(AgentManager& agent_manager, History& history)
    : agent_manager_(agent_manager), history_(history) {}

void MessageRouter::RouteMessage(const std::string& message) {
  std::regex pattern(R"((#\w+)?\s*(\@\w+)?\s*(.*))");
  std::smatch match;

  if (std::regex_match(message, match, pattern)) {
    std::string channel = match[1].str();
    std::string agent = match[2].str();
    std::string content = match[3].str();

    if (channel.empty()) channel = "#general";
    if (agent.empty()) agent = "@assistant";

    History::Message history_entry{"user", std::chrono::system_clock::now(),
                                   "text", content};
    history_.AddEntry(history_entry);

    std::string response = agent_manager_.SendMessage(agent, content);
    std::cout << agent << ": " << response << std::endl;
  } else {
    std::cerr << "Invalid message format." << std::endl;
  }
}

}  // namespace codeart::llmcli
