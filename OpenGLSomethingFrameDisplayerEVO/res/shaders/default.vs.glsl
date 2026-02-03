#version 330 core

//#include "windFX.glsl"
vec3 applyWindFX(vec3 position, float windSpeed, float time)
{
    float baseFrequency = 2.0;
    float baseAmplitude = 0.03;
    float waveX = sin(time * windSpeed + position.z * baseFrequency) * baseAmplitude;
    float waveZ = sin(time * windSpeed + position.x * baseFrequency) * baseAmplitude;

    vec3 animatedPos = position;
    animatedPos.x += waveX;
    animatedPos.z += waveZ;

    return animatedPos;
}
//

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 viewPos;
uniform float fogDensity;
uniform float fogStart;
uniform float fogEnd;

uniform float time;
uniform float windSpeed;

out float FogFactor;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec3 ViewPos;

uniform int useWindFX = 0;

void main()
{
    vec3 animatedPos;
    if (useWindFX == 1)
        animatedPos = applyWindFX(position, windSpeed, time);
    else
        animatedPos = position;

    FragPos = vec3(model * vec4(animatedPos, 1.0f));   
    Normal = mat3(transpose(inverse(model))) * normal;  
    TexCoords = texCoord;
    ViewPos = viewPos;

    float fogFactor = (length(FragPos - viewPos) - fogStart) / (fogEnd - fogStart);
    FogFactor = clamp(fogFactor, 0.0, 1.0);
    
    TexCoords = texCoord;
    gl_Position = projection * view * model * vec4(animatedPos, 1.0f);
}