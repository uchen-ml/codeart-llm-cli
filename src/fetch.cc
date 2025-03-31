#include "src/fetch.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "absl/cleanup/cleanup.h"
#include "absl/log/log.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"

#include "curl/curl.h"

namespace uchen::chat {

namespace {
constexpr uint16_t kMaxLogLevel = 5;
constexpr uint16_t kHeadersLog = 3;
}  // namespace

// Write callback function for CURL
size_t Response::CurlWriteCallback(char* ptr, size_t size, size_t nmemb,
                                   void* userdata) {
  auto* response = static_cast<Response*>(userdata);
  std::copy(ptr, ptr + size * nmemb, std::back_inserter(response->body_));
  return size * nmemb;
}

absl::StatusOr<nlohmann::json> Response::Json() const {
  // Parse response without exceptions
  nlohmann::json json_response = nlohmann::json::parse(body_, nullptr, false);

  if (json_response.is_discarded()) {
    return absl::InternalError(
        absl::StrCat("Failed to parse JSON: ",
                     std::string_view(body_.data(), body_.size())));
  }
  return json_response;
}

absl::StatusOr<Response> CurlFetch::Post(const std::string& url,
                                         absl::Span<const Header> headers,
                                         const nlohmann::json& payload) const {
  Response response;
  VLOG(kMaxLogLevel) << "POST " << url << " with payload: " << payload.dump();
  CURL* curl = curl_easy_init();
  absl::Cleanup curl_cleanup = [curl] { curl_easy_cleanup(curl); };
  std::string payload_str = payload.dump();
  // Set up CURL options
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload_str.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, payload_str.size());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Response::CurlWriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

  // Set headers
  struct curl_slist* curl_headers = nullptr;
  absl::Cleanup headers_cleanup = [&curl_headers] {
    curl_slist_free_all(curl_headers);
  };

  for (const Header& header : headers) {
    VLOG(kHeadersLog) << absl::StrCat(header.key + ": " + header.value);
    curl_headers = curl_slist_append(
        curl_headers, absl::StrCat(header.key + ": " + header.value).c_str());
  }
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);

  // Perform request
  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    return absl::InternalError(
        absl::StrCat("Failed to perform request: ", curl_easy_strerror(res)));
  }
  VLOG(kMaxLogLevel) << "Response: " << absl::StrCat(response);
  return response;
}

}  // namespace uchen::chat