#include "application.h"
#include "mainwindow.h"
#include "project_manager.h"
#include "project_selection_dialog.h"
#include "deps/toolchain_manager.h"

// 需要显式清理的静态单例
#include "debug_manager.h"
#include "builder/builder_factory.h"

#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <QFile>
#include <QIcon>
#include <QDir>
#include <memory>   // ★ 新增：std::make_unique

// 使用命名空间SprintIDE
using namespace SprintIDE;

/**
 * @brief 应用程序主入口
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return 应用程序退出代码
 */
int main(int argc, char *argv[])
{
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
    QFont font;
#ifdef Q_OS_WIN
    font.setFamily("Microsoft YaHei");
#elif defined(Q_OS_MAC)
    font.setFamily("JetBrainsMono-Regular");
#else
    font.setFamily("WenQuanYi Micro Hei");
#endif
    font.setPointSize(10);
    font.setStyleHint(QFont::SansSerif);
    app.setFont(font);

    // 创建必要的目录结构
    QDir().mkdir("config");
    QDir().mkdir("projects");
    QDir().mkdir("temp");

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

        QString projectPath;
        if (argc > 1) {
            projectPath = QString::fromLocal8Bit(argv[1]);
        }

        if (projectPath.isEmpty()) {
            ProjectSelectionDialog dialog(projectManager->recentProjects());

            if (dialog.exec() == QDialog::Accepted) {
                if (dialog.isCreateNewProject()) {
                    qDebug() << "User chose to create new project";
                    projectPath = "create_new";
                } else {
                    projectPath = dialog.selectedProjectPath();
                    if (!projectPath.isEmpty()) {
                        projectManager->openProject(projectPath);
                    }
                }
            } else {
                // 用户取消，走正常清理流程（避免直接 return 0 跳过清理）
                sprintApp.shutdown();
                return 0;
            }
        } else {
            projectManager->openProject(projectPath);
        }

        // ================================================================
        // ★ 关键改动 1：用 unique_ptr 接管 MainWindow 的生命周期
        //   原因：MainWindow 是栈对象时，只能在 main() 末尾析构，
        //         那时已经是 QApplication 析构之后，来不及清理单例。
        //         用 unique_ptr 可以在 app.exec() 返回后立即 reset()。
        // ================================================================
        auto mainWindow = std::make_unique<SprintIDE::MainWindow>();

        if (projectManager->hasOpenProject()) {
            QString projectName = projectManager->currentProjectName();
            mainWindow->setWindowTitle(QString("SprintIDE - %1").arg(projectName));
            mainWindow->openProject(projectManager->currentProjectPath());
        } else if (projectPath == "create_new") {
            mainWindow->onNewProject();
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
                QString arg = QString::fromLocal8Bit(argv[i]);

                if (arg.startsWith("-")) {
                    continue;
                }

                if (QFile::exists(arg)) {
                    if (arg.endsWith(".lightcppproj")) {
                        qDebug() << "打开项目文件：" << arg;
                        mainWindow->openProject(arg);
                    } else {
                        qDebug() << "打开代码文件：" << arg;
                        mainWindow->openFile(arg);
                    }
                } else {
                    qWarning() << "文件不存在：" << arg;
                    QMessageBox::warning(nullptr, "文件不存在",
                                         QString("无法打开文件：%1\n文件不存在。").arg(arg));
                }
            }
        }

        // ================================================================
        // 进入事件循环
        // ================================================================
        int result = app.exec();

        qDebug() << "[main] app.exec() 返回，退出码:" << result;

        // ================================================================
        // ★ 关键改动 2：显式销毁 MainWindow
        //   这会触发：
        //     - LSPManager::shutdownAll()（在 ~MainWindow 里）
        //     - PythonDebugger::stopDebugging()（在 ~MainWindow 里）
        //     - 所有 TextEditor / LanguageHighlighter 的析构
        //     - 断开所有信号连接
        //   必须在 QApplication 还活着时完成。
        // ================================================================
        mainWindow.reset();
        qDebug() << "[main] MainWindow 已销毁";

        // ================================================================
        // ★ 关键改动 3：在 QApplication 析构之前，显式清理所有依赖 Qt 的静态单例
        //
        //   这些单例（Meyers singleton）如果放任不管，它们的析构时机在
        //   main() 返回后、进程退出前的 atexit 阶段——此时 QApplication
        //   已经析构，QProcess / QTcpSocket 等会访问已释放的 eventDispatcher
        //   导致 SIGABRT（macOS 上表现为"意外退出"）。
        //
        //   显式调用 shutdown() 后，这些单例内部持有的 Qt 对象被销毁，
        //   析构函数再被调用时就是空操作，不会再崩溃。
        // ================================================================

        // 3.1 DebugManager：持有 QProcess（LLDB/GDB）
        DebugManager::instance().shutdown();
        qDebug() << "[main] DebugManager 已 shutdown";

        // 3.2 BuilderFactory：纯静态资源，但习惯上一起清理
        BuilderFactory::shutdown();
        qDebug() << "[main] BuilderFactory 已 shutdown";

        // 3.3 Application：清理 ConfigManager（QObject 子对象）
        //     注意：Application 单例本身的析构还会在 atexit 阶段跑一次，
        //          但因为内部状态已清空，第二次 shutdown() 是幂等的空操作。
        sprintApp.shutdown();

        qDebug() << "SprintIDE正常退出，退出码：" << result;
        return result;

    } catch (const std::exception& e) {
        qCritical() << "致命错误:" << e.what();

        // 异常路径也要尝试清理
        DebugManager::instance().shutdown();
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
        BuilderFactory::shutdown();

        QMessageBox::critical(nullptr, "未知错误",
                              "发生未知的严重错误。\n\n"
                              "SprintIDE将退出。请重启程序。");
        return -1;
    }
}
