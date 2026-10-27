cmake_minimum_required(VERSION 3.20)
project(InfiniteCubesRelease LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(CUBE_LINUX ON)
elseif(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "Use a Windows x64 compiler or build natively on Linux x86_64 (see README.md)")
endif()
find_package(Python3 REQUIRED COMPONENTS Interpreter)
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ExpandSQLite.py"
  "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/sqlite/sqlite3.c.xz" "${CMAKE_CURRENT_BINARY_DIR}/sqlite3.c"
  COMMAND_ERROR_IS_FATAL ANY)
add_library(sqlite STATIC
  ${CMAKE_CURRENT_BINARY_DIR}/sqlite3.c
)
target_include_directories(sqlite PUBLIC thirdparty/sqlite/.)
target_include_directories(sqlite PRIVATE thirdparty/libogg/include thirdparty/libvorbis/include thirdparty/libvorbis/lib)
add_library(libogg STATIC
  thirdparty/libogg/src/framing.c
  thirdparty/libogg/src/bitwise.c
)
target_include_directories(libogg PUBLIC thirdparty/libogg/include)
target_include_directories(libogg PRIVATE thirdparty/libogg/include thirdparty/libvorbis/include thirdparty/libvorbis/lib)
add_library(libvorbis STATIC
  thirdparty/libvorbis/lib/analysis.c
  thirdparty/libvorbis/lib/bitrate.c
  thirdparty/libvorbis/lib/block.c
  thirdparty/libvorbis/lib/codebook.c
  thirdparty/libvorbis/lib/envelope.c
  thirdparty/libvorbis/lib/floor0.c
  thirdparty/libvorbis/lib/floor1.c
  thirdparty/libvorbis/lib/info.c
  thirdparty/libvorbis/lib/lookup.c
  thirdparty/libvorbis/lib/lpc.c
  thirdparty/libvorbis/lib/lsp.c
  thirdparty/libvorbis/lib/mapping0.c
  thirdparty/libvorbis/lib/mdct.c
  thirdparty/libvorbis/lib/psy.c
  thirdparty/libvorbis/lib/registry.c
  thirdparty/libvorbis/lib/res0.c
  thirdparty/libvorbis/lib/sharedbook.c
  thirdparty/libvorbis/lib/smallft.c
  thirdparty/libvorbis/lib/synthesis.c
  thirdparty/libvorbis/lib/vorbisenc.c
  thirdparty/libvorbis/lib/window.c
)
target_include_directories(libvorbis PUBLIC thirdparty/libvorbis/include)
target_include_directories(libvorbis PRIVATE thirdparty/libogg/include thirdparty/libvorbis/include thirdparty/libvorbis/lib)
add_library(vorbisfile STATIC
  thirdparty/libvorbis/lib/vorbisfile.c
)
target_include_directories(vorbisfile PUBLIC thirdparty/libvorbis/include)
target_include_directories(vorbisfile PRIVATE thirdparty/libogg/include thirdparty/libvorbis/include thirdparty/libvorbis/lib)
add_library(zlib STATIC
  thirdparty/zlib/adler32.c
  thirdparty/zlib/compress.c
  thirdparty/zlib/crc32.c
  thirdparty/zlib/deflate.c
  thirdparty/zlib/gzclose.c
  thirdparty/zlib/gzlib.c
  thirdparty/zlib/gzread.c
  thirdparty/zlib/gzwrite.c
  thirdparty/zlib/infback.c
  thirdparty/zlib/contrib/masmx64/inffas8664.c
  thirdparty/zlib/inffast.c
  thirdparty/zlib/inflate.c
  thirdparty/zlib/inftrees.c
  thirdparty/zlib/contrib/minizip/ioapi.c
  thirdparty/zlib/trees.c
  thirdparty/zlib/uncompr.c
  thirdparty/zlib/contrib/minizip/unzip.c
  thirdparty/zlib/contrib/minizip/zip.c
  thirdparty/zlib/zutil.c
)
target_include_directories(zlib PUBLIC thirdparty/zlib/. thirdparty/zlib/contrib/masmx86)
target_include_directories(zlib PRIVATE thirdparty/libogg/include thirdparty/libvorbis/include thirdparty/libvorbis/lib)
add_library(freetype STATIC
  thirdparty/freetype/src/autofit/autofit.c
  thirdparty/freetype/src/bdf/bdf.c
  thirdparty/freetype/src/cff/cff.c
  thirdparty/freetype/src/base/ftbase.c
  thirdparty/freetype/src/base/ftbitmap.c
  thirdparty/freetype/src/cache/ftcache.c
  thirdparty/freetype/builds/win32/ftdebug.c
  thirdparty/freetype/src/base/ftfstype.c
  thirdparty/freetype/src/base/ftgasp.c
  thirdparty/freetype/src/base/ftglyph.c
  thirdparty/freetype/src/gzip/ftgzip.c
  thirdparty/freetype/src/base/ftinit.c
  thirdparty/freetype/src/lzw/ftlzw.c
  thirdparty/freetype/src/base/ftstroke.c
  thirdparty/freetype/src/base/ftsystem.c
  thirdparty/freetype/src/smooth/smooth.c
  thirdparty/freetype/src/base/ftbbox.c
  thirdparty/freetype/src/base/ftmm.c
  thirdparty/freetype/src/base/ftpfr.c
  thirdparty/freetype/src/base/ftsynth.c
  thirdparty/freetype/src/base/fttype1.c
  thirdparty/freetype/src/base/ftwinfnt.c
  thirdparty/freetype/src/base/ftxf86.c
  thirdparty/freetype/src/base/ftlcdfil.c
  thirdparty/freetype/src/base/ftgxval.c
  thirdparty/freetype/src/base/ftotval.c
  thirdparty/freetype/src/base/ftpatent.c
  thirdparty/freetype/src/pcf/pcf.c
  thirdparty/freetype/src/pfr/pfr.c
  thirdparty/freetype/src/psaux/psaux.c
  thirdparty/freetype/src/pshinter/pshinter.c
  thirdparty/freetype/src/psnames/psmodule.c
  thirdparty/freetype/src/raster/raster.c
  thirdparty/freetype/src/sfnt/sfnt.c
  thirdparty/freetype/src/truetype/truetype.c
  thirdparty/freetype/src/type1/type1.c
  thirdparty/freetype/src/cid/type1cid.c
  thirdparty/freetype/src/type42/type42.c
  thirdparty/freetype/src/winfonts/winfnt.c
)
target_include_directories(freetype PUBLIC thirdparty/freetype/include)
target_include_directories(freetype PRIVATE thirdparty/libogg/include thirdparty/libvorbis/include thirdparty/libvorbis/lib)
target_compile_definitions(freetype PRIVATE FT2_BUILD_LIBRARY)
add_library(pugixml STATIC
  thirdparty/pugixml/src/pugixml.cpp
)
target_include_directories(pugixml PUBLIC thirdparty/pugixml/src)
target_include_directories(pugixml PRIVATE thirdparty/libogg/include thirdparty/libvorbis/include thirdparty/libvorbis/lib)
target_compile_definitions(pugixml PUBLIC PUGIXML_WCHAR_MODE)
include(cmake/FreeImage.cmake)
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS src/*.cpp plasma/src/*.cpp)
if(CUBE_LINUX)
  include(cmake/Linux.cmake)
else()
  add_executable(cubeworld WIN32 ${GAME_SOURCES} platform/FreeImageRuntime.cpp)
  target_include_directories(cubeworld PRIVATE include plasma/include thirdparty/steamworks)
  target_compile_definitions(cubeworld PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0602 DIRECTINPUT_VERSION=0x0800 _CRT_SECURE_NO_WARNINGS)
  target_link_libraries(cubeworld PRIVATE pugixml sqlite vorbisfile libvorbis libogg zlib freetype FreeImage d3d11 dinput8 dxguid xinput winmm ws2_32 glu32 gdi32 ole32 shell32)
  set(STEAMWORKS_SDK "" CACHE PATH "Steamworks SDK root; empty builds local offline mode")
  if(STEAMWORKS_SDK)
    target_link_libraries(cubeworld PRIVATE "${STEAMWORKS_SDK}/redistributable_bin/win64/steam_api64.lib")
  else()
    target_sources(cubeworld PRIVATE platform/SteamOffline.cpp)
    target_compile_definitions(cubeworld PRIVATE STEAM_API_NODLL CUBE_OFFLINE)
  endif()
  if(MINGW)
    target_link_options(cubeworld PRIVATE -static -static-libgcc -static-libstdc++)
  else()
    target_compile_options(cubeworld PRIVATE /std:c++17 /bigobj /EHsc)
  endif()
endif()

include(cmake/Shaders.cmake)
# The mod (src/mod): the JSON reader of its data, and the data (data/mod) next to the executable.
target_include_directories(cubeworld PRIVATE thirdparty/nlohmann)
add_custom_command(TARGET cubeworld POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/data/mod" "$<TARGET_FILE_DIR:cubeworld>/mod")
if(NOT CUBE_LINUX)
  # XAudio2 2.8 (Windows 8 and later), the version the game was written for.
  target_link_libraries(cubeworld PRIVATE xaudio2_8)
endif()
if(MINGW)
  # This large sequence of sprite registrations otherwise triggers very costly GCC inlining.
  set_source_files_properties(src/cube/SpriteManager.cpp PROPERTIES COMPILE_OPTIONS -O1)
endif()
# Keep the game's zlib separate from FreeImage's newer bundled zlib.
target_compile_definitions(zlib PUBLIC Z_PREFIX)
target_compile_definitions(zlib PRIVATE z_errmsg=cube_z_errmsg)
