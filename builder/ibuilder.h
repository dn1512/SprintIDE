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
#ifndef IBUILDER_H
#define IBUILDER_H

#include "build_task.h"
#include "build_result.h"
#include <QProcessEnvironment>

namespace SprintIDE {

/**
 * @brief 构建器抽象接口
 *
 * 继承 QObject 以便发出 outputReady 信号（实时输出）。
 */
class IBuilder : public QObject {
    Q_OBJECT
public:
    explicit IBuilder(QObject* parent = nullptr) : QObject(parent) {}
    ~IBuilder() override = default;

    /**
     * @brief 准备构建所需的环境变量
     * @param task 构建任务
     * @return 环境变量对象
     */
    virtual QProcessEnvironment prepareEnvironment(const BuildTask& task) = 0;

    /**
     * @brief 执行构建
     * @param task 构建任务
     * @param result 输出结果
     * @return 是否成功启动（不代表构建成功，需检查 result.success）
     */
    virtual bool build(const BuildTask& task, BuildResult& result) = 0;

    /**
     * @brief 清理构建产物
     * @param task 构建任务（提供项目路径等）
     * @param result 输出结果
     * @return 是否成功清理
     */
    virtual bool clean(const BuildTask& task, BuildResult& result) = 0;

    /**
     * @brief 运行生成的可执行文件
     * @param task 构建任务（含运行参数）
     * @param buildResult 之前构建的结果（含可执行文件路径）
     * @param output 运行输出
     * @return 是否成功运行
     */
    virtual bool run(const BuildTask& task, const BuildResult& buildResult, QString& output) = 0;

    /**
     * @brief 获取构建产物的可执行文件路径
     * @param task 构建任务
     * @return 可执行文件的完整路径，如果无法确定则返回空字符串
     */
    virtual QString getExecutablePath(const BuildTask& task) = 0;

signals:
    /**
     * @brief 构建过程中有输出到达（可能触发多次）
     * @param chunk    输出片段（可能含多行）
     * @param isError  是否为 stderr
     *
     * 注意：此信号可能在**工作线程**发出（因为 build() 是在
     *      QtConcurrent 线程池里跑的）。接收方必须用
     *      Qt::QueuedConnection 或接收方本身在 QObject 里。
     */
    void outputReady(const QString& chunk, bool isError);
};

} // namespace SprintIDE

#endif // IBUILDER_H
