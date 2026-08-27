/****************************************************************************
** Meta object code from reading C++ file 'debug_manager.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../debug_manager.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'debug_manager.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN9SprintIDE12DebugManagerE_t {};
} // unnamed namespace

template <> constexpr inline auto SprintIDE::DebugManager::qt_create_metaobjectdata<qt_meta_tag_ZN9SprintIDE12DebugManagerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "SprintIDE::DebugManager",
        "stateChanged",
        "",
        "DebugState",
        "state",
        "debuggingStarted",
        "debuggingStopped",
        "breakpointHit",
        "filePath",
        "lineNumber",
        "programStopped",
        "reason",
        "programContinued",
        "programExited",
        "exitCode",
        "programOutput",
        "output",
        "isError",
        "variablesUpdated",
        "QList<DebugVariable>",
        "variables",
        "callStackUpdated",
        "QList<StackFrame>",
        "stack",
        "errorOccurred",
        "error",
        "onDebuggerReadyRead",
        "onDebuggerReadyReadError",
        "onDebuggerFinished",
        "QProcess::ExitStatus",
        "exitStatus",
        "onDebuggerErrorOccurred",
        "QProcess::ProcessError"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'stateChanged'
        QtMocHelpers::SignalData<void(DebugState)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'debuggingStarted'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'debuggingStopped'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'breakpointHit'
        QtMocHelpers::SignalData<void(const QString &, int)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 8 }, { QMetaType::Int, 9 },
        }}),
        // Signal 'programStopped'
        QtMocHelpers::SignalData<void(const QString &, int, const QString &)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 8 }, { QMetaType::Int, 9 }, { QMetaType::QString, 11 },
        }}),
        // Signal 'programContinued'
        QtMocHelpers::SignalData<void()>(12, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'programExited'
        QtMocHelpers::SignalData<void(int)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 14 },
        }}),
        // Signal 'programOutput'
        QtMocHelpers::SignalData<void(const QString &, bool)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 16 }, { QMetaType::Bool, 17 },
        }}),
        // Signal 'variablesUpdated'
        QtMocHelpers::SignalData<void(const QList<DebugVariable> &)>(18, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 19, 20 },
        }}),
        // Signal 'callStackUpdated'
        QtMocHelpers::SignalData<void(const QList<StackFrame> &)>(21, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 22, 23 },
        }}),
        // Signal 'errorOccurred'
        QtMocHelpers::SignalData<void(const QString &)>(24, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 25 },
        }}),
        // Slot 'onDebuggerReadyRead'
        QtMocHelpers::SlotData<void()>(26, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onDebuggerReadyReadError'
        QtMocHelpers::SlotData<void()>(27, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onDebuggerFinished'
        QtMocHelpers::SlotData<void(int, QProcess::ExitStatus)>(28, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 14 }, { 0x80000000 | 29, 30 },
        }}),
        // Slot 'onDebuggerErrorOccurred'
        QtMocHelpers::SlotData<void(QProcess::ProcessError)>(31, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 32, 25 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<DebugManager, qt_meta_tag_ZN9SprintIDE12DebugManagerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject SprintIDE::DebugManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE12DebugManagerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE12DebugManagerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN9SprintIDE12DebugManagerE_t>.metaTypes,
    nullptr
} };

void SprintIDE::DebugManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<DebugManager *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->stateChanged((*reinterpret_cast<std::add_pointer_t<DebugState>>(_a[1]))); break;
        case 1: _t->debuggingStarted(); break;
        case 2: _t->debuggingStopped(); break;
        case 3: _t->breakpointHit((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 4: _t->programStopped((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3]))); break;
        case 5: _t->programContinued(); break;
        case 6: _t->programExited((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 7: _t->programOutput((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 8: _t->variablesUpdated((*reinterpret_cast<std::add_pointer_t<QList<DebugVariable>>>(_a[1]))); break;
        case 9: _t->callStackUpdated((*reinterpret_cast<std::add_pointer_t<QList<StackFrame>>>(_a[1]))); break;
        case 10: _t->errorOccurred((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 11: _t->onDebuggerReadyRead(); break;
        case 12: _t->onDebuggerReadyReadError(); break;
        case 13: _t->onDebuggerFinished((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        case 14: _t->onDebuggerErrorOccurred((*reinterpret_cast<std::add_pointer_t<QProcess::ProcessError>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (DebugManager::*)(DebugState )>(_a, &DebugManager::stateChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (DebugManager::*)()>(_a, &DebugManager::debuggingStarted, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (DebugManager::*)()>(_a, &DebugManager::debuggingStopped, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (DebugManager::*)(const QString & , int )>(_a, &DebugManager::breakpointHit, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (DebugManager::*)(const QString & , int , const QString & )>(_a, &DebugManager::programStopped, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (DebugManager::*)()>(_a, &DebugManager::programContinued, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (DebugManager::*)(int )>(_a, &DebugManager::programExited, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (DebugManager::*)(const QString & , bool )>(_a, &DebugManager::programOutput, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (DebugManager::*)(const QList<DebugVariable> & )>(_a, &DebugManager::variablesUpdated, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (DebugManager::*)(const QList<StackFrame> & )>(_a, &DebugManager::callStackUpdated, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (DebugManager::*)(const QString & )>(_a, &DebugManager::errorOccurred, 10))
            return;
    }
}

const QMetaObject *SprintIDE::DebugManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SprintIDE::DebugManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE12DebugManagerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SprintIDE::DebugManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 15;
    }
    return _id;
}

// SIGNAL 0
void SprintIDE::DebugManager::stateChanged(DebugState _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void SprintIDE::DebugManager::debuggingStarted()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void SprintIDE::DebugManager::debuggingStopped()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void SprintIDE::DebugManager::breakpointHit(const QString & _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1, _t2);
}

// SIGNAL 4
void SprintIDE::DebugManager::programStopped(const QString & _t1, int _t2, const QString & _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1, _t2, _t3);
}

// SIGNAL 5
void SprintIDE::DebugManager::programContinued()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void SprintIDE::DebugManager::programExited(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1);
}

// SIGNAL 7
void SprintIDE::DebugManager::programOutput(const QString & _t1, bool _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1, _t2);
}

// SIGNAL 8
void SprintIDE::DebugManager::variablesUpdated(const QList<DebugVariable> & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 8, nullptr, _t1);
}

// SIGNAL 9
void SprintIDE::DebugManager::callStackUpdated(const QList<StackFrame> & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 9, nullptr, _t1);
}

// SIGNAL 10
void SprintIDE::DebugManager::errorOccurred(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 10, nullptr, _t1);
}
QT_WARNING_POP
