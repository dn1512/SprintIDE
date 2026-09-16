/****************************************************************************
** Meta object code from reading C++ file 'dap_client.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../dap/dap_client.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'dap_client.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN9SprintIDE9DapClientE_t {};
} // unnamed namespace

template <> constexpr inline auto SprintIDE::DapClient::qt_create_metaobjectdata<qt_meta_tag_ZN9SprintIDE9DapClientE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "SprintIDE::DapClient",
        "adapterStarted",
        "",
        "adapterExited",
        "exitCode",
        "initialized",
        "stopped",
        "SprintIDE::Dap::StoppedInfo",
        "info",
        "continued",
        "threadId",
        "exited",
        "terminated",
        "output",
        "category",
        "text",
        "breakpointsValidated",
        "sourcePath",
        "QList<SprintIDE::Dap::Breakpoint>",
        "breakpoints",
        "stackTraceReceived",
        "QList<SprintIDE::Dap::StackFrame>",
        "frames",
        "scopesReceived",
        "frameId",
        "QList<SprintIDE::Dap::Scope>",
        "scopes",
        "variablesReceived",
        "parentRef",
        "QList<SprintIDE::Dap::Variable>",
        "variables",
        "evaluateResult",
        "expression",
        "result",
        "type",
        "processStartedEvent",
        "name",
        "systemProcessId",
        "threadEvent",
        "reason",
        "errorOccurred",
        "error",
        "onStdout",
        "onStderr",
        "onFinished",
        "QProcess::ExitStatus",
        "status",
        "onErrorOccurred",
        "QProcess::ProcessError",
        "err",
        "State",
        "Disconnected",
        "Starting",
        "Initializing",
        "Launching",
        "Configuring",
        "Running",
        "Stopping"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'adapterStarted'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'adapterExited'
        QtMocHelpers::SignalData<void(int)>(3, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 4 },
        }}),
        // Signal 'initialized'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'stopped'
        QtMocHelpers::SignalData<void(const SprintIDE::Dap::StoppedInfo &)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 7, 8 },
        }}),
        // Signal 'continued'
        QtMocHelpers::SignalData<void(int)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 10 },
        }}),
        // Signal 'exited'
        QtMocHelpers::SignalData<void(int)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 4 },
        }}),
        // Signal 'terminated'
        QtMocHelpers::SignalData<void()>(12, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'output'
        QtMocHelpers::SignalData<void(const QString &, const QString &)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 14 }, { QMetaType::QString, 15 },
        }}),
        // Signal 'breakpointsValidated'
        QtMocHelpers::SignalData<void(const QString &, const QList<SprintIDE::Dap::Breakpoint> &)>(16, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 17 }, { 0x80000000 | 18, 19 },
        }}),
        // Signal 'stackTraceReceived'
        QtMocHelpers::SignalData<void(const QList<SprintIDE::Dap::StackFrame> &)>(20, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 21, 22 },
        }}),
        // Signal 'scopesReceived'
        QtMocHelpers::SignalData<void(int, const QList<SprintIDE::Dap::Scope> &)>(23, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 24 }, { 0x80000000 | 25, 26 },
        }}),
        // Signal 'variablesReceived'
        QtMocHelpers::SignalData<void(int, const QList<SprintIDE::Dap::Variable> &)>(27, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 28 }, { 0x80000000 | 29, 30 },
        }}),
        // Signal 'evaluateResult'
        QtMocHelpers::SignalData<void(const QString &, const QString &, const QString &)>(31, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 32 }, { QMetaType::QString, 33 }, { QMetaType::QString, 34 },
        }}),
        // Signal 'processStartedEvent'
        QtMocHelpers::SignalData<void(const QString &, int)>(35, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 36 }, { QMetaType::Int, 37 },
        }}),
        // Signal 'threadEvent'
        QtMocHelpers::SignalData<void(const QString &, int)>(38, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 39 }, { QMetaType::Int, 10 },
        }}),
        // Signal 'errorOccurred'
        QtMocHelpers::SignalData<void(const QString &)>(40, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 41 },
        }}),
        // Slot 'onStdout'
        QtMocHelpers::SlotData<void()>(42, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onStderr'
        QtMocHelpers::SlotData<void()>(43, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onFinished'
        QtMocHelpers::SlotData<void(int, QProcess::ExitStatus)>(44, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 4 }, { 0x80000000 | 45, 46 },
        }}),
        // Slot 'onErrorOccurred'
        QtMocHelpers::SlotData<void(QProcess::ProcessError)>(47, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 48, 49 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
        // enum 'State'
        QtMocHelpers::EnumData<enum State>(50, 50, QMC::EnumIsScoped).add({
            {   51, State::Disconnected },
            {   52, State::Starting },
            {   53, State::Initializing },
            {   54, State::Launching },
            {   55, State::Configuring },
            {   56, State::Running },
            {   57, State::Stopping },
        }),
    };
    return QtMocHelpers::metaObjectData<DapClient, qt_meta_tag_ZN9SprintIDE9DapClientE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject SprintIDE::DapClient::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE9DapClientE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE9DapClientE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN9SprintIDE9DapClientE_t>.metaTypes,
    nullptr
} };

void SprintIDE::DapClient::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<DapClient *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->adapterStarted(); break;
        case 1: _t->adapterExited((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 2: _t->initialized(); break;
        case 3: _t->stopped((*reinterpret_cast<std::add_pointer_t<SprintIDE::Dap::StoppedInfo>>(_a[1]))); break;
        case 4: _t->continued((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 5: _t->exited((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 6: _t->terminated(); break;
        case 7: _t->output((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 8: _t->breakpointsValidated((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QList<SprintIDE::Dap::Breakpoint>>>(_a[2]))); break;
        case 9: _t->stackTraceReceived((*reinterpret_cast<std::add_pointer_t<QList<SprintIDE::Dap::StackFrame>>>(_a[1]))); break;
        case 10: _t->scopesReceived((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QList<SprintIDE::Dap::Scope>>>(_a[2]))); break;
        case 11: _t->variablesReceived((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QList<SprintIDE::Dap::Variable>>>(_a[2]))); break;
        case 12: _t->evaluateResult((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3]))); break;
        case 13: _t->processStartedEvent((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 14: _t->threadEvent((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 15: _t->errorOccurred((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 16: _t->onStdout(); break;
        case 17: _t->onStderr(); break;
        case 18: _t->onFinished((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        case 19: _t->onErrorOccurred((*reinterpret_cast<std::add_pointer_t<QProcess::ProcessError>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (DapClient::*)()>(_a, &DapClient::adapterStarted, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (DapClient::*)(int )>(_a, &DapClient::adapterExited, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (DapClient::*)()>(_a, &DapClient::initialized, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (DapClient::*)(const SprintIDE::Dap::StoppedInfo & )>(_a, &DapClient::stopped, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (DapClient::*)(int )>(_a, &DapClient::continued, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (DapClient::*)(int )>(_a, &DapClient::exited, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (DapClient::*)()>(_a, &DapClient::terminated, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (DapClient::*)(const QString & , const QString & )>(_a, &DapClient::output, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (DapClient::*)(const QString & , const QList<SprintIDE::Dap::Breakpoint> & )>(_a, &DapClient::breakpointsValidated, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (DapClient::*)(const QList<SprintIDE::Dap::StackFrame> & )>(_a, &DapClient::stackTraceReceived, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (DapClient::*)(int , const QList<SprintIDE::Dap::Scope> & )>(_a, &DapClient::scopesReceived, 10))
            return;
        if (QtMocHelpers::indexOfMethod<void (DapClient::*)(int , const QList<SprintIDE::Dap::Variable> & )>(_a, &DapClient::variablesReceived, 11))
            return;
        if (QtMocHelpers::indexOfMethod<void (DapClient::*)(const QString & , const QString & , const QString & )>(_a, &DapClient::evaluateResult, 12))
            return;
        if (QtMocHelpers::indexOfMethod<void (DapClient::*)(const QString & , int )>(_a, &DapClient::processStartedEvent, 13))
            return;
        if (QtMocHelpers::indexOfMethod<void (DapClient::*)(const QString & , int )>(_a, &DapClient::threadEvent, 14))
            return;
        if (QtMocHelpers::indexOfMethod<void (DapClient::*)(const QString & )>(_a, &DapClient::errorOccurred, 15))
            return;
    }
}

const QMetaObject *SprintIDE::DapClient::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SprintIDE::DapClient::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE9DapClientE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SprintIDE::DapClient::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 20)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 20;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 20)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 20;
    }
    return _id;
}

// SIGNAL 0
void SprintIDE::DapClient::adapterStarted()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void SprintIDE::DapClient::adapterExited(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void SprintIDE::DapClient::initialized()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void SprintIDE::DapClient::stopped(const SprintIDE::Dap::StoppedInfo & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void SprintIDE::DapClient::continued(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void SprintIDE::DapClient::exited(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}

// SIGNAL 6
void SprintIDE::DapClient::terminated()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void SprintIDE::DapClient::output(const QString & _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1, _t2);
}

// SIGNAL 8
void SprintIDE::DapClient::breakpointsValidated(const QString & _t1, const QList<SprintIDE::Dap::Breakpoint> & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 8, nullptr, _t1, _t2);
}

// SIGNAL 9
void SprintIDE::DapClient::stackTraceReceived(const QList<SprintIDE::Dap::StackFrame> & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 9, nullptr, _t1);
}

// SIGNAL 10
void SprintIDE::DapClient::scopesReceived(int _t1, const QList<SprintIDE::Dap::Scope> & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 10, nullptr, _t1, _t2);
}

// SIGNAL 11
void SprintIDE::DapClient::variablesReceived(int _t1, const QList<SprintIDE::Dap::Variable> & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 11, nullptr, _t1, _t2);
}

// SIGNAL 12
void SprintIDE::DapClient::evaluateResult(const QString & _t1, const QString & _t2, const QString & _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 12, nullptr, _t1, _t2, _t3);
}

// SIGNAL 13
void SprintIDE::DapClient::processStartedEvent(const QString & _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 13, nullptr, _t1, _t2);
}

// SIGNAL 14
void SprintIDE::DapClient::threadEvent(const QString & _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 14, nullptr, _t1, _t2);
}

// SIGNAL 15
void SprintIDE::DapClient::errorOccurred(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 15, nullptr, _t1);
}
QT_WARNING_POP
