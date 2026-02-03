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

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# Подключаем модуль
add_subdirectory(OpenGLSomethingFrameDisplayerEVO)

# Проверяем что модуль создан
if(NOT TARGET OpenGLSomethingFrameDisplayerEVO)
    message(FATAL_ERROR "Module OpenGLSomethingFrameDisplayerEVO not found")
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

pkg_check_modules(SDL2 REQUIRED sdl2)

# Собираем исходники основного приложения
file(GLOB_RECURSE SOURCE_FILES
    "videoPlayer/*.cpp"
)

# Создаем исполняемый файл
add_executable(${PROJECT_NAME} ${SOURCE_FILES} main.cpp Globals.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Линкуем зависимости
target_link_libraries(${PROJECT_NAME} PRIVATE
    PkgConfig::LIBAV
    avcodec
    avformat
    avutil
    swscale
    ${SDL2_LIBRARIES}
    ${CMAKE_DL_LIBS}
    OpenGLSomethingFrameDisplayerEVO  # ПРОСТОЕ ИМЯ БЕЗ :::
)

# Копируем ресурсы модуля
copy_opengl_resources(${PROJECT_NAME})

# Флаги компиляции
if(LINUX)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:RelWithDebInfo>:-O3 -g -ffast-math>
        $<$<CONFIG:Release>:-O3 -DNDEBUG -ffast-math -march=native -mtune=native>
    )
elseif(MACOS)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:RelWithDebInfo>:-O3 -g -ffast-math>
        $<$<CONFIG:Release>:-O3 -DNDEBUG -ffast-math>
    )
endif()

# Копируем тестовое видео
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/video.mp4
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/video.mp4
    COMMENT "Copying test video to executable directory"
)