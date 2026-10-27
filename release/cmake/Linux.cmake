# Native Linux build: Direct3D 11 through DXVK-native, XAudio2 through FAudio, window and input through SDL3.
# The Win32 functions the game uses come from platform/linux (include/ shadows the Windows SDK headers).
include(FetchContent)

find_package(SDL3 REQUIRED CONFIG)
find_package(Threads REQUIRED)

FetchContent_Declare(dxvk_native
 URL https://github.com/doitsujin/dxvk/releases/download/v3.1.1/dxvk-native-3.1.1-steamrt-sniper.tar.gz
 URL_HASH SHA256=506fa1ea5bb6ce9a89e11436f7ab5191bc0196e8a577e0cf0b16a45bbc9991b7)
FetchContent_MakeAvailable(dxvk_native)
set(DXVK_DIR "${dxvk_native_SOURCE_DIR}")
foreach(lib d3d11 dxgi)
 add_library(dxvk_${lib} SHARED IMPORTED)
 set_target_properties(dxvk_${lib} PROPERTIES
  IMPORTED_LOCATION "${DXVK_DIR}/lib/libdxvk_${lib}.so.0"
  IMPORTED_NO_SONAME OFF)
endforeach()

set(BUILD_SHARED_LIBS OFF)
set(BUILD_SDL3 ON CACHE BOOL "" FORCE)
FetchContent_Declare(faudio
 URL https://github.com/FNA-XNA/FAudio/archive/refs/tags/26.10.tar.gz
 URL_HASH SHA256=c508f297ec8d065b456b2d9d74bab89ffc84ac3afbcb428bd54e40f09f8b901e)
FetchContent_MakeAvailable(faudio)

list(REMOVE_ITEM GAME_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
file(GLOB LINUX_SOURCES CONFIGURE_DEPENDS platform/linux/*.cpp platform/linux/*.c)
add_executable(cubeworld ${GAME_SOURCES} ${LINUX_SOURCES} platform/FreeImageRuntime.cpp platform/SteamOffline.cpp)
target_include_directories(cubeworld BEFORE PRIVATE platform/linux/include)
target_include_directories(cubeworld PRIVATE include plasma/include thirdparty/steamworks "${DXVK_DIR}/include/dxvk")
target_compile_definitions(cubeworld PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN DIRECTINPUT_VERSION=0x0800
 STEAM_API_NODLL CUBE_OFFLINE CUBE_LINUX)
# MSVC builds saw windows.h through precompiled and transitive includes; every source gets it here.
target_compile_options(cubeworld PRIVATE -fpermissive -Wno-multichar -fno-strict-aliasing
 $<$<COMPILE_LANGUAGE:CXX>:-include;windows.h>)
target_include_directories(cubeworld PRIVATE "${faudio_SOURCE_DIR}/include")
target_link_libraries(cubeworld PRIVATE pugixml sqlite vorbisfile libvorbis libogg zlib freetype FreeImage
 dxvk_d3d11 dxvk_dxgi FAudio-static SDL3::SDL3 GLU Threads::Threads ${CMAKE_DL_LIBS})
# Finds the DXVK libraries next to the executable once installed, and in the download directory while developing.
set_target_properties(cubeworld PROPERTIES BUILD_RPATH "$ORIGIN;${DXVK_DIR}/lib" INSTALL_RPATH "$ORIGIN")
# This large sequence of sprite registrations otherwise triggers very costly GCC inlining.
set_source_files_properties(src/cube/SpriteManager.cpp PROPERTIES COMPILE_OPTIONS -O1)
# The game's zlib needs unistd.h for its gz* file functions.
target_compile_definitions(zlib PRIVATE HAVE_UNISTD_H)
