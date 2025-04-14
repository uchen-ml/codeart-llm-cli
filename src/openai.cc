#include "src/openai.h"

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"

#include "nlohmann/json.hpp"
#include "src/client.h"
#include "src/fetch.h"

ABSL_FLAG(std::optional<std::string>, openai_api_key, std::nullopt,
          "OpenAI API key. If not set, will use the environment variable "
          "OPENAI_API_KEY.");

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
          {.key = "Content-Type", .value = "application/json"},
          {.key = "Authorization", .value = absl::StrCat("Bearer ", api_key_)},
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
    return absl::InternalError(
        absl::StrCat("Invalid response format from OpenAI API. Full response: ",
                     json_response->dump(2)));
  }

  return (*json_response)["choices"][0]["message"]["content"]
      .get<std::string>();
}

class OpenAIModelProvider : public ModelProvider {
 public:
  OpenAIModelProvider(std::shared_ptr<Fetch> fetch, Parameters parameters)
      : fetch_(std::move(fetch)), parameters_(std::move(parameters)) {}
  ~OpenAIModelProvider() override = default;

  std::string_view name() const override { return "OpenAI"; }

  absl::StatusOr<ModelHandle> ConnectToModel() const override {
    auto api_key = GetOpenAIKey();
    if (!api_key.has_value()) {
      return absl::InvalidArgumentError("API key is required");
    }
    auto client = std::make_unique<OpenAIClient>(
        parameters_.model, name(), *api_key, parameters_.max_tokens);
    return ModelHandle(std::move(client));
  }

  std::vector<std::string> ListModels() const override {
    auto api_key = GetOpenAIKey();
    if (!api_key.has_value()) {
      return {};
    }
    auto response =
        fetch_->Get("https://api.openai.com/v1/models",
                    {
                        {.key = "Authorization",
                         .value = absl::StrCat("Bearer ", *api_key)},
                    });
    if (!response.ok()) {
      LOG(ERROR) << "Failed to fetch models: " << response.status();
      return {};
    }
    auto json_response = response->Json();
    if (!json_response.ok()) {
      LOG(ERROR) << "Failed to parse models response: " << json_response.status();
      return {};
    }
    if (json_response->contains("error")) {
      LOG(ERROR) << "OpenAI API returned an error: "
                 << (*json_response)["error"].dump(2);
      return {};
    }
    if (!json_response->contains("data") ||
        !(*json_response)["data"].is_array()) {
      LOG(ERROR) << "Invalid response format from OpenAI API: "
                 << json_response->dump(2);
      return {};
    }
    std::vector<std::string> models;
    for (const auto& model : (*json_response)["data"]) {
      if (model.contains("id") && model["id"].is_string()) {
        models.push_back(model["id"].get<std::string>());
      }
    }
    std::ranges::sort(models);
    return models;
  }

 private:
  static std::optional<std::string> GetOpenAIKey() {
    if (auto key = absl::GetFlag(FLAGS_openai_api_key); key.has_value()) {
      return key;
    }
    return std::nullopt;
  }

  std::shared_ptr<Fetch> fetch_;
  Parameters parameters_;
};

}  // namespace

std::unique_ptr<ModelProvider> MakeOpenAIModelProvider(
    std::shared_ptr<Fetch> fetch, Parameters parameters) {
  return std::make_unique<OpenAIModelProvider>(std::move(fetch),
                                               std::move(parameters));
}

}  // namespace uchen::chat
