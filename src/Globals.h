#pragma once

#include "OpenGLSomethingFrameDisplayerEVO.h"
#include <memory>

class GLobal
{
public:
    static std::unique_ptr<OpenGLSomethingFrameDisplayerEVO::OpenGLSomethingFrameDisplayerEVO> frameDisplayer;
    static bool shouldStop;
};

