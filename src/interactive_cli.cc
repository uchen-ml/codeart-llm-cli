#include "src/interactive_cli.h"

#include <iostream>
#include <string>
#include <string_view>

#include "absl/log/log.h"
#include "absl/status/status.h"

#include "src/command_parser.h"

namespace codeart::llmcli {

InteractiveCLI::InteractiveCLI(std::shared_ptr<ModelManager> model_manager,
                               History history)
    : model_manager_(std::move(model_manager)), history_(std::move(history)) {
  // Initialize chat with callback
  chat_ = std::make_unique<Chat>(
      model_manager_,
      [this](const Message& message) { OnModelResponse(message); });
}

InteractiveCLI::~InteractiveCLI() = default;

absl::Status InteractiveCLI::Run() {
  running_ = true;
  DisplayHistory();

  while (running_) {
    DisplayPrompt();

    // Read user input
    std::string input;
    std::getline(std::cin, input);

    // Process special commands
    if (input == "/quit" || input == "/exit") {
      running_ = false;
      continue;
    }

    absl::Status status = ProcessInput(input);
    if (!status.ok()) {
      return status;
    }
  }

  return absl::OkStatus();
}

absl::Status InteractiveCLI::ProcessCommand(
    const CommandParser::ParsedCommand& command) {
  // Handle command
  LOG(INFO) << "Recognized command: " << command.command;

  // Handle special commands
  if (command.command == "clear") {
    // Clear screen
    std::cout << "\033[2J\033[1;1H";
    return absl::OkStatus();
  }

  // Other commands will be dispatched to the appropriate handler
  // For now, just acknowledge
  std::cout << "Command recognized: " << command.command << std::endl;
  return absl::OkStatus();
}

absl::Status InteractiveCLI::SendMessage(std::string_view message) {
  // Regular message to the model
  Message user_message(message, true);

  // Add to history
  history_.AddEntry(History::Message{"user", std::chrono::system_clock::now(),
                                     "text", message});

  // Send to chat
  auto status = chat_->SendMessage(user_message);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to send message: " << status;
    std::cout << "Error: Failed to send message to model" << std::endl;
  }
  return absl::OkStatus();
}

absl::Status InteractiveCLI::ProcessInput(std::string_view input) {
  // Check if input is a command
  if (auto parsed_command = CommandParser::Parse(input);
      parsed_command.has_value()) {
    return ProcessCommand(*parsed_command);
  }
  return SendMessage(input);
}

void InteractiveCLI::OnModelResponse(const Message& message) {
  // Add to history
  history_.AddEntry(History::Message{"assistant",
                                     std::chrono::system_clock::now(), "text",
                                     message.contents()});

  // Display the response
  std::cout << "\nAssistant: " << message.contents() << std::endl;
}

void InteractiveCLI::DisplayHistory() {
  // Clear screen
  std::cout << "\033[2J\033[1;1H";

  // Display last N entries from history
  constexpr int kMaxHistoryToShow = 10;
  const auto& entries = history_.entries();

  size_t start_idx = (entries.size() > kMaxHistoryToShow)
                         ? entries.size() - kMaxHistoryToShow
                         : 0;

  for (size_t i = start_idx; i < entries.size(); ++i) {
    const auto& entry = entries[i];
    std::string role = entry.author() == "user" ? "You" : "Assistant";
    std::cout << role << ": " << entry.content() << "\n\n";
  }
}

// static
void InteractiveCLI::DisplayPrompt() {
  std::cout << "\nYou: ";
  std::cout.flush();
}

}  // namespace codeart::llmcli