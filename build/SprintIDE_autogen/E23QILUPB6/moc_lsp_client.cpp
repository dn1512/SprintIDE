/****************************************************************************
** Meta object code from reading C++ file 'lsp_client.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../lsp/lsp_client.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'lsp_client.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN9SprintIDE9LSPClientE_t {};
} // unnamed namespace

template <> constexpr inline auto SprintIDE::LSPClient::qt_create_metaobjectdata<qt_meta_tag_ZN9SprintIDE9LSPClientE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "SprintIDE::LSPClient",
        "serverStarted",
        "",
        "serverStopped",
        "serverError",
        "error",
        "serverReady",
        "diagnosticsReceived",
        "filePath",
        "QList<LSPDiagnostic>",
        "diagnostics",
        "completionReceived",
        "QList<LSPCompletionItem>",
        "items",
        "definitionReceived",
        "line",
        "column",
        "onServerReadyRead",
        "onServerErrorRead",
        "onServerFinished",
        "exitCode",
        "QProcess::ExitStatus",
        "status",
        "onReparseTimer",
        "onInitializeTimeout"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'serverStarted'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'serverStopped'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'serverError'
        QtMocHelpers::SignalData<void(const QString &)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 5 },
        }}),
        // Signal 'serverReady'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'diagnosticsReceived'
        QtMocHelpers::SignalData<void(const QString &, const QList<LSPDiagnostic> &)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 8 }, { 0x80000000 | 9, 10 },
        }}),
        // Signal 'completionReceived'
        QtMocHelpers::SignalData<void(const QString &, const QList<LSPCompletionItem> &)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 8 }, { 0x80000000 | 12, 13 },
        }}),
        // Signal 'definitionReceived'
        QtMocHelpers::SignalData<void(const QString &, int, int)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 8 }, { QMetaType::Int, 15 }, { QMetaType::Int, 16 },
        }}),
        // Slot 'onServerReadyRead'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onServerErrorRead'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onServerFinished'
        QtMocHelpers::SlotData<void(int, QProcess::ExitStatus)>(19, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 20 }, { 0x80000000 | 21, 22 },
        }}),
        // Slot 'onReparseTimer'
        QtMocHelpers::SlotData<void()>(23, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onInitializeTimeout'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<LSPClient, qt_meta_tag_ZN9SprintIDE9LSPClientE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject SprintIDE::LSPClient::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE9LSPClientE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE9LSPClientE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN9SprintIDE9LSPClientE_t>.metaTypes,
    nullptr
} };

void SprintIDE::LSPClient::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<LSPClient *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->serverStarted(); break;
        case 1: _t->serverStopped(); break;
        case 2: _t->serverError((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->serverReady(); break;
        case 4: _t->diagnosticsReceived((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QList<LSPDiagnostic>>>(_a[2]))); break;
        case 5: _t->completionReceived((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QList<LSPCompletionItem>>>(_a[2]))); break;
        case 6: _t->definitionReceived((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3]))); break;
        case 7: _t->onServerReadyRead(); break;
        case 8: _t->onServerErrorRead(); break;
        case 9: _t->onServerFinished((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        case 10: _t->onReparseTimer(); break;
        case 11: _t->onInitializeTimeout(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (LSPClient::*)()>(_a, &LSPClient::serverStarted, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (LSPClient::*)()>(_a, &LSPClient::serverStopped, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (LSPClient::*)(const QString & )>(_a, &LSPClient::serverError, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (LSPClient::*)()>(_a, &LSPClient::serverReady, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (LSPClient::*)(const QString & , const QList<LSPDiagnostic> & )>(_a, &LSPClient::diagnosticsReceived, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (LSPClient::*)(const QString & , const QList<LSPCompletionItem> & )>(_a, &LSPClient::completionReceived, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (LSPClient::*)(const QString & , int , int )>(_a, &LSPClient::definitionReceived, 6))
            return;
    }
}

const QMetaObject *SprintIDE::LSPClient::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SprintIDE::LSPClient::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE9LSPClientE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SprintIDE::LSPClient::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 12)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 12;
    }
    return _id;
}

// SIGNAL 0
void SprintIDE::LSPClient::serverStarted()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void SprintIDE::LSPClient::serverStopped()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void SprintIDE::LSPClient::serverError(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void SprintIDE::LSPClient::serverReady()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void SprintIDE::LSPClient::diagnosticsReceived(const QString & _t1, const QList<LSPDiagnostic> & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1, _t2);
}

// SIGNAL 5
void SprintIDE::LSPClient::completionReceived(const QString & _t1, const QList<LSPCompletionItem> & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1, _t2);
}

// SIGNAL 6
void SprintIDE::LSPClient::definitionReceived(const QString & _t1, int _t2, int _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1, _t2, _t3);
}
QT_WARNING_POP
