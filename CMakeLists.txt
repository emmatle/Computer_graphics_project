cmake_minimum_required(VERSION 3.10)
if (WIN32)
    message(STATUS "Using vcpkg toolchain on Windows")
    set(CMAKE_TOOLCHAIN_FILE "$ENV{USERPROFILE}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
endif ()

set(APP_NAME "Escape_Room")
project(${APP_NAME} LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)

if (APPLE)
    # Rileva l'architettura attuale: x86_64 (Intel) oppure arm64 (Apple Silicon)
    execute_process(
            COMMAND uname -m
            OUTPUT_VARIABLE ARCHITECTURE
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if (ARCHITECTURE STREQUAL "arm64")
        message(STATUS "Detected Apple Silicon (arm64) — using /opt/homebrew paths")
        include_directories(
                /opt/homebrew/include
                /opt/homebrew/include/freetype2
                /opt/homebrew/opt/assimp/include
                /opt/homebrew/opt/glfw/include
        )
        link_directories(
                /opt/homebrew/lib
                /opt/homebrew/opt/assimp/lib
                /opt/homebrew/opt/glfw/lib
        )
    else ()
        message(STATUS "Detected Intel macOS (x86_64) — using /usr/local paths")
        include_directories(
                /usr/local/include
                /usr/local/include/freetype2
                /usr/local/opt/assimp/include
                /usr/local/opt/glfw/include
        )
        link_directories(
                /usr/local/lib
                /usr/local/opt/assimp/lib
                /usr/local/opt/glfw/lib
        )
    endif ()
endif ()

# === Collect all source files ===
file(GLOB_RECURSE APP_SOURCES
        "OpenGLApp/*.cpp"
        "OpenGLApp/*/*.cpp"
        "glad/src/glad.c"
        "imgui/*.cpp"
        "imgui/*/*.cpp"
)


# === Create executable ===
if (APPLE)
    message(STATUS "Create executable on Apple")
    add_executable(${APP_NAME} ${APP_SOURCES} OpenGLApp)
elseif (WIN32)
    message(STATUS "Create executable on Windows")
    add_executable(${APP_NAME}
            ${APP_SOURCES}
            OpenGLApp
            OpenGLApp/app_icon.rc
    )
elseif (UNIX AND NOT APPLE)
    message(STATUS "Create executable on Linux")
    add_executable(${APP_NAME} ${APP_SOURCES} OpenGLApp
            OpenGLApp/text_renderer.cpp
            OpenGLApp/text_renderer.h)
endif ()

# 🔧 Include directories assegnati correttamente al target
message(STATUS "Include directories")
target_include_directories(${APP_NAME} PRIVATE
        .
        OpenGLApp
        glad/include
        glm
        imgui
        json
        stb
)

# === Link libraries ===
if (WIN32)
    message(STATUS "Link directories on Windows")
    find_package(OpenGL REQUIRED)
    find_package(Freetype REQUIRED)
    find_package(SndFile REQUIRED)
    find_package(assimp CONFIG REQUIRED)
    find_package(glfw3 CONFIG REQUIRED)
    find_package(OpenAL CONFIG REQUIRED)
    target_link_libraries(${APP_NAME} PRIVATE
            assimp::assimp
            OpenGL::GL
            glfw
            freetype
            SndFile::sndfile
            OpenAL::OpenAL
    )
elseif (APPLE)
    message(STATUS "Link directories on Apple")
    find_package(OpenGL REQUIRED)
    find_package(Freetype REQUIRED)
    find_package(SndFile REQUIRED)
    target_link_libraries(${APP_NAME}
            PRIVATE
            ${OPENGL_LIBRARIES}
            glfw
            assimp
            freetype
            ${SNDFILE_LIBRARIES}
            "-framework OpenAL"
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreVideo"
    )
elseif (UNIX AND NOT APPLE)
    message(STATUS "Link directories on Linux")
    find_package(OpenGL REQUIRED)
    find_package(Freetype REQUIRED)
    find_package(assimp REQUIRED)
    find_package(glfw3 REQUIRED)
    find_package(OpenAL REQUIRED)
    find_package(SndFile REQUIRED)

    target_link_libraries(${APP_NAME} PRIVATE
            ${OPENGL_LIBRARIES}
            glfw
            assimp
            freetype
            ${SNDFILE_LIBRARIES}
            ${OPENAL_LIBRARIES}
            dl pthread
    )
endif ()

# === Create a macOS .app bundle and assign the app icon ===
if (APPLE)
    message(STATUS "Create a macOS .app bundle")
    set_target_properties(${APP_NAME} PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_ICON_FILE AppIcon.icns)
endif ()

# === Create the absolute path for the resource folder as C++ macro ===
if (APPLE)
    message(STATUS " Create the absolute path for the resource on APPLE")
    target_compile_definitions(${APP_NAME} PRIVATE RESOURCE_PATH="../Resources")
else ()
    message(STATUS " Create the absolute path for the resource on Windows/Linux")
    target_compile_definitions(${APP_NAME} PRIVATE RESOURCE_PATH="resources")
endif ()

# === Post-build actions ===
if (APPLE)
    message(STATUS "Post-build actions on Apple")
    add_custom_command(TARGET ${APP_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/OpenGLApp/resources
            ${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}.app/Contents/Resources
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/OpenGLApp/resources/Icon/Icon.icns
            ${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}.app/Contents/Resources/AppIcon.icns
    )
elseif (WIN32 OR (UNIX AND NOT APPLE))
    message(STATUS "Post-build actions on Windows/Linux")
    add_custom_command(TARGET ${APP_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/OpenGLApp/resources
            ${CMAKE_CURRENT_BINARY_DIR}/resources
    )
endif ()

# === Build summary ===
message(STATUS "---- BUILD CONFIGURATION ----")
message(STATUS "OpenGL: ${OPENGL_LIBRARIES}")
message(STATUS "FreeType: ${FREETYPE_LIBRARIES}")
message(STATUS "Assimp: ${ASSIMP_LIBRARIES}")
message(STATUS "GLFW: ${GLFW_LIBRARIES}")
message(STATUS "SndFile: ${SNDFILE_LIBRARIES}")
message(STATUS "-----------------------------")
