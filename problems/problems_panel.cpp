#include "problems/problems_panel.h"
#include "theme_manager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QLineEdit>
#include <QToolButton>
#include <QLabel>
#include <QIcon>
#include <QFileInfo>
#include <QMenu>
#include <QHeaderView>
#include <QFont>
#include <QtGui/qshortcut.h>

namespace SprintIDE {

    namespace {

        QIcon severityIcon(const QString& severity)
        {
            if (severity == QLatin1String("error"))
            return QIcon(QStringLiteral(":/icons/icons/log_error.svg"));
            if (severity == QLatin1String("warning"))
            return QIcon(QStringLiteral(":/icons/icons/log_warning.svg"));
            return QIcon(QStringLiteral(":/icons/icons/log_info.svg"));
        }

        QString severityLabel(const QString& severity)
        {
            if (severity == QLatin1String("error"))   return QStringLiteral("error");
            if (severity == QLatin1String("warning")) return QStringLiteral("warning");
            if (severity == QLatin1String("info"))    return QStringLiteral("info");
            if (severity == QLatin1String("hint"))    return QStringLiteral("hint");
            return severity;
        }

    } // anonymous namespace

    ProblemsPanel::ProblemsPanel(DiagnosticsStore* store, QWidget* parent)
    : QWidget(parent)
    , m_store(store)
    {
        setObjectName("ProblemsPanel");

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(0);

        // ==================== 顶部工具栏 ====================
        auto* toolbar = new QWidget(this);
        toolbar->setObjectName("ProblemsToolbar");
        auto* tbLayout = new QHBoxLayout(toolbar);
        tbLayout->setContentsMargins(6, 4, 6, 4);
        tbLayout->setSpacing(6);

        // 搜索框
        m_searchEdit = new QLineEdit(toolbar);
        m_searchEdit->setPlaceholderText(tr("过滤消息… (Esc 清空)"));
        // 不再启用 clearButton

        {
            auto* escSc = new QShortcut(QKeySequence(Qt::Key_Escape), m_searchEdit);
            escSc->setContext(Qt::WidgetShortcut);
            connect(escSc, &QShortcut::activated, this, [this]() {
                    if (m_searchEdit && !m_searchEdit->text().isEmpty()) {
                        m_searchEdit->clear();
                    }
                });
        }
        m_searchEdit->setMinimumWidth(160);
        tbLayout->addWidget(m_searchEdit, 1);

        // 三档过滤按钮
        auto makeFilterBtn = [&](const QString& iconPath, const QString& tip) {
            auto* b = new QToolButton(toolbar);
            b->setIcon(QIcon(iconPath));
            b->setIconSize(QSize(14, 14));
            b->setFixedSize(22, 20);
            b->setCheckable(true);
            b->setToolTip(tip);
            b->setFocusPolicy(Qt::NoFocus);
            b->setCursor(Qt::PointingHandCursor);
            return b;
        };

        m_errorBtn   = makeFilterBtn(":/icons/icons/log_error.svg",   tr("显示错误"));
        m_warningBtn = makeFilterBtn(":/icons/icons/log_warning.svg", tr("显示警告"));
        m_infoBtn    = makeFilterBtn(":/icons/icons/log_info.svg",    tr("显示信息"));

        m_errorBtn->setChecked(m_showError);
        m_warningBtn->setChecked(m_showWarning);
        m_infoBtn->setChecked(m_showInfo);

        tbLayout->addWidget(m_errorBtn);
        tbLayout->addWidget(m_warningBtn);
        tbLayout->addWidget(m_infoBtn);

        root->addWidget(toolbar);

        // ==================== 树 ====================
        m_tree = new QTreeWidget(this);
        m_tree->setObjectName("ProblemsTree");
        m_tree->setHeaderHidden(true);
        m_tree->setRootIsDecorated(true);
        m_tree->setUniformRowHeights(true);
        m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
        m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
        m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
        root->addWidget(m_tree, 1);

        // ==================== 底部摘要 ====================
        m_summary = new QLabel(this);
        m_summary->setObjectName("ProblemsSummary");
        m_summary->setContentsMargins(8, 4, 8, 4);
        root->addWidget(m_summary);

        // ==================== 连接 ====================
        if (m_store) {
            connect(m_store, &DiagnosticsStore::diagnosticsChanged,
                this, &ProblemsPanel::rebuild);
            // 首次填充
            rebuild();
        }

        connect(m_errorBtn,   &QToolButton::toggled, this, &ProblemsPanel::onFilterErrorToggled);
        connect(m_warningBtn, &QToolButton::toggled, this, &ProblemsPanel::onFilterWarningToggled);
        connect(m_infoBtn,    &QToolButton::toggled, this, &ProblemsPanel::onFilterInfoToggled);
        connect(m_searchEdit, &QLineEdit::textChanged, this, &ProblemsPanel::onSearchTextChanged);
        connect(m_tree, &QTreeWidget::itemDoubleClicked, this, &ProblemsPanel::onItemDoubleClicked);
        connect(m_tree, &QTreeWidget::customContextMenuRequested, this, &ProblemsPanel::onContextMenu);

        applyTheme();
        connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &ProblemsPanel::applyTheme);
    }

    // ============================================================
    //  主题
    // ============================================================
    void ProblemsPanel::applyTheme()
    {
        ThemeManager& tm = ThemeManager::instance();

        const QString bg      = tm.colorString(ThemeManager::UIElement::EditorBackground);
        const QString mainBg  = tm.colorString(ThemeManager::UIElement::MainWindowBackground);
        const QString border  = tm.colorString(ThemeManager::UIElement::BorderColor);
        const QString text    = tm.colorString(ThemeManager::UIElement::TextColor);
        const QString secText = tm.colorString(ThemeManager::UIElement::SecondaryTextColor);
        const QString hover   = tm.colorString(ThemeManager::UIElement::TreeHoverBackground);
        const QString sel     = tm.colorString(ThemeManager::UIElement::TreeSelectedBackground);

        setStyleSheet(QString(
                "QWidget#ProblemsPanel {"
                "  background-color: %1;"
                "}"
                "QWidget#ProblemsToolbar {"
                "  background-color: %2;"
                "  border-bottom: 1px solid %3;"
                "}"
                "QLineEdit {"
                "  background-color: %4;"
                "  color: %5;"
                "  border: 1px solid %3;"
                "  border-radius: 3px;"
                "  padding: 3px 6px;"
                "}"
                "QLineEdit:focus { border: 1px solid %6; }"
                "QToolButton {"
                "  background: transparent;"
                "  border: 1px solid transparent;"
                "  border-radius: 3px;"
                "  padding: 0;"
                "}"
                "QToolButton:hover { background-color: %7; border-color: %3; }"
                "QToolButton:checked { background-color: %8; border-color: %3; }"
                "QTreeWidget#ProblemsTree {"
                "  background-color: %1;"
                "  color: %5;"
                "  border: none;"
                "  outline: none;"
                "}"
                "QTreeWidget#ProblemsTree::item { padding: 3px 4px; }"
                "QTreeWidget#ProblemsTree::item:hover { background-color: %7; }"
                "QTreeWidget#ProblemsTree::item:selected { background-color: %8; color: %5; }"
                "QLabel#ProblemsSummary {"
                "  background-color: %2;"
                "  color: %9;"
                "  border-top: 1px solid %3;"
                "  font-size: 11px;"
                "}"
            ).arg(bg, mainBg, border, bg, text,
                tm.colorString(ThemeManager::UIElement::PrimaryColor),
                hover, sel, secText));

        m_tree->viewport()->update();
    }

    // ============================================================
    //  过滤 / 搜索判定
    // ============================================================
    bool ProblemsPanel::passesFilter(const Diagnostic& d) const
    {
        if (d.isError())   return m_showError;
        if (d.isWarning()) return m_showWarning;
        return m_showInfo;   // info / hint
    }

    bool ProblemsPanel::passesSearch(const Diagnostic& d) const
    {
        const QString text = m_searchEdit->text().trimmed();
        if (text.isEmpty()) return true;
        return d.message.contains(text, Qt::CaseInsensitive);
    }

    // ============================================================
    //  重建整棵树
    // ============================================================
    void ProblemsPanel::rebuild()
    {
        m_tree->clear();
        if (!m_store) {
            updateSummary();
            return;
        }

        // 按 filePath 分组
        QHash<QString, QList<Diagnostic>> byFile;
        for (const Diagnostic& d : m_store->all()) {
            if (!passesFilter(d)) continue;
            if (!passesSearch(d)) continue;
            byFile[d.filePath].append(d);
        }

        // 按文件名升序
        QStringList files = byFile.keys();
        files.sort(Qt::CaseInsensitive);

        for (const QString& filePath : files) {
            const auto& diags = byFile[filePath];

            // 文件节点：显示文件名 + 计数
            const QString display = QStringLiteral("%1 (%2)")
            .arg(QFileInfo(filePath).fileName())
            .arg(diags.size());
            auto* fileItem = new QTreeWidgetItem(m_tree);
            fileItem->setText(0, display);
            fileItem->setIcon(0, QIcon(QStringLiteral(":/icons/icons/file_default.svg")));
            fileItem->setData(0, Qt::UserRole + 0, filePath);
            fileItem->setData(0, Qt::UserRole + 1, -1);  // 无 line → 不是诊断

            QFont f = fileItem->font(0);
            f.setBold(true);
            fileItem->setFont(0, f);

            // 诊断子节点
            for (const Diagnostic& d : diags) {
                auto* item = new QTreeWidgetItem(fileItem);
                item->setText(0, QStringLiteral("%1:%2  %3")
                    .arg(d.line)
                    .arg(d.column)
                    .arg(d.message));
                item->setIcon(0, severityIcon(d.severity));
                item->setData(0, Qt::UserRole + 0, filePath);
                item->setData(0, Qt::UserRole + 1, d.line);
                item->setData(0, Qt::UserRole + 2, d.column);
                item->setData(0, Qt::UserRole + 3, d.severity);
                item->setData(0, Qt::UserRole + 4, d.message);
                item->setToolTip(0, QStringLiteral("[%1] %2")
                    .arg(severityLabel(d.severity), d.message));
            }

            fileItem->setExpanded(true);
        }

        updateSummary();
    }

    void ProblemsPanel::updateSummary()
    {
        if (!m_summary) return;

        if (!m_store) {
            m_summary->setText(tr("无诊断"));
            return;
        }

        const int errs = m_store->errorCount();
        const int warns = m_store->warningCount();
        m_summary->setText(tr("错误") + QString("(%1)").arg(errs) + tr("警告") + QString("(%1)").arg(warns));
    }

    // ============================================================
    //  Slots
    // ============================================================
    void ProblemsPanel::onFilterErrorToggled(bool on)
    {
        m_showError = on;
        rebuild();
    }

    void ProblemsPanel::onFilterWarningToggled(bool on)
    {
        m_showWarning = on;
        rebuild();
    }

    void ProblemsPanel::onFilterInfoToggled(bool on)
    {
        m_showInfo = on;
        rebuild();
    }

    void ProblemsPanel::onSearchTextChanged(const QString& /*text*/)
    {
        rebuild();
    }

    void ProblemsPanel::onItemDoubleClicked(QTreeWidgetItem* item, int /*column*/)
    {
        if (!item) return;
        const QString filePath = item->data(0, Qt::UserRole + 0).toString();
        const int line = item->data(0, Qt::UserRole + 1).toInt();
        const int col  = item->data(0, Qt::UserRole + 2).toInt();

        if (filePath.isEmpty()) return;
        emit jumpToFileRequested(filePath, line > 0 ? line : 1,
            col > 0 ? col : 1);
    }

    void ProblemsPanel::onContextMenu(const QPoint& pos)
    {
        QTreeWidgetItem* item = m_tree->itemAt(pos);
        if (!item) return;

        const QString filePath = item->data(0, Qt::UserRole + 0).toString();
        const int line = item->data(0, Qt::UserRole + 1).toInt();
        const int col  = item->data(0, Qt::UserRole + 2).toInt();
        if (filePath.isEmpty()) return;

        QMenu menu(this);
        QAction* jumpAct = menu.addAction(tr("跳转"));
        QAction* fixAct  = nullptr;
        if (line > 0) {
            fixAct = menu.addAction(tr("显示修复..."));
        }

        QAction* chosen = menu.exec(m_tree->viewport()->mapToGlobal(pos));
        if (!chosen) return;

        if (chosen == jumpAct) {
            emit jumpToFileRequested(filePath, line > 0 ? line : 1,
                col > 0 ? col : 1);
        } else if (chosen == fixAct) {
            emit quickFixRequested(filePath, line, col);
        }
    }

} // namespace SprintIDE