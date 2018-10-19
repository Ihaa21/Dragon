#if !defined(FILE_ASSETS_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Ihor Szlachtycz $
   $Notice: (C) Copyright 2014 by Dream.Inc, Inc. All Rights Reserved. $
   ======================================================================== */

enum AssetFonts
{
    Font_General,

    Font_NumElements,
};

enum AssetModels
{
    Model_Cube,
    Model_Sphere,
    Model_Bunny,
    Model_Sponza,
    Model_Elephant,
    
    Model_NumElements,
};

enum AssetTextures
{
    Texture_SkyBox,

    Texture_NumElements,
};

struct file_glyph
{
    u32 Width;
    u32 Height;
    v2 AlignPos;
    u64 PixelOffset;
};

struct file_font
{
    // NOTE: Where assuming that the glyphs are in one range
    u32 MinGlyph;
    u32 MaxGlyph;

    // NOTE: Vertical font data    
    i32 MaxAscent;
    i32 MaxDescent;
    i32 LineGap;

    u64 StepArrayOffset;
    u64 KernArrayOffset;
    u64 GlyphArrayOffset;
};

struct file_texture
{
    u32 Width;
    u32 Height;
    v2 Min;
    v2 Dim;
    u64 PixelOffset;
};

struct file_vertex
{
    v3 Pos;
    v2 UV;
    v3 Normal;
};

struct file_mesh
{
    u32 NumVertices;
    i32 TextureId;
    u64 VertexArrayOffset;
};

struct file_model
{
    u32 NumTextures;
    u64 TextureArrayOffset;
    u32 NumMeshes;
    u64 MeshArrayOffset;
};

struct file_header
{
    u64 FontArrayPos;
    u64 FontCount;
        
    u64 ModelArrayPos;
    u64 ModelCount;
        
    u64 TextureArrayPos;
    u64 TextureCount;
};

#define FILE_ASSETS_H
#endif
