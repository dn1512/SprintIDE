#ifndef EDITOR_SETTINGS_PAGE_H
#define EDITOR_SETTINGS_PAGE_H

#include <QWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPlainTextEdit>
#include "settings_dialog.h"   // SettingsPage 基类

class QScrollArea;
namespace SprintIDE {
/**
 * @brief 编辑器设置页面
 *
 * 布局：
 *   [字体]    字体族 / 字号 / (实时预览)
 *   [缩进]    Tab 宽度 / 使用空格
 *   [显示]    显示行号 / 高亮当前行
 *
 * 行距不出现在 UI 中——由 ThemeManager::kEditorLineHeightPercent 固定。
 */
class EditorSettingsPage : public SettingsPage
{
    Q_OBJECT
public:
    explicit EditorSettingsPage(QWidget *parent = nullptr);

    void loadSettings() override;
    void saveSettings() override;

private slots:
    void onFontFamilyChanged(int index);
    void onFontSizeChanged(int value);
    void onTabWidthChanged(int value);
    void onUseSpacesToggled(bool checked);
    void onShowLineNumbersToggled(bool checked);
    void onHighlightCurrentLineToggled(bool checked);

private:
    void setupUI();
    void buildFontFamilyCombo();
    void applyPreviewStyle();        // 预览区字体 + 行距
    void applyLineHeightToPreview(); // 与编辑器一致的行高设置
    void applyScrollAreaTheme(QScrollArea* scrollArea);   // v8.5

    // v8.5：滚动容器（供主题切换时刷新样式）
    QScrollArea*    m_scrollArea = nullptr;

    // 字体
    QComboBox*      m_fontFamilyCombo = nullptr;
    QSpinBox*       m_fontSizeSpin    = nullptr;

    // 缩进
    QSpinBox*       m_tabWidthSpin    = nullptr;
    QCheckBox*      m_useSpacesCheck  = nullptr;

    // 显示
    QCheckBox*      m_showLineNumbersCheck       = nullptr;
    QCheckBox*      m_highlightCurrentLineCheck  = nullptr;

    // 预览
    QPlainTextEdit* m_preview = nullptr;
};

} // namespace SprintIDE
#endif
