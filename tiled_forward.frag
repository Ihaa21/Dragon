#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define TILE_SIZE 4

struct point_light
{
    vec3 Pos;
    float Radius;

    vec3 Color;
    float AmbientIntensity;
    
    float DiffuseIntensity;
};

layout(push_constant) uniform frag_constants
{
    layout(offset = 128)
    float SpecularIntensity;
    float SpecularPower;
    uint ScreenX;
    uint ScreenY;

    uint NumTilesX;
    uint MaxLightsPerTile;
} Const;

layout(set = 0, binding = 0) uniform sampler2D DiffuseSampler;

layout(set = 1, binding = 0) readonly buffer point_light_array
{
    point_light PointLightArray[];
};

layout(set = 1, binding = 1) readonly buffer point_light_index_buffer
{
    uint PointLightIndexArray[];
};

layout(set = 1, binding = 2, r32i) uniform iimage2D DebugHeatMap;

layout(location = 0) in vec3 InWorldPos;
layout(location = 1) in vec2 InTexCoord;
layout(location = 2) in vec3 InWorldNormal;

layout(location = 0) out vec4 OutColor;

void main()
{
    vec4 Diffuse = texture(DiffuseSampler, InTexCoord).rgba;
    vec3 Normal = normalize(InWorldNormal);

    //
    // NOTE: Apply point lights on fragment
    //

    // NOTE: Get the tile we are in
    uvec2 PixelPos = uvec2(gl_FragCoord.xy);
    uvec2 TilePos = PixelPos / uvec2(TILE_SIZE, TILE_SIZE);
    uint TileId = TilePos.y*Const.NumTilesX + TilePos.x;
    uint BufferPtr = TileId*Const.MaxLightsPerTile;

    // TODO: Should be a input
    vec3 EyeWorldPos = vec3(0, 0, 0);

    uint NumLightsInTile = PointLightIndexArray[BufferPtr];
    vec3 MixedColor = vec3(0, 0, 0);
    //for (uint LightIndexId = 1; LightIndexId < 2; ++LightIndexId)
    for (uint LightIndexId = 1; LightIndexId < (NumLightsInTile + 1); ++LightIndexId)
    {
        uint LightId = PointLightIndexArray[BufferPtr + LightIndexId];
        point_light CurrLight = PointLightArray[LightId];

	vec3 LightToWorld = InWorldPos.xyz - CurrLight.Pos;
        float LightDist = length(LightToWorld);
        
        // NOTE: Calculate attenuation
        float Attenuation = 1.0 - (LightDist / CurrLight.Radius);
        float TestZ = step(0, CurrLight.Radius - LightDist);

        vec3 WorldToEye = normalize(EyeWorldPos - InWorldPos.xyz);
        vec3 LightReflectionDir = normalize(reflect(LightToWorld, Normal));

        float DiffuseFactor = dot(Normal, -LightToWorld);
        float SpecularFactor = pow(max(0.0, dot(WorldToEye, LightReflectionDir)), Const.SpecularPower);
	
	vec3 AmbientColor = CurrLight.AmbientIntensity*CurrLight.Color*Diffuse.rgb;
	vec3 DiffuseColor = DiffuseFactor*CurrLight.DiffuseIntensity*CurrLight.Color*Diffuse.rgb;
	vec3 SpecularColor = SpecularFactor*Const.SpecularIntensity*CurrLight.Color;
        
        MixedColor += TestZ*Attenuation*(AmbientColor + DiffuseColor + SpecularColor);
    }

    //
    // NOTE: Apply directional lights on fragment
    //

    OutColor = vec4(MixedColor, 1);
}
