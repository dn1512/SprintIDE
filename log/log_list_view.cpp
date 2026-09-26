#include "log_list_view.h"
#include "log_list_model.h"
#include "core/log_buffer.h"
#include <QKeyEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QApplication>
#include <QClipboard>
#include <QScrollBar>
#include <QDateTime>

namespace SprintIDE {

LogListView::LogListView(QWidget* parent)
    : QListView(parent)
{
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setUniformItemSizes(true);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setWordWrap(false);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    connect(this, &QListView::doubleClicked,
            this, &LogListView::onDoubleClicked);
}

void LogListView::setLogModel(LogListModel* m)
{
    m_logModel = m;
    QListView::setModel(m);
}

// ============================================================
//  Ctrl+C 复制选中行
//  —— 这就是你"快捷键复制不行"的终极答案：在视图里自己处理，
//     不依赖 MainWindow 的 QShortcut，也不依赖 QTextEdit 的焦点。
// ============================================================
void LogListView::keyPressEvent(QKeyEvent* e)
{
    const bool isCopy = (e->matches(QKeySequence::Copy))
                        || (e->key() == Qt::Key_C
                            && (e->modifiers() & Qt::ControlModifier));
    if (isCopy) {
        copySelectionToClipboard();
        return;
    }
    if (e->matches(QKeySequence::SelectAll)) {
        selectAll();
        return;
    }
    QListView::keyPressEvent(e);
}

void LogListView::copySelectionToClipboard()
{
    const QModelIndexList rows = selectionModel()->selectedRows();
    if (rows.isEmpty()) return;

    // 按视图行号升序
    QList<int> sorted;
    sorted.reserve(rows.size());
    for (const QModelIndex& i : rows) sorted.append(i.row());
    std::sort(sorted.begin(), sorted.end());

    QStringList lines;
    lines.reserve(sorted.size());
    for (int r : sorted) {
        const LogEntry& e = m_logModel->entryAt(r);
        QString line;
        if (e.timestamp > 0) {
            line += QDateTime::fromMSecsSinceEpoch(e.timestamp)
            .toString("hh:mm:ss") + "  ";
        }
        if (!e.source.isEmpty())
            line += QString("[%1] ").arg(e.source);
        line += e.message;
        lines << line;
    }
    QApplication::clipboard()->setText(lines.join('\n'));
}

void LogListView::copyAllToClipboard()
{
    if (!m_logModel) return;
    QStringList lines;
    lines.reserve(m_logModel->viewCount());
    for (int r = 0; r < m_logModel->viewCount(); ++r) {
        const LogEntry& e = m_logModel->entryAt(r);
        QString line;
        if (e.timestamp > 0) {
            line += QDateTime::fromMSecsSinceEpoch(e.timestamp)
            .toString("hh:mm:ss") + "  ";
        }
        if (!e.source.isEmpty())
            line += QString("[%1] ").arg(e.source);
        line += e.message;
        lines << line;
    }
    QApplication::clipboard()->setText(lines.join('\n'));
}

void LogListView::contextMenuEvent(QContextMenuEvent* e)
{
    QMenu menu(this);

    QAction* copySel = menu.addAction(tr("复制选中(&C)"));
    copySel->setShortcut(QKeySequence::Copy);
    copySel->setEnabled(!selectionModel()->selectedRows().isEmpty());
    connect(copySel, &QAction::triggered, this, &LogListView::copySelectionToClipboard);

    QAction* copyAll = menu.addAction(tr("复制全部(&A)"));
    connect(copyAll, &QAction::triggered, this, &LogListView::copyAllToClipboard);

    menu.addSeparator();
    QAction* selAll = menu.addAction(tr("全选(&S)"));
    connect(selAll, &QAction::triggered, this, &LogListView::selectAll);

    menu.exec(e->globalPos());
}

void LogListView::onDoubleClicked(const QModelIndex& idx)
{
    if (!idx.isValid() || !m_logModel) return;
    const LogEntry& e = m_logModel->entryAt(idx.row());
    if (!e.filePath.isEmpty() && e.lineNumber > 0) {
        emit jumpRequested(e.filePath, e.lineNumber, e.column);
    }
}

} // namespace SprintIDE
