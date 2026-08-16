add_library(kglobalaccelwidgetsprompt MODULE kglobalaccelwidgetsprompt.cpp)

target_link_libraries(kglobalaccelwidgetsprompt
PRIVATE
    KF6GlobalAccel
    Qt6::Widgets
)

target_include_directories(kglobalaccelwidgetsprompt PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

install(TARGETS kglobalaccelwidgetsprompt DESTINATION ${KDE_INSTALL_PLUGINDIR}/kglobalaccel)
