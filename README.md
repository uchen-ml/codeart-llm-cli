# codeart-llm-cli

## Overview
`codeart-llm-cli` is a command-line tool for interacting with LLMs.

## Features
- Interactive chat
- Built with C++20 for modern performance optimizations
- Uses Bazel for efficient builds and dependency management
- Includes GoogleTest for unit testing
- Modular design for future expansions

## Build Instructions
### Prerequisites
Ensure you have the following installed:
- **Bazel** (latest recommended version)
- **C++20-compatible compiler** (e.g., GCC, Clang, MSVC)

### Clone the Repository
```sh
git clone https://github.com/your-repo/codeart-llm-cli.git
cd codeart-llm-cli
```

### Build the Project
```sh
bazel build //src:codeart_llm_cli
```

### Run the CLI
```sh
bazel run //src:codeart_llm_cli
```

## Testing
To run unit tests:
```sh
bazel test //tests/...
```

## Contributing
Contributions are welcome! Please follow the coding standards and ensure tests pass before submitting a pull request.

## License
MIT License. See `LICENSE` for details.

