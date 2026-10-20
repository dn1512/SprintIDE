#include "search_in_files_dialog.h"

#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QTimer>
#include <QMessageBox>
#include <QDir>
#include <QShortcut>

namespace SprintIDE {

SearchInFilesDialog::SearchInFilesDialog(const QString& rootPath, QWidget* parent)
    : QDialog(parent)
    , m_rootPath(rootPath)
{
    setWindowTitle(tr("在文件中查找"));
    resize(880, 600);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    buildUi();
}

SearchInFilesDialog::~SearchInFilesDialog() = default;

void SearchInFilesDialog::buildUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // ---------- 搜索行 ----------
    auto* searchRow = new QHBoxLayout;
    searchRow->setSpacing(6);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("输入查找内容... (Esc 清空)"));
    // 不再启用 clearButton —— 见 OutputPanel 同类问题

    // Esc 清空（焦点在搜索框时）
    {
        auto* escSc = new QShortcut(QKeySequence(Qt::Key_Escape), m_searchEdit);
        escSc->setContext(Qt::WidgetShortcut);
        connect(escSc, &QShortcut::activated, this, [this]() {
                if (m_searchEdit && !m_searchEdit->text().isEmpty()) {
                    m_searchEdit->clear();
                }
            });
    }

    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &SearchInFilesDialog::onSearchTextChanged);
    connect(m_searchEdit, &QLineEdit::returnPressed,
            this, &SearchInFilesDialog::onSearchClicked);
    searchRow->addWidget(m_searchEdit, 1);

    m_searchButton = new QPushButton(tr("查找"), this);
    m_searchButton->setEnabled(false);
    connect(m_searchButton, &QPushButton::clicked,
            this, &SearchInFilesDialog::onSearchClicked);
    searchRow->addWidget(m_searchButton);

    m_cancelButton = new QPushButton(tr("停止"), this);
    m_cancelButton->setEnabled(false);
    connect(m_cancelButton, &QPushButton::clicked,
            this, &SearchInFilesDialog::onCancelClicked);
    searchRow->addWidget(m_cancelButton);

    mainLayout->addLayout(searchRow);

    // ---------- 选项行 ----------
    auto* optRow = new QHBoxLayout;
    optRow->setSpacing(12);

    m_caseSensitive = new QCheckBox(tr("大小写敏感"), this);
    m_wholeWord     = new QCheckBox(tr("全字匹配"),   this);
    m_useRegex      = new QCheckBox(tr("正则表达式"), this);

    connect(m_useRegex, &QCheckBox::toggled,
            this, &SearchInFilesDialog::onRegexToggled);

    optRow->addWidget(m_caseSensitive);
    optRow->addWidget(m_wholeWord);
    optRow->addWidget(m_useRegex);
    optRow->addStretch();
    mainLayout->addLayout(optRow);

    // ---------- 结果树 ----------
    m_resultTree = new QTreeWidget(this);
    m_resultTree->setColumnCount(1);
    m_resultTree->setHeaderLabels({tr("结果")});
    m_resultTree->setRootIsDecorated(true);
    m_resultTree->setUniformRowHeights(true);
    m_resultTree->setAlternatingRowColors(false);
    m_resultTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_resultTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultTree->header()->setStretchLastSection(true);
    connect(m_resultTree, &QTreeWidget::itemActivated,
            this, &SearchInFilesDialog::onItemActivated);
    mainLayout->addWidget(m_resultTree, 1);

    // ---------- 底部状态 ----------
    auto* bottomRow = new QHBoxLayout;

    m_statusLabel = new QLabel(tr("就绪"), this);
    bottomRow->addWidget(m_statusLabel, 1);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setMaximumWidth(220);
    m_progressBar->setVisible(false);
    bottomRow->addWidget(m_progressBar);

    m_closeButton = new QPushButton(tr("关闭"), this);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::close);
    bottomRow->addWidget(m_closeButton);

    mainLayout->addLayout(bottomRow);
}

void SearchInFilesDialog::setRootPath(const QString& path)
{
    if (m_rootPath == path) return;
    m_rootPath = path;
    clearResults();
    m_fileList.clear();
    m_statusLabel->setText(tr("就绪"));
}

void SearchInFilesDialog::setFileList(const QStringList& files)
{
    m_fileList = files;
    if (!m_searching) {
        m_statusLabel->setText(tr("待搜索文件：%1 个").arg(files.size()));
    }
}

void SearchInFilesDialog::setSearchText(const QString& text)
{
    m_searchEdit->setText(text);
    m_searchEdit->selectAll();
    m_searchEdit->setFocus();
}

QString SearchInFilesDialog::searchText() const
{
    return m_searchEdit ? m_searchEdit->text() : QString();
}

void SearchInFilesDialog::onSearchTextChanged(const QString& text)
{
    if (m_searching) return;
    m_searchButton->setEnabled(!text.isEmpty());
}

void SearchInFilesDialog::onRegexToggled(bool checked)
{
    // 正则模式下"全字匹配"没意义（用户可以用 \b）
    m_wholeWord->setEnabled(!checked);
    if (checked) m_wholeWord->setChecked(false);
}

void SearchInFilesDialog::onSearchClicked()
{
    if (m_searching) return;

    const QString needle = m_searchEdit->text();
    if (needle.isEmpty()) return;
    if (m_fileList.isEmpty()) {
        QMessageBox::information(this, tr("在文件中查找"),
                                 tr("没有可搜索的文件。请先打开项目。"));
        return;
    }

    m_caseSensitiveFlag = m_caseSensitive->isChecked();
    m_wholeWordFlag     = m_wholeWord->isChecked();
    m_useRegexFlag      = m_useRegex->isChecked();

    if (m_useRegexFlag) {
        QRegularExpression::PatternOptions opts = QRegularExpression::NoPatternOption;
        if (!m_caseSensitiveFlag)
            opts |= QRegularExpression::CaseInsensitiveOption;
        m_regex = QRegularExpression(needle, opts);
        if (!m_regex.isValid()) {
            QMessageBox::warning(this, tr("正则表达式错误"),
                                 m_regex.errorString());
            return;
        }
    }
    m_needle = needle;

    clearResults();
    m_currentIndex = 0;
    m_totalMatches = 0;
    m_searching    = true;

    // UI 进入搜索态
    m_searchButton->setEnabled(false);
    m_cancelButton->setEnabled(true);
    m_searchEdit->setEnabled(false);
    m_caseSensitive->setEnabled(false);
    m_wholeWord->setEnabled(false);
    m_useRegex->setEnabled(false);
    m_progressBar->setVisible(true);
    m_progressBar->setMaximum(m_fileList.size());
    m_progressBar->setValue(0);

    QTimer::singleShot(0, this, &SearchInFilesDialog::processNextBatch);
}

void SearchInFilesDialog::onCancelClicked()
{
    if (!m_searching) return;
    m_searching = false;
    m_statusLabel->setText(tr("已停止，已找到 %1 处匹配").arg(m_totalMatches));
    m_progressBar->setVisible(false);
    restoreInputUi();
}

void SearchInFilesDialog::restoreInputUi()
{
    m_searchButton->setEnabled(!m_searchEdit->text().isEmpty());
    m_cancelButton->setEnabled(false);
    m_searchEdit->setEnabled(true);
    m_caseSensitive->setEnabled(true);
    m_wholeWord->setEnabled(!m_useRegex->isChecked());
    m_useRegex->setEnabled(true);
}

void SearchInFilesDialog::clearResults()
{
    m_resultTree->clear();
    m_lastFilePath.clear();
    m_lastFileItem = nullptr;
}

int SearchInFilesDialog::findInLine(const QString& line) const
{
    if (line.isEmpty()) return -1;

    if (m_useRegexFlag) {
        const auto m = m_regex.match(line);
        return m.hasMatch() ? (m.capturedStart() + 1) : -1;
    }

    const Qt::CaseSensitivity cs =
        m_caseSensitiveFlag ? Qt::CaseSensitive : Qt::CaseInsensitive;

    int start = 0;
    while (true) {
        const int idx = line.indexOf(m_needle, start, cs);
        if (idx < 0) return -1;

        if (!m_wholeWordFlag) return idx + 1;

        // 全字匹配边界检查
        const bool leftOk  = (idx == 0) || !line[idx - 1].isLetterOrNumber();
        const int rightIdx = idx + m_needle.length();
        const bool rightOk = (rightIdx >= line.length()) || !line[rightIdx].isLetterOrNumber();

        if (leftOk && rightOk) return idx + 1;

        start = idx + 1;
    }
}

void SearchInFilesDialog::processNextBatch()
{
    if (!m_searching) return;

    const int total = m_fileList.size();
    int processed = 0;

    while (m_currentIndex < total
           && processed < kBatchSize
           && m_totalMatches < kMaxMatches)
    {
        const QString path = m_fileList[m_currentIndex];

        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            // 简单二进制探测：前 8KB 有 '\0' 就跳过
            const QByteArray peek = file.peek(8192);
            if (!peek.contains('\0')) {
                QTextStream ts(&file);
                ts.setAutoDetectUnicode(true);

                int lineNum = 0;
                while (!ts.atEnd() && m_totalMatches < kMaxMatches) {
                    const QString line = ts.readLine();
                    ++lineNum;

                    const int col = findInLine(line);
                    if (col <= 0) continue;

                    // 同一文件连续追加到同一个顶层节点
                    if (m_lastFilePath != path || !m_lastFileItem) {
                        m_lastFileItem = new QTreeWidgetItem(m_resultTree);
                        const QString rel = m_rootPath.isEmpty()
                                                ? path
                                                : QDir(m_rootPath).relativeFilePath(path);
                        m_lastFileItem->setText(0, rel);
                        m_lastFileItem->setToolTip(0, path);
                        m_lastFileItem->setExpanded(true);
                        m_lastFileItem->setData(0, Qt::UserRole + 10, rel); // 显示路径
                        m_lastFilePath = path;
                    }

                    auto* item = new QTreeWidgetItem(m_lastFileItem);
                    item->setText(0, QString("  %1: %2")
                                      .arg(lineNum, 5)
                                      .arg(line.trimmed()));
                    item->setToolTip(0, line);
                    item->setData(0, Qt::UserRole,     path);
                    item->setData(0, Qt::UserRole + 1, lineNum);
                    item->setData(0, Qt::UserRole + 2, col);

                    ++m_totalMatches;
                }
            }
            file.close();
        }

        ++m_currentIndex;
        ++processed;
    }

    m_progressBar->setValue(m_currentIndex);
    updateStatusLabel();

    if (m_totalMatches >= kMaxMatches) {
        m_searching = false;
        m_statusLabel->setText(
            tr("匹配过多（超过 %1 条），已停止。请细化搜索条件。").arg(kMaxMatches));
        m_progressBar->setVisible(false);
        restoreInputUi();
        return;
    }

    if (m_currentIndex >= total) {
        finishSearch();
        return;
    }

    // 让出事件循环，避免大项目 UI 卡顿
    QTimer::singleShot(0, this, &SearchInFilesDialog::processNextBatch);
}

void SearchInFilesDialog::finishSearch()
{
    m_searching = false;
    m_progressBar->setVisible(false);

    // 更新每个顶层文件节点的显示为 "路径  (N)"
    for (int i = 0; i < m_resultTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* fileItem = m_resultTree->topLevelItem(i);
        if (!fileItem) continue;
        const int cnt = fileItem->childCount();
        const QString rel = fileItem->data(0, Qt::UserRole + 10).toString();
        fileItem->setText(0, QString("%1  (%2)").arg(rel).arg(cnt));
    }

    m_resultTree->expandAll();

    m_statusLabel->setText(
        tr("完成。共 %1 处匹配，%2 个文件。")
            .arg(m_totalMatches)
            .arg(m_resultTree->topLevelItemCount()));

    restoreInputUi();
}

void SearchInFilesDialog::updateStatusLabel()
{
    m_statusLabel->setText(
        tr("扫描 %1/%2，找到 %3 处匹配...")
            .arg(m_currentIndex)
            .arg(m_fileList.size())
            .arg(m_totalMatches));
}

void SearchInFilesDialog::onItemActivated(QTreeWidgetItem* item, int /*column*/)
{
    if (!item) return;
    const QString path = item->data(0, Qt::UserRole).toString();
    if (path.isEmpty()) {
        // 顶层文件节点 → 折叠/展开
        item->setExpanded(!item->isExpanded());
        return;
    }
    const int line = item->data(0, Qt::UserRole + 1).toInt();
    const int col  = item->data(0, Qt::UserRole + 2).toInt();
    emit openFileAt(path, line, col);
}

} // namespace SprintIDE
