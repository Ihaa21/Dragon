#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(input_attachment_index = 0, binding = 1) uniform subpassInput PosSampler;
layout(input_attachment_index = 1, binding = 2) uniform subpassInput DepthSampler;

layout(set=0, binding=0) uniform UniformBuffer
{
    mat4 VP;
    float SampleRadius;
    float Kernel[32];
};

layout(location = 0) out vec4 FragColor;

void main()
{
    vec3 Pos = subpassLoad(PosSampler).xyz;
    float AO = 0.0;
    
    for (int i = 0; i < 32; ++i)
    {
        vec3 SamplePos = Pos + Kernel[i];
        vec4 ProjSample = VP*vec4(SamplePos, 1); // NOTE: Project to near clipping plane
        ProjSample.xy /= ProjSample.w; // NOTE: Perspective divide
        ProjSample.xy = ProjSample.xy*0.5 + vec2(0.5); // NOTE: Transform to (0, 1)
        
        float SampleDepth = subpassLoad(DepthSampler).b;
        if (abs(Pos.z - SampleDepth) < SampleRadius)
        {
            AO += step(SampleDepth, SamplePos.z);
        }
    }

    AO = 1.0f - (AO/32.0);
    FragColor = vec4(pow(AO, 2.0));
}