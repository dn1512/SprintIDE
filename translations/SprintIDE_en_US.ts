<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="en_US">
<context>
    <name>CompilerSettingsPage</name>
    <message>
        <source>编译器设置</source>
        <translation>Compiler Settings</translation>
    </message>
    <message>
        <source>自动检测</source>
        <translation>Auto Detect</translation>
    </message>
    <message>
        <source>浏览...</source>
        <translation>Browse...</translation>
    </message>
    <message>
        <source>编译器类型:</source>
        <translation>Compiler Type:</translation>
    </message>
    <message>
        <source>编译器路径:</source>
        <translation>Compiler Path:</translation>
    </message>
    <message>
        <source>C++ 标准</source>
        <translation>C++ Standard</translation>
    </message>
    <message>
        <source>优化选项</source>
        <translation>Optimization</translation>
    </message>
    <message>
        <source>包含调试信息 (-g)</source>
        <translation>Include debug info (-g)</translation>
    </message>
    <message>
        <source>启用所有警告 (-Wall -Wextra)</source>
        <translation>Enable all warnings (-Wall -Wextra)</translation>
    </message>
    <message>
        <source>将警告视为错误 (-Werror)</source>
        <translation>Treat warnings as errors (-Werror)</translation>
    </message>
    <message>
        <source>选择编译器</source>
        <translation>Select Compiler</translation>
    </message>
</context>
<context>
    <name>EnvironmentSettingsPage</name>
    <message>
        <source>这些环境变量将全局生效，用于配置各语言的 SDK 路径。它们与具体项目无关，代表 SprintIDE 当前具备开发哪些语言的能力。</source>
        <translation>These environment variables are applied globally to configure SDK paths for each language. They are independent of any specific project and represent which languages SprintIDE can currently develop.</translation>
    </message>
    <message>
        <source>快速填充 SDK:</source>
        <translation>Quick fill SDK:</translation>
    </message>
    <message>
        <source>选择已检测的 SDK...</source>
        <translation>Select a detected SDK...</translation>
    </message>
    <message>
        <source>自动检测已安装 SDK</source>
        <translation>Auto-detect installed SDKs</translation>
    </message>
    <message>
        <source>变量名</source>
        <translation>Variable</translation>
    </message>
    <message>
        <source>值</source>
        <translation>Value</translation>
    </message>
    <message>
        <source>添加</source>
        <translation>Add</translation>
    </message>
    <message>
        <source>删除选中</source>
        <translation>Remove Selected</translation>
    </message>
    <message>
        <source>清空全部</source>
        <translation>Clear All</translation>
    </message>
    <message>
        <source>确认清空</source>
        <translation>Confirm Clear</translation>
    </message>
    <message>
        <source>确定要删除所有环境变量吗？</source>
        <translation>Delete all environment variables?</translation>
    </message>
    <message>
        <source>确认删除</source>
        <translation>Confirm Delete</translation>
    </message>
    <message>
        <source>确定要删除环境变量 '%1' 吗？</source>
        <translation>Delete environment variable '%1'?</translation>
    </message>
    <message>
        <source>提示</source>
        <translation>Notice</translation>
    </message>
    <message>
        <source>未检测到 %1 的安装路径，请手动输入。</source>
        <translation>Could not detect the install path for %1. Please enter it manually.</translation>
    </message>
    <message>
        <source>检测完成</source>
        <translation>Detection Complete</translation>
    </message>
    <message>
        <source>共检测到 %1 个 SDK 环境变量。</source>
        <translation>Detected %1 SDK environment variables.</translation>
    </message>
</context>
<context>
    <name>LanguageSettingsPage</name>
    <message>
        <source>选择 SprintIDE 的界面语言。切换后需要重启应用才能生效。</source>
        <translation>Choose the UI language for SprintIDE. A restart is required for the change to take effect.</translation>
    </message>
    <message>
        <source>界面语言</source>
        <translation>UI Language</translation>
    </message>
    <message>
        <source>语言:</source>
        <translation>Language:</translation>
    </message>
    <message>
        <source>⚠ 语言已更改，重启 SprintIDE 后生效。</source>
        <translation>⚠ Language changed. Restart SprintIDE to apply.</translation>
    </message>
</context>
<context>
    <name>LspSettingsPage</name>
    <message>
        <source>控制 LSP 服务器返回的诊断信息如何显示。
错误（error）始终显示，不受此设置影响。</source>
        <translation>Controls how diagnostics returned by the LSP server are displayed.
Errors are always shown and are not affected by this setting.</translation>
    </message>
    <message>
        <source>警告显示模式</source>
        <translation>Warning Display Mode</translation>
    </message>
    <message>
        <source>只显示错误（推荐）</source>
        <translation>Show errors only (recommended)</translation>
    </message>
    <message>
        <source>显示所有警告和错误</source>
        <translation>Show all warnings and errors</translation>
    </message>
    <message>
        <source>自定义忽略规则</source>
        <translation>Custom ignore rules</translation>
    </message>
    <message>
        <source>• 只显示错误：隐藏所有 warning，编辑器最干净
• 显示所有警告和错误：使用 LSP 服务器的全部诊断
• 自定义忽略规则：按下方黑名单过滤 warning</source>
        <translation>• Errors only: hide all warnings, cleanest editor
• Show all warnings and errors: use all diagnostics from the LSP server
• Custom ignore rules: filter warnings by the blacklist below</translation>
    </message>
    <message>
        <source>忽略规则（每行一条，支持正则）:</source>
        <translation>Ignore rules (one per line, regex supported):</translation>
    </message>
    <message>
        <source>imported but unused
line too long
^missing-.*docstring</source>
        <translation>imported but unused
line too long
^missing-.*docstring</translation>
    </message>
    <message>
        <source>匹配 warning 消息中的子串，不区分大小写。支持 QRegularExpression 语法。</source>
        <translation>Matches a substring of the warning message, case-insensitive. QRegularExpression syntax is supported.</translation>
    </message>
</context>
<context>
    <name>MainWindow</name>
    <message>
        <location filename="../mainwindow.ui" line="14"/>
        <source>MainWindow</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>ProjectCreationDialog</name>
    <message>
        <location filename="../project_creation_dialog.ui" line="14"/>
        <location filename="../build/SprintIDE_autogen/include/ui_project_creation_dialog.h" line="378"/>
        <source>创建新项目</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../project_creation_dialog.ui" line="24"/>
        <location filename="../build/SprintIDE_autogen/include/ui_project_creation_dialog.h" line="390"/>
        <source>基本信息</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../project_creation_dialog.ui" line="30"/>
        <location filename="../build/SprintIDE_autogen/include/ui_project_creation_dialog.h" line="379"/>
        <source>项目信息</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../project_creation_dialog.ui" line="36"/>
        <location filename="../build/SprintIDE_autogen/include/ui_project_creation_dialog.h" line="380"/>
        <source>项目名称:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../project_creation_dialog.ui" line="50"/>
        <location filename="../build/SprintIDE_autogen/include/ui_project_creation_dialog.h" line="381"/>
        <source>显示名称:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../project_creation_dialog.ui" line="60"/>
        <location filename="../build/SprintIDE_autogen/include/ui_project_creation_dialog.h" line="382"/>
        <source>项目位置:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../project_creation_dialog.ui" line="72"/>
        <location filename="../build/SprintIDE_autogen/include/ui_project_creation_dialog.h" line="383"/>
        <source>浏览...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../project_creation_dialog.ui" line="81"/>
        <location filename="../build/SprintIDE_autogen/include/ui_project_creation_dialog.h" line="384"/>
        <source>路径预览</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../project_creation_dialog.ui" line="91"/>
        <location filename="../build/SprintIDE_autogen/include/ui_project_creation_dialog.h" line="385"/>
        <source>项目模板:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../project_creation_dialog.ui" line="104"/>
        <location filename="../build/SprintIDE_autogen/include/ui_project_creation_dialog.h" line="386"/>
        <source>模板信息</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../project_creation_dialog.ui" line="110"/>
        <location filename="../build/SprintIDE_autogen/include/ui_project_creation_dialog.h" line="387"/>
        <source>模板描述</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../project_creation_dialog.ui" line="120"/>
        <location filename="../build/SprintIDE_autogen/include/ui_project_creation_dialog.h" line="388"/>
        <source>文件列表</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../project_creation_dialog.ui" line="132"/>
        <location filename="../build/SprintIDE_autogen/include/ui_project_creation_dialog.h" line="389"/>
        <source>查看示例代码</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../project_creation_dialog.ui" line="158"/>
        <location filename="../build/SprintIDE_autogen/include/ui_project_creation_dialog.h" line="402"/>
        <source>编译设置</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../project_creation_dialog.ui" line="164"/>
        <location filename="../build/SprintIDE_autogen/include/ui_project_creation_dialog.h" line="391"/>
        <source>编译模式</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../project_creation_dialog.ui" line="170"/>
        <location filename="../build/SprintIDE_autogen/include/ui_project_creation_dialog.h" line="392"/>
        <source>简单模式 (推荐初学者使用，自动配置最佳编译选项)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../project_creation_dialog.ui" line="180"/>
        <location filename="../build/SprintIDE_autogen/include/ui_project_creation_dialog.h" line="393"/>
        <source>高级模式 (允许自定义所有编译选项，适合有经验的学习者)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../project_creation_dialog.ui" line="190"/>
        <location filename="../build/SprintIDE_autogen/include/ui_project_creation_dialog.h" line="394"/>
        <source>高级编译配置</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../project_creation_dialog.ui" line="199"/>
        <location filename="../build/SprintIDE_autogen/include/ui_project_creation_dialog.h" line="395"/>
        <source>编译器:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../project_creation_dialog.ui" line="209"/>
        <location filename="../build/SprintIDE_autogen/include/ui_project_creation_dialog.h" line="396"/>
        <source>C++标准:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../project_creation_dialog.ui" line="219"/>
        <location filename="../build/SprintIDE_autogen/include/ui_project_creation_dialog.h" line="397"/>
        <source>优化级别:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../project_creation_dialog.ui" line="229"/>
        <location filename="../build/SprintIDE_autogen/include/ui_project_creation_dialog.h" line="398"/>
        <source>警告级别:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../project_creation_dialog.ui" line="239"/>
        <location filename="../build/SprintIDE_autogen/include/ui_project_creation_dialog.h" line="399"/>
        <source>调试信息:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../project_creation_dialog.ui" line="246"/>
        <location filename="../build/SprintIDE_autogen/include/ui_project_creation_dialog.h" line="400"/>
        <source>包含调试信息 (便于调试程序)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../project_creation_dialog.ui" line="259"/>
        <location filename="../build/SprintIDE_autogen/include/ui_project_creation_dialog.h" line="401"/>
        <source>更多高级配置...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../project_creation_dialog.ui" line="280"/>
        <location filename="../build/SprintIDE_autogen/include/ui_project_creation_dialog.h" line="407"/>
        <source>高级选项</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../project_creation_dialog.ui" line="286"/>
        <location filename="../build/SprintIDE_autogen/include/ui_project_creation_dialog.h" line="403"/>
        <source>项目选项</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../project_creation_dialog.ui" line="292"/>
        <location filename="../build/SprintIDE_autogen/include/ui_project_creation_dialog.h" line="404"/>
        <source>创建后自动打开项目</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../project_creation_dialog.ui" line="302"/>
        <location filename="../build/SprintIDE_autogen/include/ui_project_creation_dialog.h" line="405"/>
        <source>创建Git仓库 (需要安装Git)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../project_creation_dialog.ui" line="309"/>
        <location filename="../build/SprintIDE_autogen/include/ui_project_creation_dialog.h" line="406"/>
        <source>添加.gitignore文件 (忽略编译输出等文件)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../project_creation_dialog.ui" line="354"/>
        <location filename="../build/SprintIDE_autogen/include/ui_project_creation_dialog.h" line="409"/>
        <source>重置为默认设置</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>SettingsDialog</name>
    <message>
        <source>设置</source>
        <translation>Settings</translation>
    </message>
    <message>
        <source>编辑器</source>
        <translation>Editor</translation>
    </message>
    <message>
        <source>编译配置</source>
        <translation>Compiler</translation>
    </message>
    <message>
        <source>环境变量</source>
        <translation>Environment</translation>
    </message>
    <message>
        <source>Python</source>
        <translation>Python</translation>
    </message>
    <message>
        <source>AI</source>
        <translation>AI</translation>
    </message>
    <message>
        <source>Git</source>
        <translation>Git</translation>
    </message>
    <message>
        <source>LSP 诊断</source>
        <translation>LSP Diagnostics</translation>
    </message>
    <message>
        <source>语言 / Language</source>
        <translation>Language / 语言</translation>
    </message>
    <message>
        <source>语言已更改</source>
        <translation>Language Changed</translation>
    </message>
    <message>
        <source>界面语言已更改，需要重启 SprintIDE 才能生效。

是否立即重启？</source>
        <translation>The UI language has changed. SprintIDE must be restarted to take effect.

Restart now?</translation>
    </message>
</context>
<!-- ============================================================ -->
<!-- ai_settings_page.cpp                                         -->
<!-- ============================================================ -->
<context>
    <name>SprintIDE::AISettingsPage</name>
    <message>
        <source>配置 SprintIDE 与本地 AI Agent 之间的通信地址。
默认 127.0.0.1:9876，请确保 Agent 端监听同一地址。</source>
        <translation>Configure the communication address between SprintIDE and the local AI Agent.
Default is 127.0.0.1:9876; make sure the Agent listens on the same address.</translation>
    </message>
    <message>
        <source>AI 服务器</source>
        <translation>AI Server</translation>
    </message>
    <message>
        <source>Agent 绑定的地址，通常为本机 127.0.0.1</source>
        <translation>Address the Agent binds to, usually 127.0.0.1</translation>
    </message>
    <message>
        <source>监听地址:</source>
        <translation>Listen Address:</translation>
    </message>
    <message>
        <source>Agent 监听的端口号（1 - 65535）</source>
        <translation>Port the Agent listens on (1 - 65535)</translation>
    </message>
    <message>
        <source>监听端口:</source>
        <translation>Listen Port:</translation>
    </message>
    <message>
        <source>启动 SprintIDE 时自动启动 AI 服务器</source>
        <translation>Automatically start the AI server when SprintIDE starts</translation>
    </message>
    <message>
        <source>测试连接</source>
        <translation>Test Connection</translation>
    </message>
    <message>
        <source>状态:</source>
        <translation>Status:</translation>
    </message>
    <message>
        <source>AI 行为</source>
        <translation>AI Behavior</translation>
    </message>
    <message>
        <source>编译 / 运行错误时自动请求 AI 修复</source>
        <translation>Automatically ask AI to fix compile / run errors</translation>
    </message>
    <message>
        <source>对应 AI/AutoFixErrors 配置，供后续 agent.autoFix 流程使用</source>
        <translation>Corresponds to the AI/AutoFixErrors setting, used by the agent.autoFix flow</translation>
    </message>
    <message>
        <source>发送消息时自动附带当前文件 / 项目上下文</source>
        <translation>Automatically include current file / project context when sending messages</translation>
    </message>
    <message>
        <source>AI 设置</source>
        <translation>AI Settings</translation>
    </message>
    <message>
        <source>监听地址不能为空，已使用默认值 127.0.0.1。</source>
        <translation>Listen address cannot be empty; the default 127.0.0.1 has been used.</translation>
    </message>
    <message>
        <source>⚠ 地址已更改，点击【重启 AI 服务器】后生效。</source>
        <translation>⚠ Address changed. Click [Restart AI Server] to apply.</translation>
    </message>
    <message>
        <source>✓ 设置已保存</source>
        <translation>✓ Settings saved</translation>
    </message>
    <message>
        <source>请先填写监听地址。</source>
        <translation>Please fill in the listen address first.</translation>
    </message>
    <message>
        <source>正在连接 %1:%2 ...</source>
        <translation>Connecting to %1:%2 ...</translation>
    </message>
    <message>
        <source>✓ 已连接到 %1:%2</source>
        <translation>✓ Connected to %1:%2</translation>
    </message>
    <message>
        <source>✗ 无法连接：%1</source>
        <translation>✗ Cannot connect: %1</translation>
    </message>
    <message>
        <source>重启 AI 服务器</source>
        <translation>Restart AI Server</translation>
    </message>
    <message>
        <source>⏳ 已发送重启请求：%1:%2</source>
        <translation>⏳ Restart request sent: %1:%2</translation>
    </message>
</context>
<!-- ============================================================ -->
<!-- chat_panel.cpp                                               -->
<!-- ============================================================ -->
<context>
    <name>SprintIDE::ChatPanel</name>
    <message>
        <source>AI 助手</source>
        <translation>AI Assistant</translation>
    </message>
    <message>
        <source>AI 助手已连接</source>
        <translation>AI Assistant connected</translation>
    </message>
    <message>
        <source>AI 助手已断开连接</source>
        <translation>AI Assistant disconnected</translation>
    </message>
    <message>
        <source>AI 错误 (%1): %2</source>
        <translation>AI error (%1): %2</translation>
    </message>
    <message>
        <source>清空</source>
        <translation>Clear</translation>
    </message>
    <message>
        <source>清空所有对话消息</source>
        <translation>Clear all chat messages</translation>
    </message>
    <message>
        <source>复制</source>
        <translation>Copy</translation>
    </message>
    <message>
        <source>复制整个对话内容</source>
        <translation>Copy the entire conversation</translation>
    </message>
    <message>
        <source>对话已复制到剪贴板</source>
        <translation>Conversation copied to clipboard</translation>
    </message>
    <message>
        <source>输入消息，按 Enter 发送，Ctrl+Enter 换行...</source>
        <translation>Type a message, press Enter to send, Ctrl+Enter for newline...</translation>
    </message>
    <message>
        <source>发送</source>
        <translation>Send</translation>
    </message>
    <message>
        <source>发送消息 (Enter)</source>
        <translation>Send message (Enter)</translation>
    </message>
    <message>
        <source>● 未连接</source>
        <translation>● Disconnected</translation>
    </message>
    <message>
        <source>🟢 已连接</source>
        <translation>🟢 Connected</translation>
    </message>
    <message>
        <source>🔴 未连接</source>
        <translation>🔴 Disconnected</translation>
    </message>
    <message>
        <source>AI 助手 (已连接)</source>
        <translation>AI Assistant (Connected)</translation>
    </message>
    <message>
        <source>AI 助手 (未连接)</source>
        <translation>AI Assistant (Disconnected)</translation>
    </message>
    <message>
        <source>对话已清空</source>
        <translation>Conversation cleared</translation>
    </message>
    <message>
        <source>用户: </source>
        <translation>User: </translation>
    </message>
    <message>
        <source>AI: </source>
        <translation>AI: </translation>
    </message>
    <message>
        <source>系统</source>
        <translation>System</translation>
    </message>
    <message>
        <source>--- 完整内容 ---</source>
        <translation>--- Full content ---</translation>
    </message>
</context>
<!-- ============================================================ -->
<!-- breakpoint_list_panel.cpp                                    -->
<!-- ============================================================ -->
<context>
    <name>SprintIDE::BreakpointListPanel</name>
    <message>
        <source>共 %1 个断点，%2 个已启用</source>
        <translation>%1 breakpoints, %2 enabled</translation>
    </message>
    <message>
        <source>禁用断点</source>
        <translation>Disable Breakpoint</translation>
    </message>
    <message>
        <source>启用断点</source>
        <translation>Enable Breakpoint</translation>
    </message>
    <message>
        <source>编辑条件...</source>
        <translation>Edit Condition...</translation>
    </message>
    <message>
        <source>清除条件</source>
        <translation>Clear Condition</translation>
    </message>
    <message>
        <source>删除断点</source>
        <translation>Remove Breakpoint</translation>
    </message>
    <message>
        <source>行: %2%3</source>
        <translation>Line: %2%3</translation>
    </message>
    <message>
        <source>条件: %1</source>
        <translation>Condition: %1</translation>
    </message>
</context>
<!-- ============================================================ -->
<!-- debug_panel.cpp                                              -->
<!-- ============================================================ -->
<context>
    <name>SprintIDE::DebugPanel</name>
    <message>
        <source>调试控制</source>
        <translation>Debug Control</translation>
    </message>
    <message>
        <source>继续</source>
        <translation>Continue</translation>
    </message>
    <message>
        <source>步过</source>
        <translation>Step Over</translation>
    </message>
    <message>
        <source>步入</source>
        <translation>Step Into</translation>
    </message>
    <message>
        <source>步出</source>
        <translation>Step Out</translation>
    </message>
    <message>
        <source>停止</source>
        <translation>Stop</translation>
    </message>
    <message>
        <source>清空</source>
        <translation>Clear</translation>
    </message>
    <message>
        <source>调用栈</source>
        <translation>Call Stack</translation>
    </message>
    <message>
        <source>函数</source>
        <translation>Function</translation>
    </message>
    <message>
        <source>文件</source>
        <translation>File</translation>
    </message>
    <message>
        <source>行号</source>
        <translation>Line</translation>
    </message>
    <message>
        <source>局部变量</source>
        <translation>Local Variables</translation>
    </message>
    <message>
        <source>名称</source>
        <translation>Name</translation>
    </message>
    <message>
        <source>值</source>
        <translation>Value</translation>
    </message>
    <message>
        <source>类型</source>
        <translation>Type</translation>
    </message>
    <message>
        <source>表达式求值</source>
        <translation>Evaluate Expression</translation>
    </message>
    <message>
        <source>输入表达式，回车求值…</source>
        <translation>Enter expression, press Enter to evaluate...</translation>
    </message>
    <message>
        <source>调试控制台</source>
        <translation>Debug Console</translation>
    </message>
    <message>
        <source>Loading...</source>
        <translation>Loading...</translation>
    </message>
</context>

<!-- ============================================================ -->
<!-- toolchain_dialog.cpp                                         -->
<!-- ============================================================ -->
<context>
    <name>SprintIDE::ToolchainDialog</name>
    <message>
        <source>缺少依赖组件</source>
        <translation>Missing Dependency</translation>
    </message>
    <message>
        <source>在终端中执行：</source>
        <translation>Run in terminal:</translation>
    </message>
    <message>
        <source>复制命令</source>
        <translation>Copy Command</translation>
    </message>
    <message>
        <source>查看详情</source>
        <translation>View Details</translation>
    </message>
    <message>
        <source>稍后</source>
        <translation>Later</translation>
    </message>
    <message>
        <source>我已安装，重新检测</source>
        <translation>I've installed it, re-check</translation>
    </message>
    <message>
        <source>已复制 ✓</source>
        <translation>Copied ✓</translation>
    </message>
</context>
<!-- ============================================================ -->
<!-- editor_settings_page.cpp                                     -->
<!-- ============================================================ -->
<context>
    <name>SprintIDE::EditorSettingsPage</name>
    <message>
        <source>字体</source>
        <translation>Font</translation>
    </message>
    <message>
        <source>字体族:</source>
        <translation>Font Family:</translation>
    </message>
    <message>
        <source> pt</source>
        <translation> pt</translation>
    </message>
    <message>
        <source>字号:</source>
        <translation>Font Size:</translation>
    </message>
    <message>
        <source>行距由 SprintIDE 自动按字号计算（1.5 倍字号），无需手动设置。</source>
        <translation>Line spacing is automatically calculated by SprintIDE (1.5× font size); no manual setting needed.</translation>
    </message>
    <message>
        <source>缩进</source>
        <translation>Indentation</translation>
    </message>
    <message>
        <source> 字符</source>
        <translation> characters</translation>
    </message>
    <message>
        <source>Tab 宽度:</source>
        <translation>Tab Width:</translation>
    </message>
    <message>
        <source>使用空格代替 Tab</source>
        <translation>Use spaces instead of tabs</translation>
    </message>
    <message>
        <source>显示</source>
        <translation>Display</translation>
    </message>
    <message>
        <source>显示行号</source>
        <translation>Show line numbers</translation>
    </message>
    <message>
        <source>高亮当前行</source>
        <translation>Highlight current line</translation>
    </message>
    <message>
        <source>预览</source>
        <translation>Preview</translation>
    </message>
</context>
<!-- ============================================================ -->
<!-- find_dialog.cpp                                              -->
<!-- ============================================================ -->
<context>
    <name>SprintIDE::FindDialog</name>
    <message>
        <source>查找</source>
        <translation>Find</translation>
    </message>
    <message>
        <source>输入查找内容...</source>
        <translation>Enter search text...</translation>
    </message>
    <message>
        <source>大小写敏感</source>
        <translation>Case sensitive</translation>
    </message>
    <message>
        <source>全字匹配</source>
        <translation>Whole word</translation>
    </message>
    <message>
        <source>查找下一个</source>
        <translation>Find Next</translation>
    </message>
    <message>
        <source>查找上一个</source>
        <translation>Find Previous</translation>
    </message>
    <message>
        <source>关闭</source>
        <translation>Close</translation>
    </message>
</context>
<!-- ============================================================ -->
<!-- git_diff_dialog.cpp                                          -->
<!-- ============================================================ -->
<context>
    <name>SprintIDE::Git::GitDiffDialog</name>
    <message>
        <source>Git Diff</source>
        <translation>Git Diff</translation>
    </message>
    <message>
        <source>打开文件</source>
        <translation>Open File</translation>
    </message>
    <message>
        <source>复制</source>
        <translation>Copy</translation>
    </message>
    <message>
        <source>复制全部 diff</source>
        <translation>Copy entire diff</translation>
    </message>
    <message>
        <source>✕</source>
        <translation>✕</translation>
    </message>
    <message>
        <source>关闭</source>
        <translation>Close</translation>
    </message>
    <message>
        <source>Diff - %1</source>
        <translation>Diff - %1</translation>
    </message>
    <message>
        <source>Commit - %1</source>
        <translation>Commit - %1</translation>
    </message>
    <message>
        <source>已复制</source>
        <translation>Copied</translation>
    </message>
    <message>
        <source>无差异</source>
        <translation>No differences</translation>
    </message>
    <message>
        <source>此文件没有任何差异。</source>
        <translation>This file has no differences.</translation>
    </message>
    <message>
        <source>+%1  −%2</source>
        <translation>+%1  −%2</translation>
    </message>
    <message>
        <source>  （共 %1 行，仅显示前 %2 行）</source>
        <translation>  (total %1 lines, showing first %2 lines only)</translation>
    </message>
    <message>
        <source>... 剩余 %1 行已省略（diff 过大）</source>
        <translation>... %1 more lines omitted (diff too large)</translation>
    </message>
</context>
<!-- ============================================================ -->
<!-- git_file_history_dialog.cpp                                  -->
<!-- ============================================================ -->
<context>
    <name>SprintIDE::Git::GitFileHistoryDialog</name>
    <message>
        <source>历史 - %1</source>
        <translation>History - %1</translation>
    </message>
    <message>
        <source>文件: &lt;b&gt;%1&lt;/b&gt;</source>
        <translation>File: &lt;b&gt;%1&lt;/b&gt;</translation>
    </message>
    <message>
        <source>关闭</source>
        <translation>Close</translation>
    </message>
    <message>
        <source>共 %1 条提交</source>
        <translation>%1 commits</translation>
    </message>
    <message>
        <source>(该文件没有任何提交记录)</source>
        <translation>(This file has no commit history)</translation>
    </message>
    <message>
        <source>提示</source>
        <translation>Notice</translation>
    </message>
    <message>
        <source>无法获取该 commit 的 diff。</source>
        <translation>Cannot retrieve diff for this commit.</translation>
    </message>
</context>
<context>
    <name>SprintIDE::GitSettingsPage</name>
    <message>
        <source>Git 可执行文件</source>
        <translation>Git Executable</translation>
    </message>
    <message>
        <source>留空则自动检测</source>
        <translation>Leave empty to auto-detect</translation>
    </message>
    <message>
        <source>浏览...</source>
        <translation>Browse...</translation>
    </message>
    <message>
        <source>路径:</source>
        <translation>Path:</translation>
    </message>
    <message>
        <source>自动检测</source>
        <translation>Auto Detect</translation>
    </message>
    <message>
        <source>未检测</source>
        <translation>Not detected</translation>
    </message>
    <message>
        <source>版本:</source>
        <translation>Version:</translation>
    </message>
    <message>
        <source>提交身份（git config user.*）</source>
        <translation>Commit Identity (git config user.*)</translation>
    </message>
    <message>
        <source>例如：张三</source>
        <translation>e.g. John Doe</translation>
    </message>
    <message>
        <source>用户名:</source>
        <translation>User name:</translation>
    </message>
    <message>
        <source>例如：zhangsan@example.com</source>
        <translation>e.g. john@example.com</translation>
    </message>
    <message>
        <source>邮箱:</source>
        <translation>Email:</translation>
    </message>
    <message>
        <source>写入当前仓库配置</source>
        <translation>Write to current repository config</translation>
    </message>
    <message>
        <source>自动刷新</source>
        <translation>Auto Refresh</translation>
    </message>
    <message>
        <source> 秒</source>
        <translation> sec</translation>
    </message>
    <message>
        <source>禁用</source>
        <translation>Disabled</translation>
    </message>
    <message>
        <source>状态刷新间隔:</source>
        <translation>Status refresh interval:</translation>
    </message>
    <message>
        <source>未检测到 git</source>
        <translation>Git not detected</translation>
    </message>
    <message>
        <source>选择 Git 可执行文件</source>
        <translation>Select Git Executable</translation>
    </message>
    <message>
        <source>检测失败</source>
        <translation>Detection Failed</translation>
    </message>
    <message>
        <source>未在系统 PATH 或常见安装路径中找到 git。</source>
        <translation>Git was not found in PATH or common installation locations.</translation>
    </message>
    <message>
        <source>提示</source>
        <translation>Notice</translation>
    </message>
    <message>
        <source>当前没有打开的 Git 仓库，无法写入仓库级配置。</source>
        <translation>No Git repository is currently open; cannot write repository-level config.</translation>
    </message>
    <message>
        <source>成功</source>
        <translation>Success</translation>
    </message>
    <message>
        <source>已写入当前仓库的 git config。</source>
        <translation>Written to the current repository's git config.</translation>
    </message>
    <message>
        <source>失败</source>
        <translation>Failed</translation>
    </message>
    <message>
        <source>写入 git config 失败，请检查仓库权限。</source>
        <translation>Failed to write git config. Check repository permissions.</translation>
    </message>
</context>
<context>
    <name>SprintIDE::LogListView</name>
    <message>
        <location filename="../log/log_list_view.cpp" line="108"/>
        <source>复制选中(&amp;C)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../log/log_list_view.cpp" line="113"/>
        <source>复制全部(&amp;A)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../log/log_list_view.cpp" line="117"/>
        <source>全选(&amp;S)</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>SprintIDE::MainWindow</name>
    <message>
        <source>AI 服务器</source>
        <translation>AI Server</translation>
    </message>
    <message>
        <source>重启失败：无法在 %1:%2 上监听。
请检查端口是否被占用，或地址是否合法。</source>
        <translation>Failed to restart: cannot listen on %1:%2.
Check whether the port is occupied or the address is valid.</translation>
    </message>
    <message>
        <source>性能监控</source>
        <translation>Performance Monitor</translation>
    </message>
    <message>
        <source>查看断点</source>
        <translation>Show Breakpoints</translation>
    </message>
    <message>
        <source>禁用所有断点</source>
        <translation>Disable All Breakpoints</translation>
    </message>
    <message>
        <source>恢复所有断点</source>
        <translation>Enable All Breakpoints</translation>
    </message>
    <message>
        <source>断点</source>
        <translation>Breakpoint</translation>
    </message>
    <message>
        <source>编辑断点条件</source>
        <translation>Edit Breakpoint Condition</translation>
    </message>
    <message>
        <source>第 %1 行的断点条件（留空 = 无条件）：</source>
        <translation>Breakpoint condition at line %1 (empty = none):</translation>
    </message>
    <message>
        <source>文件(&amp;F)</source>
        <translation>File(&amp;F)</translation>
    </message>
    <message>
        <source>新建(&amp;N)</source>
        <translation>New(&amp;N)</translation>
    </message>
    <message>
        <source>文件...</source>
        <translation>File...</translation>
    </message>
    <message>
        <source>C++ 源文件</source>
        <translation>C++ Source File</translation>
    </message>
    <message>
        <source>新建 C++ 文件</source>
        <translation>New C++ File</translation>
    </message>
    <message>
        <source>C++ 头文件</source>
        <translation>C++ Header File</translation>
    </message>
    <message>
        <source>新建 C++ 头文件</source>
        <translation>New C++ Header</translation>
    </message>
    <message>
        <source>Python 文件</source>
        <translation>Python File</translation>
    </message>
    <message>
        <source>新建 Python 文件</source>
        <translation>New Python File</translation>
    </message>
    <message>
        <source>JavaScript 文件</source>
        <translation>JavaScript File</translation>
    </message>
    <message>
        <source>新建 JavaScript 文件</source>
        <translation>New JavaScript File</translation>
    </message>
    <message>
        <source>HTML 文件</source>
        <translation>HTML File</translation>
    </message>
    <message>
        <source>新建 HTML 文件</source>
        <translation>New HTML File</translation>
    </message>
    <message>
        <source>纯文本文件</source>
        <translation>Plain Text File</translation>
    </message>
    <message>
        <source>新建文本文件</source>
        <translation>New Text File</translation>
    </message>
    <message>
        <source>项目</source>
        <translation>Project</translation>
    </message>
    <message>
        <source>打开(&amp;O)</source>
        <translation>Open(&amp;O)</translation>
    </message>
    <message>
        <source>快速打开文件...</source>
        <translation>Quick Open File...</translation>
    </message>
    <message>
        <source>输入文件名快速打开 (Ctrl+O)</source>
        <translation>Type a file name to open quickly (Ctrl+O)</translation>
    </message>
    <message>
        <source>打开文件...</source>
        <translation>Open File...</translation>
    </message>
    <message>
        <source>保存(&amp;S)</source>
        <translation>Save(&amp;S)</translation>
    </message>
    <message>
        <source>保存全部(&amp;A)</source>
        <translation>Save All(&amp;A)</translation>
    </message>
    <message>
        <source>另存为(&amp;A)</source>
        <translation>Save As(&amp;A)</translation>
    </message>
    <message>
        <source>关闭(&amp;C)</source>
        <translation>Close(&amp;C)</translation>
    </message>
    <message>
        <source>退出(&amp;X)</source>
        <translation>Exit(&amp;X)</translation>
    </message>
    <message>
        <source>编辑(&amp;E)</source>
        <translation>Edit(&amp;E)</translation>
    </message>
    <message>
        <source>撤销(&amp;U)</source>
        <translation>Undo(&amp;U)</translation>
    </message>
    <message>
        <source>重做(&amp;R)</source>
        <translation>Redo(&amp;R)</translation>
    </message>
    <message>
        <source>剪切(&amp;T)</source>
        <translation>Cut(&amp;T)</translation>
    </message>
    <message>
        <source>复制(&amp;C)</source>
        <translation>Copy(&amp;C)</translation>
    </message>
    <message>
        <source>粘贴(&amp;P)</source>
        <translation>Paste(&amp;P)</translation>
    </message>
    <message>
        <source>复制行(&amp;D)</source>
        <translation>Duplicate Line(&amp;D)</translation>
    </message>
    <message>
        <source>删除行(&amp;Y)</source>
        <translation>Delete Line(&amp;Y)</translation>
    </message>
    <message>
        <source>安装依赖</source>
        <translation>Install Dependencies</translation>
    </message>
    <message>
        <source>提示</source>
        <translation>Notice</translation>
    </message>
    <message>
        <source>未打开 Python 项目</source>
        <translation>No Python project is open</translation>
    </message>
    <message>
        <source>创建虚拟环境</source>
        <translation>Create Virtual Environment</translation>
    </message>
    <message>
        <source>格式化代码</source>
        <translation>Format Code</translation>
    </message>
    <message>
        <source>当前文件不是 Python 文件</source>
        <translation>The current file is not a Python file</translation>
    </message>
    <message>
        <source>设置...</source>
        <translation>Settings...</translation>
    </message>
    <message>
        <source>跳转到行(&amp;L)...</source>
        <translation>Go to Line(&amp;L)...</translation>
    </message>
    <message>
        <source>查找(&amp;F)</source>
        <translation>Find(&amp;F)</translation>
    </message>
    <message>
        <source>替换(&amp;R)</source>
        <translation>Replace(&amp;R)</translation>
    </message>
    <message>
        <source>在文件中查找(&amp;F)...</source>
        <translation>Find in Files(&amp;F)...</translation>
    </message>
    <message>
        <source>在整个项目中搜索文本 (Ctrl+Shift+F)</source>
        <translation>Search text in the whole project (Ctrl+Shift+F)</translation>
    </message>
    <message>
        <source>格式化代码(&amp;O)</source>
        <translation>Format Code(&amp;O)</translation>
    </message>
    <message>
        <source>注释(&amp;C)</source>
        <translation>Comment(&amp;C)</translation>
    </message>
    <message>
        <source>构建(&amp;B)</source>
        <translation>Build(&amp;B)</translation>
    </message>
    <message>
        <source>编译当前文件(&amp;C)</source>
        <translation>Compile Current File(&amp;C)</translation>
    </message>
    <message>
        <source>编译项目(&amp;P)</source>
        <translation>Build Project(&amp;P)</translation>
    </message>
    <message>
        <source>清理构建(&amp;O)</source>
        <translation>Clean Build(&amp;O)</translation>
    </message>
    <message>
        <source>调试(&amp;D)</source>
        <translation>Debug(&amp;D)</translation>
    </message>
    <message>
        <source>启动调试(&amp;S)</source>
        <translation>Start Debugging(&amp;S)</translation>
    </message>
    <message>
        <source>停止调试(&amp;T)</source>
        <translation>Stop Debugging(&amp;T)</translation>
    </message>
    <message>
        <source>步过(&amp;N)</source>
        <translation>Step Over(&amp;N)</translation>
    </message>
    <message>
        <source>步入(&amp;S)</source>
        <translation>Step Into(&amp;S)</translation>
    </message>
    <message>
        <source>步出(&amp;O)</source>
        <translation>Step Out(&amp;O)</translation>
    </message>
    <message>
        <source>继续(&amp;C)</source>
        <translation>Continue(&amp;C)</translation>
    </message>
    <message>
        <source>添加断点(&amp;B)</source>
        <translation>Add Breakpoint(&amp;B)</translation>
    </message>
    <message>
        <source>打开 Git 窗口(&amp;O)</source>
        <translation>Open Git Window(&amp;O)</translation>
    </message>
    <message>
        <source>显示 / 隐藏 Git 窗口 (Ctrl+Shift+G)</source>
        <translation>Show / hide the Git window (Ctrl+Shift+G)</translation>
    </message>
    <message>
        <source>初始化仓库(&amp;I)</source>
        <translation>Initialize Repository(&amp;I)</translation>
    </message>
    <message>
        <source>提交(&amp;C)...</source>
        <translation>Commit(&amp;C)...</translation>
    </message>
    <message>
        <source>获取(&amp;F)</source>
        <translation>Fetch(&amp;F)</translation>
    </message>
    <message>
        <source>拉取(&amp;P)</source>
        <translation>Pull(&amp;P)</translation>
    </message>
    <message>
        <source>推送(&amp;U)</source>
        <translation>Push(&amp;U)</translation>
    </message>
    <message>
        <source>刷新状态(&amp;R)</source>
        <translation>Refresh Status(&amp;R)</translation>
    </message>
    <message>
        <source>查看历史(&amp;H)</source>
        <translation>View History(&amp;H)</translation>
    </message>
    <message>
        <source>工具(&amp;T)</source>
        <translation>Tools(&amp;T)</translation>
    </message>
    <message>
        <source>主题设置(&amp;H)</source>
        <translation>Theme(&amp;H)</translation>
    </message>
    <message>
        <source>浅色主题(&amp;L)</source>
        <translation>Light Theme(&amp;L)</translation>
    </message>
    <message>
        <source>深色主题(&amp;D)</source>
        <translation>Dark Theme(&amp;D)</translation>
    </message>
    <message>
        <source>插件管理(&amp;P)</source>
        <translation>Plugin Manager(&amp;P)</translation>
    </message>
    <message>
        <source>性能监控(&amp;M)</source>
        <translation>Performance Monitor(&amp;M)</translation>
    </message>
    <message>
        <source>显示 / 隐藏状态栏性能监控指示器</source>
        <translation>Show / hide the performance monitor in the status bar</translation>
    </message>
    <message>
        <source>终端(&amp;T)</source>
        <translation>Terminal(&amp;T)</translation>
    </message>
    <message>
        <source>视图(&amp;V)</source>
        <translation>View(&amp;V)</translation>
    </message>
    <message>
        <source>底部面板(&amp;B)</source>
        <translation>Bottom Panel(&amp;B)</translation>
    </message>
    <message>
        <source>显示 / 隐藏底部面板 (Ctrl+J)</source>
        <translation>Show / hide the bottom panel (Ctrl+J)</translation>
    </message>
    <message>
        <source>项目面板(&amp;P)</source>
        <translation>Project Panel(&amp;P)</translation>
    </message>
    <message>
        <source>显示 / 隐藏项目面板 (Ctrl+Shift+E)</source>
        <translation>Show / hide the project panel (Ctrl+Shift+E)</translation>
    </message>
    <message>
        <source>帮助(&amp;H)</source>
        <translation>Help(&amp;H)</translation>
    </message>
    <message>
        <source>快速指南(&amp;G)</source>
        <translation>Quick Start(&amp;G)</translation>
    </message>
    <message>
        <source>文档中心(&amp;D)</source>
        <translation>Documentation(&amp;D)</translation>
    </message>
    <message>
        <source>关于(&amp;A)</source>
        <translation>About(&amp;A)</translation>
    </message>
    <message>
        <source>主工具栏</source>
        <translation>Main Toolbar</translation>
    </message>
    <message>
        <source>新建</source>
        <translation>New</translation>
    </message>
    <message>
        <source>打开</source>
        <translation>Open</translation>
    </message>
    <message>
        <source>保存</source>
        <translation>Save</translation>
    </message>
    <message>
        <source>编译</source>
        <translation>Compile</translation>
    </message>
    <message>
        <source>运行</source>
        <translation>Run</translation>
    </message>
    <message>
        <source>调试</source>
        <translation>Debug</translation>
    </message>
    <message>
        <source>就绪</source>
        <translation>Ready</translation>
    </message>
    <message>
        <source>行: 1  列: 1</source>
        <translation>Ln 1, Col 1</translation>
    </message>
    <message>
        <source>空格: 4</source>
        <translation>Spaces: 4</translation>
    </message>
    <message>
        <source>项目面板</source>
        <translation>Project Panel</translation>
    </message>
    <message>
        <source>面板</source>
        <translation>Panel</translation>
    </message>
    <message>
        <source>输出</source>
        <translation>Output</translation>
    </message>
    <message>
        <source>终端</source>
        <translation>Terminal</translation>
    </message>
    <message>
        <source>没有断点可禁用</source>
        <translation>No breakpoints to disable</translation>
    </message>
    <message>
        <source>已禁用所有断点</source>
        <translation>All breakpoints disabled</translation>
    </message>
    <message>
        <source>已恢复所有断点</source>
        <translation>All breakpoints enabled</translation>
    </message>
    <message>
        <source>正在编译</source>
        <translation>Compiling</translation>
    </message>
    <message>
        <source>当前正在编译，是否停止编译并退出？</source>
        <translation>Compiling. Stop and exit?</translation>
    </message>
    <message>
        <source>未保存的更改</source>
        <translation>Unsaved Changes</translation>
    </message>
    <message>
        <source>有未保存的更改，是否保存？</source>
        <translation>There are unsaved changes. Save them?</translation>
    </message>
    <message>
        <source>行: %1  列: %2</source>
        <translation>Ln %1, Col %2</translation>
    </message>
    <message>
        <source>打开项目失败</source>
        <translation>Failed to Open Project</translation>
    </message>
    <message>
        <source>无法打开项目：</source>
        <translation>Cannot open project: </translation>
    </message>
    <message>
        <source>打开错误</source>
        <translation>Open Error</translation>
    </message>
    <message>
        <source>无法打开文件:
%1</source>
        <translation>Cannot open file:
%1</translation>
    </message>
    <message>
        <source>文件 '%1' 有未保存的更改。
是否保存更改？</source>
        <translation>File '%1' has unsaved changes.
Save changes?</translation>
    </message>
    <message>
        <source>不保存(&amp;D)</source>
        <translation>Don't Save(&amp;D)</translation>
    </message>
    <message>
        <source>取消(&amp;C)</source>
        <translation>Cancel(&amp;C)</translation>
    </message>
    <message>
        <source>正在编辑: %1</source>
        <translation>Editing: %1</translation>
    </message>
    <message>
        <source>行: -  列: -</source>
        <translation>Ln -, Col -</translation>
    </message>
    <message>
        <source>已加载: %1</source>
        <translation>Loaded: %1</translation>
    </message>
    <message>
        <source>文件已保存</source>
        <translation>File saved</translation>
    </message>
    <message>
        <source>已重命名: %1 → %2</source>
        <translation>Renamed: %1 → %2</translation>
    </message>
    <message>
        <source>调试已结束</source>
        <translation>Debugging finished</translation>
    </message>
    <message>
        <source>=== 调试会话结束 ===</source>
        <translation>=== Debug session ended ===</translation>
    </message>
    <message>
        <source>[%1] 开始编译: %2</source>
        <translation>[%1] Build started: %2</translation>
    </message>
    <message>
        <source>正在编译 %1...</source>
        <translation>Compiling %1...</translation>
    </message>
    <message>
        <source>正在编译... %1% - %2</source>
        <translation>Compiling... %1% - %2</translation>
    </message>
    <message>
        <source>正在编译... %1%</source>
        <translation>Compiling... %1%</translation>
    </message>
    <message>
        <source>[%1] 编译进度: %2%</source>
        <translation>[%1] Build progress: %2%</translation>
    </message>
    <message>
        <source>[%1] 编译成功! 耗时: %2ms</source>
        <translation>[%1] Build succeeded! Time: %2 ms</translation>
    </message>
    <message>
        <source> 错误: %1, 警告: %2</source>
        <translation> Errors: %1, Warnings: %2</translation>
    </message>
    <message>
        <source>编译成功</source>
        <translation>Build succeeded</translation>
    </message>
    <message>
        <source>[%1] 编译失败! 耗时: %2ms</source>
        <translation>[%1] Build failed! Time: %2 ms</translation>
    </message>
    <message>
        <source>编译失败</source>
        <translation>Build failed</translation>
    </message>
    <message>
        <source>错误: %1, 警告: %2</source>
        <translation>Errors: %1, Warnings: %2</translation>
    </message>
    <message>
        <source>[%1] 编译错误: %2</source>
        <translation>[%1] Build error: %2</translation>
    </message>
    <message>
        <source>编译错误</source>
        <translation>Build error</translation>
    </message>
    <message>
        <source>运行错误</source>
        <translation>Run Error</translation>
    </message>
    <message>
        <source>可执行文件不存在:
%1</source>
        <translation>Executable does not exist:
%1</translation>
    </message>
    <message>
        <source>; echo '程序执行完毕，按任意键退出...'; read -n1</source>
        <translation>; echo 'Program finished. Press any key to exit...'; read -n1</translation>
    </message>
    <message>
        <source>[%1] 程序执行完毕，退出码: %2</source>
        <translation>[%1] Program finished, exit code: %2</translation>
    </message>
    <message>
        <source>[%1] 程序异常退出</source>
        <translation>[%1] Program exited abnormally</translation>
    </message>
    <message>
        <source>程序执行完成</source>
        <translation>Program finished</translation>
    </message>
    <message>
        <source>无法启动程序</source>
        <translation>Failed to start the program</translation>
    </message>
    <message>
        <source>程序运行中...</source>
        <translation>Running...</translation>
    </message>
    <message>
        <source>请先打开一个文件再开始调试。</source>
        <translation>Open a file before starting debugging.</translation>
    </message>
    <message>
        <source>错误</source>
        <translation>Error</translation>
    </message>
    <message>
        <source>请先保存文件。</source>
        <translation>Save the file first.</translation>
    </message>
    <message>
        <source>保存错误</source>
        <translation>Save Error</translation>
    </message>
    <message>
        <source>无法保存文件，调试中止。</source>
        <translation>Failed to save the file; debugging aborted.</translation>
    </message>
    <message>
        <source>当前正在编译，请等待编译完成后再调试。</source>
        <translation>Compiling, please wait before debugging.</translation>
    </message>
    <message>
        <source>当前正在调试中，请先停止调试。</source>
        <translation>Already debugging. Stop the current session first.</translation>
    </message>
    <message>
        <source>=== 调试会话开始: %1 ===</source>
        <translation>=== Debug session started: %1 ===</translation>
    </message>
    <message>
        <source>编译并调试</source>
        <translation>Compile and Debug</translation>
    </message>
    <message>
        <source>文件尚未编译或已过期，是否现在编译并调试?</source>
        <translation>The file has not been compiled or is out of date. Compile and debug now?</translation>
    </message>
    <message>
        <source>=== C++ 调试会话开始: %1 ===</source>
        <translation>=== C++ debug session started: %1 ===</translation>
    </message>
    <message>
        <source>调试已停止</source>
        <translation>Debugging stopped</translation>
    </message>
    <message>
        <source>正在启动调试...</source>
        <translation>Starting debugger...</translation>
    </message>
    <message>
        <source>调试运行中</source>
        <translation>Debugger running</translation>
    </message>
    <message>
        <source>调试暂停（断点命中）</source>
        <translation>Paused (breakpoint hit)</translation>
    </message>
    <message>
        <source>单步执行中...</source>
        <translation>Stepping...</translation>
    </message>
    <message>
        <source>调试错误</source>
        <translation>Debug error</translation>
    </message>
    <!-- ============ 新建项目 / 项目类型 ============ -->
    <message>
            <source>项目创建成功！</source>
            <translation>Project created successfully!</translation>
        </message>
        <message>
            <source>项目名称: %1</source>
            <translation>Project name: %1</translation>
        </message>
        <message>
            <source>项目类型: %1</source>
            <translation>Project type: %1</translation>
        </message>
        <message>
            <source>项目路径: %1</source>
            <translation>Project path: %1</translation>
        </message>
        <message>
            <source>已创建项目: %1</source>
            <translation>Project created: %1</translation>
        </message>
        <message>
            <source>无法创建项目:
    %1</source>
            <translation>Cannot create project:
    %1</translation>
        </message>
        <message>
            <source>控制台应用程序</source>
            <translation>Console Application</translation>
        </message>
        <message>
            <source>简单游戏</source>
            <translation>Simple Game</translation>
        </message>
        <message>
            <source>图形界面程序</source>
            <translation>GUI Application</translation>
        </message>
        <message>
            <source>空白项目</source>
            <translation>Empty Project</translation>
        </message>
        <message>
            <source>未知类型</source>
            <translation>Unknown Type</translation>
        </message>

        <!-- ============ 格式化 / Clang-Format ============ -->
        <message>
            <source>请先打开一个文件进行格式化。</source>
            <translation>Open a file before formatting.</translation>
        </message>
        <message>
            <source>代码为空，无需格式化。</source>
            <translation>The code is empty; nothing to format.</translation>
        </message>
        <message>
            <source>无法创建临时文件。</source>
            <translation>Cannot create a temporary file.</translation>
        </message>
        <message>
            <source>请先保存文件。</source>
            <translation>Please save the file first.</translation>
        </message>
        <message>
            <source>Python 代码格式化完成。</source>
            <translation>Python code formatted.</translation>
        </message>
        <message>
            <source>Python 格式化失败: </source>
            <translation>Python formatting failed: </translation>
        </message>
        <message>
            <source>未找到 Black 格式化工具。
    请安装: pip install black</source>
            <translation>Black formatter not found.
    Install it with: pip install black</translation>
        </message>
        <message>
            <source>未找到Clang Format工具，请确保已安装。</source>
            <translation>Clang Format not found. Make sure it is installed.</translation>
        </message>
        <message>
            <source>Clang Format执行超时。</source>
            <translation>Clang Format timed out.</translation>
        </message>
        <message>
            <source>Clang Format错误: %1</source>
            <translation>Clang Format error: %1</translation>
        </message>
        <message>
            <source>代码格式化完成。</source>
            <translation>Code formatted.</translation>
        </message>
        <message>
            <source>格式化后代码为空，可能是输入代码有问题。</source>
            <translation>The formatted code is empty; the input may be invalid.</translation>
        </message>

        <!-- ============ 跳转 / 查找 ============ -->
        <message>
            <source>请先打开一个文件进行跳转。</source>
            <translation>Open a file before jumping.</translation>
        </message>
        <message>
            <source>请输入行号:</source>
            <translation>Enter line number:</translation>
        </message>
        <message>
            <source>请先打开一个文件进行查找。</source>
            <translation>Open a file before searching.</translation>
        </message>
        <message>
            <source>请先打开一个文件进行替换。</source>
            <translation>Open a file before replacing.</translation>
        </message>
        <message>
            <source>在文件中查找</source>
            <translation>Find in Files</translation>
        </message>
        <message>
            <source>请先打开一个项目或文件。</source>
            <translation>Open a project or a file first.</translation>
        </message>

        <!-- ============ 快速打开 ============ -->
        <message>
            <source>快速打开</source>
            <translation>Quick Open</translation>
        </message>
        <message>
            <source>项目中没有可打开的文件。</source>
            <translation>No files to open in the project.</translation>
        </message>
        <message>
            <source>文件不存在: %1</source>
            <translation>File does not exist: %1</translation>
        </message>

        <!-- ============ 保存 / 另存 ============ -->
        <message>
            <source>保存错误</source>
            <translation>Save Error</translation>
        </message>
        <message>
            <source>无法保存文件。</source>
            <translation>Cannot save the file.</translation>
        </message>
        <message>
            <source>没有打开的文件可以保存。</source>
            <translation>No open file to save.</translation>
        </message>
        <message>
            <source>另存为</source>
            <translation>Save As</translation>
        </message>
        <message>
            <source>C++ 文件 (*.cpp *.h *.hpp);;所有文件 (*.*)</source>
            <translation>C++ Files (*.cpp *.h *.hpp);;All Files (*.*)</translation>
        </message>
        <message>
            <source>已另存为: %1</source>
            <translation>Saved as: %1</translation>
        </message>
        <message>
            <source>没有打开的文件可以另存。</source>
            <translation>No open file to save as.</translation>
        </message>

        <!-- ============ 保存全部 / 程序运行 ============ -->
        <message>
            <source>已保存所有文件</source>
            <translation>All files saved</translation>
        </message>
        <message>
            <source>请先打开一个文件进行编译。</source>
            <translation>Open a file before compiling.</translation>
        </message>
        <message>
            <source>文件未保存</source>
            <translation>File Not Saved</translation>
        </message>
        <message>
            <source>请先保存文件再进行编译。</source>
            <translation>Save the file before compiling.</translation>
        </message>
        <message>
            <source>无法保存文件，编译中止。</source>
            <translation>Cannot save file; compilation aborted.</translation>
        </message>
        <message>
            <source>无法启动编译任务。</source>
            <translation>Cannot start the compilation task.</translation>
        </message>
        <message>
            <source>请先打开一个文件进行运行。</source>
            <translation>Open a file before running.</translation>
        </message>
        <message>
            <source>请先保存文件再进行运行。</source>
            <translation>Save the file before running.</translation>
        </message>
        <message>
            <source>无法保存文件，运行中止。</source>
            <translation>Cannot save file; run aborted.</translation>
        </message>
        <message>
            <source>编译并运行</source>
            <translation>Compile and Run</translation>
        </message>
        <message>
            <source>文件尚未编译或已过期，是否现在编译并运行?</source>
            <translation>The file has not been compiled or is out of date. Compile and run now?</translation>
        </message>
        <message>
            <source>使用可执行文件: %1</source>
            <translation>Using executable: %1</translation>
        </message>
        <message>
            <source>使用 Python 解释器: %1</source>
            <translation>Using Python interpreter: %1</translation>
        </message>
        <message>
            <source>工作目录: %1</source>
            <translation>Working directory: %1</translation>
        </message>

        <!-- ============ 项目构建 ============ -->
        <message>
            <source>提示</source>
            <translation>Notice</translation>
        </message>
        <message>
            <source>无法确定项目路径。请先打开一个文件，或通过 "文件 → 打开项目" 打开项目根目录。</source>
            <translation>Cannot determine the project path. Open a file, or use "File → Open Project" to open the project root.</translation>
        </message>
        <message>
            <source>无法识别项目类型</source>
            <translation>Unknown Project Type</translation>
        </message>
        <message>
            <source>未能识别项目语言。请确保项目根目录包含对应的构建文件（如 pom.xml, CMakeLists.txt 等）。</source>
            <translation>Could not identify the project language. Make sure the project root contains the corresponding build file (e.g. pom.xml, CMakeLists.txt).</translation>
        </message>
        <message>
            <source>无法自动识别项目语言，请确保项目根目录包含相应的构建文件。</source>
            <translation>Could not auto-detect the project language. Make sure the project root contains the corresponding build file.</translation>
        </message>
        <message>
            <source>清理构建</source>
            <translation>Clean Build</translation>
        </message>
        <message>
            <source>确定要清理编译产物吗？这将删除所有生成的文件。</source>
            <translation>Clean build outputs? This will delete all generated files.</translation>
        </message>
        <message>
            <source>清理构建完成</source>
            <translation>Clean completed</translation>
        </message>
        <message>
            <source>清理构建失败</source>
            <translation>Clean failed</translation>
        </message>
        <message>
            <source>清理构建失败: </source>
            <translation>Clean failed: </translation>
        </message>
        <message>
            <source>检测到的编译器:</source>
            <translation>Detected compilers:</translation>
        </message>
        <message>
            <source>Qt 项目，切换到项目构建模式</source>
            <translation>Qt project; switching to project build mode</translation>
        </message>
        <message>
            <source>C++ 项目构建系统: CMake</source>
            <translation>C++ build system: CMake</translation>
        </message>
        <message>
            <source>C++ 项目构建系统: Makefile</source>
            <translation>C++ build system: Makefile</translation>
        </message>
        <message>
            <source>C++ 项目构建系统: SingleFile</source>
            <translation>C++ build system: Single File</translation>
        </message>
        <message>
            <source>Qt SDK: %1</source>
            <translation>Qt SDK: %1</translation>
        </message>
        <message>
            <source>警告：未在设置里配置 QT_DIR，CMake 将依赖系统自动探测 Qt。</source>
            <translation>Warning: QT_DIR is not configured in settings; CMake will rely on system auto-detection for Qt.</translation>
        </message>

        <!-- ============ 关于 / 快速指南 ============ -->
        <message>
            <source>快速开始指南</source>
            <translation>Quick Start Guide</translation>
        </message>
        <message>
            <source>SprintIDE 快速开始：
    1. 新建或打开C++文件
    2. 编写代码
    3. 保存文件
    4. 编译运行</source>
            <translation>SprintIDE Quick Start:
    1. Create or open a C++ file
    2. Write your code
    3. Save the file
    4. Compile and run</translation>
        </message>
        <message>
            <source>关于 SprintIDE</source>
            <translation>About SprintIDE</translation>
        </message>
        <message>
            <source>SprintIDE v1.0.0
    轻量级 C++ 集成开发环境
    刘腾骏胡乱做的</source>
            <translation>SprintIDE v1.0.0
    A lightweight C++ IDE
    Made by Liu Tengjun</translation>
        </message>

        <!-- ============ 主题切换 ============ -->
        <message>
            <source>已切换到深色主题</source>
            <translation>Switched to dark theme</translation>
        </message>
        <message>
            <source>主题已切换为深色</source>
            <translation>Theme switched to dark</translation>
        </message>
        <message>
            <source>已切换到浅色主题</source>
            <translation>Switched to light theme</translation>
        </message>
        <message>
            <source>主题已切换为浅色</source>
            <translation>Theme switched to light</translation>
        </message>

        <!-- ============ 新建文件对话框 ============ -->
        <message>
            <source>请输入文件名（含扩展名）:
    例如: main.cpp, test.py, app.js, index.html</source>
            <translation>Enter the file name (with extension):
    e.g. main.cpp, test.py, app.js, index.html</translation>
        </message>
        <message>
            <source>文件已存在</source>
            <translation>File Already Exists</translation>
        </message>
        <message>
            <source>文件 '%1' 已存在，是否打开它？</source>
            <translation>File '%1' already exists. Open it?</translation>
        </message>
        <message>
            <source>无法创建目录:
    %1</source>
            <translation>Cannot create directory:
    %1</translation>
        </message>
        <message>
            <source>无法创建文件:
    %1
    错误: %2</source>
            <translation>Cannot create file:
    %1
    Error: %2</translation>
        </message>
        <message>
            <source>已创建文件: %1</source>
            <translation>File created: %1</translation>
        </message>
        <message>
            <source>已创建: %1</source>
            <translation>Created: %1</translation>
        </message>

        <!-- ============ 本地历史对话框 ============ -->
        <message>
            <source>本地历史 - %1</source>
            <translation>Local History - %1</translation>
        </message>
        <message>
            <source>本地历史</source>
            <translation>Local History</translation>
        </message>
        <message>
            <source>该文件没有历史记录。
    保存文件时会自动创建历史版本。</source>
            <translation>This file has no history.
    History versions are created automatically when you save the file.</translation>
        </message>
        <message>
            <source>共 %1 个版本</source>
            <translation>%1 versions</translation>
        </message>
        <message>
            <source>恢复到选中版本</source>
            <translation>Restore Selected Version</translation>
        </message>
        <message>
            <source>关闭</source>
            <translation>Close</translation>
        </message>
        <message>
            <source>差异对比（选中版本 vs 当前文件）</source>
            <translation>Diff (selected version vs current file)</translation>
        </message>
        <message>
            <source>无法读取版本内容</source>
            <translation>Cannot read version content</translation>
        </message>
        <message>
            <source>请先选择一个版本</source>
            <translation>Please select a version first</translation>
        </message>
        <message>
            <source>恢复失败</source>
            <translation>Restore Failed</translation>
        </message>
        <message>
            <source>恢复成功</source>
            <translation>Restore Succeeded</translation>
        </message>
        <message>
            <source>已恢复到版本 %1</source>
            <translation>Restored to version %1</translation>
        </message>
        <message>
            <source>无法写入文件</source>
            <translation>Cannot write file</translation>
        </message>
</context>
<context>
    <name>SprintIDE::OutputPanel</name>
    <message>
        <source>在输出中过滤...</source>
        <translation>Filter output...</translation>
    </message>
    <message>
        <source>错误</source>
        <translation>Error</translation>
    </message>
    <message>
        <source>警告</source>
        <translation>Warning</translation>
    </message>
    <message>
        <source>信息</source>
        <translation>Info</translation>
    </message>
    <message>
        <source>↓ 跟随</source>
        <translation>↓ Follow</translation>
    </message>
    <message>
        <source>自动滚动到最新</source>
        <translation>Auto-scroll to latest</translation>
    </message>
    <message>
        <source>清空</source>
        <translation>Clear</translation>
    </message>
    <message>
        <source>错误 %1 · 警告 %2 · 信息 %3</source>
        <translation>Errors %1 · Warnings %2 · Info %3</translation>
    </message>
    <message>
        <source>  ·  过滤后 %1 条</source>
        <translation>  ·  %1 after filtering</translation>
    </message>
    <message>
        <source>  ·  共 %1 条</source>
        <translation>  ·  %1 total</translation>
    </message>
    <message>
        <source>显示 / 隐藏错误</source>
        <translation>Show / hide errors</translation>
    </message>
    <message>
        <source>显示 / 隐藏警告</source>
        <translation>Show / hide warnings</translation>
    </message>
    <message>
        <source>显示 / 隐藏信息</source>
        <translation>Show / hide info</translation>
    </message>
    <message>
        <source>跟随最新输出（自动滚动到底部）</source>
        <translation>Follow latest output (auto-scroll to bottom)</translation>
    </message>
    <message>
        <source>清空输出</source>
        <translation>Clear output</translation>
    </message>
</context>
<context>
    <name>SprintIDE::ProjectPanel</name>
    <message>
        <source>重命名</source>
        <translation>Rename</translation>
    </message>
    <message>
        <source>新名称:</source>
        <translation>New name:</translation>
    </message>
    <message>
        <source>新建文件</source>
        <translation>New File</translation>
    </message>
    <message>
        <source>文件名:</source>
        <translation>File name:</translation>
    </message>
    <message>
        <source>新建目录</source>
        <translation>New Folder</translation>
    </message>
    <message>
        <source>目录名:</source>
        <translation>Folder name:</translation>
    </message>
    <message>
        <source>创建失败</source>
        <translation>Create Failed</translation>
    </message>
    <message>
        <source>无法创建目录: </source>
        <translation>Cannot create folder: </translation>
    </message>
    <message>
        <source>无法创建文件: </source>
        <translation>Cannot create file: </translation>
    </message>
    <message>
        <source>重命名失败</source>
        <translation>Rename Failed</translation>
    </message>
    <message>
        <source>目标文件已存在: </source>
        <translation>Target already exists: </translation>
    </message>
    <message>
        <source>无法重命名文件</source>
        <translation>Cannot rename file</translation>
    </message>
    <message>
        <source>确认删除</source>
        <translation>Confirm Delete</translation>
    </message>
    <message>
        <source>确定要删除 "%1" 吗？</source>
        <translation>Delete "%1"?</translation>
    </message>
    <message>
        <source>删除失败</source>
        <translation>Delete Failed</translation>
    </message>
    <message>
        <source>无法删除: </source>
        <translation>Cannot delete: </translation>
    </message>
    <message>
        <source>打开</source>
        <translation>Open</translation>
    </message>
    <message>
        <source>删除</source>
        <translation>Delete</translation>
    </message>
    <message>
        <source>复制路径</source>
        <translation>Copy Path</translation>
    </message>
    <message>
        <source>在文件管理器中显示</source>
        <translation>Show in File Manager</translation>
    </message>
    <message>
        <source>查看本地历史</source>
        <translation>Local History</translation>
    </message>
    <message>
        <source>刷新</source>
        <translation>Refresh</translation>
    </message>
    <message>
        <source>展开</source>
        <translation>Expand</translation>
    </message>
    <message>
        <source>测试 (pytest)</source>
        <translation>Tests (pytest)</translation>
    </message>
    <message>
        <source>Git</source>
        <translation>Git</translation>
    </message>
    <message>
        <source>提交...</source>
        <translation>Commit...</translation>
    </message>
    <message>
        <source>添加</source>
        <translation>Add</translation>
    </message>
    <message>
        <source>使用 Git 追溯注解</source>
        <translation>Annotate with Git</translation>
    </message>
    <message>
        <source>显示差异</source>
        <translation>Show Diff</translation>
    </message>
    <message>
        <source>与修订比较...</source>
        <translation>Compare with Revision...</translation>
    </message>
    <message>
        <source>与分支比较...</source>
        <translation>Compare with Branch...</translation>
    </message>
    <message>
        <source>显示历史记录</source>
        <translation>Show History</translation>
    </message>
    <message>
        <source>显示当前修订</source>
        <translation>Show Current Revision</translation>
    </message>
    <message>
        <source>回滚...</source>
        <translation>Rollback...</translation>
    </message>
    <message>
        <source>推送...</source>
        <translation>Push...</translation>
    </message>
    <message>
        <source>拉取...</source>
        <translation>Pull...</translation>
    </message>
    <message>
        <source>提取</source>
        <translation>Fetch</translation>
    </message>
    <message>
        <source>合并...</source>
        <translation>Merge...</translation>
    </message>
    <message>
        <source>变基...</source>
        <translation>Rebase...</translation>
    </message>
    <message>
        <source>分支...</source>
        <translation>Branches...</translation>
    </message>
    <message>
        <source>新建分支...</source>
        <translation>New Branch...</translation>
    </message>
    <message>
        <source>新建标记...</source>
        <translation>New Tag...</translation>
    </message>
    <message>
        <source>重置 HEAD...</source>
        <translation>Reset HEAD...</translation>
    </message>
    <message>
        <source>存储变更...</source>
        <translation>Stash Changes...</translation>
    </message>
    <message>
        <source>取消存储变更...</source>
        <translation>Unstash Changes...</translation>
    </message>
    <message>
        <source>管理远程...</source>
        <translation>Manage Remotes...</translation>
    </message>
    <message>
        <source>将在后续版本实现</source>
        <translation>To be implemented in a future release</translation>
    </message>
</context>
<context>
    <name>SprintIDE::PythonSettingsPage</name>
    <message>
        <source>依赖管理</source>
        <translation>Dependency Management</translation>
    </message>
    <message>
        <source>默认依赖管理器:</source>
        <translation>Default dependency manager:</translation>
    </message>
    <message>
        <source>打开项目时自动安装依赖</source>
        <translation>Automatically install dependencies when opening a project</translation>
    </message>
    <message>
        <source>例如: --index-url https://pypi.tuna.tsinghua.edu.cn/simple</source>
        <translation>e.g. --index-url https://pypi.tuna.tsinghua.edu.cn/simple</translation>
    </message>
    <message>
        <source>pip 额外参数:</source>
        <translation>Extra pip arguments:</translation>
    </message>
    <message>
        <source>工具</source>
        <translation>Tools</translation>
    </message>
    <message>
        <source>测试运行器:</source>
        <translation>Test runner:</translation>
    </message>
    <message>
        <source>代码格式化工具:</source>
        <translation>Code formatter:</translation>
    </message>
</context>
<context>
    <name>SprintIDE::QuickOpenDialog</name>
    <message>
        <source>快速打开文件</source>
        <translation>Quick Open File</translation>
    </message>
    <message>
        <source>输入文件名进行过滤（支持路径片段，如 src/main.cpp）</source>
        <translation>Type a file name to filter (path fragments are supported, e.g. src/main.cpp)</translation>
    </message>
    <message>
        <source>↑↓ 选择 · Enter 打开 · Esc 关闭</source>
        <translation>↑↓ Select · Enter Open · Esc Close</translation>
    </message>
    <message>
        <source>共 %1 个匹配（显示前 %2） · ↑↓ 选择 · Enter 打开 · Esc 关闭</source>
        <translation>%1 matches (showing first %2) · ↑↓ Select · Enter Open · Esc Close</translation>
    </message>
    <message>
        <source>共 %1 个匹配 · ↑↓ 选择 · Enter 打开 · Esc 关闭</source>
        <translation>%1 matches · ↑↓ Select · Enter Open · Esc Close</translation>
    </message>
</context>
<context>
    <name>SprintIDE::SearchInFilesDialog</name>
    <message>
        <source>在文件中查找</source>
        <translation>Find in Files</translation>
    </message>
    <message>
        <source>输入查找内容...</source>
        <translation>Enter search text...</translation>
    </message>
    <message>
        <source>查找</source>
        <translation>Find</translation>
    </message>
    <message>
        <source>停止</source>
        <translation>Stop</translation>
    </message>
    <message>
        <source>大小写敏感</source>
        <translation>Case sensitive</translation>
    </message>
    <message>
        <source>全字匹配</source>
        <translation>Whole word</translation>
    </message>
    <message>
        <source>正则表达式</source>
        <translation>Regular expression</translation>
    </message>
    <message>
        <source>结果</source>
        <translation>Results</translation>
    </message>
    <message>
        <source>就绪</source>
        <translation>Ready</translation>
    </message>
    <message>
        <source>关闭</source>
        <translation>Close</translation>
    </message>
    <message>
        <source>待搜索文件：%1 个</source>
        <translation>Files to search: %1</translation>
    </message>
    <message>
        <source>没有可搜索的文件。请先打开项目。</source>
        <translation>No files to search. Open a project first.</translation>
    </message>
    <message>
        <source>正则表达式错误</source>
        <translation>Regular Expression Error</translation>
    </message>
    <message>
        <source>已停止，已找到 %1 处匹配</source>
        <translation>Stopped. %1 matches found.</translation>
    </message>
    <message>
        <source>匹配过多（超过 %1 条），已停止。请细化搜索条件。</source>
        <translation>Too many matches (more than %1). Stopped. Please refine your search.</translation>
    </message>
    <message>
        <source>完成。共 %1 处匹配，%2 个文件。</source>
        <translation>Done. %1 matches in %2 files.</translation>
    </message>
    <message>
        <source>扫描 %1/%2，找到 %3 处匹配...</source>
        <translation>Scanning %1/%2, %3 matches found...</translation>
    </message>
</context>
<context>
    <name>SprintIDE::Git::GitPanel</name>
    <message>
        <source>继续</source>
        <translation>Continue</translation>
    </message>
    <message>
        <source>中止</source>
        <translation>Abort</translation>
    </message>
    <message>
        <source>冲突全部解决后继续当前操作</source>
        <translation>Continue the current operation once all conflicts are resolved</translation>
    </message>
    <message>
        <source>此目录尚未初始化为 Git 仓库。

点击上方「初始化仓库」按钮开始使用。</source>
        <translation>This directory is not yet a Git repository.

Click "Initialize Repository" above to get started.</translation>
    </message>
    <message>
        <source>未打开项目。</source>
        <translation>No project is open.</translation>
    </message>
    <message>
        <source>分支操作</source>
        <translation>Branch Actions</translation>
    </message>
    <message>
        <source>新建分支...</source>
        <translation>New Branch...</translation>
    </message>
    <message>
        <source>合并分支到当前分支...</source>
        <translation>Merge Branch into Current...</translation>
    </message>
    <message>
        <source>将当前分支 Rebase 到...</source>
        <translation>Rebase Current Branch onto...</translation>
    </message>
    <message>
        <source>刷新状态 (F5)</source>
        <translation>Refresh Status (F5)</translation>
    </message>
    <message>
        <source> 初始化仓库</source>
        <translation> Initialize Repository</translation>
    </message>
    <message>
        <source>在当前项目根目录执行 git init</source>
        <translation>Run git init in the current project root</translation>
    </message>
    <message>
        <source>已暂存的更改 (0)</source>
        <translation>Staged Changes (0)</translation>
    </message>
    <message>
        <source>已暂存的更改 (%1)</source>
        <translation>Staged Changes (%1)</translation>
    </message>
    <message>
        <source>−</source>
        <translation>−</translation>
    </message>
    <message>
        <source>全部取消暂存</source>
        <translation>Unstage All</translation>
    </message>
    <message>
        <source>更改 (0)</source>
        <translation>Changes (0)</translation>
    </message>
    <message>
        <source>更改 (%1)</source>
        <translation>Changes (%1)</translation>
    </message>
    <message>
        <source>+</source>
        <translation>+</translation>
    </message>
    <message>
        <source>全部暂存</source>
        <translation>Stage All</translation>
    </message>
    <message>
        <source>提交信息 (Ctrl+Enter 提交)</source>
        <translation>Commit message (Ctrl+Enter to commit)</translation>
    </message>
    <message>
        <source>修正上次提交</source>
        <translation>Amend last commit</translation>
    </message>
    <message>
        <source>提交</source>
        <translation>Commit</translation>
    </message>
    <message>
        <source>提交 (%1)</source>
        <translation>Commit (%1)</translation>
    </message>
    <message>
        <source>Stash (0)</source>
        <translation>Stash (0)</translation>
    </message>
    <message>
        <source>Stash (%1)</source>
        <translation>Stash (%1)</translation>
    </message>
    <message>
        <source>创建 stash（暂存当前改动）</source>
        <translation>Create stash (stash current changes)</translation>
    </message>
    <message>
        <source>提交历史</source>
        <translation>Commit History</translation>
    </message>
    <message>
        <source>(非 Git 仓库)</source>
        <translation>(Not a Git repository)</translation>
    </message>
    <message>
        <source>(尚无提交)</source>
        <translation>(No commits yet)</translation>
    </message>
    <message>
        <source>游离 HEAD（%1）
当前不在任何分支上。建议创建一个分支以保存提交。</source>
        <translation>Detached HEAD (%1)
You are not on any branch. Create a branch to save your commits.</translation>
    </message>
    <message>
        <source>  正在进行：%1</source>
        <translation>  In progress: %1</translation>
    </message>
    <message>
        <source>（提示：本模式下「接受我的提交 / 目标分支」已自动按 rebase 语义处理）</source>
        <translation>(Note: in this mode, "Accept my commit / target branch" is automatically handled with rebase semantics)</translation>
    </message>
    <message>
        <source>初始化 Git 仓库</source>
        <translation>Initialize Git Repository</translation>
    </message>
    <message>
        <source>将在当前项目根目录执行 git init：

%1

继续？</source>
        <translation>Will run git init in the current project root:

%1

Continue?</translation>
    </message>
    <message>
        <source>初始化失败</source>
        <translation>Initialization Failed</translation>
    </message>
    <message>
        <source>无法初始化 Git 仓库：

%1</source>
        <translation>Cannot initialize Git repository:

%1</translation>
    </message>
    <message>
        <source>切换分支</source>
        <translation>Switch Branch</translation>
    </message>
    <message>
        <source>切换到分支：%1 ？</source>
        <translation>Switch to branch: %1 ?</translation>
    </message>
    <message>
        <source>  当前处于 &lt;b&gt;游离 HEAD&lt;/b&gt; 状态（%1）。&lt;br&gt;切走后，此处的提交将不属于任何分支，&lt;b&gt;可能难以找回&lt;/b&gt;。</source>
        <translation>  You are currently in a &lt;b&gt;detached HEAD&lt;/b&gt; state (%1).&lt;br&gt;After switching away, commits here will not belong to any branch and &lt;b&gt;may be hard to recover&lt;/b&gt;.</translation>
    </message>
    <message>
        <source>  当前有 &lt;b&gt;%1&lt;/b&gt; 个未提交的更改（含 %2 个未跟踪文件）。&lt;br&gt;切换分支时可能产生冲突或被合并到其他基线。</source>
        <translation>  There are &lt;b&gt;%1&lt;/b&gt; uncommitted changes (including %2 untracked files).&lt;br&gt;Switching branches may cause conflicts or merge them into a different baseline.</translation>
    </message>
    <message>
        <source>取消</source>
        <translation>Cancel</translation>
    </message>
    <message>
        <source>先暂存(stash)再切换</source>
        <translation>Stash first, then switch</translation>
    </message>
    <message>
        <source>仍然直接切换</source>
        <translation>Switch anyway</translation>
    </message>
    <message>
        <source>切换分支 %1 → %2 前自动暂存</source>
        <translation>Auto-stash before switching branch %1 → %2</translation>
    </message>
    <message>
        <source>新建分支</source>
        <translation>New Branch</translation>
    </message>
    <message>
        <source>例如 feature/diff-window</source>
        <translation>e.g. feature/diff-window</translation>
    </message>
    <message>
        <source>分支名:</source>
        <translation>Branch name:</translation>
    </message>
    <message>
        <source>当前 HEAD（%1）</source>
        <translation>Current HEAD (%1)</translation>
    </message>
    <message>
        <source>(detached: %1)</source>
        <translation>(detached: %1)</translation>
    </message>
    <message>
        <source>基点:</source>
        <translation>Base:</translation>
    </message>
    <message>
        <source>创建后立即切换到新分支</source>
        <translation>Switch to the new branch immediately after creation</translation>
    </message>
    <message>
        <source>创建</source>
        <translation>Create</translation>
    </message>
    <message>
        <source>分支名不能为空。</source>
        <translation>Branch name cannot be empty.</translation>
    </message>
    <message>
        <source>分支名包含非法字符。&lt;br&gt;&lt;br&gt;不允许：&lt;code&gt;空格 ~ ^ : ? * [ \\ .. &lt;/code&gt;，&lt;br&gt;不允许以 &lt;code&gt;- . /&lt;/code&gt; 开头，&lt;br&gt;不允许以 &lt;code&gt;.lock&lt;/code&gt; 或 &lt;code&gt;/&lt;/code&gt; 结尾。</source>
        <translation>Branch name contains invalid characters.&lt;br&gt;&lt;br&gt;Not allowed: &lt;code&gt;space ~ ^ : ? * [ \\ .. &lt;/code&gt;,&lt;br&gt;cannot start with &lt;code&gt;- . /&lt;/code&gt;,&lt;br&gt;cannot end with &lt;code&gt;.lock&lt;/code&gt; or &lt;code&gt;/&lt;/code&gt;.</translation>
    </message>
    <message>
        <source>分支 &lt;b&gt;%1&lt;/b&gt; 已存在。</source>
        <translation>Branch &lt;b&gt;%1&lt;/b&gt; already exists.</translation>
    </message>
    <message>
        <source>合并分支</source>
        <translation>Merge Branch</translation>
    </message>
    <message>
        <source>选择要合并到当前分支 &lt;b&gt;%1&lt;/b&gt; 的分支：</source>
        <translation>Choose the branch to merge into the current branch &lt;b&gt;%1&lt;/b&gt;:</translation>
    </message>
    <message>
        <source>确定</source>
        <translation>OK</translation>
    </message>
    <message>
        <source>没有其他可选的本地分支。</source>
        <translation>No other local branches are available.</translation>
    </message>
    <message>
        <source>合并</source>
        <translation>Merge</translation>
    </message>
    <message>
        <source>已有进行中的操作：%1，请先处理。</source>
        <translation>An operation is already in progress: %1. Please handle it first.</translation>
    </message>
    <message>
        <source>确认合并</source>
        <translation>Confirm Merge</translation>
    </message>
    <message>
        <source>将把 &lt;b&gt;%1&lt;/b&gt; 合并到 &lt;b&gt;%2&lt;/b&gt;。&lt;br&gt;&lt;br&gt;继续？</source>
        <translation>&lt;b&gt;%1&lt;/b&gt; will be merged into &lt;b&gt;%2&lt;/b&gt;.&lt;br&gt;&lt;br&gt;Continue?</translation>
    </message>
    <message>
        <source>Rebase</source>
        <translation>Rebase</translation>
    </message>
    <message>
        <source>工作区有 &lt;b&gt;%1&lt;/b&gt; 个未提交的更改。&lt;br&gt;&lt;br&gt;请先提交或 stash，再执行 rebase。</source>
        <translation>The working tree has &lt;b&gt;%1&lt;/b&gt; uncommitted changes.&lt;br&gt;&lt;br&gt;Please commit or stash before rebasing.</translation>
    </message>
    <message>
        <source>把当前分支 &lt;b&gt;%1&lt;/b&gt; rebase 到：</source>
        <translation>Rebase current branch &lt;b&gt;%1&lt;/b&gt; onto:</translation>
    </message>
    <message>
        <source>确认 Rebase</source>
        <translation>Confirm Rebase</translation>
    </message>
    <message>
        <source>将把当前分支 &lt;b&gt;%1&lt;/b&gt; 的所有提交重放到 &lt;b&gt;%2&lt;/b&gt; 之上。&lt;br&gt;&lt;br&gt;  Rebase 会重写当前分支的提交历史。&lt;br&gt;  如果该分支已推送到远程，之后需要 &lt;code&gt;git push --force&lt;/code&gt;。&lt;br&gt;  冲突时 ours/theirs 语义与 merge 相反。&lt;br&gt;&lt;br&gt;确定继续？</source>
        <translation>All commits on the current branch &lt;b&gt;%1&lt;/b&gt; will be replayed onto &lt;b&gt;%2&lt;/b&gt;.&lt;br&gt;...</translation>
    </message>
    <message>
        <source>开始 Rebase</source>
        <translation>Start Rebase</translation>
    </message>
    <message>
        <source>中止操作</source>
        <translation>Abort Operation</translation>
    </message>
    <message>
        <source>确定要中止当前的 &lt;b&gt;%1&lt;/b&gt; 吗？&lt;br&gt;&lt;br&gt;工作区里已解决的改动会保留，未提交的改动不会丢失。</source>
        <translation>Abort the current &lt;b&gt;%1&lt;/b&gt;?...</translation>
    </message>
    <message>
        <source>暂不支持中止 %1。</source>
        <translation>Aborting %1 is not supported yet.</translation>
    </message>
    <message>
        <source>继续 Rebase</source>
        <translation>Continue Rebase</translation>
    </message>
    <message>
        <source>仍有未解决的冲突文件，请先在「更改」列表里全部解决。</source>
        <translation>There are still unresolved conflicts. Please resolve all of them in the "Changes" list first.</translation>
    </message>
    <message>
        <source>Git 身份未配置</source>
        <translation>Git Identity Not Configured</translation>
    </message>
    <message>
        <source>提交前需要先配置 Git 身份。&lt;br&gt;&lt;br&gt;在终端运行以下命令：&lt;br&gt;&lt;code&gt;git config --global user.name "你的名字"&lt;/code&gt;&lt;br&gt;&lt;code&gt;git config --global user.email "你的邮箱"&lt;/code&gt;&lt;br&gt;&lt;br&gt;或打开设置对话框，在 &lt;b&gt;Git&lt;/b&gt; 页填写并保存。</source>
        <translation>Git identity must be configured before committing.&lt;br&gt;&lt;br&gt;Run the following in a terminal:&lt;br&gt;&lt;code&gt;git config --global user.name "Your Name"&lt;/code&gt;&lt;br&gt;&lt;code&gt;git config --global user.email "you@example.com"&lt;/code&gt;&lt;br&gt;&lt;br&gt;Or open the Settings dialog and fill in the &lt;b&gt;Git&lt;/b&gt; page.</translation>
    </message>
    <message>
        <source>打开设置</source>
        <translation>Open Settings</translation>
    </message>
    <message>
        <source>请先暂存至少一个文件。</source>
        <translation>Please stage at least one file first.</translation>
    </message>
    <message>
        <source>已暂存: %1</source>
        <translation>Staged: %1</translation>
    </message>
    <message>
        <source>工作区: %1</source>
        <translation>Working tree: %1</translation>
    </message>
    <message>
        <source>查看差异</source>
        <translation>View Diff</translation>
    </message>
    <message>
        <source>取消暂存</source>
        <translation>Unstage</translation>
    </message>
    <message>
        <source>打开文件</source>
        <translation>Open File</translation>
    </message>
    <message>
        <source>打开编辑器手动解决</source>
        <translation>Open in editor to resolve manually</translation>
    </message>
    <message>
        <source>接受我的提交（正在重放的版本）</source>
        <translation>Accept my commit (the version being replayed)</translation>
    </message>
    <message>
        <source>Rebase 冲突时，保留你正在重放的提交的内容
底层执行：git checkout --theirs</source>
        <translation>On a rebase conflict, keep the content of the commit being replayed.
Underlying command: git checkout --theirs</translation>
    </message>
    <message>
        <source>接受目标分支（base 上的版本）</source>
        <translation>Accept target branch (the base version)</translation>
    </message>
    <message>
        <source>Rebase 冲突时，保留目标分支（base）的内容
底层执行：git checkout --ours</source>
        <translation>On a rebase conflict, keep the target branch (base) content.
Underlying command: git checkout --ours</translation>
    </message>
    <message>
        <source>接受当前分支（本地的版本）</source>
        <translation>Accept current branch (local version)</translation>
    </message>
    <message>
        <source>保留当前分支（HEAD）的内容
底层执行：git checkout --ours</source>
        <translation>Keep the content of the current branch (HEAD).
Underlying command: git checkout --ours</translation>
    </message>
    <message>
        <source>接受被合入的分支（外来的版本）</source>
        <translation>Accept the incoming branch (the other version)</translation>
    </message>
    <message>
        <source>保留被合入分支的内容
底层执行：git checkout --theirs</source>
        <translation>Keep the content of the incoming branch.
Underlying command: git checkout --theirs</translation>
    </message>
    <message>
        <source>标记为已解决</source>
        <translation>Mark as resolved</translation>
    </message>
    <message>
        <source>（git add）</source>
        <translation> (git add)</translation>
    </message>
    <message>
        <source>暂存此文件</source>
        <translation>Stage this file</translation>
    </message>
    <message>
        <source>丢弃更改…</source>
        <translation>Discard Changes...</translation>
    </message>
    <message>
        <source>丢弃更改</source>
        <translation>Discard Changes</translation>
    </message>
    <message>
        <source>冲突文件不能直接丢弃，请先用「接受本地 / 接受远程 / 手动解决」处理。</source>
        <translation>Conflicted files cannot be discarded directly. Use "Accept local / Accept remote / Resolve manually" first.</translation>
    </message>
    <message>
        <source>删除文件</source>
        <translation>Delete File</translation>
    </message>
    <message>
        <source>确定要删除以下未跟踪文件吗？&lt;br&gt;&lt;br&gt;&lt;code&gt;%1&lt;/code&gt;&lt;br&gt;&lt;br&gt;此操作不可恢复。</source>
        <translation>Delete the following untracked file?&lt;br&gt;&lt;br&gt;&lt;code&gt;%1&lt;/code&gt;&lt;br&gt;&lt;br&gt;This action cannot be undone.</translation>
    </message>
    <message>
        <source>确定要丢弃对该文件的所有未提交更改吗？&lt;br&gt;&lt;br&gt;&lt;code&gt;%1&lt;/code&gt;&lt;br&gt;&lt;br&gt;此操作不可恢复。</source>
        <translation>Discard all uncommitted changes to this file?&lt;br&gt;&lt;br&gt;&lt;code&gt;%1&lt;/code&gt;&lt;br&gt;&lt;br&gt;This action cannot be undone.</translation>
    </message>
    <message>
        <source>删除</source>
        <translation>Delete</translation>
    </message>
    <message>
        <source>无法获取该 commit 的 diff。</source>
        <translation>Cannot retrieve the diff for this commit.</translation>
    </message>
    <message>
        <source>创建 Stash</source>
        <translation>Create Stash</translation>
    </message>
    <message>
        <source>Stash 消息（可留空，默认 WIP）:</source>
        <translation>Stash message (leave blank for default "WIP"):</translation>
    </message>
    <message>
        <source>Stash 操作</source>
        <translation>Stash Action</translation>
    </message>
    <message>
        <source>对 %1 执行什么操作？</source>
        <translation>What do you want to do with %1?</translation>
    </message>
    <message>
        <source>应用并删除 (pop)</source>
        <translation>Apply and drop (pop)</translation>
    </message>
    <message>
        <source>仅应用 (apply)</source>
        <translation>Apply only (apply)</translation>
    </message>
    <message>
        <source>清空所有 Stash</source>
        <translation>Clear All Stashes</translation>
    </message>
    <message>
        <source>清空 Stash</source>
        <translation>Clear Stashes</translation>
    </message>
    <message>
        <source>确定要清空所有 stash 吗？此操作不可恢复。</source>
        <translation>Clear all stashes? This cannot be undone.</translation>
    </message>
    <message>
        <source>删除此 Stash</source>
        <translation>Delete This Stash</translation>
    </message>
    <message>
        <source>删除 Stash</source>
        <translation>Delete Stash</translation>
    </message>
    <message>
        <source>确定要删除此 stash 吗？此操作不可恢复。</source>
        <translation>Delete this stash? This cannot be undone.</translation>
    </message>
</context>
<context>
    <name>SprintIDE::ProjectSelectionDialog</name>
    <message>
        <source>选择或创建项目</source>
        <translation>Select or Create a Project</translation>
    </message>
    <message>
        <source>请选择要打开的项目，或创建新项目</source>
        <translation>Choose a project to open, or create a new one</translation>
    </message>
    <message>
        <source>最近使用的项目：</source>
        <translation>Recent projects:</translation>
    </message>
    <message>
        <source>打开现有项目</source>
        <translation>Open Existing Project</translation>
    </message>
    <message>
        <source>创建新项目</source>
        <translation>Create New Project</translation>
    </message>
    <message>
        <source>选择项目目录</source>
        <translation>Select Project Directory</translation>
    </message>
    <message>
        <source>打开现有项目...</source>
        <translation>Open Existing Project...</translation>
    </message>
    <message>
        <source>创建新项目...</source>
        <translation>Create New Project...</translation>
    </message>
    <message>
        <source>打开项目</source>
        <translation>Open Project</translation>
    </message>
    <message>
        <source>从历史记录中移除</source>
        <translation>Remove from History</translation>
    </message>
    <message>
        <source>打开其他项目...</source>
        <translation>Open Another Project...</translation>
    </message>
    <message>
        <source>确定要把以下项目从历史记录中移除吗？

%1

· 只移除历史记录
· 磁盘上的项目文件不会被删除</source>
        <translation>Remove the following project from history?

%1

· Only the history entry is removed
· Project files on disk are not deleted</translation>
    </message>
    <message>
        <source>没有最近使用的项目</source>
        <translation>No recent projects</translation>
    </message>
    <message>
        <source>项目路径: %1
上次打开: %2

右键可移除历史记录</source>
        <translation>Project path: %1
Last opened: %2

Right-click to remove from history</translation>
    </message>
    <message>
        <source>项目名称: &lt;b&gt;%1&lt;/b&gt;&lt;br&gt;项目路径: &lt;b&gt;%2&lt;/b&gt;&lt;br&gt;配置文件大小: &lt;b&gt;%3&lt;/b&gt;&lt;br&gt;上次修改时间: &lt;b&gt;%4&lt;/b&gt;</source>
        <translation>Project name: &lt;b&gt;%1&lt;/b&gt;&lt;br&gt;Project path: &lt;b&gt;%2&lt;/b&gt;&lt;br&gt;Config file size: &lt;b&gt;%3&lt;/b&gt;&lt;br&gt;Last modified: &lt;b&gt;%4&lt;/b&gt;</translation>
    </message>
    <message>
        <source> KB</source>
        <translation> KB</translation>
    </message>
</context>
<context>
    <name>SprintIDE::LocalHistory</name>
    <message>
        <source>自动保存</source>
        <translation>Auto Save</translation>
    </message>
    <message>
        <source> B</source>
        <translation> B</translation>
    </message>
    <message>
        <source> KB</source>
        <translation> KB</translation>
    </message>
    <message>
        <source> MB</source>
        <translation> MB</translation>
    </message>
</context>
<context>
    <name>SprintIDE::ProcessManager</name>
    <message>
        <source>命令不可用或不存在: %1</source>
        <translation>Command unavailable or not found: %1</translation>
    </message>
    <message>
        <source>无法启动进程: %1</source>
        <translation>Cannot start process: %1</translation>
    </message>
    <message>
        <source>进程执行超时 (%1 毫秒)</source>
        <translation>Process timed out (%1 ms)</translation>
    </message>
    <message>
        <source>进程被中断</source>
        <translation>Process interrupted</translation>
    </message>
    <message>
        <source>进程退出码: %1</source>
        <translation>Process exit code: %1</translation>
    </message>
    <message>
        <source>进程不存在</source>
        <translation>Process does not exist</translation>
    </message>
    <message>
        <source>进程启动失败</source>
        <translation>Process failed to start</translation>
    </message>
    <message>
        <source>进程崩溃</source>
        <translation>Process crashed</translation>
    </message>
    <message>
        <source>进程超时</source>
        <translation>Process timed out</translation>
    </message>
    <message>
        <source>读取错误</source>
        <translation>Read error</translation>
    </message>
    <message>
        <source>写入错误</source>
        <translation>Write error</translation>
    </message>
    <message>
        <source>未知进程错误</source>
        <translation>Unknown process error</translation>
    </message>
    <message>
        <source>进程执行超时</source>
        <translation>Process timed out</translation>
    </message>
</context>
<context>
    <name>SprintIDE::CompletionPopup</name>
</context>
<context>
    <name>SprintIDE::TextEditor</name>
    <message>
        <source>第 %1 行的断点条件（空 = 无条件）:</source>
        <translation>Breakpoint condition at line %1 (empty = none):</translation>
    </message>
    <message>
        <source>编辑断点条件...</source>
        <translation>Edit Breakpoint Condition...</translation>
    </message>
    <message>
        <source>移除断点</source>
        <translation>Remove Breakpoint</translation>
    </message>
    <message>
        <source>断点条件</source>
        <translation>Breakpoint Condition</translation>
    </message>
    <message>
        <source>查找:</source>
        <translation>Find:</translation>
    </message>
    <message>
        <source>输入查找内容...</source>
        <translation>Enter search text...</translation>
    </message>
    <message>
        <source>大小写敏感</source>
        <translation>Case sensitive</translation>
    </message>
    <message>
        <source>全字匹配</source>
        <translation>Whole word</translation>
    </message>
    <message>
        <source>上一个 (Shift+F3)</source>
        <translation>Previous (Shift+F3)</translation>
    </message>
    <message>
        <source>下一个 (F3)</source>
        <translation>Next (F3)</translation>
    </message>
    <message>
        <source>显示替换</source>
        <translation>Show Replace</translation>
    </message>
    <message>
        <source>替换为...</source>
        <translation>Replace with...</translation>
    </message>
    <message>
        <source>替换</source>
        <translation>Replace</translation>
    </message>
    <message>
        <source>全部替换</source>
        <translation>Replace All</translation>
    </message>
    <message>
        <source>关闭 (Esc)</source>
        <translation>Close (Esc)</translation>
    </message>
    <message>
        <source>未命名</source>
        <translation>Untitled</translation>
    </message>
    <message>
        <source>文件打开失败</source>
        <translation>Open Failed</translation>
    </message>
    <message>
        <source>无法打开文件：%1
错误：%2</source>
        <translation>Cannot open file: %1
Error: %2</translation>
    </message>
    <message>
        <source>保存文件</source>
        <translation>Save File</translation>
    </message>
    <message>
        <source>C++文件 (*.cpp *.h *.hpp);;所有文件 (*.*)</source>
        <translation>C++ Files (*.cpp *.h *.hpp);;All Files (*.*)</translation>
    </message>
    <message>
        <source>文件保存失败</source>
        <translation>Save Failed</translation>
    </message>
    <message>
        <source>无法保存文件：%1
错误：%2</source>
        <translation>Cannot save file: %1
Error: %2</translation>
    </message>
    <message>
        <source>撤销 (Ctrl+Z)</source>
        <translation>Undo (Ctrl+Z)</translation>
    </message>
    <message>
        <source>重做 (Ctrl+Y)</source>
        <translation>Redo (Ctrl+Y)</translation>
    </message>
    <message>
        <source>剪切 (Ctrl+X)</source>
        <translation>Cut (Ctrl+X)</translation>
    </message>
    <message>
        <source>复制 (Ctrl+C)</source>
        <translation>Copy (Ctrl+C)</translation>
    </message>
    <message>
        <source>粘贴 (Ctrl+V)</source>
        <translation>Paste (Ctrl+V)</translation>
    </message>
    <message>
        <source>全选 (Ctrl+A)</source>
        <translation>Select All (Ctrl+A)</translation>
    </message>
    <message>
        <source>注释选中行 (Ctrl+/)</source>
        <translation>Comment Selection (Ctrl+/)</translation>
    </message>
    <message>
        <source>取消注释 (Ctrl+/)</source>
        <translation>Uncomment (Ctrl+/)</translation>
    </message>
    <message>
        <source>复制当前行 (Ctrl+D)</source>
        <translation>Duplicate Line (Ctrl+D)</translation>
    </message>
    <message>
        <source>删除当前行 (Ctrl+Y)</source>
        <translation>Delete Line (Ctrl+Y)</translation>
    </message>
    <message>
        <source>跳转到行 (Ctrl+G)...</source>
        <translation>Go to Line (Ctrl+G)...</translation>
    </message>
    <message>
        <source>跳转到行</source>
        <translation>Go to Line</translation>
    </message>
    <message>
        <source>行号:</source>
        <translation>Line number:</translation>
    </message>
    <message>
        <source>编辑断点条件...</source>
        <translation>Edit Breakpoint Condition...</translation>
    </message>
    <message>
        <source>移除断点</source>
        <translation>Remove Breakpoint</translation>
    </message>
    <message>
        <source>断点条件</source>
        <translation>Breakpoint Condition</translation>
    </message>
    <message>
        <source>第 %1 行的断点条件（空 = 无条件）:</source>
        <translation>Breakpoint condition at line %1 (empty = none):</translation>
    </message>
</context>
<context>
    <name>SprintIDE::ProjectCreationDialog</name>
    <message>
        <source>创建新项目 - SprintIDE</source>
        <translation>Create New Project - SprintIDE</translation>
    </message>
    <message>
        <source>编程语言:</source>
        <translation>Programming Language:</translation>
    </message>
    <message>
        <source>语言选项</source>
        <translation>Language Options</translation>
    </message>
    <message>
        <source>创建虚拟环境 (.venv)</source>
        <translation>Create virtual environment (.venv)</translation>
    </message>
    <message>
        <source>留空则自动探测 python3 / py</source>
        <translation>Leave blank to auto-detect python3 / py</translation>
    </message>
    <message>
        <source>例如 example.com/myapp</source>
        <translation>e.g. example.com/myapp</translation>
    </message>
    <message>
        <source>可执行程序 (bin)</source>
        <translation>Binary (bin)</translation>
    </message>
    <message>
        <source>库 (lib)</source>
        <translation>Library (lib)</translation>
    </message>
    <message>
        <source>入口文件:</source>
        <translation>Entry file:</translation>
    </message>
    <message>
        <source>解释器路径:</source>
        <translation>Interpreter path:</translation>
    </message>
    <message>
        <source>Module 路径:</source>
        <translation>Module path:</translation>
    </message>
    <message>
        <source>目标类型:</source>
        <translation>Target type:</translation>
    </message>
    <message>
        <source>语言变体:</source>
        <translation>Language variant:</translation>
    </message>
    <message>
        <source>包管理器:</source>
        <translation>Package manager:</translation>
    </message>
    <message>
        <source>Build 命令:</source>
        <translation>Build command:</translation>
    </message>
    <message>
        <source>Run 命令:</source>
        <translation>Run command:</translation>
    </message>
    <message>
        <source>Clean 命令:</source>
        <translation>Clean command:</translation>
    </message>
    <message>
        <source>例如 make -j4</source>
        <translation>e.g. make -j4</translation>
    </message>
    <message>
        <source>例如 ./my_app</source>
        <translation>e.g. ./my_app</translation>
    </message>
    <message>
        <source>例如 rm -rf build/</source>
        <translation>e.g. rm -rf build/</translation>
    </message>
    <message>
        <source>例如: MyFirstGame</source>
        <translation>e.g. MyFirstGame</translation>
    </message>
    <message>
        <source>例如: 我的第一个游戏</source>
        <translation>e.g. My First Game</translation>
    </message>
    <message>
        <source>选择项目保存的位置</source>
        <translation>Choose where to save the project</translation>
    </message>
    <message>
        <source>自动检测</source>
        <translation>Auto Detect</translation>
    </message>
    <message>
        <source>创建</source>
        <translation>Create</translation>
    </message>
    <message>
        <source>取消</source>
        <translation>Cancel</translation>
    </message>
    <message>
        <source>请修正以下错误：

</source>
        <translation>Please fix the following errors:

</translation>
    </message>
    <message>
        <source>输入错误</source>
        <translation>Input Error</translation>
    </message>
    <message>
        <source>无法创建项目目录结构</source>
        <translation>Cannot create project directory structure</translation>
    </message>
    <message>
        <source>创建失败</source>
        <translation>Create Failed</translation>
    </message>
    <message>
        <source>无法生成项目文件</source>
        <translation>Cannot generate project files</translation>
    </message>
    <message>
        <source>选择项目位置</source>
        <translation>Choose Project Location</translation>
    </message>
    <message>
        <source>项目名称不能为空</source>
        <translation>Project name cannot be empty</translation>
    </message>
    <message>
        <source>项目名称只能包含字母、数字和下划线，且不能以数字开头</source>
        <translation>Project name may contain only letters, digits and underscores, and cannot start with a digit</translation>
    </message>
    <message>
        <source>项目名称是系统保留名称，请选择其他名称</source>
        <translation>Project name is a reserved system name. Please choose another.</translation>
    </message>
    <message>
        <source>显示名称不能为空</source>
        <translation>Display name cannot be empty</translation>
    </message>
    <message>
        <source>项目路径不能为空</source>
        <translation>Project path cannot be empty</translation>
    </message>
    <message>
        <source>项目路径必须是绝对路径</source>
        <translation>Project path must be an absolute path</translation>
    </message>
    <message>
        <source>路径包含非法字符</source>
        <translation>Path contains invalid characters</translation>
    </message>
    <message>
        <source>无法创建目录，请检查权限</source>
        <translation>Cannot create directory. Check permissions.</translation>
    </message>
    <message>
        <source>目录不可写，请检查权限</source>
        <translation>Directory is not writable. Check permissions.</translation>
    </message>
    <message>
        <source>项目目录已存在且不为空</source>
        <translation>Project directory already exists and is not empty</translation>
    </message>
    <message>
        <source>项目名称: </source>
        <translation>Project name: </translation>
    </message>
    <message>
        <source>项目路径: </source>
        <translation>Project path: </translation>
    </message>
    <message>
        <source>无法创建项目目录: </source>
        <translation>Cannot create project directory: </translation>
    </message>
    <message>
        <source>无法创建目录: </source>
        <translation>Cannot create directory: </translation>
    </message>
    <message>
        <source>无法生成配置文件:

</source>
        <translation>Cannot generate config file:

</translation>
    </message>
    <message>
        <source>无法创建 main.py</source>
        <translation>Cannot create main.py</translation>
    </message>
    <message>
        <source>无法创建 main.go</source>
        <translation>Cannot create main.go</translation>
    </message>
    <message>
        <source>无法创建 go.mod</source>
        <translation>Cannot create go.mod</translation>
    </message>
    <message>
        <source>无法创建 Cargo.toml</source>
        <translation>Cannot create Cargo.toml</translation>
    </message>
    <message>
        <source>无法创建 src 目录</source>
        <translation>Cannot create src directory</translation>
    </message>
    <message>
        <source>无法创建 package.json</source>
        <translation>Cannot create package.json</translation>
    </message>
    <message>
        <source>无法创建源码目录</source>
        <translation>Cannot create source directory</translation>
    </message>
    <message>
        <source>无法创建入口文件</source>
        <translation>Cannot create entry file</translation>
    </message>
    <message>
        <source>无法创建 tsconfig.json</source>
        <translation>Cannot create tsconfig.json</translation>
    </message>
    <message>
        <source>无法写入配置文件: </source>
        <translation>Cannot write config file: </translation>
    </message>
    <message>
        <source>高级配置</source>
        <translation>Advanced Configuration</translation>
    </message>
    <message>
        <source>高级编译配置功能正在开发中...
未来版本将允许您设置：
- 自定义包含路径
- 库链接设置
- 预处理器定义
- 自定义编译标志</source>
        <translation>Advanced build configuration is under development.
Future versions will let you set:
- Custom include paths
- Library linking
- Preprocessor definitions
- Custom compiler flags</translation>
    </message>
    <message>
        <source>示例代码预览 - </source>
        <translation>Example Code Preview - </translation>
    </message>
    <message>
        <source>（暂无示例代码）</source>
        <translation>(No example code yet)</translation>
    </message>
    <message>
        <source>请填写项目名称和路径</source>
        <translation>Please fill in the project name and path</translation>
    </message>
    <message>
        <source>(目录已存在，为空)</source>
        <translation>(Directory exists and is empty)</translation>
    </message>
    <message>
        <source>(目录已存在，不为空)</source>
        <translation>(Directory exists and is not empty)</translation>
    </message>
    <message>
        <source>(将创建新目录)</source>
        <translation>(New directory will be created)</translation>
    </message>
    <message>
        <source>将创建以下文件:

</source>
        <translation>The following files will be created:

</translation>
    </message>
    <message>
        <source>

目录结构:

</source>
        <translation>

Directory structure:

</translation>
    </message>
    <message>
        <source>控制台应用程序</source>
        <translation>Console Application</translation>
    </message>
    <message>
        <source>创建一个简单的命令行程序，适合学习基础 C++ 语法。
包含基本的 main 函数结构和标准库使用示例。</source>
        <translation>Create a simple command-line program, great for learning basic C++ syntax.
Includes a basic main function structure and standard library examples.</translation>
    </message>
    <message>
        <source>简单游戏</source>
        <translation>Simple Game</translation>
    </message>
    <message>
        <source>创建一个有趣的小游戏项目，如猜数字、井字棋等。
增加编程的趣味性，适合有一定基础的学习者。</source>
        <translation>Create a fun mini-game project, such as number guessing or tic-tac-toe.
Adds fun to programming; suitable for learners with some basics.</translation>
    </message>
    <message>
        <source>图形界面程序</source>
        <translation>GUI Application</translation>
    </message>
    <message>
        <source>创建一个带窗口界面的应用程序，使用 Qt 框架。
学习图形界面编程，创建更直观的用户界面。</source>
        <translation>Create a windowed GUI application using the Qt framework.
Learn GUI programming and build more intuitive user interfaces.</translation>
    </message>
    <message>
        <source>空白项目</source>
        <translation>Empty Project</translation>
    </message>
    <message>
        <source>创建一个完全自定义的项目结构，没有预置代码。
适合有经验的学习者，或需要特殊结构的项目。</source>
        <translation>Create a fully custom project structure with no preset code.
Suitable for experienced learners or projects with special structure requirements.</translation>
    </message>
    <message>
        <source>Python 空白项目</source>
        <translation>Python Blank Project</translation>
    </message>
    <message>
        <source>创建一个空白 Python 项目。
包含 main.py、requirements.txt 和可选的虚拟环境 (.venv)。</source>
        <translation>Create a blank Python project.
Includes main.py, requirements.txt and an optional virtual environment (.venv).</translation>
    </message>
    <message>
        <source>Go 空白项目</source>
        <translation>Go Blank Project</translation>
    </message>
    <message>
        <source>创建一个 Go 模块项目。
包含 main.go 和 go.mod，可直接用 go build 构建。</source>
        <translation>Create a Go module project.
Includes main.go and go.mod; buildable with go build directly.</translation>
    </message>
    <message>
        <source>Rust 项目</source>
        <translation>Rust Project</translation>
    </message>
    <message>
        <source>创建一个 Rust 项目（可执行或库）。
包含 Cargo.toml 与 src/ 目录，可直接用 cargo build 构建。</source>
        <translation>Create a Rust project (binary or library).
Includes Cargo.toml and src/; buildable with cargo build directly.</translation>
    </message>
    <message>
        <source>Node.js 项目 (JavaScript)</source>
        <translation>Node.js Project (JavaScript)</translation>
    </message>
    <message>
        <source>创建一个 Node.js 项目，使用 JavaScript。
包含 package.json 与 index.js。</source>
        <translation>Create a Node.js project using JavaScript.
Includes package.json and index.js.</translation>
    </message>
    <message>
        <source>Node.js 项目 (TypeScript)</source>
        <translation>Node.js Project (TypeScript)</translation>
    </message>
    <message>
        <source>创建一个 Node.js 项目，使用 TypeScript。
包含 package.json、tsconfig.json 与 src/index.ts。</source>
        <translation>Create a Node.js project using TypeScript.
Includes package.json, tsconfig.json and src/index.ts.</translation>
    </message>
    <message>
        <source>AI 通用适配器项目</source>
        <translation>AI Generic Adapter Project</translation>
    </message>
    <message>
        <source>适用于任意语言 / 工具链的空项目。
可在下方指定 build / run / clean 命令，或让 AI 自动补全。
命令会被写入 .lightcppproj 文件。</source>
        <translation>An empty project for any language / toolchain.
You can specify build / run / clean commands below, or let AI complete them.
Commands are written to the .lightcppproj file.</translation>
    </message>
    <message>
        <source>AI 通用适配器</source>
        <translation>AI Generic Adapter</translation>
    </message>
    <message>
        <source>未知</source>
        <translation>Unknown</translation>
    </message>
    <message>
        <source>无法写入文件: </source>
        <translation>Cannot write file: </translation>
    </message>
    <message>
        <source>无法写入配置文件: </source>
        <translation>Cannot write config file: </translation>
    </message>
    <message>
        <source>项目文件生成失败</source>
        <translation>Failed to generate project files</translation>
    </message>
    <message>
        <source>Python 空白项目</source>
        <translation>Python Blank Project</translation>
    </message>
    <message>
        <source>创建一个空白 Python 项目。
    包含 main.py、requirements.txt 和可选的虚拟环境 (.venv)。</source>
        <translation>Create a blank Python project.
    Includes main.py, requirements.txt and an optional virtual environment (.venv).</translation>
    </message>
    <message>
        <source>Go 空白项目</source>
        <translation>Go Blank Project</translation>
    </message>
    <message>
        <source>创建一个 Go 模块项目。
    包含 main.go 和 go.mod，可直接用 go build 构建。</source>
        <translation>Create a Go module project.
    Includes main.go and go.mod; buildable with go build directly.</translation>
    </message>
    <message>
        <source>Rust 项目</source>
        <translation>Rust Project</translation>
    </message>
    <message>
        <source>创建一个 Rust 项目（可执行或库）。
    包含 Cargo.toml 与 src/ 目录，可直接用 cargo build 构建。</source>
        <translation>Create a Rust project (binary or library).
    Includes Cargo.toml and src/; buildable with cargo build directly.</translation>
    </message>
    <message>
        <source>Node.js 项目 (JavaScript)</source>
        <translation>Node.js Project (JavaScript)</translation>
    </message>
    <message>
        <source>创建一个 Node.js 项目，使用 JavaScript。
    包含 package.json 与 index.js。</source>
        <translation>Create a Node.js project using JavaScript.
    Includes package.json and index.js.</translation>
    </message>
    <message>
        <source>Node.js 项目 (TypeScript)</source>
        <translation>Node.js Project (TypeScript)</translation>
    </message>
    <message>
        <source>创建一个 Node.js 项目，使用 TypeScript。
    包含 package.json、tsconfig.json 与 src/index.ts。</source>
        <translation>Create a Node.js project using TypeScript.
    Includes package.json, tsconfig.json and src/index.ts.</translation>
    </message>
    <message>
        <source>AI 通用适配器项目</source>
        <translation>AI Generic Adapter Project</translation>
        </message>
    <message>
        <source>适用于任意语言 / 工具链的空项目。
    可在下方指定 build / run / clean 命令，或让 AI 自动补全。
    命令会被写入 .lightcppproj 文件。</source>
        <translation>An empty project for any language / toolchain.
    You can specify build / run / clean commands below, or let AI complete them.
    Commands are written to the .lightcppproj file.</translation>
    </message>
    <message>
        <source>AI 通用适配器</source>
        <translation>AI Generic Adapter</translation>
    </message>
    <message>
        <source>未知</source>
        <translation>Unknown</translation>
    </message>
    <message>
        <source>语言选项</source>
        <translation>Language Options</translation>
    </message>
    <message>
        <source>创建虚拟环境 (.venv)</source>
        <translation>Create virtual environment (.venv)</translation>
    </message>
    <message>
        <source>留空则自动探测 python3 / py</source>
        <translation>Leave blank to auto-detect python3 / py</translation>
    </message>
    <message>
        <source>例如 example.com/myapp</source>
        <translation>e.g. example.com/myapp</translation>
    </message>
    <message>
        <source>可执行程序 (bin)</source>
        <translation>Binary (bin)</translation>
    </message>
    <message>
        <source>库 (lib)</source>
        <translation>Library (lib)</translation>
    </message>
    <message>
        <source>入口文件:</source>
        <translation>Entry file:</translation>
    </message>
    <message>
        <source>解释器路径:</source>
        <translation>Interpreter path:</translation>
    </message>
    <message>
        <source>Module 路径:</source>
        <translation>Module path:</translation>
    </message>
    <message>
        <source>目标类型:</source>
        <translation>Target type:</translation>
    </message>
    <message>
        <source>语言变体:</source>
        <translation>Language variant:</translation>
    </message>
    <message>
        <source>包管理器:</source>
        <translation>Package manager:</translation>
    </message>
    <message>
        <source>Build 命令:</source>
        <translation>Build command:</translation>
    </message>
    <message>
        <source>Run 命令:</source>
        <translation>Run command:</translation>
    </message>
    <message>
        <source>Clean 命令:</source>
        <translation>Clean command:</translation>
    </message>
    <message>
        <source>例如 make -j4</source>
        <translation>e.g. make -j4</translation>
    </message>
    <message>
        <source>例如 ./my_app</source>
        <translation>e.g. ./my_app</translation>
    </message>
    <message>
        <source>例如 rm -rf build/</source>
        <translation>e.g. rm -rf build/</translation>
    </message>
    <message>
        <source>编程语言:</source>
        <translation>Programming Language:</translation>
        </message>
    <message>
        <source>无法创建 main.py</source>
        <translation>Cannot create main.py</translation>
    </message>
    <message>
        <source>无法创建 main.go</source>
        <translation>Cannot create main.go</translation>
    </message>
    <message>
        <source>无法创建 go.mod</source>
        <translation>Cannot create go.mod</translation>
    </message>
    <message>
        <source>无法创建 Cargo.toml</source>
        <translation>Cannot create Cargo.toml</translation>
    </message>
    <message>
        <source>无法创建 src 目录</source>
        <translation>Cannot create src directory</translation>
    </message>
    <message>
        <source>无法创建 package.json</source>
        <translation>Cannot create package.json</translation>
    </message>
    <message>
        <source>无法创建源码目录</source>
        <translation>Cannot create source directory</translation>
    </message>
    <message>
        <source>无法创建入口文件</source>
        <translation>Cannot create entry file</translation>
    </message>
    <message>
        <source>无法创建 tsconfig.json</source>
        <translation>Cannot create tsconfig.json</translation>
    </message>
    <message>
        <source>无法写入配置文件: </source>
        <translation>Cannot write config file: </translation>
    </message>
    <message>
        <source>由 SprintIDE 的 AI 通用适配器创建。

    请在 `.lightcppproj` 的 `generic_build` 字段中配置命令。
    </source>
        <translation>Created by SprintIDE's AI Generic Adapter.

    Please configure commands in the `generic_build` field of `.lightcppproj`.
    </translation>
    </message>
</context>

</TS>
