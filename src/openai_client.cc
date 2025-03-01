#include "openai_client.h"

#include <curl/curl.h>

#include "absl/log/log.h"

namespace codeart::llmcli {

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
  auto* stream = static_cast<std::stringstream*>(userp);
  stream->write(static_cast<const char*>(contents), size * nmemb);
  return size * nmemb;
}

absl::StatusOr<std::string> OpenAIClient::SendMessage(
    const std::string& message) {
  CURL* curl = curl_easy_init();
  if (!curl) return absl::InternalError("Failed to initialize CURL");

  constexpr char kUrl[] = "https://api.openai.com/v1/chat/completions";
  std::string post_data =
      R"({"model": "gpt-4", "messages": [{"role": "user", "content": ")" +
      message + R"("}]})";

  struct curl_slist* headers = nullptr;
  headers = curl_slist_append(headers, "Authorization: Bearer YOUR_API_KEY");
  headers = curl_slist_append(headers, "Content-Type: application/json");

  std::stringstream response;
  curl_easy_setopt(curl, CURLOPT_URL, kUrl);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

  CURLcode res = curl_easy_perform(curl);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    return absl::InternalError(
        absl::StrFormat("CURL request failed: %s", curl_easy_strerror(res)));
  }

  return response.str();
}

}  // namespace codeart::llmcli
