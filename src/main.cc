#include <array>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "absl/cleanup/cleanup.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/functional/any_invocable.h"
#include "absl/log/initialize.h"
#include "absl/status/statusor.h"

#include "curl/curl.h"
#include "src/claude_client.h"
#include "src/client.h"
#include "src/input.h"

namespace {

constexpr std::string_view kApiKey = "api_key";

using Parameters = std::unordered_map<std::string_view, std::string>;

using ClientFactory =
    absl::AnyInvocable<absl::StatusOr<std::unique_ptr<processfile::Client>>(
        const Parameters&) const>;

class ClaudeFactory {
 public:
  ClaudeFactory(std::string_view model, std::string_view name)
      : model_(model), name_(name) {}

  absl::StatusOr<std::unique_ptr<processfile::Client>> operator()(
      const Parameters& parameters) const {
    auto it = parameters.find(kApiKey);
    if (it == parameters.end()) {
      return absl::InvalidArgumentError("Missing 'api_key' parameter");
    }
    return std::make_unique<processfile::ClaudeClient>(model_, name_,
                                                       it->second, 1024);
  }

 private:
  std::string_view model_;
  std::string_view name_;
};

std::unordered_map<std::string_view, ClientFactory> PrepareClients() {
  std::unordered_map<std::string_view, ClientFactory> clients;
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
    clients.emplace(id, ClaudeFactory(model, name));
  }
  return clients;
}

template <typename T>
std::invoke_result_t<T> SpinWhile(const T& func) {
  absl::Mutex mu;
  absl::CondVar cv;
  std::optional<std::invoke_result_t<T>> result;
  std::thread spinner_thread([&]() {
    auto r = func();
    absl::MutexLock lock(&mu);
    result = std::move(r);
    cv.Signal();
  });
  absl::Cleanup cleanup = [&]() { spinner_thread.join(); };
  constexpr std::string_view kSpinChars = "|/-\\";
  absl::MutexLock lock(&mu);
  int spin = 0;
  std::cout << " ";
  while (!result.has_value()) {
    std::cout << "\r" << kSpinChars[spin++ % kSpinChars.size()];
    std::cout.flush();
    cv.WaitWithTimeout(&mu, absl::Milliseconds(200));
  }
  std::cout << "\r";
  return std::move(result).value();
}

}  // namespace

ABSL_FLAG(std::string, model, "claude",
          "LLM model to use: 'claude' or 'openai'.");
ABSL_FLAG(std::optional<std::string>, api_key, std::nullopt, "API key.");

int main(int argc, char* argv[]) {
  curl_global_init(CURL_GLOBAL_ALL);
  absl::SetProgramUsageMessage(
      "Filter program that sends input through an LLM with a prompt.\n"
      "Usage: processfile --prompt=<text> [--model=<model>] [input_file]");
  std::vector<char*> positional_args = absl::ParseCommandLine(argc, argv);
  absl::InitializeLog();

  std::string model = absl::GetFlag(FLAGS_model);

  auto factories = PrepareClients();
  auto it = factories.find(model);
  if (it == factories.end()) {
    std::cerr << "Error: Unknown model: " << model << std::endl;
    return 1;
  }

  Parameters parameters;
  if (absl::GetFlag(FLAGS_api_key).has_value()) {
    parameters[kApiKey] = absl::GetFlag(FLAGS_api_key).value();
  }

  auto client = it->second(parameters);
  if (!client.ok()) {
    std::cerr << "Error: " << client.status().message() << std::endl;
    return 1;
  }

  std::cout << "Uchen Chat CLI. Type your message below:";
  uchen::chat::InputReader reader(std::cin);
  while (true) {
    std::cout << "\n> ";
    auto prompt = reader();
    if (prompt == std::nullopt) {
      return 0;
    }
    if (!prompt->empty()) {
      processfile::CurlFetch fetch;
      auto response = SpinWhile(
          [&]() { return client.value()->Query(fetch, *prompt, {}); });
      if (!response.ok()) {
        std::cerr << "Error: " << response.status().message() << std::endl;
        return 1;
      }
      std::cout << "Response: " << *response << std::endl;
    }
  }

  return 0;
}