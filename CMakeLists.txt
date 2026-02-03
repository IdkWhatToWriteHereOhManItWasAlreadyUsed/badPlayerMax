cmake_minimum_required(VERSION 3.10.0)
project(badPlayer VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Определяем ОС
if(APPLE)
    set(MACOS TRUE)
elseif(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

find_package(PkgConfig REQUIRED)

# Найти библиотеки через pkg-config
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    libavformat
    libavcodec
    libswscale
    libavutil
    libswresample
)

find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

# Для OpenGL на разных платформах
if(LINUX)
    set(OpenGL_GL_PREFERENCE GLVND)
    find_package(OpenGL REQUIRED)
    find_package(Threads REQUIRED)
elseif(MACOS)
    find_package(OpenGL REQUIRED)
    # На macOS OpenGL встроен в систему
endif()

file(GLOB_RECURSE SOURCE_FILES
    "OpenGLSomethingFrameDisplayerEVO/*.cpp"
    "videoPlayer/*.cpp"
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES} main.cpp Globals.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/OpenGLSomethingFrameDisplayerEVO/"
)


target_link_libraries(${PROJECT_NAME} PRIVATE
    PkgConfig::LIBAV
    glfw
    GLEW::GLEW
    OpenGL::GL
    avcodec
    avformat
    avutil
    swscale
    ${SDL2_LIBRARIES}
    ${CMAKE_DL_LIBS}
)

if(MACOS)
    # На macOS другие библиотеки
    find_library(COCOA_LIBRARY Cocoa)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COREVIDEO_LIBRARY CoreVideo)
    
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${COCOA_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREVIDEO_LIBRARY}
        glfw
        "-framework OpenGL"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework Cocoa"
    )
endif()


# Флаги компиляции для разных платформ
if(LINUX)
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -ffast-math")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -ffast-math -march=native -mtune=native")
elseif(MACOS)
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -ffast-math")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -ffast-math")
endif()

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:RelWithDebInfo>:-O3 -g -ffast-math>
    $<$<CONFIG:Release>:-O3 -DNDEBUG -ffast-math>
)

# Дополнительные флаги для Linux
if(LINUX)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:-march=native -mtune=native>
    )
endif()


# Копирование папки res в директорию с исполняемым файлом
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/OpenGLSomethingFrameDisplayerEVO/res
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/res
    COMMENT "Copying res folder to executable directory"
)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/video.mp4
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/video.mp4
    COMMENT "Copying test video to executable directory"
)