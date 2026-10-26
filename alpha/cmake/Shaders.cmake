find_program(FXC_EXECUTABLE fxc HINTS "$ENV{WindowsSdkDir}/bin/$ENV{WindowsSDKVersion}/x64")
if(FXC_EXECUTABLE)
 set(SHADER_DEFAULT ON)
else()
 set(SHADER_DEFAULT OFF)
endif()
option(REBUILD_SHADERS "Compile HLSL with fxc" ${SHADER_DEFAULT})
file(GLOB_RECURSE SHADER_INCLUDES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/cube/shaders/*.hlsli" "${CMAKE_CURRENT_SOURCE_DIR}/plasma/src/plasma/shaders/*.hlsli")
if(REBUILD_SHADERS)
 if(NOT FXC_EXECUTABLE)
  message(FATAL_ERROR "REBUILD_SHADERS requires fxc from the Windows SDK")
 endif()
 foreach(app Cube Server)
  target_include_directories(${app} BEFORE PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
 endforeach()
 file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders")
 add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/WorldVertexShader.h"
 COMMAND "${FXC_EXECUTABLE}" /nologo /T vs_3_0 /E main /Fo "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/WorldVertexShader.h.bin" "${CMAKE_CURRENT_SOURCE_DIR}/src/cube/shaders/WorldVertexShader.hlsl"
 COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/BytecodeHeader.py" "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/WorldVertexShader.h.bin" "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/WorldVertexShader.h" worldVertexShader
 DEPENDS "src/cube/shaders/WorldVertexShader.hlsl" ${SHADER_INCLUDES} cmake/BytecodeHeader.py VERBATIM)
 target_sources(Cube PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/WorldVertexShader.h")
 target_sources(Server PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/WorldVertexShader.h")
 file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders")
 add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/WorldPixelShader.h"
 COMMAND "${FXC_EXECUTABLE}" /nologo /T ps_3_0 /E main /Fo "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/WorldPixelShader.h.bin" "${CMAKE_CURRENT_SOURCE_DIR}/src/cube/shaders/WorldPixelShader.hlsl"
 COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/BytecodeHeader.py" "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/WorldPixelShader.h.bin" "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/WorldPixelShader.h" worldPixelShader
 DEPENDS "src/cube/shaders/WorldPixelShader.hlsl" ${SHADER_INCLUDES} cmake/BytecodeHeader.py VERBATIM)
 target_sources(Cube PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/WorldPixelShader.h")
 target_sources(Server PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/WorldPixelShader.h")
 file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders")
 add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/SkyPixelShader.h"
 COMMAND "${FXC_EXECUTABLE}" /nologo /T ps_3_0 /E main /Fo "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/SkyPixelShader.h.bin" "${CMAKE_CURRENT_SOURCE_DIR}/src/cube/shaders/SkyPixelShader.hlsl"
 COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/BytecodeHeader.py" "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/SkyPixelShader.h.bin" "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/SkyPixelShader.h" skyPixelShader
 DEPENDS "src/cube/shaders/SkyPixelShader.hlsl" ${SHADER_INCLUDES} cmake/BytecodeHeader.py VERBATIM)
 target_sources(Cube PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/SkyPixelShader.h")
 target_sources(Server PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/SkyPixelShader.h")
 file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders")
 add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/CloudPixelShader.h"
 COMMAND "${FXC_EXECUTABLE}" /nologo /T ps_3_0 /E main /Fo "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/CloudPixelShader.h.bin" "${CMAKE_CURRENT_SOURCE_DIR}/src/cube/shaders/CloudPixelShader.hlsl"
 COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/BytecodeHeader.py" "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/CloudPixelShader.h.bin" "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/CloudPixelShader.h" cloudPixelShader
 DEPENDS "src/cube/shaders/CloudPixelShader.hlsl" ${SHADER_INCLUDES} cmake/BytecodeHeader.py VERBATIM)
 target_sources(Cube PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/CloudPixelShader.h")
 target_sources(Server PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/CloudPixelShader.h")
 file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders")
 add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/WaterPixelShader.h"
 COMMAND "${FXC_EXECUTABLE}" /nologo /T ps_3_0 /E main /Fo "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/WaterPixelShader.h.bin" "${CMAKE_CURRENT_SOURCE_DIR}/src/cube/shaders/WaterPixelShader.hlsl"
 COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/BytecodeHeader.py" "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/WaterPixelShader.h.bin" "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/WaterPixelShader.h" waterPixelShader
 DEPENDS "src/cube/shaders/WaterPixelShader.hlsl" ${SHADER_INCLUDES} cmake/BytecodeHeader.py VERBATIM)
 target_sources(Cube PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/WaterPixelShader.h")
 target_sources(Server PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/WaterPixelShader.h")
 file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders")
 add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/UnusedPixelShader.h"
 COMMAND "${FXC_EXECUTABLE}" /nologo /T ps_3_0 /E main /Fo "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/UnusedPixelShader.h.bin" "${CMAKE_CURRENT_SOURCE_DIR}/src/cube/shaders/UnusedPixelShader.hlsl"
 COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/BytecodeHeader.py" "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/UnusedPixelShader.h.bin" "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/UnusedPixelShader.h" unusedPixelShader
 DEPENDS "src/cube/shaders/UnusedPixelShader.hlsl" ${SHADER_INCLUDES} cmake/BytecodeHeader.py VERBATIM)
 target_sources(Cube PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/UnusedPixelShader.h")
 target_sources(Server PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/cube/shaders/UnusedPixelShader.h")
 file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders")
 add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/ShapeVertexShader.h"
 COMMAND "${FXC_EXECUTABLE}" /nologo /T vs_3_0 /E main /Fo "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/ShapeVertexShader.h.bin" "${CMAKE_CURRENT_SOURCE_DIR}/plasma/src/plasma/shaders/ShapeVertexShader.hlsl"
 COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/BytecodeHeader.py" "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/ShapeVertexShader.h.bin" "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/ShapeVertexShader.h" shapeVertexShader
 DEPENDS "plasma/src/plasma/shaders/ShapeVertexShader.hlsl" ${SHADER_INCLUDES} cmake/BytecodeHeader.py VERBATIM)
 target_sources(Cube PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/ShapeVertexShader.h")
 file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders")
 add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/ShapePixelShader.h"
 COMMAND "${FXC_EXECUTABLE}" /nologo /T ps_3_0 /E main /Fo "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/ShapePixelShader.h.bin" "${CMAKE_CURRENT_SOURCE_DIR}/plasma/src/plasma/shaders/ShapePixelShader.hlsl"
 COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/BytecodeHeader.py" "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/ShapePixelShader.h.bin" "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/ShapePixelShader.h" shapePixelShader
 DEPENDS "plasma/src/plasma/shaders/ShapePixelShader.hlsl" ${SHADER_INCLUDES} cmake/BytecodeHeader.py VERBATIM)
 target_sources(Cube PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/ShapePixelShader.h")
 file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders")
 add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/SurfaceVertexShader.h"
 COMMAND "${FXC_EXECUTABLE}" /nologo /T vs_3_0 /E main /Fo "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/SurfaceVertexShader.h.bin" "${CMAKE_CURRENT_SOURCE_DIR}/plasma/src/plasma/shaders/SurfaceVertexShader.hlsl"
 COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/BytecodeHeader.py" "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/SurfaceVertexShader.h.bin" "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/SurfaceVertexShader.h" surfaceVertexShader
 DEPENDS "plasma/src/plasma/shaders/SurfaceVertexShader.hlsl" ${SHADER_INCLUDES} cmake/BytecodeHeader.py VERBATIM)
 target_sources(Cube PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/SurfaceVertexShader.h")
 file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders")
 add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/SurfacePixelShader.h"
 COMMAND "${FXC_EXECUTABLE}" /nologo /T ps_3_0 /E main /Fo "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/SurfacePixelShader.h.bin" "${CMAKE_CURRENT_SOURCE_DIR}/plasma/src/plasma/shaders/SurfacePixelShader.hlsl"
 COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/BytecodeHeader.py" "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/SurfacePixelShader.h.bin" "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/SurfacePixelShader.h" surfacePixelShader
 DEPENDS "plasma/src/plasma/shaders/SurfacePixelShader.hlsl" ${SHADER_INCLUDES} cmake/BytecodeHeader.py VERBATIM)
 target_sources(Cube PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/SurfacePixelShader.h")
 file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders")
 add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/Downsample2PixelShader.h"
 COMMAND "${FXC_EXECUTABLE}" /nologo /T ps_3_0 /E main /Fo "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/Downsample2PixelShader.h.bin" "${CMAKE_CURRENT_SOURCE_DIR}/plasma/src/plasma/shaders/Downsample2PixelShader.hlsl"
 COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/BytecodeHeader.py" "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/Downsample2PixelShader.h.bin" "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/Downsample2PixelShader.h" downsample2PixelShader
 DEPENDS "plasma/src/plasma/shaders/Downsample2PixelShader.hlsl" ${SHADER_INCLUDES} cmake/BytecodeHeader.py VERBATIM)
 target_sources(Cube PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/Downsample2PixelShader.h")
 file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders")
 add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/Downsample3PixelShader.h"
 COMMAND "${FXC_EXECUTABLE}" /nologo /T ps_3_0 /E main /Fo "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/Downsample3PixelShader.h.bin" "${CMAKE_CURRENT_SOURCE_DIR}/plasma/src/plasma/shaders/Downsample3PixelShader.hlsl"
 COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/BytecodeHeader.py" "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/Downsample3PixelShader.h.bin" "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/Downsample3PixelShader.h" downsample3PixelShader
 DEPENDS "plasma/src/plasma/shaders/Downsample3PixelShader.hlsl" ${SHADER_INCLUDES} cmake/BytecodeHeader.py VERBATIM)
 target_sources(Cube PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/Downsample3PixelShader.h")
 file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders")
 add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/Downsample4PixelShader.h"
 COMMAND "${FXC_EXECUTABLE}" /nologo /T ps_3_0 /E main /Fo "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/Downsample4PixelShader.h.bin" "${CMAKE_CURRENT_SOURCE_DIR}/plasma/src/plasma/shaders/Downsample4PixelShader.hlsl"
 COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/BytecodeHeader.py" "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/Downsample4PixelShader.h.bin" "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/Downsample4PixelShader.h" downsample4PixelShader
 DEPENDS "plasma/src/plasma/shaders/Downsample4PixelShader.hlsl" ${SHADER_INCLUDES} cmake/BytecodeHeader.py VERBATIM)
 target_sources(Cube PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/Downsample4PixelShader.h")
 file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders")
 add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/VerticalBlurPixelShader.h"
 COMMAND "${FXC_EXECUTABLE}" /nologo /T ps_2_0 /E main /Fo "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/VerticalBlurPixelShader.h.bin" "${CMAKE_CURRENT_SOURCE_DIR}/plasma/src/plasma/shaders/VerticalBlurPixelShader.hlsl"
 COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/BytecodeHeader.py" "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/VerticalBlurPixelShader.h.bin" "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/VerticalBlurPixelShader.h" verticalBlurPixelShader
 DEPENDS "plasma/src/plasma/shaders/VerticalBlurPixelShader.hlsl" ${SHADER_INCLUDES} cmake/BytecodeHeader.py VERBATIM)
 target_sources(Cube PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/VerticalBlurPixelShader.h")
 file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders")
 add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/HorizontalBlurPixelShader.h"
 COMMAND "${FXC_EXECUTABLE}" /nologo /T ps_3_0 /E main /Fo "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/HorizontalBlurPixelShader.h.bin" "${CMAKE_CURRENT_SOURCE_DIR}/plasma/src/plasma/shaders/HorizontalBlurPixelShader.hlsl"
 COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/BytecodeHeader.py" "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/HorizontalBlurPixelShader.h.bin" "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/HorizontalBlurPixelShader.h" horizontalBlurPixelShader
 DEPENDS "plasma/src/plasma/shaders/HorizontalBlurPixelShader.hlsl" ${SHADER_INCLUDES} cmake/BytecodeHeader.py VERBATIM)
 target_sources(Cube PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/HorizontalBlurPixelShader.h")
endif()
