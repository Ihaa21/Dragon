#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(push_constant, std430) uniform vert_constants
{
    mat4 WTransform;
    mat4 WVPTransform;
} Const;

layout(location = 0) in vec3 InPos;
layout(location = 1) in vec2 InTexCoord;
layout(location = 2) in vec3 InNormal;

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(location = 0) out vec3 OutWorldPos;
layout(location = 1) out vec2 OutTexCoord;
layout(location = 2) out vec3 OutNormal;

void main()
{
    gl_Position = Const.WVPTransform*vec4(InPos, 1.0);
    OutWorldPos = (Const.WTransform*vec4(InPos, 1.0)).xyz;
    OutNormal = InNormal;
    OutTexCoord = InTexCoord;
}