import sys
from PyQt5.QtWidgets import QApplication, QMainWindow
from PyQt5.QtCore import QTimer

# Diğer dosyalardan importlar
from tasarim import Ui_MainWindow
from algoritma import MAVLinkThread, JoystickThread

class PixhawkGUI(QMainWindow):
    def __init__(self):
        super().__init__()
        
        # Arayüzü (tasarim.py'den) yükle
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)
        
        self.mav_thread = None
        self.joy_thread = None
        self.pressed_buttons = set()
        
        self.button_map = {
            0: "A", 1: "B", 2: "X", 3: "Y",
            4: "LB", 5: "RB", 6: "Back", 7: "Start", 8: "Logitech",
            9: "Sol Stick Tık", 10: "Sağ Stick Tık"
        }
        self.axes_state = {0: 0.0, 1: 0.0}
        
        # Servo durumu (False: Kapalı/1100, True: Açık/1900)
        self.servo_open = False
        self.servo_pin = 9 # AUX 1 = 9
        
        # Buton aksiyonlarını bağla
        self.ui.btn_connect.clicked.connect(self.toggle_connection)
        
        # 500ms sonra otomatik başla
        QTimer.singleShot(500, self.start_connection)
        QTimer.singleShot(500, self.start_joystick)

    def toggle_connection(self):
        if self.mav_thread is None or not self.mav_thread.running:
            self.start_connection()
        else:
            self.stop_connection()

    def start_connection(self):
        if self.mav_thread and self.mav_thread.running:
            return

        port = self.ui.port_combo.currentText()
        baudrate = int(self.ui.baud_combo.currentText())
        
        self.mav_thread = MAVLinkThread(port=port, baudrate=baudrate)
        self.mav_thread.attitude_signal.connect(self.update_attitude)
        self.mav_thread.vfr_hud_signal.connect(self.update_vfr_hud)
        self.mav_thread.battery_signal.connect(self.update_battery)
        self.mav_thread.status_signal.connect(self.update_status)
        
        self.mav_thread.start()
        self.ui.btn_connect.setText("Bağlantıyı Kes")
        self.ui.btn_connect.setStyleSheet("background-color: #e74c3c; color: white;")
        
    def stop_connection(self):
        if self.mav_thread:
            self.mav_thread.stop()
            self.mav_thread = None
        
        self.ui.btn_connect.setText("Bağlan")
        self.ui.btn_connect.setStyleSheet("")
        self.update_status("Durum: Bağlantı Kesildi")
        self.reset_labels()

    def start_joystick(self):
        self.joy_thread = JoystickThread()
        self.joy_thread.button_signal.connect(self.update_joy_button)
        self.joy_thread.axis_signal.connect(self.update_joy_axis)
        self.joy_thread.status_signal.connect(self.update_joy_status)
        self.joy_thread.start()

    def update_joy_status(self, msg):
        self.ui.lbl_pad_status.setText(f"Durum: {msg}")
        if "Bağlandı" in msg:
            self.ui.lbl_pad_status.setStyleSheet("color: green; font-weight: bold;")
        else:
            self.ui.lbl_pad_status.setStyleSheet("color: red;")

    def update_joy_button(self, btn_id, state):
        btn_name = self.button_map.get(btn_id, f"Tuş {btn_id}")
        
        # Servo toggle mantığı (START tuşuna basıldığında)
        if btn_id == 7 and state == 1: # START tuşu (id=7) ve basılma anı
            self.toggle_servo()

        # Ekranda gösterim kısmı
        if state == 1:
            self.pressed_buttons.add(btn_name)
        elif state == 0 and btn_name in self.pressed_buttons:
            self.pressed_buttons.remove(btn_name)
            
        if self.pressed_buttons:
            self.ui.lbl_pad_buttons.setText("Basılan Tuş: " + ", ".join(self.pressed_buttons))
        else:
            self.ui.lbl_pad_buttons.setText("Basılan Tuş: Yok")

    def toggle_servo(self):
        if not self.mav_thread or not self.mav_thread.running:
            return # Pixhawk'a bağlı değilsek komut gönderemeyiz
            
        self.servo_open = not self.servo_open
        if self.servo_open:
            pwm_val = 1900 # 90 Derece Açık (Bazen 2000'dir)
            self.ui.lbl_servo_status.setText("Açık (90°)")
            self.ui.lbl_servo_status.setStyleSheet("color: green;")
        else:
            pwm_val = 1100 # Kapalı / 0 Derece (Bazen 1000'dir)
            self.ui.lbl_servo_status.setText("Kapalı")
            self.ui.lbl_servo_status.setStyleSheet("color: black;")
            
        # Algoritma üzerinden MAVLink komutunu fırlat
        self.mav_thread.set_servo(self.servo_pin, pwm_val)

    def update_joy_axis(self, axis_id, value):
        if axis_id in (0, 1):
            self.axes_state[axis_id] = value
            self.ui.lbl_pad_axes.setText(f"Sol Stick: X: {self.axes_state[0]:.2f}  Y: {self.axes_state[1]:.2f}")
        elif axis_id in (6, 7):
            name = "Sağ" if axis_id == 6 and value > 0 else "Sol" if axis_id == 6 and value < 0 else "Aşağı" if axis_id == 7 and value > 0 else "Yukarı" if axis_id == 7 and value < 0 else None
            
            dpad_names = {"Sağ", "Sol", "Yukarı", "Aşağı"}
            self.pressed_buttons -= dpad_names
            
            if name:
                self.pressed_buttons.add(name)
                
            if self.pressed_buttons:
                self.ui.lbl_pad_buttons.setText("Basılan Tuş: " + ", ".join(self.pressed_buttons))
            else:
                self.ui.lbl_pad_buttons.setText("Basılan Tuş: Yok")

    def reset_labels(self):
        labels = [self.ui.lbl_roll, self.ui.lbl_pitch, self.ui.lbl_yaw,
                  self.ui.lbl_alt, self.ui.lbl_heading, self.ui.lbl_speed,
                  self.ui.lbl_voltage, self.ui.lbl_battery]
        for lbl in labels:
            lbl.setText("---")
        self.ui.attitude_indicator.set_attitude(0, 0)

    def update_attitude(self, roll, pitch, yaw):
        # Pixhawk fiziksel olarak yan yatık (90 derece) monte edildiği için düzeltme:
        roll = roll + 90
        # Pitch ters ise düzelt (kullanıcının bir önceki isteğine göre ters bırakıldı veya eklendi)
        # pitch = -pitch 

        
        self.ui.lbl_roll.setText(f"{roll:.2f}°")
        self.ui.lbl_pitch.setText(f"{pitch:.2f}°")
        self.ui.lbl_yaw.setText(f"{yaw:.2f}°")
        self.ui.attitude_indicator.set_attitude(roll, pitch)

    def update_vfr_hud(self, alt, heading, speed):
        self.ui.lbl_alt.setText(f"{alt:.2f}")
        self.ui.lbl_heading.setText(f"{heading:.0f}°")
        self.ui.lbl_speed.setText(f"{speed:.2f}")

    def update_battery(self, voltage, remaining):
        self.ui.lbl_voltage.setText(f"{voltage:.2f}V")
        self.ui.lbl_battery.setText(f"{remaining}%")

    def update_status(self, msg):
        self.ui.lbl_status.setText(msg)
        if "Hata" in msg:
            self.ui.lbl_status.setStyleSheet("color: red; font-weight: bold;")
            self.stop_connection()
        elif "Başarılı" in msg:
            self.ui.lbl_status.setStyleSheet("color: green; font-weight: bold;")

    def closeEvent(self, event):
        self.stop_connection()
        if self.joy_thread:
            self.joy_thread.stop()
        event.accept()

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = PixhawkGUI()
    window.show()
    sys.exit(app.exec_())
