#ifndef BREAKPOINT_LIST_PANEL_H
#define BREAKPOINT_LIST_PANEL_H

#include <QWidget>
#include <QListWidget>

class QListWidget;
class QLabel;

namespace SprintIDE {

class DebugManager;

/**
 * @brief 断点列表页签内容（放在底部 Tab 区域，由窄条按钮唤出）。
 *
 * 数据源：DebugManager::getBreakpoints()
 * 用户操作以信号发出，由 MainWindow 统一改 DebugManager 后回调 refresh()。
 */
class BreakpointListPanel : public QWidget
{
    Q_OBJECT
public:
    explicit BreakpointListPanel(DebugManager* manager, QWidget* parent = nullptr);

    /// 从 DebugManager 重新拉取并渲染
    void refresh();

signals:
    void breakpointActivated(const QString& filePath, int line);
    void breakpointRemoveRequested(const QString& filePath, int line);
    void breakpointToggleEnabledRequested(const QString& filePath, int line, bool enabled);
    void breakpointEditConditionRequested(const QString& filePath, int line);

private slots:
    void onItemDoubleClicked(QListWidgetItem* item);
    void onContextMenuRequested(const QPoint& pos);

private:
    DebugManager* m_debugManager = nullptr;
    QListWidget*  m_list         = nullptr;
    QLabel*       m_summaryLabel = nullptr;
};

} // namespace SprintIDE

#endif // BREAKPOINT_LIST_PANEL_H
