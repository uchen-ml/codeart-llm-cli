# Onboarding Guide for Senior Software Engineer

## **1. Project Overview**
### **CodeArt-LLM-CLI**
CodeArt-LLM-CLI is a command-line tool designed to facilitate AI-assisted workflows for software development. It provides structured discussions, code reviews, and automated documentation generation via Large Language Models (LLMs). The system is designed for **extensibility, security, and usability** while maintaining efficiency in resource utilization.

### **Core Features**
- **Multi-agent collaboration** within structured **projects** and **channels**.
- **LLM integration** with support for OpenAI, Claude, and future models.
- **Sandboxed tool execution** with permission control.
- **Hierarchical configuration** using TOML files.
- **Command-driven interface** (`/commands` for execution, `#channels` for grouping, `@agents` for AI interactions).
- **Pluggable storage** for chat history (initially plain text).
- **Dynamic model management** through a `ModelManager` abstraction.

## **2. Project Architecture**
### **2.1 Key Components**

#### **ModelManager** (Implemented ✅)
- Tracks available LLMs and handles dynamic selection.
- Supports multiple backend providers (OpenAI, Claude, future models).
- Allows registration/unregistration of model instances.
- Provides dynamic retrieval of models for interaction.

#### **ModelInterface** (Implemented ✅)
- Encapsulates a specific model backend.
- Uses RAII to unregister models upon destruction.
- Holds a `std::shared_ptr<ModelManager>` for dynamic lookups.

#### **Chat** (Implemented ✅)
- Manages active conversation with a given model.
- Tracks connected models and delegates messages.
- Uses a callback mechanism for frontend integration.

#### **History** (Implemented ✅)
- Stores and retrieves conversation history.
- Uses JSON serialization via `nlohmann::json`.
- Supports pluggable storage backends (currently file-based).

#### **CommandParser** (Implemented ✅)
- Parses user input for commands.
- Uses `absl::StrSplit` for efficient whitespace handling.
- Returns structured command objects.

#### **Interactive CLI Interface** (Pending ⏳)
- Requires implementation of an interactive text UI.
- Should support scrolling, input buffering, and multi-line prompts.
- Will integrate with `Chat` and `ModelManager`.

## **3. Implementation Status**
| Component       | Status       | Notes |
|---------------|-------------|-------|
| ModelManager  | ✅ Implemented | Handles dynamic model registration and lookup. |
| ModelInterface | ✅ Implemented | Uses RAII, holds `std::shared_ptr<ModelManager>`. |
| Chat | ✅ Implemented | Manages active conversations, delegates model responses. |
| History | ✅ Implemented | Supports JSON-based storage, pluggable backends. |
| CommandParser | ✅ Implemented | Uses `absl::StrSplit`, returns structured command objects. |
| Interactive CLI | ⏳ Pending | Requires implementation of user-friendly TUI. |

## **4. How to Proceed**
### **4.1 Immediate Next Steps**
1. **Implement Interactive CLI (Top Priority)**
   - Create a text-based UI for chat sessions.
   - Integrate with `Chat` and `ModelManager`.
   - Support scrolling, multi-line input, and message history display.
   
2. **Refine ModelManager API**
   - Optimize model selection logic.
   - Improve error handling for model retrieval failures.
   
3. **Enhance Logging & Debugging**
   - Add structured logs for model interactions.
   - Implement better error reporting for LLM API calls.
   
### **4.2 Longer-Term Tasks**
1. **Implement Sandboxed Tool Execution**
   - Allow agents to run external tools securely.
   - Implement a permission-based execution model.
   
2. **Expand LLM Backend Support**
   - Add integration for additional AI models (Claude, Gemini, etc.).
   - Implement per-model customization options.
   
3. **Optimize Performance & Storage**
   - Implement a caching mechanism for model responses.
   - Evaluate alternative storage formats (e.g., SQLite, Redis).

## **5. Development Environment Setup**
### **5.1 Prerequisites**
- **C++20** compiler (GCC 11+/Clang 13+ recommended).
- **Bazel** as the build system.
- **Abseil, nlohmann_json, and Curl** as dependencies.

### **5.2 Building the Project**
```sh
bazel build //src:codeart_llm_cli
```

### **5.3 Running Tests**
```sh
bazel test //tests:all
```

### **5.4 Running the CLI**
```sh
./bazel-bin/src/codeart_llm_cli --api_key=<your_openai_key>
```

## **6. Key Design Decisions**
### **6.1 Model Management**
- ModelManager dynamically tracks and registers models.
- ModelInterface ensures clean model interactions.

### **6.2 Security & Execution Control**
- Tools will be sandboxed and require explicit user permissions.
- Model interactions are logged for debugging and auditability.

### **6.3 Storage & Configuration**
- History storage is initially plain text but is designed for extensibility.
- Configuration is hierarchical (machine, user, project, channel levels).

## **7. Contact & Collaboration**
- **Primary Developer:** [Your Name]
- **Repository:** [GitHub/Repo Link]
- **Communication:** Use Slack/Discord/Project Management Tool

---

This document provides everything needed for a new engineer to get started. The top priority is implementing the interactive CLI while refining ModelManager and enhancing debugging. If anything needs clarification, reach out!

