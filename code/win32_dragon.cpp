#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <intrin.h>

#define VK_NO_PROTOTYPES 1
#define VK_USE_PLATFORM_WIN32_KHR 1
#include <vulkan.h>

#include "dragon_typedefs.h"
#include "dragon_math.h"
#include "dragon_math.cpp"
#include "dragon_string.h"
#include "dragon_memory.h"
#include "dragon_memory.cpp"
#include "dragon_gpu_memory.h"
#include "dragon_gpu_memory.cpp"
#include "dragon_file_assets.h"
#include "dragon_assets.h"

#include "win32_dragon.h"

global prog_state GlobalState;

internal VkShaderModule CreateShaderModule(char* FileName);
internal void CreateBuffer(gpu_linear_allocator* Allocator, VkBufferUsageFlags Usage,
                           VkMemoryPropertyFlagBits MemoryProperty, u32 BufferSize,
                           VkBuffer* BufferHandle);
internal void CreateImage(gpu_linear_allocator* Allocator, u32 Width, u32 Height, VkFormat Format,
                          i32 Usage, VkImageAspectFlags AspectMask, VkImageLayout Layout,
                          VkImage* OutImg, VkImageView* OutView);
inline void MoveBufferToGpuMemory(VkCommandBuffer CmdBuffer, VkBuffer Buffer, void* Data, u32 BufferSize);
inline void MoveImageToGpuMemory(gpu_linear_allocator* Allocator, VkCommandBuffer CmdBuffer,
                                 u32 ImageSize, u32 Width, u32 Height, void* Data, VkImage Image);

#include "dragon_assets.cpp"
#include "dragon_render.cpp"
#include "dragon.cpp"

#define VK_EXPORTED_FUNC(func) PFN_##func func;
#define VK_GLOBAL_LEVEL_FUNC(func) PFN_##func func;
#define VK_INSTANCE_LEVEL_FUNC(func) PFN_##func func;
#define VK_DEVICE_LEVEL_FUNC(func) PFN_##func func;

#define VULKAN_FUNC_LIST                                                \
    VK_EXPORTED_FUNC(       vkGetInstanceProcAddr);                     \
    VK_GLOBAL_LEVEL_FUNC(   vkCreateInstance);                          \
    VK_GLOBAL_LEVEL_FUNC(   vkEnumerateInstanceExtensionProperties);    \
    VK_GLOBAL_LEVEL_FUNC(   vkEnumerateInstanceLayerProperties);        \
    VK_INSTANCE_LEVEL_FUNC( vkDestroyInstance );                        \
    VK_INSTANCE_LEVEL_FUNC( vkEnumeratePhysicalDevices );               \
    VK_INSTANCE_LEVEL_FUNC( vkGetPhysicalDeviceProperties );            \
    VK_INSTANCE_LEVEL_FUNC( vkGetPhysicalDeviceFeatures );              \
    VK_INSTANCE_LEVEL_FUNC( vkGetPhysicalDeviceQueueFamilyProperties ); \
    VK_INSTANCE_LEVEL_FUNC( vkCreateDevice );                           \
    VK_INSTANCE_LEVEL_FUNC( vkGetDeviceProcAddr );                      \
    VK_INSTANCE_LEVEL_FUNC( vkEnumerateDeviceExtensionProperties );     \
    VK_INSTANCE_LEVEL_FUNC( vkDestroySurfaceKHR );                      \
    VK_INSTANCE_LEVEL_FUNC( vkGetPhysicalDeviceSurfaceSupportKHR );     \
    VK_INSTANCE_LEVEL_FUNC( vkGetPhysicalDeviceSurfaceCapabilitiesKHR ); \
    VK_INSTANCE_LEVEL_FUNC( vkGetPhysicalDeviceSurfaceFormatsKHR );     \
    VK_INSTANCE_LEVEL_FUNC( vkGetPhysicalDeviceSurfacePresentModesKHR ); \
    VK_INSTANCE_LEVEL_FUNC( vkCreateWin32SurfaceKHR );                  \
    VK_INSTANCE_LEVEL_FUNC( vkGetPhysicalDeviceMemoryProperties );      \
    VK_INSTANCE_LEVEL_FUNC( vkCreateDebugReportCallbackEXT );           \
    VK_INSTANCE_LEVEL_FUNC( vkDebugReportMessageEXT );                  \
    VK_INSTANCE_LEVEL_FUNC( vkDestroyDebugReportCallbackEXT );          \
    VK_DEVICE_LEVEL_FUNC(   vkGetDeviceQueue );                         \
    VK_DEVICE_LEVEL_FUNC(   vkDestroyDevice );                          \
    VK_DEVICE_LEVEL_FUNC(   vkDeviceWaitIdle );                         \
    VK_DEVICE_LEVEL_FUNC(   vkCreateSwapchainKHR );                     \
    VK_DEVICE_LEVEL_FUNC(   vkDestroySwapchainKHR );                    \
    VK_DEVICE_LEVEL_FUNC(   vkGetSwapchainImagesKHR );                  \
    VK_DEVICE_LEVEL_FUNC(   vkAcquireNextImageKHR );                    \
    VK_DEVICE_LEVEL_FUNC(   vkQueuePresentKHR );                        \
    VK_DEVICE_LEVEL_FUNC(   vkCreateSemaphore );                        \
    VK_DEVICE_LEVEL_FUNC(   vkQueueSubmit );                            \
    VK_DEVICE_LEVEL_FUNC(   vkCreateCommandPool );                      \
    VK_DEVICE_LEVEL_FUNC(   vkAllocateCommandBuffers );                 \
    VK_DEVICE_LEVEL_FUNC(   vkBeginCommandBuffer );                     \
    VK_DEVICE_LEVEL_FUNC(   vkCmdPipelineBarrier );                     \
    VK_DEVICE_LEVEL_FUNC(   vkCmdClearColorImage );                     \
    VK_DEVICE_LEVEL_FUNC(   vkEndCommandBuffer );                       \
    VK_DEVICE_LEVEL_FUNC(   vkCreateImageView );                        \
    VK_DEVICE_LEVEL_FUNC(   vkCreateRenderPass );                       \
    VK_DEVICE_LEVEL_FUNC(   vkCreateFramebuffer );                      \
    VK_DEVICE_LEVEL_FUNC(   vkCreateShaderModule );                     \
    VK_DEVICE_LEVEL_FUNC(   vkCreatePipelineLayout );                   \
    VK_DEVICE_LEVEL_FUNC(   vkCreateGraphicsPipelines );                \
    VK_DEVICE_LEVEL_FUNC(   vkCmdBeginRenderPass );                     \
    VK_DEVICE_LEVEL_FUNC(   vkCmdBindPipeline );                        \
    VK_DEVICE_LEVEL_FUNC(   vkCmdDraw );                                \
    VK_DEVICE_LEVEL_FUNC(   vkCmdEndRenderPass );                       \
    VK_DEVICE_LEVEL_FUNC(   vkCreateFence );                            \
    VK_DEVICE_LEVEL_FUNC(   vkCreateBuffer );                           \
    VK_DEVICE_LEVEL_FUNC(   vkGetBufferMemoryRequirements );            \
    VK_DEVICE_LEVEL_FUNC(   vkAllocateMemory );                         \
    VK_DEVICE_LEVEL_FUNC(   vkBindBufferMemory );                       \
    VK_DEVICE_LEVEL_FUNC(   vkMapMemory );                              \
    VK_DEVICE_LEVEL_FUNC(   vkFlushMappedMemoryRanges );                \
    VK_DEVICE_LEVEL_FUNC(   vkUnmapMemory );                            \
    VK_DEVICE_LEVEL_FUNC(   vkCmdSetViewport );                         \
    VK_DEVICE_LEVEL_FUNC(   vkCmdSetScissor );                          \
    VK_DEVICE_LEVEL_FUNC(   vkCmdBindVertexBuffers );                   \
    VK_DEVICE_LEVEL_FUNC(   vkWaitForFences );                          \
    VK_DEVICE_LEVEL_FUNC(   vkResetFences );                            \
    VK_DEVICE_LEVEL_FUNC(   vkFreeMemory );                             \
    VK_DEVICE_LEVEL_FUNC(   vkDestroyBuffer );                          \
    VK_DEVICE_LEVEL_FUNC(   vkDestroyFence );                           \
    VK_DEVICE_LEVEL_FUNC(   vkDestroyFramebuffer );                     \
    VK_DEVICE_LEVEL_FUNC(   vkCmdCopyBuffer );                          \
    VK_DEVICE_LEVEL_FUNC(   vkCreateImage );                            \
    VK_DEVICE_LEVEL_FUNC(   vkGetImageMemoryRequirements );             \
    VK_DEVICE_LEVEL_FUNC(   vkBindImageMemory );                        \
    VK_DEVICE_LEVEL_FUNC(   vkCreateSampler );                          \
    VK_DEVICE_LEVEL_FUNC(   vkCmdCopyBufferToImage );                   \
    VK_DEVICE_LEVEL_FUNC(   vkCreateDescriptorSetLayout );              \
    VK_DEVICE_LEVEL_FUNC(   vkCreateDescriptorPool );                   \
    VK_DEVICE_LEVEL_FUNC(   vkAllocateDescriptorSets );                 \
    VK_DEVICE_LEVEL_FUNC(   vkUpdateDescriptorSets );                   \
    VK_DEVICE_LEVEL_FUNC(   vkCmdBindDescriptorSets );                  \
    VK_DEVICE_LEVEL_FUNC(   vkDestroyDescriptorPool );                  \
    VK_DEVICE_LEVEL_FUNC(   vkDestroyDescriptorSetLayout );             \
    VK_DEVICE_LEVEL_FUNC(   vkDestroySampler );                         \
    VK_DEVICE_LEVEL_FUNC(   vkDestroyImage );                           \
    VK_DEVICE_LEVEL_FUNC(   vkCmdNextSubpass );                         \
    VK_DEVICE_LEVEL_FUNC(   vkFreeCommandBuffers );                     \
    

VULKAN_FUNC_LIST;

#undef VK_EXPORTED_FUNC
#undef VK_GLOBAL_LEVEL_FUNC
#undef VK_INSTANCE_LEVEL_FUNC
#undef VK_DEVICE_LEVEL_FUNC

//
// NOTE: Multi threaded
//

inline u32 GetThreadId()
{
    u8 *ThreadLocalStorage = (u8 *)__readgsqword(0x30);
    u32 Result = *(u32 *)(ThreadLocalStorage + 0x48);
    
    return Result;
}

inline void Win32AddWorkQueueEntry(work_queue* Queue, work_queue_callback* Callback, void* Data)
{
    // TODO: This will crash if a thread takes so long that it wraps around here in the circular buffer
    u32 OrigNextEntryToWrite;
    u32 NewNextEntryToWrite;
    for (;;)
    {
        OrigNextEntryToWrite = Queue->NextEntryToWrite;
        NewNextEntryToWrite = ((OrigNextEntryToWrite + 1) % ArrayCount(Queue->Entries));
        u32 Index = InterlockedCompareExchange((LONG volatile*)&Queue->NextEntryToWrite,
                                               NewNextEntryToWrite, OrigNextEntryToWrite);
        if (Index == OrigNextEntryToWrite)
        {
            break;
        }
    }

    _WriteBarrier();
    
    Assert(NewNextEntryToWrite != Queue->NextEntryToRead);

    work_queue_entry* Entry = Queue->Entries + OrigNextEntryToWrite;
    Entry->Callback = Callback;
    Entry->Data = Data;
    _WriteBarrier();
    InterlockedIncrement((LONG volatile*)&Queue->CompletionGoal);

    for (;;)
    {
        u32 OrigNextEntryToMakeVisible = Queue->NextEntryToMakeVisible;
        if (OrigNextEntryToMakeVisible == OrigNextEntryToWrite)
        {
            u32 Index = InterlockedCompareExchange((LONG volatile*)&Queue->NextEntryToMakeVisible,
                                                   NewNextEntryToWrite, OrigNextEntryToMakeVisible);
            if (Index == OrigNextEntryToMakeVisible)
            {
                break;
            }
        }
    }

    _WriteBarrier();
    
    ReleaseSemaphore(Queue->SemaphoreHandle, 1, 0);
}

internal b32 Win32DoNextWorkQueueEntry(work_queue* Queue)
{
    b32 ShouldSleep = false;

    u32 OriginalNextEntryToRead = Queue->NextEntryToRead;
    u32 NewNextEntryToRead = (OriginalNextEntryToRead + 1) % ArrayCount(Queue->Entries);
    if (OriginalNextEntryToRead != Queue->NextEntryToMakeVisible)
    {
        u32 Index = InterlockedCompareExchange((LONG volatile*)&Queue->NextEntryToRead,
                                               NewNextEntryToRead, OriginalNextEntryToRead);
        if (Index == OriginalNextEntryToRead)
        {
            u32 ThreadId = GetThreadId();
            {
                char Buffer[256];
                wsprintf(Buffer, "Thread %u has index %u\n", ThreadId, Index);
                OutputDebugString(Buffer);
            }
            work_queue_entry Entry = Queue->Entries[Index];
            Entry.Callback(Queue, Entry.Data);
            InterlockedIncrement((LONG volatile*)&Queue->CompletionCount);
        }
    }
    else
    {
        // NOTE: We sleep if next entry to do is equal to entry count
        ShouldSleep = true;
    }

    return ShouldSleep;
}

internal void Win32CompleteAllWork(work_queue* Queue)
{
    while (Queue->CompletionGoal != Queue->CompletionCount)
    {
        Win32DoNextWorkQueueEntry(Queue);
    }

    Queue->CompletionGoal = 0;
    Queue->CompletionCount = 0;
}

internal void Win32WaitForAllWork(work_queue* Queue)
{
    while (Queue->CompletionGoal != Queue->CompletionCount)
    {
    }
}

DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
    win32_thread_info* ThreadInfo = (win32_thread_info*)lpParameter;

    for (;;)
    {
        if (Win32DoNextWorkQueueEntry(ThreadInfo->Queue))
        {
            // NOTE: Thread goes to sleep
            WaitForSingleObjectEx(ThreadInfo->Queue->SemaphoreHandle, INFINITE, FALSE);
        }
    }
}

//
//
//

internal WORK_QUEUE_CALLBACK(Test)
{
    OutputDebugStringA("Test\n");
}

//
//
//

internal VkShaderModule CreateShaderModule(char* FileName)
{
    FILE* File = fopen(FileName, "rb");
    if (!File)
    {
        InvalidCodePath;
    }

    fseek(File, 0, SEEK_END);
    u32 CodeSize = ftell(File);
    fseek(File, 0, SEEK_SET);
    u32* Code = (u32*)malloc(CodeSize);
    fread(Code, CodeSize, 1, File);
    fclose(File);
    
    VkShaderModuleCreateInfo ShaderModuleCreateInfo =
    {
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        0,
        0,
        CodeSize,
        Code,
    };

    VkShaderModule ShaderModule;
    if (vkCreateShaderModule(GlobalState.Device, &ShaderModuleCreateInfo, 0, &ShaderModule) != VK_SUCCESS)
    {
        InvalidCodePath;
    }

    free(Code);
    
    return ShaderModule;
}

internal void CreateBuffer(gpu_linear_allocator* Allocator, VkBufferUsageFlags Usage,
                           VkMemoryPropertyFlagBits MemoryProperty, u32 BufferSize,
                           VkBuffer* BufferHandle)
{
    VkBufferCreateInfo BufferCreateInfo = {};
    BufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCreateInfo.pNext = 0;
    BufferCreateInfo.flags = 0;
    BufferCreateInfo.size = BufferSize;
    BufferCreateInfo.usage = Usage;
    BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    BufferCreateInfo.queueFamilyIndexCount = 0;
    BufferCreateInfo.pQueueFamilyIndices = 0;

    if (vkCreateBuffer(GlobalState.Device, &BufferCreateInfo, 0, BufferHandle) != VK_SUCCESS)
    {
        InvalidCodePath;
    }

    VkMemoryRequirements BufferMemRequirements;
    vkGetBufferMemoryRequirements(GlobalState.Device, *BufferHandle, &BufferMemRequirements);
    gpu_ptr MemPtr = PushSize(Allocator, BufferMemRequirements.size, BufferMemRequirements.alignment);
    if (vkBindBufferMemory(GlobalState.Device, *BufferHandle, *MemPtr.Memory, MemPtr.Offset) != VK_SUCCESS)
    {
        InvalidCodePath;
    }
}

struct load_texture_work
{
    u32 Width;
    u32 Height;
    
    VkFormat Format;
    VkImageUsageFlags Usage;
    VkImageLayout Layout;
    VkImageAspectFlags AspectFlags;

    asset_texture* Texture;
};

#if 0
internal void WorkLoadTexture()
{
    load_texture_work* Work = ;

    Work->Texture->MinUV = V2(0, 0);
    Work->Texture->DimUV = V2(1, 1);
    Work->Texture->AspectRatio = (f32)Work->Width / (f32)Work->Height;
    
    // NOTE: Create image handle
    VkImageCreateInfo ImageCreateInfo = {};
    ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageCreateInfo.pNext = 0;
    ImageCreateInfo.flags = 0;
    ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    ImageCreateInfo.format = Work->Format;
    ImageCreateInfo.extent.width = Work->Width;
    ImageCreateInfo.extent.height = Work->Height;
    ImageCreateInfo.extent.depth = 1;
    ImageCreateInfo.mipLevels = 1;
    ImageCreateInfo.arrayLayers = 1;
    ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    ImageCreateInfo.usage = Work->Usage;
    ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ImageCreateInfo.queueFamilyIndexCount = 0;
    ImageCreateInfo.pQueueFamilyIndices = 0;
    ImageCreateInfo.initialLayout = Work->Layout;

    if (vkCreateImage(GlobalState.Device, &ImageCreateInfo, 0, &Work->Texture->Handle) != VK_SUCCESS)
    {
        InvalidCodePath;
    }

    // NOTE: Allocate mem for image
    VkMemoryRequirements ImageMemRequirements;
    vkGetImageMemoryRequirements(GlobalState.Device, *OutImg, &ImageMemRequirements);

    for (u32 MemTypeId = 0; MemTypeId < GlobalState.MemoryProperties.memoryTypeCount; ++MemTypeId)
    {
        if ((ImageMemRequirements.memoryTypeBits & (1 << MemTypeId)) &&
            (GlobalState.MemoryProperties.memoryTypes[MemTypeId].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
        {
            VkMemoryAllocateInfo MemoryAllocateInfo =
                {
                    VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                    0,
                    ImageMemRequirements.size,
                    MemTypeId,
                };

            if (vkAllocateMemory(GlobalState.Device, &MemoryAllocateInfo, 0, OutMemory) != VK_SUCCESS)
            {
                InvalidCodePath;
            }
        }
    }

    if (vkBindImageMemory(GlobalState.Device, Work->Texture->Handle, Work->Texture->Memory, 0) != VK_SUCCESS)
    {
        InvalidCodePath;
    }

    // NOTE: Create image view
    VkImageViewCreateInfo ImgViewCreateInfo = {};
    ImgViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ImgViewCreateInfo.pNext = 0;
    ImgViewCreateInfo.flags = 0;
    ImgViewCreateInfo.image = *OutImg;
    ImgViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ImgViewCreateInfo.format = Format;
    ImgViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    ImgViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    ImgViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    ImgViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    ImgViewCreateInfo.subresourceRange.aspectMask = Work->AspectMask;
    ImgViewCreateInfo.subresourceRange.baseMipLevel = 0;
    ImgViewCreateInfo.subresourceRange.levelCount = 1;
    ImgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    ImgViewCreateInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(GlobalState.Device, &ImgViewCreateInfo, 0, &Work->Texture->View) != VK_SUCCESS)
    {
        InvalidCodePath;
    }

    // NOTE: Find a staging buffer to use
    
    // NOTE: Move data to GPU
    // TODO: Shouldn't this just work?
    if (Width == 0 || Height == 0)
    {
        return;
    }
    
    VkBuffer TempBuffer;
    VkDeviceMemory TempMemory;
    CreateBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 ImageSize, &TempBuffer, &TempMemory);
    MoveBufferToGpuMemory(GlobalState.PrimaryCmdBuffer, TempBuffer, Data, ImageSize);
    
    // NOTE: Copy texels from staging buffer to gpu local mem
    VkCommandBufferBeginInfo CmdBufferBeginInfo = {};
    CmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CmdBufferBeginInfo.pNext = 0;
    CmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    CmdBufferBeginInfo.pInheritanceInfo = 0;

    vkBeginCommandBuffer(CmdBuffer, &CmdBufferBeginInfo);

    VkImageSubresourceRange ImgSubresourceRange = {};
    ImgSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ImgSubresourceRange.baseMipLevel = 0;
    ImgSubresourceRange.levelCount = 1;
    ImgSubresourceRange.baseArrayLayer = 0;
    ImgSubresourceRange.layerCount = 1;

    VkImageMemoryBarrier ImgMemoryBarrierFromUndefinedToTransferDst = {};
    ImgMemoryBarrierFromUndefinedToTransferDst.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    ImgMemoryBarrierFromUndefinedToTransferDst.pNext = 0;
    ImgMemoryBarrierFromUndefinedToTransferDst.srcAccessMask = 0;
    ImgMemoryBarrierFromUndefinedToTransferDst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    ImgMemoryBarrierFromUndefinedToTransferDst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ImgMemoryBarrierFromUndefinedToTransferDst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    ImgMemoryBarrierFromUndefinedToTransferDst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    ImgMemoryBarrierFromUndefinedToTransferDst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    ImgMemoryBarrierFromUndefinedToTransferDst.image = Image;
    ImgMemoryBarrierFromUndefinedToTransferDst.subresourceRange = ImgSubresourceRange;

    vkCmdPipelineBarrier(CmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, 0, 0, 0, 1, &ImgMemoryBarrierFromUndefinedToTransferDst);

    VkBufferImageCopy BufferImgCopyInfo = {};
    BufferImgCopyInfo.bufferOffset = 0;
    BufferImgCopyInfo.bufferRowLength = 0;
    BufferImgCopyInfo.bufferImageHeight = 0;
    BufferImgCopyInfo.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    BufferImgCopyInfo.imageSubresource.mipLevel = 0;
    BufferImgCopyInfo.imageSubresource.baseArrayLayer = 0;
    BufferImgCopyInfo.imageSubresource.layerCount = 1;
    BufferImgCopyInfo.imageOffset.x = 0;
    BufferImgCopyInfo.imageOffset.y = 0;
    BufferImgCopyInfo.imageOffset.z = 0;
    BufferImgCopyInfo.imageExtent.width = Width;
    BufferImgCopyInfo.imageExtent.height = Height;
    BufferImgCopyInfo.imageExtent.depth = 1;
    vkCmdCopyBufferToImage(CmdBuffer, TempBuffer, Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                           &BufferImgCopyInfo);

    VkImageMemoryBarrier ImgMemoryBarrierFromTransferToShaderRead = {};
    ImgMemoryBarrierFromTransferToShaderRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    ImgMemoryBarrierFromTransferToShaderRead.pNext = 0;
    ImgMemoryBarrierFromTransferToShaderRead.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    ImgMemoryBarrierFromTransferToShaderRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    ImgMemoryBarrierFromTransferToShaderRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    ImgMemoryBarrierFromTransferToShaderRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    ImgMemoryBarrierFromTransferToShaderRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    ImgMemoryBarrierFromTransferToShaderRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    ImgMemoryBarrierFromTransferToShaderRead.image = Image;
    ImgMemoryBarrierFromTransferToShaderRead.subresourceRange = ImgSubresourceRange;
    vkCmdPipelineBarrier(CmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, 0, 0, 0, 1, &ImgMemoryBarrierFromTransferToShaderRead);
    vkEndCommandBuffer(CmdBuffer);

    VkSubmitInfo SubmitInfo = {};
    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.pNext = 0;
    SubmitInfo.waitSemaphoreCount = 0;
    SubmitInfo.pWaitSemaphores = 0;
    SubmitInfo.pWaitDstStageMask = 0;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &CmdBuffer;
    SubmitInfo.signalSemaphoreCount = 0;
    SubmitInfo.pSignalSemaphores = 0;
    if (vkQueueSubmit(GlobalState.GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
    {
        InvalidCodePath;
    }

    vkDeviceWaitIdle(GlobalState.Device);

    vkDestroyBuffer(GlobalState.Device, TempBuffer, 0);
    vkFreeMemory(GlobalState.Device, TempMemory, 0);
}
#endif

internal void CreateImage(gpu_linear_allocator* Allocator, u32 Width, u32 Height, VkFormat Format,
                          i32 Usage, VkImageAspectFlags AspectMask, VkImageLayout Layout,
                          VkImage* OutImg, VkImageView* OutView)
{
    VkImageCreateInfo ImageCreateInfo = {};
    ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageCreateInfo.pNext = 0;
    ImageCreateInfo.flags = 0;
    ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    ImageCreateInfo.format = Format;
    ImageCreateInfo.extent.width = Width;
    ImageCreateInfo.extent.height = Height;
    ImageCreateInfo.extent.depth = 1;
    ImageCreateInfo.mipLevels = 1;
    ImageCreateInfo.arrayLayers = 1;
    ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    ImageCreateInfo.usage = Usage;
    ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ImageCreateInfo.queueFamilyIndexCount = 0;
    ImageCreateInfo.pQueueFamilyIndices = 0;
    ImageCreateInfo.initialLayout = Layout;

    if (vkCreateImage(GlobalState.Device, &ImageCreateInfo, 0, OutImg) != VK_SUCCESS)
    {
        InvalidCodePath;
    }

    VkMemoryRequirements ImageMemRequirements;
    vkGetImageMemoryRequirements(GlobalState.Device, *OutImg, &ImageMemRequirements);
    gpu_ptr MemPtr = PushSize(Allocator, ImageMemRequirements.size, ImageMemRequirements.alignment);
    if (vkBindImageMemory(GlobalState.Device, *OutImg, *MemPtr.Memory, MemPtr.Offset) != VK_SUCCESS)
    {
        InvalidCodePath;
    }

    // NOTE: Create image view
    VkImageViewCreateInfo ImgViewCreateInfo = {};
    ImgViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ImgViewCreateInfo.pNext = 0;
    ImgViewCreateInfo.flags = 0;
    ImgViewCreateInfo.image = *OutImg;
    ImgViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ImgViewCreateInfo.format = Format;
    ImgViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    ImgViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    ImgViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    ImgViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    ImgViewCreateInfo.subresourceRange.aspectMask = AspectMask;
    ImgViewCreateInfo.subresourceRange.baseMipLevel = 0;
    ImgViewCreateInfo.subresourceRange.levelCount = 1;
    ImgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    ImgViewCreateInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(GlobalState.Device, &ImgViewCreateInfo, 0, OutView) != VK_SUCCESS)
    {
        InvalidCodePath;
    }
}

inline void MoveMemoryChunkToGpuMemory(VkCommandBuffer CmdBuffer, VkBuffer Buffer, u32 CurrByte,
                                       u32 BufferSize, u8* Data)
{
#if 0
    void* StagingBufferMemPtr;
    if (vkMapMemory(GlobalState.Device, GlobalState.StagingBufferMem, 0, BufferSize, 0, &StagingBufferMemPtr) != VK_SUCCESS)
    {
        InvalidCodePath;
    }
    memcpy(StagingBufferMemPtr, Data, BufferSize);
    vkUnmapMemory(GlobalState.Device, GlobalState.StagingBufferMem);
        
    // NOTE: Tells driver which parts of mem where modified
    VkMappedMemoryRange FlushRange = {};
    FlushRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    FlushRange.pNext = 0;
    FlushRange.memory = GlobalState.StagingBufferMem;
    FlushRange.offset = 0;
    FlushRange.size = BufferSize;
    vkFlushMappedMemoryRanges(GlobalState.Device, 1, &FlushRange);

    // NOTE: Copy data to device local memory
    VkCommandBufferBeginInfo CmdBufferBeginInfo = {};
    CmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CmdBufferBeginInfo.pNext = 0;
    CmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    CmdBufferBeginInfo.pInheritanceInfo = 0;
            
    vkBeginCommandBuffer(CmdBuffer, &CmdBufferBeginInfo);

    VkBufferCopy BufferCopyInfo = {};
    BufferCopyInfo.srcOffset = 0;
    BufferCopyInfo.dstOffset = CurrByte;
    BufferCopyInfo.size = BufferSize;    
    vkCmdCopyBuffer(CmdBuffer, GlobalState.StagingBuffer, Buffer, 1, &BufferCopyInfo);

    VkBufferMemoryBarrier BufferMemoryBarrier = {};
    BufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    BufferMemoryBarrier.pNext = 0;
    BufferMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    BufferMemoryBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    BufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    BufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    BufferMemoryBarrier.buffer = Buffer;
    BufferMemoryBarrier.offset = 0;
    BufferMemoryBarrier.size = VK_WHOLE_SIZE;
    vkCmdPipelineBarrier(CmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                         0, 0, 0, 1, &BufferMemoryBarrier, 0, 0);
    vkEndCommandBuffer(CmdBuffer);

    VkSubmitInfo SubmitInfo = {};
    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.pNext = 0;
    SubmitInfo.waitSemaphoreCount = 0;
    SubmitInfo.pWaitSemaphores = 0;
    SubmitInfo.pWaitDstStageMask = 0;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &CmdBuffer;
    SubmitInfo.signalSemaphoreCount = 0;
    SubmitInfo.pSignalSemaphores = 0;
    if (vkQueueSubmit(GlobalState.GraphicsQueue, 1, &SubmitInfo, GlobalState.StagingFence) != VK_SUCCESS)
    {
        InvalidCodePath;
    }
#endif
}

inline void MoveBufferToGpuMemory(VkCommandBuffer CmdBuffer, VkBuffer Buffer, void* Data, u32 BufferSize)
{
#if 0
    // NOTE: Write the memory to the staging buffer in chunks
    u8* CurrDataPtr = (u8*)Data;
    u32 CurrByte = 0;
    u32 NumFullCopies = (BufferSize / GlobalState.StagingBufferSize);
    while (NumFullCopies > 0)
    {
        MoveMemoryChunkToGpuMemory(CmdBuffer, Buffer, CurrByte, GlobalState.StagingBufferSize, CurrDataPtr);
        CurrByte += GlobalState.StagingBufferSize;
        CurrDataPtr += GlobalState.StagingBufferSize;
        NumFullCopies -= 1;

        if (vkWaitForFences(GlobalState.Device, 1, &GlobalState.StagingFence, VK_FALSE, 1000000000) != VK_SUCCESS)
        {
            InvalidCodePath;
        }
        vkResetFences(GlobalState.Device, 1, &GlobalState.StagingFence);
    }

    MoveMemoryChunkToGpuMemory(CmdBuffer, Buffer, CurrByte, BufferSize - CurrByte, CurrDataPtr);

    if (vkWaitForFences(GlobalState.Device, 1, &GlobalState.StagingFence, VK_FALSE, 1000000000) != VK_SUCCESS)
    {
        InvalidCodePath;
    }
    vkResetFences(GlobalState.Device, 1, &GlobalState.StagingFence);
#endif
}

inline void MoveImageToGpuMemory(gpu_linear_allocator* Allocator, VkCommandBuffer CmdBuffer,
                                 u32 ImageSize, u32 Width, u32 Height, void* Data, VkImage Image)
{
    // TODO: Shouldn't this just work?
    if (Width == 0 || Height == 0)
    {
        return;
    }

    // TODO: Fix this temporary buffer allocation stuff
    VkBuffer TempBuffer;
    CreateBuffer(Allocator, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 ImageSize, &TempBuffer);
    MoveBufferToGpuMemory(GlobalState.PrimaryCmdBuffer, TempBuffer, Data, ImageSize);
    
    // NOTE: Copy texels from staging buffer to gpu local mem
    VkCommandBufferBeginInfo CmdBufferBeginInfo = {};
    CmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CmdBufferBeginInfo.pNext = 0;
    CmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    CmdBufferBeginInfo.pInheritanceInfo = 0;

    vkBeginCommandBuffer(CmdBuffer, &CmdBufferBeginInfo);

    VkImageSubresourceRange ImgSubresourceRange = {};
    ImgSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ImgSubresourceRange.baseMipLevel = 0;
    ImgSubresourceRange.levelCount = 1;
    ImgSubresourceRange.baseArrayLayer = 0;
    ImgSubresourceRange.layerCount = 1;

    VkImageMemoryBarrier ImgMemoryBarrierFromUndefinedToTransferDst = {};
    ImgMemoryBarrierFromUndefinedToTransferDst.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    ImgMemoryBarrierFromUndefinedToTransferDst.pNext = 0;
    ImgMemoryBarrierFromUndefinedToTransferDst.srcAccessMask = 0;
    ImgMemoryBarrierFromUndefinedToTransferDst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    ImgMemoryBarrierFromUndefinedToTransferDst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ImgMemoryBarrierFromUndefinedToTransferDst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    ImgMemoryBarrierFromUndefinedToTransferDst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    ImgMemoryBarrierFromUndefinedToTransferDst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    ImgMemoryBarrierFromUndefinedToTransferDst.image = Image;
    ImgMemoryBarrierFromUndefinedToTransferDst.subresourceRange = ImgSubresourceRange;

    vkCmdPipelineBarrier(CmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, 0, 0, 0, 1, &ImgMemoryBarrierFromUndefinedToTransferDst);

    VkBufferImageCopy BufferImgCopyInfo = {};
    BufferImgCopyInfo.bufferOffset = 0;
    BufferImgCopyInfo.bufferRowLength = 0;
    BufferImgCopyInfo.bufferImageHeight = 0;
    BufferImgCopyInfo.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    BufferImgCopyInfo.imageSubresource.mipLevel = 0;
    BufferImgCopyInfo.imageSubresource.baseArrayLayer = 0;
    BufferImgCopyInfo.imageSubresource.layerCount = 1;
    BufferImgCopyInfo.imageOffset.x = 0;
    BufferImgCopyInfo.imageOffset.y = 0;
    BufferImgCopyInfo.imageOffset.z = 0;
    BufferImgCopyInfo.imageExtent.width = Width;
    BufferImgCopyInfo.imageExtent.height = Height;
    BufferImgCopyInfo.imageExtent.depth = 1;
    vkCmdCopyBufferToImage(CmdBuffer, TempBuffer, Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                           &BufferImgCopyInfo);

    VkImageMemoryBarrier ImgMemoryBarrierFromTransferToShaderRead = {};
    ImgMemoryBarrierFromTransferToShaderRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    ImgMemoryBarrierFromTransferToShaderRead.pNext = 0;
    ImgMemoryBarrierFromTransferToShaderRead.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    ImgMemoryBarrierFromTransferToShaderRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    ImgMemoryBarrierFromTransferToShaderRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    ImgMemoryBarrierFromTransferToShaderRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    ImgMemoryBarrierFromTransferToShaderRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    ImgMemoryBarrierFromTransferToShaderRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    ImgMemoryBarrierFromTransferToShaderRead.image = Image;
    ImgMemoryBarrierFromTransferToShaderRead.subresourceRange = ImgSubresourceRange;
    vkCmdPipelineBarrier(CmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, 0, 0, 0, 1, &ImgMemoryBarrierFromTransferToShaderRead);
    vkEndCommandBuffer(CmdBuffer);

    VkSubmitInfo SubmitInfo = {};
    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.pNext = 0;
    SubmitInfo.waitSemaphoreCount = 0;
    SubmitInfo.pWaitSemaphores = 0;
    SubmitInfo.pWaitDstStageMask = 0;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &CmdBuffer;
    SubmitInfo.signalSemaphoreCount = 0;
    SubmitInfo.pSignalSemaphores = 0;
    if (vkQueueSubmit(GlobalState.GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
    {
        InvalidCodePath;
    }

    vkDeviceWaitIdle(GlobalState.Device);

    vkDestroyBuffer(GlobalState.Device, TempBuffer, 0);
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(VkDebugReportFlagsEXT flags,
                                                   VkDebugReportObjectTypeEXT objectType,
                                                   uint64_t object, size_t location,
                                                   int32_t messageCode, const char* pLayerPrefix,
                                                   const char* pMessage, void* pUserData)
{
    InvalidCodePath;
    return VK_FALSE;
}

//
//
//

inline LARGE_INTEGER Win32GetClock()
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);

    return Result;
}

inline f32 Win32GetSecondsBetween(LARGE_INTEGER End, LARGE_INTEGER Start)
{
    f32 Result = ((f32)(End.QuadPart - Start.QuadPart) / (f32)GlobalState.TimerFrequency);
    return Result;
}

inline FILETIME Win32GetLastFileTime(char* FileName)
{
    FILETIME LastWriteTime = {};

    WIN32_FILE_ATTRIBUTE_DATA FileData;
    if (GetFileAttributesEx(FileName, GetFileExInfoStandard, &FileData))
    {
        LastWriteTime = FileData.ftLastWriteTime;
    }

    return LastWriteTime;
}

internal LRESULT CALLBACK Win32MainWindowCallBack(HWND Window, UINT Message, WPARAM WParam,
                                                  LPARAM LParam)
{
    LRESULT Result = 0;

    switch (Message)
    {
        case WM_CLOSE:
        case WM_DESTROY:
        {
            GlobalState.GameIsRunning = false;
        } break;
        
        default:
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }

    return Result;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    //
    // NOTE: Init program state
    //
    {
        mm ProgMemSize = MegaBytes(100);    
        LPVOID BaseAddress = (LPVOID)TeraBytes(2);
        
        void* ProgramMemory = VirtualAlloc(BaseAddress, ProgMemSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (!ProgramMemory)
        {
            InvalidCodePath;
        }

        GlobalState.Arena = InitArena(ProgramMemory, ProgMemSize);
        GlobalState.GameIsRunning = true;
    }

#if 0
    // TODO: REMOVE
    {
        u32 ArenaSize = MegaBytes(2);
        list_allocator Allocator = InitListAllocator(PushSize(&GlobalState.Arena, ArenaSize), ArenaSize);

        void* Mem1 = PushSize(&Allocator, 15);
        void* Mem2 = PushSize(&Allocator, 30);
        FreeAlloc(&Allocator, Mem2);

        int i = 0;
    }
#endif
    
    //
    // NOTE: Init the work queue
    //
    {
        win32_thread_info ThreadInfo[NUM_THREADS - 1] = {};
        u32 ThreadCount = ArrayCount(ThreadInfo);
        u32 InitialCount = 0;
        GlobalState.WorkQueue.SemaphoreHandle = CreateSemaphoreEx(0, InitialCount, ThreadCount, 0, 0, SEMAPHORE_ALL_ACCESS);

        for (u32 ThreadId = 0; ThreadId < ThreadCount; ++ThreadId)
        {
            win32_thread_info* Info = ThreadInfo + ThreadId;
            Info->Queue = &GlobalState.WorkQueue;
            Info->ThreadId = ThreadId;

            DWORD Win32ThreadId;
            HANDLE ThreadHandle = CreateThread(0, 0, ThreadProc, Info, 0, &Win32ThreadId);
            CloseHandle(ThreadHandle);
        }
        
        Win32CompleteAllWork(&GlobalState.WorkQueue);
    }

    //
    // NOTE: Init window and display
    //
    
    u32 WindowWidth = RENDER_WIDTH;
    u32 WindowHeight = RENDER_HEIGHT;
    {
        WNDCLASSA WindowClass = {};
        WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        WindowClass.lpfnWndProc = Win32MainWindowCallBack;
        WindowClass.hInstance = hInstance;
        WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
        WindowClass.lpszClassName = "DragonClass";

        if (!RegisterClassA(&WindowClass))
        {
            InvalidCodePath;
        }
        
        GlobalState.WindowHandle = CreateWindowExA(0,
                                                   WindowClass.lpszClassName,
                                                   "Haw Yeah",
                                                   WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                                   CW_USEDEFAULT,
                                                   CW_USEDEFAULT,
                                                   WindowWidth,
                                                   WindowHeight,
                                                   0,
                                                   0,
                                                   hInstance,
                                                   0);

        if (!GlobalState.WindowHandle)
        {
            InvalidCodePath;
        }
        
        GlobalState.DeviceContext = GetDC(GlobalState.WindowHandle);
    }

    //
    // NOTE: Init Vulkan
    //
    {
        // NOTE: Load vulkan lib and get instance proc addr function
        HMODULE VulkanLib = LoadLibrary("vulkan-1.dll");
        if (!VulkanLib)
        {
            InvalidCodePath;
        }

#define VK_EXPORTED_FUNC(func)                                  \
        func = (PFN_##func)GetProcAddress(VulkanLib, #func);    \
        if (!func)                                              \
        {                                                       \
            InvalidCodePath;                                    \
        }                                                       \
    
#define VK_GLOBAL_LEVEL_FUNC(func)                          \
        func = (PFN_##func)vkGetInstanceProcAddr(0, #func); \
        if (!func)                                          \
        {                                                   \
            InvalidCodePath;                                \
        }                                                   \

#define VK_INSTANCE_LEVEL_FUNC
#define VK_DEVICE_LEVEL_FUNC
        
        VULKAN_FUNC_LIST;

#undef VK_EXPORTED_FUNC
#undef VK_GLOBAL_LEVEL_FUNC
#undef VK_INSTANCE_LEVEL_FUNC
#undef VK_DEVICE_LEVEL_FUNC

        // NOTE: Create a vulkan instance
        {
            // NOTE: Check if instance extensions supported
            u32 ExtensionCount = 0;
            if (vkEnumerateInstanceExtensionProperties(0, &ExtensionCount, 0) != VK_SUCCESS)
            {
                InvalidCodePath;
            }

            VkExtensionProperties* Extensions = (VkExtensionProperties*)malloc(sizeof(VkExtensionProperties)*ExtensionCount);
            if (vkEnumerateInstanceExtensionProperties(0, &ExtensionCount, Extensions) != VK_SUCCESS)
            {
                InvalidCodePath;
            }

            const char* RequiredExtensions[] =
                {
                    VK_KHR_SURFACE_EXTENSION_NAME,
                    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
                    "VK_EXT_debug_report",
                };
            
            for (u32 RequiredId = 0; RequiredId < ArrayCount(RequiredExtensions); ++RequiredId)
            {
                b32 Found = false;
                for (u32 ExtensionId = 0; ExtensionId < ExtensionCount; ++ExtensionId)
                {
                    if (strcmp(RequiredExtensions[RequiredId], Extensions[ExtensionId].extensionName))
                    {
                        Found = true;
                        break;
                    }
                }

                if (!Found)
                {
                    InvalidCodePath;
                }
            }

            free(Extensions);
        
            // NOTE: Create a instance of vulkan
            VkApplicationInfo AppInfo =
                {
                    VK_STRUCTURE_TYPE_APPLICATION_INFO,
                    0,
                    "Dragon",
                    VK_MAKE_VERSION(1, 0, 0),
                    "Dragon",
                    VK_MAKE_VERSION(1, 0, 0),
                    VK_MAKE_VERSION(1, 0, 0),
                };

            const char* RequiredLayers[] =
                {
                    "VK_LAYER_LUNARG_standard_validation",
                };
            
            VkInstanceCreateInfo InstanceCreateInfo = {};
            InstanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            InstanceCreateInfo.pNext = 0;
            InstanceCreateInfo.flags = 0;
            InstanceCreateInfo.pApplicationInfo = &AppInfo;
            InstanceCreateInfo.enabledLayerCount = ArrayCount(RequiredLayers);
            InstanceCreateInfo.ppEnabledLayerNames = RequiredLayers;
            InstanceCreateInfo.enabledExtensionCount = ArrayCount(RequiredExtensions);
            InstanceCreateInfo.ppEnabledExtensionNames = RequiredExtensions;

            if (vkCreateInstance(&InstanceCreateInfo, 0, &GlobalState.Instance))
            {
                InvalidCodePath;
            }
        }
        
#define VK_INSTANCE_LEVEL_FUNC(func)                                    \
        func = (PFN_##func)vkGetInstanceProcAddr(GlobalState.Instance, #func); \
        if (!func)                                                      \
        {                                                               \
            InvalidCodePath;                                            \
        }                                                               \

#define VK_EXPORTED_FUNC
#define VK_GLOBAL_LEVEL_FUNC
#define VK_DEVICE_LEVEL_FUNC
        
        VULKAN_FUNC_LIST;
        
#undef VK_EXPORTED_FUNC
#undef VK_GLOBAL_LEVEL_FUNC
#undef VK_INSTANCE_LEVEL_FUNC
#undef VK_DEVICE_LEVEL_FUNC
    }
    
    // NOTE: Create a surface
    {
        VkWin32SurfaceCreateInfoKHR SurfaceCreateInfo =
            {
                VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
                0,
                0,
                hInstance,
                GlobalState.WindowHandle,
            };

        if (vkCreateWin32SurfaceKHR(GlobalState.Instance, &SurfaceCreateInfo, 0, &GlobalState.WindowSurface) != VK_SUCCESS)
        {
            InvalidCodePath;
        }
    }
        
    // NOTE: Select a physical device for our app
    {
        VkPhysicalDevice SelectedPhysicalDevice = VK_NULL_HANDLE;
        u32 NumDevices = 0;
        if (vkEnumeratePhysicalDevices(GlobalState.Instance, &NumDevices, 0) != VK_SUCCESS ||
            NumDevices == 0)
        {
            InvalidCodePath;
        }

        VkPhysicalDevice* PhysicalDevices = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice)*NumDevices);
        if (vkEnumeratePhysicalDevices(GlobalState.Instance, &NumDevices, PhysicalDevices) != VK_SUCCESS)
        {
            InvalidCodePath;
        }

        u32 SelectedGraphicsQueueFamilyIndex = UINT32_MAX;
        u32 SelectedPresentQueueFamilyIndex = UINT32_MAX;
        for (u32 DeviceId = 0; DeviceId < NumDevices; ++DeviceId)
        {
            VkPhysicalDevice CurrDevice = PhysicalDevices[DeviceId];

            // NOTE: Check if supports extensions we want
            u32 ExtensionCount = 0;
            if (vkEnumerateDeviceExtensionProperties(CurrDevice, 0, &ExtensionCount, 0) != VK_SUCCESS)
            {
                InvalidCodePath;
            }

            VkExtensionProperties* Extensions = (VkExtensionProperties*)malloc(sizeof(VkExtensionProperties)*ExtensionCount);
            if (vkEnumerateDeviceExtensionProperties(CurrDevice, 0, &ExtensionCount, Extensions) != VK_SUCCESS)
            {
                InvalidCodePath;
            }

            const char* RequiredExtensions[] =
                {
                    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                };

            for (u32 RequiredId = 0; RequiredId < ArrayCount(RequiredExtensions); ++RequiredId)
            {
                b32 Found = false;
                for (u32 ExtensionId = 0; ExtensionId < ExtensionCount; ++ExtensionId)
                {
                    if (strcmp(RequiredExtensions[RequiredId], Extensions[ExtensionId].extensionName))
                    {
                        Found = true;
                        break;
                    }
                }

                if (!Found)
                {
                    InvalidCodePath;
                }
            }
                
            free(Extensions);
                
            // NOTE: Figure out api version, limit on data stuff (textures), whether features are
            // supported like geometry shaders
            VkPhysicalDeviceProperties DeviceProperties;
            VkPhysicalDeviceFeatures DeviceFeatures;

            vkGetPhysicalDeviceProperties(CurrDevice, &DeviceProperties);
            vkGetPhysicalDeviceFeatures(CurrDevice, &DeviceFeatures);

            u32 MajorVer = VK_VERSION_MAJOR(DeviceProperties.apiVersion);
            u32 MinorVer = VK_VERSION_MINOR(DeviceProperties.apiVersion);
            u32 PatchVer = VK_VERSION_PATCH(DeviceProperties.apiVersion);

            if (MajorVer < 1 && DeviceProperties.limits.maxImageDimension2D < 4096)
            {
                InvalidCodePath;
            }

            // NOTE: Check the command buffer queue types we have available
            u32 QueueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(CurrDevice, &QueueFamilyCount, 0);
            if (QueueFamilyCount == 0)
            {
                InvalidCodePath;
            }

            // NOTE: Check queue's for graphics, compute, transfer, etc support
            VkQueueFamilyProperties* QueueFamilyProperties = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties)*QueueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(CurrDevice, &QueueFamilyCount, QueueFamilyProperties);
            for (u32 FamilyId = 0; FamilyId < QueueFamilyCount; ++FamilyId)
            {
                // NOTE: Check if queue supports swap chain as well
                u32 PresentQueueSupport = UINT32_MAX;
                vkGetPhysicalDeviceSurfaceSupportKHR(CurrDevice, FamilyId, GlobalState.WindowSurface, &PresentQueueSupport);

                // NOTE: Select queue that supports graphics
                if (QueueFamilyProperties[FamilyId].queueCount > 0 &&
                    QueueFamilyProperties[FamilyId].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    // IMPORTANT: We can have diff queues for graphics, present. We may want
                    // to handle that case
                    // NOTE: We take queue that supports graphics and present
                    if (PresentQueueSupport)
                    {
                        SelectedGraphicsQueueFamilyIndex = FamilyId;
                        SelectedPresentQueueFamilyIndex = FamilyId;
                        SelectedPhysicalDevice = CurrDevice;
                        break;
                    }
                }
            }

            free(QueueFamilyProperties);

            // NOTE: Check if we found a device
            if (SelectedPhysicalDevice != VK_NULL_HANDLE)
            {
                break;
            }
        }

        free(PhysicalDevices);

        if (SelectedPhysicalDevice == VK_NULL_HANDLE)
        {
            InvalidCodePath;
        }
            
        GlobalState.PhysicalDevice = SelectedPhysicalDevice;

        // NOTE: Create a device
        u32 NumQueues = 0;
        f32 QueuePriorities[] = {1.0f};
        VkDeviceQueueCreateInfo QueueCreateInfos[2];

        ++NumQueues;
        QueueCreateInfos[0] = 
            {
                VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                0,
                0,
                SelectedGraphicsQueueFamilyIndex,
                ArrayCount(QueuePriorities),
                QueuePriorities,
            };

        if (SelectedGraphicsQueueFamilyIndex != SelectedPresentQueueFamilyIndex)
        {
            ++NumQueues;
            QueueCreateInfos[1] =
                {
                    VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    0,
                    0,
                    SelectedPresentQueueFamilyIndex,
                    ArrayCount(QueuePriorities),
                    QueuePriorities,
                };
        }

        // NOTE: Add required extensions
        const char* Extensions[] =
            {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            };
            
        VkDeviceCreateInfo DeviceCreateInfo =
            {
                VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                0,
                0,
                NumQueues,
                QueueCreateInfos,
                0,
                0,
                ArrayCount(Extensions),
                Extensions,
                0,
            };

        if (vkCreateDevice(SelectedPhysicalDevice, &DeviceCreateInfo, 0, &GlobalState.Device) != VK_SUCCESS)
        {
            InvalidCodePath;
        }

        GlobalState.GraphicsQueueFamId = SelectedGraphicsQueueFamilyIndex;
        GlobalState.PresentQueueFamId = SelectedPresentQueueFamilyIndex;
        
#define VK_DEVICE_LEVEL_FUNC(func)                                      \
        func = (PFN_##func)vkGetDeviceProcAddr(GlobalState.Device, #func); \
        if (!func)                                                      \
        {                                                               \
            InvalidCodePath;                                            \
        }                                                               \

#define VK_EXPORTED_FUNC
#define VK_GLOBAL_LEVEL_FUNC
#define VK_INSTANCE_LEVEL_FUNC(func)
        
        VULKAN_FUNC_LIST;
        
#undef VK_EXPORTED_FUNC
#undef VK_GLOBAL_LEVEL_FUNC
#undef VK_INSTANCE_LEVEL_FUNC
#undef VK_DEVICE_LEVEL_FUNC

        // NOTE: Get device queue
        vkGetDeviceQueue(GlobalState.Device, GlobalState.GraphicsQueueFamId, 0, &GlobalState.GraphicsQueue);
        vkGetDeviceQueue(GlobalState.Device, GlobalState.PresentQueueFamId, 0, &GlobalState.PresentQueue);
            
        // NOTE: Get Physical device mem properties
        vkGetPhysicalDeviceMemoryProperties(GlobalState.PhysicalDevice, &GlobalState.MemoryProperties);
    }

    // NOTE: Setup debug callback
    {
        VkDebugReportCallbackCreateInfoEXT CallbackCreateInfo = {};
        CallbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        CallbackCreateInfo.pNext = 0;
        CallbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        CallbackCreateInfo.pfnCallback = &DebugReportCallback;
        CallbackCreateInfo.pUserData = 0;

        VkDebugReportCallbackEXT Callback;
        if (vkCreateDebugReportCallbackEXT(GlobalState.Instance, &CallbackCreateInfo, 0, &Callback) != VK_SUCCESS)
        {
            InvalidCodePath;
        }
    }

    // NOTE: Create swap chain
    {
        VkSurfaceCapabilitiesKHR SurfaceCapabilities;
        if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(GlobalState.PhysicalDevice, GlobalState.WindowSurface, &SurfaceCapabilities) != VK_SUCCESS)
        {
            InvalidCodePath;
        }

        u32 FormatsCount;
        if (vkGetPhysicalDeviceSurfaceFormatsKHR(GlobalState.PhysicalDevice, GlobalState.WindowSurface, &FormatsCount, 0) != VK_SUCCESS ||
            FormatsCount == 0)
        {
            InvalidCodePath;
        }

        VkSurfaceFormatKHR* SurfaceFormats = (VkSurfaceFormatKHR*)malloc(sizeof(VkSurfaceFormatKHR)*FormatsCount);
        if (vkGetPhysicalDeviceSurfaceFormatsKHR(GlobalState.PhysicalDevice, GlobalState.WindowSurface, &FormatsCount, SurfaceFormats) != VK_SUCCESS)
        {
            InvalidCodePath;
        }

        u32 PresentModesCount;
        if (vkGetPhysicalDeviceSurfacePresentModesKHR(GlobalState.PhysicalDevice, GlobalState.WindowSurface, &PresentModesCount, 0) != VK_SUCCESS ||
            PresentModesCount == 0)
        {
            InvalidCodePath;
        }

        VkPresentModeKHR* PresentModes = (VkPresentModeKHR*)malloc(sizeof(VkPresentModeKHR)*PresentModesCount);
        if (vkGetPhysicalDeviceSurfacePresentModesKHR(GlobalState.PhysicalDevice, GlobalState.WindowSurface, &PresentModesCount, PresentModes) != VK_SUCCESS)
        {
            InvalidCodePath;
        }

        // NOTE: We define how many images we want in the swap chain
        u32 ImageCount = SurfaceCapabilities.minImageCount + 1;
        if (SurfaceCapabilities.maxImageCount > 0 &&
            ImageCount > SurfaceCapabilities.maxImageCount)
        {
            ImageCount = SurfaceCapabilities.maxImageCount;
        }

        // NOTE: Select image format
        VkSurfaceFormatKHR ChosenFormat = {};
        {
            if (FormatsCount == 1 && SurfaceFormats[0].format == VK_FORMAT_UNDEFINED)
            {
                // NOTE: No preferred format, choose what we want
                ChosenFormat = {VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR};
            }

            b32 Found = false;
            for (u32 FormatId = 0; FormatId < FormatsCount; ++FormatId)
            {
                if (SurfaceFormats[FormatId].format == VK_FORMAT_R8G8B8A8_UNORM)
                {
                    ChosenFormat = SurfaceFormats[FormatId];
                    Found = true;
                    break;
                }
            }

            if (!Found)
            {
                ChosenFormat = SurfaceFormats[0];
            }
        }

        // NOTE: Select swap chain image size
        VkExtent2D SwapChainExtent = { RENDER_WIDTH, RENDER_HEIGHT };
        if (SurfaceCapabilities.currentExtent.width == -1)
        {
            if (SwapChainExtent.width < SurfaceCapabilities.minImageExtent.width)
            {
                SwapChainExtent.width = SurfaceCapabilities.minImageExtent.width;
            }
            if (SwapChainExtent.width > SurfaceCapabilities.maxImageExtent.width)
            {
                SwapChainExtent.width = SurfaceCapabilities.maxImageExtent.width;
            }
            if (SwapChainExtent.height < SurfaceCapabilities.minImageExtent.height)
            {
                SwapChainExtent.height = SurfaceCapabilities.minImageExtent.height;
            }
            if (SwapChainExtent.height > SurfaceCapabilities.maxImageExtent.height)
            {
                SwapChainExtent.height = SurfaceCapabilities.maxImageExtent.height;
            }
        }

        // NOTE: Ask for the render target to have a color buffer, and to be cleared
        VkImageUsageFlags Flags = {};
        if (SurfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        {
            Flags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        else
        {
            InvalidCodePath;
        }

        // NOTE: For mobile, we want our swap chain to be transformed (rotation)
        VkSurfaceTransformFlagBitsKHR SurfaceTransform = {};
        if (SurfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        {
            SurfaceTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        }
        else
        {
            SurfaceTransform = SurfaceCapabilities.currentTransform;
        }

        // NOTE: Select a presentation mode (we choose triple buffering)
        VkPresentModeKHR PresentMode = {};
        for (u32 PresentationId = 0; PresentationId < PresentModesCount; ++PresentationId)
        {
            if (PresentModes[PresentationId] == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                PresentMode = PresentModes[PresentationId];
            }
        }

        if (PresentMode != VK_PRESENT_MODE_MAILBOX_KHR)
        {
            for (u32 PresentationId = 0; PresentationId < PresentModesCount; ++PresentationId)
            {
                if (PresentModes[PresentationId] == VK_PRESENT_MODE_FIFO_KHR)
                {
                    PresentMode = PresentModes[PresentationId];
                }
            }

            Assert(PresentMode == VK_PRESENT_MODE_FIFO_KHR);
        }
            
        free(SurfaceFormats);
        free(PresentModes);

        // NOTE: Finally create the swap chain
        VkSwapchainKHR OldSwapChain = GlobalState.SwapChain;
        VkSwapchainCreateInfoKHR SwapChainCreateInfo =
            {
                VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                0,
                0,
                GlobalState.WindowSurface,
                ImageCount,
                ChosenFormat.format,
                ChosenFormat.colorSpace,
                SwapChainExtent,
                1,
                Flags,
                VK_SHARING_MODE_EXCLUSIVE,
                0,
                0,
                SurfaceTransform,
                VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                PresentMode,
                VK_TRUE,
                OldSwapChain,
            };

        if (vkCreateSwapchainKHR(GlobalState.Device, &SwapChainCreateInfo, 0, &GlobalState.SwapChain) != VK_SUCCESS)
        {
            InvalidCodePath;
        }

        if (OldSwapChain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(GlobalState.Device, OldSwapChain, 0);
        }

        // NOTE: Set global meta data about swap chain
        GlobalState.SwapChainFormat = ChosenFormat.format;
        GlobalState.NumSwapChainImgs = ImageCount;
        GlobalState.SwapChainExtent = SwapChainExtent;

        // NOTE: Get swap chain images
        GlobalState.SwapChainImgs = (VkImage*)malloc(sizeof(VkImage)*GlobalState.NumSwapChainImgs);
        if (vkGetSwapchainImagesKHR(GlobalState.Device, GlobalState.SwapChain, &ImageCount, GlobalState.SwapChainImgs) != VK_SUCCESS)
        {
            InvalidCodePath;
        }
            
        // NOTE: Create image views for swap chain images
        GlobalState.SwapChainViews = (VkImageView*)malloc(sizeof(VkImageView)*GlobalState.NumSwapChainImgs);
        for (u32 ImgId = 0; ImgId < GlobalState.NumSwapChainImgs; ++ImgId)
        {
            VkImageViewCreateInfo ImageViewCreateInfo =
                {
                    VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    0,
                    0,
                    GlobalState.SwapChainImgs[ImgId],
                    VK_IMAGE_VIEW_TYPE_2D,
                    GlobalState.SwapChainFormat,
                    {
                        VK_COMPONENT_SWIZZLE_IDENTITY,
                        VK_COMPONENT_SWIZZLE_IDENTITY,
                        VK_COMPONENT_SWIZZLE_IDENTITY,
                        VK_COMPONENT_SWIZZLE_IDENTITY,
                    },
                    {
                        VK_IMAGE_ASPECT_COLOR_BIT,
                        0,
                        1,
                        0,
                        1,
                    },
                };

            if (vkCreateImageView(GlobalState.Device, &ImageViewCreateInfo, 0, &GlobalState.SwapChainViews[ImgId]) != VK_SUCCESS)
            {
                InvalidCodePath;
            }
        }
    }

    // NOTE: Init Gpu Memory Allocator
    {
        u64 ProgGpuAllocSize = MegaBytes(512);
        
        VkMemoryAllocateInfo MemoryAllocateInfo = {};
        MemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        MemoryAllocateInfo.pNext = 0;
        MemoryAllocateInfo.allocationSize = ProgGpuAllocSize;
        MemoryAllocateInfo.memoryTypeIndex = 0;

        if (vkAllocateMemory(GlobalState.Device, &MemoryAllocateInfo, 0, &GlobalState.GpuLocalMemory) != VK_SUCCESS)
        {
            InvalidCodePath;
        }

        gpu_ptr BasePtr = {};
        BasePtr.Memory = &GlobalState.GpuLocalMemory;
        BasePtr.Offset = 0;

        GlobalState.GpuAllocator = InitGpuLinearAllocator(BasePtr, ProgGpuAllocSize);
    }

    // NOTE: Create command pools
    {
        // NOTE: Allocate graphics pools
        VkCommandPoolCreateInfo CmdPoolCreateInfo = {};
        CmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        CmdPoolCreateInfo.pNext = 0;
        CmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        CmdPoolCreateInfo.queueFamilyIndex = GlobalState.GraphicsQueueFamId;

        for (u32 PoolId = 0; PoolId < NUM_THREADS; ++PoolId)
        {
            VkCommandPool* CmdPool = GlobalState.GraphicsCmdPools + PoolId;
            if (vkCreateCommandPool(GlobalState.Device, &CmdPoolCreateInfo, 0, CmdPool) != VK_SUCCESS)
            {
                InvalidCodePath;
            }
        }

        // NOTE: Allocate command buffers, create semaphores, and fences
        VkCommandBufferAllocateInfo CmdBufferAllocateInfo = {};
        CmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        CmdBufferAllocateInfo.pNext = 0;
        CmdBufferAllocateInfo.commandPool = GlobalState.GraphicsCmdPools[0];
        CmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        CmdBufferAllocateInfo.commandBufferCount = 1;

        VkSemaphoreCreateInfo SemaphoreCreateInfo = {};
        SemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        SemaphoreCreateInfo.pNext = 0;
        SemaphoreCreateInfo.flags = 0;

        VkFenceCreateInfo FenceCreateInfo = {};
        FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        FenceCreateInfo.pNext = 0;
        FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkAllocateCommandBuffers(GlobalState.Device, &CmdBufferAllocateInfo, &GlobalState.PrimaryCmdBuffer) != VK_SUCCESS)
        {
            InvalidCodePath;
        }

        if (vkCreateSemaphore(GlobalState.Device, &SemaphoreCreateInfo, 0, &GlobalState.ImageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(GlobalState.Device, &SemaphoreCreateInfo, 0, &GlobalState.FinishedRenderingSemaphore) != VK_SUCCESS)
        {
            InvalidCodePath;
        }

        if (vkCreateFence(GlobalState.Device, &FenceCreateInfo, 0, &GlobalState.Fence) != VK_SUCCESS)
        {
            InvalidCodePath;
        }
    }

    // NOTE: Create render pass
    {
        // TODO: I think all of these need to be same format or atleast have same color channels
        VkFormat WorldPosFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
        VkFormat DiffuseFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
        VkFormat WorldNormalFormat = VK_FORMAT_R32G32B32A32_SFLOAT; // TODO: Why does R32B32G32 not work??
        VkFormat DepthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;
        VkFormat OutputFormat = GlobalState.SwapChainFormat;
            
        // NOTE: Create GBuffer images
        CreateImage(&GlobalState.GpuAllocator, RENDER_WIDTH, RENDER_HEIGHT, WorldPosFormat,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    &GlobalState.GBuffer.WorldPosImg, &GlobalState.GBuffer.WorldPosView);
            
        CreateImage(&GlobalState.GpuAllocator, RENDER_WIDTH, RENDER_HEIGHT, DiffuseFormat,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    &GlobalState.GBuffer.DiffuseImg, &GlobalState.GBuffer.DiffuseView);
            
        CreateImage(&GlobalState.GpuAllocator, RENDER_WIDTH, RENDER_HEIGHT, WorldNormalFormat,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    &GlobalState.GBuffer.WorldNormalImg, &GlobalState.GBuffer.WorldNormalView);
            
        // NOTE: Create depth map and convert it
        VkImageAspectFlags DepthAspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        CreateImage(&GlobalState.GpuAllocator, RENDER_WIDTH, RENDER_HEIGHT, DepthFormat,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    DepthAspectMask, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    &GlobalState.GBuffer.DepthImg, &GlobalState.GBuffer.DepthView);

        // TODO: Make this a transition image layout function
        {
            VkCommandBufferBeginInfo CmdBufferBeginInfo = {};
            CmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            CmdBufferBeginInfo.pNext = 0;
            CmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            CmdBufferBeginInfo.pInheritanceInfo = 0;

            vkBeginCommandBuffer(GlobalState.PrimaryCmdBuffer, &CmdBufferBeginInfo);

            VkImageMemoryBarrier Barrier = {};
            Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            Barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            Barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            Barrier.image = GlobalState.GBuffer.DepthImg;
            Barrier.subresourceRange.aspectMask = DepthAspectMask;
            Barrier.subresourceRange.baseMipLevel = 0;
            Barrier.subresourceRange.levelCount = 1;
            Barrier.subresourceRange.baseArrayLayer = 0;
            Barrier.subresourceRange.layerCount = 1;
            Barrier.srcAccessMask = 0;
            Barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            VkPipelineStageFlags SourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            VkPipelineStageFlags DestinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

            vkCmdPipelineBarrier(GlobalState.PrimaryCmdBuffer, SourceStage, DestinationStage, 0, 0, 0, 0, 0, 1, &Barrier);
            vkEndCommandBuffer(GlobalState.PrimaryCmdBuffer);

            VkSubmitInfo SubmitInfo = {};
            SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            SubmitInfo.pNext = 0;
            SubmitInfo.waitSemaphoreCount = 0;
            SubmitInfo.pWaitSemaphores = 0;
            SubmitInfo.pWaitDstStageMask = 0;
            SubmitInfo.commandBufferCount = 1;
            SubmitInfo.pCommandBuffers = &GlobalState.PrimaryCmdBuffer;
            SubmitInfo.signalSemaphoreCount = 0;
            SubmitInfo.pSignalSemaphores = 0;
            if (vkQueueSubmit(GlobalState.GraphicsQueue, 1, &SubmitInfo, GlobalState.Fence) != VK_SUCCESS)
            {
                InvalidCodePath;
            }

            if (vkWaitForFences(GlobalState.Device, 1, &GlobalState.Fence, VK_FALSE, 1000000000) != VK_SUCCESS)
            {
                InvalidCodePath;
            }
            vkResetFences(GlobalState.Device, 1, &GlobalState.Fence);
        }

        VkAttachmentDescription AttachmentDescriptions[5] = {};
        AttachmentDescriptions[0].flags = 0;
        AttachmentDescriptions[0].format = WorldPosFormat;
        AttachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
        AttachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        AttachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        AttachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        AttachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        AttachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        AttachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        AttachmentDescriptions[1].flags = 0;
        AttachmentDescriptions[1].format = DiffuseFormat;
        AttachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
        AttachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        AttachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        AttachmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        AttachmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        AttachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        AttachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        AttachmentDescriptions[2].flags = 0;
        AttachmentDescriptions[2].format = WorldNormalFormat;
        AttachmentDescriptions[2].samples = VK_SAMPLE_COUNT_1_BIT;
        AttachmentDescriptions[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        AttachmentDescriptions[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        AttachmentDescriptions[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        AttachmentDescriptions[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        AttachmentDescriptions[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        AttachmentDescriptions[2].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        AttachmentDescriptions[3].flags = 0;
        AttachmentDescriptions[3].format = DepthFormat;
        AttachmentDescriptions[3].samples = VK_SAMPLE_COUNT_1_BIT;
        AttachmentDescriptions[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        AttachmentDescriptions[3].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        AttachmentDescriptions[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        AttachmentDescriptions[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        AttachmentDescriptions[3].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        AttachmentDescriptions[3].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        AttachmentDescriptions[4].flags = 0;
        AttachmentDescriptions[4].format = OutputFormat;
        AttachmentDescriptions[4].samples = VK_SAMPLE_COUNT_1_BIT;
        AttachmentDescriptions[4].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        AttachmentDescriptions[4].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        AttachmentDescriptions[4].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        AttachmentDescriptions[4].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        AttachmentDescriptions[4].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        AttachmentDescriptions[4].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkSubpassDescription SubpassDescriptions[3] = {};

        // NOTE: Create GBuffer subpass
        {
            VkAttachmentReference ColorReferences[3] = {};
            ColorReferences[0].attachment = 0;
            ColorReferences[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            ColorReferences[1].attachment = 1;
            ColorReferences[1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            ColorReferences[2].attachment = 2;
            ColorReferences[2].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkAttachmentReference DepthReference = {};
            DepthReference.attachment = 3;
            DepthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            SubpassDescriptions[0].flags = 0;
            SubpassDescriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            SubpassDescriptions[0].inputAttachmentCount = 0;
            SubpassDescriptions[0].pInputAttachments = 0;
            SubpassDescriptions[0].colorAttachmentCount = ArrayCount(ColorReferences);
            SubpassDescriptions[0].pColorAttachments = ColorReferences;
            SubpassDescriptions[0].pResolveAttachments = 0;
            SubpassDescriptions[0].pDepthStencilAttachment = &DepthReference;
            SubpassDescriptions[0].preserveAttachmentCount = 0;
            SubpassDescriptions[0].pPreserveAttachments = 0;
        }

        // NOTE: Create point light subpass
        {
            VkAttachmentReference ColorReferences[1] = {};
            ColorReferences[0].attachment = 4;
            ColorReferences[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkAttachmentReference InputReferences[3] = {};
            InputReferences[0].attachment = 0;
            InputReferences[0].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            InputReferences[1].attachment = 1;
            InputReferences[1].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            InputReferences[2].attachment = 2;
            InputReferences[2].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                
            VkAttachmentReference DepthReference = {};
            DepthReference.attachment = 3;
            DepthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            SubpassDescriptions[1].flags = 0;
            SubpassDescriptions[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            SubpassDescriptions[1].inputAttachmentCount = ArrayCount(InputReferences);
            SubpassDescriptions[1].pInputAttachments = InputReferences;
            SubpassDescriptions[1].colorAttachmentCount = ArrayCount(ColorReferences);
            SubpassDescriptions[1].pColorAttachments = ColorReferences;
            SubpassDescriptions[1].pResolveAttachments = 0;
            SubpassDescriptions[1].pDepthStencilAttachment = &DepthReference;
            SubpassDescriptions[1].preserveAttachmentCount = 0;
            SubpassDescriptions[1].pPreserveAttachments = 0;
        }

        // NOTE: Create point light subpass
        {
            VkAttachmentReference ColorReferences[1] = {};
            ColorReferences[0].attachment = 4;
            ColorReferences[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkAttachmentReference InputReferences[3] = {};
            InputReferences[0].attachment = 0;
            InputReferences[0].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            InputReferences[1].attachment = 1;
            InputReferences[1].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            InputReferences[2].attachment = 2;
            InputReferences[2].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                
            VkAttachmentReference DepthReference = {};
            DepthReference.attachment = 3;
            DepthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            SubpassDescriptions[2].flags = 0;
            SubpassDescriptions[2].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            SubpassDescriptions[2].inputAttachmentCount = ArrayCount(InputReferences);
            SubpassDescriptions[2].pInputAttachments = InputReferences;
            SubpassDescriptions[2].colorAttachmentCount = ArrayCount(ColorReferences);
            SubpassDescriptions[2].pColorAttachments = ColorReferences;
            SubpassDescriptions[2].pResolveAttachments = 0;
            SubpassDescriptions[2].pDepthStencilAttachment = &DepthReference;
            SubpassDescriptions[2].preserveAttachmentCount = 0;
            SubpassDescriptions[2].pPreserveAttachments = 0;
        }
            
        VkSubpassDependency Dependencies[4] = {};
        Dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        Dependencies[0].dstSubpass = 0;
        Dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        Dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        Dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        Dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        Dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
            
        Dependencies[1].srcSubpass = 0;
        Dependencies[1].dstSubpass = 1;
        Dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        Dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        Dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        Dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        Dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
            
        Dependencies[2].srcSubpass = 0;
        Dependencies[2].dstSubpass = 2;
        Dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        Dependencies[2].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        Dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        Dependencies[2].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        Dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        Dependencies[3].srcSubpass = 2;
        Dependencies[3].dstSubpass = VK_SUBPASS_EXTERNAL;
        Dependencies[3].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        Dependencies[3].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        Dependencies[3].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        Dependencies[3].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        Dependencies[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
            
        VkRenderPassCreateInfo RenderPassCreateInfo = {};
        RenderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        RenderPassCreateInfo.pNext = 0;
        RenderPassCreateInfo.flags = 0;
        RenderPassCreateInfo.attachmentCount = ArrayCount(AttachmentDescriptions);
        RenderPassCreateInfo.pAttachments = AttachmentDescriptions;
        RenderPassCreateInfo.subpassCount = ArrayCount(SubpassDescriptions);
        RenderPassCreateInfo.pSubpasses = SubpassDescriptions;
        RenderPassCreateInfo.dependencyCount = ArrayCount(Dependencies);
        RenderPassCreateInfo.pDependencies = Dependencies;

        if (vkCreateRenderPass(GlobalState.Device, &RenderPassCreateInfo, 0, &GlobalState.RenderPass) != VK_SUCCESS)
        {
            InvalidCodePath;
        }
    }
     
    // NOTE: Create staging data
    {
        // NOTE: Create staging command pool
        VkCommandPoolCreateInfo CmdPoolCreateInfo = {};
        CmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        CmdPoolCreateInfo.pNext = 0;
        CmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        CmdPoolCreateInfo.queueFamilyIndex = GlobalState.TransferQueueFamId;

        if (vkCreateCommandPool(GlobalState.Device, &CmdPoolCreateInfo, 0, &GlobalState.TransferCmdPool) != VK_SUCCESS)
        {
            InvalidCodePath;
        }
        
        // NOTE: Create staging command buffers
        VkCommandBufferAllocateInfo CmdBufferAllocateInfo = {};
        CmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        CmdBufferAllocateInfo.pNext = 0;
        CmdBufferAllocateInfo.commandPool = GlobalState.GraphicsCmdPools[0];
        CmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        CmdBufferAllocateInfo.commandBufferCount = ArrayCount(GlobalState.StagingCmdBuffers);

        if (vkAllocateCommandBuffers(GlobalState.Device, &CmdBufferAllocateInfo, GlobalState.StagingCmdBuffers) != VK_SUCCESS)
        {
            InvalidCodePath;
        }

        // NOTE: Create staging buffers
        VkBufferCreateInfo BufferCreateInfo = {};
        BufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        BufferCreateInfo.pNext = 0;
        BufferCreateInfo.flags = 0;
        BufferCreateInfo.size = STAGING_BUFFER_SIZE;
        BufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        BufferCreateInfo.queueFamilyIndexCount = 0;
        BufferCreateInfo.pQueueFamilyIndices = 0;

        for (u32 BufferId = 0; BufferId < ArrayCount(GlobalState.StagingBuffers); ++BufferId)
        {
            VkBuffer* CurrBuffer = GlobalState.StagingBuffers + BufferId;

            if (vkCreateBuffer(GlobalState.Device, &BufferCreateInfo, 0, CurrBuffer) != VK_SUCCESS)
            {
                InvalidCodePath;
            }
        }
        
        VkMemoryRequirements BufferMemRequirements;
        vkGetBufferMemoryRequirements(GlobalState.Device, GlobalState.StagingBuffers[0], &BufferMemRequirements);

        for (u32 i = 0; i < GlobalState.MemoryProperties.memoryTypeCount; ++i)
        {
            if ((BufferMemRequirements.memoryTypeBits & (1 << i)) &&
                (GlobalState.MemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
            {
                VkMemoryAllocateInfo MemoryAllocateInfo = {};
                MemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                MemoryAllocateInfo.pNext = 0;
                MemoryAllocateInfo.allocationSize = BufferMemRequirements.size*STAGING_NUM_BUFFERS;
                MemoryAllocateInfo.memoryTypeIndex = i;

                if (vkAllocateMemory(GlobalState.Device, &MemoryAllocateInfo, 0, &GlobalState.StagingBufferMem) == VK_SUCCESS)
                {
                    break;
                }
            }
        }

        // NOTE: Create fences and bind memory to buffers
        VkFenceCreateInfo FenceCreateInfo = {};
        FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        FenceCreateInfo.pNext = 0;
        FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        u32 Offset = 0;
        for (u32 StagingId = 0; StagingId < ArrayCount(GlobalState.StagingBuffers); ++StagingId)
        {
            VkBuffer* CurrBuffer = GlobalState.StagingBuffers + StagingId;
            VkFence* CurrFence = GlobalState.StagingFences + StagingId;
            
            if (vkCreateFence(GlobalState.Device, &FenceCreateInfo, 0, CurrFence) != VK_SUCCESS)
            {
                InvalidCodePath;
            }

            if (vkBindBufferMemory(GlobalState.Device, *CurrBuffer, GlobalState.StagingBufferMem, Offset) != VK_SUCCESS)
            {
                InvalidCodePath;
            }

            Offset += (u32)BufferMemRequirements.size;
        }
    }

    // NOTE: Create GBuffer descriptor pools/layouts
    {
        VkDescriptorSetLayoutBinding LayoutBindings[3] = {};
        LayoutBindings[0].binding = 0;
        LayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        LayoutBindings[0].descriptorCount = 1;
        LayoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        LayoutBindings[0].pImmutableSamplers = 0;
            
        LayoutBindings[1].binding = 1;
        LayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        LayoutBindings[1].descriptorCount = 1;
        LayoutBindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        LayoutBindings[1].pImmutableSamplers = 0;
            
        LayoutBindings[2].binding = 2;
        LayoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        LayoutBindings[2].descriptorCount = 1;
        LayoutBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        LayoutBindings[2].pImmutableSamplers = 0;

        VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo = {};
        DescriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        DescriptorSetLayoutCreateInfo.pNext = 0;
        DescriptorSetLayoutCreateInfo.flags = 0;
        DescriptorSetLayoutCreateInfo.bindingCount = ArrayCount(LayoutBindings);
        DescriptorSetLayoutCreateInfo.pBindings = LayoutBindings;

        if (vkCreateDescriptorSetLayout(GlobalState.Device, &DescriptorSetLayoutCreateInfo,
                                        0, &GlobalState.GBufferDescriptorSetLayout) != VK_SUCCESS)
        {
            InvalidCodePath;
        }

        // NOTE: Create descriptor pool
        // TODO: Why do i have to specify descriptor count here as if i include all sets sizes?
        VkDescriptorPoolSize PoolSizes[3] = {};
        PoolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        PoolSizes[0].descriptorCount = 100;
        PoolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        PoolSizes[1].descriptorCount = 100;
        PoolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        PoolSizes[2].descriptorCount = 100;

        VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo = {};
        DescriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        DescriptorPoolCreateInfo.pNext = 0;
        DescriptorPoolCreateInfo.flags = 0;
        DescriptorPoolCreateInfo.maxSets = 100;
        DescriptorPoolCreateInfo.poolSizeCount = ArrayCount(PoolSizes);
        DescriptorPoolCreateInfo.pPoolSizes = PoolSizes;

        if (vkCreateDescriptorPool(GlobalState.Device, &DescriptorPoolCreateInfo, 0, &GlobalState.GBufferDescriptorPool) != VK_SUCCESS)
        {
            InvalidCodePath;
        }
    }

    // NOTE: Create point light descriptor pools/layouts
    {
        VkDescriptorSetLayoutBinding LayoutBindings[5] = {};
        LayoutBindings[0].binding = 0;
        LayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        LayoutBindings[0].descriptorCount = 1;
        LayoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        LayoutBindings[0].pImmutableSamplers = 0;
            
        LayoutBindings[1].binding = 1;
        LayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        LayoutBindings[1].descriptorCount = 1;
        LayoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        LayoutBindings[1].pImmutableSamplers = 0;

        LayoutBindings[2].binding = 2;
        LayoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        LayoutBindings[2].descriptorCount = 1;
        LayoutBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        LayoutBindings[2].pImmutableSamplers = 0;

        LayoutBindings[3].binding = 3;
        LayoutBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        LayoutBindings[3].descriptorCount = 1;
        LayoutBindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        LayoutBindings[3].pImmutableSamplers = 0;

        LayoutBindings[4].binding = 4;
        LayoutBindings[4].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        LayoutBindings[4].descriptorCount = 1;
        LayoutBindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        LayoutBindings[4].pImmutableSamplers = 0;

        VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo = {};
        DescriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        DescriptorSetLayoutCreateInfo.pNext = 0;
        DescriptorSetLayoutCreateInfo.flags = 0;
        DescriptorSetLayoutCreateInfo.bindingCount = ArrayCount(LayoutBindings);
        DescriptorSetLayoutCreateInfo.pBindings = LayoutBindings;

        if (vkCreateDescriptorSetLayout(GlobalState.Device, &DescriptorSetLayoutCreateInfo,
                                        0, &GlobalState.PointLightDescriptorSetLayout) != VK_SUCCESS)
        {
            InvalidCodePath;
        }

        // NOTE: Create descriptor pool
        // TODO: Why do i have to specify descriptor count here as if i include all sets sizes?
        VkDescriptorPoolSize PoolSizes[5] = {};
        PoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        PoolSizes[0].descriptorCount = 100 + 10;
        PoolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        PoolSizes[1].descriptorCount = 100 + 10;
        PoolSizes[2].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        PoolSizes[2].descriptorCount = 100 + 10;
        PoolSizes[3].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        PoolSizes[3].descriptorCount = 100 + 10;
        PoolSizes[4].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        PoolSizes[4].descriptorCount = 100 + 10;

        VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo = {};
        DescriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        DescriptorPoolCreateInfo.pNext = 0;
        DescriptorPoolCreateInfo.flags = 0;
        DescriptorPoolCreateInfo.maxSets = 100 + 10;
        DescriptorPoolCreateInfo.poolSizeCount = ArrayCount(PoolSizes);
        DescriptorPoolCreateInfo.pPoolSizes = PoolSizes;

        if (vkCreateDescriptorPool(GlobalState.Device, &DescriptorPoolCreateInfo, 0, &GlobalState.PointLightDescriptorPool) != VK_SUCCESS)
        {
            InvalidCodePath;
        }
    }
    
    // NOTE: Create gbuffer pipeline
    {
        // NOTE: Specfiy pipeline shaders
        VkShaderModule VertexShaderModule = CreateShaderModule("deferred_gbuffer_vert.spv");
        VkShaderModule FragmentShaderModule = CreateShaderModule("deferred_gbuffer_frag.spv");
            
        VkPipelineShaderStageCreateInfo ShaderStageCreateInfos[2] = {};
        ShaderStageCreateInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderStageCreateInfos[0].pNext = 0;
        ShaderStageCreateInfos[0].flags = 0;
        ShaderStageCreateInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        ShaderStageCreateInfos[0].module = VertexShaderModule;
        ShaderStageCreateInfos[0].pName = "main";
        ShaderStageCreateInfos[0].pSpecializationInfo = 0;
            
        ShaderStageCreateInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderStageCreateInfos[1].pNext = 0;
        ShaderStageCreateInfos[1].flags = 0;
        ShaderStageCreateInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        ShaderStageCreateInfos[1].module = FragmentShaderModule;
        ShaderStageCreateInfos[1].pName = "main";
        ShaderStageCreateInfos[1].pSpecializationInfo = 0;

        // NOTE: Specify input vertex data format
        VkVertexInputBindingDescription VertexBindingDescriptions[1] = {};
        VertexBindingDescriptions[0].binding = 0;
        VertexBindingDescriptions[0].stride = VERTEX_SIZE;
        VertexBindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription VertexAttributeDescriptions[3] = {};
        VertexAttributeDescriptions[0].location = 0;
        VertexAttributeDescriptions[0].binding = VertexBindingDescriptions[0].binding;
        VertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        VertexAttributeDescriptions[0].offset = 0;
            
        VertexAttributeDescriptions[2].location = 1;
        VertexAttributeDescriptions[2].binding = VertexBindingDescriptions[0].binding;
        VertexAttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        VertexAttributeDescriptions[2].offset = sizeof(v3);
            
        VertexAttributeDescriptions[1].location = 2;
        VertexAttributeDescriptions[1].binding = VertexBindingDescriptions[0].binding;
        VertexAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        VertexAttributeDescriptions[1].offset = sizeof(v3) + sizeof(v2);
                        
        VkPipelineVertexInputStateCreateInfo VertexInputStateCreateInfo = {};
        VertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        VertexInputStateCreateInfo.pNext = 0;
        VertexInputStateCreateInfo.flags = 0;
        VertexInputStateCreateInfo.vertexBindingDescriptionCount = ArrayCount(VertexBindingDescriptions);
        VertexInputStateCreateInfo.pVertexBindingDescriptions = VertexBindingDescriptions;
        VertexInputStateCreateInfo.vertexAttributeDescriptionCount = ArrayCount(VertexAttributeDescriptions);
        VertexInputStateCreateInfo.pVertexAttributeDescriptions = VertexAttributeDescriptions;

        // NOTE: Specify how vertices are connected
        VkPipelineInputAssemblyStateCreateInfo InputAssemblyStateCreateInfo = {};
        InputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        InputAssemblyStateCreateInfo.pNext = 0;
        InputAssemblyStateCreateInfo.flags = 0;
        InputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        InputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

        // NOTE: Specify view port info
        VkPipelineViewportStateCreateInfo ViewPortStateCreateInfo = {};
        ViewPortStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        ViewPortStateCreateInfo.pNext = 0;
        ViewPortStateCreateInfo.flags = 0;
        ViewPortStateCreateInfo.viewportCount = 1;
        ViewPortStateCreateInfo.pViewports = 0;
        ViewPortStateCreateInfo.scissorCount = 1;
        ViewPortStateCreateInfo.pScissors = 0;

        // NOTE: Specify rasterization flags
        VkPipelineRasterizationStateCreateInfo RasterizationStateCreateInfo = {};
        RasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        RasterizationStateCreateInfo.pNext = 0;
        RasterizationStateCreateInfo.flags = 0;
        RasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
        RasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
        RasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
        RasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        RasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        RasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
        RasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
        RasterizationStateCreateInfo.depthBiasClamp = 0.0f;
        RasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
        RasterizationStateCreateInfo.lineWidth = 1.0f;

        // NOTE: Set the multi sampling state
        VkPipelineMultisampleStateCreateInfo MultiSampleStateCreateInfo = {};
        MultiSampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        MultiSampleStateCreateInfo.pNext = 0;
        MultiSampleStateCreateInfo.flags = 0;
        MultiSampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        MultiSampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
        MultiSampleStateCreateInfo.minSampleShading = 1.0f;
        MultiSampleStateCreateInfo.pSampleMask = 0;
        MultiSampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
        MultiSampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

        // NOTE: Set the blending state
        VkPipelineColorBlendAttachmentState ColorBlendAttachmentStates[3] = {};
        ColorBlendAttachmentStates[0].blendEnable = VK_FALSE;
        ColorBlendAttachmentStates[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        ColorBlendAttachmentStates[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        ColorBlendAttachmentStates[0].colorBlendOp = VK_BLEND_OP_ADD;
        ColorBlendAttachmentStates[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        ColorBlendAttachmentStates[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        ColorBlendAttachmentStates[0].alphaBlendOp = VK_BLEND_OP_ADD;
        ColorBlendAttachmentStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        ColorBlendAttachmentStates[1].blendEnable = VK_FALSE;
        ColorBlendAttachmentStates[1].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        ColorBlendAttachmentStates[1].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        ColorBlendAttachmentStates[1].colorBlendOp = VK_BLEND_OP_ADD;
        ColorBlendAttachmentStates[1].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        ColorBlendAttachmentStates[1].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        ColorBlendAttachmentStates[1].alphaBlendOp = VK_BLEND_OP_ADD;
        ColorBlendAttachmentStates[1].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        ColorBlendAttachmentStates[2].blendEnable = VK_FALSE;
        ColorBlendAttachmentStates[2].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        ColorBlendAttachmentStates[2].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        ColorBlendAttachmentStates[2].colorBlendOp = VK_BLEND_OP_ADD;
        ColorBlendAttachmentStates[2].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        ColorBlendAttachmentStates[2].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        ColorBlendAttachmentStates[2].alphaBlendOp = VK_BLEND_OP_ADD;
        ColorBlendAttachmentStates[2].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo ColorBlendStateCreateInfo = {};
        ColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        ColorBlendStateCreateInfo.pNext = 0;
        ColorBlendStateCreateInfo.flags = 0;
        ColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
        ColorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
        ColorBlendStateCreateInfo.attachmentCount = ArrayCount(ColorBlendAttachmentStates);
        ColorBlendStateCreateInfo.pAttachments = ColorBlendAttachmentStates;
        ColorBlendStateCreateInfo.blendConstants[0] = 0.0f;
        ColorBlendStateCreateInfo.blendConstants[1] = 0.0f;
        ColorBlendStateCreateInfo.blendConstants[2] = 0.0f;
        ColorBlendStateCreateInfo.blendConstants[3] = 0.0f;

        // NOTE: Specify depth stencil state
        VkPipelineDepthStencilStateCreateInfo DepthStencilCreateInfo = {};
        DepthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        DepthStencilCreateInfo.pNext = 0;
        DepthStencilCreateInfo.flags = 0;
        DepthStencilCreateInfo.depthTestEnable = VK_TRUE;
        DepthStencilCreateInfo.depthWriteEnable = VK_TRUE;
        DepthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
        DepthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
        DepthStencilCreateInfo.stencilTestEnable = VK_FALSE;
        DepthStencilCreateInfo.front = {};
        DepthStencilCreateInfo.back = {};
            
        // NOTE: Describe how the input data is arranged in memory
        VkPipelineLayoutCreateInfo LayoutCreateInfo = {};
        LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        LayoutCreateInfo.pNext = 0;
        LayoutCreateInfo.flags = 0;
        LayoutCreateInfo.setLayoutCount = 1;
        LayoutCreateInfo.pSetLayouts = &GlobalState.GBufferDescriptorSetLayout;
        LayoutCreateInfo.pushConstantRangeCount = 0;
        LayoutCreateInfo.pPushConstantRanges = 0;

        if (vkCreatePipelineLayout(GlobalState.Device, &LayoutCreateInfo, 0, &GlobalState.GBufferPipelineLayout) != VK_SUCCESS)
        {
            InvalidCodePath;
        }

        // NOTE: Spec dynamic state
        VkDynamicState DynamicStates[2] = {};
        DynamicStates[0] = VK_DYNAMIC_STATE_VIEWPORT;
        DynamicStates[1] = VK_DYNAMIC_STATE_SCISSOR;

        VkPipelineDynamicStateCreateInfo DynamicStateCreateInfo = {};
        DynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        DynamicStateCreateInfo.pNext = 0;
        DynamicStateCreateInfo.flags = 0;
        DynamicStateCreateInfo.dynamicStateCount = ArrayCount(DynamicStates);
        DynamicStateCreateInfo.pDynamicStates = DynamicStates;
            
        VkGraphicsPipelineCreateInfo PipelineCreateInfo = {};
        PipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        PipelineCreateInfo.pNext = 0;
        PipelineCreateInfo.flags = 0;
        PipelineCreateInfo.stageCount = ArrayCount(ShaderStageCreateInfos);
        PipelineCreateInfo.pStages = ShaderStageCreateInfos;
        PipelineCreateInfo.pVertexInputState = &VertexInputStateCreateInfo;
        PipelineCreateInfo.pInputAssemblyState = &InputAssemblyStateCreateInfo;
        PipelineCreateInfo.pTessellationState = 0;
        PipelineCreateInfo.pViewportState = &ViewPortStateCreateInfo;
        PipelineCreateInfo.pRasterizationState = &RasterizationStateCreateInfo;
        PipelineCreateInfo.pMultisampleState = &MultiSampleStateCreateInfo;
        PipelineCreateInfo.pDepthStencilState = &DepthStencilCreateInfo;
        PipelineCreateInfo.pColorBlendState = &ColorBlendStateCreateInfo;
        PipelineCreateInfo.pDynamicState = &DynamicStateCreateInfo;
        PipelineCreateInfo.layout = GlobalState.GBufferPipelineLayout;
        PipelineCreateInfo.renderPass = GlobalState.RenderPass;
        PipelineCreateInfo.subpass = 0;
        PipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        PipelineCreateInfo.basePipelineIndex = -1;

        if (vkCreateGraphicsPipelines(GlobalState.Device, VK_NULL_HANDLE, 1, &PipelineCreateInfo,
                                      0, &GlobalState.GBufferPipeline) != VK_SUCCESS)
        {
            InvalidCodePath;
        }
    }
     
    // NOTE: Create point light pipeline
    {
        // NOTE: Specfiy pipeline shaders
        VkShaderModule VertexShaderModule = CreateShaderModule("deferred_point_light_vert.spv");
        VkShaderModule FragmentShaderModule = CreateShaderModule("deferred_point_light_frag.spv");
            
        VkPipelineShaderStageCreateInfo ShaderStageCreateInfos[2] = {};
        ShaderStageCreateInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderStageCreateInfos[0].pNext = 0;
        ShaderStageCreateInfos[0].flags = 0;
        ShaderStageCreateInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        ShaderStageCreateInfos[0].module = VertexShaderModule;
        ShaderStageCreateInfos[0].pName = "main";
        ShaderStageCreateInfos[0].pSpecializationInfo = 0;
            
        ShaderStageCreateInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderStageCreateInfos[1].pNext = 0;
        ShaderStageCreateInfos[1].flags = 0;
        ShaderStageCreateInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        ShaderStageCreateInfos[1].module = FragmentShaderModule;
        ShaderStageCreateInfos[1].pName = "main";
        ShaderStageCreateInfos[1].pSpecializationInfo = 0;

        // NOTE: Specify input vertex data format
        VkVertexInputBindingDescription VertexBindingDescriptions[1] = {};
        VertexBindingDescriptions[0].binding = 0;
        VertexBindingDescriptions[0].stride = VERTEX_SIZE;
        VertexBindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription VertexAttributeDescriptions[1] = {};
        VertexAttributeDescriptions[0].location = 0;
        VertexAttributeDescriptions[0].binding = VertexBindingDescriptions[0].binding;
        VertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        VertexAttributeDescriptions[0].offset = 0;
                        
        VkPipelineVertexInputStateCreateInfo VertexInputStateCreateInfo = {};
        VertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        VertexInputStateCreateInfo.pNext = 0;
        VertexInputStateCreateInfo.flags = 0;
        VertexInputStateCreateInfo.vertexBindingDescriptionCount = ArrayCount(VertexBindingDescriptions);
        VertexInputStateCreateInfo.pVertexBindingDescriptions = VertexBindingDescriptions;
        VertexInputStateCreateInfo.vertexAttributeDescriptionCount = ArrayCount(VertexAttributeDescriptions);
        VertexInputStateCreateInfo.pVertexAttributeDescriptions = VertexAttributeDescriptions;

        // NOTE: Specify how vertices are connected
        VkPipelineInputAssemblyStateCreateInfo InputAssemblyStateCreateInfo = {};
        InputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        InputAssemblyStateCreateInfo.pNext = 0;
        InputAssemblyStateCreateInfo.flags = 0;
        InputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        InputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

        // NOTE: Specify view port info
        VkPipelineViewportStateCreateInfo ViewPortStateCreateInfo = {};
        ViewPortStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        ViewPortStateCreateInfo.pNext = 0;
        ViewPortStateCreateInfo.flags = 0;
        ViewPortStateCreateInfo.viewportCount = 1;
        ViewPortStateCreateInfo.pViewports = 0;
        ViewPortStateCreateInfo.scissorCount = 1;
        ViewPortStateCreateInfo.pScissors = 0;

        // NOTE: Specify rasterization flags
        VkPipelineRasterizationStateCreateInfo RasterizationStateCreateInfo = {};
        RasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        RasterizationStateCreateInfo.pNext = 0;
        RasterizationStateCreateInfo.flags = 0;
        RasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
        RasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
        RasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
        RasterizationStateCreateInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
        RasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        RasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
        RasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
        RasterizationStateCreateInfo.depthBiasClamp = 0.0f;
        RasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
        RasterizationStateCreateInfo.lineWidth = 1.0f;

        // NOTE: Set the multi sampling state
        VkPipelineMultisampleStateCreateInfo MultiSampleStateCreateInfo = {};
        MultiSampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        MultiSampleStateCreateInfo.pNext = 0;
        MultiSampleStateCreateInfo.flags = 0;
        MultiSampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        MultiSampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
        MultiSampleStateCreateInfo.minSampleShading = 1.0f;
        MultiSampleStateCreateInfo.pSampleMask = 0;
        MultiSampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
        MultiSampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

        // NOTE: Set the belnding state
        VkPipelineColorBlendAttachmentState ColorBlendAttachmentStates[1] = {};
        ColorBlendAttachmentStates[0].blendEnable = VK_TRUE;
        ColorBlendAttachmentStates[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        ColorBlendAttachmentStates[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        ColorBlendAttachmentStates[0].colorBlendOp = VK_BLEND_OP_ADD;
        ColorBlendAttachmentStates[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        ColorBlendAttachmentStates[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        ColorBlendAttachmentStates[0].alphaBlendOp = VK_BLEND_OP_ADD;
        ColorBlendAttachmentStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo ColorBlendStateCreateInfo = {};
        ColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        ColorBlendStateCreateInfo.pNext = 0;
        ColorBlendStateCreateInfo.flags = 0;
        ColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
        ColorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
        ColorBlendStateCreateInfo.attachmentCount = ArrayCount(ColorBlendAttachmentStates);
        ColorBlendStateCreateInfo.pAttachments = ColorBlendAttachmentStates;
        ColorBlendStateCreateInfo.blendConstants[0] = 0.0f;
        ColorBlendStateCreateInfo.blendConstants[1] = 0.0f;
        ColorBlendStateCreateInfo.blendConstants[2] = 0.0f;
        ColorBlendStateCreateInfo.blendConstants[3] = 0.0f;

        // NOTE: Describe how the input data is arranged in memory
        VkPipelineLayoutCreateInfo LayoutCreateInfo = {};
        LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        LayoutCreateInfo.pNext = 0;
        LayoutCreateInfo.flags = 0;
        LayoutCreateInfo.setLayoutCount = 1;
        LayoutCreateInfo.pSetLayouts = &GlobalState.PointLightDescriptorSetLayout;
        LayoutCreateInfo.pushConstantRangeCount = 0;
        LayoutCreateInfo.pPushConstantRanges = 0;

        if (vkCreatePipelineLayout(GlobalState.Device, &LayoutCreateInfo, 0, &GlobalState.PointLightPipelineLayout) != VK_SUCCESS)
        {
            InvalidCodePath;
        }

        // NOTE: Spec dynamic state
        VkDynamicState DynamicStates[2] = {};
        DynamicStates[0] = VK_DYNAMIC_STATE_VIEWPORT;
        DynamicStates[1] = VK_DYNAMIC_STATE_SCISSOR;

        VkPipelineDynamicStateCreateInfo DynamicStateCreateInfo = {};
        DynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        DynamicStateCreateInfo.pNext = 0;
        DynamicStateCreateInfo.flags = 0;
        DynamicStateCreateInfo.dynamicStateCount = ArrayCount(DynamicStates);
        DynamicStateCreateInfo.pDynamicStates = DynamicStates;
            
        VkGraphicsPipelineCreateInfo PipelineCreateInfo = {};
        PipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        PipelineCreateInfo.pNext = 0;
        PipelineCreateInfo.flags = 0;
        PipelineCreateInfo.stageCount = ArrayCount(ShaderStageCreateInfos);
        PipelineCreateInfo.pStages = ShaderStageCreateInfos;
        PipelineCreateInfo.pVertexInputState = &VertexInputStateCreateInfo;
        PipelineCreateInfo.pInputAssemblyState = &InputAssemblyStateCreateInfo;
        PipelineCreateInfo.pTessellationState = 0;
        PipelineCreateInfo.pViewportState = &ViewPortStateCreateInfo;
        PipelineCreateInfo.pRasterizationState = &RasterizationStateCreateInfo;
        PipelineCreateInfo.pMultisampleState = &MultiSampleStateCreateInfo;
        PipelineCreateInfo.pDepthStencilState = 0;
        PipelineCreateInfo.pColorBlendState = &ColorBlendStateCreateInfo;
        PipelineCreateInfo.pDynamicState = &DynamicStateCreateInfo;
        PipelineCreateInfo.layout = GlobalState.PointLightPipelineLayout;
        PipelineCreateInfo.renderPass = GlobalState.RenderPass;
        PipelineCreateInfo.subpass = 1;
        PipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        PipelineCreateInfo.basePipelineIndex = -1;

        if (vkCreateGraphicsPipelines(GlobalState.Device, VK_NULL_HANDLE, 1, &PipelineCreateInfo,
                                      0, &GlobalState.PointLightPipeline) != VK_SUCCESS)
        {
            InvalidCodePath;
        }
    }

    // NOTE: Create dir light pipeline
    {
        // NOTE: Specfiy pipeline shaders
        VkShaderModule VertexShaderModule = CreateShaderModule("fullscreen_vert.spv");
        VkShaderModule FragmentShaderModule = CreateShaderModule("deferred_dir_light_frag.spv");
            
        VkPipelineShaderStageCreateInfo ShaderStageCreateInfos[2] = {};
        ShaderStageCreateInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderStageCreateInfos[0].pNext = 0;
        ShaderStageCreateInfos[0].flags = 0;
        ShaderStageCreateInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        ShaderStageCreateInfos[0].module = VertexShaderModule;
        ShaderStageCreateInfos[0].pName = "main";
        ShaderStageCreateInfos[0].pSpecializationInfo = 0;
            
        ShaderStageCreateInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderStageCreateInfos[1].pNext = 0;
        ShaderStageCreateInfos[1].flags = 0;
        ShaderStageCreateInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        ShaderStageCreateInfos[1].module = FragmentShaderModule;
        ShaderStageCreateInfos[1].pName = "main";
        ShaderStageCreateInfos[1].pSpecializationInfo = 0;

        // NOTE: Specify input vertex data format
        VkPipelineVertexInputStateCreateInfo VertexInputStateCreateInfo = {};
        VertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        VertexInputStateCreateInfo.pNext = 0;
        VertexInputStateCreateInfo.flags = 0;
        VertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
        VertexInputStateCreateInfo.pVertexBindingDescriptions = 0;
        VertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
        VertexInputStateCreateInfo.pVertexAttributeDescriptions = 0;

        // NOTE: Specify how vertices are connected
        VkPipelineInputAssemblyStateCreateInfo InputAssemblyStateCreateInfo = {};
        InputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        InputAssemblyStateCreateInfo.pNext = 0;
        InputAssemblyStateCreateInfo.flags = 0;
        InputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        InputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

        // NOTE: Specify view port info
        VkPipelineViewportStateCreateInfo ViewPortStateCreateInfo = {};
        ViewPortStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        ViewPortStateCreateInfo.pNext = 0;
        ViewPortStateCreateInfo.flags = 0;
        ViewPortStateCreateInfo.viewportCount = 1;
        ViewPortStateCreateInfo.pViewports = 0;
        ViewPortStateCreateInfo.scissorCount = 1;
        ViewPortStateCreateInfo.pScissors = 0;

        // NOTE: Specify rasterization flags
        VkPipelineRasterizationStateCreateInfo RasterizationStateCreateInfo = {};
        RasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        RasterizationStateCreateInfo.pNext = 0;
        RasterizationStateCreateInfo.flags = 0;
        RasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
        RasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
        RasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
        RasterizationStateCreateInfo.cullMode = VK_CULL_MODE_NONE;
        RasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        RasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
        RasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
        RasterizationStateCreateInfo.depthBiasClamp = 0.0f;
        RasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
        RasterizationStateCreateInfo.lineWidth = 1.0f;

        // NOTE: Set the multi sampling state
        VkPipelineMultisampleStateCreateInfo MultiSampleStateCreateInfo = {};
        MultiSampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        MultiSampleStateCreateInfo.pNext = 0;
        MultiSampleStateCreateInfo.flags = 0;
        MultiSampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        MultiSampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
        MultiSampleStateCreateInfo.minSampleShading = 1.0f;
        MultiSampleStateCreateInfo.pSampleMask = 0;
        MultiSampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
        MultiSampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

        // NOTE: Set the belnding state
        VkPipelineColorBlendAttachmentState ColorBlendAttachmentStates[1] = {};
        ColorBlendAttachmentStates[0].blendEnable = VK_TRUE;
        ColorBlendAttachmentStates[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        ColorBlendAttachmentStates[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        ColorBlendAttachmentStates[0].colorBlendOp = VK_BLEND_OP_ADD;
        ColorBlendAttachmentStates[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        ColorBlendAttachmentStates[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        ColorBlendAttachmentStates[0].alphaBlendOp = VK_BLEND_OP_ADD;
        ColorBlendAttachmentStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo ColorBlendStateCreateInfo = {};
        ColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        ColorBlendStateCreateInfo.pNext = 0;
        ColorBlendStateCreateInfo.flags = 0;
        ColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
        ColorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
        ColorBlendStateCreateInfo.attachmentCount = ArrayCount(ColorBlendAttachmentStates);
        ColorBlendStateCreateInfo.pAttachments = ColorBlendAttachmentStates;
        ColorBlendStateCreateInfo.blendConstants[0] = 0.0f;
        ColorBlendStateCreateInfo.blendConstants[1] = 0.0f;
        ColorBlendStateCreateInfo.blendConstants[2] = 0.0f;
        ColorBlendStateCreateInfo.blendConstants[3] = 0.0f;

        // NOTE: Specify depth stencil state
        VkPipelineDepthStencilStateCreateInfo DepthStencilCreateInfo = {};
        DepthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        DepthStencilCreateInfo.pNext = 0;
        DepthStencilCreateInfo.flags = 0;
        DepthStencilCreateInfo.depthTestEnable = VK_FALSE;
        DepthStencilCreateInfo.depthWriteEnable = VK_FALSE;
        DepthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_NEVER;
        DepthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
        DepthStencilCreateInfo.stencilTestEnable = VK_FALSE;
            
        // NOTE: Describe how the input data is arranged in memory
        VkPipelineLayoutCreateInfo LayoutCreateInfo = {};
        LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        LayoutCreateInfo.pNext = 0;
        LayoutCreateInfo.flags = 0;
        LayoutCreateInfo.setLayoutCount = 1;
        LayoutCreateInfo.pSetLayouts = &GlobalState.PointLightDescriptorSetLayout;
        LayoutCreateInfo.pushConstantRangeCount = 0;
        LayoutCreateInfo.pPushConstantRanges = 0;

        if (vkCreatePipelineLayout(GlobalState.Device, &LayoutCreateInfo, 0, &GlobalState.DirLightPipelineLayout) != VK_SUCCESS)
        {
            InvalidCodePath;
        }

        // NOTE: Spec dynamic state
        VkDynamicState DynamicStates[2] = {};
        DynamicStates[0] = VK_DYNAMIC_STATE_VIEWPORT;
        DynamicStates[1] = VK_DYNAMIC_STATE_SCISSOR;

        VkPipelineDynamicStateCreateInfo DynamicStateCreateInfo = {};
        DynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        DynamicStateCreateInfo.pNext = 0;
        DynamicStateCreateInfo.flags = 0;
        DynamicStateCreateInfo.dynamicStateCount = ArrayCount(DynamicStates);
        DynamicStateCreateInfo.pDynamicStates = DynamicStates;
            
        VkGraphicsPipelineCreateInfo PipelineCreateInfo = {};
        PipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        PipelineCreateInfo.pNext = 0;
        PipelineCreateInfo.flags = 0;
        PipelineCreateInfo.stageCount = ArrayCount(ShaderStageCreateInfos);
        PipelineCreateInfo.pStages = ShaderStageCreateInfos;
        PipelineCreateInfo.pVertexInputState = &VertexInputStateCreateInfo;
        PipelineCreateInfo.pInputAssemblyState = &InputAssemblyStateCreateInfo;
        PipelineCreateInfo.pTessellationState = 0;
        PipelineCreateInfo.pViewportState = &ViewPortStateCreateInfo;
        PipelineCreateInfo.pRasterizationState = &RasterizationStateCreateInfo;
        PipelineCreateInfo.pMultisampleState = &MultiSampleStateCreateInfo;
        PipelineCreateInfo.pDepthStencilState = &DepthStencilCreateInfo;
        PipelineCreateInfo.pColorBlendState = &ColorBlendStateCreateInfo;
        PipelineCreateInfo.pDynamicState = &DynamicStateCreateInfo;
        PipelineCreateInfo.layout = GlobalState.DirLightPipelineLayout;
        PipelineCreateInfo.renderPass = GlobalState.RenderPass;
        PipelineCreateInfo.subpass = 1;
        PipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        PipelineCreateInfo.basePipelineIndex = -1;

        if (vkCreateGraphicsPipelines(GlobalState.Device, VK_NULL_HANDLE, 1, &PipelineCreateInfo,
                                      0, &GlobalState.DirLightPipeline) != VK_SUCCESS)
        {
            InvalidCodePath;
        }
    }

    //
    // NOTE: Init Assets
    //
    {
        HANDLE FileHandle = CreateFileA("dragon.assets", GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
        if(FileHandle == INVALID_HANDLE_VALUE)
        {
            InvalidCodePath;
        }
    
        LARGE_INTEGER FileSize;
        if(!GetFileSizeEx(FileHandle, &FileSize))
        {
            InvalidCodePath;
        }
    
        u32 FileSize32 = SafeTruncateU64(FileSize.QuadPart);        
        u8* AssetFilePtr = (u8*)VirtualAlloc(0, FileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (!AssetFilePtr)
        {
            InvalidCodePath;
        }

        DWORD BytesRead;
        if(!(ReadFile(FileHandle, AssetFilePtr, FileSize32, &BytesRead, 0) && (FileSize32 == BytesRead)))
        {
            InvalidCodePath;
        }
        CloseHandle(FileHandle);
        GlobalState.Assets = LoadAssets(AssetFilePtr, &GlobalState.Arena);
        VirtualFree(AssetFilePtr, 0, MEM_RELEASE);
    }
    
    // NOTE: Create a texture sampler
    {
        // NOTE: Create a sampler
        VkSamplerCreateInfo SamplerCreateInfo = {};
        SamplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        SamplerCreateInfo.pNext = 0;
        SamplerCreateInfo.flags = 0;
        SamplerCreateInfo.magFilter = VK_FILTER_NEAREST;
        SamplerCreateInfo.minFilter = VK_FILTER_NEAREST;
        SamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        SamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        SamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        SamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        SamplerCreateInfo.mipLodBias = 0.0f;
        SamplerCreateInfo.anisotropyEnable = VK_FALSE;
        SamplerCreateInfo.maxAnisotropy = 1.0f;
        SamplerCreateInfo.compareEnable = VK_FALSE;
        SamplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        SamplerCreateInfo.minLod = 0.0f;
        SamplerCreateInfo.maxLod = 0.0f;
        SamplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        SamplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

        if (vkCreateSampler(GlobalState.Device, &SamplerCreateInfo, 0, &GlobalState.TextureSampler) != VK_SUCCESS)
        {
            InvalidCodePath;
        }
    }

    // NOTE: Create a world to render
    {
        // NOTE: Setup VP Transfrom
        f32 AspectRatio = (f32)(RENDER_WIDTH) / (f32)(RENDER_HEIGHT);
        f32 Fov = DegreeToRad(90.0f);
        f32 Near = 0.001f;
        f32 Far = 100.0f;
        m4 VPTransform = PerspProjMat(AspectRatio, Fov, Near, Far);
        
        // NOTE: Create a white texture for meshes
        u32 Width = 1;
        u32 Height = 1;
        u32 DataSize = sizeof(u32);
        u32 TextureData = 0xFFFFFFFF;

        VkImage WhiteTexture;
        VkImageView WhiteTextureView;
        CreateImage(&GlobalState.GpuAllocator, 1, 1, VK_FORMAT_R8G8B8A8_UNORM,
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, &WhiteTexture,
                    &WhiteTextureView);
        MoveImageToGpuMemory(&GlobalState.GpuAllocator, GlobalState.PrimaryCmdBuffer, DataSize, 1, 1, &TextureData, WhiteTexture);

        // NOTE: Allocate descriptor sets
        VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo = {};
        DescriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        DescriptorSetAllocateInfo.pNext = 0;
        DescriptorSetAllocateInfo.descriptorPool = GlobalState.GBufferDescriptorPool;
        DescriptorSetAllocateInfo.descriptorSetCount = 1;
        DescriptorSetAllocateInfo.pSetLayouts = &GlobalState.GBufferDescriptorSetLayout;

        // NOTE: Setup render mesh data
        for (i32 ModelZ = -1; ModelZ <= 1; ++ModelZ)
        {
            for (i32 ModelY = -1; ModelY <= 1; ++ModelY)
            {
                for (i32 ModelX = -1; ModelX <= 1; ++ModelX)
                {
                    asset_mesh** CurrMesh = GlobalState.MeshAssets + GlobalState.NumModels;
                    VkBuffer* CurrVertUniformBuffer = GlobalState.ModelVertUniformBuffers + GlobalState.NumModels;
                    VkBuffer* CurrFragUniformBuffer = GlobalState.ModelFragUniformBuffers + GlobalState.NumModels;
                    VkDescriptorSet* CurrDescriptorSet = GlobalState.ModelDescriptorSets + GlobalState.NumModels;
                    
                    // NOTE: Get model asset
                    asset_model* CurrModel = GetModel(&GlobalState.Assets, Model_Bunny);
                    *CurrMesh = CurrModel->Meshes + 0;

                    // NOTE: Setup vert uniform buffer
                    u32 VertUniformBufferSize = sizeof(gbuffer_vert_uniforms);
                    {
                        v3 ModelPos = V3(2.0f*(f32)ModelX, 2.0f*(f32)ModelY, -4.0f + (f32)ModelZ);
                        gbuffer_vert_uniforms VertUniforms = {};
                        VertUniforms.WTransform = PosMat(ModelPos.x, ModelPos.y, ModelPos.z)*ScaleMat(0.45, 0.45, 0.45);
                        VertUniforms.WVPTransform = VPTransform*VertUniforms.WTransform;

                        CreateBuffer(&GlobalState.GpuAllocator,
                                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VertUniformBufferSize,
                                     CurrVertUniformBuffer);
                        MoveBufferToGpuMemory(GlobalState.PrimaryCmdBuffer, *CurrVertUniformBuffer, (void*)&VertUniforms, VertUniformBufferSize);
                    }
                    
                    // NOTE: Setup frag uniform buffer
                    u32 FragUniformBufferSize = sizeof(gbuffer_frag_uniforms);
                    {
                        gbuffer_frag_uniforms FragUniforms = {};
                        FragUniforms.SpecularIntensity = 1.0f;
                        FragUniforms.SpecularPower = 16.0f;

                        CreateBuffer(&GlobalState.GpuAllocator,
                                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, FragUniformBufferSize,
                                     CurrFragUniformBuffer);
                        MoveBufferToGpuMemory(GlobalState.PrimaryCmdBuffer, *CurrFragUniformBuffer, (void*)&FragUniforms, FragUniformBufferSize);
                    }
                    
                    // NOTE: Setup model descriptor set
                    if (vkAllocateDescriptorSets(GlobalState.Device, &DescriptorSetAllocateInfo,
                                                 CurrDescriptorSet) != VK_SUCCESS)
                    {
                        InvalidCodePath;
                    }

                    VkDescriptorImageInfo ImageInfo = {};
                    ImageInfo.sampler = GlobalState.TextureSampler;
                    ImageInfo.imageView = WhiteTextureView;
                    ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                    VkDescriptorBufferInfo VertBufferInfo = {};
                    VertBufferInfo.buffer = *CurrVertUniformBuffer;
                    VertBufferInfo.offset = 0;
                    VertBufferInfo.range = VertUniformBufferSize;

                    VkDescriptorBufferInfo FragBufferInfo = {};
                    FragBufferInfo.buffer = *CurrFragUniformBuffer;
                    FragBufferInfo.offset = 0;
                    FragBufferInfo.range = FragUniformBufferSize;
            
                    VkWriteDescriptorSet DescriptorWrites[3] = {};
                    DescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    DescriptorWrites[0].pNext = 0;
                    DescriptorWrites[0].dstSet = *CurrDescriptorSet;
                    DescriptorWrites[0].dstBinding = 0;
                    DescriptorWrites[0].dstArrayElement = 0;
                    DescriptorWrites[0].descriptorCount = 1;
                    DescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    DescriptorWrites[0].pImageInfo = &ImageInfo;
                    DescriptorWrites[0].pBufferInfo = 0;
                    DescriptorWrites[0].pTexelBufferView = 0;
            
                    DescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    DescriptorWrites[1].pNext = 0;
                    DescriptorWrites[1].dstSet = *CurrDescriptorSet;
                    DescriptorWrites[1].dstBinding = 1;
                    DescriptorWrites[1].dstArrayElement = 0;
                    DescriptorWrites[1].descriptorCount = 1;
                    DescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    DescriptorWrites[1].pImageInfo = 0;
                    DescriptorWrites[1].pBufferInfo = &VertBufferInfo;
                    DescriptorWrites[1].pTexelBufferView = 0;

                    DescriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    DescriptorWrites[2].pNext = 0;
                    DescriptorWrites[2].dstSet = *CurrDescriptorSet;
                    DescriptorWrites[2].dstBinding = 2;
                    DescriptorWrites[2].dstArrayElement = 0;
                    DescriptorWrites[2].descriptorCount = 1;
                    DescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    DescriptorWrites[2].pImageInfo = 0;
                    DescriptorWrites[2].pBufferInfo = &FragBufferInfo;
                    DescriptorWrites[2].pTexelBufferView = 0;

                    vkUpdateDescriptorSets(GlobalState.Device, ArrayCount(DescriptorWrites), DescriptorWrites, 0, 0);

                    GlobalState.NumModels += 1;
                }
            }
        }

        // NOTE: Setup point light sphere model
        {
            asset_model* SphereModel = GetModel(&GlobalState.Assets, Model_Sphere);
            GlobalState.SphereMesh = SphereModel->Meshes + 0;
        }

        // NOTE: Setup point light data
        {
            // NOTE: Allocate descriptor sets
            VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo = {};
            DescriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            DescriptorSetAllocateInfo.pNext = 0;
            DescriptorSetAllocateInfo.descriptorPool = GlobalState.PointLightDescriptorPool;
            DescriptorSetAllocateInfo.descriptorSetCount = 1;
            DescriptorSetAllocateInfo.pSetLayouts = &GlobalState.PointLightDescriptorSetLayout;

            v3 RandomColors[] =
            {
                V3(1, 0, 0),
                V3(1, 1, 0),
                V3(0.2, 0, 1),
                V3(0.5, 1, 1),
                V3(0, 0.5, 0),
                V3(0, 0.5, 0.5),
                V3(0, 0.5, 0),
            };
            
            for (i32 LightZ = 0; LightZ <= 1; ++LightZ)
            {
                for (i32 LightY = -1; LightY <= 1; ++LightY)
                {
                    for (i32 LightX = -1; LightX <= 1; ++LightX)
                    {
                        VkBuffer* CurrVertUniformBuffer = GlobalState.PointLightVertUniformBuffers + GlobalState.NumPointLights;
                        VkBuffer* CurrFragUniformBuffer = GlobalState.PointLightFragUniformBuffers + GlobalState.NumPointLights;
                        VkDescriptorSet* CurrDescriptorSet = GlobalState.PointLightDescriptorSets + GlobalState.NumPointLights;

                        // NOTE: Point light meta data
                        v3 LightColor = RandomColors[(LightX+LightY+LightZ) % ArrayCount(RandomColors)];
                        f32 AmbientIntensity = 0.0f;
                        f32 DiffuseIntensity = 0.8f;
                        v3 LightPos = 0.5f*V3i(LightX, LightY, -3 + LightZ);
                        f32 Scale = 2.0f;
            
                        // NOTE: Setup vert uniform buffer
                        u32 VertUniformBufferSize = sizeof(point_light_vert_uniforms);
                        {
                            point_light_vert_uniforms VertUniforms = {};
                            VertUniforms.WVPTransform = VPTransform*PosMat(LightPos.x, LightPos.y, LightPos.z)*ScaleMat(Scale, Scale, Scale);

                            CreateBuffer(&GlobalState.GpuAllocator,
                                         VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VertUniformBufferSize,
                                         CurrVertUniformBuffer);
                            MoveBufferToGpuMemory(GlobalState.PrimaryCmdBuffer, *CurrVertUniformBuffer, (void*)&VertUniforms, VertUniformBufferSize);
                        }
                    
                        // NOTE: Setup frag uniform buffer
                        u32 FragUniformBufferSize = sizeof(point_light_frag_uniforms);
                        {
                            point_light_frag_uniforms FragUniforms = {};
                            FragUniforms.LightColor = LightColor;
                            FragUniforms.AmbientIntensity = AmbientIntensity;
                            FragUniforms.LightPos = LightPos;
                            FragUniforms.DiffuseIntensity = DiffuseIntensity;
                            FragUniforms.EyeWorldPos = V3(0, 0, 0); // TODO: Camera pos
                            FragUniforms.LightRadius = Scale;
                            
                            CreateBuffer(&GlobalState.GpuAllocator,
                                         VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, FragUniformBufferSize,
                                         CurrFragUniformBuffer);
                            MoveBufferToGpuMemory(GlobalState.PrimaryCmdBuffer, *CurrFragUniformBuffer, (void*)&FragUniforms, FragUniformBufferSize);
                        }
                    
                        // NOTE: Setup model descriptor set
                        if (vkAllocateDescriptorSets(GlobalState.Device, &DescriptorSetAllocateInfo,
                                                     CurrDescriptorSet) != VK_SUCCESS)
                        {
                            InvalidCodePath;
                        }

                        VkDescriptorBufferInfo VertBufferInfo = {};
                        VertBufferInfo.buffer = *CurrVertUniformBuffer;
                        VertBufferInfo.offset = 0;
                        VertBufferInfo.range = VertUniformBufferSize;

                        VkDescriptorBufferInfo FragBufferInfo = {};
                        FragBufferInfo.buffer = *CurrFragUniformBuffer;
                        FragBufferInfo.offset = 0;
                        FragBufferInfo.range = FragUniformBufferSize;

                        VkDescriptorImageInfo PosImageInfo = {};
                        PosImageInfo.sampler = VK_NULL_HANDLE;
                        PosImageInfo.imageView = GlobalState.GBuffer.WorldPosView;
                        PosImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                        VkDescriptorImageInfo DiffuseImageInfo = {};
                        DiffuseImageInfo.sampler = VK_NULL_HANDLE;
                        DiffuseImageInfo.imageView = GlobalState.GBuffer.DiffuseView;
                        DiffuseImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                        VkDescriptorImageInfo NormalImageInfo = {};
                        NormalImageInfo.sampler = VK_NULL_HANDLE;
                        NormalImageInfo.imageView = GlobalState.GBuffer.WorldNormalView;
                        NormalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            
                        VkWriteDescriptorSet DescriptorWrites[5] = {};
                        DescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        DescriptorWrites[0].pNext = 0;
                        DescriptorWrites[0].dstSet = *CurrDescriptorSet;
                        DescriptorWrites[0].dstBinding = 0;
                        DescriptorWrites[0].dstArrayElement = 0;
                        DescriptorWrites[0].descriptorCount = 1;
                        DescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                        DescriptorWrites[0].pImageInfo = 0;
                        DescriptorWrites[0].pBufferInfo = &VertBufferInfo;
                        DescriptorWrites[0].pTexelBufferView = 0;

                        DescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        DescriptorWrites[1].pNext = 0;
                        DescriptorWrites[1].dstSet = *CurrDescriptorSet;
                        DescriptorWrites[1].dstBinding = 1;
                        DescriptorWrites[1].dstArrayElement = 0;
                        DescriptorWrites[1].descriptorCount = 1;
                        DescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                        DescriptorWrites[1].pImageInfo = 0;
                        DescriptorWrites[1].pBufferInfo = &FragBufferInfo;
                        DescriptorWrites[1].pTexelBufferView = 0;

                        DescriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        DescriptorWrites[2].pNext = 0;
                        DescriptorWrites[2].dstSet = *CurrDescriptorSet;
                        DescriptorWrites[2].dstBinding = 2;
                        DescriptorWrites[2].dstArrayElement = 0;
                        DescriptorWrites[2].descriptorCount = 1;
                        DescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
                        DescriptorWrites[2].pImageInfo = &PosImageInfo;
                        DescriptorWrites[2].pBufferInfo = 0;
                        DescriptorWrites[2].pTexelBufferView = 0;

                        DescriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        DescriptorWrites[3].pNext = 0;
                        DescriptorWrites[3].dstSet = *CurrDescriptorSet;
                        DescriptorWrites[3].dstBinding = 3;
                        DescriptorWrites[3].dstArrayElement = 0;
                        DescriptorWrites[3].descriptorCount = 1;
                        DescriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
                        DescriptorWrites[3].pImageInfo = &DiffuseImageInfo;
                        DescriptorWrites[3].pBufferInfo = 0;
                        DescriptorWrites[3].pTexelBufferView = 0;

                        DescriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        DescriptorWrites[4].pNext = 0;
                        DescriptorWrites[4].dstSet = *CurrDescriptorSet;
                        DescriptorWrites[4].dstBinding = 4;
                        DescriptorWrites[4].dstArrayElement = 0;
                        DescriptorWrites[4].descriptorCount = 1;
                        DescriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
                        DescriptorWrites[4].pImageInfo = &NormalImageInfo;
                        DescriptorWrites[4].pBufferInfo = 0;
                        DescriptorWrites[4].pTexelBufferView = 0;

                        vkUpdateDescriptorSets(GlobalState.Device, ArrayCount(DescriptorWrites), DescriptorWrites, 0, 0);

                        GlobalState.NumPointLights += 1;

                    }
                }
            }
        }

        // NOTE: Setup dir light data
        {
            // NOTE: Allocate descriptor sets
            VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo = {};
            DescriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            DescriptorSetAllocateInfo.pNext = 0;
            DescriptorSetAllocateInfo.descriptorPool = GlobalState.PointLightDescriptorPool;
            DescriptorSetAllocateInfo.descriptorSetCount = 1;
            DescriptorSetAllocateInfo.pSetLayouts = &GlobalState.PointLightDescriptorSetLayout;
            
            VkBuffer* CurrUniformBuffer = GlobalState.DirLightUniformBuffers + GlobalState.NumDirLights;
            VkDescriptorSet* CurrDescriptorSet = GlobalState.DirLightDescriptorSets + GlobalState.NumDirLights;
            
            // NOTE: Setup vert uniform buffer
            u32 UniformBufferSize = sizeof(dir_light_uniforms);
            {
                dir_light_uniforms Uniforms = {};
                Uniforms.Color = V3(1, 1, 0);
                Uniforms.AmbientIntensity = 0.0f;
                Uniforms.Dir = V3(0, 0, -1);
                Uniforms.DiffuseIntensity = 0.4f;
                Uniforms.EyeWorldPos = V3(0, 0, 0); // TODO: Camera pos

                CreateBuffer(&GlobalState.GpuAllocator, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, UniformBufferSize, CurrUniformBuffer);
                MoveBufferToGpuMemory(GlobalState.PrimaryCmdBuffer, *CurrUniformBuffer, (void*)&Uniforms, UniformBufferSize);
            }
                    
            // NOTE: Setup model descriptor set
            if (vkAllocateDescriptorSets(GlobalState.Device, &DescriptorSetAllocateInfo,
                                         CurrDescriptorSet) != VK_SUCCESS)
            {
                InvalidCodePath;
            }

            VkDescriptorBufferInfo BufferInfo = {};
            BufferInfo.buffer = *CurrUniformBuffer;
            BufferInfo.offset = 0;
            BufferInfo.range = UniformBufferSize;

            VkDescriptorImageInfo PosImageInfo = {};
            PosImageInfo.sampler = VK_NULL_HANDLE;
            PosImageInfo.imageView = GlobalState.GBuffer.WorldPosView;
            PosImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkDescriptorImageInfo DiffuseImageInfo = {};
            DiffuseImageInfo.sampler = VK_NULL_HANDLE;
            DiffuseImageInfo.imageView = GlobalState.GBuffer.DiffuseView;
            DiffuseImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkDescriptorImageInfo NormalImageInfo = {};
            NormalImageInfo.sampler = VK_NULL_HANDLE;
            NormalImageInfo.imageView = GlobalState.GBuffer.WorldNormalView;
            NormalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            
            VkWriteDescriptorSet DescriptorWrites[4] = {};
            DescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[0].pNext = 0;
            DescriptorWrites[0].dstSet = *CurrDescriptorSet;
            DescriptorWrites[0].dstBinding = 1;
            DescriptorWrites[0].dstArrayElement = 0;
            DescriptorWrites[0].descriptorCount = 1;
            DescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            DescriptorWrites[0].pImageInfo = 0;
            DescriptorWrites[0].pBufferInfo = &BufferInfo;
            DescriptorWrites[0].pTexelBufferView = 0;

            DescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[1].pNext = 0;
            DescriptorWrites[1].dstSet = *CurrDescriptorSet;
            DescriptorWrites[1].dstBinding = 2;
            DescriptorWrites[1].dstArrayElement = 0;
            DescriptorWrites[1].descriptorCount = 1;
            DescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            DescriptorWrites[1].pImageInfo = &PosImageInfo;
            DescriptorWrites[1].pBufferInfo = 0;
            DescriptorWrites[1].pTexelBufferView = 0;

            DescriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[2].pNext = 0;
            DescriptorWrites[2].dstSet = *CurrDescriptorSet;
            DescriptorWrites[2].dstBinding = 3;
            DescriptorWrites[2].dstArrayElement = 0;
            DescriptorWrites[2].descriptorCount = 1;
            DescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            DescriptorWrites[2].pImageInfo = &DiffuseImageInfo;
            DescriptorWrites[2].pBufferInfo = 0;
            DescriptorWrites[2].pTexelBufferView = 0;

            DescriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[3].pNext = 0;
            DescriptorWrites[3].dstSet = *CurrDescriptorSet;
            DescriptorWrites[3].dstBinding = 4;
            DescriptorWrites[3].dstArrayElement = 0;
            DescriptorWrites[3].descriptorCount = 1;
            DescriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            DescriptorWrites[3].pImageInfo = &NormalImageInfo;
            DescriptorWrites[3].pBufferInfo = 0;
            DescriptorWrites[3].pTexelBufferView = 0;

            vkUpdateDescriptorSets(GlobalState.Device, ArrayCount(DescriptorWrites), DescriptorWrites, 0, 0);

            GlobalState.NumDirLights += 1;
        }
    }
    
    LARGE_INTEGER TimerFrequency;
    QueryPerformanceFrequency(&TimerFrequency);
    GlobalState.TimerFrequency = TimerFrequency.QuadPart;

    LARGE_INTEGER LastFrameTime = Win32GetClock();

    while (GlobalState.GameIsRunning)
    {
        // NOTE: Process inputs
        {
            MSG Message;
            while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
            {
                switch (Message.message)
                {
                    case WM_QUIT:
                    {
                        GlobalState.GameIsRunning = false;
                    } break;

                    case WM_SYSKEYDOWN:
                    case WM_SYSKEYUP:
                    case WM_KEYDOWN:
                    case WM_KEYUP:
                    {
                        u32 VKCode = (u32)Message.wParam;
                        b32 WasDown = ((Message.lParam & (1 << 30)) != 0);
                        b32 IsDown = ((Message.lParam & (1 << 31)) == 0);
                    } break;
                    
                    default:
                    {
                        TranslateMessage(&Message);
                        DispatchMessageA(&Message);
                    }
                }
            }
        }
        
        // NOTE: Record commands into command buffers
        {
            // NOTE: Wait for frame to finish rendering on GPU
            if (vkWaitForFences(GlobalState.Device, 1, &GlobalState.Fence, VK_TRUE, 1000000000) != VK_SUCCESS)
            {
                InvalidCodePath;
            }
            vkResetFences(GlobalState.Device, 1, &GlobalState.Fence);

            u32 ImageIndex;
            VkResult Result = vkAcquireNextImageKHR(GlobalState.Device, GlobalState.SwapChain,
                                                    UINT64_MAX, GlobalState.ImageAvailableSemaphore,
                                                    VK_NULL_HANDLE, &ImageIndex);

            // NOTE: Create frame buffer for render pass
            {
                GlobalState.GBuffer.OutputView = GlobalState.SwapChainViews[ImageIndex];
                if (GlobalState.GBuffer.FrameBuffer != VK_NULL_HANDLE)
                {
                    vkDestroyFramebuffer(GlobalState.Device, GlobalState.GBuffer.FrameBuffer, 0);
                }
            
                VkFramebufferCreateInfo GBufferCreateInfo = {};
                GBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                GBufferCreateInfo.pNext = 0;
                GBufferCreateInfo.flags = 0;
                GBufferCreateInfo.renderPass = GlobalState.RenderPass;
                GBufferCreateInfo.attachmentCount = ArrayCount(GlobalState.GBuffer.Views);
                GBufferCreateInfo.pAttachments = GlobalState.GBuffer.Views;
                GBufferCreateInfo.width = RENDER_WIDTH;
                GBufferCreateInfo.height = RENDER_HEIGHT;
                GBufferCreateInfo.layers = 1;

                if (vkCreateFramebuffer(GlobalState.Device, &GBufferCreateInfo, 0, &GlobalState.GBuffer.FrameBuffer) != VK_SUCCESS)
                {
                    InvalidCodePath;
                }
            }
            
            switch (Result)
            {
                case VK_SUCCESS:
                case VK_SUBOPTIMAL_KHR:
                {
                } break;

                case VK_ERROR_OUT_OF_DATE_KHR:
                {
                    // NOTE: Window size changed
                    InvalidCodePath;
                } break;
            
                default:
                {
                    InvalidCodePath;
                } break;
            }

            VkCommandBufferBeginInfo CmdBufferBeginInfo = {};
            CmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            CmdBufferBeginInfo.pNext = 0;
            CmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            CmdBufferBeginInfo.pInheritanceInfo = 0;

            vkBeginCommandBuffer(GlobalState.PrimaryCmdBuffer, &CmdBufferBeginInfo);

            VkImageSubresourceRange ImageSubResourceRange = {};
            ImageSubResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            ImageSubResourceRange.baseMipLevel = 0;
            ImageSubResourceRange.levelCount = 1;
            ImageSubResourceRange.baseArrayLayer = 0;
            ImageSubResourceRange.layerCount = 1;

            if (GlobalState.GraphicsQueueFamId != GlobalState.PresentQueueFamId)
            {
                // NOTE: Change image format for each op
                VkImageMemoryBarrier BarrierFromPresentToClear = {};
                BarrierFromPresentToClear.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                BarrierFromPresentToClear.pNext = 0;
                BarrierFromPresentToClear.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                BarrierFromPresentToClear.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                BarrierFromPresentToClear.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                BarrierFromPresentToClear.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                BarrierFromPresentToClear.srcQueueFamilyIndex = GlobalState.PresentQueueFamId;
                BarrierFromPresentToClear.dstQueueFamilyIndex = GlobalState.GraphicsQueueFamId;
                BarrierFromPresentToClear.image = GlobalState.SwapChainImgs[ImageIndex];
                BarrierFromPresentToClear.subresourceRange = ImageSubResourceRange;

                vkCmdPipelineBarrier(GlobalState.PrimaryCmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, 0, 0, 0, 1,
                                     &BarrierFromPresentToClear);
            }
            
            VkClearValue ClearColors[5] = {};
            // TODO: Set the Z to the far plane so that the light never affects it
            ClearColors[0].color = { 0.0f, 0.0f, 1000.0f, 0.0f };
            ClearColors[1].color = { 0.0f, 0.0f, 0.0f, 0.0f };
            ClearColors[2].color = { 0.0f, 0.0f, 0.0f, 0.0f };
            ClearColors[3].depthStencil = { 1.0f, 0 };
            ClearColors[4].color = { 0.0f, 0.0f, 0.0f, 0.0f };
            
            VkRenderPassBeginInfo RenderPassBeginInfo = {};
            RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            RenderPassBeginInfo.pNext = 0;
            RenderPassBeginInfo.renderPass = GlobalState.RenderPass;
            RenderPassBeginInfo.framebuffer = GlobalState.GBuffer.FrameBuffer;
            RenderPassBeginInfo.renderArea.offset = { 0, 0 };
            RenderPassBeginInfo.renderArea.extent = GlobalState.SwapChainExtent;
            RenderPassBeginInfo.clearValueCount = ArrayCount(ClearColors);
            RenderPassBeginInfo.pClearValues = ClearColors;

            vkCmdBeginRenderPass(GlobalState.PrimaryCmdBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

            VkViewport ViewPort = {};
            ViewPort.x = 0.0f;
            ViewPort.y = 0.0f;
            ViewPort.width = (f32)GlobalState.SwapChainExtent.width;
            ViewPort.height = (f32)GlobalState.SwapChainExtent.height;
            ViewPort.minDepth = 0.0f;
            ViewPort.maxDepth = 1.0f;

            VkRect2D Scissor =
            {
                {
                    0,
                    0,
                },
                {
                    GlobalState.SwapChainExtent.width,
                    GlobalState.SwapChainExtent.height,
                },
            };

            vkCmdSetViewport(GlobalState.PrimaryCmdBuffer, 0, 1, &ViewPort);
            vkCmdSetScissor(GlobalState.PrimaryCmdBuffer, 0, 1, &Scissor);

            // NOTE: Render models to gbuffer
            vkCmdBindPipeline(GlobalState.PrimaryCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GlobalState.GBufferPipeline);

            for (u32 ModelId = 0; ModelId < GlobalState.NumModels; ++ModelId)
            {
                asset_mesh* CurrMesh = GlobalState.MeshAssets[ModelId];
                
                VkDeviceSize Offset = 0;
                vkCmdBindVertexBuffers(GlobalState.PrimaryCmdBuffer, 0, 1, &CurrMesh->Handle, &Offset);
                vkCmdBindDescriptorSets(GlobalState.PrimaryCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GlobalState.GBufferPipelineLayout, 0, 1, GlobalState.ModelDescriptorSets + ModelId, 0, 0);
                vkCmdDraw(GlobalState.PrimaryCmdBuffer, CurrMesh->NumVertices, 1, 0, 0);
            }
            
            // NOTE: Render point lights
            vkCmdNextSubpass(GlobalState.PrimaryCmdBuffer, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(GlobalState.PrimaryCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GlobalState.PointLightPipeline);

            for (u32 PointLightId = 0; PointLightId < GlobalState.NumPointLights; ++PointLightId)
            {
                VkDeviceSize Offset = 0;
                vkCmdBindVertexBuffers(GlobalState.PrimaryCmdBuffer, 0, 1, &GlobalState.SphereMesh->Handle, &Offset);
                vkCmdBindDescriptorSets(GlobalState.PrimaryCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GlobalState.PointLightPipelineLayout, 0, 1, GlobalState.PointLightDescriptorSets + PointLightId, 0, 0);
                vkCmdDraw(GlobalState.PrimaryCmdBuffer, GlobalState.SphereMesh->NumVertices, 1, 0, 0);
            }

            // NOTE: Render dir light
            vkCmdNextSubpass(GlobalState.PrimaryCmdBuffer, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(GlobalState.PrimaryCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GlobalState.DirLightPipeline);

            for (u32 DirLightId = 0; DirLightId < GlobalState.NumDirLights; ++DirLightId)
            {
                vkCmdBindDescriptorSets(GlobalState.PrimaryCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GlobalState.PointLightPipelineLayout, 0, 1, GlobalState.DirLightDescriptorSets + DirLightId, 0, 0);
                vkCmdDraw(GlobalState.PrimaryCmdBuffer, 6, 1, 0, 0);
            }
            
            vkCmdEndRenderPass(GlobalState.PrimaryCmdBuffer);

            if (GlobalState.GraphicsQueueFamId != GlobalState.PresentQueueFamId)
            {
                VkImageMemoryBarrier BarrierFromClearToPresent = {};
                BarrierFromClearToPresent.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                BarrierFromClearToPresent.pNext = 0;
                BarrierFromClearToPresent.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                BarrierFromClearToPresent.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                BarrierFromClearToPresent.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                BarrierFromClearToPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                BarrierFromClearToPresent.srcQueueFamilyIndex = GlobalState.GraphicsQueueFamId;
                BarrierFromClearToPresent.dstQueueFamilyIndex = GlobalState.PresentQueueFamId;
                BarrierFromClearToPresent.image = GlobalState.SwapChainImgs[ImageIndex];
                BarrierFromClearToPresent.subresourceRange = ImageSubResourceRange;
                
                vkCmdPipelineBarrier(GlobalState.PrimaryCmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                     VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, 0, 0, 0, 1,
                                     &BarrierFromClearToPresent);
            }
            
            if (vkEndCommandBuffer(GlobalState.PrimaryCmdBuffer) != VK_SUCCESS)
            {
                InvalidCodePath;
            }
        
            // NOTE: Render to our window surface
            // NOTE: Tell queue where we render to surface to wait
            VkPipelineStageFlags WaitDstMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            VkSubmitInfo SubmitInfo = {};
            SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            SubmitInfo.pNext = 0;
            SubmitInfo.waitSemaphoreCount = 1;
            SubmitInfo.pWaitSemaphores = &GlobalState.ImageAvailableSemaphore;
            SubmitInfo.pWaitDstStageMask = &WaitDstMask;
            SubmitInfo.commandBufferCount = 1;
            SubmitInfo.pCommandBuffers = &GlobalState.PrimaryCmdBuffer;
            SubmitInfo.signalSemaphoreCount = 1;
            SubmitInfo.pSignalSemaphores = &GlobalState.FinishedRenderingSemaphore;

            if (vkQueueSubmit(GlobalState.GraphicsQueue, 1, &SubmitInfo, GlobalState.Fence) != VK_SUCCESS)
            {
                InvalidCodePath;
            }

            VkPresentInfoKHR PresentInfo = {};
            PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            PresentInfo.pNext = 0;
            PresentInfo.waitSemaphoreCount = 1;
            PresentInfo.pWaitSemaphores = &GlobalState.FinishedRenderingSemaphore;
            PresentInfo.swapchainCount = 1;
            PresentInfo.pSwapchains = &GlobalState.SwapChain;
            PresentInfo.pImageIndices = &ImageIndex;
            PresentInfo.pResults = 0;

            Result = vkQueuePresentKHR(GlobalState.PresentQueue, &PresentInfo);

            switch (Result)
            {
                case VK_SUCCESS:
                {
                } break;

                case VK_ERROR_OUT_OF_DATE_KHR:
                case VK_SUBOPTIMAL_KHR:
                {
                    // NOTE: Window size changed
                    InvalidCodePath;
                } break;

                default:
                {
                    InvalidCodePath;
                } break;
            }
        }
    }

    // NOTE: Cleanup vulkan to avoid crash at program close
    if (GlobalState.Device != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(GlobalState.Device);
        vkDestroyDevice(GlobalState.Device, 0);
    }

    if (GlobalState.Instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(GlobalState.Instance, 0);
    }

    return 0;
}
