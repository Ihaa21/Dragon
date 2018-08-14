#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(input_attachment_index = 0, binding = 2) uniform subpassInput PosSampler;
layout(input_attachment_index = 1, binding = 3) uniform subpassInput DiffuseSampler;
layout(input_attachment_index = 2, binding = 4) uniform subpassInput NormalSampler;

layout(set=0, binding=1) uniform UniformBuffer
{
    vec3 LightColor;
    float AmbientIntensity;

    vec3 LightDir;
    float DiffuseIntensity;

    vec3 EyeWorldPos;
};

layout(location = 0) out vec4 PixelColor;

vec3 CalcLightColor(vec3 WorldPos, vec3 Normal, vec3 LightDir, float AmbientIntensity, 
                    float DiffuseIntensity, float SpecularIntensity, float SpecularPower, 
                    vec3 LightColor)
{
    vec3 Result;

    // NOTE: Calculate the lighting for this point
    float DiffuseFactor = dot(Normal, -LightDir);
    vec3 DiffuseColor = vec3(0, 0, 0);
    vec3 SpecularColor = vec3(0, 0, 0);
    if (DiffuseFactor > 0)
    {
        DiffuseColor = vec3(LightColor*DiffuseIntensity*DiffuseFactor);

        vec3 VertexToEye = normalize(EyeWorldPos - WorldPos);
        vec3 LightReflectionDir = normalize(reflect(LightDir, Normal));
        float SpecularFactor = dot(VertexToEye, LightReflectionDir);
        if (SpecularFactor > 0)
        {
            SpecularFactor = pow(SpecularFactor, SpecularPower);
            SpecularColor = vec3(LightColor * SpecularIntensity * SpecularFactor);
        }
    }

    Result = AmbientIntensity*LightColor + DiffuseColor + SpecularColor;
    return Result;
}

void main()
{
    vec4 WorldPos = subpassLoad(PosSampler).rgba;
    vec4 Diffuse = subpassLoad(DiffuseSampler).rgba;
    vec3 Normal = normalize(subpassLoad(NormalSampler).rgb);

    float SpecularIntensity = WorldPos.a;
    float SpecularPower = Diffuse.a;

    // NOTE: Calc specular color properties
    vec3 VertexToEye = normalize(EyeWorldPos - WorldPos.xyz);
    vec3 LightReflectionDir = normalize(reflect(LightDir, Normal));
    float SpecularFactor = max(0.0, dot(VertexToEye, LightReflectionDir));
    SpecularFactor = pow(SpecularFactor, SpecularPower);
    
    vec3 AmbientColor = AmbientIntensity*LightColor*Diffuse.rgb;
    vec3 DiffuseColor = DiffuseIntensity*LightColor*Diffuse.rgb*dot(Normal, -LightDir);
    vec3 SpecularColor = SpecularIntensity*LightColor*SpecularFactor;

    PixelColor = vec4(AmbientColor + DiffuseColor + SpecularColor, 1.0);
}
