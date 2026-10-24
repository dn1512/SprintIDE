// SPDX-License-Identifier: GPL-3.0-or-later
/*
* SprintIDE - SprintIDE is a cross-platform IDE built from scratch with Qt 6 and C++20. Its core capability is to provide AI Agents with an executable, verifiable code execution environment.
* Copyright (C) 2026  Liu Tengjun (刘腾骏)
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#pragma once

#include <QDialog>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QDateTime>

// 新增：语言选择相关控件的前置包含
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QLabel>

#include "project_config.h"

namespace Ui {
class ProjectCreationDialog;
}

namespace SprintIDE {

// ============================================================
//  编程语言枚举
// ============================================================
enum class ProjectLanguage
{
    Cpp,        ///< C / C++
    Python,     ///< Python
    Go,         ///< Go
    Rust,       ///< Rust
    Qt,       ///< Qt
    Node,       ///< Node.js（含 JavaScript / TypeScript 两种变体）
    GenericAI   ///< AI 通用适配器（命令从 .lightcppproj 的 JSON 读取）
};

// 将语言枚举转换成用于界面显示的中文名称
QString projectLanguageToString(ProjectLanguage lang);

// 将语言枚举转换成用于持久化的内部 ID（写入配置文件用）
QString projectLanguageToId(ProjectLanguage lang);

// 从内部 ID 还原语言枚举（读取配置文件用）
ProjectLanguage projectLanguageFromId(const QString& id);

//  项目模板结构体

//  每个模板自带一个 language 字段，用于按语言过滤模板列表。
struct ProjectTemplate
{
    QString id;                                        /// 模板唯一标识符（英文）
    QString name;                                      /// 显示名称（中文）
    QString description;                               /// 详细描述
    ProjectLanguage language = ProjectLanguage::Cpp;   /// 新增：该模板所属语言
    ProjectType type;                                  /// 项目类型（C++ 语义为主）
    QString icon;                                      /// 图标资源路径
    QStringList defaultFiles;                          /// 默认包含的文件（仅用于展示）
    QStringList directories;                           /// 需要创建的目录结构

    // 模板自带的示例文件
    struct ExampleFile {
        QString path;        /// 相对项目根目录的路径
        QString content;     /// 文件内容
        QString language;    /// 文件语言（用于编辑器高亮，非关键）
    };
    QList<ExampleFile> exampleFiles;                   /// 示例文件列表

    QMap<QString, QString> variables;                  /// 模板变量（可选）
};

// ============================================================
//  项目创建结果结构体
// ============================================================
struct ProjectCreationResult
{
    bool success = false;                              ///< 创建是否成功
    QString projectPath;                               ///< 项目根目录
    QString projectName;                               ///< 项目英文名（标识符）
    QString displayName;                               ///< 项目显示名（中文）
    ProjectLanguage language = ProjectLanguage::Cpp;   ///< 项目语言
    ProjectType projectType;                           ///< 项目类型
    QString errorMessage;                              ///< 错误消息
    QDateTime createdTime;                             ///< 创建时间
    QStringList createdDirectories;                    ///< 创建的目录
    QStringList createdFiles;                          ///< 创建的文件

    void reset()
    {
        success = false;
        projectPath.clear();
        projectName.clear();
        displayName.clear();
        language = ProjectLanguage::Cpp;
        projectType = ProjectType::ConsoleApp;
        errorMessage.clear();
        createdTime = QDateTime();
        createdDirectories.clear();
        createdFiles.clear();
    }
};

// ============================================================
//  项目创建对话框
// ============================================================
class ProjectCreationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProjectCreationDialog(QWidget *parent = nullptr);
    ~ProjectCreationDialog();

    // ---------- 公共查询接口 ----------
    ProjectCreationResult getResult() const;
    ProjectConfig         getProjectConfig() const;

    // ---------- 默认路径 ----------
    void    setDefaultProjectPath(const QString& defaultPath);
    QString defaultProjectPath() const;

    // ---------- 模板 ----------
    bool                     loadTemplates();
    QList<ProjectTemplate>   availableTemplates() const;

    /**
    * @brief 设置"仅注册"模式
    *
    * 为 true 时，accept() 不会创建文件夹、文件、示例代码，
    * 只把用户填写的路径写入 m_result。
    *
    * 用途：AI Agent 通过 project.register 注册已有目录时，
    *       或用户在 UI 上只想登记项目而不生成文件。
    */
   void setRegisterOnly(bool enable) { m_registerOnly = enable; }
   bool isRegisterOnly() const { return m_registerOnly; }
public slots:
    // 重写 accept()：按语言分发不同的项目生成器
    void accept() override;

private slots:
    // ---------- 用户交互槽 ----------
    void onBrowseLocation();
    void onProjectNameChanged(const QString& name);
    void onDisplayNameChanged(const QString& name);
    void onLocationChanged(const QString& path);

    void onLanguageChanged(int index);           /// 语言下拉框切换

    void onTemplateChanged(int index);
    void onCompileModeChanged(bool checked);
    void onCompilerChanged(int index);
    void onCppStandardChanged(int index);
    void onOptimizationChanged(int index);
    void onWarningLevelChanged(int index);
    void onShowAdvancedConfig();
    void onShowExampleCode();
    void onResetToDefaults();

    void setDisplayName(const QString& displayName);
    void setProjectType(ProjectType type);
    void setCompilerType(ProjectCompilerType type);

private:
    struct AcceptGuard {
        bool& flag;
        explicit AcceptGuard(bool& f) : flag(f) { flag = true; }
        ~AcceptGuard() { flag = false; }
    };
    // ---------- 初始化 ----------
    void setupUI();
    void setupConnections();
    void initTemplates();
    void initCompilerSettings();

    // ---------- 多语言支持相关方法 ----------
    void             createLanguageComboBox();          ///< 创建语言下拉框
    void             insertLanguageComboBoxIntoLayout();///< 把语言下拉框插入到模板下拉框上方
    void             createLanguageSpecificGroup();     ///< 创建"语言特有选项"分组
    void             refreshTemplateList(ProjectLanguage lang); ///< 按语言过滤模板
    void             refreshLanguageSpecificUI();       ///< 按语言切换特有选项的显示
    ProjectLanguage  currentLanguage() const;           ///< 读取当前语言

    // ---------- 验证 ----------
    bool validateInput(QStringList& errors) const;
    bool validateProjectName(const QString& name, QString& error) const;
    bool validateProjectPath(const QString& path, QString& error) const;

    // ---------- C++ 项目生成流程 ----------
    bool createProjectStructure();
    bool generateProjectFiles();
    bool generateConfigFile();
    bool generateExampleFiles();

    // ---------- 其他语言专属生成器 ----------
    bool generatePythonProject();
    bool generateGoProject();
    bool generateRustProject();
    bool generateNodeProject();
    bool generateQtProject();   ///< Qt 项目生成器

    /// 按语言选择配置文件扩展名（C++/Qt 用 .lightcppproj，其余用 .sprintide.json）
    /// 同时写一份 .lightcppproj 兼容旧加载器（MainWindow::openProject 目前只找它）
    bool saveLanguageAwareConfig(ProjectLanguage lang);

    // ---------- UI 更新 ----------
    void updatePathPreview();
    void updateTemplateDescription();
    void updateCompilerOptions();
    void applyCurrentTemplate();
    ProjectTemplate getCurrentTemplate() const;

    // ---------- 模板变量替换 ----------
    QString replaceTemplateVariables(const QString& content,
                                     const QMap<QString, QString>& variables) const;

    // ---------- 状态存取 ----------
    void saveDialogState();
    void loadDialogState();

    // ---------- 辅助 ----------
    bool writeTextFile(const QString& filePath, const QString& content);
    void appendCreatedFile(const QString& filePath);
    bool saveProjectConfigFile();  ///< 统一保存 .lightcppproj 的辅助函数

    /// @param existedBefore    本次操作前根目录是否已存在
    void rollbackPartialProject(const QString& fullProjectPath, bool existedBefore);

    //  成员变量
    Ui::ProjectCreationDialog *ui;              ///< UI 对象（由 uic 生成）
    ProjectCreationResult      m_result;        ///< 项目创建结果
    ProjectConfig              m_projectConfig; ///< 项目配置对象

    QList<ProjectTemplate>          m_templates;    ///< 所有语言的模板合集
    QMap<QString, ProjectTemplate>  m_templateMap;  ///< 按 ID 索引的模板表

    QString m_defaultProjectPath;      ///< 默认项目路径
    bool    m_autoOpenProject;         ///< 创建后自动打开
    bool    m_createGitRepo;           ///< 是否初始化 Git
    bool    m_initializeGitIgnore;     ///< 是否生成 .gitignore
    bool    m_initialized;             ///< 是否完成初始化（用于抑制过早触发信号）

    QString m_lastUsedPath;            ///< 上次使用的路径
    QMap<QString, QString> m_exampleCodeCache;

    // 动态控件
    QComboBox*   m_languageComboBox      = nullptr;  ///< 语言选择下拉框
    QGroupBox*   m_languageSpecificGroup = nullptr;  ///< 语言特有选项分组
    QFormLayout* m_languageSpecificForm  = nullptr;  ///< 语言特有选项表单

    // Python
    QCheckBox* m_pythonVenvCheckBox    = nullptr;  ///< 是否创建 .venv
    // QLineEdit* m_pythonInterpreterEdit = nullptr;  ///< 自定义解释器路径
    QLineEdit* m_pythonVenvNameEdit    = nullptr;  ///< 虚拟环境目录名
    QLabel* m_pythonEntryLabel = nullptr;          ///入口文件名标签（复用）

    // Go
    QLineEdit* m_goModuleEdit = nullptr;           ///< Go module 名

    // Rust
    QComboBox* m_rustKindCombo = nullptr;          ///< bin / lib

    QComboBox* m_qtTemplateCombo  = nullptr;   ///< Widget 应用 / 控制台 / 空白
    QComboBox* m_qtVersionCombo   = nullptr;   ///< Qt5 / Qt6
    QLineEdit* m_qtDirEdit        = nullptr;   ///< QT_DIR（可选，留空走系统探测）

    // Node
    QComboBox* m_nodeVariantCombo = nullptr;       ///< JavaScript / TypeScript
    QComboBox* m_nodePmCombo      = nullptr;       ///< npm / pnpm / yarn
    QLineEdit* m_nodeEntryEdit    = nullptr;       ///< 入口文件

    // GenericAI
    QLineEdit* m_genericBuildEdit = nullptr;       ///< build 命令
    QLineEdit* m_genericRunEdit   = nullptr;       ///< run 命令
    QLineEdit* m_genericCleanEdit = nullptr;       ///< clean 命令

    bool m_registerOnly = false;   ///< 仅注册项目（不创建文件）
    bool m_acceptInProgress = false;   // 防止 accept() 重入
};

} // namespace SprintIDE
