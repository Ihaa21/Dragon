#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(push_constant) uniform push_constants
{
    mat4 WVPTransform;
} Const;

layout(location = 0) in vec3 InVertPos;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    gl_Position = Const.WVPTransform*vec4(InVertPos, 1.0);
}