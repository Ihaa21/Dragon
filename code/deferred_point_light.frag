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

    vec3 LightPos;
    float DiffuseIntensity;

    vec3 EyeWorldPos;
    float LightRadius;
};

layout(location = 0) out vec4 PixelColor;

void main()
{
    vec4 WorldPos = subpassLoad(PosSampler).rgba;
    vec4 Diffuse = subpassLoad(DiffuseSampler).rgba;
    vec3 Normal = normalize(subpassLoad(NormalSampler).rgb);

    float SpecularIntensity = WorldPos.a;
    float SpecularPower = Diffuse.a;

    vec3 LightDir = WorldPos.xyz - LightPos;
    float LightDist = length(LightDir);
    
    // NOTE: If point too far from light, don't light it
    float ZTest = step(0.0, LightRadius - LightDist);

    // NOTE: Calculate light attenuation
    float Attenuation = 1.0 - (LightDist / (LightRadius));

    // NOTE: Calc specular color properties
    vec3 VertexToEye = normalize(EyeWorldPos - WorldPos.xyz);
    vec3 LightReflectionDir = normalize(reflect(LightDir, Normal));
    float SpecularFactor = max(0.0, dot(VertexToEye, LightReflectionDir));
    SpecularFactor = pow(SpecularFactor, SpecularPower);
    
    vec3 AmbientColor = AmbientIntensity*LightColor*Diffuse.rgb;
    vec3 DiffuseColor = DiffuseIntensity*LightColor*Diffuse.rgb*dot(Normal, -LightDir);
    vec3 SpecularColor = SpecularIntensity*LightColor*SpecularFactor;

    PixelColor.rgb = AmbientColor + DiffuseColor + SpecularColor;
    PixelColor.rgb *= ZTest*Attenuation;
    PixelColor.a = 1.0;
}