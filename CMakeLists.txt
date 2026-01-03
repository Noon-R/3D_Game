cmake_minimum_required(VERSION 3.15)
project(DirectX12_3DPlatformer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows only
if(NOT WIN32)
    message(FATAL_ERROR "This project requires Windows and DirectX 12")
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/DX12Core.cpp
    src/Player.cpp
    src/Platform.cpp
)

# Header files
set(HEADERS
    include/DX12Core.h
    include/Player.h
    include/Platform.h
    include/d3dx12.h
)

# Shader files
set(SHADERS
    shaders/VertexShader.hlsl
    shaders/PixelShader.hlsl
)

# Create executable
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS} ${SHADERS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    d3d12.lib
    dxgi.lib
    d3dcompiler.lib
)

# Set working directory for debugging
set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# Copy shaders to output directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
)

# Set shader files as HLSL in Visual Studio
if(MSVC)
    foreach(SHADER ${SHADERS})
        set_source_files_properties(${SHADER} PROPERTIES
            VS_TOOL_OVERRIDE "None"
        )
    endforeach()
endif()
