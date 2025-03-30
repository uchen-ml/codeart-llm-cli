#ifndef SRC_PROCESSFILE_FETCH_H_
#define SRC_PROCESSFILE_FETCH_H_

#include "absl/status/statusor.h"
#include "absl/types/span.h"

#include "nlohmann/json.hpp"  // IWYU pragma: keep

namespace processfile {

struct Header {
  std::string key;
  std::string value;
};

class Response {
 public:
  static size_t CurlWriteCallback(char* ptr, size_t size, size_t nmemb,
                                  void* userdata);

  absl::StatusOr<nlohmann::json> Json() const;

 private:
  std::vector<char> body_;
};

class Fetch {
 public:
  virtual ~Fetch() = default;

  virtual absl::StatusOr<Response> Post(
      const std::string& url, absl::Span<const Header> headers,
      const nlohmann::json& payload) const = 0;
};

class CurlFetch : public Fetch {
 public:
  absl::StatusOr<Response> Post(const std::string& url,
                                absl::Span<const Header> headers,
                                const nlohmann::json& payload) const override;
};

}  // namespace processfile

#endif  // SRC_PROCESSFILE_FETCH_H_