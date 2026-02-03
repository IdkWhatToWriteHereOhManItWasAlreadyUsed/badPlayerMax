
#include "OpenGLSomethingFrameDisplayerEVO.h"
#include "GlobalThreadPool.h"
#include "ThreadPool/ThreadPool.h"
#include <mutex>
#include <algorithm>

const int OpenGLSomethingFrameDisplayerEVO::PERMUTATION[512] = {
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
    8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
    35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
    134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
    55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
    18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
    250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
    189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
    172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
    228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
    107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
    8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
    35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
    134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
    55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
    18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
    250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
    189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
    172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
    228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
    107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

OpenGLSomethingFrameDisplayerEVO::OpenGLSomethingFrameDisplayerEVO()
    : m_threadPool(&globalThreadPool)
    , m_threadCount(4)
    , m_randomEngine(std::random_device{}())
{
}

OpenGLSomethingFrameDisplayerEVO::~OpenGLSomethingFrameDisplayerEVO()
{
    m_threadPool->Shutdown();
}

void OpenGLSomethingFrameDisplayerEVO::SetThreadCount(int count)
{
    globalThreadPool.SetThreadCount(count);
    m_threadCount = count;
}

void OpenGLSomethingFrameDisplayerEVO::SetVideoSize(int videoWidth, int videoHeight)
{
    std::lock_guard<std::mutex> lock(m_mtx);
    m_videoWidth = (videoWidth - videoWidth % CHUNK_LENGTH) / DOWNSCALE;
    m_videoHeight = (videoHeight - videoHeight % CHUNK_LENGTH) / DOWNSCALE;
    m_frameHeightsBuffer.resize(m_videoHeight);
    
    for (auto& screenLine : m_frameHeightsBuffer) 
        screenLine.resize(m_videoWidth);
    
    m_isVideoSizeSet = true;
    m_isVideoSizeSetCV.notify_all();
}

void OpenGLSomethingFrameDisplayerEVO::WaitForSetVideoSize()
{
    std::unique_lock<std::mutex> lock(m_mtx);
    m_isVideoSizeSetCV.wait(lock, [this] { return m_isVideoSizeSet; });
}

void OpenGLSomethingFrameDisplayerEVO::InitialiseGame(int screenWidth, int screenHeight)
{
    std::lock_guard<std::mutex> lock(m_mtx);
    m_game.Initialise(screenWidth, screenHeight, m_videoWidth, m_videoHeight);
    m_isGameInited = true;
    m_isGameInitedCV.notify_all();   
}

void OpenGLSomethingFrameDisplayerEVO::WaitForGameInit()
{
    std::unique_lock<std::mutex> lock(m_mtx);
    m_isGameInitedCV.wait(lock, [this] { return m_isGameInited; });
}

void OpenGLSomethingFrameDisplayerEVO::Start()
{
    m_threadCount = m_threadPool->GetThreadCount();
    m_game.Run();
}

void OpenGLSomethingFrameDisplayerEVO::DisplayFrame(uint8_t **data)
{
    uint8_t* pixel_data = data[0];
    int stride = m_videoWidth * 3;
    
    int rowsPerStrip = m_videoHeight / m_threadCount;
    int extraRows = m_videoHeight % m_threadCount;
    
    int currentY = 0;
    for (int stripIndex = 0; stripIndex < m_threadCount; ++stripIndex)
    {
        int stripHeight = rowsPerStrip;
        if (stripIndex == m_threadCount - 1)
            stripHeight += extraRows;
        if (stripHeight == 0) continue;
        
        int endY = currentY + stripHeight;
        int threadIndex = stripIndex;
        
        auto task = std::make_shared<Task<void, uint8_t*, int, int, int, int, int, int>>(
            std::bind(&OpenGLSomethingFrameDisplayerEVO::ProcessCompleteStrip, this,
                     std::placeholders::_1, std::placeholders::_2,
                     std::placeholders::_3, std::placeholders::_4,
                     std::placeholders::_5, std::placeholders::_6,
                     std::placeholders::_7),
            pixel_data, stride, stripIndex, m_threadCount, 
            currentY, endY, threadIndex
        );
        
        m_threadPool->AddTask(task);
        currentY = endY;
    }
    
    m_threadPool->WaitAll();
    m_game.DisplayFrame(m_frameHeightsBuffer);
}

void OpenGLSomethingFrameDisplayerEVO::ProcessCompleteStrip(uint8_t* pixel_data, int stride, 
                                                        int stripIndex, int totalStrips,
                                                        int startY, int endY, 
                                                        int threadIndex)
{
    static float timeOffset = 0.0f;
    timeOffset += 0.001f;
    
    for (int y = startY; y < endY; y++) 
    {
        uint8_t* row_start = pixel_data + (y * DOWNSCALE * DOWNSCALE * stride);
        for (int x = 0; x < m_videoWidth; x++) 
        {
            uint8_t* pixel = row_start + (x * DOWNSCALE * 3);
            
            float brightness = (pixel[0] * 0.299f + 
                               pixel[1] * 0.587f + 
                               pixel[2] * 0.114f) / 255.0f;
            
            float baseHeight = MIN_BRIGHTNESS_HEIGHT + 
                             brightness * (MAX_BRIGHTNESS_HEIGHT - MIN_BRIGHTNESS_HEIGHT);
            
            float transition = SmoothTransition(brightness);
            
            float mountainEffect = ApplyMountainNoise(x + timeOffset, y + timeOffset, brightness);
            float valleyEffect = ApplyValleyNoise(x + timeOffset, y + timeOffset, brightness);
            
            float finalHeight = baseHeight + 
                              mountainEffect * transition + 
                              valleyEffect * (1.0f - transition);
            
            m_frameHeightsBuffer[y][x] = ClampHeight(finalHeight);
        }
    }
}

float OpenGLSomethingFrameDisplayerEVO::ApplyMountainNoise(float x, float y, float brightness)
{
    if (brightness < 0.5f) return 0.0f;
    
    float mountainFactor = (brightness - 0.5f) * 2.0f;
    float noise = GeneratePerlinNoise(x * MOUNTAIN_NOISE_SCALE, y * MOUNTAIN_NOISE_SCALE);
    
    return noise * mountainFactor * MOUNTAIN_INTENSITY;
}

float OpenGLSomethingFrameDisplayerEVO::ApplyValleyNoise(float x, float y, float brightness)
{
    if (brightness > 0.5f) return 0.0f;
    
    float valleyFactor = (0.5f - brightness) * 2.0f;
    float noise = GeneratePerlinNoise(x * VALLEY_NOISE_SCALE, y * VALLEY_NOISE_SCALE);
    
    return noise * valleyFactor * VALLEY_INTENSITY;
}

float OpenGLSomethingFrameDisplayerEVO::SmoothTransition(float t)
{
    float center = 0.5f;
    float width = 0.6f;
    
    float adjusted = (t - (center - width/2)) / width;
    adjusted = std::clamp(adjusted, 0.0f, 1.0f);
    
    return 0.5f * (1.0f - std::cos(adjusted * 3.1415926535f));
}

float OpenGLSomethingFrameDisplayerEVO::GeneratePerlinNoise(float x, float y)
{
    int xi = static_cast<int>(std::floor(x)) & 255;
    int yi = static_cast<int>(std::floor(y)) & 255;
    
    float xf = x - std::floor(x);
    float yf = y - std::floor(y);
    float zf = 0.0f;
    
    float u = Fade(xf);
    float v = Fade(yf);
    
    int a = PERMUTATION[xi] + yi;
    int aa = PERMUTATION[a];
    int ab = PERMUTATION[a + 1];
    int b = PERMUTATION[xi + 1] + yi;
    int ba = PERMUTATION[b];
    int bb = PERMUTATION[b + 1];
    
    float x1 = Lerp(Grad(PERMUTATION[aa], xf, yf, zf),
                    Grad(PERMUTATION[ba], xf - 1, yf, zf), u);
    float x2 = Lerp(Grad(PERMUTATION[ab], xf, yf - 1, zf),
                    Grad(PERMUTATION[bb], xf - 1, yf - 1, zf), u);
    
    return (Lerp(x1, x2, v) + 1.0f) * 0.5f;
}

float OpenGLSomethingFrameDisplayerEVO::Fade(float t)
{
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float OpenGLSomethingFrameDisplayerEVO::Lerp(float a, float b, float t)
{
    return a + t * (b - a);
}

float OpenGLSomethingFrameDisplayerEVO::Grad(int hash, float x, float y, float z)
{
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

uint8_t OpenGLSomethingFrameDisplayerEVO::ClampHeight(float height)
{
    return static_cast<uint8_t>(std::clamp(height, 0.0f, static_cast<float>(MAX_HEIGHT)));
}