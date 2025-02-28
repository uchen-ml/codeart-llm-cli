# CodeArt-LLM-CLI: High-Level Design

## 1. Overview

**CodeArt-LLM-CLI** is a command-line interface for interacting with large language models (LLMs), designed for developers and teams who need AI-assisted code review, documentation generation, and structured discussions in software projects. It supports interactive dialog and single-message command execution, with structured discussions through projects, channels, and agents. The tool is designed for extensibility, security, and usability, while maintaining efficient resource utilization.

## 2. Core Concepts

### 2.1. Projects
A **project** serves as the primary context for discussions. It can be mapped to a file system folder containing source files and related assets.

### 2.2. Channels
A **channel** represents an independent thread of discussion within a project. Channels may share context but maintain separate conversation histories.

### 2.3. Agents
An **agent** is an LLM instance configured for a specific task. Agents:
- Are assigned to channels.
- Can collaborate among themselves.
- Can recall state beyond a single session.
- Use tools to perform tasks such as reading/writing files, searching the web, etc.

### 2.4. Tools
Tools are external utilities that agents can invoke. To control complexity in the initial implementation, only a predefined set of tool categories will be supported. They:
- Are defined in the initial agent prompt.
- Operate within a sandboxed environment with permission prompts.
- Run as OS processes.
- Require user approval for accessing new files.
- Support a dry-run mode for previewing changes before execution.

## 3. Configuration
Configuration is structured in a hierarchical manner and stored in **TOML** files:

| Level     | Description |
|-----------|-------------|
| **Machine** | System-wide defaults applicable to all users and projects. |
| **User** | User-specific settings that override machine-level configurations. |
| **Project** | Project-specific settings that override user-level configurations. |
| **Channel** | Channel-specific overrides for agent behavior and tools. |

Example configuration file structure:
```
<project>/
 ├── agents/
 │   ├── angryreviewer.toml
 │   ├── security_audit.toml
 ├── config.toml
```

## 4. Command Structure & UX

### 4.1. Syntax
- `#channel @agent message` → Sends a message to an agent in a specific channel.
- Messages without an explicit channel are sent to the current active channel.
- Subsequent messages default to the last used channel unless another is specified.
- Agents can see messages directed to other agents but are not inclined to respond unless addressed.
- `/commands` → Special system commands that are **not** forwarded to the LLM.

### 4.2. Key Commands
| Command | Description |
|---------|-------------|
| `/help` | Displays available commands. |
| `/kick @agent` | Unlinks an agent from a channel. |
| `/list` | Lists active projects, channels, agents, or tools. |
| `/config set key=value` | Sets user/project configuration. |

### 4.3. Ephemeral Chats
- Ephemeral chats are automatically **deleted** after use.
- They do **not** persist in history.

## 5. Tool Execution & Security

- **Global file access whitelist** with user approval for new files.
- **Directory subtrees can be whitelisted** for easier access control.
- **Agents are restricted to specific tools** based on configuration.
- **Sequential and parallel tool execution** is supported.
- **Dry-run mode** allows previewing changes before execution.

## 6. Agent Behavior & Collaboration

- **Multiple agents can be invited to a channel.**
- **Agents do not share context across channels.**
- **Agents can proactively message users.**
- **Agents recall state beyond the session history.**
- **Agent behavior can be updated mid-session (e.g., enabling a new tool).**

## 7. History & Data Storage

- **Each channel maintains a separate history.**
- **History is stored in plain text files (pluggable storage planned).**
- **No history compaction** (users manually delete files if needed).
- **Logs/debugging info are stored separately from history.**
- **No encryption** is applied to stored history.

## 8. Performance & Scalability

- **Long-running tasks can run in the background.**
- **Batch processing can be triggered via Cron.**
- **Minimal caching will be implemented to improve performance while keeping complexity low. Optimized storage layers will be considered in future iterations.**

## 9. Future Considerations
- Expanding storage formats beyond plain text.
- Supporting agent-driven workflow automation.
- Introducing finer-grained tool permission management.
- Enhancing performance with caching and async execution models.

## 10. Conclusion
CodeArt-LLM-CLI is designed as a flexible, extensible, and secure CLI tool for structured LLM interactions. Unlike traditional CLI-based AI tools, it integrates multi-agent collaboration, sandboxed tool execution, and hierarchical configuration, making it particularly well-suited for AI-assisted software development workflows. Its modular architecture ensures adaptability to various LLM providers, providing developers with a powerful and customizable AI-driven assistant. With a focus on modularity and clear separation of concerns, it enables efficient AI-powered discussions in development workflows.

