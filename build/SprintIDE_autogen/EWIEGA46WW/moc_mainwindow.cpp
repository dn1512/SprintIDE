/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../mainwindow.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.10.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN9SprintIDE10MainWindowE_t {};
} // unnamed namespace

template <> constexpr inline auto SprintIDE::MainWindow::qt_create_metaobjectdata<qt_meta_tag_ZN9SprintIDE10MainWindowE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "SprintIDE::MainWindow",
        "onStartDebug",
        "",
        "onStopDebug",
        "onStepOver",
        "onStepInto",
        "onStepOut",
        "onContinueDebug",
        "onAddBreakpoint",
        "onOutputPanelContextMenu",
        "QPoint",
        "pos",
        "clearOutputPanel",
        "onDebugStateChanged",
        "DebugState",
        "state",
        "onBreakpointHit",
        "filePath",
        "lineNumber",
        "onProgramStopped",
        "reason",
        "onProgramExited",
        "exitCode",
        "onDebugError",
        "error",
        "onBreakpointToggled",
        "line",
        "onPythonDebugFinished",
        "onFormatCode",
        "createNewEditorTab",
        "title",
        "closeEditorTab",
        "index",
        "closeEditorForFile",
        "updateTabTitle",
        "onTabCloseRequested",
        "onTabCurrentChanged",
        "onTabContextMenuRequested",
        "onEditorModificationChanged",
        "modified",
        "onEditorCursorPositionChanged",
        "column",
        "onFileLoaded",
        "onFileSaved",
        "onEditorTextChanged",
        "TextEditor*",
        "editor",
        "onProjectItemDoubleClicked",
        "QModelIndex",
        "onCompileStarted",
        "taskId",
        "sourceFile",
        "onCompileProgress",
        "progress",
        "currentFile",
        "onCompileOutput",
        "output",
        "isError",
        "onCompileFinished",
        "CompileResult",
        "result",
        "onCompileError",
        "errorMessage",
        "onCompilersDetected",
        "compilers",
        "stopCompile",
        "askRunProgram",
        "executablePath",
        "runProgram",
        "inTerminal",
        "syncBreakpointsToDebugger",
        "onLSPDiagnostics",
        "QList<LSPDiagnostic>",
        "diagnostics",
        "onNewFile",
        "onOpenFile",
        "onSaveFile",
        "onSaveAs",
        "onCloseFile",
        "onExit",
        "onUndo",
        "onRedo",
        "onCut",
        "onCopy",
        "onPaste",
        "onFind",
        "onReplace",
        "onComment",
        "onGotoLine",
        "onOpenProject",
        "onCloseProject",
        "onBuildProject",
        "onCleanBuild",
        "onCompile",
        "onRun",
        "findDebugger",
        "onCompileSettings",
        "onZoomIn",
        "onZoomOut",
        "onZoomReset",
        "onSettings",
        "onQuickStart",
        "onDocumentation",
        "onAbout",
        "onThemeChanged",
        "ThemeManager::ThemeType",
        "theme",
        "onThemeLight",
        "onThemeDark",
        "onCustomTheme",
        "createNewFileWithName",
        "defaultName",
        "customDir",
        "onFindNext",
        "text",
        "caseSensitive",
        "wholeWord",
        "onFindPrevious",
        "onReplaceCurrent",
        "findText",
        "replaceText",
        "onReplaceAll",
        "onDebuggerContinued"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'onStartDebug'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onStopDebug'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onStepOver'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onStepInto'
        QtMocHelpers::SlotData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onStepOut'
        QtMocHelpers::SlotData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onContinueDebug'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onAddBreakpoint'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onOutputPanelContextMenu'
        QtMocHelpers::SlotData<void(const QPoint &)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 10, 11 },
        }}),
        // Slot 'clearOutputPanel'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onDebugStateChanged'
        QtMocHelpers::SlotData<void(DebugState)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 14, 15 },
        }}),
        // Slot 'onBreakpointHit'
        QtMocHelpers::SlotData<void(const QString &, int)>(16, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 17 }, { QMetaType::Int, 18 },
        }}),
        // Slot 'onProgramStopped'
        QtMocHelpers::SlotData<void(const QString &, int, const QString &)>(19, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 17 }, { QMetaType::Int, 18 }, { QMetaType::QString, 20 },
        }}),
        // Slot 'onProgramExited'
        QtMocHelpers::SlotData<void(int)>(21, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 22 },
        }}),
        // Slot 'onDebugError'
        QtMocHelpers::SlotData<void(const QString &)>(23, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 24 },
        }}),
        // Slot 'onBreakpointToggled'
        QtMocHelpers::SlotData<void(const QString &, int)>(25, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 17 }, { QMetaType::Int, 26 },
        }}),
        // Slot 'onPythonDebugFinished'
        QtMocHelpers::SlotData<void()>(27, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onFormatCode'
        QtMocHelpers::SlotData<void()>(28, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'createNewEditorTab'
        QtMocHelpers::SlotData<void(const QString &)>(29, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 30 },
        }}),
        // Slot 'closeEditorTab'
        QtMocHelpers::SlotData<bool(int)>(31, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::Int, 32 },
        }}),
        // Slot 'closeEditorForFile'
        QtMocHelpers::SlotData<bool(const QString &)>(33, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 17 },
        }}),
        // Slot 'updateTabTitle'
        QtMocHelpers::SlotData<void(int, const QString &)>(34, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 32 }, { QMetaType::QString, 30 },
        }}),
        // Slot 'onTabCloseRequested'
        QtMocHelpers::SlotData<void(int)>(35, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 32 },
        }}),
        // Slot 'onTabCurrentChanged'
        QtMocHelpers::SlotData<void(int)>(36, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 32 },
        }}),
        // Slot 'onTabContextMenuRequested'
        QtMocHelpers::SlotData<void(int, const QPoint &)>(37, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 32 }, { 0x80000000 | 10, 11 },
        }}),
        // Slot 'onEditorModificationChanged'
        QtMocHelpers::SlotData<void(bool)>(38, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 39 },
        }}),
        // Slot 'onEditorCursorPositionChanged'
        QtMocHelpers::SlotData<void(int, int)>(40, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 26 }, { QMetaType::Int, 41 },
        }}),
        // Slot 'onFileLoaded'
        QtMocHelpers::SlotData<void(const QString &)>(42, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 17 },
        }}),
        // Slot 'onFileSaved'
        QtMocHelpers::SlotData<void(const QString &)>(43, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 17 },
        }}),
        // Slot 'onEditorTextChanged'
        QtMocHelpers::SlotData<void(TextEditor *)>(44, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 45, 46 },
        }}),
        // Slot 'onProjectItemDoubleClicked'
        QtMocHelpers::SlotData<void(const QModelIndex &)>(47, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 48, 32 },
        }}),
        // Slot 'onCompileStarted'
        QtMocHelpers::SlotData<void(const QString &, const QString &)>(49, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 50 }, { QMetaType::QString, 51 },
        }}),
        // Slot 'onCompileProgress'
        QtMocHelpers::SlotData<void(const QString &, int, const QString &)>(52, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 50 }, { QMetaType::Int, 53 }, { QMetaType::QString, 54 },
        }}),
        // Slot 'onCompileOutput'
        QtMocHelpers::SlotData<void(const QString &, const QString &, bool)>(55, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 50 }, { QMetaType::QString, 56 }, { QMetaType::Bool, 57 },
        }}),
        // Slot 'onCompileFinished'
        QtMocHelpers::SlotData<void(const QString &, const CompileResult &)>(58, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 50 }, { 0x80000000 | 59, 60 },
        }}),
        // Slot 'onCompileError'
        QtMocHelpers::SlotData<void(const QString &, const QString &)>(61, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 50 }, { QMetaType::QString, 62 },
        }}),
        // Slot 'onCompilersDetected'
        QtMocHelpers::SlotData<void(const QStringList &)>(63, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QStringList, 64 },
        }}),
        // Slot 'stopCompile'
        QtMocHelpers::SlotData<void()>(65, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'askRunProgram'
        QtMocHelpers::SlotData<void(const QString &)>(66, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 67 },
        }}),
        // Slot 'runProgram'
        QtMocHelpers::SlotData<void(const QString &, bool)>(68, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 67 }, { QMetaType::Bool, 69 },
        }}),
        // Slot 'runProgram'
        QtMocHelpers::SlotData<void(const QString &)>(68, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void, {{
            { QMetaType::QString, 67 },
        }}),
        // Slot 'syncBreakpointsToDebugger'
        QtMocHelpers::SlotData<void()>(70, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onLSPDiagnostics'
        QtMocHelpers::SlotData<void(const QString &, const QList<LSPDiagnostic> &)>(71, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 17 }, { 0x80000000 | 72, 73 },
        }}),
        // Slot 'onNewFile'
        QtMocHelpers::SlotData<void()>(74, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onOpenFile'
        QtMocHelpers::SlotData<void()>(75, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onSaveFile'
        QtMocHelpers::SlotData<void()>(76, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onSaveAs'
        QtMocHelpers::SlotData<void()>(77, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onCloseFile'
        QtMocHelpers::SlotData<void()>(78, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onExit'
        QtMocHelpers::SlotData<void()>(79, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onUndo'
        QtMocHelpers::SlotData<void()>(80, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onRedo'
        QtMocHelpers::SlotData<void()>(81, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onCut'
        QtMocHelpers::SlotData<void()>(82, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onCopy'
        QtMocHelpers::SlotData<void()>(83, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onPaste'
        QtMocHelpers::SlotData<void()>(84, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onFind'
        QtMocHelpers::SlotData<void()>(85, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onReplace'
        QtMocHelpers::SlotData<void()>(86, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onComment'
        QtMocHelpers::SlotData<void()>(87, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onGotoLine'
        QtMocHelpers::SlotData<void()>(88, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onOpenProject'
        QtMocHelpers::SlotData<void()>(89, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onCloseProject'
        QtMocHelpers::SlotData<void()>(90, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onBuildProject'
        QtMocHelpers::SlotData<void()>(91, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onCleanBuild'
        QtMocHelpers::SlotData<void()>(92, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onCompile'
        QtMocHelpers::SlotData<void()>(93, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onRun'
        QtMocHelpers::SlotData<void()>(94, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'findDebugger'
        QtMocHelpers::SlotData<QString()>(95, 2, QMC::AccessPublic, QMetaType::QString),
        // Slot 'onCompileSettings'
        QtMocHelpers::SlotData<void()>(96, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onZoomIn'
        QtMocHelpers::SlotData<void()>(97, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onZoomOut'
        QtMocHelpers::SlotData<void()>(98, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onZoomReset'
        QtMocHelpers::SlotData<void()>(99, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onSettings'
        QtMocHelpers::SlotData<void()>(100, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onQuickStart'
        QtMocHelpers::SlotData<void()>(101, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onDocumentation'
        QtMocHelpers::SlotData<void()>(102, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onAbout'
        QtMocHelpers::SlotData<void()>(103, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onThemeChanged'
        QtMocHelpers::SlotData<void(ThemeManager::ThemeType)>(104, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 105, 106 },
        }}),
        // Slot 'onThemeLight'
        QtMocHelpers::SlotData<void()>(107, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onThemeDark'
        QtMocHelpers::SlotData<void()>(108, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onCustomTheme'
        QtMocHelpers::SlotData<void()>(109, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'createNewFileWithName'
        QtMocHelpers::SlotData<void(const QString &, const QString &, const QString &)>(110, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 111 }, { QMetaType::QString, 30 }, { QMetaType::QString, 112 },
        }}),
        // Slot 'createNewFileWithName'
        QtMocHelpers::SlotData<void(const QString &, const QString &)>(110, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void, {{
            { QMetaType::QString, 111 }, { QMetaType::QString, 30 },
        }}),
        // Slot 'createNewFileWithName'
        QtMocHelpers::SlotData<void(const QString &)>(110, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void, {{
            { QMetaType::QString, 111 },
        }}),
        // Slot 'onFindNext'
        QtMocHelpers::SlotData<void(const QString &, bool, bool)>(113, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 114 }, { QMetaType::Bool, 115 }, { QMetaType::Bool, 116 },
        }}),
        // Slot 'onFindPrevious'
        QtMocHelpers::SlotData<void(const QString &, bool, bool)>(117, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 114 }, { QMetaType::Bool, 115 }, { QMetaType::Bool, 116 },
        }}),
        // Slot 'onReplaceCurrent'
        QtMocHelpers::SlotData<void(const QString &, const QString &, bool, bool)>(118, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 119 }, { QMetaType::QString, 120 }, { QMetaType::Bool, 115 }, { QMetaType::Bool, 116 },
        }}),
        // Slot 'onReplaceAll'
        QtMocHelpers::SlotData<void(const QString &, const QString &, bool, bool)>(121, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 119 }, { QMetaType::QString, 120 }, { QMetaType::Bool, 115 }, { QMetaType::Bool, 116 },
        }}),
        // Slot 'onDebuggerContinued'
        QtMocHelpers::SlotData<void()>(122, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<MainWindow, qt_meta_tag_ZN9SprintIDE10MainWindowE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject SprintIDE::MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE10MainWindowE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE10MainWindowE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN9SprintIDE10MainWindowE_t>.metaTypes,
    nullptr
} };

void SprintIDE::MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MainWindow *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->onStartDebug(); break;
        case 1: _t->onStopDebug(); break;
        case 2: _t->onStepOver(); break;
        case 3: _t->onStepInto(); break;
        case 4: _t->onStepOut(); break;
        case 5: _t->onContinueDebug(); break;
        case 6: _t->onAddBreakpoint(); break;
        case 7: _t->onOutputPanelContextMenu((*reinterpret_cast<std::add_pointer_t<QPoint>>(_a[1]))); break;
        case 8: _t->clearOutputPanel(); break;
        case 9: _t->onDebugStateChanged((*reinterpret_cast<std::add_pointer_t<DebugState>>(_a[1]))); break;
        case 10: _t->onBreakpointHit((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 11: _t->onProgramStopped((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3]))); break;
        case 12: _t->onProgramExited((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 13: _t->onDebugError((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 14: _t->onBreakpointToggled((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 15: _t->onPythonDebugFinished(); break;
        case 16: _t->onFormatCode(); break;
        case 17: _t->createNewEditorTab((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 18: { bool _r = _t->closeEditorTab((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 19: { bool _r = _t->closeEditorForFile((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 20: _t->updateTabTitle((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 21: _t->onTabCloseRequested((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 22: _t->onTabCurrentChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 23: _t->onTabContextMenuRequested((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QPoint>>(_a[2]))); break;
        case 24: _t->onEditorModificationChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 25: _t->onEditorCursorPositionChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 26: _t->onFileLoaded((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 27: _t->onFileSaved((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 28: _t->onEditorTextChanged((*reinterpret_cast<std::add_pointer_t<TextEditor*>>(_a[1]))); break;
        case 29: _t->onProjectItemDoubleClicked((*reinterpret_cast<std::add_pointer_t<QModelIndex>>(_a[1]))); break;
        case 30: _t->onCompileStarted((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 31: _t->onCompileProgress((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3]))); break;
        case 32: _t->onCompileOutput((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[3]))); break;
        case 33: _t->onCompileFinished((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<CompileResult>>(_a[2]))); break;
        case 34: _t->onCompileError((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 35: _t->onCompilersDetected((*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[1]))); break;
        case 36: _t->stopCompile(); break;
        case 37: _t->askRunProgram((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 38: _t->runProgram((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 39: _t->runProgram((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 40: _t->syncBreakpointsToDebugger(); break;
        case 41: _t->onLSPDiagnostics((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QList<LSPDiagnostic>>>(_a[2]))); break;
        case 42: _t->onNewFile(); break;
        case 43: _t->onOpenFile(); break;
        case 44: _t->onSaveFile(); break;
        case 45: _t->onSaveAs(); break;
        case 46: _t->onCloseFile(); break;
        case 47: _t->onExit(); break;
        case 48: _t->onUndo(); break;
        case 49: _t->onRedo(); break;
        case 50: _t->onCut(); break;
        case 51: _t->onCopy(); break;
        case 52: _t->onPaste(); break;
        case 53: _t->onFind(); break;
        case 54: _t->onReplace(); break;
        case 55: _t->onComment(); break;
        case 56: _t->onGotoLine(); break;
        case 57: _t->onOpenProject(); break;
        case 58: _t->onCloseProject(); break;
        case 59: _t->onBuildProject(); break;
        case 60: _t->onCleanBuild(); break;
        case 61: _t->onCompile(); break;
        case 62: _t->onRun(); break;
        case 63: { QString _r = _t->findDebugger();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 64: _t->onCompileSettings(); break;
        case 65: _t->onZoomIn(); break;
        case 66: _t->onZoomOut(); break;
        case 67: _t->onZoomReset(); break;
        case 68: _t->onSettings(); break;
        case 69: _t->onQuickStart(); break;
        case 70: _t->onDocumentation(); break;
        case 71: _t->onAbout(); break;
        case 72: _t->onThemeChanged((*reinterpret_cast<std::add_pointer_t<ThemeManager::ThemeType>>(_a[1]))); break;
        case 73: _t->onThemeLight(); break;
        case 74: _t->onThemeDark(); break;
        case 75: _t->onCustomTheme(); break;
        case 76: _t->createNewFileWithName((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3]))); break;
        case 77: _t->createNewFileWithName((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 78: _t->createNewFileWithName((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 79: _t->onFindNext((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[3]))); break;
        case 80: _t->onFindPrevious((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[3]))); break;
        case 81: _t->onReplaceCurrent((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[4]))); break;
        case 82: _t->onReplaceAll((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[4]))); break;
        case 83: _t->onDebuggerContinued(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 28:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< TextEditor* >(); break;
            }
            break;
        }
    }
}

const QMetaObject *SprintIDE::MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SprintIDE::MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE10MainWindowE_t>.strings))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int SprintIDE::MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 84)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 84;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 84)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 84;
    }
    return _id;
}
QT_WARNING_POP
