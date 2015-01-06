add_definitions(-DTRANSLATION_DOMAIN=\"kglobalaccel5\")
remove_definitions(-DQT_NO_CAST_FROM_BYTEARRAY)
remove_definitions(-DQT_NO_CAST_FROM_ASCII)

###############################################################################
### KDED Global Accel Daemon

set(kglobalaccel_SRCS
    main.cpp
    kglobalacceld.cpp
    component.cpp
    globalshortcut.cpp
    globalshortcutsregistry.cpp
    globalshortcutcontext.cpp)

if (${XCB_XCB_FOUND} AND ${XCB_KEYSYMS_FOUND})
  set( kglobalaccel_SRCS ${kglobalaccel_SRCS} kglobalaccel_x11.cpp )
endif()
if ( Q_WS_MAC )
  set( kglobalaccel_SRCS ${kglobalaccel_SRCS} kglobalaccel_mac.cpp )
endif ()
if ( Q_WS_WIN )
  set( kglobalaccel_SRCS ${kglobalaccel_SRCS} kglobalaccel_win.cpp )
endif ()

configure_file(config-kglobalaccel.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kglobalaccel.h )

add_executable(kglobalaccel5 ${kglobalaccel_SRCS} )
target_link_libraries(kglobalaccel5
   Qt5::DBus
   KF5::GlobalAccel
   KF5::I18n
   KF5::WindowSystem # KKeyServer
   KF5::DBusAddons   # KDBusService
   KF5::CoreAddons   # KAboutData
   KF5::ConfigCore
   KF5::Crash)

if (${XCB_XCB_FOUND} AND ${XCB_KEYSYMS_FOUND})
    target_link_libraries(kglobalaccel5 Qt5::X11Extras XCB::XCB XCB::KEYSYMS)
endif()

if(Q_WS_MAC)
   target_link_libraries(kglobalaccel5 ${CARBON_LIBRARY})
endif()

# Install application and configuration
install( TARGETS kglobalaccel5 ${KF5_INSTALL_TARGETS_DEFAULT_ARGS} )
install( FILES kglobalaccel.desktop  DESTINATION ${SERVICES_INSTALL_DIR} RENAME kglobalaccel5.desktop)

configure_file(org.kde.kglobalaccel.service.in
               ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kglobalaccel.service)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kglobalaccel.service
        DESTINATION ${DBUS_SERVICES_INSTALL_DIR})
