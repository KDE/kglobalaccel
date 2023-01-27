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

add_library(KF6GlobalAccelPrivate ${kglobalaccelprivate_SRCS})
add_library(KF6::GlobalAccelPrivate ALIAS KF6GlobalAccelPrivate)
ecm_generate_export_header(KF6GlobalAccelPrivate
    EXPORT_FILE_NAME kf6globalaccelprivate_export.h
    BASE_NAME KGlobalAccelPrivate
    GROUP_BASE_NAME KF
    VERSION ${KF_VERSION}
    DEPRECATED_BASE_VERSION 0
    DEPRECATION_VERSIONS 4.3 5.90
    EXCLUDE_DEPRECATED_BEFORE_AND_AT ${EXCLUDE_DEPRECATED_BEFORE_AND_AT}
)

target_include_directories(KF6GlobalAccelPrivate INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR_KF}/KGlobalAccel>")

target_link_libraries(KF6GlobalAccelPrivate
    PUBLIC
        Qt6::DBus
        KF6::GlobalAccel
        KF6::WindowSystem # KKeyServer
        KF6::CoreAddons   # KAboutData
        KF6::ConfigCore
)

set_target_properties(KF6GlobalAccelPrivate PROPERTIES VERSION ${KGLOBALACCEL_VERSION}
                                                SOVERSION ${KGLOBALACCEL_SOVERSION}
                                                EXPORT_NAME "GlobalAccelPrivate"
)

if(XCB_XCB_FOUND)
    target_link_libraries(KF6GlobalAccelPrivate PUBLIC Qt6::GuiPrivate) # qtx11extras_p.h
endif()

add_executable(kglobalaccel5 main.cpp logging.cpp)
ecm_mark_nongui_executable(kglobalaccel5)

target_include_directories(kglobalaccel5 PRIVATE ${CMAKE_BINARY_DIR})

target_link_libraries(kglobalaccel5
    KF6GlobalAccelPrivate
    KF6::DBusAddons   # KDBusService
    KF6::Crash
    )

add_subdirectory(plugins)

# Install application and configuration
install(TARGETS KF6GlobalAccelPrivate EXPORT KF6GlobalAccelTargets ${KF_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
install( TARGETS kglobalaccel5 ${KF_INSTALL_TARGETS_DEFAULT_ARGS} )

if(NOT EXCLUDE_DEPRECATED_BEFORE_AND_AT STREQUAL "CURRENT" AND EXCLUDE_DEPRECATED_BEFORE_AND_AT VERSION_LESS 5.102.0)
    install( FILES kglobalaccel.desktop  DESTINATION ${KDE_INSTALL_KSERVICESDIR} RENAME kglobalaccel5.desktop)
endif()

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/kf6globalaccelprivate_export.h
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
