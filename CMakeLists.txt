cmake_minimum_required(VERSION 3.19)
project(myWindowsApp LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(myWindowsApp
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    Helpers/buttonhelpers.h
    Helpers/buttonhelpers.cpp
    Models/studybutton.h
    Data/studybuttonrepository.h
    Data/studybuttonrepository.cpp

)

target_link_libraries(myWindowsApp
    PRIVATE
        Qt::Core
        Qt::Widgets
)

install(TARGETS myWindowsApp
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET myWindowsApp
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
