###############################################################################
### KDED Global Accel Daemon

set(kglobalaccelprivate_SRCS
    kglobalacceld.cpp
    kglobalaccel_interface.cpp
    kserviceactioncomponent.cpp
    component.cpp
    logging.cpp
    globalshortcut.cpp
    globalshortcutsregistry.cpp
    globalshortcutcontext.cpp
    ../sequencehelpers_p.cpp
)
ecm_create_qm_loader(kglobalaccelprivate_SRCS kglobalaccel5_qt)

configure_file(config-kglobalaccel.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kglobalaccel.h )

add_library(KF5GlobalAccelPrivate ${kglobalaccelprivate_SRCS})
add_library(KF5::GlobalAccelPrivate ALIAS KF5GlobalAccelPrivate)
ecm_generate_export_header(KF5GlobalAccelPrivate
    EXPORT_FILE_NAME kf5globalaccelprivate_export.h
    BASE_NAME KGlobalAccelPrivate
    GROUP_BASE_NAME KF
    VERSION ${KF_VERSION}
    DEPRECATED_BASE_VERSION 0
    DEPRECATION_VERSIONS 4.3 5.90
    EXCLUDE_DEPRECATED_BEFORE_AND_AT ${EXCLUDE_DEPRECATED_BEFORE_AND_AT}
)

target_include_directories(KF5GlobalAccelPrivate INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR_KF}/KGlobalAccel>")

target_link_libraries(KF5GlobalAccelPrivate
    PUBLIC
        Qt${QT_MAJOR_VERSION}::DBus
        KF5::GlobalAccel
        KF5::WindowSystem # KKeyServer
        KF5::CoreAddons   # KAboutData
        KF5::ConfigCore
)

set_target_properties(KF5GlobalAccelPrivate PROPERTIES VERSION ${KGLOBALACCEL_VERSION}
                                                SOVERSION ${KGLOBALACCEL_SOVERSION}
                                                EXPORT_NAME "GlobalAccelPrivate"
)

if(XCB_XCB_FOUND)
    if (TARGET Qt5::X11Extras)
        target_link_libraries(KF5GlobalAccelPrivate PUBLIC Qt5::X11Extras)
    elseif (TARGET Qt6::Gui)
        target_link_libraries(KF5GlobalAccelPrivate PUBLIC Qt6::GuiPrivate) # qtx11extras_p.h
    endif()
endif()

add_executable(kglobalaccel5 main.cpp logging.cpp)
ecm_mark_nongui_executable(kglobalaccel5)

target_include_directories(kglobalaccel5 PRIVATE ${CMAKE_BINARY_DIR})

target_link_libraries(kglobalaccel5
    KF5GlobalAccelPrivate
    KF5::DBusAddons   # KDBusService
    KF5::Crash
    )

add_subdirectory(plugins)

# Install application and configuration
install(TARGETS KF5GlobalAccelPrivate EXPORT KF5GlobalAccelTargets ${KF_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
install( TARGETS kglobalaccel5 ${KF_INSTALL_TARGETS_DEFAULT_ARGS} )

if(NOT EXCLUDE_DEPRECATED_BEFORE_AND_AT STREQUAL "CURRENT" AND EXCLUDE_DEPRECATED_BEFORE_AND_AT VERSION_LESS 5.102.0)
    install( FILES kglobalaccel.desktop  DESTINATION ${KDE_INSTALL_KSERVICESDIR} RENAME kglobalaccel5.desktop)
endif()

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/kf5globalaccelprivate_export.h
  kglobalacceld.h
  kglobalaccel_interface.h
  DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF}/KGlobalAccel/private COMPONENT Devel
)

# D-Bus activation service file, and associated systemd service
include (ECMGenerateDBusServiceFile)
include (ECMConfiguredInstall)
ecm_generate_dbus_service_file(
   NAME org.kde.kglobalaccel
   EXECUTABLE "${KDE_INSTALL_FULL_BINDIR}/kglobalaccel5"
   SYSTEMD_SERVICE plasma-kglobalaccel.service
   DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR}
)

ecm_install_configured_files(
   INPUT plasma-kglobalaccel.service.in
   DESTINATION ${KDE_INSTALL_SYSTEMDUSERUNITDIR}
)
