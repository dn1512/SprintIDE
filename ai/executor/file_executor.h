// SPDX-License-Identifier: GPL-3.0-or-later
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
#ifndef FILE_EXECUTOR_H
#define FILE_EXECUTOR_H

#include <QObject>
#include "ai/executor/i_executor.h"

namespace SprintIDE {

class MainWindow;

namespace Protocol {

/**
 * @brief 处理 file.* 域
 *
 * 阶段 1 实现：read / create / delete / rename
 * 阶段 3 实现：edit（需要 TextEditor + LocalHistory）
 */
class FileExecutor : public QObject, public IExecutor
{
    Q_OBJECT
public:
    explicit FileExecutor(MainWindow* mw, QObject* parent = nullptr);

    QString domain() const override { return QStringLiteral("file"); }
    ExecutionResult execute(const QJsonObject& item) override;

private:
    ExecutionResult handleRead(const QJsonObject& item);
    ExecutionResult handleCreate(const QJsonObject& item);
    ExecutionResult handleDelete(const QJsonObject& item);
    ExecutionResult handleRename(const QJsonObject& item);
    ExecutionResult handleEdit(const QJsonObject& item);   // 骨架

    /// 相对路径 → 绝对路径（基于当前项目根；无项目则基于 cwd）
    QString resolvePath(const QString& rawPath) const;
    MainWindow* m_mw = nullptr;
};

} // namespace Protocol
} // namespace SprintIDE

#endif // FILE_EXECUTOR_H
