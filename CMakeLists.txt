cmake_minimum_required(VERSION 3.23)
project(Main)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenMP    REQUIRED)
find_package(OpenGL    REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(EPOXY REQUIRED epoxy)
find_package(glfw3     REQUIRED)

# ---- ImGui (downloaded at configure time) ---------------------------------
include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.6
    GIT_SHALLOW    TRUE
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)

    # Wrapper that injects <epoxy/gl.h> before imgui's OpenGL3 backend, then
    # defines IMGUI_IMPL_OPENGL_LOADER_CUSTOM so imgui doesn't try to include
    # GLEW/GLAD (which would fail on a Wayland/EGL context).
    set(IMGUI_GL3_WRAP ${CMAKE_BINARY_DIR}/imgui_opengl3_epoxy.cpp)
    file(WRITE ${IMGUI_GL3_WRAP}
        "#include <epoxy/gl.h>\n"
        "#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM\n"
        "#include \"imgui_impl_opengl3.cpp\"\n"
    )

    add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_GL3_WRAP}
    )
    target_include_directories(imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends   # lets the wrapper find imgui_impl_opengl3.cpp
        ${EPOXY_INCLUDE_DIRS}
    )
    target_link_libraries(imgui PUBLIC glfw ${EPOXY_LINK_LIBRARIES})
endif()
# ---------------------------------------------------------------------------

add_executable(Main)
target_sources(Main PRIVATE Main.cpp)
target_compile_options(Main PRIVATE -O3 -march=native)
target_include_directories(Main PRIVATE ${EPOXY_INCLUDE_DIRS})
target_link_libraries(Main PRIVATE
    OpenMP::OpenMP_CXX
    OpenGL::GL
    ${EPOXY_LINK_LIBRARIES}
    glfw
    imgui
)
