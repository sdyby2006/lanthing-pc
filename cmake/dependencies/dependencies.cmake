add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/tomlplusplus)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/utfcpp)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/amf)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/prebuilt/ffmpeg/${LT_PLAT})
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/breakpad_builder)
#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/lodepng)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/prebuilt/sqlite/${LT_PLAT})

set(protobuf_MODULE_COMPATIBLE ON)
#protobuf����absl
find_package(absl REQUIRED PATHS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/prebuilt/protobuf/${LT_THIRD_POSTFIX}/lib/cmake)
find_package(utf8_range REQUIRED PATHS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/prebuilt/protobuf/${LT_THIRD_POSTFIX}/lib/cmake)
find_package(Protobuf REQUIRED PATHS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/prebuilt/protobuf/${LT_THIRD_POSTFIX})
find_package(GTest REQUIRED PATHS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/prebuilt/googletest/${LT_THIRD_POSTFIX})
find_package(g3log REQUIRED PATHS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/prebuilt/g3log/${LT_THIRD_POSTFIX})
find_package(MbedTLS REQUIRED PATHS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/prebuilt/mbedtls/${LT_THIRD_POSTFIX})
find_package(libuv REQUIRED PATHS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/prebuilt/libuv/${LT_THIRD_POSTFIX})
find_package(SDL2 REQUIRED PATHS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/prebuilt/sdl/${LT_THIRD_POSTFIX})
find_package(Opus REQUIRED PATHS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/prebuilt/opus/${LT_THIRD_POSTFIX})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui_builder)

add_subdirectory(ltproto)
# TODO: ��ltlib�ϲ���lanthing
add_subdirectory(ltlib)
add_subdirectory(transport)

#Qt
list(APPEND CMAKE_PREFIX_PATH ${LT_QT_CMAKE_PATH})
set(CMAKE_AUTORCC ON)
find_package(Qt6 REQUIRED COMPONENTS Widgets Gui LinguistTools)
qt_standard_project_setup()
get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${_qt_bin_dir}")

if(LT_WINDOWS)
    include(${CMAKE_CURRENT_LIST_DIR}/windows.cmake)
elseif (LT_LINUX)
    include(${CMAKE_CURRENT_LIST_DIR}/linux.cmake)
elseif(LT_MAC)
    include(${CMAKE_CURRENT_LIST_DIR}/mac.cmake)
endif()