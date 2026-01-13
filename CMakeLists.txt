cmake_minimum_required(VERSION 3.16)
project(VoicePhone LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia Network)
find_package(PkgConfig REQUIRED)
pkg_check_modules(OPUS REQUIRED opus)

# 服务器可执行文件
qt_add_executable(voicephone-server
  server/main.cpp
  server/server.cpp
  server/server.h
)

target_link_libraries(voicephone-server PRIVATE 
  Qt6::Core 
  Qt6::Network
)

# 客户端可执行文件
qt_add_executable(voicephone
  src/main.cpp
  src/mainwindow.cpp
  src/mainwindow.h
  src/mainwindow.ui
  src/audioengine.cpp
  src/audioengine.h
  src/opuscodec.cpp
  src/opuscodec.h
  client/networkclient.cpp
  client/networkclient.h
)

target_link_libraries(voicephone PRIVATE 
  Qt6::Widgets 
  Qt6::Multimedia 
  Qt6::Network
  ${OPUS_LIBRARIES}
)

target_include_directories(voicephone PRIVATE
  ${OPUS_INCLUDE_DIRS}
)

# 设置输出目录
set_target_properties(voicephone voicephone-server PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

