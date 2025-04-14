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
  std::copy(ptr, ptr + (size * nmemb), std::back_inserter(response->body_));
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

absl::StatusOr<Response> CurlFetch::Get(
    const std::string& url, absl::Span<const Header> headers) const {
  return CurlFetch::Request(HttpMethod::kGet, url, headers, {});
}

absl::StatusOr<Response> CurlFetch::Post(const std::string& url,
                                         absl::Span<const Header> headers,
                                         const nlohmann::json& payload) const {
  const std::string payload_str = payload.dump();
  std::span<const char> payload_span(payload_str.data(), payload_str.size());
  return CurlFetch::Request(HttpMethod::kPost, url, headers, payload_span);
}

absl::StatusOr<Response> CurlFetch::Request(HttpMethod method,
                                            const std::string& url,
                                            absl::Span<const Header> headers,
                                            std::span<const char> payload) {
  Response response;
  CURL* curl = curl_easy_init();
  if (!curl) return absl::InternalError("curl_easy_init failed");
  absl::Cleanup curl_cleanup = [curl] { curl_easy_cleanup(curl); };

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Response::CurlWriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

  struct curl_slist* curl_headers = nullptr;
  absl::Cleanup headers_cleanup = [&curl_headers] {
    curl_slist_free_all(curl_headers);
  };

  for (const Header& header : headers) {
    VLOG(kHeadersLog) << absl::StrCat(header.key, ": ", header.value);
    curl_headers = curl_slist_append(
        curl_headers, absl::StrCat(header.key, ": ", header.value).c_str());
  }
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);

  switch (method) {
    case HttpMethod::kGet:
      if (payload.size() > 0) {
        return absl::InvalidArgumentError(
            "GET method does not support payload");
      }
      break;
    case HttpMethod::kPost:
      if (payload.size() == 0) {
        return absl::InvalidArgumentError("POST method requires payload");
      }
      curl_easy_setopt(curl, CURLOPT_POST, 1L);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.data());
      curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, payload.size());
      break;
    default:
      return absl::InvalidArgumentError("Unsupported HTTP method");
  }

  VLOG(kMaxLogLevel) << (method == HttpMethod::kPost ? "POST " : "GET ") << url;
  if (payload.size()) {
    VLOG(kMaxLogLevel) << "Payload: "
                       << std::string_view(payload.data(), payload.size());
  }

  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    return absl::InternalError(
        absl::StrCat("Failed to perform request: ", curl_easy_strerror(res)));
  }

  VLOG(kMaxLogLevel) << "Response: " << absl::StrCat(response);
  return response;
}

}  // namespace uchen::chat