#!/usr/bin/env python3
"""AnaROV bench test and calibration UI. Uses the camera ESP TCP bridge."""

from __future__ import annotations

import queue
import socket
import subprocess
import threading
import tkinter as tk
from pathlib import Path
from tkinter import messagebox, ttk


ROOT = Path(__file__).resolve().parent
CALIBRATION_FILE = ROOT / "kalibrasyon.txt"
MOTOR_COUNT = 8
TORPEDO_COUNT = 3
MOTOR_MIN_LIMIT = 1295
MOTOR_MAX_LIMIT = 1600
MOTOR_NEUTRAL_DEFAULT = 1487
TORPEDO_MIN_DEFAULTS = [1476, 1487, 1487]
TORPEDO_NEUTRAL_DEFAULTS = [1487, 1487, 1487]
TORPEDO_MAX_DEFAULTS = [1642, 1642, 1642]


def default_calibration() -> dict[str, list[dict[str, int]]]:
    return {
        "motors": [
            {"min": MOTOR_MIN_LIMIT, "neutral": MOTOR_NEUTRAL_DEFAULT,
             "max": MOTOR_MAX_LIMIT}
            for _ in range(MOTOR_COUNT)
        ],
        "torpedoes": [
            {"min": TORPEDO_MIN_DEFAULTS[i],
             "neutral": TORPEDO_NEUTRAL_DEFAULTS[i],
             "max": TORPEDO_MAX_DEFAULTS[i]}
            for i in range(TORPEDO_COUNT)
        ],
    }


def load_calibration() -> dict[str, list[dict[str, int]]]:
    data = default_calibration()
    if not CALIBRATION_FILE.exists():
        return data
    try:
        for raw_line in CALIBRATION_FILE.read_text(encoding="utf-8").splitlines():
            line = raw_line.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, raw_value = (part.strip() for part in line.split("=", 1))
            value = int(raw_value)
            for kind, count in (("motor", MOTOR_COUNT), ("torpedo", TORPEDO_COUNT)):
                for index in range(1, count + 1):
                    for point in ("min", "neutral", "max"):
                        if key == f"{kind}{index}_{point}":
                            section = "motors" if kind == "motor" else "torpedoes"
                            data[section][index - 1][point] = value
    except (OSError, ValueError):
        return default_calibration()
    return data


class TestApp:
    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        self.root.title("AnaROV Motor ve Torpido Test / Kalibrasyon")
        self.root.geometry("1120x900")
        self.root.minsize(850, 650)

        self.calibration = load_calibration()
        self.motor_pulses = [item["neutral"] for item in self.calibration["motors"]]
        self.motor_test_index: int | None = None
        self.armed = False
        self.sock: socket.socket | None = None
        self.net_lock = threading.Lock()
        self.events: queue.Queue[tuple[str, str]] = queue.Queue()
        self.closing = False
        self.auto_reconnect = True
        self.connecting = False
        self.retry_after_id: str | None = None
        self.motor_scales: list[tk.Scale] = []
        self.motor_value_labels: list[ttk.Label] = []
        self.motor_test_buttons: list[ttk.Button] = []
        self.torpedo_scales: list[tk.Scale] = []
        self.torpedo_value_labels: list[ttk.Label] = []

        self._build_ui()
        self._poll_events()
        self._heartbeat()
        self.root.after(350, lambda: self._connect(automatic=True))
        self.root.protocol("WM_DELETE_WINDOW", self._on_close)

    def _build_ui(self) -> None:
        top = ttk.Frame(self.root, padding=10)
        top.pack(fill="x")

        ttk.Label(top, text="Kamera ESP IP:").pack(side="left")
        self.host_var = tk.StringVar(value="192.168.2.220")
        ttk.Entry(top, textvariable=self.host_var, width=18).pack(side="left", padx=(5, 12))
        ttk.Label(top, text="TCP port:").pack(side="left")
        self.port_var = tk.StringVar(value="8888")
        ttk.Entry(top, textvariable=self.port_var, width=7).pack(side="left", padx=(5, 12))

        self.connect_button = ttk.Button(top, text="Bağlantıyı kes", command=self._on_connect_button)
        self.connect_button.pack(side="left", padx=3)
        self.arm_button = ttk.Button(top, text="ARM Et", command=self._arm)
        self.arm_button.pack(side="left", padx=3)
        self.disarm_button = ttk.Button(top, text="DISARM / ACİL DURDUR", command=self._disarm)
        self.disarm_button.pack(side="left", padx=3)

        self.status_var = tk.StringVar(value="Bağlı değil")
        ttk.Label(top, textvariable=self.status_var, foreground="#174ea6").pack(
            side="left", padx=12
        )

        warning = ttk.Label(
            self.root,
            text=("GÜVENLİK: İlk testte pervaneleri sök, aracı sabitle ve torpidoları boşalt. "
                  "ARM motorlara enerji verir. Torpido komutu 3 saniye çalışır."),
            foreground="#a00000", wraplength=1080, padding=(12, 4),
        )
        warning.pack(fill="x")

        notebook = ttk.Notebook(self.root)
        notebook.pack(fill="both", expand=True, padx=8, pady=8)
        motor_tab = ttk.Frame(notebook)
        torpedo_tab = ttk.Frame(notebook)
        notebook.add(motor_tab, text="8 Motor Kalibrasyonu")
        notebook.add(torpedo_tab, text="Torpido Kalibrasyonu / Test")

        self._build_motor_tab(motor_tab)
        self._build_torpedo_tab(torpedo_tab)

        bottom = ttk.Frame(self.root, padding=(10, 0, 10, 8))
        bottom.pack(fill="x")
        ttk.Label(
            bottom,
            text=f"Kalibrasyon dosyası: {CALIBRATION_FILE}",
            foreground="#444", wraplength=1080,
        ).pack(anchor="w")
        ttk.Button(bottom, text="kalibrasyon.txt Kaydet", command=self._save_calibration).pack(
            anchor="e", pady=(4, 0)
        )

    def _build_motor_tab(self, parent: ttk.Frame) -> None:
        outer = ttk.Frame(parent)
        outer.pack(fill="both", expand=True)
        canvas = tk.Canvas(outer, highlightthickness=0)
        scrollbar = ttk.Scrollbar(outer, orient="vertical", command=canvas.yview)
        content = ttk.Frame(canvas, padding=8)
        content_window = canvas.create_window((0, 0), window=content, anchor="nw")
        content.bind("<Configure>", lambda _e: canvas.configure(scrollregion=canvas.bbox("all")))
        canvas.bind(
            "<Configure>",
            lambda event: canvas.itemconfigure(content_window, width=event.width),
        )
        canvas.configure(yscrollcommand=scrollbar.set)
        canvas.pack(side="left", fill="both", expand=True)
        scrollbar.pack(side="right", fill="y")

        ttk.Label(
            content,
            text=("Çubuğu seçili motorun darbe değerine getir. Altındaki Geri/Orta/İleri "
                  "düğmeleri o değeri dosyaya kaydeder. Hızı slider ile canlı ayarlamak için "
                  "ARM sonrası 'Motoru test et' düğmesine bas; tekrar basınca motor nötre döner."),
            wraplength=1020,
        ).pack(anchor="w", pady=(0, 8))

        for index in range(MOTOR_COUNT):
            row = ttk.LabelFrame(content, text=f"Motor {index + 1}", padding=(8, 6))
            row.pack(fill="x", expand=True, pady=4)
            slider_row = ttk.Frame(row)
            slider_row.pack(fill="x")
            scale = tk.Scale(
                slider_row, from_=MOTOR_MIN_LIMIT, to=MOTOR_MAX_LIMIT, orient="horizontal",
                resolution=1, showvalue=False, length=650, sliderlength=28,
            )
            scale.pack(side="left", fill="x", expand=True, padx=(4, 10))
            value_label = ttk.Label(slider_row, text=str(scale.get()), width=6,
                                    anchor="e", font=("TkFixedFont", 10))
            value_label.pack(side="right", padx=(0, 4))
            self.motor_scales.append(scale)
            self.motor_value_labels.append(value_label)
            scale.configure(command=lambda value, i=index: self._motor_scale_changed(i, value))
            scale.set(self.calibration["motors"][index]["neutral"])

            buttons = ttk.Frame(row)
            buttons.pack(fill="x", padx=4, pady=(3, 0))
            for label, point in (("Geri / Min", "min"),
                                 ("Orta / Nötr", "neutral"),
                                 ("İleri / Max", "max")):
                ttk.Button(
                    buttons, text=label,
                    command=lambda i=index, p=point: self._capture_motor(i, p),
                ).pack(side="left", fill="x", expand=True, padx=2)

            test = ttk.Button(
                buttons, text="Motoru test et",
                command=lambda i=index: self._toggle_motor_test(i),
            )
            test.pack(side="right", fill="x", expand=True, padx=2)
            self.motor_test_buttons.append(test)

    def _build_torpedo_tab(self, parent: ttk.Frame) -> None:
        intro = ttk.Label(
            parent,
            text=("Torpido ayarları min/kolu kurma darbesi, nötr ve ateşleme darbesidir. "
                  "Kaydet düğmeleri değeri kalibrasyon.txt'ye yazar ve bağlı ESP'ye TCFG gönderir."),
            wraplength=1050, padding=10,
        )
        intro.pack(anchor="w")

        for index in range(TORPEDO_COUNT):
            row = ttk.LabelFrame(parent, text=f"Torpido {index + 1}", padding=(8, 6))
            row.pack(fill="x", padx=8, pady=5)
            slider_row = ttk.Frame(row)
            slider_row.pack(fill="x")
            scale = tk.Scale(
                slider_row, from_=1000, to=2000, orient="horizontal", resolution=1,
                showvalue=False, length=650, sliderlength=28,
            )
            scale.pack(side="left", fill="x", expand=True, padx=(4, 10))
            value_label = ttk.Label(slider_row, text=str(scale.get()), width=6,
                                    anchor="e", font=("TkFixedFont", 10))
            value_label.pack(side="right", padx=(0, 4))
            self.torpedo_scales.append(scale)
            self.torpedo_value_labels.append(value_label)
            scale.configure(command=lambda value, i=index: self._torpedo_scale_changed(i, value))
            scale.set(self.calibration["torpedoes"][index]["neutral"])

            buttons = ttk.Frame(row)
            buttons.pack(fill="x", padx=4, pady=(3, 0))
            for label, point in (("Kaydet Min", "min"),
                                 ("Kaydet Nötr", "neutral"),
                                 ("Kaydet Max", "max")):
                ttk.Button(
                    buttons, text=label,
                    command=lambda i=index, p=point: self._capture_torpedo(i, p),
                ).pack(side="left", fill="x", expand=True, padx=2)
            ttk.Button(
                buttons, text="Torpido ateşle…",
                command=lambda i=index: self._torpedo_action(i, reverse=False),
            ).pack(side="left", fill="x", expand=True, padx=2)
            ttk.Button(
                buttons, text="Ters test…",
                command=lambda i=index: self._torpedo_action(i, reverse=True),
            ).pack(side="left", fill="x", expand=True, padx=2)

        ttk.Button(
            parent, text="Torpido sayacını sıfırla (DISARMED)",
            command=self._reset_torpedoes,
        ).pack(anchor="e", padx=12, pady=10)

    def _on_connect_button(self) -> None:
        if self.sock is not None:
            self.auto_reconnect = False
            self._disconnect()
            return
        self.auto_reconnect = True
        if self.retry_after_id is not None:
            self.root.after_cancel(self.retry_after_id)
            self.retry_after_id = None
        self._connect()

    def _prepare_ethernet(self) -> bool:
        """Add the AnaROV subnet address without flushing DHCP/other addresses."""
        commands = (
            ["sudo", "-n", "ip", "addr", "replace", "192.168.2.10/24", "dev", "enp46s0"],
            ["sudo", "-n", "ip", "link", "set", "enp46s0", "up"],
        )
        try:
            results = [
                subprocess.run(command, stdout=subprocess.DEVNULL,
                               stderr=subprocess.DEVNULL, timeout=3, check=False)
                for command in commands
            ]
            return all(result.returncode == 0 for result in results)
        except (OSError, subprocess.TimeoutExpired):
            return False

    def _schedule_reconnect(self) -> None:
        if self.auto_reconnect and not self.closing and self.retry_after_id is None:
            self.retry_after_id = self.root.after(2000, self._retry_connect)

    def _retry_connect(self) -> None:
        self.retry_after_id = None
        if self.auto_reconnect and self.sock is None:
            self._connect(automatic=True)

    def _connect(self, automatic: bool = False) -> None:
        if self.sock is not None or self.connecting:
            return
        self.connecting = True
        ethernet_ready = self._prepare_ethernet()
        try:
            host = self.host_var.get().strip()
            port = int(self.port_var.get().strip())
            conn = socket.create_connection((host, port), timeout=3.0)
            conn.settimeout(0.25)
        except (OSError, ValueError) as exc:
            self.connecting = False
            suffix = " Ethernet ayarı için `sudo -v` çalıştır." if not ethernet_ready else ""
            self.status_var.set(f"{host if 'host' in locals() else 'AnaROV'} bağlantısı bekleniyor: {exc}.{suffix}")
            self.connect_button.configure(text="Yeniden dene")
            if not automatic:
                messagebox.showerror("Bağlantı kurulamadı", self.status_var.get(), parent=self.root)
            self._schedule_reconnect()
            return

        self.connecting = False
        with self.net_lock:
            self.sock = conn
        self.connect_button.configure(text="Bağlantıyı kes")
        self.status_var.set(f"Bağlandı: {host}:{port}")
        threading.Thread(target=self._receive_loop, args=(conn,), daemon=True).start()
        self._send_torpedo_config()

    def _disconnect(self) -> None:
        if self.armed:
            self._disarm()
        with self.net_lock:
            conn, self.sock = self.sock, None
        if conn:
            try:
                conn.shutdown(socket.SHUT_RDWR)
            except OSError:
                pass
            try:
                conn.close()
            except OSError:
                pass
        self.connect_button.configure(text="Bağlan")
        self.status_var.set("Bağlı değil")

    def _receive_loop(self, conn: socket.socket) -> None:
        buffer = bytearray()
        while not self.closing:
            try:
                chunk = conn.recv(1024)
                if not chunk:
                    break
                buffer.extend(chunk)
                while b"\n" in buffer:
                    raw, _, rest = buffer.partition(b"\n")
                    buffer = bytearray(rest)
                    line = raw.decode("utf-8", errors="replace").strip()
                    if line:
                        self.events.put(("line", line))
            except socket.timeout:
                continue
            except OSError as exc:
                self.events.put(("error", str(exc)))
                break
        with self.net_lock:
            if self.sock is conn:
                self.sock = None
        self.events.put(("disconnected", "TCP bağlantısı kapandı"))

    def _send(self, line: str) -> bool:
        payload = (line.strip() + "\n").encode("utf-8")
        with self.net_lock:
            conn = self.sock
            if conn is None:
                return False
            try:
                conn.sendall(payload)
                return True
            except OSError as exc:
                self.events.put(("error", str(exc)))
                return False

    def _poll_events(self) -> None:
        try:
            while True:
                kind, message = self.events.get_nowait()
                if kind == "line":
                    self.status_var.set(message)
                    if message == "ARMED":
                        self.armed = True
                        self.status_var.set("ARMED — test hazır")
                    elif message == "ARMING":
                        self.status_var.set("ARMING — ESC'ler hazırlanıyor")
                    elif message == "DISARMED":
                        self.armed = False
                        self._end_motor_test(send_neutral=False)
                        self.status_var.set("DISARMED")
                    elif message.startswith("TORPEDO:"):
                        self.status_var.set(f"Torpido durumu: {message}")
                elif kind in ("error", "disconnected"):
                    self.armed = False
                    self._end_motor_test(send_neutral=False)
                    self.connect_button.configure(text="Bağlan")
                    self.status_var.set(message)
                    self._schedule_reconnect()
        except queue.Empty:
            pass
        if not self.closing:
            self.root.after(100, self._poll_events)

    def _arm(self) -> None:
        if self.sock is None:
            messagebox.showwarning("Bağlantı yok", "Önce 192.168.2.220:8888 köprüsüne bağlan.", parent=self.root)
            return
        if not messagebox.askyesno(
            "Motorları ARM et",
            "Pervaneler sökülmüş, araç sabitlenmiş ve torpidolar boş mu?\n\n"
            "ARM komutu ESC'leri etkinleştirir.",
            parent=self.root,
        ):
            return
        self._send("ARM")
        self.status_var.set("ARM komutu gönderildi; ESP32 yanıtı bekleniyor…")

    def _disarm(self) -> None:
        self._end_motor_test(send_neutral=False)
        self.motor_pulses = [self.calibration["motors"][i]["neutral"] for i in range(MOTOR_COUNT)]
        self._send("DISARM")
        self.armed = False
        self.status_var.set("DISARM komutu gönderildi")

    def _motor_scale_changed(self, index: int, value: str) -> None:
        pulse = int(float(value))
        self.motor_value_labels[index].configure(text=str(pulse))
        if self.motor_test_index == index:
            self.motor_pulses[index] = pulse
            self._send_motor_vector()

    def _capture_motor(self, index: int, point: str) -> None:
        pulse = int(self.motor_scales[index].get())
        self.calibration["motors"][index][point] = pulse
        self._save_calibration()
        self.status_var.set(f"Motor {index + 1} {point}={pulse} µs kalibrasyona kaydedildi")

    def _begin_motor_test(self, index: int) -> None:
        if not self.armed or self.sock is None:
            self.status_var.set("Motor testi için TCP bağlantısı ve ARM gerekli")
            return
        if self.motor_test_index is not None and self.motor_test_index != index:
            self._end_motor_test()
        self.motor_test_index = index
        # Test sırasında yalnız seçili motor hareket etsin; diğer kanallar sabit
        # firmware nötründe kalır, kaydedilmemiş kalibrasyon değerleri uygulanmaz.
        self.motor_pulses = [item["neutral"] for item in self.calibration["motors"]]
        self.motor_pulses[index] = int(self.motor_scales[index].get())
        self._set_motor_test_button_state()
        self._send_motor_vector()
        self.status_var.set(f"Motor {index + 1} testte — slider hızı anlık değiştirir")

    def _toggle_motor_test(self, index: int) -> None:
        if self.motor_test_index == index:
            self._end_motor_test()
            self.status_var.set(f"Motor {index + 1} testi durdu; nötr komutu gönderildi")
            return
        self._begin_motor_test(index)

    def _set_motor_test_button_state(self) -> None:
        for i, button in enumerate(self.motor_test_buttons):
            button.configure(text="Testi durdur" if self.motor_test_index == i else "Motoru test et")

    def _end_motor_test(self, index: int | None = None, send_neutral: bool = True) -> None:
        if index is not None and self.motor_test_index != index:
            return
        was_testing = self.motor_test_index is not None
        self.motor_test_index = None
        self._set_motor_test_button_state()
        if was_testing and send_neutral and self.armed:
            self.motor_pulses = [item["neutral"] for item in self.calibration["motors"]]
            self._send_motor_vector()

    def _send_motor_vector(self) -> None:
        values = [max(MOTOR_MIN_LIMIT, min(MOTOR_MAX_LIMIT, v)) for v in self.motor_pulses]
        self._send("M:" + ",".join(str(value) for value in values))

    def _heartbeat(self) -> None:
        if self.armed and self.sock is not None and self.motor_test_index is not None:
            self._send_motor_vector()
        if not self.closing:
            self.root.after(150, self._heartbeat)

    def _torpedo_scale_changed(self, index: int, value: str) -> None:
        self.torpedo_value_labels[index].configure(text=str(int(float(value))))

    def _capture_torpedo(self, index: int, point: str) -> None:
        pulse = int(self.torpedo_scales[index].get())
        self.calibration["torpedoes"][index][point] = pulse
        self._save_calibration()
        self._send_torpedo_config()
        self.status_var.set(f"Torpido {index + 1} {point}={pulse} µs kaydedildi; TCFG gönderildi")

    def _send_torpedo_config(self) -> None:
        values: list[int] = []
        for item in self.calibration["torpedoes"]:
            values.extend((item["min"], item["neutral"], item["max"]))
        self._send("TCFG:" + ",".join(str(value) for value in values))

    def _torpedo_action(self, index: int, reverse: bool) -> None:
        if self.sock is None or not self.armed:
            messagebox.showwarning("ARM gerekli", "Torpido testi için bağlantı ve ARM gerekli.", parent=self.root)
            return
        action = "TERS TEST" if reverse else "ATEŞLEME TESTİ"
        if not messagebox.askyesno(
            f"Torpido {index + 1} — {action}",
            "Bu komut torpido çıkışını yaklaşık 3 saniye sürer.\n"
            "Torpido boş ve araç güvenli biçimde sabit mi?",
            parent=self.root,
        ):
            return
        self._send(f"TORPEDOREV:{index}" if reverse else f"TORPEDO:{index}")
        self.status_var.set(f"Torpido {index + 1}: {action} komutu gönderildi")

    def _reset_torpedoes(self) -> None:
        if self.armed:
            messagebox.showwarning("DISARM gerekli", "Torpido sayacı yalnız DISARMED durumunda sıfırlanabilir.", parent=self.root)
            return
        if messagebox.askyesno("Torpido sayacı", "Üç torpidoyu da yeniden kullanılabilir olarak işaretle?", parent=self.root):
            self._send("TORPEDORESET")

    def _save_calibration(self) -> None:
        lines = [
            "# AnaROV kalibrasyon kayıtları — test-arayuz.py tarafından oluşturulur.",
            "# Bu dosya ham ölçüm/ayar çıktısıdır; firmware'i kendiliğinden değiştirmez.",
            "# İlk değerler firmware varsayılanlarıdır; ölçülmüş kalibrasyon sayılmaz.",
            "# Değerler ESC darbe genişliğidir (mikrosaniye, us).",
            "#",
            "# Kalibrasyon tamamlanınca bu dosyayı Codex'e gönder.",
            "# Güncellenecek dosyalar:",
            "#   SuAltiEsp32AnaRov/AnaRovBeyin/AnaRovBeyin.ino",
            "#     MOTOR MIN_US/MAX_US ve motor başına nötr/sınır değerleri.",
            "#   include/MotorMixer.hpp ve include/RovSettings.hpp",
            "#     Ana uygulamanın motor karışımı/nötr varsayılanları.",
            "#   SuAltiEsp32AnaRov/AnaRovBeyin/AnaRovBeyin.ino",
            "#     torpedoArmLowUs, torpedoNeutralUs, torpedoFireUs varsayılanları.",
            "# Motor firmware sınırları: 1295..1600 us. Torpido darbeleri 1000..2000 us.",
            "#",
            "# --- Motorlar ---",
        ]
        for index, item in enumerate(self.calibration["motors"], start=1):
            lines.extend((
                f"motor{index}_min={item['min']}",
                f"motor{index}_neutral={item['neutral']}",
                f"motor{index}_max={item['max']}",
            ))
        lines.extend(("", "# --- Torpidolar ---"))
        for index, item in enumerate(self.calibration["torpedoes"], start=1):
            lines.extend((
                f"torpedo{index}_min={item['min']}",
                f"torpedo{index}_neutral={item['neutral']}",
                f"torpedo{index}_max={item['max']}",
            ))
        try:
            CALIBRATION_FILE.write_text("\n".join(lines) + "\n", encoding="utf-8")
        except OSError as exc:
            messagebox.showerror("Dosya kaydedilemedi", str(exc), parent=self.root)

    def _on_close(self) -> None:
        if self.armed and self.sock is not None:
            self._send("DISARM")
        self.closing = True
        self._disconnect()
        self.root.destroy()


if __name__ == "__main__":
    app_root = tk.Tk()
    TestApp(app_root)
    app_root.mainloop()
