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

out vec4 FragColor;
in vec2 TexCoords;
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform vec3 viewPos;
uniform DirectionalLight sunLight;
uniform PointLight tempPlayerLight;

vec3 CalcDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 diffuseColor, float specularIntensity)
{
    vec3 ambient = light.ambient * diffuseColor;
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * diffuseColor;
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = light.specular * spec * diffuseColor;
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
    vec2 screenPos = gl_FragCoord.xy / vec2(800.0, 600.0);
    
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Albedo = texture(gAlbedoSpec, TexCoords).rgb;
    float Specular = texture(gAlbedoSpec, TexCoords).a;
    
    if(length(Normal) < 0.001)
    {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
        
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
        
    vec3 lighting = CalcDirectionalLight(sunLight, norm, viewDir, Albedo, Specular);
    vec3 playerLighting = CalcPointLight(tempPlayerLight, norm, FragPos, viewDir);
        
    vec3 result = lighting + playerLighting;
        
    // Отображаем результат с тестовым светом
    FragColor = vec4(result, 1.0);
}