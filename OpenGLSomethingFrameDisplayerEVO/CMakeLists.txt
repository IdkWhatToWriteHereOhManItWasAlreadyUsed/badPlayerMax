cmake_minimum_required(VERSION 3.16)
project(OpenGLSomethingFrameDisplayerEVO LANGUAGES CXX)

# Определение платформы
if(APPLE)
    set(MACOS TRUE)
elseif(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

# Автоматический сбор исходных файлов
file(GLOB_RECURSE SOURCE_FILES
    "*.cpp"
    "imgui/*.cpp"
    "libs/*.cpp"
    "renderer/*.cpp"
    "ThreadPool/*.cpp"
    "VectorPool/*.cpp"
    "videoCore/*.cpp"
    "videoWorld/*.cpp"
)

# Создаем библиотеку с простым именем
add_library(OpenGLSomethingFrameDisplayerEVO STATIC ${SOURCE_FILES})

# Публичные заголовки
target_include_directories(OpenGLSomethingFrameDisplayerEVO
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

# Настройки компиляции
target_compile_features(OpenGLSomethingFrameDisplayerEVO PRIVATE cxx_std_17)

# Зависимости OpenGL
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)

# Платформо-специфичные настройки
if(LINUX)
    set(OpenGL_GL_PREFERENCE GLVND)
    find_package(Threads REQUIRED)
    target_link_libraries(OpenGLSomethingFrameDisplayerEVO PRIVATE Threads::Threads)
endif()

# Основные зависимости
target_link_libraries(OpenGLSomethingFrameDisplayerEVO PRIVATE
    glfw
    GLEW::GLEW
    OpenGL::GL
    ${CMAKE_DL_LIBS}
)

# macOS специфичные зависимости
if(MACOS)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COREVIDEO_LIBRARY CoreVideo)
    
    target_link_libraries(OpenGLSomethingFrameDisplayerEVO PRIVATE
        ${COCOA_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREVIDEO_LIBRARY}
        "-framework OpenGL"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework Cocoa"
    )
endif()

# Флаги компиляции
target_compile_options(OpenGLSomethingFrameDisplayerEVO PRIVATE
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:RelWithDebInfo>:-O3 -g -ffast-math>
    $<$<CONFIG:Release>:-O3 -DNDEBUG -ffast-math>
)

if(LINUX)
    target_compile_options(OpenGLSomethingFrameDisplayerEVO PRIVATE
        $<$<CONFIG:Release>:-march=native -mtune=native>
    )
endif()

# Установка (опционально, можно убрать если не нужно)
install(TARGETS OpenGLSomethingFrameDisplayerEVO
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# ★★★★ ФУНКЦИЯ ДЛЯ КОПИРОВАНИЯ РЕСУРСОВ ★★★★
function(copy_opengl_resources TARGET_NAME)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/res)
        add_custom_command(
            TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/res
                $<TARGET_FILE_DIR:${TARGET_NAME}>/res
            COMMENT "Copying OpenGLSomethingFrameDisplayerEVO resources"
            VERBATIM
        )
    else()
        message(WARNING "Resource directory not found: ${CMAKE_CURRENT_SOURCE_DIR}/res")
    endif()
endfunction()

# Экспортируем функцию в основной проект
set(copy_opengl_resources copy_opengl_resources PARENT_SCOPE)