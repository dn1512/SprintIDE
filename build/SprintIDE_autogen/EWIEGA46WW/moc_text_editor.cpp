/****************************************************************************
** Meta object code from reading C++ file 'text_editor.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../text_editor.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'text_editor.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN9SprintIDE10TextEditorE_t {};
} // unnamed namespace

template <> constexpr inline auto SprintIDE::TextEditor::qt_create_metaobjectdata<qt_meta_tag_ZN9SprintIDE10TextEditorE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "SprintIDE::TextEditor",
        "modificationChanged",
        "",
        "modified",
        "cursorPositionChanged",
        "line",
        "column",
        "fileSaved",
        "filePath",
        "fileLoaded",
        "gotoLineRequested",
        "breakpointToggled",
        "findRequested",
        "replaceRequested",
        "formatRequested",
        "findBarVisibilityChanged",
        "visible",
        "replaceCompleted",
        "count",
        "completionRequested",
        "definitionRequested",
        "updateLineNumberAreaWidth",
        "highlightCurrentLine",
        "updateLineNumberArea",
        "QRect",
        "rect",
        "dy",
        "onCursorPositionChanged",
        "onTextChanged",
        "onGotoLineAction",
        "onErrorMarkerTimer",
        "onFindTextChanged",
        "text",
        "onFindNextClicked",
        "onFindPreviousClicked",
        "onReplaceTextChanged",
        "onReplaceClicked",
        "onReplaceAllClicked",
        "onToggleCaseSensitive",
        "checked",
        "onToggleWholeWord",
        "onToggleReplaceMode",
        "onCloseFindBar",
        "onCompletionItemClicked",
        "QListWidgetItem*",
        "item",
        "onCompletionItemDoubleClicked"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'modificationChanged'
        QtMocHelpers::SignalData<void(bool)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 3 },
        }}),
        // Signal 'cursorPositionChanged'
        QtMocHelpers::SignalData<void(int, int)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 5 }, { QMetaType::Int, 6 },
        }}),
        // Signal 'fileSaved'
        QtMocHelpers::SignalData<void(const QString &)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 8 },
        }}),
        // Signal 'fileLoaded'
        QtMocHelpers::SignalData<void(const QString &)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 8 },
        }}),
        // Signal 'gotoLineRequested'
        QtMocHelpers::SignalData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'breakpointToggled'
        QtMocHelpers::SignalData<void(const QString &, int)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 8 }, { QMetaType::Int, 5 },
        }}),
        // Signal 'findRequested'
        QtMocHelpers::SignalData<void()>(12, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'replaceRequested'
        QtMocHelpers::SignalData<void()>(13, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'formatRequested'
        QtMocHelpers::SignalData<void()>(14, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'findBarVisibilityChanged'
        QtMocHelpers::SignalData<void(bool)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 16 },
        }}),
        // Signal 'replaceCompleted'
        QtMocHelpers::SignalData<void(int)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 18 },
        }}),
        // Signal 'completionRequested'
        QtMocHelpers::SignalData<void(const QString &, int, int)>(19, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 8 }, { QMetaType::Int, 5 }, { QMetaType::Int, 6 },
        }}),
        // Signal 'definitionRequested'
        QtMocHelpers::SignalData<void(const QString &, int, int)>(20, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 8 }, { QMetaType::Int, 5 }, { QMetaType::Int, 6 },
        }}),
        // Slot 'updateLineNumberAreaWidth'
        QtMocHelpers::SlotData<void()>(21, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'highlightCurrentLine'
        QtMocHelpers::SlotData<void()>(22, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'updateLineNumberArea'
        QtMocHelpers::SlotData<void(const QRect &, int)>(23, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 24, 25 }, { QMetaType::Int, 26 },
        }}),
        // Slot 'onCursorPositionChanged'
        QtMocHelpers::SlotData<void()>(27, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onTextChanged'
        QtMocHelpers::SlotData<void()>(28, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onGotoLineAction'
        QtMocHelpers::SlotData<void()>(29, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onErrorMarkerTimer'
        QtMocHelpers::SlotData<void()>(30, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onFindTextChanged'
        QtMocHelpers::SlotData<void(const QString &)>(31, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 32 },
        }}),
        // Slot 'onFindNextClicked'
        QtMocHelpers::SlotData<void()>(33, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onFindPreviousClicked'
        QtMocHelpers::SlotData<void()>(34, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onReplaceTextChanged'
        QtMocHelpers::SlotData<void(const QString &)>(35, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 32 },
        }}),
        // Slot 'onReplaceClicked'
        QtMocHelpers::SlotData<void()>(36, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onReplaceAllClicked'
        QtMocHelpers::SlotData<void()>(37, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onToggleCaseSensitive'
        QtMocHelpers::SlotData<void(bool)>(38, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 39 },
        }}),
        // Slot 'onToggleWholeWord'
        QtMocHelpers::SlotData<void(bool)>(40, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 39 },
        }}),
        // Slot 'onToggleReplaceMode'
        QtMocHelpers::SlotData<void()>(41, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onCloseFindBar'
        QtMocHelpers::SlotData<void()>(42, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onCompletionItemClicked'
        QtMocHelpers::SlotData<void(QListWidgetItem *)>(43, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 44, 45 },
        }}),
        // Slot 'onCompletionItemDoubleClicked'
        QtMocHelpers::SlotData<void(QListWidgetItem *)>(46, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 44, 45 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<TextEditor, qt_meta_tag_ZN9SprintIDE10TextEditorE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject SprintIDE::TextEditor::staticMetaObject = { {
    QMetaObject::SuperData::link<QPlainTextEdit::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE10TextEditorE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE10TextEditorE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN9SprintIDE10TextEditorE_t>.metaTypes,
    nullptr
} };

void SprintIDE::TextEditor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<TextEditor *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->modificationChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 1: _t->cursorPositionChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 2: _t->fileSaved((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->fileLoaded((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->gotoLineRequested(); break;
        case 5: _t->breakpointToggled((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 6: _t->findRequested(); break;
        case 7: _t->replaceRequested(); break;
        case 8: _t->formatRequested(); break;
        case 9: _t->findBarVisibilityChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 10: _t->replaceCompleted((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 11: _t->completionRequested((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3]))); break;
        case 12: _t->definitionRequested((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3]))); break;
        case 13: _t->updateLineNumberAreaWidth(); break;
        case 14: _t->highlightCurrentLine(); break;
        case 15: _t->updateLineNumberArea((*reinterpret_cast<std::add_pointer_t<QRect>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 16: _t->onCursorPositionChanged(); break;
        case 17: _t->onTextChanged(); break;
        case 18: _t->onGotoLineAction(); break;
        case 19: _t->onErrorMarkerTimer(); break;
        case 20: _t->onFindTextChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 21: _t->onFindNextClicked(); break;
        case 22: _t->onFindPreviousClicked(); break;
        case 23: _t->onReplaceTextChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 24: _t->onReplaceClicked(); break;
        case 25: _t->onReplaceAllClicked(); break;
        case 26: _t->onToggleCaseSensitive((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 27: _t->onToggleWholeWord((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 28: _t->onToggleReplaceMode(); break;
        case 29: _t->onCloseFindBar(); break;
        case 30: _t->onCompletionItemClicked((*reinterpret_cast<std::add_pointer_t<QListWidgetItem*>>(_a[1]))); break;
        case 31: _t->onCompletionItemDoubleClicked((*reinterpret_cast<std::add_pointer_t<QListWidgetItem*>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (TextEditor::*)(bool )>(_a, &TextEditor::modificationChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (TextEditor::*)(int , int )>(_a, &TextEditor::cursorPositionChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (TextEditor::*)(const QString & )>(_a, &TextEditor::fileSaved, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (TextEditor::*)(const QString & )>(_a, &TextEditor::fileLoaded, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (TextEditor::*)()>(_a, &TextEditor::gotoLineRequested, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (TextEditor::*)(const QString & , int )>(_a, &TextEditor::breakpointToggled, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (TextEditor::*)()>(_a, &TextEditor::findRequested, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (TextEditor::*)()>(_a, &TextEditor::replaceRequested, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (TextEditor::*)()>(_a, &TextEditor::formatRequested, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (TextEditor::*)(bool )>(_a, &TextEditor::findBarVisibilityChanged, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (TextEditor::*)(int )>(_a, &TextEditor::replaceCompleted, 10))
            return;
        if (QtMocHelpers::indexOfMethod<void (TextEditor::*)(const QString & , int , int )>(_a, &TextEditor::completionRequested, 11))
            return;
        if (QtMocHelpers::indexOfMethod<void (TextEditor::*)(const QString & , int , int )>(_a, &TextEditor::definitionRequested, 12))
            return;
    }
}

const QMetaObject *SprintIDE::TextEditor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SprintIDE::TextEditor::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE10TextEditorE_t>.strings))
        return static_cast<void*>(this);
    return QPlainTextEdit::qt_metacast(_clname);
}

int SprintIDE::TextEditor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QPlainTextEdit::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 32)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 32;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 32)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 32;
    }
    return _id;
}

// SIGNAL 0
void SprintIDE::TextEditor::modificationChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void SprintIDE::TextEditor::cursorPositionChanged(int _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2);
}

// SIGNAL 2
void SprintIDE::TextEditor::fileSaved(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void SprintIDE::TextEditor::fileLoaded(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void SprintIDE::TextEditor::gotoLineRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void SprintIDE::TextEditor::breakpointToggled(const QString & _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1, _t2);
}

// SIGNAL 6
void SprintIDE::TextEditor::findRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void SprintIDE::TextEditor::replaceRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void SprintIDE::TextEditor::formatRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void SprintIDE::TextEditor::findBarVisibilityChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 9, nullptr, _t1);
}

// SIGNAL 10
void SprintIDE::TextEditor::replaceCompleted(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 10, nullptr, _t1);
}

// SIGNAL 11
void SprintIDE::TextEditor::completionRequested(const QString & _t1, int _t2, int _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 11, nullptr, _t1, _t2, _t3);
}

// SIGNAL 12
void SprintIDE::TextEditor::definitionRequested(const QString & _t1, int _t2, int _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 12, nullptr, _t1, _t2, _t3);
}
namespace {
struct qt_meta_tag_ZN9SprintIDE14LineNumberAreaE_t {};
} // unnamed namespace

template <> constexpr inline auto SprintIDE::LineNumberArea::qt_create_metaobjectdata<qt_meta_tag_ZN9SprintIDE14LineNumberAreaE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "SprintIDE::LineNumberArea"
    };

    QtMocHelpers::UintData qt_methods {
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<LineNumberArea, qt_meta_tag_ZN9SprintIDE14LineNumberAreaE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject SprintIDE::LineNumberArea::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE14LineNumberAreaE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE14LineNumberAreaE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN9SprintIDE14LineNumberAreaE_t>.metaTypes,
    nullptr
} };

void SprintIDE::LineNumberArea::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<LineNumberArea *>(_o);
    (void)_t;
    (void)_c;
    (void)_id;
    (void)_a;
}

const QMetaObject *SprintIDE::LineNumberArea::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SprintIDE::LineNumberArea::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE14LineNumberAreaE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int SprintIDE::LineNumberArea::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    return _id;
}
QT_WARNING_POP
