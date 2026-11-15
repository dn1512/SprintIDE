// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QWidget>
#include <QList>

#include "terminal/terminal_output_parser.h"

class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QToolButton;

namespace SprintIDE {

class TerminalOutputOverlay : public QWidget
{
    Q_OBJECT
public:
    explicit TerminalOutputOverlay(QWidget* parent = nullptr);
    ~TerminalOutputOverlay() override;

    void showResult(const ParsedOutputResult& result);
    void dismiss();
    bool isEmpty() const;

signals:
    void actionTriggered(const QString& action);

protected:
    void keyPressEvent(QKeyEvent* e) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void setupUI();
    void applyTheme();
    void reposition();
    void onItemActivated(QListWidgetItem* item);
    void renderItems(const QString& filter);   // ★ C3：按 filter 重建列表

    QLabel*       m_titleLabel    = nullptr;
    QLabel*       m_subtitleLabel = nullptr;
    QLineEdit*    m_searchEdit    = nullptr;
    QToolButton*  m_closeButton   = nullptr;
    QListWidget*  m_list          = nullptr;

    QList<ParsedOutputItem> m_allItems;   // ★ C3：原始完整列表（用于过滤）
};

} // namespace SprintIDE