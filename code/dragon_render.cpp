#if 0
inline render_state InitRenderState(mem_arena* Arena, f32 AspectRatio, f32 Fov, f32 Near, f32 Far)
{
    render_state Result = {};
    
    u32 AllocSize = MegaBytes(4);
    u8* RenderData = (u8*)PushSize(Arena, AllocSize);
    Result.Arena = InitDoubleArena(RenderData, AllocSize);
    Result.ProjMat = PerspProjMat(AspectRatio, Fov, Near, Far);
    
    return Result;
}

inline void PushModel(render_state* RenderState, m4 WTransform, f32 SpecularIntensity, f32 SpecularPower,
                      asset_model* Model)
{
    for (u32 MeshId = 0; MeshId < Model->NumMeshes; ++MeshId)
    {
        asset_mesh* Mesh = Model->Meshes + MeshId;

        if (Mesh->NumVertices == 0)
        {
            continue;
        }
        
        model_data* ModelData = 0;
        u32* RenderCmdType = PushStruct(&RenderState->Arena, u32);
        *RenderCmdType = RenderCmdType_Mesh;
    
        ModelData = PushStruct(&RenderState->Arena, model_data);
        if (Mesh->Texture)
        {
            //ModelData->TextureId = Mesh->Texture->GLId;
            // TODO: Fix
            ModelData->TextureId = -1;
        }
        else
        {
            // NOTE: Notify OpenGL to use a white texture for this model
            ModelData->TextureId = -1;
        }

        // TODO: Fix
        //ModelData->VboId = Mesh->GLId;
        ModelData->NumVertices = Mesh->NumVertices;
        ModelData->WTransform = WTransform;
        ModelData->SpecularIntensity = SpecularIntensity;
        ModelData->SpecularPower = SpecularPower;
    }
}

inline void PushPointLight(render_state* RenderState, assets* Assets, point_light PointLight)
{
    u32* RenderCmdType = BotPushStruct(&RenderState->Arena, u32);
    *RenderCmdType = RenderCmdType_PointLight;

    point_light* PointLightData = BotPushStruct(&RenderState->Arena, point_light);
    *PointLightData = PointLight;
    asset_model* Sphere = GetModel(Assets, Model_Sphere);
    // TODO: Fix
    //PointLightData->GLSphereId = Sphere->Meshes[0].GLId;
    PointLightData->NumVertices = Sphere->Meshes[0].NumVertices;
}

inline void PushDirLight(render_state* RenderState, assets* Assets, dir_light DirLight)
{
    u32* RenderCmdType = BotPushStruct(&RenderState->Arena, u32);
    *RenderCmdType = RenderCmdType_DirLight;

    dir_light* DirLightData = BotPushStruct(&RenderState->Arena, dir_light);
    *DirLightData = DirLight;
}
#endif
