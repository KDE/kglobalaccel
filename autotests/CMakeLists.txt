remove_definitions(-DQT_NO_CAST_FROM_BYTEARRAY)
remove_definitions(-DQT_NO_CAST_FROM_ASCII)
remove_definitions(-DQT_NO_CAST_TO_ASCII)

set(additionalLibraries )
if(HAVE_X11 AND XCB_XTEST_FOUND)
    add_definitions(-DHAVE_XCB_XTEST)
    set(additionalLibraries XCB::XTEST XCB::KEYSYMS)
    list(APPEND additionalLibraries Qt6::GuiPrivate) # qtx11extras_p.h
endif()

include(ECMAddTests)

if (BUILD_RUNTIME)
    ecm_add_tests(
        kglobalshortcuttest.cpp
        LINK_LIBRARIES Qt6::Test KF6::GlobalAccelPrivate ${additionalLibraries}
    )
endif()
