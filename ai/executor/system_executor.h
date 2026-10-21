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
#ifndef SYSTEM_EXECUTOR_H
#define SYSTEM_EXECUTOR_H

#include <QObject>
#include "ai/executor/i_executor.h"

namespace SprintIDE {
// 前向声明
class MainWindow;
namespace Protocol {

/**
 * @brief 处理 system.* 域
 *
 * 已实现：
 *  - system.getServerAddress  查询当前 AI 监听地址 + 端口
 *  - system.setServerAddress  修改监听地址 + 端口（需要重连）
 *
 * 与 meta.* 的区别：
 *   meta.*   是"操作元信息"（batch / abort / done）
 *   system.* 是"IDE 运行时环境"（连接配置、系统信息等）
 */
class SystemExecutor : public QObject, public IExecutor
{
    Q_OBJECT
public:
    explicit SystemExecutor(QObject* parent = nullptr);

    QString domain() const override { return QStringLiteral("system"); }
    ExecutionResult execute(const QJsonObject& item) override;

private:
    ExecutionResult handleGetServerAddress(const QJsonObject& item);
    ExecutionResult handleSetServerAddress(const QJsonObject& item);

    MainWindow* m_mainWindow = nullptr;
};

} // namespace Protocol
} // namespace SprintIDE

#endif // SYSTEM_EXECUTOR_H