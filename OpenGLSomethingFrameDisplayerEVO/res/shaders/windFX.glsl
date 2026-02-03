

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