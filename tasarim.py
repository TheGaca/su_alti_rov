from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QLabel, 
                             QGroupBox, QGridLayout, QPushButton, QComboBox)
from PyQt5.QtCore import Qt, QRectF, QPointF
from PyQt5.QtGui import QFont, QPainter, QColor, QPen, QPainterPath, QPolygonF

class AttitudeIndicator(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.roll = 0.0
        self.pitch = 0.0
        self.setMinimumSize(250, 250)

    def set_attitude(self, roll, pitch):
        self.roll = roll
        self.pitch = pitch
        self.update()

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)

        width = self.width()
        height = self.height()
        side = min(width, height)

        painter.translate(width / 2, height / 2)
        painter.scale(side / 250.0, side / 250.0)

        # Draw outer circle (black bezel)
        painter.setPen(QPen(QColor(30, 30, 30), 4))
        painter.setBrush(Qt.NoBrush)
        painter.drawEllipse(-120, -120, 240, 240)
        
        # Clip area for the inner horizon
        path = QPainterPath()
        path.addEllipse(-118, -118, 236, 236)
        painter.setClipPath(path)

        # Apply roll rotation
        painter.save()
        painter.rotate(-self.roll)
        
        # Calculate pitch translation
        pitch_pixels = self.pitch * 3.0
        painter.translate(0, pitch_pixels)

        # Sky (Blue)
        painter.setBrush(QColor(74, 144, 226))
        painter.setPen(Qt.NoPen)
        painter.drawRect(-250, -500, 500, 500)

        # Ground (Brown)
        painter.setBrush(QColor(139, 69, 19))
        painter.setPen(Qt.NoPen)
        painter.drawRect(-250, 0, 500, 500)

        # White dividing line
        painter.setPen(QPen(Qt.white, 2))
        painter.drawLine(-250, 0, 250, 0)

        # Pitch lines
        painter.setPen(QPen(Qt.white, 2))
        font = QFont("Arial", 10)
        painter.setFont(font)
        
        for p in range(-90, 91, 10):
            if p == 0:
                continue
            y = -p * 3.0
            w = 40 if p % 20 == 0 else 20
            painter.drawLine(-w, int(y), w, int(y))
            
            if p > 0:
                painter.drawText(-w - 25, int(y) + 5, str(p))
                painter.drawText(w + 5, int(y) + 5, str(p))
            else:
                painter.drawText(-w - 30, int(y) + 5, str(-p))
                painter.drawText(w + 5, int(y) + 5, str(-p))

        painter.restore()

        # Draw Aircraft Symbol (Orange)
        painter.setPen(QPen(QColor(255, 140, 0), 3))
        painter.drawLine(-60, 0, -20, 0)
        painter.drawLine(-20, 0, -20, 10)
        painter.drawLine(20, 0, 60, 0)
        painter.drawLine(20, 0, 20, 10)
        
        triangle = QPolygonF()
        triangle.append(QPointF(0, -10))
        triangle.append(QPointF(-10, 5))
        triangle.append(QPointF(10, 5))
        painter.setBrush(QColor(255, 140, 0))
        painter.drawPolygon(triangle)

        # Draw fixed roll pointer at the top
        painter.setPen(QPen(Qt.yellow, 2))
        painter.setBrush(Qt.yellow)
        roll_pointer = QPolygonF()
        roll_pointer.append(QPointF(0, -118))
        roll_pointer.append(QPointF(-8, -105))
        roll_pointer.append(QPointF(8, -105))
        painter.drawPolygon(roll_pointer)


class Ui_MainWindow(object):
    def setupUi(self, MainWindow):
        MainWindow.setWindowTitle("Pixhawk Telemetri Arayüzü")
        MainWindow.setMinimumSize(800, 500)
        
        self.main_widget = QWidget()
        MainWindow.setCentralWidget(self.main_widget)
        self.main_layout = QVBoxLayout()
        
        # --- Connection Bar ---
        self.conn_layout = QHBoxLayout()
        self.port_combo = QComboBox()
        self.port_combo.addItems(["/dev/ttyACM0", "/dev/ttyUSB0", "/dev/ttyUSB1"])
        
        self.baud_combo = QComboBox()
        self.baud_combo.addItems(["115200", "57600", "921600"])
        
        self.btn_connect = QPushButton("Bağlan")
        
        self.lbl_status = QLabel("Durum: Bekleniyor...")
        self.lbl_status.setStyleSheet("color: gray; font-weight: bold;")
        
        self.conn_layout.addWidget(QLabel("Port:"))
        self.conn_layout.addWidget(self.port_combo)
        self.conn_layout.addWidget(QLabel("Baudrate:"))
        self.conn_layout.addWidget(self.baud_combo)
        self.conn_layout.addWidget(self.btn_connect)
        self.conn_layout.addWidget(self.lbl_status)
        self.conn_layout.addStretch()
        
        # --- Telemetry Data Groups ---
        self.data_layout = QGridLayout()
        
        # 1. Attitude (Yönelim)
        self.group_att = QGroupBox("Yönelim (Attitude)")
        self.layout_att = QHBoxLayout()
        
        self.text_att_layout = QGridLayout()
        self.lbl_roll = self.create_data_label()
        self.lbl_pitch = self.create_data_label()
        self.lbl_yaw = self.create_data_label()
        
        self.text_att_layout.addWidget(QLabel("Roll:"), 0, 0)
        self.text_att_layout.addWidget(self.lbl_roll, 0, 1)
        self.text_att_layout.addWidget(QLabel("Pitch:"), 1, 0)
        self.text_att_layout.addWidget(self.lbl_pitch, 1, 1)
        self.text_att_layout.addWidget(QLabel("Yaw:"), 2, 0)
        self.text_att_layout.addWidget(self.lbl_yaw, 2, 1)
        
        self.attitude_indicator = AttitudeIndicator()
        
        self.layout_att.addLayout(self.text_att_layout)
        self.layout_att.addWidget(self.attitude_indicator)
        self.group_att.setLayout(self.layout_att)
        
        # 2. VFR HUD (Hız, Yükseklik, Yön)
        self.group_vfr = QGroupBox("Seyrüsefer (VFR HUD)")
        self.layout_vfr = QGridLayout()
        self.lbl_alt = self.create_data_label()
        self.lbl_heading = self.create_data_label()
        self.lbl_speed = self.create_data_label()
        
        self.layout_vfr.addWidget(QLabel("İrtifa (m):"), 0, 0)
        self.layout_vfr.addWidget(self.lbl_alt, 0, 1)
        self.layout_vfr.addWidget(QLabel("Pusula Yönü:"), 1, 0)
        self.layout_vfr.addWidget(self.lbl_heading, 1, 1)
        self.layout_vfr.addWidget(QLabel("Hız (m/s):"), 2, 0)
        self.layout_vfr.addWidget(self.lbl_speed, 2, 1)
        self.group_vfr.setLayout(self.layout_vfr)
        
        # 3. System Status (Batarya, Servo vs)
        self.group_sys = QGroupBox("Sistem & Sensör")
        self.layout_sys = QGridLayout()
        self.lbl_voltage = self.create_data_label()
        self.lbl_battery = self.create_data_label()
        self.lbl_servo_status = self.create_data_label()
        self.lbl_servo_status.setText("Kapalı")
        
        self.layout_sys.addWidget(QLabel("Batarya Voltaj:"), 0, 0)
        self.layout_sys.addWidget(self.lbl_voltage, 0, 1)
        self.layout_sys.addWidget(QLabel("Batarya %:"), 1, 0)
        self.layout_sys.addWidget(self.lbl_battery, 1, 1)
        self.layout_sys.addWidget(QLabel("AUX 1 Servo Durumu:"), 2, 0)
        self.layout_sys.addWidget(self.lbl_servo_status, 2, 1)
        self.group_sys.setLayout(self.layout_sys)
        
        # 4. Gamepad
        self.group_pad = QGroupBox("Oyun Kolu (Logitech)")
        self.layout_pad = QVBoxLayout()
        self.lbl_pad_status = QLabel("Durum: Bekleniyor...")
        self.lbl_pad_status.setStyleSheet("color: gray;")
        self.lbl_pad_axes = QLabel("Sol Stick: X: 0.00  Y: 0.00")
        self.lbl_pad_buttons = QLabel("Basılan Tuş: Yok")
        self.lbl_pad_buttons.setFont(QFont("Arial", 12, QFont.Bold))
        self.lbl_pad_buttons.setStyleSheet("color: #e67e22;")
        self.lbl_pad_buttons.setWordWrap(True)
        
        self.layout_pad.addWidget(self.lbl_pad_status)
        self.layout_pad.addWidget(self.lbl_pad_axes)
        self.layout_pad.addWidget(self.lbl_pad_buttons)
        self.layout_pad.addStretch()
        self.group_pad.setLayout(self.layout_pad)
        
        # Grid placement
        self.data_layout.addWidget(self.group_att, 0, 0)
        self.data_layout.addWidget(self.group_vfr, 0, 1)
        self.data_layout.addWidget(self.group_sys, 1, 0)
        self.data_layout.addWidget(self.group_pad, 1, 1)
        
        # Assemble Main Layout
        self.main_layout.addLayout(self.conn_layout)
        self.main_layout.addLayout(self.data_layout)
        self.main_widget.setLayout(self.main_layout)
        
        self.apply_styles(MainWindow)
        
    def create_data_label(self):
        lbl = QLabel("---")
        lbl.setFont(QFont("Arial", 16, QFont.Bold))
        lbl.setStyleSheet("color: #2c3e50;")
        lbl.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        return lbl
        
    def apply_styles(self, MainWindow):
        MainWindow.setStyleSheet("""
            QGroupBox {
                font-weight: bold;
                border: 1px solid #bdc3c7;
                border-radius: 5px;
                margin-top: 10px;
                padding-top: 15px;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 10px;
                padding: 0 3px 0 3px;
                color: #2980b9;
            }
        """)
