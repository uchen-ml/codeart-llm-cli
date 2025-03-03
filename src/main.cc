// Add to the existing imports
#include <filesystem>
#include <fstream>
#include <string_view>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"

#include "src/history.h"
#include "src/interactive_cli.h"

// Define CLI flags
ABSL_FLAG(std::string, api_key, "", "OpenAI API key (required)");
ABSL_FLAG(std::string, model, "", "Model to use");
ABSL_FLAG(std::string, prompt, "", "User prompt for OpenAI");
ABSL_FLAG(std::string, history_file, "chat_history.json",
          "File to store chat history");
ABSL_FLAG(bool, interactive, true, "Run in interactive mode");

namespace codeart::llmcli {
namespace {

absl::StatusOr<History> LoadHistoryFromFile(
    const std::filesystem::path& filename) {
  std::ifstream file(filename);
  if (!file) {
    LOG(WARNING) << "History file not found, starting new history.";
    return History();
  }

  History history;
  file >> history;

  if (file.fail()) {
    return absl::InvalidArgumentError("Failed to parse chat history JSON.");
  }

  return history;
}

absl::Status SaveHistoryToFile(const std::filesystem::path& filename,
                               const History& history) {
  std::ofstream file(filename, std::ios::trunc);
  if (!file) {
    return absl::InternalError("Failed to open file for writing.");
  }

  file << history;
  return absl::OkStatus();
}

absl::Status OneOff(const std::filesystem::path& history_file,
                    std::string_view prompt, std::string_view model) {
  using json = nlohmann::json;
  // Load chat history
  absl::StatusOr<History> history_or =
      codeart::llmcli::LoadHistoryFromFile(history_file);
  if (!history_or.ok()) {
    return std::move(history_or).status();
  }
  History history = history_or.value();

  // Check if input is a command
  if (auto parsed_command = codeart::llmcli::CommandParser::Parse(prompt)) {
    LOG(INFO) << "Recognized command: " << parsed_command->command;
    for (const auto& arg : parsed_command->args) {
      LOG(INFO) << "Arg: " << arg;
    }
    return absl::UnimplementedError(
        "Command handling");  // Command handling logic will be implemented
                              // later.
  }

  // Append new user input
  history.AddEntry({"user", std::chrono::system_clock::now(), "text", prompt});

  // Convert history to OpenAI JSON format
  json chat_history_json;
  chat_history_json["model"] = model;
  json messages;
  for (const auto& entry : history.entries()) {
    std::string role = (entry.author() == "user") ? "user" : "assistant";
    messages.push_back({{"role", role}, {"content", entry.content()}});
  }
  chat_history_json["messages"] = messages;

  // Send request to OpenAI
  // LOG(INFO) << "Sending request to OpenAI with model: " << model;
  // absl::StatusOr<std::string> response =
  //     codeart::llmcli::SendOpenAIRequest(api_key, chat_history_json);
  // if (!response.ok()) {
  //   LOG(ERROR) << response.status().message();
  //   return 1;
  // }
  absl::StatusOr<std::string> response =
      absl::UnimplementedError("Unimplemented");
  json response_json =
      json::parse(*response, nullptr, /*allow_exceptions=*/false);
  if (response_json.is_discarded()) {
    return absl::UnavailableError("Failed to parse response from OpenAI.");
  }

  if (!response_json.contains("choices") || response_json["choices"].empty() ||
      !response_json["choices"][0].contains("message") ||
      !response_json["choices"][0]["message"].contains("content")) {
    return absl::ResourceExhaustedError("Invalid response format from OpenAI.");
  }

  // Extract and print assistant's reply
  std::string assistant_reply =
      response_json["choices"][0]["message"]["content"];
  std::cout << "Assistant: " << assistant_reply << std::endl;

  // Append assistant's reply to chat history
  history.AddEntry(
      {"assistant", std::chrono::system_clock::now(), "text", assistant_reply});

  // Save updated chat history
  absl::Status save_status =
      codeart::llmcli::SaveHistoryToFile(history_file, history);
  if (!save_status.ok()) {
    LOG(ERROR) << "Failed to save chat history: " << save_status.message();
  }
  return absl::OkStatus();
}

}  // namespace
}  // namespace codeart::llmcli

// Update the main function
int main(int argc, char* argv[]) {
  absl::ParseCommandLine(argc, argv);
  absl::InitializeLog();

  absl::SetProgramUsageMessage(
      "Interactive CLI for talking to a language model");

  std::string api_key = absl::GetFlag(FLAGS_api_key);
  std::string model = absl::GetFlag(FLAGS_model);
  std::string history_file = absl::GetFlag(FLAGS_history_file);
  bool interactive = absl::GetFlag(FLAGS_interactive);

  if (api_key.empty()) {
    LOG(ERROR) << "API key is required. Use --api-key=<your_api_key>";
    return 1;
  }

  // Initialize ModelManager
  auto model_manager = codeart::llmcli::ModelManager::Create();

  // Register OpenAI model backend
  // auto openai_model =
  //     std::make_unique<codeart::llmcli::OpenAIModelBackend>(model, api_key);
  // model_manager->RegisterModel(std::move(openai_model));

  // Load history
  auto history_result = codeart::llmcli::LoadHistoryFromFile(history_file);
  if (!history_result.ok()) {
    LOG(ERROR) << "Error loading history: " << history_result.status();
    return 1;
  }
  auto history = std::move(*history_result);

  if (interactive) {
    // Run in interactive mode
    codeart::llmcli::InteractiveCLI cli(model_manager, std::move(history));
    auto status = cli.Run();
    if (!status.ok()) {
      LOG(ERROR) << "Interactive CLI error: " << status;
      return 1;
    }

    // Save history on exit
    auto save_status =
        codeart::llmcli::SaveHistoryToFile(history_file, cli.GetHistory());
    if (!save_status.ok()) {
      LOG(ERROR) << "Failed to save history: " << save_status;
    }
  } else {
    std::string prompt = absl::GetFlag(FLAGS_prompt);
    absl::Status status = codeart::llmcli::OneOff(history_file, prompt, model);
    if (!status.ok()) {
      LOG(ERROR) << "Error: " << status;
      return 1;
    }
  }
  return 0;
}