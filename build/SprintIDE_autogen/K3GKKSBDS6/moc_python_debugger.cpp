/****************************************************************************
** Meta object code from reading C++ file 'python_debugger.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../python/python_debugger.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'python_debugger.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN9SprintIDE14PythonDebuggerE_t {};
} // unnamed namespace

template <> constexpr inline auto SprintIDE::PythonDebugger::qt_create_metaobjectdata<qt_meta_tag_ZN9SprintIDE14PythonDebuggerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "SprintIDE::PythonDebugger",
        "stopped",
        "",
        "file",
        "line",
        "reason",
        "continued",
        "exited",
        "exitCode",
        "terminated",
        "variablesUpdated",
        "QList<DebugVariable>",
        "variables",
        "stackUpdated",
        "QList<StackFrame>",
        "frames",
        "outputReceived",
        "output",
        "isError",
        "errorOccurred",
        "error"
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
        // Signal 'variablesUpdated'
        QtMocHelpers::SignalData<void(const QList<DebugVariable> &)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 11, 12 },
        }}),
        // Signal 'stackUpdated'
        QtMocHelpers::SignalData<void(const QList<StackFrame> &)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 14, 15 },
        }}),
        // Signal 'outputReceived'
        QtMocHelpers::SignalData<void(const QString &, bool)>(16, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 17 }, { QMetaType::Bool, 18 },
        }}),
        // Signal 'errorOccurred'
        QtMocHelpers::SignalData<void(const QString &)>(19, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 20 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<PythonDebugger, qt_meta_tag_ZN9SprintIDE14PythonDebuggerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject SprintIDE::PythonDebugger::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE14PythonDebuggerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE14PythonDebuggerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN9SprintIDE14PythonDebuggerE_t>.metaTypes,
    nullptr
} };

void SprintIDE::PythonDebugger::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<PythonDebugger *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->stopped((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3]))); break;
        case 1: _t->continued(); break;
        case 2: _t->exited((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 3: _t->terminated(); break;
        case 4: _t->variablesUpdated((*reinterpret_cast<std::add_pointer_t<QList<DebugVariable>>>(_a[1]))); break;
        case 5: _t->stackUpdated((*reinterpret_cast<std::add_pointer_t<QList<StackFrame>>>(_a[1]))); break;
        case 6: _t->outputReceived((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 7: _t->errorOccurred((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (PythonDebugger::*)(const QString & , int , const QString & )>(_a, &PythonDebugger::stopped, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (PythonDebugger::*)()>(_a, &PythonDebugger::continued, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (PythonDebugger::*)(int )>(_a, &PythonDebugger::exited, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (PythonDebugger::*)()>(_a, &PythonDebugger::terminated, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (PythonDebugger::*)(const QList<DebugVariable> & )>(_a, &PythonDebugger::variablesUpdated, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (PythonDebugger::*)(const QList<StackFrame> & )>(_a, &PythonDebugger::stackUpdated, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (PythonDebugger::*)(const QString & , bool )>(_a, &PythonDebugger::outputReceived, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (PythonDebugger::*)(const QString & )>(_a, &PythonDebugger::errorOccurred, 7))
            return;
    }
}

const QMetaObject *SprintIDE::PythonDebugger::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SprintIDE::PythonDebugger::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE14PythonDebuggerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SprintIDE::PythonDebugger::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void SprintIDE::PythonDebugger::stopped(const QString & _t1, int _t2, const QString & _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2, _t3);
}

// SIGNAL 1
void SprintIDE::PythonDebugger::continued()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void SprintIDE::PythonDebugger::exited(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void SprintIDE::PythonDebugger::terminated()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void SprintIDE::PythonDebugger::variablesUpdated(const QList<DebugVariable> & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void SprintIDE::PythonDebugger::stackUpdated(const QList<StackFrame> & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}

// SIGNAL 6
void SprintIDE::PythonDebugger::outputReceived(const QString & _t1, bool _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1, _t2);
}

// SIGNAL 7
void SprintIDE::PythonDebugger::errorOccurred(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1);
}
QT_WARNING_POP
