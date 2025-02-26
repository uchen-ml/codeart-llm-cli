#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"

// Define CLI flags
ABSL_FLAG(std::string, api_key, "", "OpenAI API key (required)");
ABSL_FLAG(std::string, model, "gpt-4", "OpenAI model to use");
ABSL_FLAG(std::string, prompt, "", "User prompt for OpenAI");
ABSL_FLAG(std::string, history_file, "chat_history.json",
          "File to store chat history");

namespace codeart::llmcli {
namespace {

// Alias for JSON library
using json = nlohmann::json;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
  auto* stream = static_cast<std::stringstream*>(userp);
  stream->write(static_cast<const char*>(contents), size * nmemb);
  return size * nmemb;
}

// Load chat history from a file
absl::StatusOr<json> LoadChatHistory(const std::string& filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    return absl::NotFoundError(
        absl::StrFormat("Chat history file not found: %s", filename));
  }

  json history;
  file >> history;
  if (file.fail()) {
    return absl::InvalidArgumentError(
        absl::StrFormat("Failed to parse JSON history in file: %s", filename));
  }

  return history;
}

// Save chat history to a file
absl::Status SaveChatHistory(const std::string& filename, const json& history) {
  std::ofstream file(filename);
  if (!file.is_open()) {
    return absl::InternalError(
        absl::StrFormat("Failed to open file for writing: %s", filename));
  }

  file << history.dump(4);
  if (file.fail()) {
    return absl::InternalError(
        absl::StrFormat("Failed to write chat history to file: %s", filename));
  }

  return absl::OkStatus();
}

// Send a request to OpenAI
absl::StatusOr<std::string> SendOpenAIRequest(const std::string& api_key,
                                              const json& chat_history) {
  CURL* curl = curl_easy_init();
  if (!curl) {
    return absl::InternalError("Failed to initialize CURL");
  }

  constexpr char kUrl[] = "https://api.openai.com/v1/chat/completions";
  const std::string post_data = chat_history.dump();

  struct curl_slist* headers = nullptr;
  headers = curl_slist_append(
      headers, absl::StrFormat("Authorization: Bearer %s", api_key).c_str());
  headers = curl_slist_append(headers, "Content-Type: application/json");

  std::stringstream response;
  curl_easy_setopt(curl, CURLOPT_URL, kUrl);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

  CURLcode res = curl_easy_perform(curl);

  curl_easy_cleanup(curl);
  curl_slist_free_all(headers);  // Moved after curl cleanup

  if (res != CURLE_OK) {
    return absl::InternalError(
        absl::StrFormat("CURL request failed: %s", curl_easy_strerror(res)));
  }

  return response.str();
}

}  // namespace
}  // namespace codeart::llmcli

int main(int argc, char* argv[]) {
  using json = nlohmann::json;
  absl::ParseCommandLine(argc, argv);
  absl::InitializeLog();

  std::string api_key = absl::GetFlag(FLAGS_api_key);
  std::string model = absl::GetFlag(FLAGS_model);
  std::string prompt = absl::GetFlag(FLAGS_prompt);
  std::string history_file = absl::GetFlag(FLAGS_history_file);

  if (api_key.empty()) {
    LOG(ERROR) << "API key is required. Use --api-key=<your_api_key>";
    return 1;
  }

  // Load chat history
  absl::StatusOr<json> chat_history_or =
      codeart::llmcli::LoadChatHistory(history_file);
  json chat_history =
      chat_history_or.ok() ? std::move(*chat_history_or) : json::array();
  if (!chat_history_or.ok()) {
    LOG(WARNING) << chat_history_or.status().message();
  }

  // Append new user input
  chat_history.push_back({{"role", "user"}, {"content", prompt}});

  // Construct request payload
  json request_payload;
  request_payload["model"] = model;
  request_payload["messages"] = chat_history;

  // Send request to OpenAI
  LOG(INFO) << "Sending request to OpenAI with model: " << model;

  absl::StatusOr<std::string> response =
      codeart::llmcli::SendOpenAIRequest(api_key, request_payload);
  if (!response.ok()) {
    LOG(ERROR) << response.status().message();
    return 1;
  }

  json response_json =
      json::parse(*response, nullptr, /*allow_exceptions=*/false);
  if (response_json.is_discarded()) {
    LOG(ERROR) << "Failed to parse response from OpenAI.";
    return 1;
  }

  if (!response_json.contains("choices") || response_json["choices"].empty() ||
      !response_json["choices"][0].contains("message") ||
      !response_json["choices"][0]["message"].contains("content")) {
    LOG(ERROR) << "Invalid response format from OpenAI.";
    return 1;
  }

  // Extract and print assistant's reply
  std::string assistant_reply =
      response_json["choices"][0]["message"]["content"];
  std::cout << "Assistant: " << assistant_reply << std::endl;

  // Append assistant's reply to chat history
  chat_history.push_back({{"role", "assistant"}, {"content", assistant_reply}});

  // Save updated chat history
  absl::Status save_status =
      codeart::llmcli::SaveChatHistory(history_file, chat_history);
  if (!save_status.ok()) {
    LOG(ERROR) << "Failed to save chat history: " << save_status.message();
  }

  return 0;
}
