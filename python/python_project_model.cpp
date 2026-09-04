#include "python_project_model.h"
#include "config_manager.h"
#include "python_virtual_env.h"
#include "application.h"
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

namespace SprintIDE {

PythonProjectModel::PythonProjectModel(const QString& projectPath)
    : m_projectPath(projectPath)
{
    m_projectName = QFileInfo(projectPath).fileName();
}

bool PythonProjectModel::load() {
    detectVenv();
    parseRequirements();
    parsePyproject();
    parseSetupPy();
    detectEntryPoint();
    return true;
}

void PythonProjectModel::detectVenv() {
    m_venvPath = PythonVirtualEnv::detectVenv(m_projectPath);
    if (!m_venvPath.isEmpty()) {
        m_interpreterPath = PythonVirtualEnv::interpreterPath(m_venvPath);
        return;
    }

    // 从 SDK 环境变量读取 PYTHON_HOME
    ConfigManager* cfg = Application::instance().configManager();
    QString pyHome;
    if (cfg) {
        QVariantMap sdkEnv = cfg->value("Sdk/Environment").toMap();
        if (sdkEnv.contains("PYTHON_HOME")) {
            pyHome = sdkEnv["PYTHON_HOME"].toString();
        }
    }
    if (!pyHome.isEmpty()) {
#ifdef Q_OS_WIN
        m_interpreterPath = pyHome + "/python.exe";
#else
        m_interpreterPath = pyHome;
#endif
        // 检查是否存在，若不存在则回退
        if (!QFile::exists(m_interpreterPath)) {
            m_interpreterPath = "python3";
        }
    } else {
        m_interpreterPath = "python3";
    }
}

void PythonProjectModel::parseRequirements() {
    QString reqFile = QDir(m_projectPath).absoluteFilePath("requirements.txt");
    if (!QFile::exists(reqFile)) return;
    QFile file(reqFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;
        // 简单提取包名（忽略版本）
        int idx = line.indexOf(QRegularExpression("[=<>!]"));
        if (idx > 0) {
            m_dependencies << line.left(idx).trimmed();
        } else {
            m_dependencies << line;
        }
    }
}

void PythonProjectModel::parsePyproject() {
    QString pyproj = QDir(m_projectPath).absoluteFilePath("pyproject.toml");
    if (!QFile::exists(pyproj)) return;
    // 简单解析：只读取 dependencies 部分（实际需要 TOML 解析库，这里简化）
    QFile file(pyproj);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QTextStream in(&file);
    bool inDeps = false;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.startsWith("[") && line.endsWith("]")) {
            inDeps = (line == "[project.dependencies]" || line == "[tool.poetry.dependencies]");
            continue;
        }
        if (inDeps && !line.isEmpty() && !line.startsWith("#")) {
            // 提取包名（以引号或字母开头）
            QRegularExpression re("^[\"']?([a-zA-Z0-9_\\-]+)");
            auto match = re.match(line);
            if (match.hasMatch()) {
                m_dependencies << match.captured(1);
            }
        }
    }
}

void PythonProjectModel::parseSetupPy() {
    QString setup = QDir(m_projectPath).absoluteFilePath("setup.py");
    if (!QFile::exists(setup)) return;
    QFile file(setup);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QTextStream in(&file);
    QString content = in.readAll();
    // 查找 install_requires = [...]
    QRegularExpression re(R"(install_requires\s*=\s*\[([^\]]*)\])", QRegularExpression::DotMatchesEverythingOption);
    auto match = re.match(content);
    if (match.hasMatch()) {
        QString deps = match.captured(1);
        // 提取包名（单引号或双引号）
        QRegularExpression pkgRe(R"([\"']([^\"']+)[\"'])");
        auto it = pkgRe.globalMatch(deps);
        while (it.hasNext()) {
            auto m = it.next();
            QString pkg = m.captured(1);
            // 去除版本
            int idx = pkg.indexOf(QRegularExpression("[=<>!]"));
            if (idx > 0) pkg = pkg.left(idx).trimmed();
            if (!pkg.isEmpty()) m_dependencies << pkg;
        }
    }
}

void PythonProjectModel::detectEntryPoint() {
    // 寻找 main.py, app.py, run.py 等
    QStringList candidates = { "main.py", "app.py", "run.py", "src/main.py", "src/app.py" };
    for (const QString& rel : candidates) {
        QString full = QDir(m_projectPath).absoluteFilePath(rel);
        if (QFile::exists(full)) {
            m_entryPoint = full;
            return;
        }
    }
    // 如果有 module 入口，可以检测 __init__.py 所在包，但简化处理
    m_entryPoint = QDir(m_projectPath).absoluteFilePath("main.py");
}

} // namespace SprintIDE
