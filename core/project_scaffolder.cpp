#include "core/project_scaffolder.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QStringConverter>
#include <QJsonObject>
#include <QJsonDocument>
#include <QProcess>
#include <QDebug>

namespace SprintIDE {

// ============================================================
// 公共入口
// ============================================================
ProjectScaffolder::CreateResult
ProjectScaffolder::create(const CreateOptions& opts)
{
    CreateResult result;

    if (opts.projectPath.isEmpty()) {
        result.errorCode    = QStringLiteral("INVALID_FIELDS");
        result.errorMessage = QStringLiteral("projectPath is empty");
        return result;
    }
    if (opts.language.isEmpty()) {
        result.errorCode    = QStringLiteral("INVALID_FIELDS");
        result.errorMessage = QStringLiteral("language is empty");
        return result;
    }

    // ---- 1. 检查目标目录 ----
    QDir dir(opts.projectPath);
    if (dir.exists()) {
        const QStringList entries =
            dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
        if (!entries.isEmpty() && !opts.overwriteIfExists) {
            result.errorCode    = QStringLiteral("PATH_EXISTS");
            result.errorMessage = QStringLiteral(
                "Target directory already exists and is not empty: %1")
                .arg(opts.projectPath);
            return result;
        }
    } else {
        if (!mkpath(opts.projectPath)) {
            result.errorCode    = QStringLiteral("IO_ERROR");
            result.errorMessage = QStringLiteral(
                "Failed to create directory: %1").arg(opts.projectPath);
            return result;
        }
        result.createdDirs.append(opts.projectPath);
    }

    // ---- 2. 按语言分发 ----
    const QString lang = opts.language.toLower();

    if (lang == QStringLiteral("python"))
        return createPython(opts);
    if (lang == QStringLiteral("cpp") || lang == QStringLiteral("c++"))
        return createCpp(opts);
    if (lang == QStringLiteral("go"))
        return createGo(opts);
    if (lang == QStringLiteral("rust"))
        return createRust(opts);
    if (lang == QStringLiteral("node") || lang == QStringLiteral("js"))
        return createNode(opts);
    if (lang == QStringLiteral("generic_ai"))
        return createGeneric(opts);

    result.errorCode    = QStringLiteral("UNSUPPORTED_LANGUAGE");
    result.errorMessage = QStringLiteral("Unsupported language: %1").arg(opts.language);
    return result;
}

// ============================================================
// Python
// ============================================================
ProjectScaffolder::CreateResult
ProjectScaffolder::createPython(const CreateOptions& opts)
{
    CreateResult r;
    QDir root(opts.projectPath);

    const QString name = opts.projectName.isEmpty()
                             ? QFileInfo(opts.projectPath).fileName()
                             : opts.projectName;

    // ---- main.py ----
    const QString mainPy = QStringLiteral(
        "def main():\n"
        "    print(\"Hello from %1!\")\n"
        "\n"
        "if __name__ == \"__main__\":\n"
        "    main()\n"
    ).arg(name);

    const QString mainPath = root.absoluteFilePath(QStringLiteral("main.py"));
    if (!writeTextFile(mainPath, mainPy)) {
        r.errorCode = QStringLiteral("IO_ERROR");
        r.errorMessage = QStringLiteral("Failed to write main.py");
        return r;
    }
    r.createdFiles.append(mainPath);

    // ---- requirements.txt ----
    const QString reqPath =
        root.absoluteFilePath(QStringLiteral("requirements.txt"));
    if (!writeTextFile(reqPath, QString())) {
        r.errorCode = QStringLiteral("IO_ERROR");
        r.errorMessage = QStringLiteral("Failed to write requirements.txt");
        return r;
    }
    r.createdFiles.append(reqPath);

    // ---- README.md ----
    const QString readme =
        QStringLiteral("# %1\n\nCreated by SprintIDE.\n").arg(name);
    const QString readmePath =
        root.absoluteFilePath(QStringLiteral("README.md"));
    if (!writeTextFile(readmePath, readme)) {
        r.errorCode = QStringLiteral("IO_ERROR");
        r.errorMessage = QStringLiteral("Failed to write README.md");
        return r;
    }
    r.createdFiles.append(readmePath);

    // ---- .lightcpproj ----
    const QString projFile = root.absoluteFilePath(
        QStringLiteral("%1.lightcpproj").arg(name));
    if (!writeTextFile(projFile, buildConfigFileContent(opts))) {
        r.errorCode = QStringLiteral("IO_ERROR");
        r.errorMessage = QStringLiteral("Failed to write .lightcpproj");
        return r;
    }
    r.createdFiles.append(projFile);

    // ---- 可选：创建 .venv ----
    if (opts.createVenv) {
        QString python = opts.interpreterPath;
        if (python.isEmpty()) python = QStringLiteral("python3");

        QProcess p;
        p.setWorkingDirectory(opts.projectPath);
        p.start(python, {QStringLiteral("-m"), QStringLiteral("venv"),
                         QStringLiteral(".venv")});
        if (p.waitForFinished(60000)) {
            r.createdDirs.append(
                root.absoluteFilePath(QStringLiteral(".venv")));
        } else {
            qWarning() << "[Scaffolder] venv creation timed out, skipped";
        }
    }

    r.success = true;
    return r;
}

// ============================================================
// C++（最简：src/main.cpp + .lightcpproj）
// ============================================================
ProjectScaffolder::CreateResult
ProjectScaffolder::createCpp(const CreateOptions& opts)
{
    CreateResult r;
    QDir root(opts.projectPath);
    root.mkpath(QStringLiteral("src"));
    r.createdDirs.append(root.absoluteFilePath(QStringLiteral("src")));

    const QString name = opts.projectName.isEmpty()
                             ? QFileInfo(opts.projectPath).fileName()
                             : opts.projectName;

    const QString mainCpp = QStringLiteral(
        "#include <iostream>\n"
        "\n"
        "int main() {\n"
        "    std::cout << \"Hello from %1!\" << std::endl;\n"
        "    return 0;\n"
        "}\n"
    ).arg(name);

    const QString mainPath =
        root.absoluteFilePath(QStringLiteral("src/main.cpp"));
    if (!writeTextFile(mainPath, mainCpp)) {
        r.errorCode = QStringLiteral("IO_ERROR");
        r.errorMessage = QStringLiteral("Failed to write src/main.cpp");
        return r;
    }
    r.createdFiles.append(mainPath);

    const QString projFile = root.absoluteFilePath(
        QStringLiteral("%1.lightcpproj").arg(name));
    if (!writeTextFile(projFile, buildConfigFileContent(opts))) {
        r.errorCode = QStringLiteral("IO_ERROR");
        r.errorMessage = QStringLiteral("Failed to write .lightcpproj");
        return r;
    }
    r.createdFiles.append(projFile);

    r.success = true;
    return r;
}

// ============================================================
// Go
// ============================================================
ProjectScaffolder::CreateResult
ProjectScaffolder::createGo(const CreateOptions& opts)
{
    CreateResult r;
    QDir root(opts.projectPath);
    const QString name = opts.projectName.isEmpty()
                             ? QFileInfo(opts.projectPath).fileName()
                             : opts.projectName;

    const QString mainGo = QStringLiteral(
        "package main\n"
        "\n"
        "import \"fmt\"\n"
        "\n"
        "func main() {\n"
        "    fmt.Println(\"Hello from %1!\")\n"
        "}\n"
    ).arg(name);

    const QString mainPath = root.absoluteFilePath(QStringLiteral("main.go"));
    if (!writeTextFile(mainPath, mainGo)) {
        r.errorCode = QStringLiteral("IO_ERROR");
        r.errorMessage = QStringLiteral("Failed to write main.go");
        return r;
    }
    r.createdFiles.append(mainPath);

    const QString modPath = root.absoluteFilePath(QStringLiteral("go.mod"));
    const QString modContent = QStringLiteral(
        "module %1\n\ngo 1.21\n").arg(name.toLower());
    if (!writeTextFile(modPath, modContent)) {
        r.errorCode = QStringLiteral("IO_ERROR");
        r.errorMessage = QStringLiteral("Failed to write go.mod");
        return r;
    }
    r.createdFiles.append(modPath);

    const QString projFile = root.absoluteFilePath(
        QStringLiteral("%1.lightcpproj").arg(name));
    writeTextFile(projFile, buildConfigFileContent(opts));
    r.createdFiles.append(projFile);

    r.success = true;
    return r;
}

// ============================================================
// Rust
// ============================================================
ProjectScaffolder::CreateResult
ProjectScaffolder::createRust(const CreateOptions& opts)
{
    CreateResult r;
    QDir root(opts.projectPath);
    root.mkpath(QStringLiteral("src"));
    r.createdDirs.append(root.absoluteFilePath(QStringLiteral("src")));

    const QString name = opts.projectName.isEmpty()
                             ? QFileInfo(opts.projectPath).fileName()
                             : opts.projectName;

    const QString cargoToml = QStringLiteral(
        "[package]\n"
        "name = \"%1\"\n"
        "version = \"0.1.0\"\n"
        "edition = \"2021\"\n\n"
        "[dependencies]\n"
    ).arg(name.toLower());
    const QString cargoPath =
        root.absoluteFilePath(QStringLiteral("Cargo.toml"));
    writeTextFile(cargoPath, cargoToml);
    r.createdFiles.append(cargoPath);

    const QString mainRs = QStringLiteral(
        "fn main() {\n"
        "    println!(\"Hello from %1!\");\n"
        "}\n"
    ).arg(name);
    const QString mainPath =
        root.absoluteFilePath(QStringLiteral("src/main.rs"));
    writeTextFile(mainPath, mainRs);
    r.createdFiles.append(mainPath);

    const QString projFile = root.absoluteFilePath(
        QStringLiteral("%1.lightcpproj").arg(name));
    writeTextFile(projFile, buildConfigFileContent(opts));
    r.createdFiles.append(projFile);

    r.success = true;
    return r;
}

// ============================================================
// Node
// ============================================================
ProjectScaffolder::CreateResult
ProjectScaffolder::createNode(const CreateOptions& opts)
{
    CreateResult r;
    QDir root(opts.projectPath);
    const QString name = opts.projectName.isEmpty()
                             ? QFileInfo(opts.projectPath).fileName()
                             : opts.projectName;

    const QString pkg = QStringLiteral(
        "{\n"
        "  \"name\": \"%1\",\n"
        "  \"version\": \"1.0.0\",\n"
        "  \"main\": \"index.js\",\n"
        "  \"scripts\": {\n"
        "    \"start\": \"node index.js\"\n"
        "  }\n"
        "}\n"
    ).arg(name.toLower());
    const QString pkgPath =
        root.absoluteFilePath(QStringLiteral("package.json"));
    writeTextFile(pkgPath, pkg);
    r.createdFiles.append(pkgPath);

    const QString indexJs = QStringLiteral(
        "console.log('Hello from %1!');\n").arg(name);
    const QString indexPath =
        root.absoluteFilePath(QStringLiteral("index.js"));
    writeTextFile(indexPath, indexJs);
    r.createdFiles.append(indexPath);

    const QString projFile = root.absoluteFilePath(
        QStringLiteral("%1.lightcpproj").arg(name));
    writeTextFile(projFile, buildConfigFileContent(opts));
    r.createdFiles.append(projFile);

    r.success = true;
    return r;
}

// ============================================================
// Generic
// ============================================================
ProjectScaffolder::CreateResult
ProjectScaffolder::createGeneric(const CreateOptions& opts)
{
    CreateResult r;
    QDir root(opts.projectPath);
    const QString name = opts.projectName.isEmpty()
                             ? QFileInfo(opts.projectPath).fileName()
                             : opts.projectName;

    const QString readme = QStringLiteral(
        "# %1\n\nCreated by SprintIDE (generic).\n").arg(name);
    const QString readmePath =
        root.absoluteFilePath(QStringLiteral("README.md"));
    writeTextFile(readmePath, readme);
    r.createdFiles.append(readmePath);

    const QString projFile = root.absoluteFilePath(
        QStringLiteral("%1.lightcpproj").arg(name));
    writeTextFile(projFile, buildConfigFileContent(opts));
    r.createdFiles.append(projFile);

    r.success = true;
    return r;
}

// ============================================================
// 工具方法
// ============================================================
bool ProjectScaffolder::writeTextFile(const QString& filePath,
                                      const QString& content)
{
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[Scaffolder] cannot write:" << filePath
                   << f.errorString();
        return false;
    }
    QTextStream ts(&f);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    ts.setEncoding(QStringConverter::Utf8);
#else
    ts.setCodec("UTF-8");
#endif
    ts << content;
    f.close();
    return true;
}

bool ProjectScaffolder::mkpath(const QString& dirPath)
{
    QDir d;
    return d.mkpath(dirPath);
}

QString ProjectScaffolder::buildConfigFileContent(const CreateOptions& opts)
{
    // 一个最小的 .lightcpproj JSON，与 ProjectConfig::loadFromFile 兼容
    QJsonObject root;
    root["project_name"]  = opts.projectName;
    root["display_name"]  = opts.displayName.isEmpty()
                                ? opts.projectName
                                : opts.displayName;
    root["project_path"]  = opts.projectPath;
    root["project_type"]  = QStringLiteral("empty");
    root["config_version"] = QStringLiteral("1.0.0");
    root["compile_mode"]  = QStringLiteral("simple");
    root["compiler_type"] = QStringLiteral("auto");

    // 按语言给一个合理的 output_path
    const QString lang = opts.language.toLower();
    if (lang == QStringLiteral("python"))
        root["output_path"] = QStringLiteral("__pycache__");
    else if (lang == QStringLiteral("rust"))
        root["output_path"] = QStringLiteral("target");
    else if (lang == QStringLiteral("node"))
        root["output_path"] = QStringLiteral("dist");
    else
        root["output_path"] = QStringLiteral("bin");

    return QString::fromUtf8(
        QJsonDocument(root).toJson(QJsonDocument::Indented));
}

QString ProjectScaffolder::buildDefaultEntryContent(const QString& language,
                                                    const QString& projectName)
{
    Q_UNUSED(language);
    Q_UNUSED(projectName);
    return QString();
}

} // namespace SprintIDE