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
inline void MoveBufferToGpuMemory(VkBuffer Buffer, u32 BufferSize, void* Data);
inline void MoveImageToGpuMemory(VkImage Image, u32 PixelSize, u32 Width, u32 Height, void* Data);

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
    VK_DEVICE_LEVEL_FUNC(   vkCreateComputePipelines );                 \
    VK_DEVICE_LEVEL_FUNC(   vkCmdDispatch );                            \
    VK_DEVICE_LEVEL_FUNC(   vkCmdPushConstants );                       \
    VK_DEVICE_LEVEL_FUNC(   vkCmdFillBuffer );                          \


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

inline void MoveBufferToGpuMemory(VkBuffer Buffer, u32 BufferSize, void* Data)
{
    memcpy(GlobalState.TransferBufferMemPtr, Data, BufferSize);
        
    // NOTE: Tells driver which parts of mem where modified
    VkMappedMemoryRange FlushRange = {};
    FlushRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    FlushRange.pNext = 0;
    FlushRange.memory = GlobalState.TransferBufferMem;
    FlushRange.offset = 0;
    FlushRange.size = BufferSize;
    vkFlushMappedMemoryRanges(GlobalState.Device, 1, &FlushRange);

    // NOTE: Copy data to device local memory
    VkCommandBufferBeginInfo CmdBufferBeginInfo = {};
    CmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CmdBufferBeginInfo.pNext = 0;
    CmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    CmdBufferBeginInfo.pInheritanceInfo = 0;
            
    if (vkBeginCommandBuffer(GlobalState.TransferCmdBuffer, &CmdBufferBeginInfo) != VK_SUCCESS)
    {
        InvalidCodePath;
    }

    VkBufferCopy BufferCopyInfo = {};
    BufferCopyInfo.srcOffset = 0;
    BufferCopyInfo.dstOffset = 0;
    BufferCopyInfo.size = BufferSize;    
    vkCmdCopyBuffer(GlobalState.TransferCmdBuffer, GlobalState.TransferBuffer, Buffer, 1, &BufferCopyInfo);
    
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
    vkCmdPipelineBarrier(GlobalState.TransferCmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, 0, 1, &BufferMemoryBarrier, 0, 0);
    
    if (vkEndCommandBuffer(GlobalState.TransferCmdBuffer) != VK_SUCCESS)
    {
        InvalidCodePath;
    }

    VkSubmitInfo SubmitInfo = {};
    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.pNext = 0;
    SubmitInfo.waitSemaphoreCount = 0;
    SubmitInfo.pWaitSemaphores = 0;
    SubmitInfo.pWaitDstStageMask = 0;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &GlobalState.TransferCmdBuffer;
    SubmitInfo.signalSemaphoreCount = 0;
    SubmitInfo.pSignalSemaphores = 0;
    if (vkQueueSubmit(GlobalState.TransferQueue, 1, &SubmitInfo, GlobalState.TransferFence) != VK_SUCCESS)
    {
        InvalidCodePath;
    }

    if (vkWaitForFences(GlobalState.Device, 1, &GlobalState.TransferFence, VK_TRUE, 1000000000) != VK_SUCCESS)
    {
        InvalidCodePath;
    }
    vkResetFences(GlobalState.Device, 1, &GlobalState.TransferFence);
}

inline void MoveImageToGpuMemory(VkImage Image, u32 PixelSize, u32 Width, u32 Height, void* Data)
{
    // TODO: Shouldn't this just work?
    if (Width == 0 || Height == 0)
    {
        return;
    }

    u32 ImageSize = Width*Height*PixelSize;
    memcpy(GlobalState.TransferBufferMemPtr, Data, ImageSize);
        
    // NOTE: Tells driver which parts of mem where modified
    VkMappedMemoryRange FlushRange = {};
    FlushRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    FlushRange.pNext = 0;
    FlushRange.memory = GlobalState.TransferBufferMem;
    FlushRange.offset = 0;
    FlushRange.size = ImageSize;
    vkFlushMappedMemoryRanges(GlobalState.Device, 1, &FlushRange);

    // NOTE: Copy texels from staging buffer to gpu local mem
    VkCommandBufferBeginInfo CmdBufferBeginInfo = {};
    CmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CmdBufferBeginInfo.pNext = 0;
    CmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    CmdBufferBeginInfo.pInheritanceInfo = 0;

    if (vkBeginCommandBuffer(GlobalState.TransferCmdBuffer, &CmdBufferBeginInfo) != VK_SUCCESS)
    {
        InvalidCodePath;
    }

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

    vkCmdPipelineBarrier(GlobalState.TransferCmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
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
    vkCmdCopyBufferToImage(GlobalState.TransferCmdBuffer, GlobalState.TransferBuffer, Image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &BufferImgCopyInfo);

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
    vkCmdPipelineBarrier(GlobalState.TransferCmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, 0, 0, 0, 1, &ImgMemoryBarrierFromTransferToShaderRead);
    if (vkEndCommandBuffer(GlobalState.TransferCmdBuffer) != VK_SUCCESS)
    {
        InvalidCodePath;
    }

    VkSubmitInfo SubmitInfo = {};
    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.pNext = 0;
    SubmitInfo.waitSemaphoreCount = 0;
    SubmitInfo.pWaitSemaphores = 0;
    SubmitInfo.pWaitDstStageMask = 0;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &GlobalState.TransferCmdBuffer;
    SubmitInfo.signalSemaphoreCount = 0;
    SubmitInfo.pSignalSemaphores = 0;
    if (vkQueueSubmit(GlobalState.TransferQueue, 1, &SubmitInfo, GlobalState.TransferFence) != VK_SUCCESS)
    {
        InvalidCodePath;
    }

    if (vkWaitForFences(GlobalState.Device, 1, &GlobalState.TransferFence, VK_TRUE, 1000000000) != VK_SUCCESS)
    {
        InvalidCodePath;
    }
    vkResetFences(GlobalState.Device, 1, &GlobalState.TransferFence);
}

internal vk_pipeline CreateComputePipeline(char* ShaderName, VkDescriptorSetLayout* Layouts,
                                           u32 NumLayouts, VkPushConstantRange* ConstRanges,
                                           u32 ConstRangeCount)
{
    vk_pipeline Result = {};
    
    // NOTE: Specfiy pipeline shaders
    VkShaderModule ComputeShaderModule = CreateShaderModule(ShaderName);
    VkPipelineShaderStageCreateInfo ShaderStageCreateInfo = {};
    ShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStageCreateInfo.pNext = 0;
    ShaderStageCreateInfo.flags = 0;
    ShaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    ShaderStageCreateInfo.module = ComputeShaderModule;
    ShaderStageCreateInfo.pName = "main";
    ShaderStageCreateInfo.pSpecializationInfo = 0;
            
    VkPipelineLayoutCreateInfo LayoutCreateInfo = {};
    LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    LayoutCreateInfo.pNext = 0;
    LayoutCreateInfo.flags = 0;
    LayoutCreateInfo.setLayoutCount = NumLayouts;
    LayoutCreateInfo.pSetLayouts = Layouts;
    LayoutCreateInfo.pushConstantRangeCount = ConstRangeCount;
    LayoutCreateInfo.pPushConstantRanges = ConstRanges;

    if (vkCreatePipelineLayout(GlobalState.Device, &LayoutCreateInfo, 0, &Result.Layout) != VK_SUCCESS)
    {
        InvalidCodePath;
    }

    VkComputePipelineCreateInfo PipelineCreateInfo = {};
    PipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    PipelineCreateInfo.pNext = 0;
    PipelineCreateInfo.flags = 0;
    PipelineCreateInfo.stage = ShaderStageCreateInfo;
    PipelineCreateInfo.layout = Result.Layout;
    PipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    PipelineCreateInfo.basePipelineIndex = -1;

    VkResult CreateResult = vkCreateComputePipelines(GlobalState.Device, VK_NULL_HANDLE, 1, &PipelineCreateInfo,
                                                     0, &Result.Pipeline);
    if (CreateResult != VK_SUCCESS)
    {
        InvalidCodePath;
    }

    return Result;
}

inline VkDescriptorSetLayout CreateDescriptorSetLayout(VkDescriptorSetLayoutBinding* Bindings,
                                                       u32 NumBindings)
{
    VkDescriptorSetLayout Result;

    VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo = {};
    DescriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    DescriptorSetLayoutCreateInfo.pNext = 0;
    DescriptorSetLayoutCreateInfo.flags = 0;
    DescriptorSetLayoutCreateInfo.bindingCount = NumBindings;
    DescriptorSetLayoutCreateInfo.pBindings = Bindings;

    if (vkCreateDescriptorSetLayout(GlobalState.Device, &DescriptorSetLayoutCreateInfo,
                                    0, &Result) != VK_SUCCESS)
    {
        InvalidCodePath;
    }

    return Result;
}

inline VkDescriptorPool CreateDescriptorPool(VkDescriptorPoolSize* PoolSizes, u32 NumPoolSizes,
                                             u32 MaxSets)
{
    VkDescriptorPool Result;
    
    VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo = {};
    DescriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    DescriptorPoolCreateInfo.pNext = 0;
    DescriptorPoolCreateInfo.flags = 0;
    DescriptorPoolCreateInfo.maxSets = MaxSets;
    DescriptorPoolCreateInfo.poolSizeCount = NumPoolSizes;
    DescriptorPoolCreateInfo.pPoolSizes = PoolSizes;

    if (vkCreateDescriptorPool(GlobalState.Device, &DescriptorPoolCreateInfo, 0, &Result) != VK_SUCCESS)
    {
        InvalidCodePath;
    }

    return Result;
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

    // NOTE: Setup VP Transfrom
    {
        f32 AspectRatio = (f32)(RENDER_WIDTH) / (f32)(RENDER_HEIGHT);
        f32 Fov = DegreeToRad(90.0f);
        GlobalState.NearZ = 0.001f;
        GlobalState.FarZ = 100.0f;
        GlobalState.VTransform = IdentityM4();
        GlobalState.PTransform = PerspProjMat(AspectRatio, Fov, GlobalState.NearZ, GlobalState.FarZ);
        GlobalState.VPTransform = GlobalState.PTransform*GlobalState.VTransform;
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

        GlobalState.TransferQueueFamId = UINT32_MAX;
        GlobalState.GraphicsQueueFamId = UINT32_MAX;
        GlobalState.PresentQueueFamId = UINT32_MAX;
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
                if (QueueFamilyProperties[FamilyId].queueCount > 0)
                {
                    if (QueueFamilyProperties[FamilyId].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                    {
                        // IMPORTANT: We can have diff queues for graphics, present. We may want
                        // to handle that case
                        // NOTE: We take queue that supports graphics and present
                        if (PresentQueueSupport)
                        {
                            GlobalState.GraphicsQueueFamId = FamilyId;
                            GlobalState.PresentQueueFamId = FamilyId;
                            SelectedPhysicalDevice = CurrDevice;
                        }
                    }
                    else if (QueueFamilyProperties[FamilyId].queueFlags & VK_QUEUE_TRANSFER_BIT)
                    {
                        GlobalState.TransferQueueFamId = FamilyId;
                    }
                    else if (QueueFamilyProperties[FamilyId].queueFlags & VK_QUEUE_COMPUTE_BIT)
                    {
                        GlobalState.ComputeQueueFamId = FamilyId;
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
        VkDeviceQueueCreateInfo QueueCreateInfos[4] = {};

        ++NumQueues;
        QueueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        QueueCreateInfos[0].pNext = 0;
        QueueCreateInfos[0].flags = 0;
        QueueCreateInfos[0].queueFamilyIndex = GlobalState.GraphicsQueueFamId;
        QueueCreateInfos[0].queueCount = ArrayCount(QueuePriorities);
        QueueCreateInfos[0].pQueuePriorities = QueuePriorities;

        ++NumQueues;
        QueueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        QueueCreateInfos[1].pNext = 0;
        QueueCreateInfos[1].flags = 0;
        QueueCreateInfos[1].queueFamilyIndex = GlobalState.TransferQueueFamId;
        QueueCreateInfos[1].queueCount = ArrayCount(QueuePriorities);
        QueueCreateInfos[1].pQueuePriorities = QueuePriorities;

        ++NumQueues;
        QueueCreateInfos[2].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        QueueCreateInfos[2].pNext = 0;
        QueueCreateInfos[2].flags = 0;
        QueueCreateInfos[2].queueFamilyIndex = GlobalState.ComputeQueueFamId;
        QueueCreateInfos[2].queueCount = ArrayCount(QueuePriorities);
        QueueCreateInfos[2].pQueuePriorities = QueuePriorities;

        if (GlobalState.GraphicsQueueFamId != GlobalState.PresentQueueFamId)
        {
            ++NumQueues;
            QueueCreateInfos[3] = {};
            QueueCreateInfos[3].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            QueueCreateInfos[3].pNext = 0;
            QueueCreateInfos[3].flags = 0;
            QueueCreateInfos[3].queueFamilyIndex = GlobalState.PresentQueueFamId;
            QueueCreateInfos[3].queueCount = ArrayCount(QueuePriorities);
            QueueCreateInfos[3].pQueuePriorities = QueuePriorities;
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
        vkGetDeviceQueue(GlobalState.Device, GlobalState.TransferQueueFamId, 0, &GlobalState.TransferQueue);
        vkGetDeviceQueue(GlobalState.Device, GlobalState.GraphicsQueueFamId, 0, &GlobalState.GraphicsQueue);
        vkGetDeviceQueue(GlobalState.Device, GlobalState.PresentQueueFamId, 0, &GlobalState.PresentQueue);
        vkGetDeviceQueue(GlobalState.Device, GlobalState.ComputeQueueFamId, 0, &GlobalState.ComputeQueue);
            
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

    // NOTE: Create graphics data
    {
        // NOTE: Allocate graphics pools
        VkCommandPoolCreateInfo CmdPoolCreateInfo = {};
        CmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        CmdPoolCreateInfo.pNext = 0;
        CmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        CmdPoolCreateInfo.queueFamilyIndex = GlobalState.GraphicsQueueFamId;
        if (vkCreateCommandPool(GlobalState.Device, &CmdPoolCreateInfo, 0, &GlobalState.GraphicsCmdPool) != VK_SUCCESS)
        {
            InvalidCodePath;
        }

        // NOTE: Allocate command buffers, create semaphores, and fences
        VkCommandBufferAllocateInfo CmdBufferAllocateInfo = {};
        CmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        CmdBufferAllocateInfo.pNext = 0;
        CmdBufferAllocateInfo.commandPool = GlobalState.GraphicsCmdPool;
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

        if (vkCreateFence(GlobalState.Device, &FenceCreateInfo, 0, &GlobalState.GraphicsFence) != VK_SUCCESS)
        {
            InvalidCodePath;
        }
    }
     
    // NOTE: Create transfer data
    {
        // NOTE: Create transfer command pool
        VkCommandPoolCreateInfo CmdPoolCreateInfo = {};
        CmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        CmdPoolCreateInfo.pNext = 0;
        CmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        CmdPoolCreateInfo.queueFamilyIndex = GlobalState.TransferQueueFamId;

        if (vkCreateCommandPool(GlobalState.Device, &CmdPoolCreateInfo, 0, &GlobalState.TransferCmdPool) != VK_SUCCESS)
        {
            InvalidCodePath;
        }
        
        // NOTE: Create transfer command buffer
        VkCommandBufferAllocateInfo CmdBufferAllocateInfo = {};
        CmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        CmdBufferAllocateInfo.pNext = 0;
        CmdBufferAllocateInfo.commandPool = GlobalState.TransferCmdPool;
        CmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        CmdBufferAllocateInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(GlobalState.Device, &CmdBufferAllocateInfo, &GlobalState.TransferCmdBuffer) != VK_SUCCESS)
        {
            InvalidCodePath;
        }

        // NOTE: Create transfer buffers
        VkBufferCreateInfo BufferCreateInfo = {};
        BufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        BufferCreateInfo.pNext = 0;
        BufferCreateInfo.flags = 0;
        BufferCreateInfo.size = TRANSFER_BUFFER_SIZE;
        BufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        BufferCreateInfo.queueFamilyIndexCount = 0;
        BufferCreateInfo.pQueueFamilyIndices = 0;

        if (vkCreateBuffer(GlobalState.Device, &BufferCreateInfo, 0, &GlobalState.TransferBuffer) != VK_SUCCESS)
        {
            InvalidCodePath;
        }

        // NOTE: Allocate buffer memory
        VkMemoryRequirements BufferMemRequirements;
        vkGetBufferMemoryRequirements(GlobalState.Device, GlobalState.TransferBuffer, &BufferMemRequirements);

        for (u32 i = 0; i < GlobalState.MemoryProperties.memoryTypeCount; ++i)
        {
            if ((BufferMemRequirements.memoryTypeBits & (1 << i)) &&
                (GlobalState.MemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
            {
                VkMemoryAllocateInfo MemoryAllocateInfo = {};
                MemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                MemoryAllocateInfo.pNext = 0;
                MemoryAllocateInfo.allocationSize = BufferMemRequirements.size;
                MemoryAllocateInfo.memoryTypeIndex = i;

                if (vkAllocateMemory(GlobalState.Device, &MemoryAllocateInfo, 0, &GlobalState.TransferBufferMem) == VK_SUCCESS)
                {
                    break;
                }
            }
        }

        u32 Offset = 0;
        if (vkBindBufferMemory(GlobalState.Device, GlobalState.TransferBuffer, GlobalState.TransferBufferMem, Offset) != VK_SUCCESS)
        {
            InvalidCodePath;
        }

        if (vkMapMemory(GlobalState.Device, GlobalState.TransferBufferMem, 0, VK_WHOLE_SIZE, 0, (void**)&GlobalState.TransferBufferMemPtr) != VK_SUCCESS)
        {
            InvalidCodePath;
        }

        // NOTE: Create fences
        VkFenceCreateInfo FenceCreateInfo = {};
        FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        FenceCreateInfo.pNext = 0;
        FenceCreateInfo.flags = 0;
            
        if (vkCreateFence(GlobalState.Device, &FenceCreateInfo, 0, &GlobalState.TransferFence) != VK_SUCCESS)
        {
            InvalidCodePath;
        }
    }
    
    // NOTE: Create render pass
    {
        // NOTE: Create depth map and convert it
        VkFormat DepthFormat = VK_FORMAT_D32_SFLOAT;
        VkImageAspectFlags DepthAspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        CreateImage(&GlobalState.GpuAllocator, RENDER_WIDTH, RENDER_HEIGHT, DepthFormat,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    DepthAspectMask, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    &GlobalState.DepthImg, &GlobalState.DepthView);
        
        VkAttachmentDescription AttachmentDescriptions[2] = {};
        AttachmentDescriptions[0].flags = 0;
        AttachmentDescriptions[0].format = GlobalState.SwapChainFormat;
        AttachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
        AttachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        AttachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        AttachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        AttachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        AttachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        AttachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        AttachmentDescriptions[1].flags = 0;
        AttachmentDescriptions[1].format = DepthFormat;
        AttachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
        AttachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        AttachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        AttachmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        AttachmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        AttachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        AttachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference ColorReference = {};
        ColorReference.attachment = 0;
        ColorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference DepthReference = {};
        DepthReference.attachment = 1;
        DepthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription SubpassDescriptions[4] = {};
        {
            SubpassDescriptions[0].flags = 0;
            SubpassDescriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            SubpassDescriptions[0].inputAttachmentCount = 0;
            SubpassDescriptions[0].pInputAttachments = 0;
            SubpassDescriptions[0].colorAttachmentCount = 0;
            SubpassDescriptions[0].pColorAttachments = 0;
            SubpassDescriptions[0].pResolveAttachments = 0;
            SubpassDescriptions[0].pDepthStencilAttachment = &DepthReference;
            SubpassDescriptions[0].preserveAttachmentCount = 0;
            SubpassDescriptions[0].pPreserveAttachments = 0;
        }
        
        {
            SubpassDescriptions[1].flags = 0;
            SubpassDescriptions[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
            SubpassDescriptions[1].inputAttachmentCount = 0;
            SubpassDescriptions[1].pInputAttachments = 0;
            SubpassDescriptions[1].colorAttachmentCount = 0;
            SubpassDescriptions[1].pColorAttachments = 0;
            SubpassDescriptions[1].pResolveAttachments = 0;
            SubpassDescriptions[1].pDepthStencilAttachment = 0;
            SubpassDescriptions[1].preserveAttachmentCount = 0;
            SubpassDescriptions[1].pPreserveAttachments = 0;
        }
        
        {
            SubpassDescriptions[2].flags = 0;
            SubpassDescriptions[2].pipelineBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
            SubpassDescriptions[2].inputAttachmentCount = 0;
            SubpassDescriptions[2].pInputAttachments = 0;
            SubpassDescriptions[2].colorAttachmentCount = 0;
            SubpassDescriptions[2].pColorAttachments = 0;
            SubpassDescriptions[2].pResolveAttachments = 0;
            SubpassDescriptions[2].pDepthStencilAttachment = 0;
            SubpassDescriptions[2].preserveAttachmentCount = 0;
            SubpassDescriptions[2].pPreserveAttachments = 0;
        }
        
        {
            SubpassDescriptions[3].flags = 0;
            SubpassDescriptions[3].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            SubpassDescriptions[3].inputAttachmentCount = 0;
            SubpassDescriptions[3].pInputAttachments = 0;
            SubpassDescriptions[3].colorAttachmentCount = 1;
            SubpassDescriptions[3].pColorAttachments = &ColorReference;
            SubpassDescriptions[3].pResolveAttachments = 0;
            SubpassDescriptions[3].pDepthStencilAttachment = &DepthReference;
            SubpassDescriptions[3].preserveAttachmentCount = 0;
            SubpassDescriptions[3].pPreserveAttachments = 0;
        }
        
        VkSubpassDependency Dependencies[5] = {};
        Dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        Dependencies[0].dstSubpass = 0;
        Dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        Dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        Dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        Dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        Dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        Dependencies[1].srcSubpass = 0;
        Dependencies[1].dstSubpass = 1;
        Dependencies[1].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        Dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        Dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        Dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        Dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        
        Dependencies[2].srcSubpass = 1;
        Dependencies[2].dstSubpass = 2;
        Dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        Dependencies[2].dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        Dependencies[2].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        Dependencies[2].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        Dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        
        Dependencies[3].srcSubpass = 2;
        Dependencies[3].dstSubpass = 3;
        Dependencies[3].srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        Dependencies[3].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        Dependencies[3].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        Dependencies[3].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        Dependencies[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        Dependencies[4].srcSubpass = 3;
        Dependencies[4].dstSubpass = VK_SUBPASS_EXTERNAL;
        Dependencies[4].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        Dependencies[4].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        Dependencies[4].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        Dependencies[4].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        Dependencies[4].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
            
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
    
    // TODO: For depth only passes, it is useful to have deinterleaved vertex attributes since
    // we only care about position for depth pass, less cache misses
    // NOTE: Create depth pre pass pipeline
    {
        // NOTE: Specfiy pipeline shaders
        VkShaderModule VertexShaderModule = CreateShaderModule("depth_pre_pass_vert.spv");
        VkPipelineShaderStageCreateInfo ShaderStageCreateInfos[1] = {};
        ShaderStageCreateInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderStageCreateInfos[0].pNext = 0;
        ShaderStageCreateInfos[0].flags = 0;
        ShaderStageCreateInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        ShaderStageCreateInfos[0].module = VertexShaderModule;
        ShaderStageCreateInfos[0].pName = "main";
        ShaderStageCreateInfos[0].pSpecializationInfo = 0;

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
        RasterizationStateCreateInfo.cullMode = VK_CULL_MODE_FRONT_BIT; // TODO:  Fix this
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
        VkPushConstantRange ConstRanges[1] = {};
        ConstRanges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        ConstRanges[0].offset = 0;
        ConstRanges[0].size = sizeof(depth_prepass_consts);

        VkPipelineLayoutCreateInfo LayoutCreateInfo = {};
        LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        LayoutCreateInfo.pNext = 0;
        LayoutCreateInfo.flags = 0;
        LayoutCreateInfo.setLayoutCount = 0;
        LayoutCreateInfo.pSetLayouts = 0;
        LayoutCreateInfo.pushConstantRangeCount = ArrayCount(ConstRanges);
        LayoutCreateInfo.pPushConstantRanges = ConstRanges;

        if (vkCreatePipelineLayout(GlobalState.Device, &LayoutCreateInfo, 0, &GlobalState.DepthPrePassPipeline.Layout) != VK_SUCCESS)
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
        PipelineCreateInfo.pColorBlendState = 0;
        PipelineCreateInfo.pDynamicState = &DynamicStateCreateInfo;
        PipelineCreateInfo.layout = GlobalState.DepthPrePassPipeline.Layout;
        PipelineCreateInfo.renderPass = GlobalState.RenderPass;
        PipelineCreateInfo.subpass = 0;
        PipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        PipelineCreateInfo.basePipelineIndex = -1;

        if (vkCreateGraphicsPipelines(GlobalState.Device, VK_NULL_HANDLE, 1, &PipelineCreateInfo,
                                      0, &GlobalState.DepthPrePassPipeline.Pipeline) != VK_SUCCESS)
        {
            InvalidCodePath;
        }
    }

    // NOTE: Create Tile Data descriptor/pipeline
    {
        // NOTE: Setup Tile data
        {
            VkDescriptorSetLayoutBinding LayoutBindings[2] = {};
            LayoutBindings[0].binding = 0;
            LayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            LayoutBindings[0].descriptorCount = 1;
            LayoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            LayoutBindings[0].pImmutableSamplers = 0;

            LayoutBindings[1].binding = 1;
            LayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            LayoutBindings[1].descriptorCount = 1;
            LayoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            LayoutBindings[1].pImmutableSamplers = 0;

            GlobalState.BuildTileDataSetLayout = CreateDescriptorSetLayout(LayoutBindings, ArrayCount(LayoutBindings));
        }

        // NOTE: Specify pipeline input
        VkDescriptorSetLayout Layouts[1] = {};
        Layouts[0] = GlobalState.BuildTileDataSetLayout;

        VkPushConstantRange ConstRanges[1] = {};
        ConstRanges[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        ConstRanges[0].offset = 0;
        ConstRanges[0].size = sizeof(build_tile_consts);

        GlobalState.BuildTileDataPipeline = CreateComputePipeline("build_tile_data_comp.spv", Layouts,
                                                                  ArrayCount(Layouts), ConstRanges,
                                                                  ArrayCount(ConstRanges));
    }
    
    // NOTE: Create Light Culling pipeline
    {
        // NOTE: Light Cull Descriptor
        {
            VkDescriptorSetLayoutBinding LayoutBindings[3] = {};
            LayoutBindings[0].binding = 0;
            LayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            LayoutBindings[0].descriptorCount = 1;
            LayoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            LayoutBindings[0].pImmutableSamplers = 0;

            LayoutBindings[1].binding = 1;
            LayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            LayoutBindings[1].descriptorCount = 1;
            LayoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            LayoutBindings[1].pImmutableSamplers = 0;

            LayoutBindings[2].binding = 2;
            LayoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            LayoutBindings[2].descriptorCount = 1;
            LayoutBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            LayoutBindings[2].pImmutableSamplers = 0;

            GlobalState.LightDataSetLayout = CreateDescriptorSetLayout(LayoutBindings, ArrayCount(LayoutBindings));
        }
        
        {        
            // NOTE: Create descriptor pool
            // TODO: Why do I have to specify descriptor count here as if i include all sets sizes?
            VkDescriptorPoolSize PoolSizes[3] = {};
            PoolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            PoolSizes[0].descriptorCount = 4;
            PoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            PoolSizes[1].descriptorCount = 1;
            PoolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            PoolSizes[2].descriptorCount = 1;
            GlobalState.LightDescriptorPool = CreateDescriptorPool(PoolSizes, ArrayCount(PoolSizes), 3);
        }

        VkDescriptorSetLayout Layouts[2] = {};
        Layouts[0] = GlobalState.BuildTileDataSetLayout;
        Layouts[1] = GlobalState.LightDataSetLayout;

        VkPushConstantRange ConstRanges[1] = {};
        ConstRanges[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        ConstRanges[0].offset = 0;
        ConstRanges[0].size = sizeof(light_cull_consts);
        
        GlobalState.LightCullPipeline = CreateComputePipeline("light_culling_comp.spv", Layouts,
                                                              ArrayCount(Layouts), ConstRanges,
                                                              ArrayCount(ConstRanges));
    }

    // NOTE: Create forward pipeline
    {
        // NOTE: Setup Set layout
        {
            VkDescriptorSetLayoutBinding LayoutBindings[1] = {};
            LayoutBindings[0].binding = 0;
            LayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            LayoutBindings[0].descriptorCount = 1;
            LayoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            LayoutBindings[0].pImmutableSamplers = 0;

            GlobalState.MeshSetLayout = CreateDescriptorSetLayout(LayoutBindings, ArrayCount(LayoutBindings));
            
            // NOTE: Create descriptor pool
            // TODO: Why do i have to specify descriptor count here as if i include all sets sizes?
            VkDescriptorPoolSize PoolSizes[1] = {};
            PoolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            PoolSizes[0].descriptorCount = 100;
            GlobalState.MeshDescriptorPool = CreateDescriptorPool(PoolSizes, ArrayCount(PoolSizes), 100);
        }

        // NOTE: Specfiy pipeline shaders
        VkShaderModule VertexShaderModule = CreateShaderModule("tiled_forward_vert.spv");
        VkShaderModule FragmentShaderModule = CreateShaderModule("tiled_forward_frag.spv");

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
                        
        VertexAttributeDescriptions[1].location = 1;
        VertexAttributeDescriptions[1].binding = VertexBindingDescriptions[0].binding;
        VertexAttributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        VertexAttributeDescriptions[1].offset = sizeof(v3);
                        
        VertexAttributeDescriptions[2].location = 2;
        VertexAttributeDescriptions[2].binding = VertexBindingDescriptions[0].binding;
        VertexAttributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        VertexAttributeDescriptions[2].offset = sizeof(v3) + sizeof(v2);
                        
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
        RasterizationStateCreateInfo.cullMode = VK_CULL_MODE_FRONT_BIT; // TODO:  Fix this
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
        DepthStencilCreateInfo.depthTestEnable = VK_TRUE;
        DepthStencilCreateInfo.depthWriteEnable = VK_TRUE;
        DepthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        DepthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
        DepthStencilCreateInfo.stencilTestEnable = VK_FALSE;
        DepthStencilCreateInfo.front = {};
        DepthStencilCreateInfo.back = {};
            
        // NOTE: Describe how the input data is arranged in memory
        VkDescriptorSetLayout SetLayouts[] =
        {
            GlobalState.MeshSetLayout,
            GlobalState.LightDataSetLayout,
        };

        VkPushConstantRange ConstRanges[2] = {};
        ConstRanges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        ConstRanges[0].offset = 0;
        ConstRanges[0].size = sizeof(m4)*2;

        ConstRanges[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        ConstRanges[1].offset = sizeof(m4)*2;
        ConstRanges[1].size = sizeof(tiled_forward_consts) - sizeof(m4)*2;
        
        VkPipelineLayoutCreateInfo LayoutCreateInfo = {};
        LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        LayoutCreateInfo.pNext = 0;
        LayoutCreateInfo.flags = 0;
        LayoutCreateInfo.setLayoutCount = ArrayCount(SetLayouts);
        LayoutCreateInfo.pSetLayouts = SetLayouts;
        LayoutCreateInfo.pushConstantRangeCount = ArrayCount(ConstRanges);
        LayoutCreateInfo.pPushConstantRanges = ConstRanges;

        if (vkCreatePipelineLayout(GlobalState.Device, &LayoutCreateInfo, 0, &GlobalState.ForwardPipeline.Layout) != VK_SUCCESS)
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
        PipelineCreateInfo.layout = GlobalState.ForwardPipeline.Layout;
        PipelineCreateInfo.renderPass = GlobalState.RenderPass;
        PipelineCreateInfo.subpass = 3;
        PipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        PipelineCreateInfo.basePipelineIndex = -1;

        if (vkCreateGraphicsPipelines(GlobalState.Device, VK_NULL_HANDLE, 1, &PipelineCreateInfo,
                                      0, &GlobalState.ForwardPipeline.Pipeline) != VK_SUCCESS)
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

    // NOTE: Setup point light data
    {
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

        temp_mem TempMem = BeginTempMem(&GlobalState.Arena);
        point_light* LightArray = PushArray(&GlobalState.Arena, point_light, 100);
        point_light* CurrLight = LightArray;
            
        for (i32 LightZ = -1; LightZ <= 1; ++LightZ)
        {
            for (i32 LightY = -1; LightY <= 1; ++LightY)
            {
                for (i32 LightX = -1; LightX <= 1; ++LightX)
                {
                    // NOTE: Point light meta data
                    point_light LightData = {};
                    LightData.Pos = 0.5f*V3((f32)LightX, (f32)LightY, 5.0f + (f32)LightZ);
                    LightData.Radius = 1.5f;
                    LightData.LightColor = RandomColors[(LightX+LightY+LightZ) % ArrayCount(RandomColors)];
                    LightData.AmbientIntensity = 0.0f;
                    LightData.DiffuseIntensity = 0.8f;
                    *CurrLight = LightData;

                    GlobalState.NumPointLights += 1;
                    CurrLight += 1;
                }
            }
        }
        
        u32 PointLightArraySize = sizeof(point_light)*100 + sizeof(u32);
        CreateBuffer(&GlobalState.GpuAllocator, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, PointLightArraySize,
                     &GlobalState.PointLightArray);
        MoveBufferToGpuMemory(GlobalState.PointLightArray, PointLightArraySize, LightArray);
        EndTempMem(TempMem);
    }
    
    // NOTE: Build Global Descriptor Sets
    u32 NumTileX = (u32)Ceil(RENDER_WIDTH / SCREEN_TILE_X);
    u32 NumTileY = (u32)Ceil(RENDER_HEIGHT / SCREEN_TILE_X);
    {
        // NOTE: Setup Build Tile Data Descriptor Set
        {
            CreateBuffer(&GlobalState.GpuAllocator, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, sizeof(light_tile)*NumTileX*NumTileY,
                         &GlobalState.LightTileArray);

            VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo = {};
            DescriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            DescriptorSetAllocateInfo.pNext = 0;
            DescriptorSetAllocateInfo.descriptorPool = GlobalState.LightDescriptorPool;
            DescriptorSetAllocateInfo.descriptorSetCount = 1;
            DescriptorSetAllocateInfo.pSetLayouts = &GlobalState.BuildTileDataSetLayout;

            if (vkAllocateDescriptorSets(GlobalState.Device, &DescriptorSetAllocateInfo,
                                         &GlobalState.BuildTileDataSet) != VK_SUCCESS)
            {
                InvalidCodePath;
            }
        }

        // NOTE: Setup Light Data Descriptor Set
        {
            CreateBuffer(&GlobalState.GpuAllocator, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                         sizeof(u32)*MAX_NUM_LIGHTS_PER_TILE*NumTileX*NumTileY,
                         &GlobalState.PointLightIndexArray);

            CreateImage(&GlobalState.GpuAllocator, NumTileX, NumTileY, VK_FORMAT_R32_SINT,
                        VK_IMAGE_USAGE_STORAGE_BIT, 0, VK_IMAGE_LAYOUT_GENERAL,
                        &GlobalState.DebugHeatMapImg, &GlobalState.DebugHeatMapView);

            VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo = {};
            DescriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            DescriptorSetAllocateInfo.pNext = 0;
            DescriptorSetAllocateInfo.descriptorPool = GlobalState.LightDescriptorPool;
            DescriptorSetAllocateInfo.descriptorSetCount = 1;
            DescriptorSetAllocateInfo.pSetLayouts = &GlobalState.LightDataSetLayout;

            if (vkAllocateDescriptorSets(GlobalState.Device, &DescriptorSetAllocateInfo,
                                         &GlobalState.LightDataSet) != VK_SUCCESS)
            {
                InvalidCodePath;
            }
        }

        VkDescriptorBufferInfo LightTileArrayInfo = {};
        LightTileArrayInfo.buffer = GlobalState.LightTileArray;
        LightTileArrayInfo.offset = 0;
        LightTileArrayInfo.range = VK_WHOLE_SIZE;
            
        VkDescriptorImageInfo DepthSamplerInfo = {};
        DepthSamplerInfo.sampler = GlobalState.TextureSampler;
        DepthSamplerInfo.imageView = GlobalState.DepthView;
        DepthSamplerInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL; //VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        
        VkDescriptorBufferInfo PointLightArrayInfo = {};
        PointLightArrayInfo.buffer = GlobalState.PointLightArray;
        PointLightArrayInfo.offset = 0;
        PointLightArrayInfo.range = VK_WHOLE_SIZE;
            
        VkDescriptorBufferInfo PointLightIndexArrayInfo = {};
        PointLightIndexArrayInfo.buffer = GlobalState.PointLightIndexArray;
        PointLightIndexArrayInfo.offset = 0;
        PointLightIndexArrayInfo.range = VK_WHOLE_SIZE;

        VkDescriptorImageInfo DebugHeatMapInfo = {};
        DebugHeatMapInfo.sampler = 0;
        DebugHeatMapInfo.imageView = GlobalState.DebugHeatMapView;
        DebugHeatMapInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        
        VkWriteDescriptorSet DescriptorWrites[5] = {};
        DescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        DescriptorWrites[0].pNext = 0;
        DescriptorWrites[0].dstSet = GlobalState.BuildTileDataSet;
        DescriptorWrites[0].dstBinding = 0;
        DescriptorWrites[0].dstArrayElement = 0;
        DescriptorWrites[0].descriptorCount = 1;
        DescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        DescriptorWrites[0].pImageInfo = 0;
        DescriptorWrites[0].pBufferInfo = &LightTileArrayInfo;
        DescriptorWrites[0].pTexelBufferView = 0;

        DescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        DescriptorWrites[1].pNext = 0;
        DescriptorWrites[1].dstSet = GlobalState.BuildTileDataSet;
        DescriptorWrites[1].dstBinding = 1;
        DescriptorWrites[1].dstArrayElement = 0;
        DescriptorWrites[1].descriptorCount = 1;
        DescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        DescriptorWrites[1].pImageInfo = &DepthSamplerInfo;
        DescriptorWrites[1].pBufferInfo = 0;
        DescriptorWrites[1].pTexelBufferView = 0;

        DescriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        DescriptorWrites[2].pNext = 0;
        DescriptorWrites[2].dstSet = GlobalState.LightDataSet;
        DescriptorWrites[2].dstBinding = 0;
        DescriptorWrites[2].dstArrayElement = 0;
        DescriptorWrites[2].descriptorCount = 1;
        DescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        DescriptorWrites[2].pImageInfo = 0;
        DescriptorWrites[2].pBufferInfo = &PointLightArrayInfo;
        DescriptorWrites[2].pTexelBufferView = 0;

        DescriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        DescriptorWrites[3].pNext = 0;
        DescriptorWrites[3].dstSet = GlobalState.LightDataSet;
        DescriptorWrites[3].dstBinding = 1;
        DescriptorWrites[3].dstArrayElement = 0;
        DescriptorWrites[3].descriptorCount = 1;
        DescriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        DescriptorWrites[3].pImageInfo = 0;
        DescriptorWrites[3].pBufferInfo = &PointLightIndexArrayInfo;
        DescriptorWrites[3].pTexelBufferView = 0;

        DescriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        DescriptorWrites[4].pNext = 0;
        DescriptorWrites[4].dstSet = GlobalState.LightDataSet;
        DescriptorWrites[4].dstBinding = 2;
        DescriptorWrites[4].dstArrayElement = 0;
        DescriptorWrites[4].descriptorCount = 1;
        DescriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        DescriptorWrites[4].pImageInfo = &DebugHeatMapInfo;
        DescriptorWrites[4].pBufferInfo = 0;
        DescriptorWrites[4].pTexelBufferView = 0;
        
        vkUpdateDescriptorSets(GlobalState.Device, ArrayCount(DescriptorWrites), DescriptorWrites, 0, 0);        
    }

    // NOTE: Create a world to render
    {
        // NOTE: Create a white texture for meshes
        u32 Width = 1;
        u32 Height = 1;
        u32 TextureData = 0xFFFFFFFF;

        VkImage WhiteTexture;
        VkImageView WhiteTextureView;
        CreateImage(&GlobalState.GpuAllocator, 1, 1, VK_FORMAT_R8G8B8A8_UNORM,
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, &WhiteTexture,
                    &WhiteTextureView);
        MoveImageToGpuMemory(WhiteTexture, sizeof(u32), 1, 1, &TextureData);

        // NOTE: Allocate descriptor sets
        VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo = {};
        DescriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        DescriptorSetAllocateInfo.pNext = 0;
        DescriptorSetAllocateInfo.descriptorPool = GlobalState.MeshDescriptorPool;
        DescriptorSetAllocateInfo.descriptorSetCount = 1;
        DescriptorSetAllocateInfo.pSetLayouts = &GlobalState.MeshSetLayout;

        // NOTE: Setup render mesh data
        for (i32 ModelZ = -1; ModelZ <= 1; ++ModelZ)
        {
            for (i32 ModelY = -1; ModelY <= 1; ++ModelY)
            {
                for (i32 ModelX = -1; ModelX <= 1; ++ModelX)
                {
                    mesh* Mesh = GlobalState.RenderMeshes + GlobalState.NumModels;
                    
                    // NOTE: Get model asset
                    asset_model* CurrModel = GetModel(&GlobalState.Assets, Model_Bunny);
                    Mesh->Asset = CurrModel->Meshes + 0;

                    // NOTE: Setup render constants
                    v3 ModelPos = V3(1.8f*(f32)ModelX, 1.8f*(f32)ModelY, 4.0f + (f32)ModelZ);
                    Mesh->WTransform = PosMat(ModelPos.x, ModelPos.y, ModelPos.z)*ScaleMat(0.55, 0.55, 0.55);
                    Mesh->WVPTransform = GlobalState.VPTransform*Mesh->WTransform;

                    // NOTE: Setup model descriptor set
                    if (vkAllocateDescriptorSets(GlobalState.Device, &DescriptorSetAllocateInfo,
                                                 &Mesh->DescriptorSet) != VK_SUCCESS)
                    {
                        InvalidCodePath;
                    }

                    VkDescriptorImageInfo ImageInfo = {};
                    ImageInfo.sampler = GlobalState.TextureSampler;
                    ImageInfo.imageView = WhiteTextureView;
                    ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            
                    VkWriteDescriptorSet DescriptorWrites[1] = {};
                    DescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    DescriptorWrites[0].pNext = 0;
                    DescriptorWrites[0].dstSet = Mesh->DescriptorSet;
                    DescriptorWrites[0].dstBinding = 0;
                    DescriptorWrites[0].dstArrayElement = 0;
                    DescriptorWrites[0].descriptorCount = 1;
                    DescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    DescriptorWrites[0].pImageInfo = &ImageInfo;
                    DescriptorWrites[0].pBufferInfo = 0;
                    DescriptorWrites[0].pTexelBufferView = 0;

                    vkUpdateDescriptorSets(GlobalState.Device, ArrayCount(DescriptorWrites), DescriptorWrites, 0, 0);
                    
                    GlobalState.NumModels += 1;
                }
            }
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
            if (vkWaitForFences(GlobalState.Device, 1, &GlobalState.GraphicsFence, VK_TRUE, 1000000000) != VK_SUCCESS)
            {
                InvalidCodePath;
            }
            vkResetFences(GlobalState.Device, 1, &GlobalState.GraphicsFence);

            u32 ImageIndex;
            VkResult Result = vkAcquireNextImageKHR(GlobalState.Device, GlobalState.SwapChain,
                                                    UINT64_MAX, GlobalState.ImageAvailableSemaphore,
                                                    VK_NULL_HANDLE, &ImageIndex);

            // NOTE: Create frame buffer for render pass
            {
                GlobalState.PresentView = GlobalState.SwapChainViews[ImageIndex];
                if (GlobalState.FrameBuffer != VK_NULL_HANDLE)
                {
                    vkDestroyFramebuffer(GlobalState.Device, GlobalState.FrameBuffer, 0);
                }
            
                VkFramebufferCreateInfo FrameBufferCreateInfo = {};
                FrameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                FrameBufferCreateInfo.pNext = 0;
                FrameBufferCreateInfo.flags = 0;
                FrameBufferCreateInfo.renderPass = GlobalState.RenderPass;
                FrameBufferCreateInfo.attachmentCount = ArrayCount(GlobalState.Views);
                FrameBufferCreateInfo.pAttachments = GlobalState.Views;
                FrameBufferCreateInfo.width = RENDER_WIDTH;
                FrameBufferCreateInfo.height = RENDER_HEIGHT;
                FrameBufferCreateInfo.layers = 1;

                if (vkCreateFramebuffer(GlobalState.Device, &FrameBufferCreateInfo, 0, &GlobalState.FrameBuffer) != VK_SUCCESS)
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
            
            VkClearValue ClearColors[2] = {};
            ClearColors[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };
            ClearColors[1].depthStencil = { 1.0f, 0 };
            
            VkRenderPassBeginInfo RenderPassBeginInfo = {};
            RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            RenderPassBeginInfo.pNext = 0;
            RenderPassBeginInfo.renderPass = GlobalState.RenderPass;
            RenderPassBeginInfo.framebuffer = GlobalState.FrameBuffer;
            RenderPassBeginInfo.renderArea.offset = { 0, 0 };
            RenderPassBeginInfo.renderArea.extent = GlobalState.SwapChainExtent;
            RenderPassBeginInfo.clearValueCount = ArrayCount(ClearColors);
            RenderPassBeginInfo.pClearValues = ClearColors;

            VkViewport ViewPort = {};
            ViewPort.x = 0.0f;
            ViewPort.y = 0.0f;
            ViewPort.width = (f32)GlobalState.SwapChainExtent.width;
            ViewPort.height = (f32)GlobalState.SwapChainExtent.height;
            ViewPort.minDepth = 0.0f;
            ViewPort.maxDepth = 1.0f;

            VkRect2D Scissor = {};
            Scissor.offset = { 0, 0 };
            Scissor.extent = { GlobalState.SwapChainExtent.width, GlobalState.SwapChainExtent.height, };

            vkCmdSetViewport(GlobalState.PrimaryCmdBuffer, 0, 1, &ViewPort);
            vkCmdSetScissor(GlobalState.PrimaryCmdBuffer, 0, 1, &Scissor);
            
            // NOTE: Depth Pre Pass
            vkCmdBeginRenderPass(GlobalState.PrimaryCmdBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
            {
                vkCmdBindPipeline(GlobalState.PrimaryCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GlobalState.DepthPrePassPipeline.Pipeline);

                for (u32 ModelId = 0; ModelId < GlobalState.NumModels; ++ModelId)
                {
                    mesh* CurrMesh = GlobalState.RenderMeshes + ModelId;
                
                    VkDeviceSize Offset = 0;
                    vkCmdBindVertexBuffers(GlobalState.PrimaryCmdBuffer, 0, 1, &CurrMesh->Asset->Handle, &Offset);

                    depth_prepass_consts Consts = {};
                    Consts.WVPTransform = CurrMesh->WVPTransform;
                    
                    vkCmdPushConstants(GlobalState.PrimaryCmdBuffer, GlobalState.BuildTileDataPipeline.Layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(depth_prepass_consts), &Consts);
                    vkCmdDraw(GlobalState.PrimaryCmdBuffer, CurrMesh->Asset->NumVertices, 1, 0, 0);
                }
            }

            // NOTE: Setup tile light data
            vkCmdNextSubpass(GlobalState.PrimaryCmdBuffer, VK_SUBPASS_CONTENTS_INLINE);
            {
                vkCmdBindPipeline(GlobalState.PrimaryCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, GlobalState.BuildTileDataPipeline.Pipeline);

                VkDescriptorSet Sets[] =
                {
                    GlobalState.BuildTileDataSet,
                };
                vkCmdBindDescriptorSets(GlobalState.PrimaryCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, GlobalState.BuildTileDataPipeline.Layout, 0, ArrayCount(Sets), Sets, 0, 0);
      
                build_tile_consts Consts = {};
                Consts.InverseVP = Inverse(GlobalState.VPTransform);
                Consts.ScreenDim = V2(RENDER_WIDTH, RENDER_HEIGHT);
                Consts.NearZ = GlobalState.NearZ;
                Consts.FarZ = GlobalState.FarZ;
                
                vkCmdPushConstants(GlobalState.PrimaryCmdBuffer, GlobalState.BuildTileDataPipeline.Layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(build_tile_consts), &Consts);
                
                vkCmdDispatch(GlobalState.PrimaryCmdBuffer, NumTileX/8, NumTileY/8, 1);
            }

            // NOTE: Perform light culling
            vkCmdNextSubpass(GlobalState.PrimaryCmdBuffer, VK_SUBPASS_CONTENTS_INLINE);
            {
                vkCmdBindPipeline(GlobalState.PrimaryCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, GlobalState.LightCullPipeline.Pipeline);

                VkDescriptorSet Sets[] =
                {
                    GlobalState.BuildTileDataSet,
                    GlobalState.LightDataSet,
                };
                
                vkCmdBindDescriptorSets(GlobalState.PrimaryCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, GlobalState.LightCullPipeline.Layout, 0, ArrayCount(Sets), Sets, 0, 0);

                light_cull_consts Consts = {};
                Consts.VPTransform = GlobalState.VPTransform;
                Consts.NumLights = GlobalState.NumPointLights;
                Consts.MaxLightsPerTile = MAX_NUM_LIGHTS_PER_TILE;
                vkCmdPushConstants(GlobalState.PrimaryCmdBuffer, GlobalState.LightCullPipeline.Layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(light_cull_consts), &Consts);
                
                vkCmdDispatch(GlobalState.PrimaryCmdBuffer, NumTileX, NumTileY, 1);
            }

            // NOTE: Forward Pass
            vkCmdNextSubpass(GlobalState.PrimaryCmdBuffer, VK_SUBPASS_CONTENTS_INLINE);
            {
                vkCmdBindPipeline(GlobalState.PrimaryCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GlobalState.ForwardPipeline.Pipeline);

                for (u32 ModelId = 0; ModelId < GlobalState.NumModels; ++ModelId)
                {
                    mesh* CurrMesh = GlobalState.RenderMeshes + ModelId;
                
                    VkDeviceSize Offset = 0;
                    vkCmdBindVertexBuffers(GlobalState.PrimaryCmdBuffer, 0, 1, &CurrMesh->Asset->Handle, &Offset);
                
                    VkDescriptorSet Sets[] =
                        {
                            CurrMesh->DescriptorSet,
                            GlobalState.LightDataSet,
                        };
                    vkCmdBindDescriptorSets(GlobalState.PrimaryCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GlobalState.ForwardPipeline.Layout, 0, ArrayCount(Sets), Sets, 0, 0);

                    tiled_forward_consts Consts = {};
                    Consts.WTransform = CurrMesh->WTransform;
                    Consts.WVPTransform = CurrMesh->WVPTransform;
                    Consts.SpecularIntensity = 1;
                    Consts.SpecularPower = 16;
                    Consts.ScreenDim = V2(RENDER_WIDTH, RENDER_HEIGHT);
                    Consts.NumTilesX = NumTileX;
                    Consts.MaxLightsPerTile = MAX_NUM_LIGHTS_PER_TILE;
                    
                    vkCmdPushConstants(GlobalState.PrimaryCmdBuffer, GlobalState.ForwardPipeline.Layout,
                                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(tiled_forward_consts), &Consts);
                    vkCmdDraw(GlobalState.PrimaryCmdBuffer, CurrMesh->Asset->NumVertices, 1, 0, 0);
                }
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

            if (vkQueueSubmit(GlobalState.GraphicsQueue, 1, &SubmitInfo, GlobalState.GraphicsFence) != VK_SUCCESS)
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
