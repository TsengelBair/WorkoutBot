cmake_minimum_required(VERSION 3.14)

project(workoutBot LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Sql REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Sql REQUIRED)

find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${CURL_INCLUDE_DIRS})

include_directories(/usr/include/qwt)


add_executable(workoutBot
  main.cpp
  bot/workoutbot.h
  bot/workoutbot.cpp
  parser/parser.h
  parser/parser.cpp
  db/dbhandler.h
  db/dbhandler.cpp
  chart/chart.h
  chart/chart.cpp
  config.h
)
target_link_libraries(workoutBot Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Sql qwt-qt5 TgBot ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${CURL_LIBRARIES} pthread stdc++fs)
