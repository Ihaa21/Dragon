#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

void main()
{
    vec2 Pos[6] = vec2[6](vec2(1, -1), vec2(1, 1), vec2(-1, 1), 
                          vec2(1, -1), vec2(-1, -1), vec2(-1, 1));
    gl_Position = vec4(Pos[gl_VertexIndex], 0, 1);
}