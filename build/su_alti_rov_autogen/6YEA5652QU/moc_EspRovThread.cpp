/****************************************************************************
** Meta object code from reading C++ file 'EspRovThread.hpp'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../include/EspRovThread.hpp"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'EspRovThread.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_EspRovThread_t {
    QByteArrayData data[20];
    char stringdata0[204];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EspRovThread_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EspRovThread_t qt_meta_stringdata_EspRovThread = {
    {
QT_MOC_LITERAL(0, 0, 12), // "EspRovThread"
QT_MOC_LITERAL(1, 13, 13), // "status_signal"
QT_MOC_LITERAL(2, 27, 0), // ""
QT_MOC_LITERAL(3, 28, 3), // "msg"
QT_MOC_LITERAL(4, 32, 12), // "armed_signal"
QT_MOC_LITERAL(5, 45, 5), // "armed"
QT_MOC_LITERAL(6, 51, 15), // "attitude_signal"
QT_MOC_LITERAL(7, 67, 4), // "roll"
QT_MOC_LITERAL(8, 72, 5), // "pitch"
QT_MOC_LITERAL(9, 78, 3), // "yaw"
QT_MOC_LITERAL(10, 82, 12), // "depth_signal"
QT_MOC_LITERAL(11, 95, 6), // "meters"
QT_MOC_LITERAL(12, 102, 17), // "vertical_speed_ms"
QT_MOC_LITERAL(13, 120, 10), // "nem_signal"
QT_MOC_LITERAL(14, 131, 12), // "humidity_pct"
QT_MOC_LITERAL(15, 144, 13), // "temperature_c"
QT_MOC_LITERAL(16, 158, 14), // "torpedo_signal"
QT_MOC_LITERAL(17, 173, 9), // "remaining"
QT_MOC_LITERAL(18, 183, 12), // "wegsh_signal"
QT_MOC_LITERAL(19, 196, 7) // "visible"

    },
    "EspRovThread\0status_signal\0\0msg\0"
    "armed_signal\0armed\0attitude_signal\0"
    "roll\0pitch\0yaw\0depth_signal\0meters\0"
    "vertical_speed_ms\0nem_signal\0humidity_pct\0"
    "temperature_c\0torpedo_signal\0remaining\0"
    "wegsh_signal\0visible"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EspRovThread[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       7,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   49,    2, 0x06 /* Public */,
       4,    1,   52,    2, 0x06 /* Public */,
       6,    3,   55,    2, 0x06 /* Public */,
      10,    2,   62,    2, 0x06 /* Public */,
      13,    2,   67,    2, 0x06 /* Public */,
      16,    1,   72,    2, 0x06 /* Public */,
      18,    2,   75,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::Bool,    5,
    QMetaType::Void, QMetaType::Float, QMetaType::Float, QMetaType::Float,    7,    8,    9,
    QMetaType::Void, QMetaType::Float, QMetaType::Float,   11,   12,
    QMetaType::Void, QMetaType::Float, QMetaType::Float,   14,   15,
    QMetaType::Void, QMetaType::Int,   17,
    QMetaType::Void, QMetaType::Float, QMetaType::Bool,    9,   19,

       0        // eod
};

void EspRovThread::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<EspRovThread *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->status_signal((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->armed_signal((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 2: _t->attitude_signal((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< float(*)>(_a[3]))); break;
        case 3: _t->depth_signal((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2]))); break;
        case 4: _t->nem_signal((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2]))); break;
        case 5: _t->torpedo_signal((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 6: _t->wegsh_signal((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (EspRovThread::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EspRovThread::status_signal)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (EspRovThread::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EspRovThread::armed_signal)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (EspRovThread::*)(float , float , float );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EspRovThread::attitude_signal)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (EspRovThread::*)(float , float );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EspRovThread::depth_signal)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (EspRovThread::*)(float , float );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EspRovThread::nem_signal)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (EspRovThread::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EspRovThread::torpedo_signal)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (EspRovThread::*)(float , bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EspRovThread::wegsh_signal)) {
                *result = 6;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject EspRovThread::staticMetaObject = { {
    QMetaObject::SuperData::link<QThread::staticMetaObject>(),
    qt_meta_stringdata_EspRovThread.data,
    qt_meta_data_EspRovThread,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *EspRovThread::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EspRovThread::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_EspRovThread.stringdata0))
        return static_cast<void*>(this);
    return QThread::qt_metacast(_clname);
}

int EspRovThread::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void EspRovThread::status_signal(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void EspRovThread::armed_signal(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void EspRovThread::attitude_signal(float _t1, float _t2, float _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void EspRovThread::depth_signal(float _t1, float _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void EspRovThread::nem_signal(float _t1, float _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void EspRovThread::torpedo_signal(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void EspRovThread::wegsh_signal(float _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
