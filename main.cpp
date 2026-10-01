#include "application.h"
#include "mainwindow.h"
#include "project_manager.h"
#include "project_selection_dialog.h"
#include "deps/toolchain_manager.h"

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

        QString projectPath;
        if (argc > 1) {
            projectPath = QString::fromLocal8Bit(argv[1]);
        }

        if (projectPath.isEmpty()) {
            while (true) {
                // ---------- 弹"选择项目"对话框 ----------
                ProjectSelectionDialog selDlg(projectManager->recentProjects());

                if (selDlg.exec() != QDialog::Accepted) {
                    // 用户取消选择 → 退出程序
                    sprintApp.shutdown();
                    return 0;
                }

                if (!selDlg.isCreateNewProject()) {
                    // 用户选了已有项目
                    projectPath = selDlg.selectedProjectPath();
                    if (!projectPath.isEmpty()) {
                        projectManager->openProject(projectPath);
                        break;   // 选定 → 离开循环
                    }
                    continue;    // 空路径 → 回到选择对话框
                }

                // ---------- 用户选了"创建新项目" → 弹"创建项目"对话框 ----------
                ProjectCreationDialog createDlg;
                createDlg.setDefaultProjectPath(
                    QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));

                if (createDlg.exec() != QDialog::Accepted) {
                    // 用户取消创建 → continue 回到"选择项目"对话框
                    continue;
                }

                ProjectCreationResult result = createDlg.getResult();
                if (result.success && !result.projectPath.isEmpty()) {
                    projectPath = result.projectPath;
                    projectManager->openProject(projectPath);
                    break;   // 创建成功 → 离开循环
                }
                // 创建失败 → 回到"选择项目"对话框
            }
        } else {
            projectManager->openProject(projectPath);
        }

        // 用 unique_ptr 接管 MainWindow 的生命周期
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
