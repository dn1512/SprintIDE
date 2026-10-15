#include "ai/executor/project_executor.h"
#include "ai/protocol/protocol_types.h"
#include "project_manager.h"
#include "mainwindow.h"
#include "core/project_scaffolder.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QStringConverter>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTimer>
#include <QDebug>

namespace SprintIDE {
namespace Protocol {

ProjectExecutor::ProjectExecutor(QObject* parent)
    : QObject(parent)
    , m_mainWindow(qobject_cast<MainWindow*>(parent))
{
    if (!m_mainWindow) {
        qWarning() << "[ProjectExecutor] MainWindow not provided; "
                      "project.open / project.register will fail";
    }
}

ExecutionResult ProjectExecutor::execute(const QJsonObject& item)
{
    const QString type = item.value("type").toString();

    if (type == QStringLiteral("project.info"))    return handleInfo(item);
    if (type == QStringLiteral("project.targets")) return handleTargets(item);
    if (type == QStringLiteral("project.open"))     return handleOpen(item);
    if (type == QStringLiteral("project.register")) return handleRegister(item);
    if (type == QStringLiteral("project.create"))   return handleCreate(item);

    return ExecutionResult::fail(
        item.value("id").toString(), type,
        ErrorCode::UnsupportedType,
        QStringLiteral("ProjectExecutor cannot handle type: %1").arg(type));
}

// ------------------------------------------------------------
// 基本信息
// ------------------------------------------------------------
QJsonObject ProjectExecutor::collectBasicInfo() const
{
    QJsonObject result;

    ProjectManager* pm = ProjectManager::instance();
    if (!pm) return result;

    const QString root = pm->currentProjectPath();
    if (root.isEmpty()) return result;

    // ---- projectId：路径 SHA1 前 16 个 hex 字符 ----
    const QByteArray hash = QCryptographicHash::hash(
        root.toUtf8(), QCryptographicHash::Sha1);
    const QString projectId = QString::fromLatin1(hash.toHex().left(16));

    result["projectId"]   = projectId;
    result["projectName"] = pm->currentProjectName();
    result["projectRoot"] = root;

    // ---- language + buildSystem ----
    QDir dir(root);
    QString language;
    QString buildSystem;

    if (dir.exists(QStringLiteral("CMakeLists.txt"))) {
        language    = QStringLiteral("cpp");
        buildSystem = QStringLiteral("cmake");
    } else if (dir.exists(QStringLiteral("Makefile")) ||
               dir.exists(QStringLiteral("makefile")) ||
               dir.exists(QStringLiteral("GNUmakefile"))) {
        language    = QStringLiteral("cpp");
        buildSystem = QStringLiteral("makefile");
    } else if (dir.exists(QStringLiteral("pyproject.toml"))) {
        language    = QStringLiteral("python");
        buildSystem = QStringLiteral("pyproject");
    } else if (dir.exists(QStringLiteral("requirements.txt")) ||
               dir.exists(QStringLiteral("setup.py"))) {
        language    = QStringLiteral("python");
        buildSystem = QStringLiteral("setuptools");
    } else if (dir.exists(QStringLiteral("pom.xml"))) {
        language    = QStringLiteral("java");
        buildSystem = QStringLiteral("maven");
    } else if (dir.exists(QStringLiteral("build.gradle")) ||
               dir.exists(QStringLiteral("build.gradle.kts"))) {
        language    = QStringLiteral("java");
        buildSystem = QStringLiteral("gradle");
    } else if (dir.exists(QStringLiteral("Cargo.toml"))) {
        language    = QStringLiteral("rust");
        buildSystem = QStringLiteral("cargo");
    } else if (dir.exists(QStringLiteral("go.mod"))) {
        language    = QStringLiteral("go");
        buildSystem = QStringLiteral("go");
    } else if (dir.exists(QStringLiteral("package.json"))) {
        language    = QStringLiteral("javascript");
        buildSystem = QStringLiteral("npm");
    } else {
        language    = QStringLiteral("unknown");
        buildSystem = QStringLiteral("unknown");
    }

    result["language"]    = language;
    result["buildSystem"] = buildSystem;

    // ---- defaultTarget / defaultEntry：第一版留空 ----
    result["defaultTarget"] = QString();
    result["defaultEntry"]  = QString();

    return result;
}

// ------------------------------------------------------------
// targets（第一版仅 CMake）
// ------------------------------------------------------------
QJsonArray ProjectExecutor::collectTargets() const
{
    QJsonArray targets;

    ProjectManager* pm = ProjectManager::instance();
    if (!pm) return targets;

    const QString root = pm->currentProjectPath();
    if (root.isEmpty()) return targets;

    const QString cmakePath = QDir(root).absoluteFilePath(
        QStringLiteral("CMakeLists.txt"));
    if (!QFile::exists(cmakePath)) return targets;

    QFile f(cmakePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return targets;

    QTextStream in(&f);
    in.setEncoding(QStringConverter::Utf8);
    const QString content = in.readAll();
    f.close();

    // 匹配 add_executable(name ...) 和 add_library(name ...)
    // 只抓以字母/下划线开头的标识符（忽略 ${VAR} 形式）
    static const QRegularExpression reExe(
        QStringLiteral(R"(add_executable\s*\(\s*([A-Za-z_][A-Za-z0-9_]*))"));
    static const QRegularExpression reLib(
        QStringLiteral(R"(add_library\s*\(\s*([A-Za-z_][A-Za-z0-9_]*))"));

    auto grab = [&](const QRegularExpression& re, const QString& type) {
        auto it = re.globalMatch(content);
        while (it.hasNext()) {
            const auto m = it.next();
            const QString name = m.captured(1);

            QJsonObject t;
            t["name"] = name;
            t["type"] = type;
            t["path"] = QString();
            targets.append(t);
        }
    };

    grab(reExe, QStringLiteral("executable"));
    grab(reLib, QStringLiteral("library"));

    return targets;
}

// ------------------------------------------------------------
// project.info
// ------------------------------------------------------------
ExecutionResult ProjectExecutor::handleInfo(const QJsonObject& item)
{
    const QString itemId = item.value("id").toString();

    QJsonObject result = collectBasicInfo();
    if (result.isEmpty()) {
        return ExecutionResult::fail(
            itemId, QStringLiteral("project.info"),
            ErrorCode::PathNotFound,
            QStringLiteral("No project is currently open"));
    }

    result["targets"] = collectTargets();

    return ExecutionResult::ok(itemId, QStringLiteral("project.info"), result);
}

// ------------------------------------------------------------
// project.targets
// ------------------------------------------------------------
ExecutionResult ProjectExecutor::handleTargets(const QJsonObject& item)
{
    const QString itemId = item.value("id").toString();

    QJsonObject result;
    result["targets"] = collectTargets();

    return ExecutionResult::ok(itemId, QStringLiteral("project.targets"), result);
}

// ------------------------------------------------------------
// project.open —— 打开文件夹作为项目（异步）
//
// 语义：
//   - 传入文件夹路径
//   - 若文件夹内没有 .lightcpproj，自动生成一个默认配置
//   - 打开项目、加载文件树、启动 LSP
//
// 为什么异步：
//   MainWindow::openProject() 内部会 loadAllProjectFiles() 遍历整棵
//   项目树，大项目可能几秒。若同步执行会阻塞 UI 线程，Agent 也会
//   因为拿不到回执而超时。所以这里先返回 Pending，等打开完成后
//   再通过 deferredResultReady 回执。
// ------------------------------------------------------------
ExecutionResult ProjectExecutor::handleOpen(const QJsonObject& item)
{
    const QString itemId = item.value("id").toString();
    const QString path   = item.value("path").toString();

    if (path.isEmpty()) {
        return ExecutionResult::fail(
            itemId, QStringLiteral("project.open"),
            ErrorCode::InvalidFields,
            QStringLiteral("Missing 'path' field"));
    }

    QFileInfo info(path);
    if (!info.exists() || !info.isDir()) {
        return ExecutionResult::fail(
            itemId, QStringLiteral("project.open"),
            ErrorCode::PathNotFound,
            QStringLiteral("Folder does not exist: %1").arg(path));
    }

    if (!m_mainWindow) {
        return ExecutionResult::fail(
            itemId, QStringLiteral("project.open"),
            ErrorCode::InternalError,
            QStringLiteral("MainWindow unavailable"));
    }

    const QString absPath = info.absoluteFilePath();

    // ---- 异步：延迟一拍，让 OperationBus 先看到 Pending ----
    QTimer::singleShot(0, m_mainWindow,
        [this, itemId, absPath]() {

            // 打开项目
            m_mainWindow->openProject(absPath);

            // 收集项目信息（此时 ProjectManager 已更新）
            QJsonObject data = collectBasicInfo();
            data["targets"] = collectTargets();

            ExecutionResult r = ExecutionResult::ok(
                itemId, QStringLiteral("project.open"), data);
            emit deferredResultReady(itemId, r);
        });

    // 返回 Pending（OperationBus 会保存 PendingState 并挂起）
    ExecutionResult pending;
    pending.itemId   = itemId;
    pending.itemType = QStringLiteral("project.open");
    pending.status   = ExecutionResult::Status::Pending;
    return pending;
}

// ------------------------------------------------------------
// project.register —— 只注册项目，不创建任何文件（同步）
//
// 与 project.open 的区别：
//   - project.open 会调用 MainWindow::openProject()，可能生成 .lightcpproj
//   - project.register 只把路径交给 MainWindow 走一次"轻量打开"流程，
//     但不会写入任何新文件，也不遍历目录树
//
// 本方法也复用 MainWindow::openProject()：
//   因为它本身不会在项目根创建文件夹/文件（.lightcpproj 是通过
//   ProjectConfig::saveToFile 生成的，属于项目元数据，不是用户代码）。
// ------------------------------------------------------------
ExecutionResult ProjectExecutor::handleRegister(const QJsonObject& item)
{
    const QString itemId = item.value("id").toString();
    const QString path   = item.value("path").toString();

    if (path.isEmpty()) {
        return ExecutionResult::fail(
            itemId, QStringLiteral("project.register"),
            ErrorCode::InvalidFields,
            QStringLiteral("Missing 'path' field"));
    }

    QFileInfo info(path);
    if (!info.exists() || !info.isDir()) {
        return ExecutionResult::fail(
            itemId, QStringLiteral("project.register"),
            ErrorCode::PathNotFound,
            QStringLiteral("Folder does not exist: %1").arg(path));
    }

    if (!m_mainWindow) {
        return ExecutionResult::fail(
            itemId, QStringLiteral("project.register"),
            ErrorCode::InternalError,
            QStringLiteral("MainWindow unavailable"));
    }

    // 同步打开
    m_mainWindow->openProject(info.absoluteFilePath());

    QJsonObject data = collectBasicInfo();
    data["targets"] = collectTargets();

    return ExecutionResult::ok(
        itemId, QStringLiteral("project.register"), data);
}

// ------------------------------------------------------------
// project.create —— 让 IDE 在磁盘上创建一个新项目
//
// 参数（两种形态二选一）：
//   ① 完整路径
//     { "path": "/Users/xxx/Documents/test_agent" }
//   ② 父目录 + 项目名
//     { "parentDir": "/Users/xxx/Documents", "name": "test_agent" }
//
// 其他字段：
//   language            必填（python / cpp / go / rust / node / generic_ai）
//   createVenv          可选，Python 专属
//   overwriteIfExists   可选，默认 false
//
// 语义：
//   - 目录不存在   → 创建
//   - 目录存在且空 → 直接用
//   - 目录存在非空 → 拒绝（除非 overwriteIfExists=true）
//   - 生成骨架 + .lightcpproj
//   - 打开项目
// ------------------------------------------------------------
ExecutionResult ProjectExecutor::handleCreate(const QJsonObject& item)
{
    const QString itemId = item.value("id").toString();

    // ---- 1. 解析路径 ----
    QString projectPath = item.value("path").toString();
    const QString parentDir = item.value("parentDir").toString();
    const QString name      = item.value("name").toString();

    if (projectPath.isEmpty()) {
        if (parentDir.isEmpty() || name.isEmpty()) {
            return ExecutionResult::fail(
                itemId, QStringLiteral("project.create"),
                ErrorCode::InvalidFields,
                QStringLiteral("Must provide either 'path', or both "
                               "'parentDir' and 'name'"));
        }
        projectPath = QDir(parentDir).absoluteFilePath(name);
    }

    // ---- 2. 校验 language ----
    const QString language = item.value("language").toString().toLower();
    if (language.isEmpty()) {
        return ExecutionResult::fail(
            itemId, QStringLiteral("project.create"),
            ErrorCode::InvalidFields,
            QStringLiteral("Missing 'language' field"));
    }

    // ---- 3. 调 scaffolder ----
    ProjectScaffolder::CreateOptions opts;
    opts.projectPath        = projectPath;
    opts.projectName        = name.isEmpty()
                                  ? QFileInfo(projectPath).fileName()
                                  : name;
    opts.displayName        = opts.projectName;
    opts.language           = language;
    opts.createVenv         = item.value("createVenv").toBool(false);
    opts.interpreterPath    = item.value("interpreterPath").toString();
    opts.overwriteIfExists  = item.value("overwriteIfExists").toBool(false);

    const auto scRes = ProjectScaffolder::create(opts);
    if (!scRes.success) {
        return ExecutionResult::fail(
            itemId, QStringLiteral("project.create"),
            scRes.errorCode.isEmpty() ? ErrorCode::InternalError : scRes.errorCode,
            scRes.errorMessage);
    }

    // ---- 4. 打开项目 ----
    if (m_mainWindow) {
        m_mainWindow->openProject(projectPath);
    } else {
        qWarning() << "[ProjectExecutor] MainWindow unavailable; "
                      "project created on disk but not opened";
    }

    // ---- 5. 收集信息 + 创建列表 ----
    QJsonObject data = collectBasicInfo();
    data["targets"] = collectTargets();

    QJsonArray createdFiles;
    for (const QString& f : scRes.createdFiles) createdFiles.append(f);
    data["createdFiles"] = createdFiles;

    QJsonArray createdDirs;
    for (const QString& d : scRes.createdDirs) createdDirs.append(d);
    data["createdDirs"] = createdDirs;

    return ExecutionResult::ok(
        itemId, QStringLiteral("project.create"), data);
}

} // namespace Protocol
} // namespace SprintIDE
