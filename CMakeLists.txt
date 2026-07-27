cmake_minimum_required(VERSION 3.10)
project(su_alti_rov)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Otomatik MOC, UIC, RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Paketleri bul
find_package(Qt5 COMPONENTS Core Gui Widgets Network SerialPort REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Kaynak dosyalar - her sinif kendi basligi/dosyasinda (bkz. include/ ve src/)
add_executable(su_alti_rov
    src/main.cpp
    src/JoystickThread.cpp
    src/CameraThread.cpp
    src/EspRovThread.cpp
    src/ColorPickerButton.cpp
    src/AttitudeIndicator.cpp
    src/MotorDiagramWidget.cpp
    src/MotorMixer.cpp
    src/RovSettings.cpp
    src/SettingsDialog.cpp
    src/AviMjpegWriter.cpp
    src/RovPanel.cpp
    src/Ui_MainWindow.cpp
    src/RovGUI.cpp

    include/JoystickThread.hpp
    include/CameraThread.hpp
    include/EspRovThread.hpp
    include/ColorPickerButton.hpp
    include/AttitudeIndicator.hpp
    include/MotorDiagramWidget.hpp
    include/MotorMixer.hpp
    include/RovSettings.hpp
    include/SettingsDialog.hpp
    include/AviMjpegWriter.hpp
    include/RovPanel.hpp
    include/Ui_MainWindow.hpp
    include/RovGUI.hpp
)

target_link_libraries(su_alti_rov
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
    Qt5::SerialPort
)

# ---- Birim testler (ctest ile calisir) ----
# MotorMixer bilerek Qt'siz yazildi; testi de Qt gerektirmez.
enable_testing()
add_executable(test_motor_mixer
    tests/test_motor_mixer.cpp
    src/MotorMixer.cpp
)
add_test(NAME motor_mixer COMMAND test_motor_mixer)
