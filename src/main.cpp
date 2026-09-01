#include <chrono>
#include <memory>
#include <thread>
#include <videoPlayer/media_player.h>
#include <iostream>
#include <filesystem>
#include "Globals.h"

namespace fs = std::filesystem;

#ifdef _WIN32
    #include <windows.h>
#elif defined(__linux__)
    #include <unistd.h>
    #include <limits.h>
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>
#endif

fs::path getExecutableDir() 
{
    #if defined(_WIN32)
        wchar_t path[MAX_PATH];
        if (GetModuleFileNameW(nullptr, path, MAX_PATH)) 
        {
            std::filesystem::path result = path;
            result = std::filesystem::weakly_canonical(result);
            result.remove_filename();
            return result;
        }
    #elif defined(__linux__)
        char path[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", path, sizeof(path) - 1);
        if (count != -1) 
        {
            path[count] = '\0';

            std::filesystem::path result = path;
            result = std::filesystem::weakly_canonical(result);
            result.remove_filename();
            return result;
        }
    #elif defined(__APPLE__)
        char path[PATH_MAX];
        uint32_t size = sizeof(path);
        if (_NSGetExecutablePath(path, &size) == 0) 
        {
            std::filesystem::path result = path;
            result = std::filesystem::weakly_canonical(result);
            result.remove_filename();
            return result;
        }
    #endif

    return fs::current_path();
}

void VideoPlayerFunc()
{
    fs::path exeDir = getExecutableDir();
    fs::current_path(exeDir);
    
    std::cout << "Working from: " << fs::current_path() << std::endl;
    
    std::string video_path;
    std::cout << "Enter video file path: " << std::endl;
    std::getline(std::cin, video_path);
    
    MediaPlayer player;
    if (!player.initialize(video_path))
    {
        return;
    }
    player.run();
    player.cleanup();
}

void printNvidiaEnv() {
    std::cout << "\n=== NVIDIA Environment Variables ===" << std::endl;

    // Список переменных, которые мы хотим проверить
    std::vector<std::string> env_vars = {
        "__NV_PRIME_RENDER_OFFLOAD",
        "__GLX_VENDOR_LIBRARY_NAME",
        "__NV_PRIME_RENDER_OFFLOAD_PROVIDER",
        "EGL_PLATFORM",
        "__GL_THREADED_OPTIMIZATIONS",
        "__GL_SYNC_TO_VBLANK",
        "vblank_mode",
        "LIBGL_DEBUG",
        "EGL_LOG_LEVEL",
        "MESA_DEBUG",
        "GLFW_DEBUG",
        "DISPLAY"
    };

    for (const auto& var : env_vars) {
        const char* val = std::getenv(var.c_str());
        if (val) {
            std::cout << var << " = " << val << std::endl;
        } else {
            std::cout << var << " = (not set)" << std::endl;
        }
    }
    std::cout << "=====================================\n" << std::endl;
}

int main()
{
    fs::path exeDir = getExecutableDir();
    fs::current_path(exeDir);
    
    std::cout << "Main working from: " << fs::current_path() << std::endl;
    printNvidiaEnv();
    GLobal::shouldStop = false;
    std::thread th([] {VideoPlayerFunc();});

    GLobal::frameDisplayer = std::make_unique<OpenGLSomethingFrameDisplayerEVO::OpenGLSomethingFrameDisplayerEVO>();
    GLobal::frameDisplayer->SetThreadCount(std::thread::hardware_concurrency() - 4);
    GLobal::frameDisplayer->WaitForSetVideoSize();
    GLobal::frameDisplayer->InitialiseGame(1300, 900);
    GLobal::frameDisplayer->Start();
    GLobal::shouldStop = true;
    th.join();
    return 0;
}