/********************************************************************************
** Form generated from reading UI file 'project_creation_dialog.ui'
**
** Created by: Qt User Interface Compiler version 6.10.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PROJECT_CREATION_DIALOG_H
#define UI_PROJECT_CREATION_DIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ProjectCreationDialog
{
public:
    QVBoxLayout *verticalLayout;
    QTabWidget *tabWidget;
    QWidget *basicTab;
    QVBoxLayout *verticalLayout_2;
    QGroupBox *projectInfoGroupBox;
    QFormLayout *formLayout;
    QLabel *projectNameLabel;
    QHBoxLayout *horizontalLayout;
    QLineEdit *projectNameLineEdit;
    QLabel *displayNameLabel;
    QLineEdit *displayNameLineEdit;
    QLabel *locationLabel;
    QHBoxLayout *horizontalLayout_2;
    QLineEdit *locationLineEdit;
    QPushButton *browseButton;
    QLabel *pathPreviewLabel;
    QLabel *templateLabel;
    QComboBox *templateComboBox;
    QGroupBox *templateInfoGroupBox;
    QVBoxLayout *verticalLayout_3;
    QLabel *templateDescriptionLabel;
    QLabel *templateFilesLabel;
    QHBoxLayout *horizontalLayout_3;
    QPushButton *exampleCodeButton;
    QSpacerItem *horizontalSpacer;
    QWidget *compileTab;
    QVBoxLayout *verticalLayout_4;
    QGroupBox *compileModeGroupBox;
    QVBoxLayout *verticalLayout_5;
    QRadioButton *simpleModeRadioButton;
    QRadioButton *advancedModeRadioButton;
    QGroupBox *advancedConfigGroupBox;
    QFormLayout *formLayout_2;
    QLabel *compilerLabel;
    QComboBox *compilerComboBox;
    QLabel *cppStandardLabel;
    QComboBox *cppStandardComboBox;
    QLabel *optimizationLabel;
    QComboBox *optimizationComboBox;
    QLabel *warningLevelLabel;
    QComboBox *warningLevelComboBox;
    QLabel *debugInfoLabel;
    QCheckBox *debugInfoCheckBox;
    QPushButton *advancedConfigButton;
    QSpacerItem *verticalSpacer;
    QWidget *optionsTab;
    QVBoxLayout *verticalLayout_6;
    QGroupBox *advancedOptionsGroupBox;
    QVBoxLayout *verticalLayout_7;
    QCheckBox *autoOpenCheckBox;
    QCheckBox *createGitRepoCheckBox;
    QCheckBox *gitIgnoreCheckBox;
    QSpacerItem *verticalSpacer_2;
    QLabel *projectNameErrorLabel;
    QHBoxLayout *horizontalLayout_4;
    QPushButton *resetButton;
    QSpacerItem *horizontalSpacer_2;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *ProjectCreationDialog)
    {
        if (ProjectCreationDialog->objectName().isEmpty())
            ProjectCreationDialog->setObjectName("ProjectCreationDialog");
        ProjectCreationDialog->resize(800, 600);
        verticalLayout = new QVBoxLayout(ProjectCreationDialog);
        verticalLayout->setObjectName("verticalLayout");
        tabWidget = new QTabWidget(ProjectCreationDialog);
        tabWidget->setObjectName("tabWidget");
        basicTab = new QWidget();
        basicTab->setObjectName("basicTab");
        verticalLayout_2 = new QVBoxLayout(basicTab);
        verticalLayout_2->setObjectName("verticalLayout_2");
        projectInfoGroupBox = new QGroupBox(basicTab);
        projectInfoGroupBox->setObjectName("projectInfoGroupBox");
        formLayout = new QFormLayout(projectInfoGroupBox);
        formLayout->setObjectName("formLayout");
        projectNameLabel = new QLabel(projectInfoGroupBox);
        projectNameLabel->setObjectName("projectNameLabel");

        formLayout->setWidget(0, QFormLayout::ItemRole::LabelRole, projectNameLabel);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        projectNameLineEdit = new QLineEdit(projectInfoGroupBox);
        projectNameLineEdit->setObjectName("projectNameLineEdit");

        horizontalLayout->addWidget(projectNameLineEdit);


        formLayout->setLayout(0, QFormLayout::ItemRole::FieldRole, horizontalLayout);

        displayNameLabel = new QLabel(projectInfoGroupBox);
        displayNameLabel->setObjectName("displayNameLabel");

        formLayout->setWidget(1, QFormLayout::ItemRole::LabelRole, displayNameLabel);

        displayNameLineEdit = new QLineEdit(projectInfoGroupBox);
        displayNameLineEdit->setObjectName("displayNameLineEdit");

        formLayout->setWidget(1, QFormLayout::ItemRole::FieldRole, displayNameLineEdit);

        locationLabel = new QLabel(projectInfoGroupBox);
        locationLabel->setObjectName("locationLabel");

        formLayout->setWidget(2, QFormLayout::ItemRole::LabelRole, locationLabel);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        locationLineEdit = new QLineEdit(projectInfoGroupBox);
        locationLineEdit->setObjectName("locationLineEdit");

        horizontalLayout_2->addWidget(locationLineEdit);

        browseButton = new QPushButton(projectInfoGroupBox);
        browseButton->setObjectName("browseButton");

        horizontalLayout_2->addWidget(browseButton);


        formLayout->setLayout(2, QFormLayout::ItemRole::FieldRole, horizontalLayout_2);

        pathPreviewLabel = new QLabel(projectInfoGroupBox);
        pathPreviewLabel->setObjectName("pathPreviewLabel");
        pathPreviewLabel->setWordWrap(true);

        formLayout->setWidget(3, QFormLayout::ItemRole::FieldRole, pathPreviewLabel);

        templateLabel = new QLabel(projectInfoGroupBox);
        templateLabel->setObjectName("templateLabel");

        formLayout->setWidget(4, QFormLayout::ItemRole::LabelRole, templateLabel);

        templateComboBox = new QComboBox(projectInfoGroupBox);
        templateComboBox->setObjectName("templateComboBox");

        formLayout->setWidget(4, QFormLayout::ItemRole::FieldRole, templateComboBox);


        verticalLayout_2->addWidget(projectInfoGroupBox);

        templateInfoGroupBox = new QGroupBox(basicTab);
        templateInfoGroupBox->setObjectName("templateInfoGroupBox");
        verticalLayout_3 = new QVBoxLayout(templateInfoGroupBox);
        verticalLayout_3->setObjectName("verticalLayout_3");
        templateDescriptionLabel = new QLabel(templateInfoGroupBox);
        templateDescriptionLabel->setObjectName("templateDescriptionLabel");
        templateDescriptionLabel->setWordWrap(true);

        verticalLayout_3->addWidget(templateDescriptionLabel);

        templateFilesLabel = new QLabel(templateInfoGroupBox);
        templateFilesLabel->setObjectName("templateFilesLabel");
        templateFilesLabel->setWordWrap(true);

        verticalLayout_3->addWidget(templateFilesLabel);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        exampleCodeButton = new QPushButton(templateInfoGroupBox);
        exampleCodeButton->setObjectName("exampleCodeButton");

        horizontalLayout_3->addWidget(exampleCodeButton);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer);


        verticalLayout_3->addLayout(horizontalLayout_3);


        verticalLayout_2->addWidget(templateInfoGroupBox);

        tabWidget->addTab(basicTab, QString());
        compileTab = new QWidget();
        compileTab->setObjectName("compileTab");
        verticalLayout_4 = new QVBoxLayout(compileTab);
        verticalLayout_4->setObjectName("verticalLayout_4");
        compileModeGroupBox = new QGroupBox(compileTab);
        compileModeGroupBox->setObjectName("compileModeGroupBox");
        verticalLayout_5 = new QVBoxLayout(compileModeGroupBox);
        verticalLayout_5->setObjectName("verticalLayout_5");
        simpleModeRadioButton = new QRadioButton(compileModeGroupBox);
        simpleModeRadioButton->setObjectName("simpleModeRadioButton");
        simpleModeRadioButton->setChecked(true);

        verticalLayout_5->addWidget(simpleModeRadioButton);

        advancedModeRadioButton = new QRadioButton(compileModeGroupBox);
        advancedModeRadioButton->setObjectName("advancedModeRadioButton");

        verticalLayout_5->addWidget(advancedModeRadioButton);


        verticalLayout_4->addWidget(compileModeGroupBox);

        advancedConfigGroupBox = new QGroupBox(compileTab);
        advancedConfigGroupBox->setObjectName("advancedConfigGroupBox");
        advancedConfigGroupBox->setCheckable(false);
        formLayout_2 = new QFormLayout(advancedConfigGroupBox);
        formLayout_2->setObjectName("formLayout_2");
        compilerLabel = new QLabel(advancedConfigGroupBox);
        compilerLabel->setObjectName("compilerLabel");

        formLayout_2->setWidget(0, QFormLayout::ItemRole::LabelRole, compilerLabel);

        compilerComboBox = new QComboBox(advancedConfigGroupBox);
        compilerComboBox->setObjectName("compilerComboBox");

        formLayout_2->setWidget(0, QFormLayout::ItemRole::FieldRole, compilerComboBox);

        cppStandardLabel = new QLabel(advancedConfigGroupBox);
        cppStandardLabel->setObjectName("cppStandardLabel");

        formLayout_2->setWidget(1, QFormLayout::ItemRole::LabelRole, cppStandardLabel);

        cppStandardComboBox = new QComboBox(advancedConfigGroupBox);
        cppStandardComboBox->setObjectName("cppStandardComboBox");

        formLayout_2->setWidget(1, QFormLayout::ItemRole::FieldRole, cppStandardComboBox);

        optimizationLabel = new QLabel(advancedConfigGroupBox);
        optimizationLabel->setObjectName("optimizationLabel");

        formLayout_2->setWidget(2, QFormLayout::ItemRole::LabelRole, optimizationLabel);

        optimizationComboBox = new QComboBox(advancedConfigGroupBox);
        optimizationComboBox->setObjectName("optimizationComboBox");

        formLayout_2->setWidget(2, QFormLayout::ItemRole::FieldRole, optimizationComboBox);

        warningLevelLabel = new QLabel(advancedConfigGroupBox);
        warningLevelLabel->setObjectName("warningLevelLabel");

        formLayout_2->setWidget(3, QFormLayout::ItemRole::LabelRole, warningLevelLabel);

        warningLevelComboBox = new QComboBox(advancedConfigGroupBox);
        warningLevelComboBox->setObjectName("warningLevelComboBox");

        formLayout_2->setWidget(3, QFormLayout::ItemRole::FieldRole, warningLevelComboBox);

        debugInfoLabel = new QLabel(advancedConfigGroupBox);
        debugInfoLabel->setObjectName("debugInfoLabel");

        formLayout_2->setWidget(4, QFormLayout::ItemRole::LabelRole, debugInfoLabel);

        debugInfoCheckBox = new QCheckBox(advancedConfigGroupBox);
        debugInfoCheckBox->setObjectName("debugInfoCheckBox");
        debugInfoCheckBox->setChecked(true);

        formLayout_2->setWidget(4, QFormLayout::ItemRole::FieldRole, debugInfoCheckBox);


        verticalLayout_4->addWidget(advancedConfigGroupBox);

        advancedConfigButton = new QPushButton(compileTab);
        advancedConfigButton->setObjectName("advancedConfigButton");

        verticalLayout_4->addWidget(advancedConfigButton);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_4->addItem(verticalSpacer);

        tabWidget->addTab(compileTab, QString());
        optionsTab = new QWidget();
        optionsTab->setObjectName("optionsTab");
        verticalLayout_6 = new QVBoxLayout(optionsTab);
        verticalLayout_6->setObjectName("verticalLayout_6");
        advancedOptionsGroupBox = new QGroupBox(optionsTab);
        advancedOptionsGroupBox->setObjectName("advancedOptionsGroupBox");
        verticalLayout_7 = new QVBoxLayout(advancedOptionsGroupBox);
        verticalLayout_7->setObjectName("verticalLayout_7");
        autoOpenCheckBox = new QCheckBox(advancedOptionsGroupBox);
        autoOpenCheckBox->setObjectName("autoOpenCheckBox");
        autoOpenCheckBox->setChecked(true);

        verticalLayout_7->addWidget(autoOpenCheckBox);

        createGitRepoCheckBox = new QCheckBox(advancedOptionsGroupBox);
        createGitRepoCheckBox->setObjectName("createGitRepoCheckBox");

        verticalLayout_7->addWidget(createGitRepoCheckBox);

        gitIgnoreCheckBox = new QCheckBox(advancedOptionsGroupBox);
        gitIgnoreCheckBox->setObjectName("gitIgnoreCheckBox");
        gitIgnoreCheckBox->setChecked(true);

        verticalLayout_7->addWidget(gitIgnoreCheckBox);


        verticalLayout_6->addWidget(advancedOptionsGroupBox);

        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_6->addItem(verticalSpacer_2);

        tabWidget->addTab(optionsTab, QString());

        verticalLayout->addWidget(tabWidget);

        projectNameErrorLabel = new QLabel(ProjectCreationDialog);
        projectNameErrorLabel->setObjectName("projectNameErrorLabel");
        projectNameErrorLabel->setStyleSheet(QString::fromUtf8("color: red;"));
        projectNameErrorLabel->setVisible(false);

        verticalLayout->addWidget(projectNameErrorLabel);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName("horizontalLayout_4");
        resetButton = new QPushButton(ProjectCreationDialog);
        resetButton->setObjectName("resetButton");

        horizontalLayout_4->addWidget(resetButton);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_2);

        buttonBox = new QDialogButtonBox(ProjectCreationDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        horizontalLayout_4->addWidget(buttonBox);


        verticalLayout->addLayout(horizontalLayout_4);


        retranslateUi(ProjectCreationDialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, ProjectCreationDialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, ProjectCreationDialog, qOverload<>(&QDialog::reject));

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(ProjectCreationDialog);
    } // setupUi

    void retranslateUi(QDialog *ProjectCreationDialog)
    {
        ProjectCreationDialog->setWindowTitle(QCoreApplication::translate("ProjectCreationDialog", "\345\210\233\345\273\272\346\226\260\351\241\271\347\233\256", nullptr));
        projectInfoGroupBox->setTitle(QCoreApplication::translate("ProjectCreationDialog", "\351\241\271\347\233\256\344\277\241\346\201\257", nullptr));
        projectNameLabel->setText(QCoreApplication::translate("ProjectCreationDialog", "\351\241\271\347\233\256\345\220\215\347\247\260:", nullptr));
        displayNameLabel->setText(QCoreApplication::translate("ProjectCreationDialog", "\346\230\276\347\244\272\345\220\215\347\247\260:", nullptr));
        locationLabel->setText(QCoreApplication::translate("ProjectCreationDialog", "\351\241\271\347\233\256\344\275\215\347\275\256:", nullptr));
        browseButton->setText(QCoreApplication::translate("ProjectCreationDialog", "\346\265\217\350\247\210...", nullptr));
        pathPreviewLabel->setText(QCoreApplication::translate("ProjectCreationDialog", "\350\267\257\345\276\204\351\242\204\350\247\210", nullptr));
        templateLabel->setText(QCoreApplication::translate("ProjectCreationDialog", "\351\241\271\347\233\256\346\250\241\346\235\277:", nullptr));
        templateInfoGroupBox->setTitle(QCoreApplication::translate("ProjectCreationDialog", "\346\250\241\346\235\277\344\277\241\346\201\257", nullptr));
        templateDescriptionLabel->setText(QCoreApplication::translate("ProjectCreationDialog", "\346\250\241\346\235\277\346\217\217\350\277\260", nullptr));
        templateFilesLabel->setText(QCoreApplication::translate("ProjectCreationDialog", "\346\226\207\344\273\266\345\210\227\350\241\250", nullptr));
        exampleCodeButton->setText(QCoreApplication::translate("ProjectCreationDialog", "\346\237\245\347\234\213\347\244\272\344\276\213\344\273\243\347\240\201", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(basicTab), QCoreApplication::translate("ProjectCreationDialog", "\345\237\272\346\234\254\344\277\241\346\201\257", nullptr));
        compileModeGroupBox->setTitle(QCoreApplication::translate("ProjectCreationDialog", "\347\274\226\350\257\221\346\250\241\345\274\217", nullptr));
        simpleModeRadioButton->setText(QCoreApplication::translate("ProjectCreationDialog", "\347\256\200\345\215\225\346\250\241\345\274\217 (\346\216\250\350\215\220\345\210\235\345\255\246\350\200\205\344\275\277\347\224\250\357\274\214\350\207\252\345\212\250\351\205\215\347\275\256\346\234\200\344\275\263\347\274\226\350\257\221\351\200\211\351\241\271)", nullptr));
        advancedModeRadioButton->setText(QCoreApplication::translate("ProjectCreationDialog", "\351\253\230\347\272\247\346\250\241\345\274\217 (\345\205\201\350\256\270\350\207\252\345\256\232\344\271\211\346\211\200\346\234\211\347\274\226\350\257\221\351\200\211\351\241\271\357\274\214\351\200\202\345\220\210\346\234\211\347\273\217\351\252\214\347\232\204\345\255\246\344\271\240\350\200\205)", nullptr));
        advancedConfigGroupBox->setTitle(QCoreApplication::translate("ProjectCreationDialog", "\351\253\230\347\272\247\347\274\226\350\257\221\351\205\215\347\275\256", nullptr));
        compilerLabel->setText(QCoreApplication::translate("ProjectCreationDialog", "\347\274\226\350\257\221\345\231\250:", nullptr));
        cppStandardLabel->setText(QCoreApplication::translate("ProjectCreationDialog", "C++\346\240\207\345\207\206:", nullptr));
        optimizationLabel->setText(QCoreApplication::translate("ProjectCreationDialog", "\344\274\230\345\214\226\347\272\247\345\210\253:", nullptr));
        warningLevelLabel->setText(QCoreApplication::translate("ProjectCreationDialog", "\350\255\246\345\221\212\347\272\247\345\210\253:", nullptr));
        debugInfoLabel->setText(QCoreApplication::translate("ProjectCreationDialog", "\350\260\203\350\257\225\344\277\241\346\201\257:", nullptr));
        debugInfoCheckBox->setText(QCoreApplication::translate("ProjectCreationDialog", "\345\214\205\345\220\253\350\260\203\350\257\225\344\277\241\346\201\257 (\344\276\277\344\272\216\350\260\203\350\257\225\347\250\213\345\272\217)", nullptr));
        advancedConfigButton->setText(QCoreApplication::translate("ProjectCreationDialog", "\346\233\264\345\244\232\351\253\230\347\272\247\351\205\215\347\275\256...", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(compileTab), QCoreApplication::translate("ProjectCreationDialog", "\347\274\226\350\257\221\350\256\276\347\275\256", nullptr));
        advancedOptionsGroupBox->setTitle(QCoreApplication::translate("ProjectCreationDialog", "\351\241\271\347\233\256\351\200\211\351\241\271", nullptr));
        autoOpenCheckBox->setText(QCoreApplication::translate("ProjectCreationDialog", "\345\210\233\345\273\272\345\220\216\350\207\252\345\212\250\346\211\223\345\274\200\351\241\271\347\233\256", nullptr));
        createGitRepoCheckBox->setText(QCoreApplication::translate("ProjectCreationDialog", "\345\210\233\345\273\272Git\344\273\223\345\272\223 (\351\234\200\350\246\201\345\256\211\350\243\205Git)", nullptr));
        gitIgnoreCheckBox->setText(QCoreApplication::translate("ProjectCreationDialog", "\346\267\273\345\212\240.gitignore\346\226\207\344\273\266 (\345\277\275\347\225\245\347\274\226\350\257\221\350\276\223\345\207\272\347\255\211\346\226\207\344\273\266)", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(optionsTab), QCoreApplication::translate("ProjectCreationDialog", "\351\253\230\347\272\247\351\200\211\351\241\271", nullptr));
        projectNameErrorLabel->setText(QString());
        resetButton->setText(QCoreApplication::translate("ProjectCreationDialog", "\351\207\215\347\275\256\344\270\272\351\273\230\350\256\244\350\256\276\347\275\256", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ProjectCreationDialog: public Ui_ProjectCreationDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PROJECT_CREATION_DIALOG_H
