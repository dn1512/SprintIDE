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

signals:
    void jumpRequested(const QString& file, int line, int column);
    void cleared();

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
