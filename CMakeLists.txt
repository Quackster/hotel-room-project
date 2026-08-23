cmake_minimum_required(VERSION 3.10)
project(hotel-room)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/main.cpp
    src/Room.cpp
    src/Camera.cpp
    src/Mesh.cpp
    src/Renderer.cpp
    src/wasm_api.cpp
)

add_executable(room ${SOURCES})

target_include_directories(room PRIVATE src)

# Emscripten-specific options
if(EMSCRIPTEN)
    set_target_properties(room PROPERTIES SUFFIX ".js")

    target_link_options(room PRIVATE
        "-sWASM=1"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sEXPORTED_FUNCTIONS=['_main','_initialize','_resize','_render','_selectTile','_setCamera','_setCameraYawPitchZoom','_getSelectedX','_getSelectedY','_getRoomWidth','_getRoomHeight','_pickTile','_handleClick']"
        "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','GL']"
        "-sUSE_WEBGL2=1"
        "-sFULL_ES2=1"
        "-sMIN_WEBGL_VERSION=2"
        "-sMAX_WEBGL_VERSION=2"
        "-sMODULARIZE=1"
        "-sEXPORT_ES6=1"
        "-sEXPORT_NAME=createRoomModule"
        "-sENVIRONMENT=web"
        "-sASYNCIFY=0"
        "-O2"
        "--bind"
    )

    # Enable WebGL and HTML5
    target_compile_options(room PRIVATE "-sUSE_WEBGL2=1")

    # Copy web files after build
    add_custom_command(TARGET room POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/build
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:room> ${CMAKE_SOURCE_DIR}/build/room.js
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/room.wasm ${CMAKE_SOURCE_DIR}/build/room.wasm
        COMMENT "Copying WASM artifacts to build/"
    )
else()
    message(WARNING "Not building with Emscripten. Use emcmake cmake ...")
endif()
