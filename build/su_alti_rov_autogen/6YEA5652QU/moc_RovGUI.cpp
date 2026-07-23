/****************************************************************************
** Meta object code from reading C++ file 'RovGUI.hpp'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../include/RovGUI.hpp"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'RovGUI.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_RovGUI_t {
    QByteArrayData data[60];
    char stringdata0[905];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_RovGUI_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_RovGUI_t qt_meta_stringdata_RovGUI = {
    {
QT_MOC_LITERAL(0, 0, 6), // "RovGUI"
QT_MOC_LITERAL(1, 7, 21), // "toggle_ana_connection"
QT_MOC_LITERAL(2, 29, 0), // ""
QT_MOC_LITERAL(3, 30, 17), // "update_ana_status"
QT_MOC_LITERAL(4, 48, 3), // "msg"
QT_MOC_LITERAL(5, 52, 16), // "update_ana_armed"
QT_MOC_LITERAL(6, 69, 5), // "armed"
QT_MOC_LITERAL(7, 75, 19), // "update_ana_attitude"
QT_MOC_LITERAL(8, 95, 4), // "roll"
QT_MOC_LITERAL(9, 100, 5), // "pitch"
QT_MOC_LITERAL(10, 106, 16), // "update_ana_depth"
QT_MOC_LITERAL(11, 123, 6), // "meters"
QT_MOC_LITERAL(12, 130, 17), // "vertical_speed_ms"
QT_MOC_LITERAL(13, 148, 22), // "toggle_mini_connection"
QT_MOC_LITERAL(14, 171, 18), // "update_mini_status"
QT_MOC_LITERAL(15, 190, 17), // "update_mini_armed"
QT_MOC_LITERAL(16, 208, 19), // "toggle_ana_joystick"
QT_MOC_LITERAL(17, 228, 20), // "toggle_mini_joystick"
QT_MOC_LITERAL(18, 249, 21), // "update_ana_joy_status"
QT_MOC_LITERAL(19, 271, 21), // "update_ana_joy_button"
QT_MOC_LITERAL(20, 293, 6), // "btn_id"
QT_MOC_LITERAL(21, 300, 5), // "state"
QT_MOC_LITERAL(22, 306, 19), // "update_ana_joy_axis"
QT_MOC_LITERAL(23, 326, 7), // "axis_id"
QT_MOC_LITERAL(24, 334, 5), // "value"
QT_MOC_LITERAL(25, 340, 22), // "update_mini_joy_status"
QT_MOC_LITERAL(26, 363, 22), // "update_mini_joy_button"
QT_MOC_LITERAL(27, 386, 20), // "update_mini_joy_axis"
QT_MOC_LITERAL(28, 407, 19), // "update_camera_frame"
QT_MOC_LITERAL(29, 427, 3), // "img"
QT_MOC_LITERAL(30, 431, 20), // "update_camera_status"
QT_MOC_LITERAL(31, 452, 19), // "update_camera_stats"
QT_MOC_LITERAL(32, 472, 3), // "fps"
QT_MOC_LITERAL(33, 476, 4), // "kbps"
QT_MOC_LITERAL(34, 481, 1), // "w"
QT_MOC_LITERAL(35, 483, 1), // "h"
QT_MOC_LITERAL(36, 485, 19), // "update_anarov_frame"
QT_MOC_LITERAL(37, 505, 20), // "update_anarov_status"
QT_MOC_LITERAL(38, 526, 19), // "update_anarov_stats"
QT_MOC_LITERAL(39, 546, 13), // "read_cam_ping"
QT_MOC_LITERAL(40, 560, 16), // "read_anarov_ping"
QT_MOC_LITERAL(41, 577, 15), // "ana_dir_pressed"
QT_MOC_LITERAL(42, 593, 16), // "ana_dir_released"
QT_MOC_LITERAL(43, 610, 16), // "mini_dir_pressed"
QT_MOC_LITERAL(44, 627, 17), // "mini_dir_released"
QT_MOC_LITERAL(45, 645, 20), // "send_motor_heartbeat"
QT_MOC_LITERAL(46, 666, 16), // "on_emergency_ana"
QT_MOC_LITERAL(47, 683, 17), // "on_emergency_mini"
QT_MOC_LITERAL(48, 701, 16), // "on_stabilize_ana"
QT_MOC_LITERAL(49, 718, 17), // "on_stabilize_mini"
QT_MOC_LITERAL(50, 736, 17), // "on_autonomous_ana"
QT_MOC_LITERAL(51, 754, 13), // "on_manual_ana"
QT_MOC_LITERAL(52, 768, 17), // "on_minirov_launch"
QT_MOC_LITERAL(53, 786, 15), // "on_torpedo_fire"
QT_MOC_LITERAL(54, 802, 14), // "on_lamp_on_ana"
QT_MOC_LITERAL(55, 817, 15), // "on_lamp_off_ana"
QT_MOC_LITERAL(56, 833, 15), // "on_lamp_on_mini"
QT_MOC_LITERAL(57, 849, 16), // "on_lamp_off_mini"
QT_MOC_LITERAL(58, 866, 12), // "toggle_theme"
QT_MOC_LITERAL(59, 879, 25) // "toggle_stabilize_mode_ana"

    },
    "RovGUI\0toggle_ana_connection\0\0"
    "update_ana_status\0msg\0update_ana_armed\0"
    "armed\0update_ana_attitude\0roll\0pitch\0"
    "update_ana_depth\0meters\0vertical_speed_ms\0"
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
    "send_motor_heartbeat\0on_emergency_ana\0"
    "on_emergency_mini\0on_stabilize_ana\0"
    "on_stabilize_mini\0on_autonomous_ana\0"
    "on_manual_ana\0on_minirov_launch\0"
    "on_torpedo_fire\0on_lamp_on_ana\0"
    "on_lamp_off_ana\0on_lamp_on_mini\0"
    "on_lamp_off_mini\0toggle_theme\0"
    "toggle_stabilize_mode_ana"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_RovGUI[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      43,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,  229,    2, 0x08 /* Private */,
       3,    1,  230,    2, 0x08 /* Private */,
       5,    1,  233,    2, 0x08 /* Private */,
       7,    2,  236,    2, 0x08 /* Private */,
      10,    2,  241,    2, 0x08 /* Private */,
      13,    0,  246,    2, 0x08 /* Private */,
      14,    1,  247,    2, 0x08 /* Private */,
      15,    1,  250,    2, 0x08 /* Private */,
      16,    0,  253,    2, 0x08 /* Private */,
      17,    0,  254,    2, 0x08 /* Private */,
      18,    1,  255,    2, 0x08 /* Private */,
      19,    2,  258,    2, 0x08 /* Private */,
      22,    2,  263,    2, 0x08 /* Private */,
      25,    1,  268,    2, 0x08 /* Private */,
      26,    2,  271,    2, 0x08 /* Private */,
      27,    2,  276,    2, 0x08 /* Private */,
      28,    1,  281,    2, 0x08 /* Private */,
      30,    1,  284,    2, 0x08 /* Private */,
      31,    4,  287,    2, 0x08 /* Private */,
      36,    1,  296,    2, 0x08 /* Private */,
      37,    1,  299,    2, 0x08 /* Private */,
      38,    4,  302,    2, 0x08 /* Private */,
      39,    0,  311,    2, 0x08 /* Private */,
      40,    0,  312,    2, 0x08 /* Private */,
      41,    0,  313,    2, 0x08 /* Private */,
      42,    0,  314,    2, 0x08 /* Private */,
      43,    0,  315,    2, 0x08 /* Private */,
      44,    0,  316,    2, 0x08 /* Private */,
      45,    0,  317,    2, 0x08 /* Private */,
      46,    0,  318,    2, 0x08 /* Private */,
      47,    0,  319,    2, 0x08 /* Private */,
      48,    0,  320,    2, 0x08 /* Private */,
      49,    0,  321,    2, 0x08 /* Private */,
      50,    0,  322,    2, 0x08 /* Private */,
      51,    0,  323,    2, 0x08 /* Private */,
      52,    0,  324,    2, 0x08 /* Private */,
      53,    0,  325,    2, 0x08 /* Private */,
      54,    0,  326,    2, 0x08 /* Private */,
      55,    0,  327,    2, 0x08 /* Private */,
      56,    0,  328,    2, 0x08 /* Private */,
      57,    0,  329,    2, 0x08 /* Private */,
      58,    0,  330,    2, 0x08 /* Private */,
      59,    0,  331,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::Bool,    6,
    QMetaType::Void, QMetaType::Float, QMetaType::Float,    8,    9,
    QMetaType::Void, QMetaType::Float, QMetaType::Float,   11,   12,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::Bool,    6,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   20,   21,
    QMetaType::Void, QMetaType::Int, QMetaType::Float,   23,   24,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   20,   21,
    QMetaType::Void, QMetaType::Int, QMetaType::Float,   23,   24,
    QMetaType::Void, QMetaType::QImage,   29,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::Int, QMetaType::Float, QMetaType::Int, QMetaType::Int,   32,   33,   34,   35,
    QMetaType::Void, QMetaType::QImage,   29,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::Int, QMetaType::Float, QMetaType::Int, QMetaType::Int,   32,   33,   34,   35,
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
    QMetaType::Void,

       0        // eod
};

void RovGUI::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<RovGUI *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->toggle_ana_connection(); break;
        case 1: _t->update_ana_status((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->update_ana_armed((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 3: _t->update_ana_attitude((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2]))); break;
        case 4: _t->update_ana_depth((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2]))); break;
        case 5: _t->toggle_mini_connection(); break;
        case 6: _t->update_mini_status((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 7: _t->update_mini_armed((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 8: _t->toggle_ana_joystick(); break;
        case 9: _t->toggle_mini_joystick(); break;
        case 10: _t->update_ana_joy_status((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 11: _t->update_ana_joy_button((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 12: _t->update_ana_joy_axis((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2]))); break;
        case 13: _t->update_mini_joy_status((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 14: _t->update_mini_joy_button((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 15: _t->update_mini_joy_axis((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2]))); break;
        case 16: _t->update_camera_frame((*reinterpret_cast< const QImage(*)>(_a[1]))); break;
        case 17: _t->update_camera_status((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 18: _t->update_camera_stats((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4]))); break;
        case 19: _t->update_anarov_frame((*reinterpret_cast< const QImage(*)>(_a[1]))); break;
        case 20: _t->update_anarov_status((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 21: _t->update_anarov_stats((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4]))); break;
        case 22: _t->read_cam_ping(); break;
        case 23: _t->read_anarov_ping(); break;
        case 24: _t->ana_dir_pressed(); break;
        case 25: _t->ana_dir_released(); break;
        case 26: _t->mini_dir_pressed(); break;
        case 27: _t->mini_dir_released(); break;
        case 28: _t->send_motor_heartbeat(); break;
        case 29: _t->on_emergency_ana(); break;
        case 30: _t->on_emergency_mini(); break;
        case 31: _t->on_stabilize_ana(); break;
        case 32: _t->on_stabilize_mini(); break;
        case 33: _t->on_autonomous_ana(); break;
        case 34: _t->on_manual_ana(); break;
        case 35: _t->on_minirov_launch(); break;
        case 36: _t->on_torpedo_fire(); break;
        case 37: _t->on_lamp_on_ana(); break;
        case 38: _t->on_lamp_off_ana(); break;
        case 39: _t->on_lamp_on_mini(); break;
        case 40: _t->on_lamp_off_mini(); break;
        case 41: _t->toggle_theme(); break;
        case 42: _t->toggle_stabilize_mode_ana(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject RovGUI::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_RovGUI.data,
    qt_meta_data_RovGUI,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *RovGUI::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *RovGUI::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_RovGUI.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int RovGUI::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 43)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 43;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 43)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 43;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
