#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"
#include "absl/strings/str_format.h"

#include <curl/curl.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <vector>

// Alias for JSON library
using json = nlohmann::json;

// Define CLI flags
ABSL_FLAG(std::string, api_key, "", "OpenAI API key (required)");
ABSL_FLAG(std::string, model, "gpt-4", "OpenAI model to use");
ABSL_FLAG(std::string, prompt, "", "User prompt for OpenAI");
ABSL_FLAG(std::string, history_file, "chat_history.json",
          "File to store chat history");

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  ((std::stringstream *)userp)->write((char *)contents, size * nmemb);
  return size * nmemb;
}

// Load chat history from a file
json LoadChatHistory(const std::string &filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    LOG(WARNING) << "No chat history found. Starting a new session.";
    return json::array();
  }
  json history;
  try {
    file >> history;
  } catch (const std::exception &e) {
    LOG(ERROR) << "Failed to parse chat history: " << e.what();
    return json::array();
  }
  return history;
}

// Save chat history to a file
void SaveChatHistory(const std::string &filename, const json &history) {
  std::ofstream file(filename);
  if (!file.is_open()) {
    LOG(ERROR) << "Failed to save chat history.";
    return;
  }
  file << history.dump(4);
}

// Send a request to OpenAI
std::string SendOpenAIRequest(const std::string &api_key,
                              const std::string &model, json &chat_history) {
  CURL *curl = curl_easy_init();
  if (!curl) {
    LOG(ERROR) << "Failed to initialize CURL";
    return "";
  }

  std::string url = "https://api.openai.com/v1/chat/completions";
  std::string post_data = chat_history.dump();

  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(
      headers, absl::StrFormat("Authorization: Bearer %s", api_key).c_str());
  headers = curl_slist_append(headers, "Content-Type: application/json");

  std::stringstream response;
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);
  curl_slist_free_all(headers);

  if (res != CURLE_OK) {
    LOG(ERROR) << "CURL request failed: " << curl_easy_strerror(res);
    return "";
  }

  return response.str();
}

int main(int argc, char *argv[]) {
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
  json chat_history = LoadChatHistory(history_file);

  // Append new user input
  chat_history.push_back({{"role", "user"}, {"content", prompt}});

  // Send request to OpenAI
  LOG(INFO) << "Sending request to OpenAI with model: " << model;
  json request_payload = {{"model", model}, {"messages", chat_history}};
  std::string response_str = SendOpenAIRequest(api_key, model, request_payload);

  if (response_str.empty()) {
    LOG(ERROR) << "Failed to get a response from OpenAI.";
    return 1;
  }

  // Parse response
  json response_json;
  try {
    response_json = json::parse(response_str);
  } catch (const std::exception &e) {
    LOG(ERROR) << "Failed to parse response: " << e.what();
    return 1;
  }

  if (!response_json.contains("choices") || response_json["choices"].empty()) {
    LOG(ERROR) << "Invalid response format from OpenAI.";
    return 1;
  }

  // Extract and print assistant's reply
  std::string assistant_reply =
      response_json["choices"][0]["message"]["content"];
  std::cout << "Assistant: " << assistant_reply << std::endl;

  // Append assistant's reply to chat history and save
  chat_history.push_back({{"role", "assistant"}, {"content", assistant_reply}});
  SaveChatHistory(history_file, chat_history);

  return 0;
}
