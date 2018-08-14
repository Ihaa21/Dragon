#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set=0, binding=1) uniform UniformBuffer 
{
    mat4 WTransform;
    mat4 WVPTransform;
};

layout(location = 0) in vec3 InVertPos;
layout(location = 1) in vec2 InVertTexCoord;
layout(location = 2) in vec3 InVertNormal;

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(location = 0) out vec3 OutWorldPos;
layout(location = 1) out vec3 OutWorldNormal;
layout(location = 2) out vec2 OutTexCoord;

void main()
{
    vec4 TempPos = WVPTransform*vec4(InVertPos, 1.0);
    gl_Position = TempPos;
    OutWorldPos = (WTransform*vec4(InVertPos, 1.0)).xyz;
    OutWorldNormal = (WTransform*vec4(InVertNormal, 0)).xyz;
    OutTexCoord = InVertTexCoord;
}