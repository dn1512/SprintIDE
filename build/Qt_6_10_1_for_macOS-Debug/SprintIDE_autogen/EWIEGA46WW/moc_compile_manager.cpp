/****************************************************************************
** Meta object code from reading C++ file 'compile_manager.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../compile_manager.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'compile_manager.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN9SprintIDE14CompileManagerE_t {};
} // unnamed namespace

template <> constexpr inline auto SprintIDE::CompileManager::qt_create_metaobjectdata<qt_meta_tag_ZN9SprintIDE14CompileManagerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "SprintIDE::CompileManager",
        "compileStarted",
        "",
        "taskId",
        "sourceFile",
        "compileProgress",
        "progress",
        "currentFile",
        "compileOutput",
        "output",
        "isError",
        "compileFinished",
        "CompileResult",
        "result",
        "compilersDetected",
        "compilers",
        "compileError",
        "errorMessage",
        "onProcessFinished",
        "processId",
        "ProcessManager::ProcessResult",
        "processResult",
        "onOutputReceived",
        "data",
        "onProcessStarted",
        "onProcessError",
        "onProgressUpdated",
        "elapsedTime"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'compileStarted'
        QtMocHelpers::SignalData<void(const QString &, const QString &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 }, { QMetaType::QString, 4 },
        }}),
        // Signal 'compileProgress'
        QtMocHelpers::SignalData<void(const QString &, int, const QString &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 }, { QMetaType::Int, 6 }, { QMetaType::QString, 7 },
        }}),
        // Signal 'compileOutput'
        QtMocHelpers::SignalData<void(const QString &, const QString &, bool)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 }, { QMetaType::QString, 9 }, { QMetaType::Bool, 10 },
        }}),
        // Signal 'compileFinished'
        QtMocHelpers::SignalData<void(const QString &, const CompileResult &)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 }, { 0x80000000 | 12, 13 },
        }}),
        // Signal 'compilersDetected'
        QtMocHelpers::SignalData<void(const QStringList &)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QStringList, 15 },
        }}),
        // Signal 'compileError'
        QtMocHelpers::SignalData<void(const QString &, const QString &)>(16, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 }, { QMetaType::QString, 17 },
        }}),
        // Slot 'onProcessFinished'
        QtMocHelpers::SlotData<void(const QString &, const ProcessManager::ProcessResult &)>(18, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 19 }, { 0x80000000 | 20, 21 },
        }}),
        // Slot 'onOutputReceived'
        QtMocHelpers::SlotData<void(const QString &, const QString &, bool)>(22, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 19 }, { QMetaType::QString, 23 }, { QMetaType::Bool, 10 },
        }}),
        // Slot 'onProcessStarted'
        QtMocHelpers::SlotData<void(const QString &)>(24, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 19 },
        }}),
        // Slot 'onProcessError'
        QtMocHelpers::SlotData<void(const QString &, const QString &)>(25, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 19 }, { QMetaType::QString, 17 },
        }}),
        // Slot 'onProgressUpdated'
        QtMocHelpers::SlotData<void(const QString &, qint64)>(26, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 19 }, { QMetaType::LongLong, 27 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<CompileManager, qt_meta_tag_ZN9SprintIDE14CompileManagerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject SprintIDE::CompileManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE14CompileManagerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE14CompileManagerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN9SprintIDE14CompileManagerE_t>.metaTypes,
    nullptr
} };

void SprintIDE::CompileManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<CompileManager *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->compileStarted((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 1: _t->compileProgress((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3]))); break;
        case 2: _t->compileOutput((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[3]))); break;
        case 3: _t->compileFinished((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<CompileResult>>(_a[2]))); break;
        case 4: _t->compilersDetected((*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[1]))); break;
        case 5: _t->compileError((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 6: _t->onProcessFinished((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<ProcessManager::ProcessResult>>(_a[2]))); break;
        case 7: _t->onOutputReceived((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[3]))); break;
        case 8: _t->onProcessStarted((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 9: _t->onProcessError((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 10: _t->onProgressUpdated((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<qint64>>(_a[2]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (CompileManager::*)(const QString & , const QString & )>(_a, &CompileManager::compileStarted, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (CompileManager::*)(const QString & , int , const QString & )>(_a, &CompileManager::compileProgress, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (CompileManager::*)(const QString & , const QString & , bool )>(_a, &CompileManager::compileOutput, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (CompileManager::*)(const QString & , const CompileResult & )>(_a, &CompileManager::compileFinished, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (CompileManager::*)(const QStringList & )>(_a, &CompileManager::compilersDetected, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (CompileManager::*)(const QString & , const QString & )>(_a, &CompileManager::compileError, 5))
            return;
    }
}

const QMetaObject *SprintIDE::CompileManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SprintIDE::CompileManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE14CompileManagerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SprintIDE::CompileManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void SprintIDE::CompileManager::compileStarted(const QString & _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2);
}

// SIGNAL 1
void SprintIDE::CompileManager::compileProgress(const QString & _t1, int _t2, const QString & _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2, _t3);
}

// SIGNAL 2
void SprintIDE::CompileManager::compileOutput(const QString & _t1, const QString & _t2, bool _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1, _t2, _t3);
}

// SIGNAL 3
void SprintIDE::CompileManager::compileFinished(const QString & _t1, const CompileResult & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1, _t2);
}

// SIGNAL 4
void SprintIDE::CompileManager::compilersDetected(const QStringList & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void SprintIDE::CompileManager::compileError(const QString & _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1, _t2);
}
QT_WARNING_POP
