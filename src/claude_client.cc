#include "src/claude_client.h"

#include <cstdlib>
#include <string>
#include <string_view>

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"

#include "nlohmann/json.hpp"
#include "src/fetch.h"

namespace processfile {

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
    return absl::InternalError("Invalid response format from Claude API");
  }

  return (*json_response)["content"][0]["text"].get<std::string>();
}

}  // namespace processfile