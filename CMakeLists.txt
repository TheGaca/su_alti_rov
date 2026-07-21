cmake_minimum_required(VERSION 3.10)
project(su_alti_rov)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Otomatik MOC, UIC, RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Paketleri bul
find_package(Qt5 COMPONENTS Core Gui Widgets Network SerialPort WebSockets REQUIRED)
find_package(OpenCV REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${OpenCV_INCLUDE_DIRS})

# Kaynak dosyalar - her sinif kendi basligi/dosyasinda (bkz. include/ ve src/)
add_executable(su_alti_rov
    src/main.cpp
    src/JoystickThread.cpp
    src/CameraThread.cpp
    src/AnaRovThread.cpp
    src/EspRovThread.cpp
    src/ColorPickerButton.cpp
    src/AttitudeIndicator.cpp
    src/MotorDiagramWidget.cpp
    src/PixhawkPanel.cpp
    src/Ui_MainWindow.cpp
    src/PixhawkGUI.cpp

    include/JoystickThread.hpp
    include/CameraThread.hpp
    include/AnaRovThread.hpp
    include/EspRovThread.hpp
    include/ColorPickerButton.hpp
    include/AttitudeIndicator.hpp
    include/MotorDiagramWidget.hpp
    include/PixhawkPanel.hpp
    include/Ui_MainWindow.hpp
    include/PixhawkGUI.hpp
)

target_link_libraries(su_alti_rov
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
    Qt5::SerialPort
    Qt5::WebSockets
    ${OpenCV_LIBS}
)
