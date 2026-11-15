// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QDialog>

class QLineEdit;
class QComboBox;
class QTableWidget;
class QLabel;
class QPushButton;

namespace SprintIDE {

class TerminalHistoryDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TerminalHistoryDialog(QWidget* parent = nullptr);
    ~TerminalHistoryDialog() override;

    void setCurrentProject(const QString& projectPath);

signals:
    void commandSelected(const QString& command, bool autoExecute);

protected:
    void keyPressEvent(QKeyEvent* e) override;
    void showEvent(QShowEvent* e) override;

private slots:
    void onSearchChanged();
    void onFilterChanged();
    void onRowActivated(int row, int column);
    void onRefreshClicked();
    void onClearAllClicked();

private:
    void rebuildProjectFilter();
    void performSearch();
    void applyTheme();
    QString selectedCommandAt(int row) const;

    QLineEdit*    m_searchEdit   = nullptr;
    QComboBox*    m_projectCombo = nullptr;
    QComboBox*    m_timeCombo    = nullptr;
    QTableWidget* m_table        = nullptr;
    QLabel*       m_countLabel   = nullptr;
    QPushButton*  m_refreshBtn   = nullptr;
    QPushButton*  m_clearBtn     = nullptr;

    QString m_currentProject;
};

} // namespace SprintIDE