set(xcb_plugin_SRCS
    kglobalaccel_x11.cpp
    kglobalaccel_x11.h
    ../../logging.cpp
)

add_library(KF6GlobalAccelPrivateXcb MODULE ${xcb_plugin_SRCS})
target_link_libraries(KF6GlobalAccelPrivateXcb
    KF6GlobalAccelPrivate
    XCB::XCB
    XCB::KEYSYMS
    XCB::XKB
    XCB::RECORD
)

install(
    TARGETS
        KF6GlobalAccelPrivateXcb
    DESTINATION
        ${KDE_INSTALL_PLUGINDIR}/org.kde.kglobalaccel5.platforms/
)
