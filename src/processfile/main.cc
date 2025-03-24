#include <fstream>
#include <iostream>
#include <istream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/status/statusor.h"
#include "absl/strings/substitute.h"

ABSL_FLAG(std::string, prompt, "", "Prompt text for the LLM.");
ABSL_FLAG(std::string, model, "claude",
          "LLM model to use: 'claude' or 'openai'.");

namespace {

absl::StatusOr<std::string> ReadInput(
    const std::optional<std::string_view>& path) {
  // Get input path from positional args if provided
  std::ostringstream ss;
  if (path.has_value()) {  // First arg is program name
    std::ifstream file{*path};
    if (!file) {
      std::cerr << "Error: Could not open input file: " << *path << std::endl;
      return absl::UnavailableError(absl::Substitute("Can not read $0", *path));
    }
    ss << file.rdbuf();
  } else {
    ss << std::cin.rdbuf();
  }
  return ss.str();
}

}  // namespace

int main(int argc, char* argv[]) {
  absl::SetProgramUsageMessage(
      "Filter program that sends input through an LLM with a prompt.\n"
      "Usage: processfile --prompt=<text> [--model=<model>] [input_file]");

  std::vector<char*> positional_args = absl::ParseCommandLine(argc, argv);

  std::string prompt = absl::GetFlag(FLAGS_prompt);
  std::string model = absl::GetFlag(FLAGS_model);

  if (prompt.empty()) {
    std::cerr << "Error: --prompt is required." << std::endl;
    return 1;
  }

  if (model != "claude" && model != "openai") {
    std::cerr << "Error: Unknown model. Use 'claude' or 'openai'." << std::endl;
    return 1;
  }

  // Get input path from positional args if provided
  std::optional<std::string_view> name =
      positional_args.size() > 1
          ? std::optional<std::string_view>{positional_args[1]}
          : std::nullopt;
  auto input_content = ReadInput(name);

  if (!input_content.ok()) {
    std::cerr << "Error: " << input_content.status().message() << std::endl;
    return 1;
  }

  // TODO(Claude): Implement LLM client calls
  std::cout << "Input size: " << input_content->size() << " bytes" << std::endl;
  std::cout << "Prompt: " << prompt << std::endl;
  std::cout << "Model: " << model << std::endl;

  return 0;
}