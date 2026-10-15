#include "application.h"
#include "mainwindow.h"
#include "project_manager.h"
#include "project_selection_dialog.h"
#include "deps/toolchain_manager.h"
#include "core/project_scaffolder.h"

// 需要显式清理的静态单例
#include "debug_manager.h"
#include "builder/builder_factory.h"
#include "font_manager.h"
#include "git/git_manager.h"

#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <QFile>
#include <QIcon>
#include <QDir>
#include <memory>   // std::make_unique
#include <QStandardPaths>
#include <QProcess>

// 使用命名空间SprintIDE
using namespace SprintIDE;

// ============================================================================
//  采纳用户登录 + 交互 shell 的环境变量
// ----------------------------------------------------------------------------
//  为什么必须用 -lic 而不是 -lc：
//    • 登录模式（-l）：读 /etc/zprofile → ~/.zprofile → ~/.zlogin
//                      → Homebrew 注入路径在这里（Apple Silicon 的 /opt/homebrew/bin）
//    • 交互模式（-i）：读 /etc/zshrc → ~/.zshrc
//                      → nvm / fnm / volta / pyenv / asdf 的加载代码都在这里
//    • 只用 -l 会漏 .zshrc；只用 -i 会漏 .zprofile。必须同时用。
//  另外：
//    • 超时 5 秒（用户的 .zshrc 里可能有 conda / rvm 等重量级初始化）
//    • 只采纳白名单变量，避免污染 IDE 自身环境
//    • 逐行解析 KEY=VALUE，对非环境变量噪音（启动脚本 echo 出来的欢迎语）自动过滤
// ============================================================================
static void adoptLoginShellEnvironment()
{
    QString shell = qEnvironmentVariable("SHELL");
    if (shell.isEmpty()) {
#ifdef Q_OS_WIN
        return;   // Windows 没这个需求
#elif defined(Q_OS_MAC)
        shell = "/bin/zsh";
#else
        shell = "/bin/bash";
#endif
    }

    // 只对 zsh / bash 使用 -lic；其它 shell（fish 等）走 -c
    const QString shellBase = QFileInfo(shell).fileName();
    QStringList args;
    if (shellBase == "zsh" || shellBase == "bash") {
        args << "-lic" << "env";
    } else {
        args << "-c" << "env";
    }

    QProcess proc;
    proc.setProcessChannelMode(QProcess::SeparateChannels);
    proc.start(shell, args);

    if (!proc.waitForStarted(2000)) {
        qWarning() << "[main] 无法启动登录 shell:" << shell;
        return;
    }
    if (!proc.waitForFinished(5000)) {
        qWarning() << "[main] 登录 shell 超时（.zshrc 里有慢初始化？），跳过环境采纳";
        proc.kill();
        proc.waitForFinished(500);
        return;
    }

    const QByteArray raw = proc.readAllStandardOutput();
    if (raw.isEmpty()) {
        qWarning() << "[main] 登录 shell 未返回环境变量";
        return;
    }

    // 只采纳这些变量；其余一律不动，防止污染 IDE 自身
    static const char* kAdoptKeys[] = {
        // 通用
        "PATH",
        // Node 版本管理器
        "NVM_DIR", "NVM_BIN", "NODE_PATH",
        "FNM_DIR", "FNM_NODE_DIST_MIRROR",
        "VOLTA_HOME",
        // Python 版本管理器
        "PYENV_ROOT",
        // Go
        "GOPATH", "GOROOT",
        // Rust
        "CARGO_HOME", "RUSTUP_HOME",
        // Java
        "JAVA_HOME",
        nullptr
    };

    int adopted = 0;
    for (const QByteArray& line : raw.split('\n')) {
        if (line.isEmpty()) continue;

        const int eq = line.indexOf('=');
        if (eq <= 0 || eq > 64) continue;   // KEY 太长，不是环境变量

        QByteArray key = line.left(eq);
        if (key.endsWith('\r')) key.chop(1);   // CRLF 处理

        // 严格校验 KEY 格式：仅 A-Z / 0-9 / _
        bool validKey = true;
        for (char c : key) {
            if (!((c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') ||
                  c == '_')) {
                validKey = false;
                break;
            }
        }
        if (!validKey) continue;

        // 白名单匹配
        bool wanted = false;
        for (const char** k = kAdoptKeys; *k; ++k) {
            if (key == *k) { wanted = true; break; }
        }
        if (!wanted) continue;

        QByteArray val = line.mid(eq + 1);
        if (val.endsWith('\r')) val.chop(1);

        qputenv(key.constData(), val);
        ++adopted;
    }

    qDebug() << "[main] 已采纳登录 shell 环境，变量数:" << adopted;
    qDebug() << "[main] PATH =" << qgetenv("PATH");
}

/**
 * @brief 应用程序主入口
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return 应用程序退出代码
 */
int main(int argc, char *argv[])
{
    adoptLoginShellEnvironment();
    // 创建QApplication实例
    QApplication app(argc, argv);

    // 设置应用程序的基本信息
    app.setApplicationName("SprintIDE");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("SprintIDE");
    app.setOrganizationDomain("sprintide.org");

    // 设置应用程序图标
    app.setWindowIcon(QIcon(":/icons/icons/app_logo.ico"));

    // 设置应用程序字体
    QFont uiFont;
#if defined(Q_OS_WIN)
    uiFont.setFamily("Microsoft YaHei UI");
#elif defined(Q_OS_MAC)
    uiFont.setFamily("Helvetica Neue");         // macOS 系统 sans 字体
#else
    uiFont.setFamily("Noto Sans CJK SC");       // Linux 中文字体
#endif
    uiFont.setPointSize(10);
    uiFont.setStyleHint(QFont::SansSerif);
    app.setFont(uiFont);

    // 创建必要的目录结构
    // QDir().mkdir("config");
    // QDir().mkdir("projects");
    // QDir().mkdir("temp");

    try {
        auto& sprintApp = SprintIDE::Application::instance();

        if (!sprintApp.initialize(argc, argv)) {
            QMessageBox::critical(nullptr, "初始化错误",
                                  "无法初始化SprintIDE核心组件。\n"
                                  "请检查：\n"
                                  "1. 是否安装了必要的Qt库\n"
                                  "2. 是否有足够的权限\n"
                                  "3. 磁盘空间是否充足");
            return -1;
        }

        ProjectManager* projectManager = ProjectManager::instance();

                // ============================================================
        // ★ 解析命令行参数
        //   支持的形式：
        //     <path>                           → 打开/创建空项目
        //     <path> --create <language>       → 创建项目骨架并打开
        //     --agent-mode                     → 跳过对话框，进空主窗口
        //     --create <language> <path>       → 等价于 <path> --create <language>
        // ============================================================
        QString projectPath;
        QString createLanguage;      // 非空 = 需要生成骨架
        bool    createVenv = false;
        bool    agentMode  = false;
        QSet<int> consumedIndices;

        if (argc > 1) {
            for (int i = 1; i < argc; ++i) {
                const QString arg = QString::fromLocal8Bit(argv[i]);

                if (arg == QStringLiteral("--agent-mode")) {
                    agentMode = true;
                    consumedIndices.insert(i);
                }
                else if (arg == QStringLiteral("--create")) {
                    consumedIndices.insert(i);
                    if (i + 1 < argc) {
                        createLanguage = QString::fromLocal8Bit(argv[++i]).toLower();
                        consumedIndices.insert(i);   // ← 消费 lang 参数
                    }
                }
                else if (arg == QStringLiteral("--venv")) {
                    createVenv = true;
                    consumedIndices.insert(i);
                }
                else if (!arg.startsWith(QLatin1Char('-'))) {
                    if (projectPath.isEmpty()) {
                        projectPath = arg;
                        consumedIndices.insert(i);   // ← 消费路径参数
                    }
                }
            }
        }

        // ---- 组合：如果给了 --create 但没给路径，报错 ----
        if (!createLanguage.isEmpty() && projectPath.isEmpty()) {
            qWarning() << "[main] --create 需要同时指定项目路径";
            createLanguage.clear();
        }

        // ============================================================
        // 处理
        // ============================================================
        if (agentMode) {
            qDebug() << "[main] --agent-mode：跳过项目选择对话框";
        }
        else if (projectPath.isEmpty()) {
            // ---- 无参数：原有交互式选择对话框 ----
            while (true) {
                ProjectSelectionDialog selDlg(projectManager->recentProjects());

                if (selDlg.exec() != QDialog::Accepted) {
                    qDebug() << "[main] 用户取消，退出";
                    sprintApp.shutdown();
                    return 0;
                }

                if (!selDlg.isCreateNewProject()) {
                    projectPath = selDlg.selectedProjectPath();
                    if (!projectPath.isEmpty()) {
                        projectManager->openProject(projectPath);
                        break;
                    }
                    continue;
                }

                ProjectCreationDialog createDlg;
                createDlg.setDefaultProjectPath(
                    QStandardPaths::writableLocation(
                        QStandardPaths::DocumentsLocation));

                if (createDlg.exec() != QDialog::Accepted) {
                    continue;
                }

                ProjectCreationResult result = createDlg.getResult();
                if (result.success && !result.projectPath.isEmpty()) {
                    projectPath = result.projectPath;
                    projectManager->openProject(projectPath);
                    break;
                }
            }
        }
        else {
            // ---- 有路径参数：自动创建/打开 ----
            const QDir projDir(projectPath);

            // 1. 路径不存在 → 创建目录
            if (!projDir.exists()) {
                if (!QDir().mkpath(projectPath)) {
                    QMessageBox::critical(
                        nullptr, QObject::tr("启动失败"),
                        QObject::tr("无法创建目录：%1").arg(projectPath));
                    return -1;
                }
            }

            // 2. 有 --create <lang> → 生成骨架
            if (!createLanguage.isEmpty()) {
                ProjectScaffolder::CreateOptions opts;
                opts.projectPath        = QFileInfo(projectPath).absoluteFilePath();
                opts.projectName        = QFileInfo(projectPath).fileName();
                opts.displayName        = opts.projectName;
                opts.language           = createLanguage;
                opts.createVenv         = createVenv;
                opts.overwriteIfExists  = true;   // 用户显式指定路径时，覆盖也没关系

                const auto scRes = ProjectScaffolder::create(opts);
                if (!scRes.success) {
                    qWarning() << "[main] 项目创建失败:"
                               << scRes.errorCode
                               << scRes.errorMessage;
                    QMessageBox::warning(
                        nullptr, QObject::tr("创建失败"),
                        QObject::tr("无法创建项目：\n%1").arg(scRes.errorMessage));
                    // 不退出，继续打开目录
                } else {
                    qDebug() << "[main] 项目已创建:"
                             << projectPath
                             << "文件数:" << scRes.createdFiles.size();
                }
            }
            // 3. 没有 --create 且没有 .lightcpproj → 生成一个默认配置
            else {
                const QFileInfoList configs = projDir.entryInfoList(
                    {QStringLiteral("*.lightcpproj")}, QDir::Files);
                if (configs.isEmpty()) {
                    QString name = QFileInfo(projectPath).fileName();
                    if (name.isEmpty()) name = QStringLiteral("Untitled");

                    ProjectConfig config;
                    config.setProjectName(name);
                    config.setDisplayName(name);
                    config.setProjectPath(QFileInfo(projectPath).absoluteFilePath());
                    config.applySimpleCompileMode();

                    const QString configPath =
                        QDir(projectPath).absoluteFilePath(
                            name + QStringLiteral(".lightcpproj"));
                    QStringList errors;
                    config.saveToFile(configPath, errors);
                }
            }

            // 4. 打开项目
            projectManager->openProject(QFileInfo(projectPath).absoluteFilePath());
        }

        // ============================================================
        // 创建主窗口
        // ============================================================
        auto mainWindow = std::make_unique<SprintIDE::MainWindow>();

        if (projectManager->hasOpenProject()) {
            QString projectName = projectManager->currentProjectName();
            mainWindow->setWindowTitle(QString("SprintIDE - %1").arg(projectName));
            mainWindow->openProject(projectManager->currentProjectPath());
        }

        {
            using namespace SprintIDE;
            auto res = ToolchainManager::instance().resolve(ToolchainId::LldbDap);
            if (res.found) {
                qDebug() << "[main] ✓ DAP adapter 已就绪:";
                qDebug() << "   path:   " << res.executablePath; 
                qDebug() << "   version:" << res.version;
                qDebug() << "   source: " << res.source;
            } else {
                qWarning() << "[main] ✗ DAP adapter 未找到";
            }
        }

        mainWindow->show();

        qDebug() << "SprintIDE启动成功！版本：" << app.applicationVersion();
        qDebug() << "应用程序字体：" << app.font().family();

        if (argc > 1) {
            for (int i = 1; i < argc; ++i) {
                // 跳过已被消费的参数
                if (consumedIndices.contains(i)) {
                    continue;
                }

                const QString arg = QString::fromLocal8Bit(argv[i]);

                // 跳过任何以 - 开头的选项
                if (arg.startsWith(QLatin1Char('-'))) {
                    continue;
                }

                const QFileInfo fi(arg);

                // 跳过（已在上面处理过）
                if (fi.isDir()) {
                    qDebug() << "[main] 跳过目录参数:" << arg;
                    continue;
                }

                // 文件不存在：只打日志，不弹窗
                //（避免因参数解析残留导致误报）
                if (!fi.exists() || !fi.isFile()) {
                    qDebug() << "[main] 跳过不存在的路径:" << arg;
                    continue;
                }

                // 是文件：正常打开
                if (arg.endsWith(QStringLiteral(".lightcpproj"))) {
                    qDebug() << "[main] 打开项目文件:" << arg;
                    mainWindow->openProject(arg);
                } else {
                    qDebug() << "[main] 打开代码文件:" << arg;
                    mainWindow->openFile(arg);
                }
            }
        }

        // 进入事件循环
        int result = app.exec();

        qDebug() << "[main] app.exec() 返回，退出码:" << result;

        // 显式销毁 MainWindow
        mainWindow.reset();
        qDebug() << "[main] MainWindow 已销毁";

        // 在 QApplication 析构之前，显式清理所有依赖 Qt 的静态单例

        // 3.1 DebugManager：持有 QProcess（LLDB/GDB）
        DebugManager::instance().shutdown();
        qDebug() << "[main] DebugManager 已 shutdown";

        // 3.2 GitManager：持有 QProcess
        SprintIDE::Git::GitManager::instance().shutdown();
        qDebug() << "[main] GitManager 已 shutdown";

        // 3.3 BuilderFactory：纯静态资源，但习惯上一起清理
        BuilderFactory::shutdown();
        qDebug() << "[main] BuilderFactory 已 shutdown";

        // 3.4 Application：清理 ConfigManager（QObject 子对象）
        //     注意：Application 单例本身的析构还会在 atexit 阶段跑一次，
        //          但因为内部状态已清空，第二次 shutdown() 是幂等的空操作。
        sprintApp.shutdown();

        // ============================================================
        // 从这里开始，改用 stderr —— qDebug 已经不可靠
        // ============================================================
        std::fprintf(stderr, "\n===== [main] 准备返回 =====\n");
        std::fflush(stderr);

        std::fprintf(stderr, "[main] 即将 return result\n");
        std::fflush(stderr);
        return result;

    } catch (const std::exception& e) {
        qCritical() << "致命错误:" << e.what();

        // 异常路径也要尝试清理
        DebugManager::instance().shutdown();
        SprintIDE::Git::GitManager::instance().shutdown();
        BuilderFactory::shutdown();

        QMessageBox::critical(nullptr, "致命错误",
                              QString("发生未处理的异常：\n\n%1\n\n"
                                      "SprintIDE将退出。请重启程序并检查文件完整性。")
                                  .arg(e.what()));
        return -1;
    } catch (...) {
        qCritical() << "未知致命错误";

        // 异常路径也要尝试清理
        DebugManager::instance().shutdown();
        SprintIDE::Git::GitManager::instance().shutdown();
        BuilderFactory::shutdown();

        QMessageBox::critical(nullptr, "未知错误",
                              "发生未知的严重错误。\n\n"
                              "SprintIDE将退出。请重启程序。");
        return -1;
    }
}
