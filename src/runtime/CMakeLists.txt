remove_definitions(-DQT_NO_CAST_FROM_BYTEARRAY)
remove_definitions(-DQT_NO_CAST_FROM_ASCII)

ecm_create_qm_loader(kglobalaccel_QM_LOADER kglobalaccel5_qt)

###############################################################################
### KDED Global Accel Daemon

set(kglobalaccel_SRCS
    kglobalacceld.cpp
    kglobalaccel_interface.cpp
    component.cpp
    logging.cpp
    globalshortcut.cpp
    globalshortcutsregistry.cpp
    globalshortcutcontext.cpp
    ${kglobalaccel_QM_LOADER})

configure_file(config-kglobalaccel.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kglobalaccel.h )

add_library(KF5GlobalAccelPrivate ${kglobalaccel_SRCS})
generate_export_header(KF5GlobalAccelPrivate BASE_NAME KF5GlobalAccelPrivate)
target_link_libraries(KF5GlobalAccelPrivate
   Qt5::DBus
   KF5::GlobalAccel
   KF5::WindowSystem # KKeyServer
   KF5::DBusAddons   # KDBusService
   KF5::CoreAddons   # KAboutData
   KF5::ConfigCore
   KF5::Crash)

if (${XCB_XCB_FOUND})
    target_link_libraries(KF5GlobalAccelPrivate Qt5::X11Extras)
endif()

add_executable(kglobalaccel5 main.cpp logging.cpp)
target_link_libraries(kglobalaccel5 KF5GlobalAccelPrivate)

add_subdirectory(plugins)

# Install application and configuration
install(TARGETS KF5GlobalAccelPrivate EXPORT KF5GlobalAccelPrivateTargets ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})
install( TARGETS kglobalaccel5 ${KF5_INSTALL_TARGETS_DEFAULT_ARGS} )
install( FILES kglobalaccel.desktop  DESTINATION ${KDE_INSTALL_KSERVICES5DIR} RENAME kglobalaccel5.desktop)

configure_file(org.kde.kglobalaccel.service.in
               ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kglobalaccel.service)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kglobalaccel.service
        DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR})
