#include "external_library_provider_factory.h"

#include <QDir>
#include <QFileInfo>

#include "providers/python_external_library_provider.h"
#include "providers/rust_external_library_provider.h"
#include "providers/go_external_library_provider.h"
#include "providers/node_external_library_provider.h"
#include "providers/cpp_external_library_provider.h"

namespace SprintIDE {

    std::unique_ptr<IExternalLibraryProvider>
    ExternalLibraryProviderFactory::forProject(const QString& projectPath)
    {
        if (projectPath.isEmpty() || !QDir(projectPath).exists()) {
            return nullptr;
        }

        QDir d(projectPath);

        // ---- Python ----
        if (d.exists("pyproject.toml")
            || d.exists("requirements.txt")
            || d.exists("setup.py")
            || d.exists(".venv")
            || d.exists("venv")
            || !d.entryList(QStringList() << "*.py", QDir::Files).isEmpty()) {
            return std::make_unique<PythonExternalLibraryProvider>();
        }

        // ---- Rust ----
        if (d.exists("Cargo.toml")) {
            return std::make_unique<RustExternalLibraryProvider>();
        }

        // ---- Go ----
        if (d.exists("go.mod")) {
            return std::make_unique<GoExternalLibraryProvider>();
        }

        // ---- Node（JS/TS） ----
        if (d.exists("package.json")
            || !d.entryList(QStringList() << "*.js" << "*.ts"
                << "*.jsx" << "*.tsx",
                QDir::Files).isEmpty()) {
            return std::make_unique<NodeExternalLibraryProvider>();
        }

        // ---- C++ / Qt ----
        if (d.exists("CMakeLists.txt")
            || d.exists("Makefile")
            || d.exists("compile_commands.json")
            || d.exists("build/compile_commands.json")) {
            return std::make_unique<CppExternalLibraryProvider>();
        }

        return nullptr;
    }

} // namespace SprintIDE