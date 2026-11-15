// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QTabWidget>
#include <QString>

class QToolButton;

namespace SprintIDE {

class TerminalPanel;

// ---------------------------------------------------------------------------
// TerminalTabManager
//
//   终端多 Tab 容器。职责：
//     - 管理一组 TerminalPanel
//     - 提供"新建 / 关闭 / 切换"操作
//     - Tab 标题自动跟随 shell 的 OSC title（如果 shell 没设置，用"终端 N"）
//     - 转发当前 TerminalPanel 的 openFileAtLine 信号给 MainWindow
//
//   不负责：
//     - 终端内部功能（由 TerminalPanel 负责）
//     - SSH 连接管理（直接输入 ssh 命令即可）
//     - 会话持久化（不做）
// ---------------------------------------------------------------------------
class TerminalTabManager : public QTabWidget
{
    Q_OBJECT
public:
    explicit TerminalTabManager(QWidget* parent = nullptr);
    ~TerminalTabManager() override;

    // 新建一个本地终端，并切换为当前 tab
    // workDir 为空时用当前工作目录
    TerminalPanel* createTerminal(const QString& workDir = QString());

    // 当前 tab 的 TerminalPanel（没有 tab 时返回 nullptr）
    TerminalPanel* currentTerminal() const;

    // 关闭所有 tab（用于 MainWindow 退出前主动清理）
    void closeAll();

signals:
    // 当前 tab 的 TerminalPanel 触发了 openFileAtLine 时转发
    // MainWindow 接到这个信号后应打开对应文件并跳转到行/列
    void openFileAtLine(const QString& path, int line, int column);

private slots:
    void onTabCloseRequested(int index);
    void onNewTabRequested();
    void onShellTitleChanged(TerminalPanel* panel, const QString& title);

    // Tab 右键菜单
    void onTabBarContextMenu(const QPoint& pos);

    // 停止按钮
    void onStopButtonClicked();

private:
    void updateTabCountUI();

    //  更新停止按钮 enable 状态（根据当前 Tab 的 shell 是否在运行）
    void updateStopButtonState();

    // 用户重命名过的 Tab 不再被 shell title 覆盖
    QSet<QWidget*> m_userRenamedTabs;

    QToolButton* m_stopButton   = nullptr;   // 停止按钮
    QToolButton* m_newTabButton = nullptr;
    int          m_nextTerminalNumber = 1;
};

} // namespace SprintIDE