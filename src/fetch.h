#ifndef SRC_FETCH_H_
#define SRC_FETCH_H_

#include "absl/status/statusor.h"
#include "absl/types/span.h"

#include "nlohmann/json.hpp"  // IWYU pragma: keep

namespace uchen::chat {

struct Header {
  std::string key;
  std::string value;
};

class Response {
 public:
  static size_t CurlWriteCallback(char* ptr, size_t size, size_t nmemb,
                                  void* userdata);

  absl::StatusOr<nlohmann::json> Json() const;

  template <typename Sink>
  friend void AbslStringify(Sink& sink, const Response& response) {
    auto json = response.Json();
    if (json.ok()) {
      sink.Append(json->dump(2));
    } else {
      sink.Append(
          std::string_view(response.body_.data(), response.body_.size()));
    }
  }

 private:
  std::vector<char> body_;
};

class Fetch {
 public:
  virtual ~Fetch() = default;

  virtual absl::StatusOr<Response> Post(
      const std::string& url, absl::Span<const Header> headers,
      const nlohmann::json& payload) const = 0;

  virtual absl::StatusOr<Response> Get(
      const std::string& url, absl::Span<const Header> headers) const = 0;
};

class CurlFetch : public Fetch {
 public:
 absl::StatusOr<Response> Get(const std::string& url,
                              absl::Span<const Header> headers) const override;
  absl::StatusOr<Response> Post(const std::string& url,
                                absl::Span<const Header> headers,
                                const nlohmann::json& payload) const override;
};

}  // namespace uchen::chat

#endif  // SRC_FETCH_H_