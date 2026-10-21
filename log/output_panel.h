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
#ifndef OUTPUT_PANEL_H
#define OUTPUT_PANEL_H

#include <QWidget>
#include "core/log_buffer.h"

QT_BEGIN_NAMESPACE
class QLineEdit;
class QCheckBox;
class QToolButton;
class QLabel;
QT_END_NAMESPACE

namespace SprintIDE {

class LogListModel;
class LogListView;

class OutputPanel : public QWidget
{
    Q_OBJECT
public:
    explicit OutputPanel(QWidget* parent = nullptr);
    ~OutputPanel() override;

    // ---- 写入接口 ----
    void appendEntry(const LogEntry& e);
    void appendEntries(const QList<LogEntry>& entries);

    // ---- 兼容旧 API（临时过渡） ----
    void appendPlainText(const QString& text);
    void appendHtml(const QString& html);   // 简单剥离标签后当普通文本

    // ---- 操作 ----
    void clear();
    void setAutoScroll(bool on);
    /// 主题切换时由 MainWindow::applyCurrentTheme() 调用
    void applyTheme();

    /// 设置 AI 是否可用（转发给内部 LogListView）
    void setAIAvailable(bool available);

    /// 让输出面板获得键盘焦点（默认给搜索框，用户 Tab 可到列表）
    void focusPanel();

protected:
    /// 拦截 Tab / Shift+Tab，在面板内按固定顺序切换焦点
    bool eventFilter(QObject* watched, QEvent* event) override;

signals:
    void jumpRequested(const QString& file, int line, int column);
    void cleared();
    /// 用户请求 AI 解释选中的错误（转发自 LogListView）
    void aiExplainErrorRequested(const QString& errorText);

private slots:
    void onFilterChanged();
    void onSearchChanged(const QString& text);
    void onCountsChanged(int errs, int warns, int infos, int visible);
    void onScrollBarValueChanged(int value);

private:
    void buildUi();
    void applyAutoScrollIfNeeded();

    LogListModel* m_model  = nullptr;
    LogListView*  m_view   = nullptr;

    QLineEdit*    m_search      = nullptr;
    QToolButton*  m_chkErr      = nullptr;   // 用 QToolButton 替代 QCheckBox
    QToolButton*  m_chkWarn     = nullptr;
    QToolButton*  m_chkInfo     = nullptr;
    QToolButton*  m_btnAutoScroll = nullptr;
    QToolButton*  m_btnClear     = nullptr;
    QLabel*       m_summary      = nullptr;

    bool m_autoScroll   = true;
    bool m_stickToBottom = true;   // 用户手动上滚后变 false
};

} // namespace SprintIDE

#endif // OUTPUT_PANEL_H
