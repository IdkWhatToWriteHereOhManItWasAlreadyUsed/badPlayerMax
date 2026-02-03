#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform int useWindFX = 0;
uniform float windSpeed;
uniform float time;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

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


void main()
{    
    if (useWindFX == 1)
        FragPos = vec3(model * vec4(position, 1.0f)); 
    else
        FragPos = vec3(model * vec4(applyWindFX(position, windSpeed, time), 1.0f)); 
    Normal = mat3(transpose(inverse(model))) * normal;  
    TexCoords = texCoord;


    gl_Position = projection * view * model * vec4(position, 1.0f);
}