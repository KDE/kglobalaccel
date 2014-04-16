configure_file(config-kglobalaccel.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kglobalaccel.h )

ecm_create_qm_loader(kglobalaccel_QM_LOADER kglobalaccel5)

set(kglobalaccel_SRCS
  kglobalaccel.cpp
  kglobalshortcutinfo.cpp
  kglobalshortcutinfo_dbus.cpp
  ${kglobalaccel_QM_LOADER}
)

set(kglobalaccel_xml ${CMAKE_CURRENT_SOURCE_DIR}/org.kde.KGlobalAccel.xml)
set_source_files_properties(${kglobalaccel_xml} PROPERTIES INCLUDE "kglobalshortcutinfo_p.h")
qt5_add_dbus_interface(kglobalaccel_SRCS ${kglobalaccel_xml} kglobalaccel_interface )
install(FILES ${kglobalaccel_xml} DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} RENAME kf5_org.kde.KGlobalAccel.xml)

set(kglobalaccel_component_xml ${CMAKE_CURRENT_SOURCE_DIR}/org.kde.kglobalaccel.Component.xml)
set_source_files_properties(${kglobalaccel_component_xml} PROPERTIES INCLUDE "kglobalshortcutinfo_p.h")
qt5_add_dbus_interface(kglobalaccel_SRCS ${kglobalaccel_component_xml} kglobalaccel_component_interface )
install(FILES ${kglobalaccel_component_xml} DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} RENAME kf5_org.kde.kglobalaccel.Component.xml)

add_library(KF5GlobalAccel ${kglobalaccel_SRCS})
generate_export_header(KF5GlobalAccel BASE_NAME KGlobalAccel)
add_library(KF5::GlobalAccel ALIAS KF5GlobalAccel)

target_include_directories(KF5GlobalAccel INTERFACE "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}/KGlobalAccel>")

target_link_libraries(KF5GlobalAccel
PUBLIC
    Qt5::DBus
    Qt5::Widgets
)

if(HAVE_X11)
    target_link_libraries(KF5GlobalAccel
    PRIVATE
        Qt5::X11Extras
    )
endif()

set_target_properties(KF5GlobalAccel PROPERTIES VERSION   ${KGLOBALACCEL_VERSION_STRING}
                                                SOVERSION ${KGLOBALACCEL_SOVERSION}
                                                EXPORT_NAME "GlobalAccel"
)

ecm_generate_headers(KGlobalAccel_HEADERS
  HEADER_NAMES
  KGlobalAccel
  KGlobalShortcutInfo

  REQUIRED_HEADERS KGlobalAccel_HEADERS
)

install(TARGETS KF5GlobalAccel EXPORT KF5GlobalAccelTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/kglobalaccel_export.h
  ${KGlobalAccel_HEADERS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/KGlobalAccel COMPONENT Devel
)

include(ECMGeneratePriFile)
ecm_generate_pri_file(BASE_NAME KGlobalAccel LIB_NAME KF5GlobalAccel DEPS "dbus widgets" FILENAME_VAR PRI_FILENAME)
install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})
