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

signals:
    /// 双击某条日志且它有 file+line 时发出，MainWindow 接过去 openFile + gotoLine
    void jumpRequested(const QString& file, int line, int column);

protected:
    void keyPressEvent(QKeyEvent* e) override;
    void contextMenuEvent(QContextMenuEvent* e) override;

private slots:
    void onDoubleClicked(const QModelIndex& idx);

private:
    void copySelectionToClipboard();
    void copyAllToClipboard();
    LogListModel* m_logModel = nullptr;
};

} // namespace SprintIDE

#endif // LOG_LIST_VIEW_H
