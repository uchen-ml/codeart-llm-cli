#include "src/models/openai.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"

#include "curl/curl.h"
#include "nlohmann/json.hpp"

namespace codeart::llmcli {

namespace {
// Callback function for CURL to handle response data
size_t WriteCallback(void* contents, size_t size, size_t nmemb,
                     std::string* response) {
  size_t total_size = size * nmemb;
  response->append(static_cast<char*>(contents), total_size);
  return total_size;
}
}  // namespace

// Define destructor outside of the class
OpenAIModelBackend::~OpenAIModelBackend() {
  // No need for curl_global_cleanup here since we're using the CURL
  // handle for each request separately
}

absl::StatusOr<Message> OpenAIModelBackend::SendMessage(
    const Message& message) {
  // Initialize CURL for this request
  curl_global_init(CURL_GLOBAL_ALL);
  CURL* curl = curl_easy_init();
  if (!curl) {
    curl_global_cleanup();
    return absl::InternalError("Failed to initialize CURL");
  }

  // Prepare messages for the API request
  nlohmann::json request_json;

  // Set model and parameters
  request_json["model"] = config_.model_name;
  request_json["temperature"] = config_.temperature;
  request_json["max_tokens"] = config_.max_tokens;

  // Format message history - assuming message contains conversation history
  std::vector<nlohmann::json> messages;

  // Add the current message and its context
  for (const auto& msg : message.context()) {
    nlohmann::json json_msg;
    json_msg["role"] = msg.is_user() ? "user" : "assistant";
    json_msg["content"] = msg.contents();
    messages.push_back(json_msg);
  }

  // Add the current message
  nlohmann::json current_msg;
  current_msg["role"] = "user";
  current_msg["content"] = message.contents();
  messages.push_back(current_msg);

  request_json["messages"] = messages;

  // Convert to string
  std::string request_body = request_json.dump();

  std::string response_data;

  // Set CURL options
  struct curl_slist* headers = NULL;
  headers = curl_slist_append(headers, "Content-Type: application/json");
  std::string auth_header =
      absl::StrCat("Authorization: Bearer ", config_.api_key);
  headers = curl_slist_append(headers, auth_header.c_str());

  curl_easy_setopt(curl, CURLOPT_URL, config_.api_url.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

  // Perform request
  CURLcode res = curl_easy_perform(curl);

  // Clean up
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  curl_global_cleanup();

  if (res != CURLE_OK) {
    return absl::InternalError(
        absl::StrFormat("CURL request failed: %s", curl_easy_strerror(res)));
  }

  // Parse response
  nlohmann::json response_json;

  // Safe parsing without exceptions
  auto parse_result = nlohmann::json::parse(response_data, nullptr, false);
  if (parse_result.is_discarded()) {
    return absl::InternalError("Failed to parse API response");
  }

  response_json = parse_result;

  // Check for errors in the response
  if (response_json.contains("error")) {
    return absl::InternalError(absl::StrFormat(
        "API error: %s", response_json["error"]["message"].get<std::string>()));
  }

  // Extract the response message
  if (!response_json.contains("choices") || response_json["choices"].empty()) {
    return absl::InternalError("Empty response from OpenAI API");
  }

  std::string response_message =
      response_json["choices"][0]["message"]["content"];

  // Create and return the response message
  Message response(response_message, false);

  return response;
}

absl::StatusOr<ModelCapabilities> OpenAIModelBackend::GetCapabilities() const {
  ModelCapabilities capabilities;
  capabilities.supports_streaming =
      false;  // Set to true if implementing streaming
  capabilities.max_context_length = 4096;  // Adjust based on model

  // You can add more capability details based on the specific model
  if (config_.model_name.find("gpt-4") != std::string::npos) {
    capabilities.max_context_length = 8192;  // For GPT-4 models
  }

  return capabilities;
}

}  // namespace codeart::llmcli