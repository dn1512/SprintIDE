#include "editor_settings_page.h"
#include "theme_manager.h"
#include "application.h"
#include "config_manager.h"
#include "font_manager.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFontDatabase>
#include <QTextBlockFormat>
#include <QTextCursor>

namespace SprintIDE {

EditorSettingsPage::EditorSettingsPage(QWidget *parent)
    : SettingsPage(parent)
{
    setupUI();
}

void EditorSettingsPage::setupUI()
{
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setSpacing(12);

    // ========================================================================
    // 1. 字体分组
    // ========================================================================
    QGroupBox* fontGroup = new QGroupBox(tr("字体"), this);
    QFormLayout* fontForm = new QFormLayout(fontGroup);

    m_fontFamilyCombo = new QComboBox(this);
    m_fontFamilyCombo->setMinimumWidth(220);
    buildFontFamilyCombo();     // 只列等宽字体
    fontForm->addRow(tr("字体族:"), m_fontFamilyCombo);

    m_fontSizeSpin = new QSpinBox(this);
    m_fontSizeSpin->setRange(8, 24);
    m_fontSizeSpin->setSuffix(tr(" pt"));
    m_fontSizeSpin->setValue(12);
    fontForm->addRow(tr("字号:"), m_fontSizeSpin);

    // 说明：行距是固定的，不暴露给用户
    QLabel* hint = new QLabel(
        tr("行距由 SprintIDE 自动按字号计算（1.5 倍字号），无需手动设置。"),
        this);
    hint->setWordWrap(true);
    hint->setStyleSheet("color: #7A8599; font-size: 11px;");
    fontForm->addRow(QString(), hint);

    root->addWidget(fontGroup);

    // ========================================================================
    // 2. 缩进分组
    // ========================================================================
    QGroupBox* indentGroup = new QGroupBox(tr("缩进"), this);
    QFormLayout* indentForm = new QFormLayout(indentGroup);

    m_tabWidthSpin = new QSpinBox(this);
    m_tabWidthSpin->setRange(1, 12);
    m_tabWidthSpin->setSuffix(tr(" 字符"));
    m_tabWidthSpin->setValue(4);
    indentForm->addRow(tr("Tab 宽度:"), m_tabWidthSpin);

    m_useSpacesCheck = new QCheckBox(tr("使用空格代替 Tab"), this);
    m_useSpacesCheck->setChecked(true);
    indentForm->addRow(QString(), m_useSpacesCheck);

    root->addWidget(indentGroup);

    // ========================================================================
    // 3. 显示分组
    // ========================================================================
    QGroupBox* displayGroup = new QGroupBox(tr("显示"), this);
    QVBoxLayout* displayLayout = new QVBoxLayout(displayGroup);

    m_showLineNumbersCheck = new QCheckBox(tr("显示行号"), this);
    m_showLineNumbersCheck->setChecked(true);
    displayLayout->addWidget(m_showLineNumbersCheck);

    m_highlightCurrentLineCheck = new QCheckBox(tr("高亮当前行"), this);
    m_highlightCurrentLineCheck->setChecked(true);
    displayLayout->addWidget(m_highlightCurrentLineCheck);

    root->addWidget(displayGroup);

    // ========================================================================
    // 4. 实时预览
    // ========================================================================
    QGroupBox* previewGroup = new QGroupBox(tr("预览"), this);
    QVBoxLayout* previewLayout = new QVBoxLayout(previewGroup);

    m_preview = new QPlainTextEdit(this);
    m_preview->setReadOnly(true);
    m_preview->setMinimumHeight(160);
    m_preview->setPlainText(
        "// 这段代码会实时反映字体与缩进设置\n"
        "#include <iostream>\n"
        "\n"
        "int main() {\n"
        "    std::cout << \"Hello, SprintIDE!\" << std::endl;\n"
        "    return 0;\n"
        "}\n");
    previewLayout->addWidget(m_preview);

    root->addWidget(previewGroup, 1);

    // ========================================================================
    // 5. 信号连接
    // ========================================================================
    connect(m_fontFamilyCombo,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EditorSettingsPage::onFontFamilyChanged);

    connect(m_fontSizeSpin,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this, &EditorSettingsPage::onFontSizeChanged);

    connect(m_tabWidthSpin,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this, &EditorSettingsPage::onTabWidthChanged);

    connect(m_useSpacesCheck, &QCheckBox::toggled,
            this, &EditorSettingsPage::onUseSpacesToggled);

    connect(m_showLineNumbersCheck, &QCheckBox::toggled,
            this, &EditorSettingsPage::onShowLineNumbersToggled);

    connect(m_highlightCurrentLineCheck, &QCheckBox::toggled,
            this, &EditorSettingsPage::onHighlightCurrentLineToggled);

    // 初始预览
    applyPreviewStyle();
}

//  构建字体族下拉框
void EditorSettingsPage::buildFontFamilyCombo()
{
    m_fontFamilyCombo->clear();

    // 1. 优先列出项目内置字体
    if (FontManager::isJetBrainsMonoAvailable()) {
        m_fontFamilyCombo->addItem("JetBrains Mono");
    }

    // 2. 列系统所有等宽字体（去重、排序）
    QFontDatabase db;
    const QStringList families = db.families();
    QStringList monoFonts;
    for (const QString& family : families) {
        // Qt 6：静态方法判断是否等宽
        if (QFontDatabase::isFixedPitch(family)) {
            monoFonts.append(family);
        }
    }
    monoFonts.sort(Qt::CaseInsensitive);

    // 避免和内置 JetBrains Mono 重复
    for (const QString& f : monoFonts) {
        if (m_fontFamilyCombo->findText(f) < 0) {
            m_fontFamilyCombo->addItem(f);
        }
    }
}

//  应用预览区样式（字体 + 行高）
void EditorSettingsPage::applyPreviewStyle()
{
    // 字体
    QFont f = m_preview->font();
    f.setFamily(m_fontFamilyCombo->currentText());
    f.setPointSize(m_fontSizeSpin->value());
    f.setFixedPitch(true);
    m_preview->setFont(f);
    m_preview->document()->setDefaultFont(f);

    // Tab 宽度
    QFontMetricsF fm(f);
    m_preview->setTabStopDistance(fm.horizontalAdvance(' ') * m_tabWidthSpin->value());

    // 行高（与 TextEditor 使用同一比例）
    applyLineHeightToPreview();

    // 主题颜色
    ThemeManager& tm = ThemeManager::instance();
    QPalette pal = m_preview->palette();
    pal.setColor(QPalette::Base,   tm.color(ThemeManager::UIElement::EditorBackground));
    pal.setColor(QPalette::Text,   tm.color(ThemeManager::UIElement::TextColor));
    m_preview->setPalette(pal);
    m_preview->setAutoFillBackground(true);
}

//  与 TextEditor 用同一条行高规则
void EditorSettingsPage::applyLineHeightToPreview()
{
    QTextBlockFormat fmt;
    fmt.setLineHeight(ThemeManager::kEditorLineHeightPercent,
                      QTextBlockFormat::ProportionalHeight);

    QTextCursor cursor(m_preview->document());
    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    cursor.mergeBlockFormat(fmt);
    cursor.endEditBlock();
}

//  各 slot
void EditorSettingsPage::onFontFamilyChanged(int /*index*/)
{
    applyPreviewStyle();
}

void EditorSettingsPage::onFontSizeChanged(int /*value*/)
{
    applyPreviewStyle();
}

void EditorSettingsPage::onTabWidthChanged(int /*value*/)
{
    applyPreviewStyle();
}

void EditorSettingsPage::onUseSpacesToggled(bool /*checked*/) {}

void EditorSettingsPage::onShowLineNumbersToggled(bool /*checked*/) {}

void EditorSettingsPage::onHighlightCurrentLineToggled(bool /*checked*/) {}

//  加载 / 保存
void EditorSettingsPage::loadSettings()
{
    ConfigManager* cfg = Application::instance().configManager();
    if (!cfg) return;

    // 字体
    const QString family = cfg->value("Editor/FontFamily", "JetBrains Mono").toString();
    int idx = m_fontFamilyCombo->findText(family);
    if (idx < 0) idx = 0;
    m_fontFamilyCombo->setCurrentIndex(idx);

    const int size = cfg->value("Editor/FontSize", 12).toInt();
    m_fontSizeSpin->setValue(qBound(8, size, 24));

    // 缩进
    m_tabWidthSpin->setValue(cfg->value("Editor/TabWidth", 4).toInt());
    m_useSpacesCheck->setChecked(cfg->value("Editor/UseSpaces", true).toBool());

    // 显示
    m_showLineNumbersCheck->setChecked(
        cfg->value("Editor/ShowLineNumbers", true).toBool());
    m_highlightCurrentLineCheck->setChecked(
        cfg->value("Editor/HighlightCurrentLine", true).toBool());

    applyPreviewStyle();
}

void EditorSettingsPage::saveSettings()
{
    ConfigManager* cfg = Application::instance().configManager();
    if (!cfg) return;

    cfg->setValue("Editor/FontFamily", m_fontFamilyCombo->currentText());
    cfg->setValue("Editor/FontSize",   m_fontSizeSpin->value());
    cfg->setValue("Editor/TabWidth",   m_tabWidthSpin->value());
    cfg->setValue("Editor/UseSpaces",  m_useSpacesCheck->isChecked());
    cfg->setValue("Editor/ShowLineNumbers",
                  m_showLineNumbersCheck->isChecked());
    cfg->setValue("Editor/HighlightCurrentLine",
                  m_highlightCurrentLineCheck->isChecked());

    //   MainWindow 监听到后遍历所有编辑器重新应用
    ThemeManager::instance().setEditorFont(
        m_fontFamilyCombo->currentText(),
        m_fontSizeSpin->value());
}

} // namespace SprintIDE
