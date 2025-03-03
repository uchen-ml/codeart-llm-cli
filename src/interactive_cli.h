#ifndef CODEART_LLMCLI_INTERACTIVE_CLI_H_
#define CODEART_LLMCLI_INTERACTIVE_CLI_H_

#include <memory>
#include <string_view>

#include "absl/status/status.h"

#include "src/chat.h"
#include "src/command_parser.h"
#include "src/history.h"

namespace codeart::llmcli {

class InteractiveCLI {
 public:
  InteractiveCLI(std::shared_ptr<ModelManager> model_manager, History history);
  ~InteractiveCLI();

  // Run the interactive CLI loop
  absl::Status Run();

  // Get the current history (for saving externally)
  const History& GetHistory() const { return history_; }

 private:
  // Display prompt and input area
  static void DisplayPrompt();

  // Display chat history
  void DisplayHistory();

  // Handle user input
  absl::Status ProcessInput(std::string_view input);

  // Callback for receiving model responses
  void OnModelResponse(const Message& message);

  absl::Status SendMessage(std::string_view message);

  absl::Status ProcessCommand(const CommandParser::ParsedCommand& command);

  std::shared_ptr<ModelManager> model_manager_;
  std::unique_ptr<Chat> chat_;
  History history_;
  bool running_ = false;
};

}  // namespace codeart::llmcli

#endif  // CODEART_LLMCLI_INTERACTIVE_CLI_H_