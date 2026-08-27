#ifndef PROJECT_SELECTION_DIALOG_H
#define PROJECT_SELECTION_DIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QDialogButtonBox>
#include <QMap>
#include <QDateTime>
#include <QMessageBox>

namespace SprintIDE {

/**
 * @class ProjectSelectionDialog
 * @brief 项目选择对话框，用于启动时选择或创建项目
 *
 * 该对话框提供以下功能：
 * - 显示最近使用的项目列表
 * - 提供打开现有项目的按钮
 * - 提供创建新项目的按钮
 * - 显示项目的基本信息（路径、上次打开时间）
 */
class ProjectSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param recentProjects 最近使用的项目列表
     * @param parent 父对象
     */
    explicit ProjectSelectionDialog(const QMap<QString, QDateTime>& recentProjects,
                                    QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~ProjectSelectionDialog();

    /**
     * @brief 获取用户选择的项目路径
     * @return 选择的项目路径，如果未选择则返回空字符串
     */
    QString selectedProjectPath() const;

    /**
     * @brief 获取用户是否选择了创建新项目
     * @return 是否选择了创建新项目
     */
    bool isCreateNewProject() const;

private slots:
    /**
     * @brief 项目列表项双击事件处理
     * @param item 双击的列表项
     */
    void onProjectItemDoubleClicked(QListWidgetItem* item);

    /**
     * @brief 打开现有项目按钮点击事件处理
     */
    void onOpenExistingProject();

    /**
     * @brief 创建新项目按钮点击事件处理
     */
    void onCreateNewProject();

    /**
     * @brief 项目列表项选择变化事件处理
     * @param current 当前选中的项
     * @param previous 之前选中的项
     */
    void onProjectSelectionChanged(QListWidgetItem* current, QListWidgetItem* previous);

private:
    /**
     * @brief 初始化UI布局
     */
    void initUI();

    /**
     * @brief 加载最近使用的项目列表
     * @param recentProjects 最近使用的项目列表
     */
    void loadRecentProjects(const QMap<QString, QDateTime>& recentProjects);

    /**
     * @brief 显示项目信息
     * @param projectPath 项目路径
     */
    void showProjectInfo(const QString& projectPath);

private:
    QString m_selectedProjectPath; ///< 用户选择的项目路径
    bool m_createNewProject; ///< 是否选择创建新项目

    QListWidget* m_projectList; ///< 项目列表
    QLabel* m_infoLabel; ///< 项目信息标签
    QPushButton* m_openButton; ///< 打开现有项目按钮
    QPushButton* m_createButton; ///< 创建新项目按钮
    QDialogButtonBox* m_buttonBox; ///< 按钮组
};

} // namespace SprintIDE

#endif // PROJECT_SELECTION_DIALOG_H
