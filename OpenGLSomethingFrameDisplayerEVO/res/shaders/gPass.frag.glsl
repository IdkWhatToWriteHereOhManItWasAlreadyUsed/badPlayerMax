#version 330 core

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D blocksTexture;

layout (location = 0) out vec3 fragPos;
layout (location = 1) out vec3 normal;
layout (location = 2) out vec4 albedoSpec;  

void main()
{
    fragPos = FragPos;
    normal = normalize(Normal);    
    vec4 texColor = texture(blocksTexture, TexCoords);    
    albedoSpec.rgb = texColor.rgb;
    albedoSpec.a = 0.0f;
}