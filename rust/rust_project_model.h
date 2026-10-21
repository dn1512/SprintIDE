#ifndef RUST_PROJECT_MODEL_H
#define RUST_PROJECT_MODEL_H

#include <QString>
#include <QStringList>

namespace SprintIDE {

    struct RustTarget {
        QString name;          // [[bin]] name 或 package name
        QString kind;          // "bin" / "lib" / "example" / "test" / "bench"
        QString path;          // 源文件路径（相对项目根）
        bool    isDefault = false;  // 是否是默认 bin（src/main.rs）
    };

    class RustProjectModel
    {
        public:
        explicit RustProjectModel(const QString& projectPath);

        bool load();
        bool isValid() const { return m_valid; }

        QString projectPath() const { return m_projectPath; }
        QString packageName() const { return m_packageName; }
        QString cargoTomlPath() const;
        QString workspaceRoot() const;      // 如果是 workspace 成员，返回 workspace 根

        QList<RustTarget> targets() const { return m_targets; }
        RustTarget defaultBinary() const;   // 优先 src/main.rs 对应的 bin
        QStringList binaryNames() const;

        /// 给定源文件 → 找到它属于哪个 bin target（用于 onRun 时选 target）
        RustTarget targetForSource(const QString& sourceFile) const;

        /// 可执行文件路径：<root>/target/{debug,release}/<targetName>
        QString executablePath(const QString& targetName, bool release) const;

        private:
        bool parseCargoToml();
        bool detectTargets();

        QString m_projectPath;
        QString m_packageName;
        QString m_workspaceRoot;
        QList<RustTarget> m_targets;
        bool m_valid = false;
    };

} // namespace SprintIDE
#endif