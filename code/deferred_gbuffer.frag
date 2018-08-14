#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set=0, binding=0) uniform sampler2D Texture;

layout(set=0, binding=2) uniform UniformBuffer 
{
    float SpecularIntensity;
    float SpecularPower;
};

layout(location = 0) in vec3 InWorldPos;
layout(location = 1) in vec3 InWorldNormal;
layout(location = 2) in vec2 InTexCoord;

layout(location = 0) out vec4 OutWorldPos;
layout(location = 1) out vec4 OutDiffuse;
layout(location = 2) out vec3 OutWorldNormal;

void main()
{
    OutWorldPos = vec4(InWorldPos, SpecularIntensity);
    OutDiffuse = texture(Texture, InTexCoord);
    OutDiffuse.a = SpecularPower;
    OutWorldNormal = normalize(InWorldNormal);
}