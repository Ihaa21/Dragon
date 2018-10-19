#if !defined(WIN32_COCO_H)

#define RENDER_WIDTH 640
#define RENDER_HEIGHT 480

#define SCREEN_TILE_X 4
#define SCREEN_TILE_Y 4
#define MAX_NUM_LIGHTS_PER_TILE 64

#define NUM_THREADS 4

//
// NOTE: Multi threaded queue
//

struct work_queue;
#define WORK_QUEUE_CALLBACK(name) void name(work_queue* Queue, void* Data)
typedef WORK_QUEUE_CALLBACK(work_queue_callback);
struct work_queue_entry
{
    work_queue_callback* Callback;
    void* Data;
};

struct work_queue
{
    u32 volatile CompletionGoal;
    u32 volatile CompletionCount;

    u32 volatile NextEntryToWrite;
    u32 volatile NextEntryToMakeVisible;
    u32 volatile NextEntryToRead;

    HANDLE SemaphoreHandle;
    work_queue_entry Entries[1024];
};

struct win32_thread_info
{
    u32 ThreadId;
    work_queue* Queue;
};

//
// NOTE: Prog State
//

// TODO: Should be in math
struct plane
{
    v3 N;
    f32 d;
};

struct light_tile
{
    plane Left;
    plane Right;
    plane Top;
    plane Bottom;
    
    f32 MinDepth;
    f32 MaxDepth;
    u32 Pad[2];
};

struct point_light
{
    v3 Pos;
    f32 Radius;
    
    v3 LightColor;
    f32 AmbientIntensity;
    f32 DiffuseIntensity;
};

struct depth_prepass_consts
{
    m4 WVPTransform;
};

struct build_tile_consts
{
    m4 InverseVP;
    v2 ScreenDim;
    f32 NearZ;
    f32 FarZ;
};

struct light_cull_consts
{
    m4 VPTransform;
    u32 NumLights;
    u32 MaxLightsPerTile;
};

struct tiled_forward_consts
{
    m4 WTransform;
    m4 WVPTransform;

    f32 SpecularIntensity;
    f32 SpecularPower;
    v2 ScreenDim;

    u32 NumTilesX;
    u32 MaxLightsPerTile;
};

struct mesh
{
    asset_mesh* Asset;
    m4 WTransform;
    m4 WVPTransform;
    VkDescriptorSet DescriptorSet;
};

struct vk_pipeline
{
    VkPipelineLayout Layout;
    VkPipeline Pipeline;
};

#define TRANSFER_BUFFER_SIZE (MegaBytes(256))
#define VERTEX_SIZE (2*sizeof(v3) + sizeof(v2))
struct prog_state
{
    mem_arena Arena;
    work_queue WorkQueue;
    
    b32 GameIsRunning;
    HDC DeviceContext;
    i64 TimerFrequency;

    assets Assets;
    
    // NOTE: Window data
    HWND WindowHandle;
    VkSurfaceKHR WindowSurface;
    VkSwapchainKHR SwapChain;
    VkFormat SwapChainFormat;
    VkExtent2D SwapChainExtent;
    u32 NumSwapChainImgs;
    VkImage* SwapChainImgs;
    VkImageView* SwapChainViews;
    
    // NOTE: Vulkan global data
    VkInstance Instance;
    VkPhysicalDevice PhysicalDevice;
    VkDevice Device;
    VkPhysicalDeviceMemoryProperties MemoryProperties;

    // NOTE: Vulkan memory
    VkDeviceMemory GpuLocalMemory;
    gpu_linear_allocator GpuAllocator;
    
    // NOTE: Vulkan queues
    u32 TransferQueueFamId;
    VkQueue TransferQueue;

    u32 GraphicsQueueFamId;
    VkQueue GraphicsQueue;

    u32 PresentQueueFamId;
    VkQueue PresentQueue;

    u32 ComputeQueueFamId;
    VkQueue ComputeQueue;

    // NOTE: Transfer data
    VkBuffer TransferBuffer;
    u8* TransferBufferMemPtr;
    VkCommandPool TransferCmdPool;
    VkCommandBuffer TransferCmdBuffer;
    VkFence TransferFence;
    VkDeviceMemory TransferBufferMem;

    // NOTE: Graphics data
    VkCommandPool GraphicsCmdPool;
    VkCommandBuffer PrimaryCmdBuffer;
    VkSemaphore ImageAvailableSemaphore;
    VkSemaphore FinishedRenderingSemaphore;
    VkFence GraphicsFence;

    // NOTE: Descriptors
    VkDescriptorSetLayout MeshSetLayout;
    VkDescriptorPool MeshDescriptorPool;
    
    // NOTE: FrameBuffers
    VkFramebuffer FrameBuffer;
    union
    {
        struct
        {
            VkImage PresentImg;
            VkImage DepthImg;
        };

        VkImage Images[2];
    };

    union
    {
        struct
        {
            VkImageView PresentView;
            VkImageView DepthView;
        };

        VkImageView Views[2];
    };

    VkRenderPass RenderPass;

    vk_pipeline DepthPrePassPipeline;
    vk_pipeline LightCullPipeline;

    // NOTE: Descriptor Set Layouts
    VkDescriptorPool LightDescriptorPool;
    VkDescriptorSetLayout LightDataSetLayout;

    VkDescriptorSet LightDataSet;
    u32 NumPointLights;
    VkBuffer PointLightArray;
    VkBuffer PointLightIndexArray;
    VkImage DebugHeatMapImg;
    VkImageView DebugHeatMapView;
    
    // NOTE: Build tile data
    vk_pipeline BuildTileDataPipeline;
    VkDescriptorSetLayout BuildTileDataSetLayout;
    VkDescriptorSet BuildTileDataSet;
    VkBuffer LightTileArray;

    // NOTE: Forward pass
    vk_pipeline ForwardPipeline;
    
    // NOTE: Texture
    VkSampler TextureSampler;

    // NOTE: World data
    u32 NumModels;
    mesh RenderMeshes[100];

    m4 VTransform;
    m4 PTransform;
    m4 VPTransform;
    f32 NearZ;
    f32 FarZ;
};

#define WIN32_COCO_H
#endif
