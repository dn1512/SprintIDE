// src/deps/toolchain_dialog.h
#ifndef TOOLCHAIN_DIALOG_H
#define TOOLCHAIN_DIALOG_H

#include <QDialog>
#include "toolchain_types.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QPlainTextEdit;
class QPushButton;
QT_END_NAMESPACE

namespace SprintIDE {

/**
 * @brief 工具链安装引导对话框
 *
 * 当 ToolchainManager 无法解析某个工具时，弹出此对话框：
 *   - 显示标题、说明
 *   - 显示安装命令（只读文本框，便于复制）
 *   - 提供 "复制命令" / "我已安装，重新检测" / "稍后" 三个按钮
 *
 * 用户点击 "我已安装" → dialog accept → Manager 重新 resolve
 */
class ToolchainDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ToolchainDialog(const ToolchainInstallHint& hint,
                             ToolchainId id,
                             QWidget* parent = nullptr);
    ~ToolchainDialog() override;

private slots:
    void onCopyCommand();
    void onOpenDetailUrl();

private:
    void setupUi(const ToolchainInstallHint& hint);

    ToolchainId m_id;
    QString m_command;

    QLabel* m_titleLabel = nullptr;
    QLabel* m_descLabel = nullptr;
    QPlainTextEdit* m_commandEdit = nullptr;
    QPushButton* m_copyButton = nullptr;
    QPushButton* m_recheckButton = nullptr;
    QPushButton* m_laterButton = nullptr;
    QPushButton* m_detailButton = nullptr;
};

} // namespace SprintIDE

#endif // TOOLCHAIN_DIALOG_H
