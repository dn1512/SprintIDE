// settings_dialog.cpp
#include "settings_dialog.h"
#include "application.h"
#include "config_manager.h"
#include "compile_manager.h"
#include "ai/ai_settings_page.h"

#include <QFileDialog>
#include <QGroupBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QDir>
#include <QProcess>
#include <QDebug>
#include <QLabel>
#include <QCoreApplication>

#include "python/python_settings_page.h"
#include "editor_settings_page.h"
#include "git/git_settings_page.h"

using namespace SprintIDE;

// ====================================================================
// CompilerSettingsPage 实现
// ====================================================================

CompilerSettingsPage::CompilerSettingsPage(QWidget *parent)
    : SettingsPage(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 编译器类型
    QGroupBox *compilerGroup = new QGroupBox(tr("编译器设置"), this);
    QFormLayout *compilerLayout = new QFormLayout(compilerGroup);

    m_compilerTypeCombo = new QComboBox(this);
    m_compilerTypeCombo->addItem(tr("自动检测"), static_cast<int>(CompilerType::AutoDetect));
    m_compilerTypeCombo->addItem("GCC", static_cast<int>(CompilerType::GCC));
    m_compilerTypeCombo->addItem("Clang", static_cast<int>(CompilerType::Clang));
    m_compilerTypeCombo->addItem("MSVC", static_cast<int>(CompilerType::MSVC));
    m_compilerTypeCombo->addItem("MinGW", static_cast<int>(CompilerType::MinGW));

    m_compilerPathEdit = new QLineEdit(this);
    m_browseButton = new QPushButton(tr("浏览..."), this);
    connect(m_browseButton, &QPushButton::clicked, this, &CompilerSettingsPage::onBrowseCompiler);

    QHBoxLayout *pathLayout = new QHBoxLayout;
    pathLayout->addWidget(m_compilerPathEdit);
    pathLayout->addWidget(m_browseButton);

    compilerLayout->addRow(tr("编译器类型:"), m_compilerTypeCombo);
    compilerLayout->addRow(tr("编译器路径:"), pathLayout);

    // C++ 标准
    QGroupBox *standardGroup = new QGroupBox(tr("C++ 标准"), this);
    QVBoxLayout *standardLayout = new QVBoxLayout(standardGroup);
    m_cppStandardCombo = new QComboBox(this);
    m_cppStandardCombo->addItems({"C++11", "C++14", "C++17", "C++20", "C++23"});
    standardLayout->addWidget(m_cppStandardCombo);

    // 优化选项
    QGroupBox *optimizationGroup = new QGroupBox(tr("优化选项"), this);
    QVBoxLayout *optimizationLayout = new QVBoxLayout(optimizationGroup);
    m_debugInfoCheck = new QCheckBox(tr("包含调试信息 (-g)"), this);
    m_warningsCheck = new QCheckBox(tr("启用所有警告 (-Wall -Wextra)"), this);
    m_warningsAsErrorsCheck = new QCheckBox(tr("将警告视为错误 (-Werror)"), this);

    optimizationLayout->addWidget(m_debugInfoCheck);
    optimizationLayout->addWidget(m_warningsCheck);
    optimizationLayout->addWidget(m_warningsAsErrorsCheck);

    mainLayout->addWidget(compilerGroup);
    mainLayout->addWidget(standardGroup);
    mainLayout->addWidget(optimizationGroup);
    mainLayout->addStretch();
}

void CompilerSettingsPage::loadSettings()
{
    ConfigManager* cfg = Application::instance().configManager();
    if (!cfg) return;

    CompilerType type = static_cast<CompilerType>(
        cfg->value("Compiler/Type", static_cast<int>(CompilerType::AutoDetect)).toInt()
        );
    int index = m_compilerTypeCombo->findData(static_cast<int>(type));
    if (index >= 0) m_compilerTypeCombo->setCurrentIndex(index);

    m_compilerPathEdit->setText(cfg->value("Compiler/Path", "").toString());

    QString std = cfg->value("Compiler/CppStandard", "C++17").toString();
    int stdIndex = m_cppStandardCombo->findText(std);
    if (stdIndex >= 0) m_cppStandardCombo->setCurrentIndex(stdIndex);

    m_debugInfoCheck->setChecked(cfg->value("Compiler/DebugInfo", true).toBool());
    m_warningsCheck->setChecked(cfg->value("Compiler/Warnings", true).toBool());
    m_warningsAsErrorsCheck->setChecked(cfg->value("Compiler/WarningsAsErrors", false).toBool());
}

void CompilerSettingsPage::saveSettings()
{
    ConfigManager* cfg = Application::instance().configManager();
    if (!cfg) return;

    cfg->setValue("Compiler/Type", m_compilerTypeCombo->currentData().toInt());
    cfg->setValue("Compiler/Path", m_compilerPathEdit->text());
    cfg->setValue("Compiler/CppStandard", m_cppStandardCombo->currentText());
    cfg->setValue("Compiler/DebugInfo", m_debugInfoCheck->isChecked());
    cfg->setValue("Compiler/Warnings", m_warningsCheck->isChecked());
    cfg->setValue("Compiler/WarningsAsErrors", m_warningsAsErrorsCheck->isChecked());
}

void CompilerSettingsPage::onBrowseCompiler()
{
    QString path = QFileDialog::getOpenFileName(this, tr("选择编译器"));
    if (!path.isEmpty()) {
        m_compilerPathEdit->setText(path);
    }
}

// ====================================================================
// EnvironmentSettingsPage 实现（全局配置）
// ====================================================================

EnvironmentSettingsPage::EnvironmentSettingsPage(QWidget *parent)
    : SettingsPage(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // ---------- 说明标签 ----------
    QLabel *infoLabel = new QLabel(tr("这些环境变量将全局生效，用于配置各语言的 SDK 路径。"
                                      "它们与具体项目无关，代表 SprintIDE 当前具备开发哪些语言的能力。"), this);
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("QLabel { color: #888888; font-size: 10pt; margin-bottom: 8px; }");
    mainLayout->addWidget(infoLabel);

    // ---------- 顶部：快速填充区域 ----------
    QHBoxLayout *topLayout = new QHBoxLayout;

    QLabel *presetLabel = new QLabel(tr("快速填充 SDK:"), this);
    m_sdkPresetCombo = new QComboBox(this);
    m_sdkPresetCombo->addItem(tr("选择已检测的 SDK..."));
    m_sdkPresetCombo->addItem("JAVA_HOME");
    m_sdkPresetCombo->addItem("MAVEN_HOME");
    m_sdkPresetCombo->addItem("GRADLE_HOME");
    m_sdkPresetCombo->addItem("ANDROID_HOME");
    m_sdkPresetCombo->addItem("ANDROID_SDK_ROOT");
    m_sdkPresetCombo->addItem("QT_DIR");
    m_sdkPresetCombo->addItem("GOROOT");
    m_sdkPresetCombo->addItem("RUSTUP_HOME");
    m_sdkPresetCombo->addItem("CARGO_HOME");
    m_sdkPresetCombo->addItem("NODE_HOME");
    m_sdkPresetCombo->addItem("PYTHON_HOME");

    m_detectButton = new QPushButton(tr("自动检测已安装 SDK"), this);

    topLayout->addWidget(presetLabel);
    topLayout->addWidget(m_sdkPresetCombo);
    topLayout->addWidget(m_detectButton);
    topLayout->addStretch();

    mainLayout->addLayout(topLayout);

    // ---------- 环境变量表格 ----------
    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setColumnCount(2);
    m_tableWidget->setHorizontalHeaderLabels({tr("变量名"), tr("值")});
    m_tableWidget->horizontalHeader()->setStretchLastSection(true);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableWidget->setAlternatingRowColors(false);

    mainLayout->addWidget(m_tableWidget);

    // ---------- 底部按钮 ----------
    QHBoxLayout *bottomLayout = new QHBoxLayout;
    m_addButton = new QPushButton(tr("添加"), this);
    m_removeButton = new QPushButton(tr("删除选中"), this);
    m_removeButton->setEnabled(false);
    QPushButton *clearButton = new QPushButton(tr("清空全部"), this);

    bottomLayout->addWidget(m_addButton);
    bottomLayout->addWidget(m_removeButton);
    bottomLayout->addWidget(clearButton);
    bottomLayout->addStretch();

    mainLayout->addLayout(bottomLayout);

    // ---------- 连接信号 ----------
    connect(m_addButton, &QPushButton::clicked, this, &EnvironmentSettingsPage::onAddRow);
    connect(m_removeButton, &QPushButton::clicked, this, &EnvironmentSettingsPage::onRemoveRow);
    connect(clearButton, &QPushButton::clicked, this, [this]() {
        if (m_tableWidget->rowCount() > 0) {
            int reply = QMessageBox::question(this, tr("确认清空"),
                                              tr("确定要删除所有环境变量吗？"),
                                              QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                m_tableWidget->setRowCount(0);
            }
        }
    });
    connect(m_detectButton, &QPushButton::clicked, this, &EnvironmentSettingsPage::onDetectSDK);
    connect(m_sdkPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EnvironmentSettingsPage::onSdkComboChanged);

    connect(m_tableWidget, &QTableWidget::cellChanged,
            this, &EnvironmentSettingsPage::onCellChanged);
    connect(m_tableWidget, &QTableWidget::itemSelectionChanged,
            this, [this]() {
                m_removeButton->setEnabled(m_tableWidget->currentRow() >= 0);
            });

    setupTable();
}

void EnvironmentSettingsPage::setupTable()
{
    m_tableWidget->setColumnWidth(0, 200);
    m_tableWidget->setEditTriggers(QAbstractItemView::DoubleClicked |
                                   QAbstractItemView::EditKeyPressed |
                                   QAbstractItemView::AnyKeyPressed);
}

void EnvironmentSettingsPage::addSdkRow(const QString& key, const QString& value)
{
    if (key.isEmpty()) return;

    // 检查是否已存在相同 key
    for (int row = 0; row < m_tableWidget->rowCount(); ++row) {
        QTableWidgetItem *item = m_tableWidget->item(row, 0);
        if (item && item->text() == key) {
            // 更新值
            m_tableWidget->setItem(row, 1, new QTableWidgetItem(value));
            return;
        }
    }

    int row = m_tableWidget->rowCount();
    m_tableWidget->insertRow(row);
    m_tableWidget->setItem(row, 0, new QTableWidgetItem(key));
    m_tableWidget->setItem(row, 1, new QTableWidgetItem(value));
}

void EnvironmentSettingsPage::loadSettings()
{
    m_tableWidget->setRowCount(0);
    QVariantMap sdkEnv = loadFromGlobalConfig();
    for (auto it = sdkEnv.begin(); it != sdkEnv.end(); ++it) {
        addSdkRow(it.key(), it.value().toString());
    }
}

void EnvironmentSettingsPage::saveSettings()
{
    QVariantMap sdkEnv;
    for (int row = 0; row < m_tableWidget->rowCount(); ++row) {
        QTableWidgetItem *keyItem = m_tableWidget->item(row, 0);
        QTableWidgetItem *valItem = m_tableWidget->item(row, 1);
        if (keyItem && valItem) {
            QString key = keyItem->text().trimmed();
            QString value = valItem->text().trimmed();
            if (!key.isEmpty() && !value.isEmpty()) {
                sdkEnv[key] = value;
            }
        }
    }
    saveToGlobalConfig(sdkEnv);
}

void EnvironmentSettingsPage::saveToGlobalConfig(const QVariantMap& sdkEnv)
{
    ConfigManager* cfg = Application::instance().configManager();
    if (!cfg) return;

    cfg->setValue("Sdk/Environment", QVariant::fromValue(sdkEnv));
    QMessageBox::information(this, tr("保存成功"),
                             tr("SDK 环境变量已保存到全局设置。"));
}

QVariantMap EnvironmentSettingsPage::loadFromGlobalConfig() const
{
    ConfigManager* cfg = Application::instance().configManager();
    if (!cfg) return QVariantMap();

    return cfg->value("Sdk/Environment").toMap();
}

// ====================================================================
// 辅助函数：通过 shell 获取环境变量
// ====================================================================
QString EnvironmentSettingsPage::getShellEnvironmentVariable(const QString& varName) const
{
    // 首先尝试系统环境变量
    QProcessEnvironment sysEnv = QProcessEnvironment::systemEnvironment();
    if (sysEnv.contains(varName)) {
        QString value = sysEnv.value(varName);
        if (!value.isEmpty()) return value;
    }

#ifdef Q_OS_WIN
    // Windows 使用 cmd
    QProcess proc;
    proc.start("cmd.exe", {"/c", "echo %" + varName + "%"});
    proc.waitForFinished(2000);
    QString output = QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed();
    if (!output.isEmpty() && output != "%" + varName + "%") {
        return output;
    }
    return QString();
#else
    // macOS/Linux：优先尝试 zsh（因为很多用户默认使用 zsh）
    QStringList shells = {"/bin/zsh", "/bin/bash", "/bin/sh"};
    for (const QString& shell : shells) {
        // 使用 -l 登录 shell 以加载用户配置
        QProcess proc;
        proc.start(shell, {"-l", "-c", "echo $" + varName});
        if (proc.waitForFinished(3000)) {
            QString output = QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed();
            if (!output.isEmpty()) {
                return output;
            }
        }
        // 如果 -l 失败，尝试不带 -l（但通常需要）
    }

    // 额外尝试：直接执行 echo $VAR 使用当前 shell（可能不加载配置文件）
    QProcess proc;
    proc.start("/bin/sh", {"-c", "echo $" + varName});
    proc.waitForFinished(2000);
    QString output = QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed();
    if (!output.isEmpty()) {
        return output;
    }

    return QString();
#endif
}

// ====================================================================
// SDK 路径检测函数
// ====================================================================
QString EnvironmentSettingsPage::detectSDKPath(const QString& key) const
{
    QString value;

    // 优先从 shell 环境获取
    value = getShellEnvironmentVariable(key);
    if (!value.isEmpty() && QFile::exists(value)) {
        return value;
    }

    // 如果环境变量未设置或路径无效，进行特定检测
    if (key == "JAVA_HOME") {
#ifdef Q_OS_WIN
        QStringList paths = {"C:/Program Files/Java", "C:/Program Files (x86)/Java"};
        for (const QString& base : paths) {
            QDir dir(base);
            if (dir.exists()) {
                QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                for (const QString& sub : subdirs) {
                    if (sub.contains("jdk", Qt::CaseInsensitive) || sub.contains("openjdk", Qt::CaseInsensitive)) {
                        QString candidate = dir.absoluteFilePath(sub);
                        if (QFile::exists(candidate + "/bin/java.exe") || QFile::exists(candidate + "/bin/java")) {
                            return candidate;
                        }
                    }
                }
            }
        }
#elif defined(Q_OS_MAC)
        // 使用 /usr/libexec/java_home
        QProcess proc;
        proc.start("/usr/libexec/java_home");
        proc.waitForFinished(1000);
        value = QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed();
        if (!value.isEmpty() && QFile::exists(value + "/bin/java")) {
            return value;
        }
        // 检查 Homebrew 安装
        QStringList brewPaths = {"/opt/homebrew/Cellar/openjdk", "/usr/local/Cellar/openjdk"};
        for (const QString& base : brewPaths) {
            QDir dir(base);
            if (dir.exists()) {
                QStringList versions = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                for (const QString& ver : versions) {
                    QString candidate = dir.absoluteFilePath(ver) + "/libexec/openjdk.jdk/Contents/Home";
                    if (QFile::exists(candidate + "/bin/java")) return candidate;
                }
            }
        }
#else
        // Linux
        QStringList paths = {"/usr/lib/jvm", "/usr/local/lib/jvm"};
        for (const QString& base : paths) {
            QDir dir(base);
            if (dir.exists()) {
                QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                for (const QString& sub : subdirs) {
                    if (sub.contains("java") || sub.contains("jdk") || sub.contains("openjdk")) {
                        QString candidate = dir.absoluteFilePath(sub);
                        if (QFile::exists(candidate + "/bin/java")) return candidate;
                    }
                }
            }
        }
#endif
    } else if (key == "MAVEN_HOME") {
        // 先尝试从 shell 环境变量获取
        QString envValue = getShellEnvironmentVariable("MAVEN_HOME");
        if (!envValue.isEmpty() && QFile::exists(envValue)) {
            return envValue;
        }

        // 如果环境变量无效，扫描常见安装路径
#ifdef Q_OS_WIN
        QStringList paths = {"C:/Program Files/Maven", "C:/Program Files (x86)/Maven", "C:/maven"};
        for (const QString& base : paths) {
            QDir dir(base);
            if (dir.exists()) {
                QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                for (const QString& sub : subdirs) {
                    if (sub.contains("apache-maven", Qt::CaseInsensitive)) {
                        QString candidate = dir.absoluteFilePath(sub);
                        if (QFile::exists(candidate + "/bin/mvn.cmd") || QFile::exists(candidate + "/bin/mvn")) {
                            return candidate;
                        }
                    }
                }
            }
        }
#elif defined(Q_OS_MAC)
        // 扫描 Homebrew 路径
        QStringList brewPaths = {"/opt/homebrew/Cellar/maven", "/usr/local/Cellar/maven"};
        for (const QString& base : brewPaths) {
            QDir dir(base);
            if (dir.exists()) {
                QStringList versions = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                for (const QString& ver : versions) {
                    QString candidate = dir.absoluteFilePath(ver) + "/libexec";
                    if (QFile::exists(candidate + "/bin/mvn")) return candidate;
                }
            }
        }
        // 常见安装位置
        if (QFile::exists("/usr/local/maven/bin/mvn")) {
            return "/usr/local/maven";
        }
        // 新增：扫描用户自定义目录（例如 ~/Application/apache-maven-*）
        QString home = QDir::homePath();
        QStringList customPaths = {
            home + "/Application/apache-maven-*",
            home + "/apache-maven-*",
            home + "/maven-*"
        };
        for (const QString& pattern : customPaths) {
            QDir dir(QFileInfo(pattern).path());
            if (dir.exists()) {
                QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
                for (const QString& entry : entries) {
                    if (entry.startsWith("apache-maven-") || entry.startsWith("maven-")) {
                        QString candidate = dir.absoluteFilePath(entry);
                        if (QFile::exists(candidate + "/bin/mvn")) {
                            return candidate;
                        }
                    }
                }
            }
        }
#else
        // Linux
        QStringList paths = {"/usr/share/maven", "/usr/local/maven", "/opt/maven"};
        for (const QString& base : paths) {
            if (QFile::exists(base + "/bin/mvn")) {
                return base;
            }
        }
        // 也扫描用户目录下的 apache-maven-*
        QString home = QDir::homePath();
        QStringList customPaths = {
            home + "/apache-maven-*",
            home + "/maven-*"
        };
        for (const QString& pattern : customPaths) {
            QDir dir(QFileInfo(pattern).path());
            if (dir.exists()) {
                QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
                for (const QString& entry : entries) {
                    if (entry.startsWith("apache-maven-") || entry.startsWith("maven-")) {
                        QString candidate = dir.absoluteFilePath(entry);
                        if (QFile::exists(candidate + "/bin/mvn")) {
                            return candidate;
                        }
                    }
                }
            }
        }
#endif
    } else if (key == "GRADLE_HOME") {
#ifdef Q_OS_WIN
        QStringList paths = {"C:/Program Files/Gradle", "C:/Program Files (x86)/Gradle", "C:/gradle"};
        for (const QString& base : paths) {
            QDir dir(base);
            if (dir.exists()) {
                QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                for (const QString& sub : subdirs) {
                    if (sub.contains("gradle", Qt::CaseInsensitive)) {
                        QString candidate = dir.absoluteFilePath(sub);
                        if (QFile::exists(candidate + "/bin/gradle.bat") || QFile::exists(candidate + "/bin/gradle")) {
                            return candidate;
                        }
                    }
                }
            }
        }
#elif defined(Q_OS_MAC)
        QStringList brewPaths = {"/opt/homebrew/Cellar/gradle", "/usr/local/Cellar/gradle"};
        for (const QString& base : brewPaths) {
            QDir dir(base);
            if (dir.exists()) {
                QStringList versions = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                for (const QString& ver : versions) {
                    QString candidate = dir.absoluteFilePath(ver) + "/libexec";
                    if (QFile::exists(candidate + "/bin/gradle")) return candidate;
                }
            }
        }
        if (QFile::exists("/usr/local/gradle/bin/gradle")) {
            return "/usr/local/gradle";
        }
#else
        QStringList paths = {"/usr/share/gradle", "/usr/local/gradle", "/opt/gradle"};
        for (const QString& base : paths) {
            if (QFile::exists(base + "/bin/gradle")) {
                return base;
            }
        }
#endif
    } else if (key == "ANDROID_HOME" || key == "ANDROID_SDK_ROOT") {
#ifdef Q_OS_WIN
        QStringList paths = {"C:/Users/Public/Android/Sdk", "C:/Android/Sdk"};
        for (const QString& path : paths) {
            if (QFile::exists(path)) return path;
        }
#elif defined(Q_OS_MAC)
        QString path = QDir::homePath() + "/Library/Android/sdk";
        if (QFile::exists(path)) return path;
#else
        QString path = QDir::homePath() + "/Android/Sdk";
        if (QFile::exists(path)) return path;
#endif
    } else if (key == "QT_DIR") {
#ifdef Q_OS_MAC
        // 1. 扫 ~/Qt/<version>/<kit>/
        QDir qtBase(QDir::homePath() + "/Qt");
        if (qtBase.exists()) {
            QStringList versions = qtBase.entryList(QDir::Dirs | QDir::NoDotAndDotDot,
                                                    QDir::Name | QDir::Reversed);
            for (const QString& ver : versions) {
                QDir verDir(qtBase.absoluteFilePath(ver));
                // kit 命名：macos / gcc_64 / msvc2019_64 / android_arm64_v8a ...
                QStringList kits = verDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                for (const QString& kit : kits) {
                    QString cand = verDir.absoluteFilePath(kit);
                    if (QFile::exists(cand + "/lib/cmake/Qt6/Qt6Config.cmake") ||
                        QFile::exists(cand + "/lib/cmake/Qt5/Qt5Config.cmake")) {
                        qDebug() << "[EnvSettings] 找到 Qt:" << cand;
                        return cand;
                    }
                }
            }
        }
        // 2. Homebrew
        for (const QString& p : {QString("/opt/homebrew/opt/qt"),
                                 QString("/usr/local/opt/qt")}) {
            if (QFile::exists(p + "/lib/cmake/Qt6/Qt6Config.cmake"))
                return QFileInfo(p).canonicalFilePath();
        }
#elif defined(Q_OS_WIN)
        QDir qtBase("C:/Qt");
        if (qtBase.exists()) {
            QStringList versions = qtBase.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QString& ver : versions) {
                QDir verDir(qtBase.absoluteFilePath(ver));
                QStringList kits = verDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                for (const QString& kit : kits) {
                    QString cand = verDir.absoluteFilePath(kit);
                    if (QFile::exists(cand + "/lib/cmake/Qt6/Qt6Config.cmake") ||
                        QFile::exists(cand + "/lib/cmake/Qt5/Qt5Config.cmake")) {
                        return cand;
                    }
                }
            }
        }
#else
        // Linux：系统包管理器安装的 Qt
        for (const QString& p : {QString("/usr/lib/x86_64-linux-gnu/cmake/Qt6"),
                                 QString("/usr/lib/cmake/Qt6"),
                                 QString("/usr/lib64/cmake/Qt6")}) {
            if (QFile::exists(p + "/Qt6Config.cmake"))
                return QFileInfo(p).absolutePath();
        }
#endif
        return QString();
    } else if (key == "GOROOT") {
        QProcess proc;
        proc.start("go", {"env", "GOROOT"});
        proc.waitForFinished(1000);
        value = QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed();
        if (!value.isEmpty() && QFile::exists(value + "/bin/go")) {
            return value;
        }
    } else if (key == "RUSTUP_HOME") {
        QString home = QDir::homePath();
        QString path = home + "/.rustup";
        if (QFile::exists(path)) return path;
    } else if (key == "CARGO_HOME") {
        QString home = QDir::homePath();
        QString path = home + "/.cargo";
        if (QFile::exists(path)) return path;
        // 也尝试从 shell 获取（上面已经做了）
    } else if (key == "NODE_HOME") {
        QProcess proc;
        proc.start("which", {"node"});
        proc.waitForFinished(1000);
        QString nodePath = QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed();
        if (!nodePath.isEmpty()) {
            return QFileInfo(nodePath).absolutePath();
        }
#ifdef Q_OS_WIN
        QStringList paths = {"C:/Program Files/nodejs", "C:/Program Files (x86)/nodejs"};
        for (const QString& path : paths) {
            if (QFile::exists(path + "/node.exe")) return path;
        }
#endif
    } else if (key == "PYTHON_HOME") {
        QProcess proc;
        proc.start("which", {"python3"});
        proc.waitForFinished(1000);
        QString pyPath = QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed();
        if (!pyPath.isEmpty()) {
            return QFileInfo(pyPath).absolutePath();
        }
        proc.start("which", {"python"});
        proc.waitForFinished(1000);
        pyPath = QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed();
        if (!pyPath.isEmpty()) {
            return QFileInfo(pyPath).absolutePath();
        }
    }

    return QString();
}

// ====================================================================
// EnvironmentSettingsPage 槽函数
// ====================================================================

void EnvironmentSettingsPage::onAddRow()
{
    int row = m_tableWidget->rowCount();
    m_tableWidget->insertRow(row);
    m_tableWidget->setItem(row, 0, new QTableWidgetItem(""));
    m_tableWidget->setItem(row, 1, new QTableWidgetItem(""));
    m_tableWidget->editItem(m_tableWidget->item(row, 0));
}

void EnvironmentSettingsPage::onRemoveRow()
{
    int row = m_tableWidget->currentRow();
    if (row < 0 || row >= m_tableWidget->rowCount()) return;

    QTableWidgetItem *keyItem = m_tableWidget->item(row, 0);
    if (keyItem && !keyItem->text().isEmpty()) {
        int reply = QMessageBox::question(this, tr("确认删除"),
                                          tr("确定要删除环境变量 '%1' 吗？").arg(keyItem->text()),
                                          QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) return;
    }

    m_tableWidget->removeRow(row);
    m_removeButton->setEnabled(false);
}

void EnvironmentSettingsPage::onCellChanged(int row, int column)
{
    Q_UNUSED(row);
    Q_UNUSED(column);
    m_removeButton->setEnabled(m_tableWidget->currentRow() >= 0);
}

void EnvironmentSettingsPage::onSdkComboChanged(int index)
{
    if (index <= 0) return;

    QString key = m_sdkPresetCombo->currentText();
    QString value = detectSDKPath(key);

    if (!value.isEmpty()) {
        addSdkRow(key, value);
        m_sdkPresetCombo->setCurrentIndex(0);
    } else {
        QMessageBox::information(this, tr("提示"),
                                 tr("未检测到 %1 的安装路径，请手动输入。").arg(key));
    }
}

void EnvironmentSettingsPage::onDetectSDK()
{
    QStringList commonKeys = {
        "JAVA_HOME",
        "MAVEN_HOME",
        "GRADLE_HOME",
        "ANDROID_HOME",
        "ANDROID_SDK_ROOT",
        "QT_DIR",
        "GOROOT",
        "RUSTUP_HOME",
        "CARGO_HOME",
        "NODE_HOME",
        "PYTHON_HOME"
    };

    int detected = 0;
    for (const QString& key : commonKeys) {
        QString value = detectSDKPath(key);
        if (!value.isEmpty()) {
            addSdkRow(key, value);
            detected++;
        }
    }

    QMessageBox::information(this, tr("检测完成"),
                             tr("共检测到 %1 个 SDK 环境变量。").arg(detected));
}

// ====================================================================
// SettingsDialog 实现
// ====================================================================

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("设置"));
    resize(900, 600);
    createUI();
    loadAllSettings();

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, &SettingsDialog::onApply);
}

void SettingsDialog::createUI()
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);

    // 左侧分类列表
    m_categoryList = new QListWidget(this);
    m_categoryList->setFixedWidth(180);
    m_categoryList->addItem(tr("编辑器"));
    m_categoryList->addItem(tr("编译配置"));
    m_categoryList->addItem(tr("环境变量"));

    // 添加 Python 设置页
    m_categoryList->addItem(tr("Python"));

    // AI 设置页
    m_categoryList->addItem(tr("AI"));

    // Git 设置页
    m_categoryList->addItem(tr("Git"));

    // 右侧堆叠页面
    m_stackedWidget = new QStackedWidget(this);


    EditorSettingsPage *editorPage = new EditorSettingsPage(this);
    m_stackedWidget->addWidget(editorPage);
    m_pages.append(editorPage);

    CompilerSettingsPage *compilerPage = new CompilerSettingsPage(this);
    m_stackedWidget->addWidget(compilerPage);
    m_pages.append(compilerPage);

    EnvironmentSettingsPage *envPage = new EnvironmentSettingsPage(this);
    m_stackedWidget->addWidget(envPage);
    m_pages.append(envPage);


    PythonSettingsPage* pyPage = new PythonSettingsPage(this);
    m_stackedWidget->addWidget(pyPage);
    m_pages.append(pyPage);

    AISettingsPage* aiPage = new AISettingsPage(this);
    m_stackedWidget->addWidget(aiPage);
    m_pages.append(aiPage);

    GitSettingsPage* gitPage = new GitSettingsPage(this);
    m_stackedWidget->addWidget(gitPage);
    m_pages.append(gitPage);

    connect(m_categoryList, &QListWidget::currentRowChanged,
            this, &SettingsDialog::onCategoryChanged);
    // 转发信号（如果 SettingsDialog 需要向上传递）：
    connect(aiPage, &AISettingsPage::aiServerRestartRequested,
            this,   &SettingsDialog::aiServerRestartRequested);
    m_categoryList->setCurrentRow(0);

    // 底部按钮
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);

    // 布局组装
    mainLayout->addWidget(m_categoryList);
    QVBoxLayout *rightLayout = new QVBoxLayout;
    rightLayout->addWidget(m_stackedWidget);
    rightLayout->addWidget(m_buttonBox);
    mainLayout->addLayout(rightLayout);
}

void SettingsDialog::onCategoryChanged(int currentRow)
{
    if (currentRow >= 0 && currentRow < m_stackedWidget->count()) {
        m_stackedWidget->setCurrentIndex(currentRow);
    }
}

void SettingsDialog::showCategory(const QString& name)
{
    if (!m_categoryList) return;
    for (int i = 0; i < m_categoryList->count(); ++i) {
        if (m_categoryList->item(i)->text() == name) {
            m_categoryList->setCurrentRow(i);
            return;
        }
    }
}

void SettingsDialog::loadAllSettings()
{
    for (SettingsPage *page : m_pages) {
        page->loadSettings();
    }
}

void SettingsDialog::saveAllSettings()
{
    for (SettingsPage *page : m_pages) {
        page->saveSettings();
    }
}

void SettingsDialog::onApply()
{
    saveAllSettings();
}

void SettingsDialog::accept()
{
    saveAllSettings();
    QDialog::accept();
}
