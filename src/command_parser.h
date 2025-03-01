#ifndef CODEART_LLMCLI_COMMAND_PARSER_H_
#define CODEART_LLMCLI_COMMAND_PARSER_H_

#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"

namespace codeart::llmcli {

class CommandParser {
 public:
  struct ParsedCommand {
    std::string command;
    std::vector<std::string> args;

    bool operator==(const ParsedCommand& other) const = default;

    template <typename Sink>
    friend void AbslStringify(Sink& sink, const ParsedCommand& message) {
      absl::Format(&sink, "Command \"%s (%s)\"", message.command,
                   absl::StrJoin(message.args, ", "));
    }
  };

  static std::optional<ParsedCommand> Parse(const std::string& input);
};

std::ostream& operator<<(std::ostream& os,
                         const CommandParser::ParsedCommand& command);

}  // namespace codeart::llmcli

#endif  // CODEART_LLMCLI_COMMAND_PARSER_H_
