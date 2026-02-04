cmake_minimum_required(VERSION 3.16)
if (WIN32)
    message(STATUS "Using vcpkg toolchain on Windows")
    set(CMAKE_TOOLCHAIN_FILE "$ENV{USERPROFILE}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
endif ()

set(APP_NAME "Escape_Room")
project(${APP_NAME} LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)

# === Install into a local folder inside the build directory (portable) ===
set(CMAKE_INSTALL_PREFIX
        ${CMAKE_SOURCE_DIR}/dist
        CACHE PATH "Install path"
        FORCE
)

if (MSVC)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME ON)
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
message(STATUS "Create executable")
if (APPLE)
    add_executable(${APP_NAME} MACOSX_BUNDLE ${APP_SOURCES} OpenGLApp)
elseif (WIN32)
    add_executable(${APP_NAME} ${APP_SOURCES} OpenGLApp OpenGLApp/app_icon.rc)
else ()
    add_executable(${APP_NAME} ${APP_SOURCES} OpenGLApp)
endif ()

# === Include local libraries ===
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
message(STATUS "Link libraries")
find_package(OpenGL REQUIRED)
find_package(Freetype REQUIRED)
find_package(SFML CONFIG REQUIRED COMPONENTS Audio)

if (WIN32)
    find_package(assimp CONFIG REQUIRED)
    find_package(glfw3 CONFIG REQUIRED)
else ()
    find_package(assimp REQUIRED)
    find_package(glfw3 REQUIRED)
endif ()

target_link_libraries(${APP_NAME} PRIVATE
        glfw
        assimp::assimp
        OpenGL::GL
        Freetype::Freetype
        SFML::Audio
        SFML::System
)
if (UNIX)
    target_link_libraries(${APP_NAME} PRIVATE dl pthread)

    if (APPLE)
        target_link_libraries(${APP_NAME} PRIVATE
                "-framework Cocoa"
                "-framework IOKit"
                "-framework CoreVideo"
                "-framework CoreAudio"
                "-framework AudioToolbox"
        )
    endif ()
endif ()

# === macOS bundle properties ===
if (APPLE)
    set_target_properties(${APP_NAME} PROPERTIES
            MACOSX_BUNDLE_ICON_FILE AppIcon.icns
            MACOSX_BUNDLE_BUNDLE_NAME "Escape Room Studio"
    )
endif ()

# === Compile definitions ===
message(STATUS "Add compile definition for the resource folder")
if (APPLE)
    target_compile_definitions(${APP_NAME} PRIVATE RESOURCE_PATH="../Resources")
else ()
    target_compile_definitions(${APP_NAME} PRIVATE RESOURCE_PATH="resources")
endif ()

# === Post-build actions ===
message(STATUS "Post-build script")
if (APPLE)
    add_custom_command(TARGET ${APP_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/OpenGLApp/resources
            ${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}.app/Contents/Resources
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/OpenGLApp/resources/Icon/Icon.icns
            ${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}.app/Contents/Resources/AppIcon.icns
    )
else ()
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
message(STATUS "-----------------------------")

# === Installation ===
install(TARGETS ${APP_NAME}
        RUNTIME DESTINATION .           # Windows / Linux
        BUNDLE DESTINATION .              # macOS .app bundle
)

# Install dynamic libraries
if (WIN32)
    # On Windows with vcpkg, DLLs are often in the same folder as the executable after build
    # We can use a post-install script or glob them.
    # Better yet, use file(GET_RUNTIME_DEPENDENCIES) in an install script.
    install(CODE [[
        file(GET_RUNTIME_DEPENDENCIES
            EXECUTABLES $<TARGET_FILE:Escape_Room>
            RESOLVED_DEPENDENCIES_VAR _resolved_deps
            UNRESOLVED_DEPENDENCIES_VAR _unresolved_deps
            POST_EXCLUDE_REGEXES ".*system32/.*" ".*SYSTEM32/.*" ".*winsxs/.*" ".*WINSXS/.*"
        )
        foreach(_file ${_resolved_deps})
            file(INSTALL
                DESTINATION "${CMAKE_INSTALL_PREFIX}"
                TYPE SHARED_LIBRARY
                FILES "${_file}"
            )
        endforeach()
    ]])
elseif (UNIX AND NOT APPLE)
    # Linux
    install(CODE [[
        file(GET_RUNTIME_DEPENDENCIES
            EXECUTABLES $<TARGET_FILE:Escape_Room>
            RESOLVED_DEPENDENCIES_VAR _resolved_deps
            UNRESOLVED_DEPENDENCIES_VAR _unresolved_deps
        )
        foreach(_file ${_resolved_deps})
            file(INSTALL
                DESTINATION "${CMAKE_INSTALL_PREFIX}"
                TYPE SHARED_LIBRARY
                FILES "${_file}"
            )
        endforeach()
    ]])
elseif (APPLE)
    # macOS - normally handled by bundle tools, but if we want them in the bundle:
    # We can use fixup_bundle in an install script
    install(CODE [[
        include(BundleUtilities)
        fixup_bundle("${CMAKE_INSTALL_PREFIX}/Escape_Room.app" "" "")
    ]])
endif ()

if (APPLE)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/OpenGLApp/resources
            DESTINATION ${APP_NAME}.app/Contents
    )
else ()
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/OpenGLApp/resources
            DESTINATION .
    )
endif ()

# Custom install targets for different build configurations
add_custom_target(install_debug
        COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR} --config Debug
        COMMENT "Installing Debug build"
)

add_custom_target(install_release
        COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR} --config Release
        COMMENT "Installing Release build"
)