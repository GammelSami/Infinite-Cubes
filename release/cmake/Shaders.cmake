find_program(FXC_EXECUTABLE fxc HINTS "$ENV{WindowsSdkDir}/bin/$ENV{WindowsSDKVersion}/x64")
if(FXC_EXECUTABLE)
 set(SHADER_DEFAULT ON)
else()
 set(SHADER_DEFAULT OFF)
endif()
option(REBUILD_SHADERS "Compile HLSL with fxc instead of the checked-in bytecode" ${SHADER_DEFAULT})
if(REBUILD_SHADERS)
 if(NOT FXC_EXECUTABLE)
  message(FATAL_ERROR "REBUILD_SHADERS requires fxc from the Windows SDK")
 endif()
 target_include_directories(cubeworld BEFORE PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
 file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/gfx/shaders")
 file(GLOB SHADER_INCLUDES_WorldVertex "${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/shaders/*.hlsli")
 add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/gfx/shaders/WorldVertexShader.h"
  COMMAND "${FXC_EXECUTABLE}" /nologo /T vs_4_0 /E main /Vn g_worldVertexShader /Fh "${CMAKE_CURRENT_BINARY_DIR}/generated/gfx/shaders/WorldVertexShader.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/shaders/WorldVertexShader.hlsl"
  DEPENDS "src/gfx/shaders/WorldVertexShader.hlsl" ${SHADER_INCLUDES_WorldVertex} VERBATIM)
 target_sources(cubeworld PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/gfx/shaders/WorldVertexShader.h")
 file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/gfx/shaders")
 file(GLOB SHADER_INCLUDES_WorldPixel "${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/shaders/*.hlsli")
 add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/gfx/shaders/WorldPixelShader.h"
  COMMAND "${FXC_EXECUTABLE}" /nologo /T ps_4_0 /E main /Vn g_worldPixelShader /Fh "${CMAKE_CURRENT_BINARY_DIR}/generated/gfx/shaders/WorldPixelShader.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/shaders/WorldPixelShader.hlsl"
  DEPENDS "src/gfx/shaders/WorldPixelShader.hlsl" ${SHADER_INCLUDES_WorldPixel} VERBATIM)
 target_sources(cubeworld PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/gfx/shaders/WorldPixelShader.h")
 file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/gfx/shaders")
 file(GLOB SHADER_INCLUDES_SkyPixel "${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/shaders/*.hlsli")
 add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/gfx/shaders/SkyPixelShader.h"
  COMMAND "${FXC_EXECUTABLE}" /nologo /T ps_4_0 /E main /Vn g_skyPixelShader /Fh "${CMAKE_CURRENT_BINARY_DIR}/generated/gfx/shaders/SkyPixelShader.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/shaders/SkyPixelShader.hlsl"
  DEPENDS "src/gfx/shaders/SkyPixelShader.hlsl" ${SHADER_INCLUDES_SkyPixel} VERBATIM)
 target_sources(cubeworld PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/gfx/shaders/SkyPixelShader.h")
 file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/gfx/shaders")
 file(GLOB SHADER_INCLUDES_WaterPixel "${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/shaders/*.hlsli")
 add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/gfx/shaders/WaterPixelShader.h"
  COMMAND "${FXC_EXECUTABLE}" /nologo /T ps_4_0 /E main /Vn g_waterPixelShader /Fh "${CMAKE_CURRENT_BINARY_DIR}/generated/gfx/shaders/WaterPixelShader.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/shaders/WaterPixelShader.hlsl"
  DEPENDS "src/gfx/shaders/WaterPixelShader.hlsl" ${SHADER_INCLUDES_WaterPixel} VERBATIM)
 target_sources(cubeworld PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/gfx/shaders/WaterPixelShader.h")
 file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/gfx/shaders")
 file(GLOB SHADER_INCLUDES_UnusedPixel "${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/shaders/*.hlsli")
 add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/gfx/shaders/UnusedPixelShader.h"
  COMMAND "${FXC_EXECUTABLE}" /nologo /T ps_4_0 /E main /Vn g_unusedPixelShader /Fh "${CMAKE_CURRENT_BINARY_DIR}/generated/gfx/shaders/UnusedPixelShader.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/shaders/UnusedPixelShader.hlsl"
  DEPENDS "src/gfx/shaders/UnusedPixelShader.hlsl" ${SHADER_INCLUDES_UnusedPixel} VERBATIM)
 target_sources(cubeworld PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/gfx/shaders/UnusedPixelShader.h")
 file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/gfx/shaders")
 file(GLOB SHADER_INCLUDES_CloudPixel "${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/shaders/*.hlsli")
 add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/gfx/shaders/CloudPixelShader.h"
  COMMAND "${FXC_EXECUTABLE}" /nologo /T ps_4_0 /E main /Vn g_cloudPixelShader /Fh "${CMAKE_CURRENT_BINARY_DIR}/generated/gfx/shaders/CloudPixelShader.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/shaders/CloudPixelShader.hlsl"
  DEPENDS "src/gfx/shaders/CloudPixelShader.hlsl" ${SHADER_INCLUDES_CloudPixel} VERBATIM)
 target_sources(cubeworld PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/gfx/shaders/CloudPixelShader.h")
 file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/gfx/shaders")
 file(GLOB SHADER_INCLUDES_TexturedVertex "${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/shaders/*.hlsli")
 add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/gfx/shaders/TexturedVertexShader.h"
  COMMAND "${FXC_EXECUTABLE}" /nologo /T vs_4_0 /E main /Vn g_texturedVertexShader /Fh "${CMAKE_CURRENT_BINARY_DIR}/generated/gfx/shaders/TexturedVertexShader.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/shaders/TexturedVertexShader.hlsl"
  DEPENDS "src/gfx/shaders/TexturedVertexShader.hlsl" ${SHADER_INCLUDES_TexturedVertex} VERBATIM)
 target_sources(cubeworld PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/gfx/shaders/TexturedVertexShader.h")
 file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/gfx/shaders")
 file(GLOB SHADER_INCLUDES_TexturedPixel "${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/shaders/*.hlsli")
 add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/gfx/shaders/TexturedPixelShader.h"
  COMMAND "${FXC_EXECUTABLE}" /nologo /T ps_4_0 /E main /Vn g_texturedPixelShader /Fh "${CMAKE_CURRENT_BINARY_DIR}/generated/gfx/shaders/TexturedPixelShader.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/shaders/TexturedPixelShader.hlsl"
  DEPENDS "src/gfx/shaders/TexturedPixelShader.hlsl" ${SHADER_INCLUDES_TexturedPixel} VERBATIM)
 target_sources(cubeworld PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/gfx/shaders/TexturedPixelShader.h")
 file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders")
 file(GLOB SHADER_INCLUDES_D3D11EngineVertex "${CMAKE_CURRENT_SOURCE_DIR}/plasma/src/plasma/shaders/*.hlsli")
 add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/D3D11EngineVertexShader.h"
  COMMAND "${FXC_EXECUTABLE}" /nologo /T vs_4_0 /E main /Vn g_vertexShaderCode /Fh "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/D3D11EngineVertexShader.h" "${CMAKE_CURRENT_SOURCE_DIR}/plasma/src/plasma/shaders/D3D11EngineVertexShader.hlsl"
  DEPENDS "plasma/src/plasma/shaders/D3D11EngineVertexShader.hlsl" ${SHADER_INCLUDES_D3D11EngineVertex} VERBATIM)
 target_sources(cubeworld PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/D3D11EngineVertexShader.h")
 file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders")
 file(GLOB SHADER_INCLUDES_D3D11EnginePixel "${CMAKE_CURRENT_SOURCE_DIR}/plasma/src/plasma/shaders/*.hlsli")
 add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/D3D11EnginePixelShader.h"
  COMMAND "${FXC_EXECUTABLE}" /nologo /T ps_4_0 /E main /Vn g_pixelShaderCode /Fh "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/D3D11EnginePixelShader.h" "${CMAKE_CURRENT_SOURCE_DIR}/plasma/src/plasma/shaders/D3D11EnginePixelShader.hlsl"
  DEPENDS "plasma/src/plasma/shaders/D3D11EnginePixelShader.hlsl" ${SHADER_INCLUDES_D3D11EnginePixel} VERBATIM)
 target_sources(cubeworld PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/plasma/shaders/D3D11EnginePixelShader.h")
endif()
