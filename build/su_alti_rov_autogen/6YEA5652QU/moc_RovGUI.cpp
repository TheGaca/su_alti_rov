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
    QByteArrayData data[68];
    char stringdata0[1005];
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
QT_MOC_LITERAL(10, 106, 3), // "yaw"
QT_MOC_LITERAL(11, 110, 16), // "update_ana_depth"
QT_MOC_LITERAL(12, 127, 6), // "meters"
QT_MOC_LITERAL(13, 134, 17), // "vertical_speed_ms"
QT_MOC_LITERAL(14, 152, 14), // "update_ana_nem"
QT_MOC_LITERAL(15, 167, 12), // "humidity_pct"
QT_MOC_LITERAL(16, 180, 13), // "temperature_c"
QT_MOC_LITERAL(17, 194, 18), // "update_ana_torpedo"
QT_MOC_LITERAL(18, 213, 9), // "remaining"
QT_MOC_LITERAL(19, 223, 16), // "update_ana_wegsh"
QT_MOC_LITERAL(20, 240, 7), // "visible"
QT_MOC_LITERAL(21, 248, 22), // "toggle_mini_connection"
QT_MOC_LITERAL(22, 271, 18), // "update_mini_status"
QT_MOC_LITERAL(23, 290, 17), // "update_mini_armed"
QT_MOC_LITERAL(24, 308, 19), // "toggle_ana_joystick"
QT_MOC_LITERAL(25, 328, 20), // "toggle_mini_joystick"
QT_MOC_LITERAL(26, 349, 21), // "update_ana_joy_status"
QT_MOC_LITERAL(27, 371, 21), // "update_ana_joy_button"
QT_MOC_LITERAL(28, 393, 6), // "btn_id"
QT_MOC_LITERAL(29, 400, 5), // "state"
QT_MOC_LITERAL(30, 406, 19), // "update_ana_joy_axis"
QT_MOC_LITERAL(31, 426, 7), // "axis_id"
QT_MOC_LITERAL(32, 434, 5), // "value"
QT_MOC_LITERAL(33, 440, 22), // "update_mini_joy_status"
QT_MOC_LITERAL(34, 463, 22), // "update_mini_joy_button"
QT_MOC_LITERAL(35, 486, 20), // "update_mini_joy_axis"
QT_MOC_LITERAL(36, 507, 19), // "update_camera_frame"
QT_MOC_LITERAL(37, 527, 3), // "img"
QT_MOC_LITERAL(38, 531, 20), // "update_camera_status"
QT_MOC_LITERAL(39, 552, 19), // "update_camera_stats"
QT_MOC_LITERAL(40, 572, 3), // "fps"
QT_MOC_LITERAL(41, 576, 4), // "kbps"
QT_MOC_LITERAL(42, 581, 1), // "w"
QT_MOC_LITERAL(43, 583, 1), // "h"
QT_MOC_LITERAL(44, 585, 19), // "update_anarov_frame"
QT_MOC_LITERAL(45, 605, 20), // "update_anarov_status"
QT_MOC_LITERAL(46, 626, 19), // "update_anarov_stats"
QT_MOC_LITERAL(47, 646, 13), // "read_cam_ping"
QT_MOC_LITERAL(48, 660, 16), // "read_anarov_ping"
QT_MOC_LITERAL(49, 677, 15), // "ana_dir_pressed"
QT_MOC_LITERAL(50, 693, 16), // "ana_dir_released"
QT_MOC_LITERAL(51, 710, 16), // "mini_dir_pressed"
QT_MOC_LITERAL(52, 727, 17), // "mini_dir_released"
QT_MOC_LITERAL(53, 745, 20), // "send_motor_heartbeat"
QT_MOC_LITERAL(54, 766, 16), // "on_emergency_ana"
QT_MOC_LITERAL(55, 783, 17), // "on_emergency_mini"
QT_MOC_LITERAL(56, 801, 16), // "on_stabilize_ana"
QT_MOC_LITERAL(57, 818, 17), // "on_stabilize_mini"
QT_MOC_LITERAL(58, 836, 17), // "on_autonomous_ana"
QT_MOC_LITERAL(59, 854, 13), // "on_manual_ana"
QT_MOC_LITERAL(60, 868, 17), // "on_minirov_launch"
QT_MOC_LITERAL(61, 886, 15), // "on_torpedo_fire"
QT_MOC_LITERAL(62, 902, 14), // "on_lamp_on_ana"
QT_MOC_LITERAL(63, 917, 15), // "on_lamp_off_ana"
QT_MOC_LITERAL(64, 933, 15), // "on_lamp_on_mini"
QT_MOC_LITERAL(65, 949, 16), // "on_lamp_off_mini"
QT_MOC_LITERAL(66, 966, 12), // "toggle_theme"
QT_MOC_LITERAL(67, 979, 25) // "toggle_stabilize_mode_ana"

    },
    "RovGUI\0toggle_ana_connection\0\0"
    "update_ana_status\0msg\0update_ana_armed\0"
    "armed\0update_ana_attitude\0roll\0pitch\0"
    "yaw\0update_ana_depth\0meters\0"
    "vertical_speed_ms\0update_ana_nem\0"
    "humidity_pct\0temperature_c\0"
    "update_ana_torpedo\0remaining\0"
    "update_ana_wegsh\0visible\0"
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
      46,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,  244,    2, 0x08 /* Private */,
       3,    1,  245,    2, 0x08 /* Private */,
       5,    1,  248,    2, 0x08 /* Private */,
       7,    3,  251,    2, 0x08 /* Private */,
      11,    2,  258,    2, 0x08 /* Private */,
      14,    2,  263,    2, 0x08 /* Private */,
      17,    1,  268,    2, 0x08 /* Private */,
      19,    2,  271,    2, 0x08 /* Private */,
      21,    0,  276,    2, 0x08 /* Private */,
      22,    1,  277,    2, 0x08 /* Private */,
      23,    1,  280,    2, 0x08 /* Private */,
      24,    0,  283,    2, 0x08 /* Private */,
      25,    0,  284,    2, 0x08 /* Private */,
      26,    1,  285,    2, 0x08 /* Private */,
      27,    2,  288,    2, 0x08 /* Private */,
      30,    2,  293,    2, 0x08 /* Private */,
      33,    1,  298,    2, 0x08 /* Private */,
      34,    2,  301,    2, 0x08 /* Private */,
      35,    2,  306,    2, 0x08 /* Private */,
      36,    1,  311,    2, 0x08 /* Private */,
      38,    1,  314,    2, 0x08 /* Private */,
      39,    4,  317,    2, 0x08 /* Private */,
      44,    1,  326,    2, 0x08 /* Private */,
      45,    1,  329,    2, 0x08 /* Private */,
      46,    4,  332,    2, 0x08 /* Private */,
      47,    0,  341,    2, 0x08 /* Private */,
      48,    0,  342,    2, 0x08 /* Private */,
      49,    0,  343,    2, 0x08 /* Private */,
      50,    0,  344,    2, 0x08 /* Private */,
      51,    0,  345,    2, 0x08 /* Private */,
      52,    0,  346,    2, 0x08 /* Private */,
      53,    0,  347,    2, 0x08 /* Private */,
      54,    0,  348,    2, 0x08 /* Private */,
      55,    0,  349,    2, 0x08 /* Private */,
      56,    0,  350,    2, 0x08 /* Private */,
      57,    0,  351,    2, 0x08 /* Private */,
      58,    0,  352,    2, 0x08 /* Private */,
      59,    0,  353,    2, 0x08 /* Private */,
      60,    0,  354,    2, 0x08 /* Private */,
      61,    0,  355,    2, 0x08 /* Private */,
      62,    0,  356,    2, 0x08 /* Private */,
      63,    0,  357,    2, 0x08 /* Private */,
      64,    0,  358,    2, 0x08 /* Private */,
      65,    0,  359,    2, 0x08 /* Private */,
      66,    0,  360,    2, 0x08 /* Private */,
      67,    0,  361,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::Bool,    6,
    QMetaType::Void, QMetaType::Float, QMetaType::Float, QMetaType::Float,    8,    9,   10,
    QMetaType::Void, QMetaType::Float, QMetaType::Float,   12,   13,
    QMetaType::Void, QMetaType::Float, QMetaType::Float,   15,   16,
    QMetaType::Void, QMetaType::Int,   18,
    QMetaType::Void, QMetaType::Float, QMetaType::Bool,   10,   20,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::Bool,    6,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   28,   29,
    QMetaType::Void, QMetaType::Int, QMetaType::Float,   31,   32,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   28,   29,
    QMetaType::Void, QMetaType::Int, QMetaType::Float,   31,   32,
    QMetaType::Void, QMetaType::QImage,   37,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::Int, QMetaType::Float, QMetaType::Int, QMetaType::Int,   40,   41,   42,   43,
    QMetaType::Void, QMetaType::QImage,   37,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::Int, QMetaType::Float, QMetaType::Int, QMetaType::Int,   40,   41,   42,   43,
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
        case 3: _t->update_ana_attitude((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< float(*)>(_a[3]))); break;
        case 4: _t->update_ana_depth((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2]))); break;
        case 5: _t->update_ana_nem((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2]))); break;
        case 6: _t->update_ana_torpedo((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 7: _t->update_ana_wegsh((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 8: _t->toggle_mini_connection(); break;
        case 9: _t->update_mini_status((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 10: _t->update_mini_armed((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 11: _t->toggle_ana_joystick(); break;
        case 12: _t->toggle_mini_joystick(); break;
        case 13: _t->update_ana_joy_status((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 14: _t->update_ana_joy_button((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 15: _t->update_ana_joy_axis((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2]))); break;
        case 16: _t->update_mini_joy_status((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 17: _t->update_mini_joy_button((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 18: _t->update_mini_joy_axis((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2]))); break;
        case 19: _t->update_camera_frame((*reinterpret_cast< const QImage(*)>(_a[1]))); break;
        case 20: _t->update_camera_status((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 21: _t->update_camera_stats((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4]))); break;
        case 22: _t->update_anarov_frame((*reinterpret_cast< const QImage(*)>(_a[1]))); break;
        case 23: _t->update_anarov_status((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 24: _t->update_anarov_stats((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4]))); break;
        case 25: _t->read_cam_ping(); break;
        case 26: _t->read_anarov_ping(); break;
        case 27: _t->ana_dir_pressed(); break;
        case 28: _t->ana_dir_released(); break;
        case 29: _t->mini_dir_pressed(); break;
        case 30: _t->mini_dir_released(); break;
        case 31: _t->send_motor_heartbeat(); break;
        case 32: _t->on_emergency_ana(); break;
        case 33: _t->on_emergency_mini(); break;
        case 34: _t->on_stabilize_ana(); break;
        case 35: _t->on_stabilize_mini(); break;
        case 36: _t->on_autonomous_ana(); break;
        case 37: _t->on_manual_ana(); break;
        case 38: _t->on_minirov_launch(); break;
        case 39: _t->on_torpedo_fire(); break;
        case 40: _t->on_lamp_on_ana(); break;
        case 41: _t->on_lamp_off_ana(); break;
        case 42: _t->on_lamp_on_mini(); break;
        case 43: _t->on_lamp_off_mini(); break;
        case 44: _t->toggle_theme(); break;
        case 45: _t->toggle_stabilize_mode_ana(); break;
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
        if (_id < 46)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 46;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 46)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 46;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
