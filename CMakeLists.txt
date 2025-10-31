cmake_minimum_required(VERSION 3.10)

if (WIN32)
    message(STATUS "Using vcpkg toolchain on Windows")
    set(CMAKE_TOOLCHAIN_FILE "C:/Users/salva/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
endif()
#TODO: Change it to your project Name
set(APP_NAME "Your_Project")


project(${APP_NAME})

set(CMAKE_CXX_STANDARD 14)

# === Include directories ===
include_directories(
        .
        OpenGLApp
        OpenGLApp/DataClasses
        OpenGLApp/GameClasses
        OpenGLApp/ViewController
        glad/include
        glm-master
)

if(APPLE)
    # Rileva l'architettura attuale: x86_64 (Intel) oppure arm64 (Apple Silicon)
    execute_process(
            COMMAND uname -m
            OUTPUT_VARIABLE ARCHITECTURE
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(ARCHITECTURE STREQUAL "arm64")
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
    else()
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
    endif()

elseif(WIN32)
    message(STATUS "Using vcpkg on Windows — no manual include/link needed")
endif()


# === Find core packages ===
find_package(OpenGL REQUIRED)
find_package(Freetype REQUIRED)
find_package(SndFile REQUIRED)
if (WIN32)
    find_package(assimp CONFIG REQUIRED)
    find_package(glfw3 CONFIG REQUIRED)
    find_package(OpenAL CONFIG REQUIRED)
endif()


# === Collect all source files ===
file(GLOB_RECURSE APP_SOURCES
        "OpenGLApp/*.cpp"
        "OpenGLApp/*.h"
        "OpenGLApp/*/*.cpp"
        "OpenGLApp/*/*.h"
        "glad/src/*.c"
)

# === Create executable ===
if (APPLE)
    add_executable(${APP_NAME} ${APP_SOURCES} OpenGLApp)
elseif (WIN32)
    add_executable(${APP_NAME}
            ${APP_SOURCES}
            OpenGLApp
            OpenGLApp/app_icon.rc
    )
endif ()

# === Link core libraries ===

if (WIN32)
    target_link_libraries(${APP_NAME} PRIVATE
            assimp::assimp
            OpenGL::GL
            glfw
            freetype
            SndFile::sndfile
            OpenAL::OpenAL
    )
elseif (APPLE)
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
endif()

# === Create a macOS .app bundle and assign the app icon ===
if(APPLE)
        set_target_properties(${APP_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_ICON_FILE Icon.icns)
endif ()
# === Post-build actions ===
if(APPLE)
    add_custom_command(TARGET ${APP_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/OpenGLApp/resources
            ${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}.app/Contents/resources
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/OpenGLApp/resources/Icon/Icon.icns
            ${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}.app/Contents/Resources/Icon.icns
    )
elseif(WIN32)
    add_custom_command(TARGET ${APP_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/OpenGLApp/resources
            ${CMAKE_CURRENT_BINARY_DIR}/resources
    )
endif()



# === Neat output summary ===
message(STATUS "---- BUILD CONFIGURATION ----")
message(STATUS "OpenGL: ${OPENGL_LIBRARIES}")
message(STATUS "FreeType: ${FREETYPE_LIBRARIES}")
message(STATUS "Assimp: /usr/local/opt/assimp/lib/libassimp.dylib")
message(STATUS "GLFW: /usr/local/opt/glfw/lib/libglfw.3.dylib")
message(STATUS "SndFile: ${SNDFILE_LIBRARIES}")
message(STATUS "-----------------------------")
