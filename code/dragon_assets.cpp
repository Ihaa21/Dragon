internal assets LoadAssets(u8* Mem, mem_arena* Arena)
{
    assets Assets = {};
    file_header* Header = (file_header*)Mem;
    Assets.FontCount = Header->FontCount;
    Assets.ModelCount = Header->ModelCount;
    Assets.TextureCount = Header->TextureCount;
    
    Assets.Fonts = PushArray(Arena, asset_font, Assets.FontCount);
    Assets.Models = PushArray(Arena, asset_model, Assets.ModelCount);
    Assets.Textures = PushArray(Arena, asset_texture, Assets.TextureCount);
    
    file_font* FileFonts = (file_font*)(Mem + Header->FontArrayPos);
    file_model* FileModels = (file_model*)(Mem + Header->ModelArrayPos);
    file_texture* FileTextures = (file_texture*)(Mem + Header->TextureArrayPos);

#if 0
    for (u64 FontIndex = 0; FontIndex < Assets.FontCount; ++FontIndex)
    {
        asset_font* AssetFont = Assets.Fonts + FontIndex;
        file_font FileFont = FileFonts[FontIndex];
        u32 NumGlyphsToLoad = FileFont.MaxGlyph - FileFont.MinGlyph;
        
        AssetFont->MinGlyph = FileFont.MinGlyph;
        AssetFont->MaxGlyph = FileFont.MaxGlyph;
        AssetFont->MaxAscent = FileFont.MaxAscent;
        AssetFont->MaxDescent = FileFont.MaxDescent;
        AssetFont->LineGap = FileFont.LineGap;
        
        // NOTE: Copy step array
        {
            u64 MemSize = FileFont.KernArrayOffset - FileFont.StepArrayOffset;
            AssetFont->StepArray = (f32*)PushSize(Arena, MemSize);
            Copy(Mem + FileFont.StepArrayOffset, AssetFont->StepArray, MemSize);
        }

        // NOTE: Copy kern array
        {
            u64 MemSize = FileFont.GlyphArrayOffset - FileFont.KernArrayOffset;
            AssetFont->KernArray = (f32*)PushSize(Arena, MemSize);
            Copy(Mem + FileFont.KernArrayOffset, AssetFont->KernArray, MemSize);
        }

        // NOTE: Copy glyph array
        {
            AssetFont->GlyphArray = PushArray(Arena, asset_glyph, NumGlyphsToLoad);

            file_glyph* FileGlyphArray = (file_glyph*)(Mem + FileFont.GlyphArrayOffset);
            for (u32 GlyphIndex = 0; GlyphIndex < NumGlyphsToLoad; ++GlyphIndex)
            {
                WorkLoadGlyph();
                
                file_glyph FileGlyph = FileGlyphArray[GlyphIndex];
                asset_glyph AssetGlyph = {};
                AssetGlyph.Width = FileGlyph.Width;
                AssetGlyph.Height = FileGlyph.Height;
                AssetGlyph.Min = V2(0, 0);
                AssetGlyph.Dim = V2(1, 1);
                AssetGlyph.AlignPos = FileGlyph.AlignPos;

                CreateImage(FileGlyph.Width, FileGlyph.Height, VK_FORMAT_R8G8B8A8_UNORM,
                            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                            VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, &AssetGlyph.Handle,
                            &AssetGlyph.View, &AssetGlyph.Memory);
                MoveImageToGpuMemory(GlobalState.PrimaryCmdBuffer, sizeof(u32)*FileGlyph.Width*FileGlyph.Height, FileGlyph.Width,
                                     FileGlyph.Height, Mem + FileGlyph.PixelOffset, AssetGlyph.Handle);

                AssetFont->GlyphArray[GlyphIndex] = AssetGlyph;
            }
        }
    }

    for (u64 ModelIndex = 0; ModelIndex < Assets.ModelCount; ++ModelIndex)
    {
        asset_model* Model = Assets.Models + ModelIndex;
        file_model FileModel = FileModels[ModelIndex];

        Model->NumTextures = FileModel.NumTextures;
        if (Model->NumTextures == 0)
        {
            Model->TextureArray = 0;
        }
        else
        {
            Model->TextureArray = PushArray(Arena, asset_texture, FileModel.NumTextures);
        }

        // NOTE: Load the textures
        for (u32 TextureId = 0; TextureId < FileModel.NumTextures; ++TextureId)
        {
            asset_texture* Texture = Model->TextureArray + TextureId;
            file_texture* FileTexture = (file_texture*)(Mem + FileModel.TextureArrayOffset);
            FileTexture += TextureId;

            WorkLoadTexture();
            
            Texture->MinUV = V2(0, 0);
            Texture->DimUV = V2(1, 1);
            Texture->AspectRatio = (f32)FileTexture->Width / (f32)FileTexture->Height;

            CreateImage(FileTexture->Width, FileTexture->Height, VK_FORMAT_R8G8B8A8_UNORM,
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, &Texture->Handle,
                        &Texture->View, &Texture->Memory);
            MoveImageToGpuMemory(GlobalState.PrimaryCmdBuffer, sizeof(u32)*FileTexture->Width*FileTexture->Height, FileTexture->Width,
                                 FileTexture->Height, Mem + FileTexture->PixelOffset, Texture->Handle);
        }
        
        Model->NumMeshes = FileModel.NumMeshes;
        Model->Meshes = PushArray(Arena, asset_mesh, 1000);

        // NOTE: Load the meshes
        file_mesh* FileMesh = (file_mesh*)(Mem + FileModel.MeshArrayOffset);
        for (u32 MeshId = 0; MeshId < Model->NumMeshes; ++MeshId)
        {
            asset_mesh* Mesh = Model->Meshes + MeshId;

            WorkLoadMesh();
            
            Mesh->NumVertices = FileMesh->NumVertices;
            if (FileMesh->NumVertices != 0)
            {
                u32 VertexBufferSize = FileMesh->NumVertices*VERTEX_SIZE;
                CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VertexBufferSize,
                             &Mesh->Handle, &Mesh->Memory);

                MoveBufferToGpuMemory(GlobalState.PrimaryCmdBuffer, Mesh->Handle, Mem + FileMesh->VertexArrayOffset,
                                      VertexBufferSize);
            }

            Mesh->Texture = Model->TextureArray + FileMesh->TextureId;

            FileMesh += 1;
        }
    }
#endif
    
    for (u64 TextureIndex = 0; TextureIndex < Assets.TextureCount; ++TextureIndex)
    {
        asset_texture* Texture = Assets.Textures + TextureIndex;

        // NOTE: We check that this isnt a sub img
        if (Texture->Handle == VK_NULL_HANDLE)
        {
            load_texture_work Work = {};
            Work.Width = FileTexture.Width;
            Work.Height = FileTexture.Height;
            Work.Format = VK_FORMAT_R8G8B8A8_UNORM;
            Work.Usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            Work.Layout = VK_IMAGE_LAYOUT_UNDEFINED;
            Work.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
            Work.Texture = Texture;

            WorkLoadTexture();

            MoveImageToGpuMemory(GlobalState.PrimaryCmdBuffer, sizeof(u32)*FileTexture.Width*FileTexture.Height, FileTexture.Width,
                                 FileTexture.Height, Mem + FileTexture.PixelOffset, Texture->Handle);
        }
    }

#if 0
    // TODO: Have a cubemap asset?
    // NOTE: Force load a cubemap
    {
        file_texture FileTexture = FileTextures[0];

        u32 Width[6] =
        {
            FileTexture.Width,
            FileTexture.Width,
            FileTexture.Width,
            FileTexture.Width,
            FileTexture.Width,
            FileTexture.Width, 
        };

        u32 Height[6] =
        {
            FileTexture.Height,
            FileTexture.Height,
            FileTexture.Height,
            FileTexture.Height,
            FileTexture.Height,
            FileTexture.Height, 
        };

        void* Pixels[6] =
        {
            Mem + FileTexture.PixelOffset,
            Mem + FileTexture.PixelOffset,
            Mem + FileTexture.PixelOffset,
            Mem + FileTexture.PixelOffset,
            Mem + FileTexture.PixelOffset,
            Mem + FileTexture.PixelOffset,
        };

        //*CubeMapTextureId = PlatformApi.GLLoadCubeMapToGpu(Width, Height, Pixels);
    }
#endif
    
    return Assets;
}
