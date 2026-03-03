#include "video_decoder.h"
#include "shared_data.h"

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

extern "C"
{
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Globals.h"

const char* vertex_shader_src = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
out vec2 TexCoord;
void main() 
{
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

const char* fragment_shader_src = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D ourTexture;
void main() 
{
    FragColor = texture(ourTexture, TexCoord);
}
)";

GLuint create_shader_program();

void decode_video(AVCodecContext* video_codec_ctx, AVRational video_time_base,
    std::shared_ptr<SharedData> shared)
{
    
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return;
    }
    
    GLFWwindow* window = glfwCreateWindow(video_codec_ctx->width, video_codec_ctx->height,
        "ergtrshsegfa", nullptr, nullptr);
        
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }
    
    glfwMakeContextCurrent(window);
    
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return;
    }
    
    GLuint shader_program = create_shader_program();
    
    float vertices[] = {1.0f,  1.0f,  1.0f, 0.0f, 1.0f,  -1.0f, 1.0f, 1.0f,
                       -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 1.0f,  0.0f, 0.0f};
                       
    unsigned int indices[] = {0, 1, 3, 1, 2, 3};
    
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
        (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    SwsContext* sws_ctx = sws_getContext(
        video_codec_ctx->width, video_codec_ctx->height, video_codec_ctx->pix_fmt,
        video_codec_ctx->width, video_codec_ctx->height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr);
        
    if (!sws_ctx)
    {
        std::cerr << "Failed to create sws context" << std::endl;
        /*
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteTextures(1, &texture);
        glDeleteProgram(shader_program);
        glfwDestroyWindow(window);
        glfwTerminate();*/
        return;
    }
    
    AVFrame* frame = av_frame_alloc();
    if (!frame)
    {
        std::cerr << "Failed to allocate video frame" << std::endl;
        sws_freeContext(sws_ctx);
        /*glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteTextures(1, &texture);
        glDeleteProgram(shader_program);
        glfwDestroyWindow(window);
        glfwTerminate();*/
        return;
    }
    

    
    double last_video_time = 0.0;
    int frames_displayed = 0;
    int frames_dropped = 0;

    auto last_fps_time = std::chrono::steady_clock::now();
    int frames_in_second = 0;

    while (shared->video_running && !GLobal::shouldStop)
    {
        std::shared_ptr<AVPacket> packet;
        
        {
            std::unique_lock<std::mutex> lock(shared->packet_mutex);
            shared->packet_cv.wait(lock, [shared]()
            {
                return !shared->video_packets.empty() || !shared->demuxer_running;
            });
            
            if (!shared->video_running || (!shared->demuxer_running && shared->video_packets.empty()))
            {
                break;
            }
            
            if (!shared->video_packets.empty())
            {
                packet = shared->video_packets.front();
                shared->video_packets.pop();
                shared->packet_cv.notify_one();
            }
        }
        
        if (!packet)
        {
            avcodec_send_packet(video_codec_ctx, nullptr);
        }
        else
        {
            int ret = avcodec_send_packet(video_codec_ctx, packet.get());
            if (ret < 0)
            {
                std::cerr << "Error sending packet to video decoder: " << ret
                    << std::endl;
                continue;
            }
        }
        
        while (true)
        {
            int ret = avcodec_receive_frame(video_codec_ctx, frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            {
                break;
            }
            else if (ret < 0)
            {
                std::cerr << "Error during video decoding: " << ret << std::endl;
                break;
            }
            
            double audio_time = shared->audio_clock.get_time();
            double video_time = frame->pts * av_q2d(video_time_base);
            
            if (frames_displayed == 0)
            {
                last_video_time = video_time;
                last_fps_time = std::chrono::steady_clock::now();
            }
            
            double diff = video_time - audio_time;
            
            if (diff > 0.1)
            {
                int64_t sleep_us = static_cast<int64_t>(diff * 1000000 - 50000);
                if (sleep_us > 0)
                {
                    std::this_thread::sleep_for(std::chrono::microseconds(sleep_us));
                }
                
                audio_time = shared->audio_clock.get_time();
                diff = video_time - audio_time;
            }
            
            if (std::abs(diff) < 0.1)
            {
                int buffer_size = av_image_get_buffer_size(AV_PIX_FMT_RGB24, video_codec_ctx->width,
                    video_codec_ctx->height, 1);
                uint8_t* buffer = (uint8_t*)av_malloc(buffer_size);
                
                if (!buffer)
                {
                    std::cerr << "Failed to allocate image buffer" << std::endl;
                    av_frame_unref(frame);
                    continue;
                }
                
                AVFrame* rgb_frame = av_frame_alloc();
                av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize, buffer,
                    AV_PIX_FMT_RGB24, video_codec_ctx->width,
                    video_codec_ctx->height, 1);
                    
                sws_scale(sws_ctx, frame->data, frame->linesize, 0,
                    video_codec_ctx->height, rgb_frame->data,
                    rgb_frame->linesize);

                GLobal::frameDisplayer->DisplayFrame(rgb_frame->data[0]);

                
                glBindTexture(GL_TEXTURE_2D, texture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, video_codec_ctx->width,
                    video_codec_ctx->height, 0, GL_RGB, GL_UNSIGNED_BYTE,
                    rgb_frame->data[0]);
                    
                glClear(GL_COLOR_BUFFER_BIT);
                glUseProgram(shader_program);
                glBindVertexArray(VAO);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                
                glfwSwapBuffers(window);
                glfwPollEvents();
                
                
                frames_displayed++;
                last_video_time = video_time;
                
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_fps_time).count();
                
                if (elapsed >= 1000)
                {
                    double fps = frames_in_second * 1000.0 / elapsed;
                    std::cout << "Video FPS: " << fps 
                            << ", Frames: " << frames_displayed 
                            << ", Dropped: " << frames_dropped << std::endl;
                    last_fps_time = now;
                    frames_in_second = 0;
                }
                frames_in_second++;
                
                av_frame_free(&rgb_frame);
                av_free(buffer);
            }
            else if (diff < -0.1)
            {
                frames_dropped++;
            }
            
            av_frame_unref(frame);
        }
    }
    
    std::cout << "Video playback finished." << std::endl;
    std::cout << "Total frames displayed: " << frames_displayed << std::endl;
    std::cout << "Total frames dropped: " << frames_dropped << std::endl;
    
    av_frame_free(&frame);
    sws_freeContext(sws_ctx);
    
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteTextures(1, &texture);
    glDeleteProgram(shader_program);
    
    glfwDestroyWindow(window);
    glfwTerminate();
}

GLuint create_shader_program()
{
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_src, nullptr);
    glCompileShader(vertex_shader);
    
    GLint success;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char info_log[512];
        glGetShaderInfoLog(vertex_shader, 512, nullptr, info_log);
        std::cerr << "Vertex shader compilation failed: " << info_log << std::endl;
    }
    
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_src, nullptr);
    glCompileShader(fragment_shader);
    
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char info_log[512];
        glGetShaderInfoLog(fragment_shader, 512, nullptr, info_log);
        std::cerr << "Fragment shader compilation failed: " << info_log
            << std::endl;
    }
    
    GLuint shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);
    
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char info_log[512];
        glGetProgramInfoLog(shader_program, 512, nullptr, info_log);
        std::cerr << "Shader program linking failed: " << info_log << std::endl;
    }
    
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    return shader_program;
}