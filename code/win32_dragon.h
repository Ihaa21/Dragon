#if !defined(WIN32_COCO_H)

#define RENDER_WIDTH 640
#define RENDER_HEIGHT 480

#define NUM_THREADS 4

struct image
{
    VkImage Image;
    VkImageView View;
};

struct gbuffer
{
    VkFramebuffer FrameBuffer;

    union
    {
        struct
        {
            VkImage WorldPosImg;
            VkImage DiffuseImg;
            VkImage WorldNormalImg;
            VkImage DepthImg;
            VkImage OutputImg;
        };

        VkImage Images[5];
    };

    union
    {
        struct
        {
            VkImageView WorldPosView;
            VkImageView DiffuseView;
            VkImageView WorldNormalView;
            VkImageView DepthView;
            VkImageView OutputView;
        };

        VkImageView Views[5];
    };
};

struct gbuffer_vert_uniforms
{
    m4 WTransform;
    m4 WVPTransform;
};

struct gbuffer_frag_uniforms
{
    f32 SpecularIntensity;
    f32 SpecularPower;
};

struct point_light_vert_uniforms
{
    m4 WVPTransform;
};

struct point_light_frag_uniforms
{
    v3 LightColor;
    f32 AmbientIntensity;

    v3 LightPos;
    f32 DiffuseIntensity;

    v3 EyeWorldPos;
    f32 LightRadius;
};

struct dir_light_uniforms
{
    v3 Color;
    f32 AmbientIntensity;

    v3 Dir;
    f32 DiffuseIntensity;

    v3 EyeWorldPos;
};

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

#define STAGING_NUM_BUFFERS 16
#define STAGING_BUFFER_SIZE (MegaBytes(2))

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
    
    // NOTE: Transfer queues
    u32 TransferQueueFamId;
    u32 NumTransferQueues;
    VkQueue* TransferQueues;
    
    // NOTE: Render data
    u32 GraphicsQueueFamId;
    VkQueue GraphicsQueue;

    u32 PresentQueueFamId;
    VkQueue PresentQueue;

    VkCommandPool GraphicsCmdPools[NUM_THREADS];
    VkCommandPool TransferCmdPool;
    
    VkCommandBuffer PrimaryCmdBuffer;
    VkSemaphore ImageAvailableSemaphore;
    VkSemaphore FinishedRenderingSemaphore;
    VkFence Fence;

    // NOTE: Deferred rendering data
    gbuffer GBuffer;
    VkRenderPass RenderPass;

    VkPipeline GBufferPipeline;
    VkPipelineLayout GBufferPipelineLayout;
    VkDescriptorSetLayout GBufferDescriptorSetLayout;
    VkDescriptorPool GBufferDescriptorPool;

    VkPipeline PointLightPipeline;
    VkPipelineLayout PointLightPipelineLayout;
    VkDescriptorSetLayout PointLightDescriptorSetLayout;
    VkDescriptorPool PointLightDescriptorPool;

    VkPipeline DirLightPipeline;
    VkPipelineLayout DirLightPipelineLayout;
    VkDescriptorSetLayout DirLightDescriptorSetLayout;
    VkDescriptorPool DirLightDescriptorPool;

    VkPipeline SsaoPipeline;
    VkPipelineLayout SsaoPipelineLayout;
    VkDescriptorSetLayout SsaoDescriptorSetLayout;
    VkDescriptorPool SsaoDescriptorPool;

    // NOTE: Staging buffer
    VkCommandBuffer StagingCmdBuffers[STAGING_NUM_BUFFERS];
    VkBuffer StagingBuffers[STAGING_NUM_BUFFERS];
    VkFence StagingFences[STAGING_NUM_BUFFERS];
    VkDeviceMemory StagingBufferMem;
    
    VkSampler TextureSampler;

    // NOTE: World data
    u32 NumDirLights;
    VkBuffer DirLightUniformBuffers[10];
    VkDescriptorSet DirLightDescriptorSets[10];

    u32 NumPointLights;
    VkBuffer PointLightVertUniformBuffers[100];
    VkBuffer PointLightFragUniformBuffers[100];
    VkDescriptorSet PointLightDescriptorSets[100];
    asset_mesh* SphereMesh;

    u32 NumModels;
    asset_mesh* MeshAssets[100];
    VkBuffer ModelVertUniformBuffers[100];
    VkBuffer ModelFragUniformBuffers[100];
    VkDescriptorSet ModelDescriptorSets[100];
};

#define WIN32_COCO_H
#endif
