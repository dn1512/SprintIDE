#ifndef LOG_LIST_VIEW_H
#define LOG_LIST_VIEW_H

#include <QListView>

namespace SprintIDE {

class LogListModel;

class LogListView : public QListView
{
    Q_OBJECT
public:
    explicit LogListView(QWidget* parent = nullptr);
    void setLogModel(LogListModel* m);
    /// 设置 AI 是否可用（OutputPanel 在 AI 状态变化时调用）
    void setAIAvailable(bool available);

signals:
    /// 双击某条日志且它有 file+line 时发出，MainWindow 接过去 openFile + gotoLine
    void jumpRequested(const QString& file, int line, int column);
    /// 用户右键选了"AI 解释此错误"，附带错误文本
    void aiExplainErrorRequested(const QString& errorText);

protected:
    void keyPressEvent(QKeyEvent* e) override;
    void contextMenuEvent(QContextMenuEvent* e) override;

private slots:
    void onDoubleClicked(const QModelIndex& idx);

private:
    void copySelectionToClipboard();
    void copyAllToClipboard();
    LogListModel* m_logModel = nullptr;
    bool m_aiAvailable = false;
};

} // namespace SprintIDE

#endif // LOG_LIST_VIEW_H
