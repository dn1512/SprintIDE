/*
* SprintIDE - SprintIDE is a cross-platform IDE built from scratch with Qt 6 and C++20. Its core capability is to provide AI Agents with an executable, verifiable code execution environment.
* Copyright (C) 2026  Liu Tengjun (刘腾骏)
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef DIAGNOSTICS_STORE_H
#define DIAGNOSTICS_STORE_H

#include <QObject>
#include <QString>
#include <QList>
#include <QHash>
#include <QMap>

namespace SprintIDE {

    /**
    * @brief 统一的诊断数据结构（编译器 + LSP 共用）
    */
    struct Diagnostic {
        QString filePath;      // 绝对路径
        int     line       = 0;   // 1-based
        int     column     = 0;   // 1-based，0 表示整行
        int     endLine    = 0;   // 1-based，0 表示未指定
        int     endColumn  = 0;
        QString severity;          // "error" / "warning" / "info" / "hint"
        QString message;
        QString source;            // "clangd" / "pylsp" / "gcc" / "g++" / ...
        QString code;              // LSP code / 编译器 errno

        bool isError()   const { return severity == QLatin1String("error"); }
        bool isWarning() const { return severity == QLatin1String("warning"); }
        bool isInfo()    const { return severity == QLatin1String("info"); }
        bool isHint()    const { return severity == QLatin1String("hint"); }
    };

    /**
    * @brief 诊断数据仓库（MainWindow 成员，不做单例）
    *
    * 设计：
    *   - 按 (filePath, source) 分组存储
    *   - 同 source 全量替换（编译完成时替换 gcc；LSP 刷新时替换 clangd）
    *   - 两个 source 独立，不互相踩
    *   - 每次变化 emit diagnosticsChanged / fileDiagnosticsChanged
    */
    class DiagnosticsStore : public QObject
    {
        Q_OBJECT
        public:
        explicit DiagnosticsStore(QObject* parent = nullptr);
        ~DiagnosticsStore() override = default;

        /// 全量替换 (filePath, source) 的诊断
        void replace(const QString& filePath, const QString& source,
            const QList<Diagnostic>& diags);

        /// 全量替换某 source 的所有诊断（跨文件，用于"清空所有编译器错误"）
        void clearSource(const QString& source);

        /// 清空某文件的所有诊断（文件关闭时）
        void clearFile(const QString& filePath);

        /// 清空所有
        void clearAll();

        // ---- 查询 ----
        QList<Diagnostic> all() const;
        QList<Diagnostic> forFile(const QString& filePath) const;
        int errorCount() const;
        int warningCount() const;

        signals:
        void diagnosticsChanged();
        void fileDiagnosticsChanged(const QString& filePath);

        private:
        // m_data[filePath][source] = diags
        //   filePath : 绝对路径
        //   source   : "clangd" / "pylsp" / "gcc" / ...
        QHash<QString, QMap<QString, QList<Diagnostic>>> m_data;
    };

} // namespace SprintIDE

#endif // DIAGNOSTICS_STORE_H