/****************************************************************************
** Meta object code from reading C++ file 'theme_manager.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../theme_manager.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'theme_manager.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN9SprintIDE12ThemeManagerE_t {};
} // unnamed namespace

template <> constexpr inline auto SprintIDE::ThemeManager::qt_create_metaobjectdata<qt_meta_tag_ZN9SprintIDE12ThemeManagerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "SprintIDE::ThemeManager",
        "themeChanged",
        "",
        "ThemeType",
        "newTheme",
        "colorChanged",
        "UIElement",
        "element",
        "QColor",
        "color",
        "fontChanged",
        "QFont",
        "font",
        "themeApplied",
        "theme",
        "editorThemeChanged",
        "isDark"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'themeChanged'
        QtMocHelpers::SignalData<void(enum ThemeType)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'colorChanged'
        QtMocHelpers::SignalData<void(enum UIElement, const QColor &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 6, 7 }, { 0x80000000 | 8, 9 },
        }}),
        // Signal 'fontChanged'
        QtMocHelpers::SignalData<void(const QFont &)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 11, 12 },
        }}),
        // Signal 'themeApplied'
        QtMocHelpers::SignalData<void(enum ThemeType)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 14 },
        }}),
        // Signal 'editorThemeChanged'
        QtMocHelpers::SignalData<void(bool)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 16 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ThemeManager, qt_meta_tag_ZN9SprintIDE12ThemeManagerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject SprintIDE::ThemeManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE12ThemeManagerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE12ThemeManagerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN9SprintIDE12ThemeManagerE_t>.metaTypes,
    nullptr
} };

void SprintIDE::ThemeManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ThemeManager *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->themeChanged((*reinterpret_cast<std::add_pointer_t<enum ThemeType>>(_a[1]))); break;
        case 1: _t->colorChanged((*reinterpret_cast<std::add_pointer_t<enum UIElement>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QColor>>(_a[2]))); break;
        case 2: _t->fontChanged((*reinterpret_cast<std::add_pointer_t<QFont>>(_a[1]))); break;
        case 3: _t->themeApplied((*reinterpret_cast<std::add_pointer_t<enum ThemeType>>(_a[1]))); break;
        case 4: _t->editorThemeChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (ThemeManager::*)(ThemeType )>(_a, &ThemeManager::themeChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (ThemeManager::*)(UIElement , const QColor & )>(_a, &ThemeManager::colorChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (ThemeManager::*)(const QFont & )>(_a, &ThemeManager::fontChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (ThemeManager::*)(ThemeType )>(_a, &ThemeManager::themeApplied, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (ThemeManager::*)(bool )>(_a, &ThemeManager::editorThemeChanged, 4))
            return;
    }
}

const QMetaObject *SprintIDE::ThemeManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SprintIDE::ThemeManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9SprintIDE12ThemeManagerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SprintIDE::ThemeManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
void SprintIDE::ThemeManager::themeChanged(ThemeType _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void SprintIDE::ThemeManager::colorChanged(UIElement _t1, const QColor & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2);
}

// SIGNAL 2
void SprintIDE::ThemeManager::fontChanged(const QFont & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void SprintIDE::ThemeManager::themeApplied(ThemeType _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void SprintIDE::ThemeManager::editorThemeChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}
QT_WARNING_POP
