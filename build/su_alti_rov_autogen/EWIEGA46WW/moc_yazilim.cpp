/****************************************************************************
** Meta object code from reading C++ file 'yazilim.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../yazilim.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'yazilim.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_PixhawkGUI_t {
    QByteArrayData data[63];
    char stringdata0[918];
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
QT_MOC_LITERAL(3, 34, 19), // "update_ana_attitude"
QT_MOC_LITERAL(4, 54, 4), // "roll"
QT_MOC_LITERAL(5, 59, 5), // "pitch"
QT_MOC_LITERAL(6, 65, 3), // "yaw"
QT_MOC_LITERAL(7, 69, 18), // "update_ana_vfr_hud"
QT_MOC_LITERAL(8, 88, 3), // "alt"
QT_MOC_LITERAL(9, 92, 7), // "heading"
QT_MOC_LITERAL(10, 100, 5), // "speed"
QT_MOC_LITERAL(11, 106, 18), // "update_ana_battery"
QT_MOC_LITERAL(12, 125, 7), // "voltage"
QT_MOC_LITERAL(13, 133, 9), // "remaining"
QT_MOC_LITERAL(14, 143, 17), // "update_ana_status"
QT_MOC_LITERAL(15, 161, 3), // "msg"
QT_MOC_LITERAL(16, 165, 22), // "toggle_mini_connection"
QT_MOC_LITERAL(17, 188, 20), // "update_mini_attitude"
QT_MOC_LITERAL(18, 209, 19), // "update_mini_vfr_hud"
QT_MOC_LITERAL(19, 229, 19), // "update_mini_battery"
QT_MOC_LITERAL(20, 249, 18), // "update_mini_status"
QT_MOC_LITERAL(21, 268, 19), // "toggle_ana_joystick"
QT_MOC_LITERAL(22, 288, 20), // "toggle_mini_joystick"
QT_MOC_LITERAL(23, 309, 21), // "update_ana_joy_status"
QT_MOC_LITERAL(24, 331, 21), // "update_ana_joy_button"
QT_MOC_LITERAL(25, 353, 6), // "btn_id"
QT_MOC_LITERAL(26, 360, 5), // "state"
QT_MOC_LITERAL(27, 366, 19), // "update_ana_joy_axis"
QT_MOC_LITERAL(28, 386, 7), // "axis_id"
QT_MOC_LITERAL(29, 394, 5), // "value"
QT_MOC_LITERAL(30, 400, 22), // "update_mini_joy_status"
QT_MOC_LITERAL(31, 423, 22), // "update_mini_joy_button"
QT_MOC_LITERAL(32, 446, 20), // "update_mini_joy_axis"
QT_MOC_LITERAL(33, 467, 19), // "update_camera_frame"
QT_MOC_LITERAL(34, 487, 3), // "img"
QT_MOC_LITERAL(35, 491, 20), // "update_camera_status"
QT_MOC_LITERAL(36, 512, 19), // "update_camera_stats"
QT_MOC_LITERAL(37, 532, 3), // "fps"
QT_MOC_LITERAL(38, 536, 4), // "kbps"
QT_MOC_LITERAL(39, 541, 1), // "w"
QT_MOC_LITERAL(40, 543, 1), // "h"
QT_MOC_LITERAL(41, 545, 19), // "update_anarov_frame"
QT_MOC_LITERAL(42, 565, 20), // "update_anarov_status"
QT_MOC_LITERAL(43, 586, 19), // "update_anarov_stats"
QT_MOC_LITERAL(44, 606, 13), // "read_cam_ping"
QT_MOC_LITERAL(45, 620, 16), // "read_anarov_ping"
QT_MOC_LITERAL(46, 637, 15), // "ana_dir_pressed"
QT_MOC_LITERAL(47, 653, 16), // "ana_dir_released"
QT_MOC_LITERAL(48, 670, 16), // "mini_dir_pressed"
QT_MOC_LITERAL(49, 687, 17), // "mini_dir_released"
QT_MOC_LITERAL(50, 705, 16), // "on_emergency_ana"
QT_MOC_LITERAL(51, 722, 17), // "on_emergency_mini"
QT_MOC_LITERAL(52, 740, 16), // "on_stabilize_ana"
QT_MOC_LITERAL(53, 757, 17), // "on_stabilize_mini"
QT_MOC_LITERAL(54, 775, 17), // "on_autonomous_ana"
QT_MOC_LITERAL(55, 793, 13), // "on_manual_ana"
QT_MOC_LITERAL(56, 807, 17), // "on_minirov_launch"
QT_MOC_LITERAL(57, 825, 15), // "on_torpedo_fire"
QT_MOC_LITERAL(58, 841, 14), // "on_lamp_on_ana"
QT_MOC_LITERAL(59, 856, 15), // "on_lamp_off_ana"
QT_MOC_LITERAL(60, 872, 15), // "on_lamp_on_mini"
QT_MOC_LITERAL(61, 888, 16), // "on_lamp_off_mini"
QT_MOC_LITERAL(62, 905, 12) // "toggle_theme"

    },
    "PixhawkGUI\0toggle_ana_connection\0\0"
    "update_ana_attitude\0roll\0pitch\0yaw\0"
    "update_ana_vfr_hud\0alt\0heading\0speed\0"
    "update_ana_battery\0voltage\0remaining\0"
    "update_ana_status\0msg\0toggle_mini_connection\0"
    "update_mini_attitude\0update_mini_vfr_hud\0"
    "update_mini_battery\0update_mini_status\0"
    "toggle_ana_joystick\0toggle_mini_joystick\0"
    "update_ana_joy_status\0update_ana_joy_button\0"
    "btn_id\0state\0update_ana_joy_axis\0"
    "axis_id\0value\0update_mini_joy_status\0"
    "update_mini_joy_button\0update_mini_joy_axis\0"
    "update_camera_frame\0img\0update_camera_status\0"
    "update_camera_stats\0fps\0kbps\0w\0h\0"
    "update_anarov_frame\0update_anarov_status\0"
    "update_anarov_stats\0read_cam_ping\0"
    "read_anarov_ping\0ana_dir_pressed\0"
    "ana_dir_released\0mini_dir_pressed\0"
    "mini_dir_released\0on_emergency_ana\0"
    "on_emergency_mini\0on_stabilize_ana\0"
    "on_stabilize_mini\0on_autonomous_ana\0"
    "on_manual_ana\0on_minirov_launch\0"
    "on_torpedo_fire\0on_lamp_on_ana\0"
    "on_lamp_off_ana\0on_lamp_on_mini\0"
    "on_lamp_off_mini\0toggle_theme"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_PixhawkGUI[] = {

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
       3,    3,  230,    2, 0x08 /* Private */,
       7,    3,  237,    2, 0x08 /* Private */,
      11,    2,  244,    2, 0x08 /* Private */,
      14,    1,  249,    2, 0x08 /* Private */,
      16,    0,  252,    2, 0x08 /* Private */,
      17,    3,  253,    2, 0x08 /* Private */,
      18,    3,  260,    2, 0x08 /* Private */,
      19,    2,  267,    2, 0x08 /* Private */,
      20,    1,  272,    2, 0x08 /* Private */,
      21,    0,  275,    2, 0x08 /* Private */,
      22,    0,  276,    2, 0x08 /* Private */,
      23,    1,  277,    2, 0x08 /* Private */,
      24,    2,  280,    2, 0x08 /* Private */,
      27,    2,  285,    2, 0x08 /* Private */,
      30,    1,  290,    2, 0x08 /* Private */,
      31,    2,  293,    2, 0x08 /* Private */,
      32,    2,  298,    2, 0x08 /* Private */,
      33,    1,  303,    2, 0x08 /* Private */,
      35,    1,  306,    2, 0x08 /* Private */,
      36,    4,  309,    2, 0x08 /* Private */,
      41,    1,  318,    2, 0x08 /* Private */,
      42,    1,  321,    2, 0x08 /* Private */,
      43,    4,  324,    2, 0x08 /* Private */,
      44,    0,  333,    2, 0x08 /* Private */,
      45,    0,  334,    2, 0x08 /* Private */,
      46,    0,  335,    2, 0x08 /* Private */,
      47,    0,  336,    2, 0x08 /* Private */,
      48,    0,  337,    2, 0x08 /* Private */,
      49,    0,  338,    2, 0x08 /* Private */,
      50,    0,  339,    2, 0x08 /* Private */,
      51,    0,  340,    2, 0x08 /* Private */,
      52,    0,  341,    2, 0x08 /* Private */,
      53,    0,  342,    2, 0x08 /* Private */,
      54,    0,  343,    2, 0x08 /* Private */,
      55,    0,  344,    2, 0x08 /* Private */,
      56,    0,  345,    2, 0x08 /* Private */,
      57,    0,  346,    2, 0x08 /* Private */,
      58,    0,  347,    2, 0x08 /* Private */,
      59,    0,  348,    2, 0x08 /* Private */,
      60,    0,  349,    2, 0x08 /* Private */,
      61,    0,  350,    2, 0x08 /* Private */,
      62,    0,  351,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Float, QMetaType::Float, QMetaType::Float,    4,    5,    6,
    QMetaType::Void, QMetaType::Float, QMetaType::Float, QMetaType::Float,    8,    9,   10,
    QMetaType::Void, QMetaType::Float, QMetaType::Int,   12,   13,
    QMetaType::Void, QMetaType::QString,   15,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Float, QMetaType::Float, QMetaType::Float,    4,    5,    6,
    QMetaType::Void, QMetaType::Float, QMetaType::Float, QMetaType::Float,    8,    9,   10,
    QMetaType::Void, QMetaType::Float, QMetaType::Int,   12,   13,
    QMetaType::Void, QMetaType::QString,   15,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   15,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   25,   26,
    QMetaType::Void, QMetaType::Int, QMetaType::Float,   28,   29,
    QMetaType::Void, QMetaType::QString,   15,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   25,   26,
    QMetaType::Void, QMetaType::Int, QMetaType::Float,   28,   29,
    QMetaType::Void, QMetaType::QImage,   34,
    QMetaType::Void, QMetaType::QString,   15,
    QMetaType::Void, QMetaType::Int, QMetaType::Float, QMetaType::Int, QMetaType::Int,   37,   38,   39,   40,
    QMetaType::Void, QMetaType::QImage,   34,
    QMetaType::Void, QMetaType::QString,   15,
    QMetaType::Void, QMetaType::Int, QMetaType::Float, QMetaType::Int, QMetaType::Int,   37,   38,   39,   40,
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
        case 1: _t->update_ana_attitude((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< float(*)>(_a[3]))); break;
        case 2: _t->update_ana_vfr_hud((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< float(*)>(_a[3]))); break;
        case 3: _t->update_ana_battery((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 4: _t->update_ana_status((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->toggle_mini_connection(); break;
        case 6: _t->update_mini_attitude((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< float(*)>(_a[3]))); break;
        case 7: _t->update_mini_vfr_hud((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< float(*)>(_a[3]))); break;
        case 8: _t->update_mini_battery((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 9: _t->update_mini_status((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 10: _t->toggle_ana_joystick(); break;
        case 11: _t->toggle_mini_joystick(); break;
        case 12: _t->update_ana_joy_status((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 13: _t->update_ana_joy_button((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 14: _t->update_ana_joy_axis((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2]))); break;
        case 15: _t->update_mini_joy_status((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 16: _t->update_mini_joy_button((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 17: _t->update_mini_joy_axis((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2]))); break;
        case 18: _t->update_camera_frame((*reinterpret_cast< const QImage(*)>(_a[1]))); break;
        case 19: _t->update_camera_status((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 20: _t->update_camera_stats((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4]))); break;
        case 21: _t->update_anarov_frame((*reinterpret_cast< const QImage(*)>(_a[1]))); break;
        case 22: _t->update_anarov_status((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 23: _t->update_anarov_stats((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4]))); break;
        case 24: _t->read_cam_ping(); break;
        case 25: _t->read_anarov_ping(); break;
        case 26: _t->ana_dir_pressed(); break;
        case 27: _t->ana_dir_released(); break;
        case 28: _t->mini_dir_pressed(); break;
        case 29: _t->mini_dir_released(); break;
        case 30: _t->on_emergency_ana(); break;
        case 31: _t->on_emergency_mini(); break;
        case 32: _t->on_stabilize_ana(); break;
        case 33: _t->on_stabilize_mini(); break;
        case 34: _t->on_autonomous_ana(); break;
        case 35: _t->on_manual_ana(); break;
        case 36: _t->on_minirov_launch(); break;
        case 37: _t->on_torpedo_fire(); break;
        case 38: _t->on_lamp_on_ana(); break;
        case 39: _t->on_lamp_off_ana(); break;
        case 40: _t->on_lamp_on_mini(); break;
        case 41: _t->on_lamp_off_mini(); break;
        case 42: _t->toggle_theme(); break;
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
