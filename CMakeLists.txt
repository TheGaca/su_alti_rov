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

# MAVLink başlık dosyası yolu
# Proje klasörüne `mavlink/` klasörünü kopyalaman gerekiyor
# (https://github.com/mavlink/c_library_v2 → common.h vs.)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/mavlink)
include_directories(${OpenCV_INCLUDE_DIRS})

# Kaynak dosyalar
add_executable(su_alti_rov
    main.cpp
    yazilim.cpp
    yazilim.h
    tasarim.cpp
    tasarim.h
    algoritma.cpp
    algoritma.h
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