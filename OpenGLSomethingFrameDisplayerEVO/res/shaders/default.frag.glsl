#version 330 core

struct DirectionalLight 
{
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight 
{
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

in vec2 TexCoords;
in float FogFactor;
in vec3 FragPos;
in vec3 Normal;
in vec3 ViewPos;

uniform vec4 fogColor;
uniform sampler2D blocksTexture; 
uniform DirectionalLight sunLight;
uniform PointLight tempPlayerLight;

out vec4 color;

vec3 CalcDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 diffuseColor, float specularIntensity)
{
    vec3 ambient = light.ambient * diffuseColor;
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * diffuseColor;
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = light.specular * spec * diffuseColor * specularIntensity;
    return ambient + diffuse + specular;
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    
    float diff = max(dot(normal, lightDir), 0.0);
    
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    
    float Distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * Distance + 
                             light.quadratic * (Distance * Distance));    
    
    vec3 ambient  = light.ambient  * vec3(0.8, 0.8, 0.8);
    vec3 diffuse  = light.diffuse  * diff * vec3(0.8, 0.8, 0.8);
    vec3 specular = light.specular * spec * vec3(0.3, 0.3, 0.3);
    
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
    
    return ambient + diffuse + specular;
}


void main()
{
    vec4 texColor = texture(blocksTexture, TexCoords);   
    
    if (texColor.a < 0.01)
        discard;
    
    vec3 norm = normalize(Normal);    

    vec3 viewDir = normalize(ViewPos - FragPos);
    
    float specularIntensity = 0.0;
    
    vec3 lighting = CalcDirectionalLight(sunLight, norm, viewDir, texColor.rgb, specularIntensity);
    vec3 playerLighting = CalcPointLight(tempPlayerLight, norm, FragPos, viewDir);
    
    vec3 finalLighting = lighting + playerLighting;
    
    vec3 finalRGB = finalLighting;
    
    color = vec4(finalRGB, texColor.a);
}