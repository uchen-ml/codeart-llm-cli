#include "src/anthropic.h"

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
#include "src/fetch.h"
#include "src/json_decode.h"
#include "src/model.h"

ABSL_FLAG(std::optional<std::string>, anthropic_api_key, std::nullopt,
          "Anthropic API key. If not set, will use the environment variable "
          "ANTHROPIC_API_KEY.");

namespace uchen::chat {
namespace {

class AnthropicModel : public Model {
 public:
  AnthropicModel(std::string_view model,
                 std::string_view api_key, int max_tokens)
      : model_(model),
        api_key_(api_key),
        max_tokens_(max_tokens) {}
  ~AnthropicModel() override = default;

  std::string_view name() const override { return model_; }

  absl::StatusOr<std::string> Prompt(
      const Fetch& fetch, std::string_view prompt,
      absl::Span<const std::string_view> input_contents) override;

 private:
  std::string model_;
  std::string api_key_;
  int max_tokens_;
};

absl::StatusOr<std::string> AnthropicModel::Prompt(
    const Fetch& fetch, std::string_view prompt,
    absl::Span<const std::string_view> input_contents) {
  std::string combined_input = absl::StrJoin(input_contents, "\n\n");

  nlohmann::json request = {
      {"model", model_},
      {"max_tokens", max_tokens_},
      {"messages",
       nlohmann::json::array(
           {{{"role", "user"},
             {"content", absl::StrCat(prompt, "\n\n", combined_input)}}})},
  };

  auto response =
      fetch.Post("https://api.anthropic.com/v1/messages",
                 {
                     {.key = "Content-Type", .value = "application/json"},
                     {.key = "x-api-key", .value = api_key_},
                     {.key = "anthropic-version", .value = "2023-06-01"},
                 },
                 request);

  if (!response.ok()) {
    return std::move(response).status();
  }

  auto json_response = response->Json();
  if (!json_response.ok()) {
    return std::move(json_response).status();
  }

  if (json_response->contains("error")) {
    return absl::InternalError(absl::StrCat("Anthropic API error: ",
                                            (*json_response)["error"].dump(2)));
  }

  auto message =
      json::JsonDecode(*json_response)["content"][0]["text"].String();
  if (!message.ok()) {
    return absl::InternalError(
        absl::StrCat("Anthropic API error: ", message.error()));
  }
  return message.value();
}

class AnthropicModelProvider : public ModelProvider {
 public:
  AnthropicModelProvider(std::shared_ptr<Fetch> fetch, Parameters parameters)
      : fetch_(std::move(fetch)), parameters_(std::move(parameters)) {}
  ~AnthropicModelProvider() override = default;

  std::string_view name() const override { return "Anthropic"; }

  absl::StatusOr<ModelHandle> ConnectToModel(
      std::string_view model) const override {
    auto api_key = GetKey();
    if (!api_key.has_value()) {
      return absl::InvalidArgumentError("Anthropic API key is required");
    }
    auto client = std::make_unique<AnthropicModel>(model, *api_key,
                                                   parameters_.max_tokens());
    return ModelHandle(std::move(client));
  }

  std::vector<std::string> ListModels() const override {
    auto api_key = GetKey();
    if (!api_key.has_value()) {
      LOG(INFO) << "Anthropic API key is required to list models.";
      return {};
    }
    auto response =
        fetch_->Get("https://api.anthropic.com/v1/models",
                    {
                        {.key = "x-api-key", .value = *api_key},
                        {.key = "anthropic-version", .value = "2023-06-01"},
                    });

    if (!response.ok()) {
      LOG(ERROR) << "Failed to fetch models: " << response.status();
      return {};
    }

    auto json_response = response->Json();
    if (!json_response.ok()) {
      LOG(ERROR) << "Failed to parse models response: "
                 << json_response.status();
      return {};
    }

    if (json_response->contains("error")) {
      LOG(ERROR) << "Anthropic API returned an error: "
                 << (*json_response)["error"].dump(2);
      return {};
    }

    if (!json_response->contains("data") ||
        !(*json_response)["data"].is_array()) {
      LOG(ERROR) << "Invalid response format from Anthropic API: "
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
  std::optional<std::string> GetKey() const {
    if (auto key = absl::GetFlag(FLAGS_anthropic_api_key); key.has_value()) {
      return key;
    }
    return parameters_.GetEnv("ANTHROPIC_API_KEY");
  }

  std::shared_ptr<Fetch> fetch_;
  Parameters parameters_;
};

}  // namespace

std::unique_ptr<ModelProvider> MakeAnthropicModelProvider(
    std::shared_ptr<Fetch> fetch, Parameters parameters) {
  return std::make_unique<AnthropicModelProvider>(std::move(fetch),
                                                  std::move(parameters));
}

}  // namespace uchen::chat
