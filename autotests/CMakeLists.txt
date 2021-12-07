find_package(Qt${QT_MAJOR_VERSION} ${REQUIRED_QT_VERSION} CONFIG REQUIRED Test)

remove_definitions(-DQT_NO_CAST_FROM_BYTEARRAY)
remove_definitions(-DQT_NO_CAST_FROM_ASCII)
remove_definitions(-DQT_NO_CAST_TO_ASCII)

set(additionalLibraries )
if(HAVE_X11 AND XCB_XTEST_FOUND)
    add_definitions(-DHAVE_XCB_XTEST)
    set(additionalLibraries XCB::XTEST XCB::KEYSYMS)
    if (TARGET Qt5::X11Extras)
        list(APPEND additionalLibraries Qt5::X11Extras)
    elseif (TARGET Qt6::Gui)
        list(APPEND additionalLibraries Qt6::GuiPrivate) # qtx11extras_p.h
    endif()

else()
    set(additionalLibraries )
endif()

include(ECMAddTests)

ecm_add_tests(
   kglobalshortcuttest.cpp
   LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test KF5::GlobalAccel ${additionalLibraries}
)
