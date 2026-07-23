#include "Ui_MainWindow.hpp"

#include <QLabel>

void Ui_MainWindow::setupUi(QWidget *MainWindow) {
    MainWindow->setWindowTitle("Su Altı ROV Kontrol Arayüzü");
    main_widget = MainWindow;

    main_layout = new QVBoxLayout(MainWindow);
    main_layout->setContentsMargins(6, 6, 6, 6);
    main_layout->setSpacing(6);

    QHBoxLayout *title_layout = new QHBoxLayout();
    title_layout->setContentsMargins(10, 2, 10, 2);

    btn_theme = new QPushButton("🌙 Koyu Mod");
    btn_theme->setObjectName("btn_theme");
    btn_theme->setCursor(Qt::PointingHandCursor);
    btn_theme->setFixedSize(170, 34);
    btn_theme->setStyleSheet("font-size: 13px; font-weight: bold; background-color: #334155; color: white; border: none; border-radius: 4px; padding: 6px 12px;");

    // Sag taraftaki tema dugmesiyle ayni genislikte bosluk birakilarak
    // ortadaki baslik yaziyi gercekten pencere ortasina hizalanir.
    title_layout->addSpacing(btn_theme->width());
    title_layout->addStretch(1);

    QLabel *lbl_title = new QLabel("SuGaca");
    lbl_title->setObjectName("app_main_title");
    lbl_title->setAlignment(Qt::AlignCenter);
    lbl_title->setStyleSheet("font-size: 28px; font-weight: bold; color: #ff69b4; padding: 4px; letter-spacing: 3px; font-family: 'Outfit', 'Inter', sans-serif;");
    title_layout->addWidget(lbl_title);

    title_layout->addStretch(1);
    title_layout->addWidget(btn_theme);

    main_layout->addLayout(title_layout);

    panels_layout = new QHBoxLayout();
    panels_layout->setSpacing(8);

    anaRovPanel  = new RovPanel("ANA ROV",  false);
    miniRovPanel = new RovPanel("MİNİ ROV", true);

    panels_layout->addWidget(anaRovPanel,  1);
    panels_layout->addWidget(miniRovPanel, 1);
    main_layout->addLayout(panels_layout, 1);

    apply_styles(MainWindow, false); // start with light theme
}

void Ui_MainWindow::apply_styles(QWidget *MainWindow, bool dark) {
    QString style;
    if (dark) {
        style = R"(
        QWidget {
            background-color: #0b0f19;
            color: #e2e8f0;
            font-family: 'DejaVu Sans', 'Segoe UI', sans-serif;
            font-size: 13px;
        }
        QGroupBox {
            border: 1px solid #1e293b;
            border-radius: 4px;
            margin-top: 12px;
            padding-top: 6px;
            background-color: #111827;
            font-weight: 600;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 6px;
            background-color: #111827;
            color: #94a3b8;
            font-size: 13px;
        }
        QGroupBox#rov_main_group {
            border: 2px solid #334155;
            background-color: #111827;
        }
        QGroupBox#rov_main_group::title {
            font-size: 15px;
            font-weight: bold;
            color: #f1f5f9;
            background-color: #0b0f19;
            padding: 4px 14px;
            border: 2px solid #334155;
            border-bottom: none;
            border-top-left-radius: 3px;
            border-top-right-radius: 3px;
        }
        QLabel { background: transparent; color: #e2e8f0; }
        QLabel#section_label {
            font-weight: 600;
            color: #94a3b8;
        }
        QLabel#att_title {
            font-size: 15px;
            font-weight: 600;
            color: #94a3b8;
        }
        QLabel#attval {
            font-size: 16px;
            font-weight: bold;
            color: #f8fafc;
        }
        QLabel#hud_val {
            font-size: 14px;
            font-weight: bold;
            color: #cbd5e1;
        }
        QLabel#servo_status {
            font-size: 15px;
            font-weight: bold;
            color: #ef4444;
        }
        QLabel#lamp_status {
            font-size: 15px;
            font-weight: bold;
            color: #ef4444;
        }
        QLabel#cam_fps {
            font-size: 18px;
            font-weight: bold;
            color: #22c55e;
        }
        QLabel#cam_bitrate {
            font-size: 16px;
            font-weight: bold;
            color: #22c55e;
        }
        QLabel#cam_res {
            font-size: 14px;
            color: #94a3b8;
        }
        QLabel#cam_ping {
            font-size: 18px;
            font-weight: bold;
            color: #ec4899;
        }
        QLabel#cam_stream {
            background-color: #000000;
            color: #6b7280;
            border: 1px solid #1e293b;
            border-radius: 3px;
        }
        QLabel#pad_status {
            font-size: 14px;
            color: #ef4444;
            font-weight: bold;
        }
        QLabel#pad_axes {
            font-size: 13px;
            color: #94a3b8;
        }
        QLabel#pad_buttons {
            font-size: 13px;
            color: #f97316;
            font-weight: bold;
        }
        QLabel#lbl_status {
            font-weight: bold;
            color: #cbd5e1;
        }

        QPushButton {
            background-color: #1e293b;
            color: #f1f5f9;
            border: 1px solid #334155;
            padding: 4px 10px;
            border-radius: 3px;
        }
        QPushButton:hover  { background-color: #334155; }
        QPushButton:pressed{ background-color: #0f172a; }
        QPushButton#btn_connect {
            background-color: #1e293b;
            color: #f1f5f9;
            border: 1px solid #334155;
            font-weight: bold;
        }
        QPushButton#dir_btn {
            background-color: #1e293b;
            color: #f1f5f9;
            border: 1px solid #334155;
            font-size: 14px;
            font-weight: bold;
        }
        QPushButton#dir_btn:pressed {
            background-color: #3b82f6;
            color: white;
        }
        QPushButton#btn_stabilize {
            background-color: #2563eb;
            color: white;
            font-weight: bold;
            border: 1px solid #1d4ed8;
            border-radius: 4px;
            font-size: 16px;
        }
        QPushButton#btn_stabilize:hover  { background-color: #1d4ed8; }
        QPushButton#btn_stabilize:pressed{ background-color: #1e40af; }
        QPushButton#btn_emergency {
            background-color: #dc2626;
            color: white;
            font-weight: bold;
            font-size: 15px;
            border: 2px solid #991b1b;
            border-radius: 4px;
        }
        QPushButton#btn_emergency:pressed { background-color: #991b1b; }
        QPushButton#cmd_btn {
            background-color: #334155;
            color: white;
            border: 1px solid #1e293b;
            font-weight: bold;
            font-size: 13px;
        }
        QPushButton#cmd_btn:hover { background-color: #475569; }
        QPushButton#lamp_on {
            background-color: #f59e0b;
            color: #111111;
            font-weight: bold;
            border: 1px solid #b45309;
        }
        QPushButton#lamp_on:hover { background-color: #fbbf24; }
        QPushButton#lamp_off {
            background-color: #4b5563;
            color: white;
            font-weight: bold;
            border: 1px solid #374151;
        }
        QPushButton#lamp_off:hover { background-color: #6b7280; }

        QLabel#led_on {
            background-color: #22c55e;
            border: 1px solid #14532d;
            border-radius: 7px;
        }
        QLabel#led_off {
            background-color: #ef4444;
            border: 1px solid #7f1d1d;
            border-radius: 7px;
        }

        QComboBox {
            background-color: #1e293b;
            color: #f1f5f9;
            border: 1px solid #334155;
            border-radius: 3px;
            padding: 2px 6px;
        }
        QComboBox::drop-down {
            border-left: 1px solid #334155;
            width: 20px;
        }
        QProgressBar {
            border: 1px solid #334155;
            background-color: #1e293b;
            text-align: center;
        }
        QProgressBar::chunk {
            background-color: #3b82f6;
        }
        QTextEdit#terminal_log {
            background-color: #020617;
            color: #38bdf8;
            font-family: 'DejaVu Sans Mono', 'Courier New', monospace;
            font-size: 12px;
            border: 1px solid #1e293b;
            border-radius: 3px;
        }
        )";
    } else {
        style = R"(
        QWidget {
            background-color: #f2f4f7;
            color: #1f2937;
            font-family: 'DejaVu Sans', 'Segoe UI', sans-serif;
            font-size: 13px;
        }
        QGroupBox {
            border: 1px solid #b0b6be;
            border-radius: 4px;
            margin-top: 12px;
            padding-top: 6px;
            background-color: #ffffff;
            font-weight: 600;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 6px;
            background-color: #ffffff;
            color: #374151;
            font-size: 13px;
        }
        QGroupBox#rov_main_group {
            border: 2px solid #4b5563;
            background-color: #fafbfc;
        }
        QGroupBox#rov_main_group::title {
            font-size: 15px;
            font-weight: bold;
            color: #111827;
            background-color: #ffffff;
            padding: 4px 14px;
            border: 2px solid #4b5563;
            border-bottom: none;
            border-top-left-radius: 3px;
            border-top-right-radius: 3px;
        }
        QLabel { background: transparent; }
        QLabel#section_label {
            font-weight: 600;
            color: #4b5563;
        }
        QLabel#att_title {
            font-size: 15px;
            font-weight: 600;
            color: #374151;
        }
        QLabel#attval {
            font-size: 16px;
            font-weight: bold;
            color: #111827;
        }
        QLabel#hud_val {
            font-size: 14px;
            font-weight: bold;
            color: #111827;
        }
        QLabel#servo_status {
            font-size: 15px;
            font-weight: bold;
            color: #c0392b;
        }
        QLabel#cam_fps {
            font-size: 18px;
            font-weight: bold;
            color: #16a34a;
        }
        QLabel#cam_bitrate {
            font-size: 16px;
            font-weight: bold;
            color: #16a34a;
        }
        QLabel#cam_res {
            font-size: 14px;
            color: #4b5563;
        }
        QLabel#cam_ping {
            font-size: 18px;
            font-weight: bold;
            color: #db2777;
        }
        QLabel#cam_stream {
            background-color: #000;
            color: #6b7280;
            border: 1px solid #374151;
            border-radius: 3px;
        }
        QLabel#pad_status {
            font-size: 14px;
            color: #c0392b;
            font-weight: bold;
        }
        QLabel#pad_axes {
            font-size: 13px;
            color: #1f2937;
        }
        QLabel#pad_buttons {
            font-size: 13px;
            color: #ea580c;
            font-weight: bold;
        }
        QLabel#lbl_status {
            font-weight: bold;
            color: #4b5563;
        }

        QPushButton {
            background-color: #ffffff;
            border: 1px solid #6b7280;
            padding: 4px 10px;
            border-radius: 3px;
        }
        QPushButton:hover  { background-color: #e5e7eb; }
        QPushButton:pressed{ background-color: #d1d5db; }
        QPushButton#btn_connect {
            background-color: #ffffff;
            border: 1px solid #374151;
            font-weight: bold;
        }
        QPushButton#dir_btn {
            background-color: #ffffff;
            border: 1px solid #6b7280;
            font-size: 14px;
            font-weight: bold;
        }
        QPushButton#dir_btn:pressed {
            background-color: #1e3a8a;
            color: white;
        }
        QPushButton#btn_stabilize {
            background-color: #2563eb;
            color: white;
            font-weight: bold;
            border: 1px solid #1d4ed8;
            border-radius: 4px;
            font-size: 16px;
        }
        QPushButton#btn_stabilize:hover  { background-color: #1d4ed8; }
        QPushButton#btn_stabilize:pressed{ background-color: #1e40af; }
        QPushButton#btn_emergency {
            background-color: #dc2626;
            color: white;
            font-weight: bold;
            font-size: 15px;
            border: 2px solid #991b1b;
            border-radius: 4px;
        }
        QPushButton#btn_emergency:pressed { background-color: #991b1b; }
        QPushButton#cmd_btn {
            background-color: #334155;
            color: white;
            border: 1px solid #1e293b;
            font-weight: bold;
            font-size: 13px;
        }
        QPushButton#cmd_btn:hover { background-color: #475569; }
        QPushButton#lamp_on {
            background-color: #f59e0b;
            color: #111;
            font-weight: bold;
            border: 1px solid #b45309;
        }
        QPushButton#lamp_on:hover { background-color: #fbbf24; }
        QPushButton#lamp_off {
            background-color: #6b7280;
            color: white;
            font-weight: bold;
            border: 1px solid #374151;
        }
        QPushButton#lamp_off:hover { background-color: #4b5563; }

        QLabel#led_on {
            background-color: #22c55e;
            border: 1px solid #14532d;
            border-radius: 7px;
        }
        QLabel#led_off {
            background-color: #ef4444;
            border: 1px solid #7f1d1d;
            border-radius: 7px;
        }

        QComboBox {
            background-color: white;
            border: 1px solid #6b7280;
            border-radius: 3px;
            padding: 2px 6px;
        }
        QComboBox::drop-down {
            border-left: 1px solid #9ca3af;
            width: 20px;
        }
        QProgressBar {
            border: 1px solid #6b7280;
            background-color: white;
            text-align: center;
        }
        QProgressBar::chunk {
            background-color: #2563eb;
        }
        QTextEdit#terminal_log {
            background-color: #0b1220;
            color: #60a5fa;
            font-family: 'DejaVu Sans Mono', 'Courier New', monospace;
            font-size: 12px;
            border: 1px solid #1f2937;
            border-radius: 3px;
        }
        )";
    }
    MainWindow->setStyleSheet(style);
}
