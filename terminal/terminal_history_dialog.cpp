// SPDX-License-Identifier: GPL-3.0-or-later
#include "terminal_history_dialog.h"
#include "terminal_history_store.h"

#include <QComboBox>
#include <QDateTime>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

#include "theme_manager.h"

namespace SprintIDE {

TerminalHistoryDialog::TerminalHistoryDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("终端命令历史"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    resize(900, 560);

    // ---------- 顶部工具条 ----------
    auto* topBar = new QHBoxLayout();
    topBar->setContentsMargins(8, 8, 8, 4);
    topBar->setSpacing(6);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("搜索命令（模糊匹配）..."));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setMinimumWidth(320);
    topBar->addWidget(m_searchEdit, 1);

    m_projectCombo = new QComboBox(this);
    m_projectCombo->setMinimumWidth(200);
    topBar->addWidget(m_projectCombo);

    m_timeCombo = new QComboBox(this);
    m_timeCombo->addItem(tr("全部时间"), qint64(0));
    m_timeCombo->addItem(tr("最近 1 天"), qint64(1));
    m_timeCombo->addItem(tr("最近 7 天"), qint64(7));
    m_timeCombo->addItem(tr("最近 30 天"), qint64(30));
    topBar->addWidget(m_timeCombo);

    m_refreshBtn = new QPushButton(tr("刷新"), this);
    topBar->addWidget(m_refreshBtn);

    m_clearBtn = new QPushButton(tr("清空全部"), this);
    topBar->addWidget(m_clearBtn);

    // ---------- 中间结果表 ----------
    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({
        tr("命令"), tr("时间"), tr("项目 / 目录"), tr("耗时")
    });
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->setShowGrid(false);

    // ---------- 底部说明 ----------
    m_countLabel = new QLabel(this);
    auto* hintLabel = new QLabel(
        tr("双击：回填到终端（用户按回车执行）    Enter：回填并执行    Esc：关闭"),
        this);
    hintLabel->setStyleSheet(QStringLiteral("color: palette(mid);"));

    auto* bottomBar = new QHBoxLayout();
    bottomBar->setContentsMargins(8, 4, 8, 8);
    bottomBar->addWidget(m_countLabel);
    bottomBar->addStretch();
    bottomBar->addWidget(hintLabel);

    // ---------- 总布局 ----------
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addLayout(topBar);
    layout->addWidget(m_table, 1);
    layout->addLayout(bottomBar);

    // ---------- 信号 ----------
    connect(m_searchEdit,   &QLineEdit::textChanged,
            this, &TerminalHistoryDialog::onSearchChanged);
    connect(m_projectCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TerminalHistoryDialog::onFilterChanged);
    connect(m_timeCombo,    QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TerminalHistoryDialog::onFilterChanged);
    connect(m_table, &QTableWidget::cellDoubleClicked,
            this, &TerminalHistoryDialog::onRowActivated);
    connect(m_refreshBtn, &QPushButton::clicked,
            this, &TerminalHistoryDialog::onRefreshClicked);
    connect(m_clearBtn, &QPushButton::clicked,
            this, &TerminalHistoryDialog::onClearAllClicked);

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &TerminalHistoryDialog::applyTheme);

    rebuildProjectFilter();
    performSearch();
}

TerminalHistoryDialog::~TerminalHistoryDialog() = default;

void TerminalHistoryDialog::setCurrentProject(const QString& projectPath)
{
    m_currentProject = projectPath;
    rebuildProjectFilter();
    performSearch();
}

void TerminalHistoryDialog::showEvent(QShowEvent* e)
{
    QDialog::showEvent(e);
    if (m_searchEdit) m_searchEdit->setFocus();
}

void TerminalHistoryDialog::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape) {
        reject();
        return;
    }
    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        // 如果焦点在 table，Enter 回填并执行
        const int row = m_table->currentRow();
        if (row >= 0) {
            const QString cmd = selectedCommandAt(row);
            if (!cmd.isEmpty()) {
                emit commandSelected(cmd, /*autoExecute=*/true);
                accept();
                return;
            }
        }
        // 焦点在搜索框时，Enter 执行当前最高优先匹配
        if (!m_table->selectedItems().isEmpty()) {
            const int r = m_table->currentRow();
            const QString cmd = selectedCommandAt(r);
            if (!cmd.isEmpty()) {
                emit commandSelected(cmd, /*autoExecute=*/true);
                accept();
                return;
            }
        }
    }
    QDialog::keyPressEvent(e);
}

void TerminalHistoryDialog::onSearchChanged()
{
    performSearch();
}

void TerminalHistoryDialog::onFilterChanged()
{
    performSearch();
}

void TerminalHistoryDialog::onRowActivated(int row, int /*column*/)
{
    const QString cmd = selectedCommandAt(row);
    if (cmd.isEmpty()) return;
    emit commandSelected(cmd, /*autoExecute=*/false);
    accept();
}

void TerminalHistoryDialog::onRefreshClicked()
{
    rebuildProjectFilter();
    performSearch();
}

void TerminalHistoryDialog::onClearAllClicked()
{
    auto* store = TerminalHistoryStore::instance();
    if (store->count() == 0) return;

    // 确认
    if (QMessageBox::question(this, tr("清空历史"),
            tr("确定要清空所有项目的命令历史吗？此操作不可恢复。"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) {
        return;
    }
    store->clearAll();
    performSearch();
}

void TerminalHistoryDialog::rebuildProjectFilter()
{
    m_projectCombo->blockSignals(true);
    m_projectCombo->clear();

    m_projectCombo->addItem(tr("所有项目"), QString());

    auto* store = TerminalHistoryStore::instance();
    const QStringList projects = store->distinctProjects();
    for (const QString& p : projects) {
        m_projectCombo->addItem(p, p);
    }

    // 如果当前项目存在，默认选中
    if (!m_currentProject.isEmpty()) {
        int idx = m_projectCombo->findData(m_currentProject);
        if (idx < 0) {
            m_projectCombo->addItem(m_currentProject, m_currentProject);
            idx = m_projectCombo->count() - 1;
        }
        m_projectCombo->setCurrentIndex(idx);
    }

    m_projectCombo->blockSignals(false);
}

void TerminalHistoryDialog::performSearch()
{
    if (!m_table) return;

    const QString query = m_searchEdit ? m_searchEdit->text().trimmed() : QString();
    const QString project = m_projectCombo ? m_projectCombo->currentData().toString() : QString();

    qint64 sinceMs = 0;
    if (m_timeCombo) {
        const qint64 days = m_timeCombo->currentData().toLongLong();
        if (days > 0) {
            sinceMs = QDateTime::currentMSecsSinceEpoch()
                    - days * 24LL * 3600LL * 1000LL;
        }
    }

    auto* store = TerminalHistoryStore::instance();
    const QList<TerminalHistoryEntry> entries =
        store->search(query, project, sinceMs, 500);

    m_table->setRowCount(0);
    m_table->setRowCount(entries.size());

    for (int i = 0; i < entries.size(); ++i) {
        const auto& e = entries[i];

        auto* cmdItem = new QTableWidgetItem(e.command);
        cmdItem->setToolTip(e.command);
        m_table->setItem(i, 0, cmdItem);

        const QString timeStr = QDateTime::fromMSecsSinceEpoch(e.startedAt)
                                    .toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
        m_table->setItem(i, 1, new QTableWidgetItem(timeStr));

        const QString locStr = e.projectPath.isEmpty() ? e.cwd : e.projectPath;
        auto* locItem = new QTableWidgetItem(locStr);
        locItem->setToolTip(locStr);
        m_table->setItem(i, 2, locItem);

        // 时长显示：<1s → "0.3s"，<60s → "12s"，<60min → "3m 5s"，其他 → "1h 2m"
        QString durStr = QStringLiteral("—");
        if (e.durationMs >= 0) {
            if (e.durationMs < 1000) {
                durStr = QStringLiteral("%1s").arg(e.durationMs / 1000.0, 0, 'f', 1);
            } else if (e.durationMs < 60000) {
                durStr = QStringLiteral("%1s").arg(e.durationMs / 1000);
            } else if (e.durationMs < 3600000) {
                const qint64 m = e.durationMs / 60000;
                const qint64 s = (e.durationMs % 60000) / 1000;
                durStr = QStringLiteral("%1m %2s").arg(m).arg(s);
            } else {
                const qint64 h = e.durationMs / 3600000;
                const qint64 m = (e.durationMs % 3600000) / 60000;
                durStr = QStringLiteral("%1h %2m").arg(h).arg(m);
            }
        }
        m_table->setItem(i, 3, new QTableWidgetItem(durStr));
    }

    if (!entries.isEmpty()) {
        m_table->selectRow(0);
    }

    if (m_countLabel) {
        m_countLabel->setText(tr("共 %1 条（共历史 %2 条）")
                                  .arg(entries.size())
                                  .arg(store->count()));
    }
}

QString TerminalHistoryDialog::selectedCommandAt(int row) const
{
    if (!m_table || row < 0 || row >= m_table->rowCount()) return QString();
    auto* item = m_table->item(row, 0);
    return item ? item->text() : QString();
}

void TerminalHistoryDialog::applyTheme()
{
    auto& tm = ThemeManager::instance();
    const QString bg = tm.colorString(ThemeManager::UIElement::MainWindowBackground);
    const QString fg = tm.colorString(ThemeManager::UIElement::TextColor);
    const QString border = tm.colorString(ThemeManager::UIElement::BorderColor);

    setStyleSheet(QStringLiteral(
        "QDialog { background: %1; }"
        "QTableWidget { background: %1; color: %2; border: 1px solid %3; }"
        "QTableWidget::item:selected { background: %4; }"
        "QLineEdit { background: %1; color: %2; border: 1px solid %3;"
        "  border-radius: 3px; padding: 3px 6px; }"
        "QComboBox { background: %1; color: %2; border: 1px solid %3;"
        "  border-radius: 3px; padding: 2px 6px; }"
        "QPushButton { background: %1; color: %2; border: 1px solid %3;"
        "  border-radius: 3px; padding: 3px 12px; }"
        "QLabel { color: %2; }")
        .arg(bg, fg, border,
             tm.colorString(ThemeManager::UIElement::TreeSelectedBackground)));
}

} // namespace SprintIDE