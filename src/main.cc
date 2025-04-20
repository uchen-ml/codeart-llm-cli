#include <array>
#include <cstddef>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/log/check.h"
#include "absl/log/initialize.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_split.h"
#include "absl/strings/substitute.h"

#include "curl/curl.h"
#include "src/anthropic.h"
#include "src/chat.h"
#include "src/event_loop.h"
#include "src/input.h"
#include "src/model.h"
#include "src/openai.h"
#include "src/tui.h"

namespace uchen::chat {
namespace {

constexpr absl::Duration kTimeout = absl::Seconds(5);

class MessageLog {
 public:
  Message WaitForMessage() {
    absl::MutexLock lock(&mutex_);
    mutex_.AwaitWithTimeout({+[](const std::vector<Message>* messages) {
                               return !messages->empty();
                             },
                             &messages_},
                            kTimeout);
    Message message = std::move(messages_.back());
    messages_.pop_back();
    return message;
  }

  void AddMessage(Message message) {
    if (message.provider() == this) {
      return;
    }
    absl::MutexLock lock(&mutex_);
    messages_.push_back(std::move(message));
  }

 private:
  absl::Mutex mutex_;
  std::vector<Message> messages_ ABSL_GUARDED_BY(&mutex_);
};

int ChatLoop(Model* model) {
  std::cout << absl::Substitute("Model: $0\nType your message below:",
                                model->name());
  auto event_loop = EventLoop::Create();
  auto chat = Chat::Create(event_loop);
  auto model_unsubscribe = model->Connect(chat);
  InputReader reader(std::cin);
  CurlFetch fetch;
  MessageLog message_log;
  auto subscription = chat->Subscribe(
      [&](Message message) { message_log.AddMessage(std::move(message)); });

  while (true) {
    std::cout << "\n> ";
    auto prompt = reader();
    if (prompt == std::nullopt) {
      return 0;
    }
    if (!prompt->empty()) {
      chat->SendMessage(Message::Origin::kUser, *prompt, std::nullopt,
                        &message_log);
      auto response = SpinWhile([&]() { return message_log.WaitForMessage(); });
      // if (!response.ok()) {
      //   std::cerr << "Error: " << response.status().message() << std::endl;
      //   return 1;
      // }
      std::cout << response.content() << std::endl;
    }
  }
}

}  // namespace
}  // namespace uchen::chat

ABSL_FLAG(std::string, model, "gpt-4o-mini-search-preview",
          "A well known model or provider:model tuple.");
ABSL_FLAG(size_t, max_tokens, 1024, "Maximum number of tokens to generate.");

ABSL_FLAG(bool, list, false, "List available models.");

int main(int argc, char* argv[], char* envp[]) {
  curl_global_init(CURL_GLOBAL_ALL);
  std::vector<std::string> segments =
      absl::StrSplit(argv[0], absl::ByAnyChar("/\\"));
  absl::SetProgramUsageMessage(
      absl::Substitute("Terminal chat client for LLM APIs.\n"
                       "Usage: $0 --model=<model> [--api_key=<key>]",
                       segments.back()));
  std::vector<char*> positional_args = absl::ParseCommandLine(argc, argv);
  absl::InitializeLog();
  auto fetch = std::make_shared<uchen::chat::CurlFetch>();
  uchen::chat::Parameters parameters(absl::GetFlag(FLAGS_max_tokens), envp);
  std::string model = absl::GetFlag(FLAGS_model);
  std::array<std::unique_ptr<uchen::chat::ModelProvider>, 2> providers = {
      uchen::chat::MakeOpenAIModelProvider(fetch, parameters),
      uchen::chat::MakeAnthropicModelProvider(fetch, parameters),
  };
  if (absl::GetFlag(FLAGS_list)) {
    for (const auto& provider : providers) {
      auto models = provider->ListModels();
      if (!models.empty()) {
        std::cout << "Available models for " << provider->name() << ":\n";
        for (const auto& model : models) {
          std::cout << "  " << model << "\n";
        }
      }
    }
  } else {
    absl::StatusOr<uchen::chat::ModelHandle> model;
    for (const auto& provider : providers) {
      model = provider->ConnectToModel(absl::GetFlag(FLAGS_model));
      if (model.ok()) {
        break;
      }
      if (model.status().code() != absl::StatusCode::kNotFound) {
        std::cerr << "Error: " << model.status().message() << std::endl;
        return 1;
      }
    }
    if (!model.ok()) {
      std::cerr << "Error: No model found for " << absl::GetFlag(FLAGS_model)
                << std::endl;
      return 1;
    }
    CHECK_NE(model->get(), nullptr);
    return uchen::chat::ChatLoop(model->get());
  }
}