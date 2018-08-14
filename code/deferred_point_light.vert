#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set=0, binding=0) uniform UniformBuffer
{
    mat4 WVPTransform;
};

layout(location = 0) in vec3 VertPos;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    gl_Position = WVPTransform*vec4(VertPos, 1);
}