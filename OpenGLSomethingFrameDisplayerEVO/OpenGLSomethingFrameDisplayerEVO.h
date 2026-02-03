#pragma once

#include "videoCore/Game.h"
#include <condition_variable>
#include <mutex>
#include <random>
#include <cmath>
#include <vector>
#include <cstdint>

class OpenGLSomethingFrameDisplayerEVO
{
private:
    Game m_game;
    std::vector<std::vector<uint8_t>> m_frameHeightsBuffer {};
    ThreadPool* m_threadPool;
    int m_threadCount;
    int m_videoWidth;
    int m_videoHeight;
    
    std::condition_variable m_isVideoSizeSetCV {};
    bool m_isVideoSizeSet {false};
    std::condition_variable m_isGameInitedCV {};
    bool m_isGameInited {false};
    mutable std::mutex m_mtx {};
    
    std::mt19937 m_randomEngine;
    
    static const int PERMUTATION[512];
    
public:
    OpenGLSomethingFrameDisplayerEVO();
    ~OpenGLSomethingFrameDisplayerEVO();
    
    void SetThreadCount(int count);
    void SetVideoSize(int videoWidth, int videoHeight);
    void WaitForSetVideoSize();   
    void InitialiseGame(int screenWidth, int screenHeight);
    void WaitForGameInit();
    void Start();
    void DisplayFrame(uint8_t **data);
    
private:
    static constexpr int MAX_HEIGHT = 25;
    static constexpr int DOWNSCALE = 1;
    
    static constexpr float MIN_BRIGHTNESS_HEIGHT = 2.0f;
    static constexpr float MAX_BRIGHTNESS_HEIGHT = 4.0f;
    
    static constexpr float MOUNTAIN_NOISE_SCALE = 0.033f;
    static constexpr float VALLEY_NOISE_SCALE = 0.017f;
    static constexpr float MOUNTAIN_INTENSITY = 32.2f;
    static constexpr float VALLEY_INTENSITY = 5.0f;
    
    static constexpr float TRANSITION_SMOOTHNESS = 777.9f;
    
    void ProcessCompleteStrip(uint8_t* pixel_data, int stride, int stripIndex, 
                              int totalStrips, int startY, int endY, 
                              int threadIndex);
    
    float GeneratePerlinNoise(float x, float y);
    float Fade(float t);
    float Lerp(float a, float b, float t);
    float Grad(int hash, float x, float y, float z);
    
    float ApplyMountainNoise(float x, float y, float brightness);
    float ApplyValleyNoise(float x, float y, float brightness);
    float SmoothTransition(float t);
    
    uint8_t ClampHeight(float height);
};