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
#include "absl/strings/substitute.h"

#include "nlohmann/json.hpp"
#include "src/fetch.h"
#include "src/json_decode.h"
#include "src/model.h"

ABSL_FLAG(std::optional<std::string>, openai_api_key, std::nullopt,
          "OpenAI API key. If not set, will use the environment variable "
          "OPENAI_API_KEY.");

namespace uchen::chat {
namespace {

class OpenAIModel : public Model {
 public:
  explicit OpenAIModel(std::string_view model, std::string_view api_key,
                       int max_tokens)
      : model_(model), api_key_(api_key), max_tokens_(max_tokens) {}
  ~OpenAIModel() override = default;

  std::string_view name() const override { return model_; }

  absl::StatusOr<std::string> Prompt(
      const Fetch& fetch, std::string_view prompt,
      absl::Span<const std::string_view> input_contents) override;

 private:
  std::string model_;
  std::string api_key_;
  int max_tokens_;
};

absl::StatusOr<std::string> OpenAIModel::Prompt(
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

  auto error = json::JsonDecode(*json_response)["error"];
  if (error.ok()) {
    auto error_message =
        error["message"].String().value_or([&]() { return error->dump(); });
    return absl::InternalError(
        absl::StrCat("OpenAI API error: ", error_message));
  }

  auto message =
      json::JsonDecode(*json_response)["choices"][0]["message"]["content"]
          .String();
  if (!message.ok()) {
    return absl::InternalError(
        absl::StrCat("OpenAI API error: ", message.error()));
  }
  return message.value();
}

class OpenAIModelProvider : public ModelProvider {
 public:
  OpenAIModelProvider(std::shared_ptr<Fetch> fetch, Parameters parameters)
      : fetch_(std::move(fetch)), parameters_(std::move(parameters)) {}
  ~OpenAIModelProvider() override = default;

  std::string_view name() const override { return "OpenAI"; }

  absl::StatusOr<ModelHandle> ConnectToModel(
      std::string_view model) const override {
    if (model.starts_with("claude")) {
      return absl::NotFoundError(absl::Substitute(
          "Model $0 is not supported by OpenAI provider.", model));
    }
    auto api_key = GetOpenAIKey();
    if (!api_key.has_value()) {
      return absl::InvalidArgumentError("API key is required");
    }
    auto client = std::make_unique<OpenAIModel>(model, *api_key,
                                                parameters_.max_tokens());
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
      LOG(ERROR) << "Failed to parse models response: "
                 << json_response.status();
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
  std::optional<std::string> GetOpenAIKey() const {
    if (auto key = absl::GetFlag(FLAGS_openai_api_key); key.has_value()) {
      return key;
    }
    return parameters_.GetEnv("OPENAI_API_KEY");
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
