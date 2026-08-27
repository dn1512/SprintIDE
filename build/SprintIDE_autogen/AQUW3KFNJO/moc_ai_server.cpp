/****************************************************************************
** Meta object code from reading C++ file 'ai_server.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../ai/ai_server.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ai_server.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN9SprintIDE8AIServerE_t {};
} // unnamed namespace

template <> constexpr inline auto SprintIDE::AIServer::qt_create_metaobjectdata<qt_meta_tag_ZN9SprintIDE8AIServerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "SprintIDE::AIServer",
        "serverStarted",
        "",
        "serverStopped",
        "agentConnected",
        "agentDisconnected",
        "ideRequestReceived",
        "requestId",
        "method",
        "QJsonObject",
        "params",
        "notificationReceived",
        "responseReceived",
        "result",
        "errorResponseReceived",
        "code",
        "message",
        "streamDataReceived",
        "streamId",
        "chunk",
        "isEnd",
        "fullContent",
        "onNewConnection",
        "onReadyRead",
        "onSocketDisconnected",
        "onSocketError",
        "QAbstractSocket::SocketError",
        "error",
        "onHeartbeatTimeout"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'serverStarted'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'serverStopped'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'agentConnected'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'agentDisconnected'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'ideRequestReceived'
        QtMocHelpers::SignalData<void(int, const QString &, const QJsonObject &)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 7 }, { QMetaType::QString, 8 }, { 0x80000000 | 9, 10 },
        }}),
        // Signal 'notificationReceived'
        QtMocHelpers::SignalData<void(const QString &, const QJsonObject &)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 8 }, { 0x80000000 | 9, 10 },
        }}),
        // Signal 'responseReceived'
        QtMocHelpers::SignalData<void(int, const QJsonObject &)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 7 }, { 0x80000000 | 9, 13 },
        }}),
        // Signal 'errorResponseReceived'
        QtMocHelpers::SignalData<void(int, int, const QString &)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 7 }, { QMetaType::Int, 15 }, { QMetaType::QString, 16 },
        }}),
        // Signal 'streamDataReceived'
        QtMocHelpers::SignalData<void(int, const QString &, const QString &, bool, const QString &)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 7 }, { QMetaType::QString, 18 }, { QMetaType::QString, 19 }, { QMetaType::Bool, 20 },
            { QMetaType::QString, 21 },
        }}),
        // Slot 'onNewConnection'
        QtMocHelpers::SlotData<void()>(22, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onReadyRead'
        QtMocHelpers::SlotData<void()>(23, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSocketDisconnected'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSocketError'
        QtMocHelpers::SlotData<void(QAbstractSocket::SocketError)>(25, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 26, 27 },
        }}),
        // Slot 'onHeartbeatTimeout'
        QtMocHelpers::SlotData<void()>(28, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<AIServer, qt_meta_tag_ZN9SprintIDE8AIServerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject SprintIDE::AIServer::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE8AIServerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE8AIServerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN9SprintIDE8AIServerE_t>.metaTypes,
    nullptr
} };

void SprintIDE::AIServer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<AIServer *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->serverStarted(); break;
        case 1: _t->serverStopped(); break;
        case 2: _t->agentConnected(); break;
        case 3: _t->agentDisconnected(); break;
        case 4: _t->ideRequestReceived((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QJsonObject>>(_a[3]))); break;
        case 5: _t->notificationReceived((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QJsonObject>>(_a[2]))); break;
        case 6: _t->responseReceived((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QJsonObject>>(_a[2]))); break;
        case 7: _t->errorResponseReceived((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3]))); break;
        case 8: _t->streamDataReceived((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[4])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[5]))); break;
        case 9: _t->onNewConnection(); break;
        case 10: _t->onReadyRead(); break;
        case 11: _t->onSocketDisconnected(); break;
        case 12: _t->onSocketError((*reinterpret_cast<std::add_pointer_t<QAbstractSocket::SocketError>>(_a[1]))); break;
        case 13: _t->onHeartbeatTimeout(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 12:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QAbstractSocket::SocketError >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (AIServer::*)()>(_a, &AIServer::serverStarted, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (AIServer::*)()>(_a, &AIServer::serverStopped, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (AIServer::*)()>(_a, &AIServer::agentConnected, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (AIServer::*)()>(_a, &AIServer::agentDisconnected, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (AIServer::*)(int , const QString & , const QJsonObject & )>(_a, &AIServer::ideRequestReceived, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (AIServer::*)(const QString & , const QJsonObject & )>(_a, &AIServer::notificationReceived, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (AIServer::*)(int , const QJsonObject & )>(_a, &AIServer::responseReceived, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (AIServer::*)(int , int , const QString & )>(_a, &AIServer::errorResponseReceived, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (AIServer::*)(int , const QString & , const QString & , bool , const QString & )>(_a, &AIServer::streamDataReceived, 8))
            return;
    }
}

const QMetaObject *SprintIDE::AIServer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SprintIDE::AIServer::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE8AIServerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SprintIDE::AIServer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    }
    return _id;
}

// SIGNAL 0
void SprintIDE::AIServer::serverStarted()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void SprintIDE::AIServer::serverStopped()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void SprintIDE::AIServer::agentConnected()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void SprintIDE::AIServer::agentDisconnected()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void SprintIDE::AIServer::ideRequestReceived(int _t1, const QString & _t2, const QJsonObject & _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1, _t2, _t3);
}

// SIGNAL 5
void SprintIDE::AIServer::notificationReceived(const QString & _t1, const QJsonObject & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1, _t2);
}

// SIGNAL 6
void SprintIDE::AIServer::responseReceived(int _t1, const QJsonObject & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1, _t2);
}

// SIGNAL 7
void SprintIDE::AIServer::errorResponseReceived(int _t1, int _t2, const QString & _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1, _t2, _t3);
}

// SIGNAL 8
void SprintIDE::AIServer::streamDataReceived(int _t1, const QString & _t2, const QString & _t3, bool _t4, const QString & _t5)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 8, nullptr, _t1, _t2, _t3, _t4, _t5);
}
QT_WARNING_POP
