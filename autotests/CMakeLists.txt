find_package(Qt5 5.2.0 CONFIG REQUIRED Test)

remove_definitions(-DQT_NO_CAST_FROM_BYTEARRAY)
remove_definitions(-DQT_NO_CAST_FROM_ASCII)
remove_definitions(-DQT_NO_CAST_TO_ASCII)

include(ECMAddTests)

ecm_add_tests(
   kglobalshortcuttest.cpp
   NAME_PREFIX "kglobalaccel-"
   LINK_LIBRARIES Qt5::Test KF5::GlobalAccel
)
