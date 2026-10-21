# SprintIDE

A cross-platform IDE built from scratch with Qt 6 + C++20, designed to provide AI Agents with an executable, verifiable code runtime.

[English](#english) | [中文](#中文)

---

## English

### Project Overview

SprintIDE is a cross-platform IDE built from scratch with **Qt 6 + C++20**, not another VS Code fork. It provides five core subsystems:

- **Editor**: Syntax highlighting (17 languages), code completion, quick fix, outline overlay, breadcrumb navigation
- **Multi-language Build System**: C++ / Python / Go / Rust / JS/TS with GenericBuilder (AI universal adapter)
- **LSP Integration**: clangd, pylsp — real-time diagnostics, code completion, go-to-definition
- **DAP Debugger**: codelldb / lldb-dap / GDB DAP — breakpoints, step-through, variable inspection, call stack, inline variables
- **AI Agent Runtime**: TCP + JSON-RPC v1.1 — agents can create projects, build, run, edit files, execute commands (user-confirmed), and chat

SprintIDE's UI supports **Chinese / English** language switching. All user-visible text is internationalized.

### Why I Built This

SprintIDE started as a C++ compiler I wrote for practice, and gradually evolved into a full-featured IDE with a complete UI capable of compiling and running C++ projects.

Later, while using AI Agents to write code, the Agent kept hallucinating — in the end it admitted: it wasn't sure if the code was correct because it had no environment to run and test it in.

I remembered SprintIDE — it already had a complete UI and could run projects. I realized: rather than letting the Agent guess in a vacuum, why not give it an environment where it can execute, verify, and iterate? So I started transforming SprintIDE into an execution sandbox for AI Agents.

### AI Agent Runtime

SprintIDE provides an **Agent Runtime** interface that allows AI Agents to launch SprintIDE and perform the following operations via TCP + JSON-RPC protocol (v1.1):

- **Project Operations**: Create projects, create files, replace code
- **Build & Run**: Compile projects, run, stop running
- **File Operations**: Read/write/delete files
- **Terminal Interaction**: Execute commands (requires user confirmation)
- **Environment Management**: Get/set environment variables, query IDE metadata
- **Conversation**: Chat with the Agent

SprintIDE feeds back the result of every operation to the AI Agent in real time. An excellent AI Agent paired with SprintIDE can significantly reduce repetitive operations, moving code from "generated" to truly "executed, verified, iterated."

#### Agent Architecture

- **Transport Layer**: IDE acts as a TCP client that proactively connects to the Agent (4-byte length prefix frame protocol)
- **Unified Routing**: OperationBus dispatches to 7 Executors by domain (chat / file / command / ide / project / env / meta)
- **Protocol Validation**: Validator checks envelope + items, silently skips unknown types
- **Response Construction**: ResultBuilder serializes by method type (supports 9 IDE→Agent methods)
- **Policy Engine**: PolicyEngine decides which operations require user confirmation (user sovereignty, strictness only, never relaxed)
- **Connection Management**: Proactive connection + exponential backoff reconnection + handshake protocol

### Terminal Security Design

SprintIDE provides terminal capabilities to AI Agents, but they are locked by default: terminal commands must be approved by the user before execution.

This is also why I designed the icon as a giant acorn. If you've seen *Ice Age*, you know how much trouble a single acorn can cause. Unlocking is easy, but I don't recommend you do it.

### Core Features

#### Editor

- Multi-language syntax highlighting (17 languages)
- Code completion (local symbols + LSP merge, 200ms debounce)
- Go to definition (Ctrl+Click, LSP-driven)
- Quick Fix (Alt+Enter / gutter red dot / right-click menu, three equivalent entry points)
- Code outline overlay (Ctrl+` toggle, 4:6 split, keyboard navigation)
- Breadcrumb navigation (top bar of editor, shows symbol path)
- Bracket matching, auto-indent, auto-pairing (JetBrains style)
- Enter in brackets expands, string/comment protection
- Inline find/replace, code folding
- Copy line/delete line (supports selection)
- Paste indent alignment, consecutive enter inherits indent

#### Multi-language Build System

One-click compile, run, and clean for **C++ / Python / Go / Rust / JavaScript/TypeScript** with dynamic extension for unknown languages via **GenericBuilder (AI Universal Adapter)**.

- **C++**: CMake project model detection, CMake configure/build/clean, automatic compile_commands.json generation, Ninja/Make generator auto-detection, full Qt project support (automatic .clangd generation)
- **Python**: Virtual environment auto-detection, dependency management (pip/poetry), code formatting (Black), test runner (pytest), import path completion
- **Go / Rust / Node.js**: Corresponding language build tool integration

#### LSP Integration

- clangd (C++), pylsp (Python) and other language server integration
- Real-time diagnostics (errors/warnings), code completion, go-to-definition
- Three-tier diagnostic filtering (errors only / all / custom)
- Quick Fix auto-recommends #include fixes (--header-insertion=iwyu)
- Qt project .clangd auto-generation (solves Qt header file path issues)

#### Debugger Integration (DAP)

- **C++**: Based on codelldb / lldb-dap / GDB DAP, supports breakpoints, step-through, variable inspection, call stack, conditional breakpoints, lazy-loaded variables, expression evaluation, inline variable display
- **Python**: Based on debugpy, supports breakpoints, step-through, variable inspection, call stack
- Auto show/hide strategy for debug panel (PyCharm/CLion style)
- Lazy-loaded variables (structs/containers expand on double-click)

#### Other Features

- **Problems Panel**: Unified data source for LSP and compiler diagnostics, tree view + search + double-click jump
- **External Library Browser**: Language-aware dependency scanning (Python site-packages / Rust cargo / Go modules / Node node_modules / C++ compile_commands + Qt)
- **Project File Tree**: Multi-language project detection, Git version control integration
- **Local History**: Auto-save versions, dialog-based restore selection
- **Settings Dialog**: PyCharm-style, 8 settings pages (Editor/Build/Python/AI/Git/LSP/Shortcuts/Language)
- **39 Configurable Shortcuts** + conflict highlighting + three-state storage
- **Dark/Light Theme Switching** (40+ theme tokens, hardcoded colors strictly prohibited)
- **Cross-platform Performance Monitor** (CPU/MEM real-time indicator)
- **Multi-instance Support** (open independent IDE instances in new windows)
- **Structured Output Panel** (log filtering + grouping + counting + multi-select copy + double-click jump)

### Tech Stack

- **Language**: C++20
- **Framework**: Qt 6 (Core / Gui / Widgets / Concurrent)
- **Build System**: CMake + Ninja/Make
- **Protocols**: LSP / DAP / JSON-RPC (TCP)
- **Debug Adapters**: codelldb / lldb-dap / GDB DAP / debugpy
- **Cross-platform**: macOS / Linux / Windows

### Architecture

#### Core Principles

- **PIMPL Decoupling**: MainWindow handles only generic UI logic; language-specific logic moves to corresponding Builder/LSPManager/Debugger
- **Factory Pattern**: BuildFactory统一管理 registration and creation of all language Builders
- **Strategy Pattern**: ToolchainManager four-level resolution strategy (user explicit → embedded → user data → system)
- **Unified Message Routing**: OperationBus dispatches by domain + 7 Executors each handling their responsibility
- **Policy Engine**: PolicyEngine guarantees user sovereignty (strictness only, never relaxed)
- **Unified Diagnostic Source**: DiagnosticsStore unifies LSP and compiler diagnostics; editor squiggles/Problems tree/status bar count always consistent

#### Key Design Decisions

- **AI Module**: IDE acts as TCP client proactively connecting to Agent (rather than Agent connecting to IDE), avoiding port exposure and firewall issues
- **QProcess Lifecycle**: Strictly follows three rules (stack-local / member + pre-disconnect / finished slot never modifies members), fixing multiple UAF bugs
- **Theme System**: ThemeManager统一管理 40+ color tokens, all UI elements retrieve colors via tokens, hardcoded colors strictly prohibited
- **Editor Behavior Layering**: Language-specific behaviors don't pile if-else into TextEditor; split by language into EditorRules/LanguageHighlighter/Builder

### Getting Started

```bash
# Clone the repository
git clone https://github.com/your-username/SprintIDE.git
cd SprintIDE

# Create build directory and compile
mkdir build && cd build
cmake -G Ninja \
      -DCMAKE_MAKE_PROGRAM=$(which ninja) \
      -DCMAKE_PREFIX_PATH="/path/to/Qt6" \
      ..
ninja -j4
ninja deploy

# Run
./build/SprintIDE.app/Contents/MacOS/SprintIDE  # macOS
# or
./SprintIDE                                      # Linux
```

---


### System Requirements

| Platform    | Requirements                           |
| :---------- | :------------------------------------- |
| **macOS**   | 12+ (Intel / Apple Silicon)            |
| **Linux**   | Mainstream distributions (glibc 2.28+) |
| **Windows** | 10 / 11                                |

**Build toolchain:**

- CMake >= 3.16
- Qt 6.5+
- C++17 compiler (Clang / GCC / MSVC)
- Ninja (optional, recommended)

### Building from Source

SprintIDE **only provides source code**, so you need to compile it yourself.

#### macOS

```bash
# 1. Install dependencies
brew install cmake ninja qt@6

# 2. Clone
git clone https://github.com/your-username/SprintIDE.git
cd SprintIDE

# 3. Build
mkdir -p build && cd build
cmake .. -G Ninja -DCMAKE_PREFIX_PATH=$(brew --prefix qt@6)
ninja -j4

# 4. Run
open SprintIDE.app
```

**If you use the official Qt installer** (path is `~/Qt/6.x.x/macos`):

```bash
cmake .. -G Ninja -DCMAKE_PREFIX_PATH=~/Qt/6.10.1/macos
```

#### Linux (Debian / Ubuntu)

```bash
sudo apt install cmake ninja-build     qt6-base-dev qt6-tools-dev qt6-svg-dev     libqt6-network6 qt6-l10n-tools

git clone https://github.com/your-username/SprintIDE.git
cd SprintIDE

mkdir -p build && cd build
cmake .. -G Ninja
ninja -j4

./SprintIDE
```

#### Windows

Requires Visual Studio 2022 + the official Qt 6 installer.

```powershell
mkdir build
cd build
cmake .. -G Ninja -DCMAKE_PREFIX_PATH=C:/Qt/6.10.1/msvc2019_64
ninja
```

### First Run

After launching:

1. Menu -> **Settings** -> **Environment Variables** -> configure SDK paths for each language (optional)
2. Menu -> **File -> Open Project** -> open your project directory
3. Menu -> **Tools -> Terminal** -> open the terminal panel
4. AI functionality requires starting the Agent and then automatically connecting (see Part 3 of the documentation)

### Design Principles

| Principle                   | Meaning                                                      |
| :-------------------------- | :----------------------------------------------------------- |
| **User sovereignty**        | Agents can only "tighten" confirmation policies, not bypass user configuration |
| **Progressive enhancement** | The simplest Agent can run by replying with just one sentence; capabilities can be layered on gradually |
| **Asymmetric send/receive** | Messages sent from the IDE to the Agent are divided into 9 semantic methods; what the Agent returns to the IDE is uniformly an "envelope + items" |
| **Tool neutrality**         | The IDE is a tool and the Agent is the user; the IDE only executes and acknowledges, and does not orchestrate on behalf of the Agent |

### Keyboard-First Design

SprintIDE is designed to be **fully operable with keyboard only**. Every panel can be switched to, every common action has a keyboard shortcut, and the focus system ensures you never need to touch the mouse.

**PanelFocusManager** is the core component that manages keyboard focus across all panels. It ensures that:

- Only one panel has focus at any time
- Focus transitions are instantaneous (no mouse needed)
- All menu actions have keyboard equivalents
- Shortcuts are fully configurable (39 shortcuts, conflict detection, three-state storage)

This design philosophy extends to file navigation, code editing, build/run triggers, and debugging — everything can be done without reaching for the mouse.

### Keyboard Shortcuts

#### Panel Switching

| Shortcut | Panel |
| :------- | :---- |
| Ctrl+1   | Editor |
| Ctrl+2   | Project Tree |
| Ctrl+3   | Outline / Breadcrumb |
| Ctrl+4   | Problems |
| Ctrl+5   | Breakpoints |
| Ctrl+6   | Debug |
| Ctrl+7   | Terminal |
| Ctrl+8   | Output / Log |
| Ctrl+9   | Chat (AI) |

#### File Navigation

| Shortcut | Action |
| :------- | :----- |
| Ctrl+Tab | Switch between open files |
| Ctrl+1~9 | Jump to file by index |
| Ctrl+F   | Find in current file |
| Ctrl+Shift+F | Find in files |
| Ctrl+O   | Open file/project |

#### Editing

| Shortcut | Action |
| :------- | :----- |
| Ctrl+S   | Save |
| Ctrl+Z   | Undo |
| Ctrl+Y   | Redo |
| Ctrl+D   | Delete line |
| Ctrl+C   | Copy line (when no selection) |
| Ctrl+Shift+K | Delete line |
| Ctrl+Shift+D | Duplicate line |
| Alt+Enter | Quick Fix |
| Ctrl+`   | Toggle outline overlay |

#### Build & Debug

| Shortcut | Action |
| :------- | :----- |
| Ctrl+B   | Build project |
| Ctrl+R   | Run project |
| Ctrl+F5  | Stop running |
| F9       | Toggle breakpoint |
| F10      | Step over |
| F11      | Step into |
| Shift+F11 | Step out |

All shortcuts are configurable in **Settings -> Shortcuts**. Conflicts are highlighted in red.

---
## 中文

### 项目简介

SprintIDE 是一个由我独立开发的跨平台集成开发环境，面向开发者和 AI Agent 共同使用。它基于 **Qt 6 + C++20** 构建，不是另一个 VS Code 的壳，而是一个从零实现的完整 IDE。

目前它包含五大核心子系统：

- **编辑器**：支持代码编辑、语法高亮和基础编辑能力
- **多语言构建系统**：支持创建项目、编译和运行不同语言项目
- **LSP 客户端**：集成语言服务器协议，提供代码分析能力
- **调试器集成**：支持项目调试和运行状态控制
- **Agent 运行时**：为 AI Agent 提供可执行、可验证的 IDE 环境

### 为什么做这个项目？

SprintIDE 最初是我练手写的一个 C++ 编译器，后来逐渐做成了带完整 UI 的 IDE，已经能编译和运行 C++ 项目。

后来我在用 AI Agent 写代码时，Agent 反复出现幻觉，最后它自己承认：它不确定代码是否正确，因为它没有可以运行和测试的环境。

我想起 SprintIDE——它当时已经有了完整的 UI，能跑项目。于是我意识到：与其让 Agent 在真空中猜测，不如给它一个可以执行、验证、迭代的环境。于是我开始改造 SprintIDE，让它成为 AI Agent 的运行沙箱。

### AI Agent 运行时

SprintIDE 提供了一套 **Agent Runtime** 接口，AI Agent 可以主动启动 SprintIDE，并通过 TCP + JSON-RPC 协议（v1.1）执行以下操作：

- **项目操作**：创建项目、创建文件、替换代码
- **构建运行**：编译项目、运行、停止运行
- **文件操作**：读取/写入/删除文件
- **终端交互**：执行命令（需用户确认）
- **环境管理**：获取/设置环境变量、查询 IDE 元信息
- **对话交互**：与 Agent 进行聊天对话

SprintIDE 会将每一步操作结果实时反馈给 AI Agent。一个优秀的 AI Agent 配合 SprintIDE，可以大幅减少重复操作，让代码从"生成"真正进入"运行、验证、迭代"。

#### Agent 接入架构

- **传输层**：IDE 作为 TCP 客户端主动连接 Agent（4 字节长度前缀帧协议）
- **统一路由**：OperationBus 按 domain 分发到 7 个 Executor（chat / file / command / ide / project / env / meta）
- **协议校验**：Validator 校验信封 + item，未知 type 静默跳过
- **回执构造**：ResultBuilder 按方法类型序列化（支持 9 种 IDE→Agent 方法）
- **策略引擎**：PolicyEngine 决定哪些操作需要用户确认（用户主权，只加严不放宽）
- **连接管理**：主动连接 + 指数退避重连 + 握手协议

### 终端安全设计

SprintIDE 为 AI Agent 提供了终端能力，但默认加锁：终端命令必须经过用户同意后才能执行。

这也是我把图标设计成巨大橡果的原因。如果你看过《冰川时代》（*Ice Age*），就知道一颗橡果能闯多大的祸。解锁很容易，但我不建议你这么做。

### 核心功能

#### 编辑器

- 多语言语法高亮（17 种语言）
- 代码补全（本地符号 + LSP 合并，200ms 防抖）
- 跳转到定义（Ctrl+点击，LSP 驱动）
- Quick Fix（Alt+Enter / 行号红点 / 右键菜单，三条等价入口）
- 代码大纲浮层（Ctrl+` toggle，4:6 分栏，键盘导航）
- 面包屑导航（编辑器顶部横条，显示符号路径）
- 括号匹配、自动缩进、自动配对（JetBrains 风格）
- 括号中回车展开、字符串/注释保护
- 内联查找/替换、代码折叠
- 复制行/删除行（支持选区）
- 粘贴缩进对齐、连续回车继承缩进

#### 多语言构建系统

支持 **C++ / Python / Go / Rust / JavaScript/TypeScript** 五大语言的一键编译、运行、清理，并通过 **GenericBuilder（AI 通用适配器）** 实现未知语言的动态扩展。

- **C++**：CMake 项目模型识别、CMake configure/build/clean、compile_commands.json 自动生成、Ninja/Make 生成器自动探测、Qt 项目完整支持（.clangd 自动生成）
- **Python**：虚拟环境自动检测、依赖管理（pip/poetry）、代码格式化（Black）、测试运行器（pytest）、import 路径补全
- **Go / Rust / Node.js**：对应的语言构建工具集成

#### LSP 集成

- clangd（C++）、pylsp（Python）等语言服务器集成
- 实时诊断（错误/警告）、代码补全、跳转到定义
- 三档诊断过滤模式（只错误 / 全部 / 自定义）
- Quick Fix 自动推荐 #include 修复（--header-insertion=iwyu）
- Qt 项目 .clangd 自动生成（解决 Qt 头文件路径问题）

#### 调试器集成（DAP）

- **C++**：基于 codelldb / lldb-dap / GDB DAP，支持断点、单步、变量查看、调用栈、条件断点、变量懒加载、表达式求值、内联变量显示
- **Python**：基于 debugpy，支持断点、单步、变量查看、调用栈
- 调试面板自动显示/隐藏策略（PyCharm/CLion 风格）
- 变量懒加载（结构体/容器双击展开）

#### 其他功能

- **Problems 诊断面板**：LSP 诊断和编译诊断统一数据源，树形展示 + 搜索 + 双击跳转
- **外部库浏览器**：按语言扫描依赖（Python site-packages / Rust cargo / Go modules / Node node_modules / C++ compile_commands + Qt）
- **项目文件树**：多语言项目识别、Git 版本控制集成
- **本地历史**：自动保存版本、对话框选择恢复
- **设置对话框**：仿 PyCharm 风格，8 个设置页（编辑器/编译/Python/AI/Git/LSP/快捷键/语言）
- **39 项可配置快捷键** + 冲突标红 + 三态存储
- **深色/浅色主题切换**（40+ 主题令牌，严禁硬编码颜色）
- **跨平台性能监控**（CPU/MEM 实时指示器）
- **多实例支持**（新窗口打开独立 IDE 实例）
- **结构化输出面板**（日志过滤 + 分组 + 计数 + 多选复制 + 双击跳转）

### 技术栈

- **语言**：C++20
- **框架**：Qt 6（Core / Gui / Widgets / Concurrent）
- **构建系统**：CMake + Ninja/Make
- **协议**：LSP / DAP / JSON-RPC（TCP）
- **调试适配器**：codelldb / lldb-dap / GDB DAP / debugpy
- **跨平台**：macOS / Linux / Windows

### 架构设计

#### 核心架构原则

- **PIMPL 解耦**：MainWindow 仅负责通用 UI 逻辑，语言特定逻辑迁移至对应的 Builder/LSPManager/调试器
- **工厂模式**：BuilderFactory 统一管理各语言 Builder 的注册与创建
- **策略模式**：ToolchainManager 四级解析策略（用户显式 → 内嵌 → 用户数据 → 系统）
- **统一消息路由**：OperationBus 按域分发 + 7 个 Executor 各司其职
- **策略引擎**：PolicyEngine 保证用户主权（只加严不放宽）
- **统一诊断数据源**：DiagnosticsStore 统一 LSP 和编译诊断，编辑器波浪线/Problems 树/状态栏计数三处始终一致

#### 关键设计决策

- **AI 模块**：IDE 作为 TCP 客户端主动连接 Agent（而非 Agent 连接 IDE），避免端口暴露和防火墙问题
- **QProcess 生命周期**：严格遵循三条规则（栈上局部 / 成员+前置disconnect / finished槽不修改成员），修复了多处 UAF bug
- **主题系统**：ThemeManager 统一管理 40+ 颜色令牌，所有 UI 元素通过令牌取色，严禁硬编码
- **编辑器行为分层**：语言相关行为不往 TextEditor 堆 if-else，按语言拆分 EditorRules/LanguageHighlighter/Builder

### 快速开始

```bash
# 克隆仓库
git clone https://github.com/你的用户名/SprintIDE.git
cd SprintIDE

# 创建构建目录并编译
mkdir build && cd build
cmake -G Ninja \
      -DCMAKE_MAKE_PROGRAM=$(which ninja) \
      -DCMAKE_PREFIX_PATH="/path/to/Qt6" \
      ..
ninja -j4
ninja deploy

# 运行
./build/SprintIDE.app/Contents/MacOS/SprintIDE  # macOS
# 或
./SprintIDE                                      # Linux
```

---


### 系统要求

| 平台        | 要求                         |
| :---------- | :--------------------------- |
| **macOS**   | 12+（Intel / Apple Silicon） |
| **Linux**   | 主流发行版（glibc 2.28+）    |
| **Windows** | 10 / 11                      |

**构建工具链**：

- CMake >= 3.16
- Qt 6.5+
- C++17 编译器（Clang / GCC / MSVC）
- Ninja（可选，推荐）

### 从源码构建

SprintIDE **只提供源码**，需要你自己编译。

#### macOS

```bash
# 1. 安装依赖
brew install cmake ninja qt@6

# 2. 克隆
git clone https://github.com/你的用户名/SprintIDE.git
cd SprintIDE

# 3. 构建
mkdir -p build && cd build
cmake .. -G Ninja -DCMAKE_PREFIX_PATH=$(brew --prefix qt@6)
ninja -j4

# 4. 运行
open SprintIDE.app
```

**如果你用 Qt 官方安装器**（路径为 `~/Qt/6.x.x/macos`）：

```bash
cmake .. -G Ninja -DCMAKE_PREFIX_PATH=~/Qt/6.10.1/macos
```

#### Linux（Debian / Ubuntu）

```bash
sudo apt install cmake ninja-build     qt6-base-dev qt6-tools-dev qt6-svg-dev     libqt6-network6 qt6-l10n-tools

git clone https://github.com/你的用户名/SprintIDE.git
cd SprintIDE

mkdir -p build && cd build
cmake .. -G Ninja
ninja -j4

./SprintIDE
```

#### Windows

需要 Visual Studio 2022 + Qt 6 官方安装器。

```powershell
mkdir build
cd build
cmake .. -G Ninja -DCMAKE_PREFIX_PATH=C:/Qt/6.10.1/msvc2019_64
ninja
```

### 首次运行

启动后：

1. 菜单 -> **设置** -> **环境变量** -> 配置各语言 SDK 路径（可选）
2. 菜单 -> **文件 -> 打开项目** -> 打开你的项目目录
3. 菜单 -> **工具 -> 终端** -> 打开终端面板
4. AI 功能需要启动 Agent 后自动连接（见文档第三部分）

### 设计原则

| 原则           | 含义                                                         |
| :------------- | :----------------------------------------------------------- |
| **用户主权**   | Agent 只能"加严"确认策略，不能绕过用户配置                   |
| **渐进增强**   | 最简 Agent 只回一句话就能跑，能力可以逐级叠加                |
| **收发不对称** | IDE 发给 Agent 的消息分 9 个语义方法；Agent 回给 IDE 的统一是"信封 + items" |
| **工具中立**   | IDE 是工具，Agent 是使用者；IDE 只执行和回执，不替 Agent 编排 |

### 键盘优先设计

SprintIDE 的设计目标是：**整个 IDE 可以完全不使用鼠标操作**。所有面板都可以通过快捷键切换，所有常用操作都有键盘快捷方式，焦点管理系统确保你不需要碰鼠标。

**PanelFocusManager** 是统一管理焦点的核心组件，它确保：

- 任意时刻只有一个面板获得焦点
- 焦点切换是瞬时的（不需要鼠标）
- 所有菜单操作都有键盘等价命令
- 快捷键完全可配置（39 项可配置快捷键、冲突检测、三态存储）

这个设计哲学延伸到文件导航、代码编辑、构建/运行触发、调试等所有操作 — 你可以完全不碰鼠标完成所有工作。

### 快捷键参考

#### 面板切换

| 快捷键     | 面板         |
| :--------- | :----------- |
| Ctrl+1     | 编辑器       |
| Ctrl+2     | 项目树       |
| Ctrl+3     | 大纲/面包屑  |
| Ctrl+4     | Problems     |
| Ctrl+5     | 断点         |
| Ctrl+6     | 调试         |
| Ctrl+7     | 终端         |
| Ctrl+8     | 输出/日志    |
| Ctrl+9     | 聊天（AI）   |

#### 文件导航

| 快捷键     | 操作             |
| :--------- | :--------------- |
| Ctrl+Tab   | 在打开的文件间切换 |
| Ctrl+1~9   | 按序号跳转到文件  |
| Ctrl+F     | 当前文件查找     |
| Ctrl+Shift+F | 多文件查找      |
| Ctrl+O     | 打开文件/项目    |

#### 编辑

| 快捷键           | 操作           |
| :--------------- | :------------- |
| Ctrl+S           | 保存           |
| Ctrl+Z           | 撤销           |
| Ctrl+Y           | 重做           |
| Ctrl+D           | 删除行         |
| Ctrl+C           | 复制行（无选区时）|
| Ctrl+Shift+K     | 删除行         |
| Ctrl+Shift+D     | 复制行         |
| Alt+Enter        | Quick Fix      |
| Ctrl+\`          | 切换大纲浮层   |

#### 构建与调试

| 快捷键     | 操作         |
| :--------- | :----------- |
| Ctrl+B     | 构建项目     |
| Ctrl+R     | 运行项目     |
| Ctrl+F5    | 停止运行     |
| F9         | 切换断点     |
| F10        | 单步跳过     |
| F11        | 单步进入     |
| Shift+F11  | 单步跳出     |

所有快捷键可在 **设置 -> 快捷键** 中自定义。冲突快捷键会以红色标出。

---
## Project Structure / 项目结构

```
SprintIDE/
├── CMakeLists.txt                          # 构建配置 / Build config
├── main.cpp                                # 入口 / Entry point
│
├── application.h/cpp                       # 应用程序单例 / Application singleton
├── config_manager.h/cpp                    # 配置管理 / Config management
├── theme_manager.h/cpp                     # 主题管理（40+ 令牌）/ Theme management (40+ tokens)
├── font_manager.h/cpp                      # 字体管理 / Font management
│
├── mainwindow.h/cpp/ui                     # 主窗口 / Main window
├── panel_focus_manager.h/cpp               # 面板焦点管理 / Panel focus management
├── tab_widget.h/cpp                        # 多标签页管理 / Tab widget management
├── editor_widget.h/cpp                     # 编辑器容器 / Editor container
│
├── text_editor.h/cpp                       # 文本编辑器（面包屑/大纲/搜索）/ Text editor (breadcrumb/outline/search)
├── breakpoint_list_panel.h/cpp             # 断点列表面板 / Breakpoint list panel
├── project_tree_proxy_model.h/cpp          # 项目树代理模型 / Project tree proxy model
├── syntax_highlighter.h/cpp                # 语法高亮（备用）/ Syntax highlighting (fallback)
│
├── project_config.h/cpp                    # 项目配置 / Project config
├── project_manager.h/cpp                   # 项目管理 / Project manager
├── project_panel.h/cpp                     # 项目文件树 / Project file tree
├── project_creation_dialog.h/cpp/ui        # 创建项目向导 / Project creation wizard
├── project_selection_dialog.h/cpp          # 选择/打开项目 / Project selection
│
├── compile_manager.h/cpp                   # 编译管理 / Compile manager
├── compile_output_parser.h/cpp             # 编译输出解析 / Compile output parser
├── process_manager.h/cpp                   # 进程管理 / Process manager
├── path_utils.h/cpp                        # 路径工具 / Path utilities
│
├── debug_manager.h/cpp                     # 调试管理 / Debug manager
├── debug_panel.h/cpp                       # 调试面板 / Debug panel
│
├── builder/                                # 多语言构建系统 / Multi-language build system
│   ├── build_task.h / build_result.h       # 构建任务/结果模型 / Build task/result models
│   ├── ibuilder.h                          # 构建器抽象接口 / Builder abstract interface
│   ├── cpp_builder.h/cpp                   # C++ 构建器 / C++ builder
│   ├── python_builder.h/cpp                # Python 构建器 / Python builder
│   ├── go_builder.h/cpp                    # Go 构建器 / Go builder
│   ├── rust_builder.h/cpp                  # Rust 构建器 / Rust builder
│   ├── node_builder.h/cpp                  # Node.js 构建器 / Node.js builder
│   ├── generic_builder.h/cpp               # AI 通用适配器 / AI universal adapter
│   └── builder_factory.h/cpp               # 构建器工厂 / Builder factory
│
├── lsp/                                    # LSP 客户端 / LSP client
│   ├── lsp_client.h/cpp                    # LSP 客户端 / LSP client
│   ├── lsp_manager.h/cpp                   # LSP 管理器 / LSP manager
│   └── diagnostic_filter.h/cpp             # 诊断过滤器 / Diagnostic filter
│
├── dap/                                    # DAP 协议客户端 / DAP protocol client
│   ├── dap_types.h                         # DAP 类型定义 / DAP type definitions
│   └── dap_client.h/cpp                    # DAP 客户端 / DAP client
│
├── language/                               # 语言识别与高亮 / Language detection & highlighting
│   ├── language_detector.h/cpp             # 语言检测 / Language detector
│   ├── language_highlighter.h/cpp          # 语言高亮 / Language highlighter
│   ├── language_type.h                     # 语言类型枚举 / Language type enum
│   └── editor_rules.h/cpp                  # 编辑器规则 / Editor rules
│
├── outline/                                # 代码大纲系统 (v8.5 新增) / Code outline system (v8.5 new)
│   ├── outline_types.h                     # 大纲符号数据结构 / Outline symbol data structures
│   ├── outline_provider.h                  # IOutlineProvider 接口 / IOutlineProvider interface
│   ├── outline_provider_factory.h/cpp      # 工厂 / Factory
│   ├── python_outline_provider.h/cpp       # Python 提供者 / Python provider
│   ├── cpp_outline_provider.h/cpp          # C++/Qt 提供者 / C++/Qt provider
│   ├── outline_controller.h/cpp            # 编辑器↔Provider 桥 / Editor↔Provider bridge
│   ├── breadcrumb_bar.h/cpp                # 面包屑导航 / Breadcrumb bar
│   ├── outline_item_delegate.h/cpp         # 大纲浮层项委托 / Outline overlay item delegate
│   └── outline_overlay.h/cpp               # 大纲浮层 / Outline overlay
│
├── problems/                               # Problems 面板 (v8.5 新增) / Problems panel (v8.5 new)
│   ├── diagnostics_store.h/cpp             # 诊断数据仓库（统一 LSP+编译）/ Diagnostic store (unified LSP+compile)
│   └── problems_panel.h/cpp                # Problems 面板 / Problems panel
│
├── external_libs/                          # 外部库浏览器 (v8.5 新增) / External library browser (v8.5 new)
│   ├── external_library_types.h            # 外部库节点数据结构 / External library node data structures
│   ├── external_library_provider.h         # IExternalLibraryProvider 接口 / Interface
│   ├── external_library_provider_factory.h/cpp  # 工厂 / Factory
│   └── providers/
│       ├── python_external_library_provider.h/cpp
│       ├── rust_external_library_provider.h/cpp
│       ├── go_external_library_provider.h/cpp
│       ├── node_external_library_provider.h/cpp
│       └── cpp_external_library_provider.h/cpp
│
├── ai/                                     # AI Agent 运行时 / AI Agent runtime
│   ├── ai_connection_manager.h/cpp         # 连接管理 / Connection manager
│   ├── ai_integration.h/cpp                # AI 集成 / AI integration
│   ├── chat_panel.h/cpp                    # 聊天面板 / Chat panel
│   ├── operation_bus.h/cpp                 # 统一消息路由 / Unified message bus
│   ├── transport/                          # 传输层 / Transport layer
│   │   ├── ai_transport.h
│   │   └── tcp_transport.h/cpp
│   ├── protocol/                           # 协议层 / Protocol layer
│   │   ├── protocol_types.h
│   │   ├── validator.h/cpp                 # 协议校验 / Protocol validation
│   │   ├── execution_result.h
│   │   └── result_builder.h/cpp            # 回执构造 / Response builder
│   ├── executor/                           # 域处理器（7个）/ Domain executors (7)
│   │   ├── i_executor.h
│   │   ├── chat_executor.h/cpp             # 对话 / Chat
│   │   ├── file_executor.h/cpp             # 文件操作 / File operations
│   │   ├── command_executor.h/cpp          # 命令执行 / Command execution
│   │   ├── ide_executor.h/cpp              # IDE 控制 / IDE control
│   │   ├── project_executor.h/cpp          # 项目管理 / Project management
│   │   ├── env_executor.h/cpp              # 环境管理 / Environment management
│   │   └── meta_executor.h/cpp             # 元信息 / Metadata
│   └── policy/
│       └── policy_engine.h/cpp             # 策略引擎（用户主权）/ Policy engine (user sovereignty)
│
├── terminal/                               # 终端面板 / Terminal panel
│   └── terminal_panel.h/cpp
│
├── history/                                # 本地历史 / Local history
├── git/                                    # Git 集成 / Git integration
│   └── git_command.h/cpp
├── core/                                   # 核心工具 / Core utilities
│   ├── log_buffer.h/cpp                    # 日志缓冲 / Log buffer
│   └── project_scaffolder.h/cpp            # 项目脚手架 / Project scaffolder
│
├── settings_dialog.h/cpp                   # 设置对话框 / Settings dialog
├── editor_settings_page.h/cpp              # 编辑器设置页 / Editor settings page
├── python/python_settings_page.h/cpp       # Python 设置页 / Python settings page
├── ai/ai_settings_page.h/cpp               # AI 设置页 / AI settings page
├── git/git_settings_page.h/cpp             # Git 设置页 / Git settings page
├── search_in_files_dialog.h/cpp            # 多文件查找 / Search in files
│
├── resources/                              # 资源文件 / Resources
│   ├── lightide_resources.qrc
│   ├── fonts.qrc
│   ├── icons/                              # SVG 图标 / SVG icons
│   ├── themes/                             # 主题文件 / Theme files
│   └── fonts/                              # 字体文件 / Font files
│
├── performance/                            # 性能监控 / Performance monitoring
│   └── performance_monitor.h/cpp
│
└── deps/                                   # 工具链管理器 / Toolchain manager
    ├── toolchain_types.h
    ├── toolchain_provider.h
    ├── toolchain_manager.h/cpp             # 四级解析策略 / Four-level resolution strategy
    ├── lldb_dap_provider.h/cpp
    ├── gdb_dap_provider.h/cpp
    └── toolchain_dialog.h/cpp
```

## Changelog / 版本历史

### English

- **v8.5** (Current): Problems panel + code outline overlay & breadcrumbs + external library browser + Qt project support + editing refinement + dead code cleanup
- **v8.4**: Quick Fix system + multi-instance support + QProcess lifecycle hardening + editor interaction aligned with JetBrains
- **v8.3**: Performance monitoring + breakpoint management panel + output panel structured refactor + LSP didChange debounce
- **v8.2**: Non-modal code completion + multi-language behavior rules layer + LSP diagnostic filter + JetBrains-style bracket skip
- **v8.1**: ToolchainManager four-level resolution + lazy variable loading + expression evaluation + conditional breakpoints
- **v8.0**: CMake project model + DAP debugger integration + universal builder architecture

### 中文

- **v8.5**（当前）：Problems 诊断面板 + 代码大纲浮层与面包屑 + 外部库浏览器 + Qt 项目支持 + 编辑交互精细化 + 死代码清理
- **v8.4**：Quick Fix 系统 + 多实例支持 + QProcess 生命周期规范固化 + 编辑器交互对齐 JetBrains
- **v8.3**：性能监控 + 断点管理面板 + 输出面板结构化重构 + LSP didChange 防抖
- **v8.2**：非模态代码补全 + 多语言行为规则层 + LSP 诊断过滤器 + JetBrains 风格闭合符跳过
- **v8.1**：ToolchainManager 四级解析 + 变量懒加载 + 表达式求值 + 条件断点
- **v8.0**：CMake 项目模型 + DAP 调试器集成 + 通用构建器架构

## License / 许可证

## License

SprintIDE is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

SprintIDE is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
SprintIDE. If not, see <https://www.gnu.org/licenses/>.

Copyright (C) 2026 Liu Tengjun (刘腾骏)
