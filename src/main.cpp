#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/log.h"
#include <cstdlib>
#include <string>

// Define flags
ABSL_FLAG(std::string, api_key, "", "OpenAI API key (required)");
ABSL_FLAG(std::string, model, "gpt-4", "OpenAI model to use");

int main(int argc, char *argv[]) {
  absl::ParseCommandLine(argc, argv);

  std::string api_key = absl::GetFlag(FLAGS_api_key);
  std::string model = absl::GetFlag(FLAGS_model);

  if (api_key.empty()) {
    LOG(ERROR) << "API key is required. Use --api-key=<your_api_key>";
    return 1;
  }

  LOG(INFO) << "Starting codeart-llm-cli with OpenAI model: " << model;

  // Placeholder for actual API request logic
  LOG(INFO) << "Connecting to OpenAI API...";

  return 0;
}
