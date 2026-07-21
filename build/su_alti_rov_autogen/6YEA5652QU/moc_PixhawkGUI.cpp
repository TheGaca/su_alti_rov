/****************************************************************************
** Meta object code from reading C++ file 'PixhawkGUI.hpp'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../include/PixhawkGUI.hpp"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'PixhawkGUI.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_PixhawkGUI_t {
    QByteArrayData data[56];
    char stringdata0[846];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_PixhawkGUI_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_PixhawkGUI_t qt_meta_stringdata_PixhawkGUI = {
    {
QT_MOC_LITERAL(0, 0, 10), // "PixhawkGUI"
QT_MOC_LITERAL(1, 11, 21), // "toggle_ana_connection"
QT_MOC_LITERAL(2, 33, 0), // ""
QT_MOC_LITERAL(3, 34, 17), // "update_ana_status"
QT_MOC_LITERAL(4, 52, 3), // "msg"
QT_MOC_LITERAL(5, 56, 16), // "update_ana_armed"
QT_MOC_LITERAL(6, 73, 5), // "armed"
QT_MOC_LITERAL(7, 79, 19), // "update_ana_attitude"
QT_MOC_LITERAL(8, 99, 4), // "roll"
QT_MOC_LITERAL(9, 104, 5), // "pitch"
QT_MOC_LITERAL(10, 110, 22), // "toggle_mini_connection"
QT_MOC_LITERAL(11, 133, 18), // "update_mini_status"
QT_MOC_LITERAL(12, 152, 17), // "update_mini_armed"
QT_MOC_LITERAL(13, 170, 19), // "toggle_ana_joystick"
QT_MOC_LITERAL(14, 190, 20), // "toggle_mini_joystick"
QT_MOC_LITERAL(15, 211, 21), // "update_ana_joy_status"
QT_MOC_LITERAL(16, 233, 21), // "update_ana_joy_button"
QT_MOC_LITERAL(17, 255, 6), // "btn_id"
QT_MOC_LITERAL(18, 262, 5), // "state"
QT_MOC_LITERAL(19, 268, 19), // "update_ana_joy_axis"
QT_MOC_LITERAL(20, 288, 7), // "axis_id"
QT_MOC_LITERAL(21, 296, 5), // "value"
QT_MOC_LITERAL(22, 302, 22), // "update_mini_joy_status"
QT_MOC_LITERAL(23, 325, 22), // "update_mini_joy_button"
QT_MOC_LITERAL(24, 348, 20), // "update_mini_joy_axis"
QT_MOC_LITERAL(25, 369, 19), // "update_camera_frame"
QT_MOC_LITERAL(26, 389, 3), // "img"
QT_MOC_LITERAL(27, 393, 20), // "update_camera_status"
QT_MOC_LITERAL(28, 414, 19), // "update_camera_stats"
QT_MOC_LITERAL(29, 434, 3), // "fps"
QT_MOC_LITERAL(30, 438, 4), // "kbps"
QT_MOC_LITERAL(31, 443, 1), // "w"
QT_MOC_LITERAL(32, 445, 1), // "h"
QT_MOC_LITERAL(33, 447, 19), // "update_anarov_frame"
QT_MOC_LITERAL(34, 467, 20), // "update_anarov_status"
QT_MOC_LITERAL(35, 488, 19), // "update_anarov_stats"
QT_MOC_LITERAL(36, 508, 13), // "read_cam_ping"
QT_MOC_LITERAL(37, 522, 16), // "read_anarov_ping"
QT_MOC_LITERAL(38, 539, 15), // "ana_dir_pressed"
QT_MOC_LITERAL(39, 555, 16), // "ana_dir_released"
QT_MOC_LITERAL(40, 572, 16), // "mini_dir_pressed"
QT_MOC_LITERAL(41, 589, 17), // "mini_dir_released"
QT_MOC_LITERAL(42, 607, 16), // "on_emergency_ana"
QT_MOC_LITERAL(43, 624, 17), // "on_emergency_mini"
QT_MOC_LITERAL(44, 642, 16), // "on_stabilize_ana"
QT_MOC_LITERAL(45, 659, 17), // "on_stabilize_mini"
QT_MOC_LITERAL(46, 677, 17), // "on_autonomous_ana"
QT_MOC_LITERAL(47, 695, 13), // "on_manual_ana"
QT_MOC_LITERAL(48, 709, 17), // "on_minirov_launch"
QT_MOC_LITERAL(49, 727, 15), // "on_torpedo_fire"
QT_MOC_LITERAL(50, 743, 14), // "on_lamp_on_ana"
QT_MOC_LITERAL(51, 758, 15), // "on_lamp_off_ana"
QT_MOC_LITERAL(52, 774, 15), // "on_lamp_on_mini"
QT_MOC_LITERAL(53, 790, 16), // "on_lamp_off_mini"
QT_MOC_LITERAL(54, 807, 12), // "toggle_theme"
QT_MOC_LITERAL(55, 820, 25) // "toggle_stabilize_mode_ana"

    },
    "PixhawkGUI\0toggle_ana_connection\0\0"
    "update_ana_status\0msg\0update_ana_armed\0"
    "armed\0update_ana_attitude\0roll\0pitch\0"
    "toggle_mini_connection\0update_mini_status\0"
    "update_mini_armed\0toggle_ana_joystick\0"
    "toggle_mini_joystick\0update_ana_joy_status\0"
    "update_ana_joy_button\0btn_id\0state\0"
    "update_ana_joy_axis\0axis_id\0value\0"
    "update_mini_joy_status\0update_mini_joy_button\0"
    "update_mini_joy_axis\0update_camera_frame\0"
    "img\0update_camera_status\0update_camera_stats\0"
    "fps\0kbps\0w\0h\0update_anarov_frame\0"
    "update_anarov_status\0update_anarov_stats\0"
    "read_cam_ping\0read_anarov_ping\0"
    "ana_dir_pressed\0ana_dir_released\0"
    "mini_dir_pressed\0mini_dir_released\0"
    "on_emergency_ana\0on_emergency_mini\0"
    "on_stabilize_ana\0on_stabilize_mini\0"
    "on_autonomous_ana\0on_manual_ana\0"
    "on_minirov_launch\0on_torpedo_fire\0"
    "on_lamp_on_ana\0on_lamp_off_ana\0"
    "on_lamp_on_mini\0on_lamp_off_mini\0"
    "toggle_theme\0toggle_stabilize_mode_ana"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_PixhawkGUI[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      41,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,  219,    2, 0x08 /* Private */,
       3,    1,  220,    2, 0x08 /* Private */,
       5,    1,  223,    2, 0x08 /* Private */,
       7,    2,  226,    2, 0x08 /* Private */,
      10,    0,  231,    2, 0x08 /* Private */,
      11,    1,  232,    2, 0x08 /* Private */,
      12,    1,  235,    2, 0x08 /* Private */,
      13,    0,  238,    2, 0x08 /* Private */,
      14,    0,  239,    2, 0x08 /* Private */,
      15,    1,  240,    2, 0x08 /* Private */,
      16,    2,  243,    2, 0x08 /* Private */,
      19,    2,  248,    2, 0x08 /* Private */,
      22,    1,  253,    2, 0x08 /* Private */,
      23,    2,  256,    2, 0x08 /* Private */,
      24,    2,  261,    2, 0x08 /* Private */,
      25,    1,  266,    2, 0x08 /* Private */,
      27,    1,  269,    2, 0x08 /* Private */,
      28,    4,  272,    2, 0x08 /* Private */,
      33,    1,  281,    2, 0x08 /* Private */,
      34,    1,  284,    2, 0x08 /* Private */,
      35,    4,  287,    2, 0x08 /* Private */,
      36,    0,  296,    2, 0x08 /* Private */,
      37,    0,  297,    2, 0x08 /* Private */,
      38,    0,  298,    2, 0x08 /* Private */,
      39,    0,  299,    2, 0x08 /* Private */,
      40,    0,  300,    2, 0x08 /* Private */,
      41,    0,  301,    2, 0x08 /* Private */,
      42,    0,  302,    2, 0x08 /* Private */,
      43,    0,  303,    2, 0x08 /* Private */,
      44,    0,  304,    2, 0x08 /* Private */,
      45,    0,  305,    2, 0x08 /* Private */,
      46,    0,  306,    2, 0x08 /* Private */,
      47,    0,  307,    2, 0x08 /* Private */,
      48,    0,  308,    2, 0x08 /* Private */,
      49,    0,  309,    2, 0x08 /* Private */,
      50,    0,  310,    2, 0x08 /* Private */,
      51,    0,  311,    2, 0x08 /* Private */,
      52,    0,  312,    2, 0x08 /* Private */,
      53,    0,  313,    2, 0x08 /* Private */,
      54,    0,  314,    2, 0x08 /* Private */,
      55,    0,  315,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::Bool,    6,
    QMetaType::Void, QMetaType::Float, QMetaType::Float,    8,    9,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::Bool,    6,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   17,   18,
    QMetaType::Void, QMetaType::Int, QMetaType::Float,   20,   21,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   17,   18,
    QMetaType::Void, QMetaType::Int, QMetaType::Float,   20,   21,
    QMetaType::Void, QMetaType::QImage,   26,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::Int, QMetaType::Float, QMetaType::Int, QMetaType::Int,   29,   30,   31,   32,
    QMetaType::Void, QMetaType::QImage,   26,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::Int, QMetaType::Float, QMetaType::Int, QMetaType::Int,   29,   30,   31,   32,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void PixhawkGUI::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<PixhawkGUI *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->toggle_ana_connection(); break;
        case 1: _t->update_ana_status((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->update_ana_armed((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 3: _t->update_ana_attitude((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2]))); break;
        case 4: _t->toggle_mini_connection(); break;
        case 5: _t->update_mini_status((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: _t->update_mini_armed((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 7: _t->toggle_ana_joystick(); break;
        case 8: _t->toggle_mini_joystick(); break;
        case 9: _t->update_ana_joy_status((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 10: _t->update_ana_joy_button((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 11: _t->update_ana_joy_axis((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2]))); break;
        case 12: _t->update_mini_joy_status((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 13: _t->update_mini_joy_button((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 14: _t->update_mini_joy_axis((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2]))); break;
        case 15: _t->update_camera_frame((*reinterpret_cast< const QImage(*)>(_a[1]))); break;
        case 16: _t->update_camera_status((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 17: _t->update_camera_stats((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4]))); break;
        case 18: _t->update_anarov_frame((*reinterpret_cast< const QImage(*)>(_a[1]))); break;
        case 19: _t->update_anarov_status((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 20: _t->update_anarov_stats((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4]))); break;
        case 21: _t->read_cam_ping(); break;
        case 22: _t->read_anarov_ping(); break;
        case 23: _t->ana_dir_pressed(); break;
        case 24: _t->ana_dir_released(); break;
        case 25: _t->mini_dir_pressed(); break;
        case 26: _t->mini_dir_released(); break;
        case 27: _t->on_emergency_ana(); break;
        case 28: _t->on_emergency_mini(); break;
        case 29: _t->on_stabilize_ana(); break;
        case 30: _t->on_stabilize_mini(); break;
        case 31: _t->on_autonomous_ana(); break;
        case 32: _t->on_manual_ana(); break;
        case 33: _t->on_minirov_launch(); break;
        case 34: _t->on_torpedo_fire(); break;
        case 35: _t->on_lamp_on_ana(); break;
        case 36: _t->on_lamp_off_ana(); break;
        case 37: _t->on_lamp_on_mini(); break;
        case 38: _t->on_lamp_off_mini(); break;
        case 39: _t->toggle_theme(); break;
        case 40: _t->toggle_stabilize_mode_ana(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject PixhawkGUI::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_PixhawkGUI.data,
    qt_meta_data_PixhawkGUI,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *PixhawkGUI::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PixhawkGUI::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_PixhawkGUI.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int PixhawkGUI::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 41)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 41;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 41)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 41;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
