find_package(Qt5Qml REQUIRED)

add_executable(kglobalacceltest kglobalacceltest.cpp)
target_link_libraries(kglobalacceltest KF5::GlobalAccel Qt${QT_MAJOR_VERSION}::Qml Qt${QT_MAJOR_VERSION}::Test)
