#include <cstdlib>
#include <string>
#include <string_view>

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"

#include "nlohmann/json.hpp"
#include "src/client.h"
#include "src/fetch.h"

namespace uchen::chat {
namespace {

class OpenAIClient : public Client {
 public:
  explicit OpenAIClient(std::string_view model, std::string_view name,
                        std::string_view api_key, int max_tokens)
      : model_(model),
        name_(name),
        api_key_(api_key),
        max_tokens_(max_tokens) {}
  ~OpenAIClient() override = default;

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

absl::StatusOr<std::string> OpenAIClient::Query(
    const Fetch& fetch, std::string_view prompt,
    absl::Span<const std::string_view> input_contents) {
  std::string combined_input = absl::StrJoin(input_contents, "\n\n");

  auto response = fetch.Post(
      "https://api.openai.com/v1/chat/completions",
      {
          {"Content-Type", "application/json"},
          {"Authorization", absl::StrCat("Bearer ", api_key_)},
      },
      {
        {"model", model_},
        {"max_tokens", max_tokens_},
        {"messages",
         nlohmann::json::array({
           {
             {"role", "user"},
             {"content", absl::StrCat(prompt, "\n\n", combined_input)}
           }
         })}
      });

  if (!response.ok()) {
    return std::move(response).status();
  }

  auto json_response = response->Json();

  if (!json_response.ok()) {
    return std::move(json_response).status();
  }

  if (json_response->contains("error")) {
    if ((*json_response)["error"].contains("message") &&
        (*json_response)["error"]["message"].is_string()) {
      return absl::InternalError(absl::StrCat(
          "OpenAI API error: ",
          (*json_response)["error"]["message"].get<std::string>()));
    }
    return absl::InternalError("OpenAI API returned an error");
  }

  if (!json_response->contains("choices") ||
      !(*json_response)["choices"].is_array() ||
      (*json_response)["choices"].empty() ||
      !(*json_response)["choices"][0]["message"].contains("content") ||
      !(*json_response)["choices"][0]["message"]["content"].is_string()) {
    return absl::InternalError(absl::StrCat(
      "Invalid response format from OpenAI API. Full response: ",
      json_response->dump(2)));
  }

  return (*json_response)["choices"][0]["message"]["content"].get<std::string>();
}

class OpenAIFactory {
 public:
  OpenAIFactory(std::string_view model, std::string_view name)
      : model_(model), name_(name) {}

  absl::StatusOr<std::unique_ptr<Client>> operator()(
      const Parameters& parameters) const {
    if (!parameters.api_key.has_value()) {
      return absl::InvalidArgumentError("Missing 'api_key' parameter");
    }
    return std::make_unique<OpenAIClient>(model_, name_, *parameters.api_key,
                                          parameters.max_tokens);
  }

 private:
  std::string_view model_;
  std::string_view name_;
};

}  // namespace

std::unordered_map<std::string_view, ClientFactory> OpenAIClients() {
  std::unordered_map<std::string_view, ClientFactory> clients;
  using std::string_view_literals::operator""sv;
  static constexpr std::array kOpenAIModels = {
      std::tuple{"gpt-4"sv, "gpt-4-turbo-2024-04-09"sv, "GPT-4 Turbo"sv},
      std::tuple{"gpt-3.5"sv, "gpt-3.5-turbo-0125"sv, "GPT-3.5 Turbo"sv},
  };
  for (const auto& [id, model, name] : kOpenAIModels) {
    clients.emplace(id, OpenAIFactory(model, name));
  }
  return clients;
}

}  // namespace uchen::chat
