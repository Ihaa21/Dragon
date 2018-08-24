#if !defined(DRAGON_GPU_MEMORY_H)

struct gpu_ptr
{
    VkDeviceMemory* Memory;
    u64 Offset;
};

struct gpu_linear_allocator
{
    u64 Size;
    u64 Used;
    gpu_ptr MemPtr;
};

#define DRAGON_GPU_MEMORY_H
#endif
