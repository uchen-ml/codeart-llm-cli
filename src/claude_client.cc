#include "src/claude_client.h"

#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"

#include "nlohmann/json.hpp"
#include "src/fetch.h"

namespace uchen::chat {
namespace {

class ClaudeClient : public Client {
 public:
  explicit ClaudeClient(std::string_view model, std::string_view name,
                        std::string_view api_key, int max_tokens)
      : model_(model),
        name_(name),
        api_key_(api_key),
        max_tokens_(max_tokens) {}
  ~ClaudeClient() override = default;

  std::string_view name() const override { return name_; }

  absl::StatusOr<std::string> Query(
      const Fetch& fetch, std::string_view prompt,
      absl::Span<const std::string_view> input_contents) override;

 private:
  std::string model_;
  std::string name_;
  std::string api_key_;
  int max_tokens_;
};

absl::StatusOr<std::string> ClaudeClient::Query(
    const Fetch& fetch, std::string_view prompt,
    absl::Span<const std::string_view> input_contents) {
  // Join input contents with newlines
  std::string combined_input = absl::StrJoin(input_contents, "\n\n");

  // Prepare request payload
  auto response = fetch.Post(
      "https://api.anthropic.com/v1/messages",
      {
          {"Content-Type", "application/json"},
          {"X-API-Key", api_key_},
          {"anthropic-version", "2023-06-01"},
      },
      {{"model", model_},
       {"max_tokens", max_tokens_},
       {"messages",
        nlohmann::json::array(
            {{{"role", "user"},
              {"content", absl::StrCat(prompt, "\n\n", combined_input)}}})}});

  if (!response.ok()) {
    return std::move(response).status();
  }

  // Parse response without exceptions
  auto json_response = response->Json();

  if (!json_response.ok()) {
    return std::move(json_response).status();
  }

  if (json_response->contains("error")) {
    if ((*json_response)["error"].contains("message") &&
        (*json_response)["error"]["message"].is_string()) {
      return absl::InternalError(absl::StrCat(
          "Claude API error: ",
          (*json_response)["error"]["message"].get<std::string>()));
    }
    return absl::InternalError("Claude API returned an error");
  }

  if (!(*json_response).contains("content") ||
      !(*json_response)["content"].is_array() ||
      (*json_response)["content"].empty() ||
      !(*json_response)["content"][0].contains("text") ||
      !(*json_response)["content"][0]["text"].is_string()) {
    return absl::InternalError(
        absl::StrCat("Invalid response format from Claude API. Full response: ",
                     json_response->dump(2)));
  }

  return (*json_response)["content"][0]["text"].get<std::string>();
}

class ClaudeFactory : public ClientFactory {
 public:
  ClaudeFactory(std::string_view model, std::string_view name)
      : model_(model), name_(name) {}

  absl::StatusOr<std::unique_ptr<Client>> create_client(
      const Parameters& parameters) const override {
    if (!parameters.api_key.has_value()) {
      return absl::InvalidArgumentError("Missing 'api_key' parameter");
    }
    return std::make_unique<ClaudeClient>(model_, name_, *parameters.api_key,
                                          parameters.max_tokens);
  }

  absl::StatusOr<std::vector<std::string>> list_models(
      const Parameters&) const override {
    return std::vector<std::string>{""};
  }

 private:
  std::string_view model_;
  std::string_view name_;
};

}  // namespace

std::unordered_map<std::string_view, std::unique_ptr<ClientFactory>>
ClaudeClients() {
  std::unordered_map<std::string_view, std::unique_ptr<ClientFactory>> clients;
  using std::string_view_literals::operator""sv;
  static constexpr std::array kClaudeModels = {
      std::tuple{"claude"sv, "claude-3-5-haiku-20241022"sv,
                 "Claude 3.5 Haiku"sv},
      std::tuple{"haiku"sv, "claude-3-5-haiku-20241022"sv,
                 "Claude 3.5 Haiku"sv},
      std::tuple{"sonnet"sv, "claude-3-7-sonnet-20250219"sv,
                 "Claude 3.7 Sonnet"sv},
  };
  for (const auto& [id, model, name] : kClaudeModels) {
    clients.emplace(id, std::make_unique<ClaudeFactory>(model, name));
  }
  return clients;
}

}  // namespace uchen::chat