#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"

#include <curl/curl.h>
#include <sstream>

// Define CLI flags
ABSL_FLAG(std::string, api_key, "", "OpenAI API key (required)");
ABSL_FLAG(std::string, model, "gpt-4", "OpenAI model to use");
ABSL_FLAG(std::string, prompt, "", "User prompt for OpenAI");

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  ((std::stringstream *)userp)->write((char *)contents, size * nmemb);
  return size * nmemb;
}

void SendOpenAIRequest(const std::string &api_key, const std::string &model,
                       const std::string &prompt) {
  CURL *curl = curl_easy_init();
  if (!curl) {
    LOG(ERROR) << "Failed to initialize CURL";
    return;
  }

  std::string url = "https://api.openai.com/v1/chat/completions";
  std::string post_data = R"({"model":")" + model +
                          R"(","messages":[{"role":"user","content":")" +
                          prompt + R"("}]})";

  struct curl_slist *headers = nullptr;
  headers =
      curl_slist_append(headers, ("Authorization: Bearer " + api_key).c_str());
  headers = curl_slist_append(headers, "Content-Type: application/json");

  std::stringstream response;
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    LOG(ERROR) << "CURL request failed: " << curl_easy_strerror(res);
  } else {
    LOG(INFO) << "OpenAI response: " << response.str();
  }

  curl_easy_cleanup(curl);
  curl_slist_free_all(headers);
}

int main(int argc, char *argv[]) {
  absl::ParseCommandLine(argc, argv);
  absl::InitializeLog();

  std::string api_key = absl::GetFlag(FLAGS_api_key);
  std::string model = absl::GetFlag(FLAGS_model);
  std::string prompt = absl::GetFlag(FLAGS_prompt);

  if (api_key.empty() || prompt.empty()) {
    LOG(ERROR) << "API key and prompt are required. Use "
                  "--api-key=<your_api_key> --prompt=\"Your message\"";
    return 1;
  }

  LOG(INFO) << "Sending request to OpenAI with model: " << model;
  SendOpenAIRequest(api_key, model, prompt);

  return 0;
}
