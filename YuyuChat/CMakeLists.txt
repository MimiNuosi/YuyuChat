cmake_minimum_required(VERSION 3.16)

project(YuyuChat VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools Network)

set(TS_FILES YuyuChat_zh_CN.ts)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        ${TS_FILES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(YuyuChat
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        logindialog.h logindialog.cpp logindialog.ui
        rc.qrc
        registerdialog.h registerdialog.cpp registerdialog.ui
        global.h
        global.cpp
        singleton.h
        httpmanager.h httpmanager.cpp
        config.ini
        timerbutton.h timerbutton.cpp
        resetdialog.h resetdialog.cpp
        resetdialog.ui
        tcpmanager.h tcpmanager.cpp
        usermanager.h usermanager.cpp
        chatdialog.h chatdialog.cpp chatdialog.ui
        customizeedit.h customizeedit.cpp
        chatuserlist.h chatuserlist.cpp
        chatuserwid.h chatuserwid.cpp chatuserwid.ui
        listitembase.h listitembase.cpp
        loadingdialog.h loadingdialog.cpp loadingdialog.ui
        chatpage.h chatpage.cpp chatpage.ui
        chatview.h chatview.cpp
        chatitembase.h chatitembase.cpp
        bubbleframe.h bubbleframe.cpp
        textbubble.h textbubble.cpp
        picturebubble.h picturebubble.cpp
        messagetextedit.h messagetextedit.cpp
        statewidget.h statewidget.cpp
        searchlist.h searchlist.cpp
        adduseritem.h adduseritem.cpp adduseritem.ui
        findsuccessdialog.h findsuccessdialog.cpp findsuccessdialog.ui
        applyfriend.h applyfriend.cpp applyfriend.ui
        contactuserlist.h contactuserlist.cpp
        conuseritem.h conuseritem.cpp conuseritem.ui
        grouptipitem.h grouptipitem.cpp grouptipitem.ui
        userdata.h userdata.cpp
        applyfriendpage.h applyfriendpage.cpp applyfriendpage.ui
        applyfrienditem.h applyfrienditem.cpp applyfrienditem.ui
        applyfriendlist.h applyfriendlist.cpp


    )
target_include_directories(YuyuChat PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET YuyuChat APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(YuyuChat SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(YuyuChat
            ${PROJECT_SOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_link_libraries(YuyuChat PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network)

# ==============================================================================
# 将 config.ini 拷贝到可执行文件所在目录 (相当于 QMAKE_POST_LINK 拷贝功能)
# ==============================================================================
if(WIN32)
    add_custom_command(TARGET YuyuChat POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/config.ini"
        $<TARGET_FILE_DIR:YuyuChat>
        COMMENT "Copying config.ini to output directory..."
    )
endif()

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.YuyuChat)
endif()
set_target_properties(YuyuChat PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS YuyuChat
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(YuyuChat)
endif()
