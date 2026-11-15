// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QWidget>

class QLineEdit;
class QToolButton;
class QLabel;

namespace SprintIDE {

// ---------------------------------------------------------------------------
// TerminalSearchBar
//
//   终端搜索栏，仿 VSCode 的紧凑样式。
//   浮在 TerminalPanel 顶部一行，由 Ctrl+F 呼出、Esc 关闭。
// ---------------------------------------------------------------------------
class TerminalSearchBar : public QWidget
{
    Q_OBJECT
public:
    explicit TerminalSearchBar(QWidget* parent = nullptr);

    // 显示 + 聚焦搜索框；如果传了 initialText 会填进搜索框
    void showAndFocus(const QString& initialText = QString());

    // 更新匹配计数显示（current 从 0 开始，-1 表示无当前项）
    void setMatchInfo(int current, int total);

    QString query() const;

signals:
    void searchChanged(const QString& query, bool caseSensitive);
    void findNext();
    void findPrev();
    void closeRequested();

protected:
    void keyPressEvent(QKeyEvent* e) override;
    void showEvent(QShowEvent* e) override;

private:
    void applyTheme();
    void onTextChanged();
    void onCaseToggled(bool on);

    QLineEdit*   m_edit       = nullptr;
    QToolButton* m_caseBtn    = nullptr;
    QToolButton* m_prevBtn    = nullptr;
    QToolButton* m_nextBtn    = nullptr;
    QLabel*      m_countLabel = nullptr;
    QToolButton* m_closeBtn   = nullptr;
};

} // namespace SprintIDE