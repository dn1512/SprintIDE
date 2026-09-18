/****************************************************************************
** Meta object code from reading C++ file 'debug_panel.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../debug_panel.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'debug_panel.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN9SprintIDE10DebugPanelE_t {};
} // unnamed namespace

template <> constexpr inline auto SprintIDE::DebugPanel::qt_create_metaobjectdata<qt_meta_tag_ZN9SprintIDE10DebugPanelE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "SprintIDE::DebugPanel",
        "continueRequested",
        "",
        "stepOverRequested",
        "stepIntoRequested",
        "stepOutRequested",
        "stopRequested",
        "variablesExpandRequested",
        "variablesReference",
        "evaluateRequested",
        "expression",
        "onChildVariablesUpdated",
        "parentRef",
        "QList<DebugVariable>",
        "children",
        "onEvaluateResult",
        "result",
        "type",
        "onContinueClicked",
        "onStepOverClicked",
        "onStepIntoClicked",
        "onStepOutClicked",
        "onStopClicked",
        "onClearConsoleClicked",
        "onVariableItemExpanded",
        "QTreeWidgetItem*",
        "item",
        "onEvaluateInputEntered"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'continueRequested'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'stepOverRequested'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'stepIntoRequested'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'stepOutRequested'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'stopRequested'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'variablesExpandRequested'
        QtMocHelpers::SignalData<void(int)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 8 },
        }}),
        // Signal 'evaluateRequested'
        QtMocHelpers::SignalData<void(const QString &)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 10 },
        }}),
        // Slot 'onChildVariablesUpdated'
        QtMocHelpers::SlotData<void(int, const QList<DebugVariable> &)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 12 }, { 0x80000000 | 13, 14 },
        }}),
        // Slot 'onEvaluateResult'
        QtMocHelpers::SlotData<void(const QString &, const QString &, const QString &)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 10 }, { QMetaType::QString, 16 }, { QMetaType::QString, 17 },
        }}),
        // Slot 'onContinueClicked'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onStepOverClicked'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onStepIntoClicked'
        QtMocHelpers::SlotData<void()>(20, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onStepOutClicked'
        QtMocHelpers::SlotData<void()>(21, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onStopClicked'
        QtMocHelpers::SlotData<void()>(22, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onClearConsoleClicked'
        QtMocHelpers::SlotData<void()>(23, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onVariableItemExpanded'
        QtMocHelpers::SlotData<void(QTreeWidgetItem *)>(24, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 25, 26 },
        }}),
        // Slot 'onEvaluateInputEntered'
        QtMocHelpers::SlotData<void()>(27, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<DebugPanel, qt_meta_tag_ZN9SprintIDE10DebugPanelE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject SprintIDE::DebugPanel::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE10DebugPanelE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE10DebugPanelE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN9SprintIDE10DebugPanelE_t>.metaTypes,
    nullptr
} };

void SprintIDE::DebugPanel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<DebugPanel *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->continueRequested(); break;
        case 1: _t->stepOverRequested(); break;
        case 2: _t->stepIntoRequested(); break;
        case 3: _t->stepOutRequested(); break;
        case 4: _t->stopRequested(); break;
        case 5: _t->variablesExpandRequested((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 6: _t->evaluateRequested((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 7: _t->onChildVariablesUpdated((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QList<DebugVariable>>>(_a[2]))); break;
        case 8: _t->onEvaluateResult((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3]))); break;
        case 9: _t->onContinueClicked(); break;
        case 10: _t->onStepOverClicked(); break;
        case 11: _t->onStepIntoClicked(); break;
        case 12: _t->onStepOutClicked(); break;
        case 13: _t->onStopClicked(); break;
        case 14: _t->onClearConsoleClicked(); break;
        case 15: _t->onVariableItemExpanded((*reinterpret_cast<std::add_pointer_t<QTreeWidgetItem*>>(_a[1]))); break;
        case 16: _t->onEvaluateInputEntered(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (DebugPanel::*)()>(_a, &DebugPanel::continueRequested, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (DebugPanel::*)()>(_a, &DebugPanel::stepOverRequested, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (DebugPanel::*)()>(_a, &DebugPanel::stepIntoRequested, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (DebugPanel::*)()>(_a, &DebugPanel::stepOutRequested, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (DebugPanel::*)()>(_a, &DebugPanel::stopRequested, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (DebugPanel::*)(int )>(_a, &DebugPanel::variablesExpandRequested, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (DebugPanel::*)(const QString & )>(_a, &DebugPanel::evaluateRequested, 6))
            return;
    }
}

const QMetaObject *SprintIDE::DebugPanel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SprintIDE::DebugPanel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE10DebugPanelE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int SprintIDE::DebugPanel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 17)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 17;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 17)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 17;
    }
    return _id;
}

// SIGNAL 0
void SprintIDE::DebugPanel::continueRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void SprintIDE::DebugPanel::stepOverRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void SprintIDE::DebugPanel::stepIntoRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void SprintIDE::DebugPanel::stepOutRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void SprintIDE::DebugPanel::stopRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void SprintIDE::DebugPanel::variablesExpandRequested(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}

// SIGNAL 6
void SprintIDE::DebugPanel::evaluateRequested(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1);
}
QT_WARNING_POP
