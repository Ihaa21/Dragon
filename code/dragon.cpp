#if 0
void RenderWorld(render_state* RenderState, assets* Assets)
{
    f32 SpecularIntensity = 1.0f;
    f32 SpecularPower = 16.0f;

    // NOTE: Bunny + 175 lights
#if 0
    {
#if 0
        point_light PointLight = {};
        {
            PointLight.Color = V3(1, 1, 1);
            PointLight.DiffuseIntensity = 0.89f;
            PointLight.ConstantAttenuation = 0.0f;
            PointLight.LinearAttenuation = 0.0f;
            PointLight.ExpAttenuation = 6.3f;
            PointLight.Pos = V3(0, 0, 6);
        }

        for (u32 LightZ = 0; LightZ < 7; ++LightZ)
        {
            for (u32 LightY = 0; LightY < 5; ++LightY)
            {
                for (u32 LightX = 0; LightX < 5; ++LightX)
                {
                    PointLight.Pos.x = 2.5f - LightX;
                    PointLight.Pos.y = 2.5f - LightY;
                    PointLight.Pos.z = 7.0f - LightZ;

                    PointLight.Color = V3(LightX / 5.0f, LightY / 5.0f, LightZ / 4.0f);
                    
                    PushPointLight(RenderState, Assets, PointLight);
                }
            }
        }
#endif

        dir_light DirLight = {};
        {
            DirLight.Color = V3(0.6, 0.5, 0.8);
            DirLight.AmbientIntensity = 0.0f;
            DirLight.DiffuseIntensity = 0.0f;
            DirLight.Dir = Normalize(V3(0, 0, 1));
        }

        PushDirLight(RenderState, Assets, DirLight);

        local_global f32 SinT = 0.0f;
        SinT += 0.005f;
        if (SinT > 2.0f*Pi32)
        {
            SinT = 0.0f;
        }

        for (u32 ModelZ = 0; ModelZ < 3; ++ModelZ)
        {
            for (u32 ModelY = 0; ModelY < 2; ++ModelY)
            {
                for (u32 ModelX = 0; ModelX < 4; ++ModelX)
                {
                    v3 ModelPos = 2.0f*V3(1.6f - ModelX, 0.5f - ModelY, ModelZ + 1.6f);
                    m4 ModelMat = PosMat(ModelPos.x, ModelPos.y, ModelPos.z)*RotMat(SinT, SinT, SinT)*ScaleMat(0.45, 0.45, 0.45);
                    asset_model* Model = GetModel(Assets, Model_Bunny);
                    PushModel(RenderState, ModelMat, SpecularIntensity, SpecularPower, Model);
                }
            }
        }
    }
    
    m4 ModelMat = PosMat(0, -10, 8)*ScaleMat(3, 3, 3);
    asset_model* Model = GetModel(Assets, Model_Bunny);
    PushModel(RenderState, ModelMat, 0, 0, Model);
#elif 0
    {
        // NOTE: Statues
        dir_light DirLight = {};
        {
            DirLight.Color = V3(0.6, 0.5, 0.8);
            DirLight.AmbientIntensity = 0.0f;
            DirLight.DiffuseIntensity = 0.0f;
            DirLight.Dir = Normalize(V3(0, 0, 1));
        }

        PushDirLight(RenderState, Assets, DirLight);

        point_light PointLight = {};
        {
            PointLight.Color = V3(1, 1, 1);
            PointLight.DiffuseIntensity = 7.89f;
            PointLight.ConstantAttenuation = 0.0f;
            PointLight.LinearAttenuation = 0.0f;
            PointLight.ExpAttenuation = 100.3f;
            PointLight.Pos = V3(0, 0, 0);
        }

        v3 LightColors[] =
        {
            V3(1, 0, 0),
            V3(0, 1, 0),
            V3(0, 0, 1),
            V3(0.5, 0.5, 0),
            V3(0.0, 0.5, 0.5),
            V3(0.5, 0.0, 0.5),
            V3(1, 0.0, 0.5),
            V3(0.5, 0.0, 1),
            V3(1, 0.5, 0),
            V3(0.5, 1, 0),
        };

        u32 LightIndex = 0;
        for (u32 ModelZ = 0; ModelZ < 3; ++ModelZ)
        {
            for (u32 ModelY = 0; ModelY < 1; ++ModelY)
            {
                for (u32 ModelX = 0; ModelX < 4; ++ModelX)
                {
                    v3 ModelPos = 3.0f*V3(1.6f - ModelX, 0.5f - ModelY, ModelZ + 1.6f);

                    PointLight.Pos = ModelPos - V3(1, 1, 0);
                    PointLight.Color = LightColors[LightIndex];
                    PushPointLight(RenderState, Assets, PointLight);

                    LightIndex = (LightIndex + 1) % ArrayCount(LightColors);
                    
                    PointLight.Pos = ModelPos - V3(-1, 0, 0);
                    PointLight.Color = LightColors[LightIndex];
                    PushPointLight(RenderState, Assets, PointLight);

                    LightIndex = (LightIndex + 1) % ArrayCount(LightColors);

                    PointLight.Pos = ModelPos - V3(1, 1, -1);
                    PointLight.Color = LightColors[LightIndex];
                    PushPointLight(RenderState, Assets, PointLight);

                    LightIndex = (LightIndex + 1) % ArrayCount(LightColors);

                    PointLight.Pos = ModelPos - V3(-1, 0, 1);
                    PointLight.Color = LightColors[LightIndex];
                    PushPointLight(RenderState, Assets, PointLight);

                    LightIndex = (LightIndex + 1) % ArrayCount(LightColors);
                    
                    m4 ModelMat = PosMat(ModelPos.x, ModelPos.y, ModelPos.z)*RotMat(0, 0, 0)*ScaleMat(0.0003, 0.0003, 0.0003);
                    asset_model* Model = GetModel(Assets, Model_Elephant);
                    PushModel(RenderState, ModelMat, SpecularIntensity, SpecularPower, Model);
                }
            }
        }
    }
#else
    // NOTE: Sponza
    {
        dir_light DirLight = {};
        {
            DirLight.Color = V3(1, 1, 1);
            DirLight.AmbientIntensity = 0.1f;
            DirLight.DiffuseIntensity = 0.4f;
            DirLight.Dir = Normalize(V3(0, 0, 1));
        }

        PushDirLight(RenderState, Assets, DirLight);

        point_light PointLight = {};
        {
            PointLight.Color = V3(1, 0, 0);
            PointLight.AmbientIntensity = 0.1f;
            PointLight.DiffuseIntensity = 15.89f;
            PointLight.ConstantAttenuation = 0.0f;
            PointLight.LinearAttenuation = 0.5f;
            PointLight.ExpAttenuation = 100.3f;
            PointLight.Pos = V3(0, 0, 0);
        }

        for (u32 LightZ = 0; LightZ < 5; ++LightZ)
        {
            for (u32 LightY = 0; LightY < 5; ++LightY)
            {
                for (u32 LightX = 0; LightX < 5; ++LightX)
                {
                    PointLight.Pos.x = 2.0f*(f32)LightX;
                    PointLight.Pos.y = (f32)LightY;
                    PointLight.Pos.z = (f32)LightZ;

                    PointLight.Color = V3(LightX / 5.0f, LightY / 5.0f, LightZ / 4.0f);
                    
                    PushPointLight(RenderState, Assets, PointLight);
                }
            }
        }

        f32 Rotation = 1.5f*Pi32;

        v3 ModelPos = V3(0, 0, 2.2f);
        m4 ModelMat = PosMat(ModelPos.x, ModelPos.y, ModelPos.z)*RotMat(Rotation, 0, 0)*ScaleMat(0.015, 0.015, 0.015);
        asset_model* Sponza = GetModel(Assets, Model_Sponza);
        PushModel(RenderState, ModelMat, SpecularIntensity, SpecularPower, Sponza);
    }
#endif
}
#endif
