#include "application.h"
#include "mainwindow.h"
#include "project_manager.h"
#include "project_selection_dialog.h"

#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <QFile>
#include <QIcon>
#include <QDir>

// 使用命名空间SprintIDE
using namespace SprintIDE;

/**
 * @brief 应用程序主入口
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return 应用程序退出代码
 *
 * 这个函数是SprintIDE的启动入口，负责：
 * 1. 创建QApplication实例
 * 2. 设置应用程序图标和字体
 * 3. 初始化应用程序核心
 * 4. 显示项目选择对话框
 * 5. 创建并显示主窗口
 * 6. 处理命令行参数
 * 7. 执行应用程序主循环
 */
int main(int argc, char *argv[])
{
    // 创建QApplication实例，管理GUI应用程序的控制流和主要设置
    QApplication app(argc, argv);

    // 设置应用程序的基本信息
    app.setApplicationName("SprintIDE");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("SprintIDE");
    app.setOrganizationDomain("sprintide.org");

    // 设置应用程序图标
    app.setWindowIcon(QIcon(":/icons/icons/app_logo.ico"));

    // 设置应用程序字体 - 优先使用适合儿童的中文字体
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
                return 0;
            }
        } else {
            projectManager->openProject(projectPath);
        }

        SprintIDE::MainWindow mainWindow;

        if (projectManager->hasOpenProject()) {
            QString projectName = projectManager->currentProjectName();
            mainWindow.setWindowTitle(QString("SprintIDE - %1").arg(projectName));
            mainWindow.openProject(projectManager->currentProjectPath());
        } else if (projectPath == "create_new") {
            mainWindow.onNewProject();
        }

        mainWindow.show();

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
                        mainWindow.openProject(arg);
                    } else {
                        qDebug() << "打开代码文件：" << arg;
                        mainWindow.openFile(arg);
                    }
                } else {
                    qWarning() << "文件不存在：" << arg;
                    QMessageBox::warning(nullptr, "文件不存在",
                                         QString("无法打开文件：%1\n文件不存在。").arg(arg));
                }
            }
        }

        int result = app.exec();

        // 清理资源
        sprintApp.shutdown();

        qDebug() << "SprintIDE正常退出，退出码：" << result;
        return result;

    } catch (const std::exception& e) {
        qCritical() << "致命错误:" << e.what();
        QMessageBox::critical(nullptr, "致命错误",
                              QString("发生未处理的异常：\n\n%1\n\n"
                                      "SprintIDE将退出。请重启程序并检查文件完整性。")
                                  .arg(e.what()));
        return -1;
    } catch (...) {
        qCritical() << "未知致命错误";
        QMessageBox::critical(nullptr, "未知错误",
                              "发生未知的严重错误。\n\n"
                              "SprintIDE将退出。请重启程序。");
        return -1;
    }
}
