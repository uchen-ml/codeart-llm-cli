#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/functional/any_invocable.h"
#include "absl/log/initialize.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_split.h"
#include "absl/strings/substitute.h"

#include "curl/curl.h"
#include "src/claude_client.h"
#include "src/client.h"
#include "src/input.h"
#include "src/openai_client.h"
#include "src/tui.h"

namespace uchen::chat {
namespace {

std::map<std::string_view, ClientFactory> PrepareClients() {
  auto anthropic_clients = ClaudeClients();
  auto openai_clients = OpenAIClients();
  std::map<std::string_view, ClientFactory> all_clients;
  all_clients.insert(std::make_move_iterator(anthropic_clients.begin()),
                     std::make_move_iterator(anthropic_clients.end()));
  all_clients.insert(std::make_move_iterator(openai_clients.begin()),
                     std::make_move_iterator(openai_clients.end()));
  return all_clients;
}

int Chat(Client* client) {
  std::cout << "Uchen Chat CLI. Type your message below:";
  uchen::chat::InputReader reader(std::cin);
  while (true) {
    std::cout << "\n> ";
    auto prompt = reader();
    if (prompt == std::nullopt) {
      return 0;
    }
    if (!prompt->empty()) {
      uchen::chat::CurlFetch fetch;
      auto response =
          SpinWhile([&]() { return client->Query(fetch, *prompt, {}); });
      if (!response.ok()) {
        std::cerr << "Error: " << response.status().message() << std::endl;
        return 1;
      }
      std::cout << *response << std::endl;
    }
  }
}

}  // namespace
}  // namespace uchen::chat

ABSL_FLAG(std::optional<std::string>, api_key, std::nullopt, "API key.");
ABSL_FLAG(std::string, model, "gpt-3.5",
          "A well known model or provider:model tuple.");

int main(int argc, char* argv[]) {
  curl_global_init(CURL_GLOBAL_ALL);
  std::vector<std::string> segments =
      absl::StrSplit(argv[0], absl::ByAnyChar("/\\"));
  absl::SetProgramUsageMessage(
      absl::Substitute("Terminal chat client for LLM APIs.\n"
                       "Usage: $0 --model=<model> [--api_key=<key>]",
                       segments.back()));
  std::vector<char*> positional_args = absl::ParseCommandLine(argc, argv);
  absl::InitializeLog();

  std::pair<std::string, std::string> provider_model =
      absl::StrSplit(absl::GetFlag(FLAGS_model), ':');

  auto factories = uchen::chat::PrepareClients();
  auto it = factories.find(provider_model.first);
  if (it == factories.end()) {
    std::cerr << absl::Substitute(
        "Error: Unknown model: $0. Known models: $1\n",
        absl::GetFlag(FLAGS_model),
        absl::StrJoin(factories, ", ", [](std::string* out, const auto& entry) {
          absl::StrAppend(out, entry.first);
        }));
    return 1;
  }

  uchen::chat::Parameters parameters = {
      .model = provider_model.second,
      .provider = provider_model.first,
      .api_key = absl::GetFlag(FLAGS_api_key),
  };

  auto client = it->second(parameters);
  if (!client.ok()) {
    std::cerr << "Error: " << client.status().message() << std::endl;
    return 1;
  }

  return uchen::chat::Chat(client->get());
}