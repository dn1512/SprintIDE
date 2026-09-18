/****************************************************************************
** Meta object code from reading C++ file 'ai_action_executor.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../ai/ai_action_executor.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ai_action_executor.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN9SprintIDE16AIActionExecutorE_t {};
} // unnamed namespace

template <> constexpr inline auto SprintIDE::AIActionExecutor::qt_create_metaobjectdata<qt_meta_tag_ZN9SprintIDE16AIActionExecutorE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "SprintIDE::AIActionExecutor",
        "actionFinished",
        "",
        "actionId",
        "AIActionResult",
        "result",
        "actionsFinished",
        "QList<AIActionResult>",
        "results",
        "outputProduced",
        "message",
        "isError",
        "onCompileFinished",
        "taskId",
        "CompileResult",
        "onDebugStateChanged",
        "DebugState",
        "state"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'actionFinished'
        QtMocHelpers::SignalData<void(const QString &, const AIActionResult &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 }, { 0x80000000 | 4, 5 },
        }}),
        // Signal 'actionsFinished'
        QtMocHelpers::SignalData<void(const QList<AIActionResult> &)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 7, 8 },
        }}),
        // Signal 'outputProduced'
        QtMocHelpers::SignalData<void(const QString &, bool)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 10 }, { QMetaType::Bool, 11 },
        }}),
        // Slot 'onCompileFinished'
        QtMocHelpers::SlotData<void(const QString &, const CompileResult &)>(12, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 13 }, { 0x80000000 | 14, 5 },
        }}),
        // Slot 'onDebugStateChanged'
        QtMocHelpers::SlotData<void(DebugState)>(15, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 16, 17 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<AIActionExecutor, qt_meta_tag_ZN9SprintIDE16AIActionExecutorE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject SprintIDE::AIActionExecutor::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE16AIActionExecutorE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE16AIActionExecutorE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN9SprintIDE16AIActionExecutorE_t>.metaTypes,
    nullptr
} };

void SprintIDE::AIActionExecutor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<AIActionExecutor *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->actionFinished((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<AIActionResult>>(_a[2]))); break;
        case 1: _t->actionsFinished((*reinterpret_cast<std::add_pointer_t<QList<AIActionResult>>>(_a[1]))); break;
        case 2: _t->outputProduced((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 3: _t->onCompileFinished((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<CompileResult>>(_a[2]))); break;
        case 4: _t->onDebugStateChanged((*reinterpret_cast<std::add_pointer_t<DebugState>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (AIActionExecutor::*)(const QString & , const AIActionResult & )>(_a, &AIActionExecutor::actionFinished, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (AIActionExecutor::*)(const QList<AIActionResult> & )>(_a, &AIActionExecutor::actionsFinished, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (AIActionExecutor::*)(const QString & , bool )>(_a, &AIActionExecutor::outputProduced, 2))
            return;
    }
}

const QMetaObject *SprintIDE::AIActionExecutor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SprintIDE::AIActionExecutor::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE16AIActionExecutorE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SprintIDE::AIActionExecutor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void SprintIDE::AIActionExecutor::actionFinished(const QString & _t1, const AIActionResult & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2);
}

// SIGNAL 1
void SprintIDE::AIActionExecutor::actionsFinished(const QList<AIActionResult> & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void SprintIDE::AIActionExecutor::outputProduced(const QString & _t1, bool _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1, _t2);
}
QT_WARNING_POP
