#ifndef PYTHON_RUN_CONFIG_WIDGET_H
#define PYTHON_RUN_CONFIG_WIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include "python_run_config.h"

QT_BEGIN_NAMESPACE
class QTableWidget;
QT_END_NAMESPACE

namespace SprintIDE {

class PythonRunConfigWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PythonRunConfigWidget(QWidget* parent = nullptr);
    ~PythonRunConfigWidget();  // 显式声明析构函数

    void setConfig(const PythonRunConfig& config);
    PythonRunConfig config() const;

signals:
    void configChanged();

private:
    QComboBox* m_entryTypeCombo;
    QLineEdit* m_entryPathEdit;
    QLineEdit* m_interpreterEdit;
    QLineEdit* m_workingDirEdit;
    QLineEdit* m_argsEdit;
    QTableWidget* m_envTable;
    QCheckBox* m_installDepsCheck;
};

} // namespace SprintIDE

#endif // PYTHON_RUN_CONFIG_WIDGET_H
