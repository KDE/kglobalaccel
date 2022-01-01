cmake_minimum_required(VERSION 3.16)

set(KF_VERSION "5.90.0") # handled by release scripts
set(KF_DEP_VERSION "5.90.0") # handled by release scripts
project(KGlobalAccel VERSION ${KF_VERSION})

# ECM setup
include(FeatureSummary)
find_package(ECM 5.90.0  NO_MODULE)
set_package_properties(ECM PROPERTIES TYPE REQUIRED DESCRIPTION "Extra CMake Modules." URL "https://commits.kde.org/extra-cmake-modules")
feature_summary(WHAT REQUIRED_PACKAGES_NOT_FOUND FATAL_ON_MISSING_REQUIRED_PACKAGES)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEGitCommitHooks)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)

include(ECMGenerateExportHeader)
include(CMakePackageConfigHelpers)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(ECMAddQch)
include(ECMMarkNonGuiExecutable)
include(ECMQtDeclareLoggingCategory)
include(ECMPoQmTools)

set(EXCLUDE_DEPRECATED_BEFORE_AND_AT 0 CACHE STRING "Control the range of deprecated API excluded from the build [default=0].")

option(BUILD_QCH "Build API documentation in QCH format (for e.g. Qt Assistant, Qt Creator & KDevelop)" OFF)
add_feature_info(QCH ${BUILD_QCH} "API documentation in QCH format (for e.g. Qt Assistant, Qt Creator & KDevelop)")

ecm_setup_version(PROJECT VARIABLE_PREFIX KGLOBALACCEL
                        VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/kglobalaccel_version.h"
                        PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KF5GlobalAccelConfigVersion.cmake"
                        SOVERSION 5)

# Dependencies
set(REQUIRED_QT_VERSION 5.15.2)
find_package(Qt${QT_MAJOR_VERSION} ${REQUIRED_QT_VERSION} CONFIG REQUIRED DBus Widgets)

# Dependencies of runtime component
find_package(KF5Config ${KF_DEP_VERSION} REQUIRED)
find_package(KF5CoreAddons ${KF_DEP_VERSION} REQUIRED)
find_package(KF5Crash ${KF_DEP_VERSION} REQUIRED)
find_package(KF5DBusAddons ${KF_DEP_VERSION} REQUIRED)
find_package(KF5WindowSystem ${KF_DEP_VERSION} REQUIRED)

# no X11 stuff on mac
if (NOT APPLE)
    find_package(XCB MODULE COMPONENTS XCB KEYSYMS XKB OPTIONAL_COMPONENTS XTEST)
    set_package_properties(XCB PROPERTIES DESCRIPTION "X protocol C-language Binding"
                       URL "http://xcb.freedesktop.org"
                       TYPE OPTIONAL
                      )

    find_package(X11)
endif()

set(HAVE_X11 0)

if(X11_FOUND AND XCB_XCB_FOUND)
    set(HAVE_X11 1)
    if (QT_MAJOR_VERSION STREQUAL "5")
        find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED X11Extras)
    elseif (QT_MAJOR_VERSION STREQUAL "6")
        find_package(Qt${QT_MAJOR_VERSION} ${REQUIRED_QT_VERSION} CONFIG REQUIRED Gui) # qtx11extras_p.h
    endif()
endif()
add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x050f02)
add_definitions(-DKF_DISABLE_DEPRECATED_BEFORE_AND_AT=0x055100)
# Subdirectories
ecm_install_po_files_as_qm(po)

add_subdirectory(src)
if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()

# create a Config.cmake and a ConfigVersion.cmake file and install them
set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KF5GlobalAccel")

if (BUILD_QCH)
    ecm_install_qch_export(
        TARGETS KF5GlobalAccel_QCH
        FILE KF5GlobalAccelQchTargets.cmake
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel
    )
    set(PACKAGE_INCLUDE_QCHTARGETS "include(\"\${CMAKE_CURRENT_LIST_DIR}/KF5GlobalAccelQchTargets.cmake\")")
endif()

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/KF5GlobalAccelConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/KF5GlobalAccelConfig.cmake"
    PATH_VARS KDE_INSTALL_DBUSINTERFACEDIR
    INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR}
)

install(FILES  "${CMAKE_CURRENT_BINARY_DIR}/KF5GlobalAccelConfig.cmake"
               "${CMAKE_CURRENT_BINARY_DIR}/KF5GlobalAccelConfigVersion.cmake"
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel )

install(EXPORT KF5GlobalAccelTargets DESTINATION "${CMAKECONFIG_INSTALL_DIR}" FILE KF5GlobalAccelTargets.cmake NAMESPACE KF5:: )


install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kglobalaccel_version.h
        DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF} COMPONENT Devel )

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
