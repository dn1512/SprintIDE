#include "python_run_config_widget.h"
#include <QVBoxLayout>
#include <QLabel>

namespace SprintIDE {

PythonRunConfigWidget::PythonRunConfigWidget(QWidget* parent)
    : QWidget(parent)
{
    // 占位：简单显示一个提示，实际功能后续实现
    QVBoxLayout* layout = new QVBoxLayout(this);
    QLabel* label = new QLabel("Python 运行配置 (开发中)", this);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
}

PythonRunConfigWidget::~PythonRunConfigWidget()
{
    // 目前无需清理资源
}

} // namespace SprintIDE
