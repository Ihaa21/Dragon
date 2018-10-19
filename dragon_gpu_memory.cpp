/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Ihor Szlachtycz $
   $Notice: (C) Copyright 2014 by Dream.Inc, Inc. All Rights Reserved. $
   ======================================================================== */

//
// NOTE: Gpu Linear Allocator
//

inline u64 GetAlignmentOffset(u64 Address, u64 Alignment)
{
    // IMPORTANT: We assume a power of 2 alignment
    u64 Result = 0;

    u64 AlignmentMask = Alignment - 1;
    if (Address & AlignmentMask)
    {
        Result = Alignment - (Address & AlignmentMask);
    }

    return Result;
}

inline gpu_linear_allocator InitGpuLinearAllocator(gpu_ptr MemPtr, u64 Size)
{
    gpu_linear_allocator Result = {};
    Result.Size = Size;
    Result.MemPtr = MemPtr;

    return Result;
}

inline gpu_ptr PushSize(gpu_linear_allocator* Allocator, u64 Size, u64 Alignment)
{
    Assert(Allocator->Used + Size <= Allocator->Size);

    u64 AlignmentOffset = GetAlignmentOffset(Allocator->Used, Alignment);
    Allocator->Used += AlignmentOffset;
    
    gpu_ptr Result = {};
    Result.Memory = Allocator->MemPtr.Memory;
    Result.Offset = Allocator->Used;

    Allocator->Used += Size;

    return Result;
}
