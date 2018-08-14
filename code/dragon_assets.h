#if !defined(DRAGON_ASSETS_H)

struct asset_glyph
{
    u32 Width;
    u32 Height;
    v2 Min;
    v2 Dim;
    v2 AlignPos;
    // NOTE: This is the atlas id if we are a sub img
    VkImage Handle;
    VkImageView View;
    VkDeviceMemory Memory;
};

struct asset_font
{
    // NOTE: Where assuming that the glyphs are in one range
    u32 MinGlyph;
    u32 MaxGlyph;

    // NOTE: Vertical font data    
    i32 MaxAscent;
    i32 MaxDescent;
    i32 LineGap;

    f32* StepArray;
    f32* KernArray;
    asset_glyph* GlyphArray;
};

struct asset_texture;
struct asset_mesh
{
    asset_texture* Texture;
    u32 NumVertices;

    VkBuffer Handle;
    VkDeviceMemory Memory;
};

struct asset_model
{
    u32 NumTextures;
    asset_texture* TextureArray;
    u32 NumMeshes;
    asset_mesh* Meshes;
};

struct asset_texture
{
    v2 MinUV;
    v2 DimUV;
    f32 AspectRatio;
    // NOTE: This is the atlas id if we are a sub img
    VkImage Handle;
    VkImageView View;
    VkDeviceMemory Memory;
};

struct assets
{
    // NOTE: In memory, we have our asset arrays layed out next to each other and after that,
    // we have all the data of our assets laid out following the same order as we see them in
    // our array. This struct just holds the pointers to the array so we can easily access
    // our assets as required.

    u64 FontCount;
    asset_font* Fonts;

    u64 ModelCount;
    asset_model* Models;

    u64 TextureCount;
    asset_texture* Textures;
};

inline asset_glyph* GetGlyph(assets* Assets, u32 Id, char CodePoint)
{
    asset_glyph* Result = 0;

    Assert(Id < Assets->FontCount);
    asset_font* Font = Assets->Fonts + Id;

    Assert((u32)CodePoint <= Font->MaxGlyph && (u32)CodePoint >= Font->MinGlyph);
    Result = Font->GlyphArray + (CodePoint - Font->MinGlyph);

    return Result;
}

inline asset_glyph* GetGlyph(assets* Assets, asset_font* Font, char CodePoint)
{
    asset_glyph* Result = 0;

    Assert((u32)CodePoint <= Font->MaxGlyph && (u32)CodePoint >= Font->MinGlyph);
    Result = Font->GlyphArray + (CodePoint - Font->MinGlyph);

    return Result;
}

inline asset_font* GetFont(assets* Assets, u32 Id)
{
    asset_font* Result = 0;

    Assert(Id < Assets->FontCount);
    Result = Assets->Fonts + Id;

    return Result;
}

inline asset_model* GetModel(assets* Assets, u32 Id)
{
    asset_model* Result = 0;

    Assert(Id < Assets->ModelCount);
    Result = Assets->Models + Id;

    return Result;
}

inline asset_texture* GetTexture(assets* Assets, u32 Id)
{
    asset_texture* Result = 0;

    Assert(Id < Assets->TextureCount);
    Result = Assets->Textures + Id;

    return Result;
}

#define DRAGON_ASSETS_H
#endif
