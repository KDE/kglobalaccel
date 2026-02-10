cmake_minimum_required(VERSION 3.27)

set(KF_VERSION "6.24.0") # handled by release scripts
set(KF_DEP_VERSION "6.23.0") # handled by release scripts
project(KGlobalAccel VERSION ${KF_VERSION})

# ECM setup
include(FeatureSummary)
find_package(ECM 6.23.0  NO_MODULE)
set_package_properties(ECM PROPERTIES TYPE REQUIRED DESCRIPTION "Extra CMake Modules." URL "https://commits.kde.org/extra-cmake-modules")
feature_summary(WHAT REQUIRED_PACKAGES_NOT_FOUND FATAL_ON_MISSING_REQUIRED_PACKAGES)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEGitCommitHooks)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)

include(ECMGenerateExportHeader)
include(CMakePackageConfigHelpers)
include(ECMGenerateQDoc)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(ECMMarkNonGuiExecutable)
include(ECMQtDeclareLoggingCategory)
include(ECMPoQmTools)
include(ECMDeprecationSettings)

set(EXCLUDE_DEPRECATED_BEFORE_AND_AT 0 CACHE STRING "Control the range of deprecated API excluded from the build [default=0].")

set(kglobalaccel_version_header "${CMAKE_CURRENT_BINARY_DIR}/src/kglobalaccel_version.h")
ecm_setup_version(PROJECT VARIABLE_PREFIX KGLOBALACCEL
                        VERSION_HEADER "${kglobalaccel_version_header}"
                        PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KF6GlobalAccelConfigVersion.cmake"
                        SOVERSION 6)

# Dependencies
set(REQUIRED_QT_VERSION 6.8.0)
find_package(Qt6 ${REQUIRED_QT_VERSION} CONFIG REQUIRED DBus Gui Widgets)

if (Qt6Gui_VERSION VERSION_GREATER_EQUAL "6.10.0")
    find_package(Qt6GuiPrivate ${REQUIRED_QT_VERSION} REQUIRED NO_MODULE)
endif()

option(WITH_X11 "Build with X11 support" ON)

ecm_set_disabled_deprecation_versions(
    QT 6.11.0
    KF 6.23.0
)

# Subdirectories
ecm_install_po_files_as_qm(poqm)

add_subdirectory(src)
if(BUILD_TESTING)
    find_package(Qt6 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Test)

    add_subdirectory(tests)
endif()

# create a Config.cmake and a ConfigVersion.cmake file and install them
set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KF6GlobalAccel")

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/KF6GlobalAccelConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/KF6GlobalAccelConfig.cmake"
    PATH_VARS KDE_INSTALL_DBUSINTERFACEDIR
    INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR}
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/KF6GlobalAccelConfig.cmake"
                "${CMAKE_CURRENT_BINARY_DIR}/KF6GlobalAccelConfigVersion.cmake"
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel)

install(EXPORT KF6GlobalAccelTargets DESTINATION "${CMAKECONFIG_INSTALL_DIR}" FILE KF6GlobalAccelTargets.cmake NAMESPACE KF6::)


install(FILES ${kglobalaccel_version_header}
        DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF}/KGlobalAccel COMPONENT Devel)

include(ECMFeatureSummary)
ecm_feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
