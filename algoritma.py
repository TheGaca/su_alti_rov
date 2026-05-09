import time
import math
import struct
import os
from PyQt5.QtCore import QThread, pyqtSignal
from pymavlink import mavutil

class MAVLinkThread(QThread):
    # Signals to update the GUI
    attitude_signal = pyqtSignal(float, float, float) # roll, pitch, yaw
    vfr_hud_signal = pyqtSignal(float, float, float)  # alt, heading, speed
    battery_signal = pyqtSignal(float, int)           # voltage, remaining %
    status_signal = pyqtSignal(str)                   # Connection status messages

    def __init__(self, port='/dev/ttyACM0', baudrate=115200):
        super().__init__()
        self.port = port
        self.baudrate = baudrate
        self.running = True
        self.master = None

    def run(self):
        try:
            self.status_signal.emit(f"Bağlanıyor: {self.port} @ {self.baudrate}...")
            self.master = mavutil.mavlink_connection(self.port, baud=self.baudrate)
            self.master.wait_heartbeat(timeout=5)
            self.status_signal.emit(f"Bağlantı Başarılı! Sistem ID: {self.master.target_system}")
            
            while self.running:
                # Wait for next MAVLink message
                msg = self.master.recv_match(blocking=True, timeout=0.1)
                if not msg:
                    continue

                msg_type = msg.get_type()

                if msg_type == 'ATTITUDE':
                    self.attitude_signal.emit(math.degrees(msg.roll), 
                                              math.degrees(msg.pitch), 
                                              math.degrees(msg.yaw))
                elif msg_type == 'VFR_HUD':
                    self.vfr_hud_signal.emit(msg.alt, msg.heading, msg.groundspeed)
                elif msg_type == 'SYS_STATUS':
                    self.battery_signal.emit(msg.voltage_battery / 1000.0, msg.battery_remaining)
                elif msg_type == 'COMMAND_ACK':
                    print(f"MAVLink Komut Cevabı: {msg}")

        except Exception as e:
            self.status_signal.emit(f"Hata: {str(e)}")

    def arm_vehicle(self):
        """ Pixhawk'ı Arm (Aktif) durumuna geçirir. Bazen servolar sadece Arm durumunda çalışır. """
        if self.master:
            try:
                self.master.mav.command_long_send(
                    self.master.target_system,
                    self.master.target_component,
                    mavutil.mavlink.MAV_CMD_COMPONENT_ARM_DISARM,
                    0, 1, 0, 0, 0, 0, 0, 0
                )
            except Exception as e:
                print(f"Arm hatası: {e}")

    def set_servo(self, servo_no, pwm):
        """
        Pixhawk'ta Servo kontrolü yapar. 
        AUX 1 genelde servo_no=9'a denk gelir. 
        """
        self.arm_vehicle() # Garanti olsun diye komuttan hemen önce ARM gönderelim
        if self.master:
            try:
                self.master.mav.command_long_send(
                    self.master.target_system,
                    self.master.target_component,
                    mavutil.mavlink.MAV_CMD_DO_SET_SERVO,
                    0,          # confirmation
                    servo_no,   # param 1: servo number
                    pwm,        # param 2: pwm value
                    0, 0, 0, 0, 0 # params 3-7 not used
                )
                print(f"Servo {servo_no} -> {pwm} komutu gönderildi.")
            except Exception as e:
                print(f"Servo gönderimi hatası: {e}")

    def stop(self):
        self.running = False
        if self.master:
            self.master.close()
        self.wait()


class JoystickThread(QThread):
    button_signal = pyqtSignal(int, int) # button_id, state
    axis_signal = pyqtSignal(int, float) # axis_id, value
    status_signal = pyqtSignal(str)
    
    def __init__(self, device='/dev/input/js0'):
        super().__init__()
        self.device = device
        self.running = True
        
    def run(self):
        # Retry loop to connect if disconnected
        while self.running:
            if not os.path.exists(self.device):
                self.status_signal.emit("Kol Bağlı Değil")
                time.sleep(2)
                continue
                
            self.status_signal.emit("Kol Bağlandı!")
            try:
                with open(self.device, 'rb') as f:
                    while self.running:
                        # Non-blocking read would be better, but blocking read works 
                        # if we expect the user to not unplug it while running
                        evbuf = f.read(8)
                        if evbuf:
                            time_ms, value, type_, number = struct.unpack('IhBB', evbuf)
                            
                            if type_ & 0x01: # Button
                                self.button_signal.emit(number, value)
                            elif type_ & 0x02: # Axis
                                self.axis_signal.emit(number, value / 32767.0)
            except Exception as e:
                self.status_signal.emit(f"Hata: {str(e)}")
                time.sleep(2)

    def stop(self):
        self.running = False
        self.wait()
