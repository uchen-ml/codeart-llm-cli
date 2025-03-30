#ifndef SRC_PROCESSFILE_INPUT_H_
#define SRC_PROCESSFILE_INPUT_H_

#include <iostream>
#include <optional>
#include <string>
#include <utility>

namespace uchen::chat {

struct Settings {
  std::string multiline_open = R"(^^^)";
  std::string multiline_close = R"(^^^)";
};

class InputReader {
  public:
    explicit InputReader(std::istream& input, Settings settings = {})
        : input_(input), settings_(std::move(settings)) {}

    std::optional<std::string> operator()() const;

  private:
    std::istream& input_;
    Settings settings_;
};

}  // namespace uchen::chat

#endif  // SRC_PROCESSFILE_INPUT_H_