/****************************************************************************
** Meta object code from reading C++ file 'cpp_debugger.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../cpp/cpp_debugger.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'cpp_debugger.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN9SprintIDE11CppDebuggerE_t {};
} // unnamed namespace

template <> constexpr inline auto SprintIDE::CppDebugger::qt_create_metaobjectdata<qt_meta_tag_ZN9SprintIDE11CppDebuggerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "SprintIDE::CppDebugger",
        "stopped",
        "",
        "file",
        "line",
        "reason",
        "continued",
        "exited",
        "exitCode",
        "terminated",
        "outputReceived",
        "output",
        "isError",
        "errorOccurred",
        "error",
        "variablesUpdated",
        "QList<DebugVariable>",
        "variables",
        "stackUpdated",
        "QList<StackFrame>",
        "frames",
        "onAdapterInitialized",
        "onDapStopped",
        "SprintIDE::Dap::StoppedInfo",
        "info",
        "onDapContinued",
        "threadId",
        "onDapExited",
        "onDapTerminated",
        "onDapOutput",
        "category",
        "text",
        "onStackTrace",
        "QList<SprintIDE::Dap::StackFrame>",
        "onScopes",
        "frameId",
        "QList<SprintIDE::Dap::Scope>",
        "scopes",
        "onVariables",
        "parentRef",
        "QList<SprintIDE::Dap::Variable>",
        "vars",
        "onAdapterError",
        "onAdapterExited"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'stopped'
        QtMocHelpers::SignalData<void(const QString &, int, const QString &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 }, { QMetaType::Int, 4 }, { QMetaType::QString, 5 },
        }}),
        // Signal 'continued'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'exited'
        QtMocHelpers::SignalData<void(int)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 8 },
        }}),
        // Signal 'terminated'
        QtMocHelpers::SignalData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'outputReceived'
        QtMocHelpers::SignalData<void(const QString &, bool)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 11 }, { QMetaType::Bool, 12 },
        }}),
        // Signal 'errorOccurred'
        QtMocHelpers::SignalData<void(const QString &)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 14 },
        }}),
        // Signal 'variablesUpdated'
        QtMocHelpers::SignalData<void(const QList<DebugVariable> &)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 16, 17 },
        }}),
        // Signal 'stackUpdated'
        QtMocHelpers::SignalData<void(const QList<StackFrame> &)>(18, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 19, 20 },
        }}),
        // Slot 'onAdapterInitialized'
        QtMocHelpers::SlotData<void()>(21, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onDapStopped'
        QtMocHelpers::SlotData<void(const SprintIDE::Dap::StoppedInfo &)>(22, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 23, 24 },
        }}),
        // Slot 'onDapContinued'
        QtMocHelpers::SlotData<void(int)>(25, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 26 },
        }}),
        // Slot 'onDapExited'
        QtMocHelpers::SlotData<void(int)>(27, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 8 },
        }}),
        // Slot 'onDapTerminated'
        QtMocHelpers::SlotData<void()>(28, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onDapOutput'
        QtMocHelpers::SlotData<void(const QString &, const QString &)>(29, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 30 }, { QMetaType::QString, 31 },
        }}),
        // Slot 'onStackTrace'
        QtMocHelpers::SlotData<void(const QList<SprintIDE::Dap::StackFrame> &)>(32, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 33, 20 },
        }}),
        // Slot 'onScopes'
        QtMocHelpers::SlotData<void(int, const QList<SprintIDE::Dap::Scope> &)>(34, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 35 }, { 0x80000000 | 36, 37 },
        }}),
        // Slot 'onVariables'
        QtMocHelpers::SlotData<void(int, const QList<SprintIDE::Dap::Variable> &)>(38, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 39 }, { 0x80000000 | 40, 41 },
        }}),
        // Slot 'onAdapterError'
        QtMocHelpers::SlotData<void(const QString &)>(42, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 14 },
        }}),
        // Slot 'onAdapterExited'
        QtMocHelpers::SlotData<void(int)>(43, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 8 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<CppDebugger, qt_meta_tag_ZN9SprintIDE11CppDebuggerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject SprintIDE::CppDebugger::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE11CppDebuggerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE11CppDebuggerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN9SprintIDE11CppDebuggerE_t>.metaTypes,
    nullptr
} };

void SprintIDE::CppDebugger::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<CppDebugger *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->stopped((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3]))); break;
        case 1: _t->continued(); break;
        case 2: _t->exited((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 3: _t->terminated(); break;
        case 4: _t->outputReceived((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 5: _t->errorOccurred((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 6: _t->variablesUpdated((*reinterpret_cast<std::add_pointer_t<QList<DebugVariable>>>(_a[1]))); break;
        case 7: _t->stackUpdated((*reinterpret_cast<std::add_pointer_t<QList<StackFrame>>>(_a[1]))); break;
        case 8: _t->onAdapterInitialized(); break;
        case 9: _t->onDapStopped((*reinterpret_cast<std::add_pointer_t<SprintIDE::Dap::StoppedInfo>>(_a[1]))); break;
        case 10: _t->onDapContinued((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 11: _t->onDapExited((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 12: _t->onDapTerminated(); break;
        case 13: _t->onDapOutput((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 14: _t->onStackTrace((*reinterpret_cast<std::add_pointer_t<QList<SprintIDE::Dap::StackFrame>>>(_a[1]))); break;
        case 15: _t->onScopes((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QList<SprintIDE::Dap::Scope>>>(_a[2]))); break;
        case 16: _t->onVariables((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QList<SprintIDE::Dap::Variable>>>(_a[2]))); break;
        case 17: _t->onAdapterError((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 18: _t->onAdapterExited((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (CppDebugger::*)(const QString & , int , const QString & )>(_a, &CppDebugger::stopped, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (CppDebugger::*)()>(_a, &CppDebugger::continued, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (CppDebugger::*)(int )>(_a, &CppDebugger::exited, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (CppDebugger::*)()>(_a, &CppDebugger::terminated, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (CppDebugger::*)(const QString & , bool )>(_a, &CppDebugger::outputReceived, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (CppDebugger::*)(const QString & )>(_a, &CppDebugger::errorOccurred, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (CppDebugger::*)(const QList<DebugVariable> & )>(_a, &CppDebugger::variablesUpdated, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (CppDebugger::*)(const QList<StackFrame> & )>(_a, &CppDebugger::stackUpdated, 7))
            return;
    }
}

const QMetaObject *SprintIDE::CppDebugger::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SprintIDE::CppDebugger::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE11CppDebuggerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SprintIDE::CppDebugger::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 19)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 19;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 19)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 19;
    }
    return _id;
}

// SIGNAL 0
void SprintIDE::CppDebugger::stopped(const QString & _t1, int _t2, const QString & _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2, _t3);
}

// SIGNAL 1
void SprintIDE::CppDebugger::continued()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void SprintIDE::CppDebugger::exited(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void SprintIDE::CppDebugger::terminated()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void SprintIDE::CppDebugger::outputReceived(const QString & _t1, bool _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1, _t2);
}

// SIGNAL 5
void SprintIDE::CppDebugger::errorOccurred(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}

// SIGNAL 6
void SprintIDE::CppDebugger::variablesUpdated(const QList<DebugVariable> & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1);
}

// SIGNAL 7
void SprintIDE::CppDebugger::stackUpdated(const QList<StackFrame> & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1);
}
QT_WARNING_POP
