cmake_minimum_required(VERSION 3.16)

project(LineDogParkour LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

set(SOURCES
    main.cpp
    MainWindow.cpp
    DogItem.cpp
    Platform.cpp
    Coin.cpp
    Obstacle.cpp
)

set(HEADERS
    Config.h
    Entity.h
    MainWindow.h
    DogItem.h
    Platform.h
    Coin.h
    Obstacle.h
)

qt_add_executable(LineDogParkour
    ${SOURCES}
    ${HEADERS}
)

qt_add_resources(LineDogParkour_resources resources.qrc)
target_sources(LineDogParkour PRIVATE ${LineDogParkour_resources})

target_link_libraries(LineDogParkour PRIVATE Qt6::Widgets)


