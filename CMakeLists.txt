cmake_minimum_required(VERSION 2.8.12)

project(KGlobalAccel)

# ECM setup
find_package(ECM 1.2.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(FeatureSummary)
include(GenerateExportHeader)
include(ECMPackageConfigHelpers)
include(ECMSetupVersion)
include(ECMGenerateHeaders)

include(ECMPoQmTools)

set(KF5_VERSION "5.3.0") # handled by release scripts

ecm_setup_version(${KF5_VERSION} VARIABLE_PREFIX KGLOBALACCEL
                        VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/kglobalaccel_version.h"
                        PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KF5GlobalAccelConfigVersion.cmake"
                        SOVERSION 5)

# Dependencies
set(REQUIRED_QT_VERSION 5.2.0)
find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED DBus Widgets)

include(KDEInstallDirs)
include(KDEFrameworkCompilerSettings)
include(KDECMakeSettings)

if (NOT APPLE)
    find_package(X11)
endif()
set(HAVE_X11 ${X11_FOUND})
if(HAVE_X11)
    find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED X11Extras)
endif()

# Subdirectories
if (IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/po")
    ecm_install_po_files_as_qm(po)
endif()

add_subdirectory(src)
add_subdirectory(autotests)

# create a Config.cmake and a ConfigVersion.cmake file and install them
set(CMAKECONFIG_INSTALL_DIR "${CMAKECONFIG_INSTALL_PREFIX}/KF5GlobalAccel")

ecm_configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/KF5GlobalAccelConfig.cmake.in"
                              "${CMAKE_CURRENT_BINARY_DIR}/KF5GlobalAccelConfig.cmake"
                              INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR}
                              )

install(FILES  "${CMAKE_CURRENT_BINARY_DIR}/KF5GlobalAccelConfig.cmake"
               "${CMAKE_CURRENT_BINARY_DIR}/KF5GlobalAccelConfigVersion.cmake"
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel )

install(EXPORT KF5GlobalAccelTargets DESTINATION "${CMAKECONFIG_INSTALL_DIR}" FILE KF5GlobalAccelTargets.cmake NAMESPACE KF5:: )


install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kglobalaccel_version.h
        DESTINATION ${KF5_INCLUDE_INSTALL_DIR} COMPONENT Devel )

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
