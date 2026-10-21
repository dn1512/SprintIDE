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
#ifndef AI_INTEGRATION_H
#define AI_INTEGRATION_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QSettings>
#include <QMenu>

#include "ai/chat_panel.h"


namespace SprintIDE {
// 前向声明（避免循环依赖）
class MainWindow;

// 前向声明 AI 模块的类
class ChatPanel;
class AiConnectionManager;

struct CompileResult;

/**
 * ============================================================
 * AI 模块集成接口
 * ============================================================
 *
 * 这个头文件提供了将 AI 模块集成到 MainWindow 的所有函数。
 * 典型的集成流程：
 *
 * 1. 在 MainWindow 构造函数中调用 initializeAIIntegration()
 * 2. 在 MainWindow 构造函数中调用 setupAIMenu()
 * 3. 在 MainWindow 的 closeEvent 中调用 saveChatPanelState()
 * 4. 在 MainWindow 构造函数或初始化函数中调用 restoreChatPanelState()
 *
 * 所有函数都是线程安全的，因为 Qt 的 GUI 操作都在主线程执行。
 */

/**
 * @brief 初始化 AI 模块并集成到主窗口
 *
 * 此函数会：
 * 1. 创建连接管理器
 * 2. 创建聊天面板并停靠在右侧
 * 3. 连接各组件之间的信号槽
 * 4. 创建 OperationBus 并注册 7 个 Executor
 *
 * @param mainWindow 主窗口指针（不能为空）
 * @param outConnMgr 输出参数，返回连接管理器指针
 * @param outExecutor 输出参数，已废弃（阶段 5 清理），始终为 nullptr
 * @param outChatPanel 输出参数，返回聊天面板指针
 * @return 初始化成功返回 true，失败返回 false
 */
bool initializeAIIntegration(MainWindow *mainWindow,
                             AiConnectionManager *&outConnMgr,
                             ChatPanel *&outChatPanel);

/**
 * @brief 处理用户发送的消息
 *
 * 当用户在聊天面板中输入消息并点击发送时，
 * 此函数会被调用来处理消息并转发给 AI Agent。
 *
 * @param mainWindow 主窗口指针（用于获取上下文）
 * @param connMgr 连接管理器指针（用于发送请求）
 * @param chatPanel 聊天面板指针
 * @param message 用户输入的消息内容
 * @param includeContext 是否包含当前 IDE 上下文
 *
 * @note 此函数通常在 ChatPanel::userMessageSent 信号中调用
 */
void handleUserMessage(MainWindow *mainWindow,
                       AiConnectionManager *connMgr,
                       ChatPanel *chatPanel,
                       const QString &message,
                       bool includeContext);

/**
 * @brief 在菜单栏中添加 AI 相关菜单项
 *
 * 创建 "AI" 菜单，包含以下子菜单：
 * - 显示对话面板（切换显示/隐藏）
 * - 清空对话
 * - 自动修复错误（开启/关闭）
 * - 连接状态（只读显示）
 *
 * @param mainWindow 主窗口指针
 * @param chatPanel 聊天面板指针
 *
 * @note 通常在 MainWindow 构造函数中调用
 */
void setupAIMenu(MainWindow *mainWindow,
                 ChatPanel *chatPanel);

/**
 * @brief 保存聊天面板的状态到设置文件
 *
 * 保存的内容包括：
 * - 面板是否可见
 * - 面板的几何位置
 *
 * @param chatPanel 聊天面板指针
 *
 * @note 通常在 MainWindow 的 closeEvent 中调用
 */
void saveChatPanelState(ChatPanel *chatPanel);

/**
 * @brief 从设置文件恢复聊天面板的状态
 *
 * 恢复的内容包括：
 * - 面板是否可见
 * - 面板的几何位置
 *
 * @param chatPanel 聊天面板指针
 *
 * @note 通常在 MainWindow 构造函数或初始化函数中调用
 */
void restoreChatPanelState(ChatPanel *chatPanel);

} // namespace SprintIDE

#endif // AI_INTEGRATION_H