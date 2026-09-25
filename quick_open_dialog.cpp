#include "quick_open_dialog.h"

#include <QLineEdit>
#include <QListWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QFileInfo>
#include <QDir>
#include <QKeyEvent>
#include <QShowEvent>
#include <QDebug>
#include <algorithm>

namespace SprintIDE {

QuickOpenDialog::QuickOpenDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("快速打开文件"));
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setModal(true);
    setMinimumWidth(680);
    setMinimumHeight(420);

    // 无边框窗口需要自己画边框
    setStyleSheet(
        "QuickOpenDialog {"
        "  border: 1px solid #5A5A5A;"
        "  background-color: #2D2D2D;"
        "}"
    );

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(
        tr("输入文件名进行过滤（支持路径片段，如 src/main.cpp）"));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->installEventFilter(this);   // 拦截 ↑↓ / Esc
    layout->addWidget(m_searchEdit);

    m_listWidget = new QListWidget(this);
    m_listWidget->setUniformItemSizes(true);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listWidget->setAlternatingRowColors(false);
    m_listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    layout->addWidget(m_listWidget, 1);

    m_hintLabel = new QLabel(tr("↑↓ 选择 · Enter 打开 · Esc 关闭"), this);
    m_hintLabel->setStyleSheet("color: #888; padding: 2px;");
    layout->addWidget(m_hintLabel);

    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &QuickOpenDialog::onTextChanged);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, [this]() {
        if (auto* item = m_listWidget->currentItem()) {
            onItemActivated(item);
        }
    });
    connect(m_listWidget, &QListWidget::itemDoubleClicked,
            this, &QuickOpenDialog::onItemActivated);
    // 列表里按 Enter 也打开（焦点可能在列表上）
    connect(m_listWidget, &QListWidget::itemActivated,
            this, &QuickOpenDialog::onItemActivated);
}

void QuickOpenDialog::setFileList(const QStringList& absolutePaths,
                                  const QString& projectRoot)
{
    m_allFiles    = absolutePaths;
    m_projectRoot = projectRoot;
    refreshList(QString());   // 初始显示全部
}

QString QuickOpenDialog::selectedFilePath() const
{
    if (auto* item = m_listWidget->currentItem()) {
        return item->data(Qt::UserRole).toString();
    }
    return QString();
}

void QuickOpenDialog::showEvent(QShowEvent* e)
{
    // 首次显示时居中到父窗口
    if (!m_centered && parentWidget()) {
        const QPoint center = parentWidget()->mapToGlobal(
            parentWidget()->rect().center());
        move(center.x() - width() / 2,
             center.y() - height() / 2);
        m_centered = true;
    }
    QDialog::showEvent(e);

    m_searchEdit->setFocus();
    m_searchEdit->selectAll();
}

bool QuickOpenDialog::eventFilter(QObject* watched, QEvent* event)
{
    // 输入框里的 ↑↓ 用于列表导航；Esc 关闭
    if (watched == m_searchEdit && event->type() == QEvent::KeyPress) {
        auto* ke = static_cast<QKeyEvent*>(event);
        switch (ke->key()) {
        case Qt::Key_Down: {
            const int row = m_listWidget->currentRow();
            if (row + 1 < m_listWidget->count())
                m_listWidget->setCurrentRow(row + 1);
            return true;
        }
        case Qt::Key_Up: {
            const int row = m_listWidget->currentRow();
            if (row > 0)
                m_listWidget->setCurrentRow(row - 1);
            return true;
        }
        case Qt::Key_Escape:
            reject();
            return true;
        default:
            break;
        }
    }
    return QDialog::eventFilter(watched, event);
}

void QuickOpenDialog::onTextChanged(const QString& text)
{
    refreshList(text);
}

void QuickOpenDialog::onItemActivated(QListWidgetItem* item)
{
    if (!item) return;
    emit fileChosen(item->data(Qt::UserRole).toString());
    accept();
}

int QuickOpenDialog::scoreMatch(const QString& absPath,
                                const QString& fileName,
                                const QString& query) const
{
    if (query.isEmpty()) return 1;

    const QString q  = query.toLower();
    const QString fn = fileName.toLower();
    const QString fp = absPath.toLower();

    if (fn == q)          return 1000;   // 文件名完全匹配
    if (fn.startsWith(q)) return 900;    // 文件名前缀匹配
    if (fn.contains(q))   return 700;    // 文件名子串匹配
    if (fp.contains(q))   return 300;    // 路径任意位置匹配
    return 0;                            // 不匹配
}

void QuickOpenDialog::refreshList(const QString& query)
{
    m_listWidget->clear();

    struct Hit { QString path; QString display; int score; };
    QList<Hit> hits;
    hits.reserve(m_allFiles.size());

    for (const QString& abs : m_allFiles) {
        const QString fileName = QFileInfo(abs).fileName();
        const int s = scoreMatch(abs, fileName, query);
        if (s <= 0) continue;

        // 显示相对项目根的路径（更易读）
        QString display = abs;
        if (!m_projectRoot.isEmpty()) {
            const QString rel = QDir(m_projectRoot).relativeFilePath(abs);
            if (!rel.startsWith("..")) display = rel;
        }
        hits.append({abs, display, s});
    }

    // 分数高 → 路径短（越短越可能相关）
    std::sort(hits.begin(), hits.end(), [](const Hit& a, const Hit& b) {
        if (a.score != b.score) return a.score > b.score;
        return a.display.size() < b.display.size();
    });

    constexpr int kMaxShow = 200;
    const int n = qMin(hits.size(), kMaxShow);
    for (int i = 0; i < n; ++i) {
        auto* item = new QListWidgetItem(hits[i].display, m_listWidget);
        item->setData(Qt::UserRole, hits[i].path);
        item->setToolTip(hits[i].path);
    }

    if (m_listWidget->count() > 0) {
        m_listWidget->setCurrentRow(0);
    }

    if (hits.size() > kMaxShow) {
        m_hintLabel->setText(
            tr("共 %1 个匹配（显示前 %2） · ↑↓ 选择 · Enter 打开 · Esc 关闭")
                .arg(hits.size()).arg(kMaxShow));
    } else {
        m_hintLabel->setText(
            tr("共 %1 个匹配 · ↑↓ 选择 · Enter 打开 · Esc 关闭")
                .arg(hits.size()));
    }
}

} // namespace SprintIDE
