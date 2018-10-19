//
// NOTE: Stack allocator
//

inline stack_allocator InitStackAllocator(void* Mem, u64 Size)
{
    stack_allocator Result = {};
    Result.Size = Size;
    Result.Base = (u8*)Mem;

    return Result;
}

inline void* PushSize(stack_allocator* Allocator, u64 Size)
{
    // NOTE: Setup the header
    stack_alloc_header* Header = (stack_alloc_header*)(Allocator->Base + Allocator->Used);
    Allocator->Used += sizeof(stack_alloc_header);

    Header->Prev = Allocator->TopHeader;
    Allocator->TopHeader = Header;

    // NOTE: Setup the memory
    Assert(Allocator->Used + Size <= Allocator->Size);
    void* Result = (void*)(Allocator->Base + Allocator->Used);
    Allocator->Used += Size;

    return Result;
}

inline void FreeAlloc(stack_allocator* Allocator, void* Mem)
{
    Assert(Allocator->Used != 0);
    
    stack_alloc_header* Header = (stack_alloc_header*)((u8*)Mem - sizeof(stack_alloc_header));

    Assert(Allocator->TopHeader == Header);
    u64 AllocSize = ((u64)Allocator->Base + Allocator->Used) - (u64)Mem;
    Allocator->Used -= sizeof(stack_alloc_header) + AllocSize;
}

//
// NOTE: Free list allocator
//

#define ListInsert(Prev, Val)                   \
    {                                           \
        Val->Prev = Prev;                       \
        Val->Next = Prev->MemOrderNext;         \
        Val->MemOrderPrev->MemOrderNext = Val;  \
        Val->MemOrderNext->MemOrderPrev = Val;  \
    }

#define ListRemove(Val)                         \
    {                                           \
        Val->Prev->Next = Val->Next;            \
        Val->Next->Prev = Val->Prev;            \
    }

inline list_allocator InitListAllocator(void* Mem, u64 Size)
{
    list_allocator Result = {};
    Result.MemList.Next = &Result.MemList;
    Result.MemList.Prev = &Result.MemList;
    Result.FreeList.Next = &Result.FreeList;
    Result.FreeList.Prev = &Result.FreeList;
    Result.MemOrderList.MemOrderNext = &Result.MemOrderList;
    Result.MemOrderList.MemOrderPrev = &Result.MemOrderList;
    
    list_alloc_header* EntireMem = (list_alloc_header*)Mem;
    EntireMem->Size = Size - sizeof(list_alloc_header);
    EntireMem->IsFreed = true;

    // NOTE: Add EntireMem to the free list
    {
        EntireMem->Prev = &Result.FreeList;
        EntireMem->Next = Result.FreeList.Next;

        EntireMem->Prev->Next = EntireMem;
        EntireMem->Next->Prev = EntireMem;
    }

    // NOTE: Add EntireMem to the mem order list
    {
        EntireMem->MemOrderPrev = &Result.MemOrderList;
        EntireMem->MemOrderNext = Result.MemOrderList.MemOrderNext;

        EntireMem->MemOrderPrev->MemOrderNext = EntireMem;
        EntireMem->MemOrderNext->MemOrderPrev = EntireMem;
    }    
    return Result;
}

inline void* PushSize(list_allocator* Allocator, u64 AllocSize)
{
    // NOTE: Loop through MemList to find the first slot large enough for size
    list_alloc_header* Found = 0;
    list_alloc_header* CurrHeader = Allocator->FreeList.Next;
    while (CurrHeader != &Allocator->FreeList)
    {
        if (CurrHeader->Size >= AllocSize)
        {
            Found = CurrHeader;
            break;
        }

        CurrHeader = CurrHeader->Next;
    }

    if (!Found)
    {
        // TODO: Alloc size too big, alloc more or defrag?
        InvalidCodePath;
    }

    // NOTE: Check if we need to split the list
    void* Result = (void*)((u8*)Found + sizeof(list_alloc_header));
    if (Found->Size - AllocSize > sizeof(list_alloc_header))
    {
        u64 OrigSize = Found->Size;
        Found->Size = AllocSize;
        
        list_alloc_header* Split = (list_alloc_header*)((u8*)Result + AllocSize);
        Split->Size = OrigSize - AllocSize - sizeof(list_alloc_header);
        Split->IsFreed = true;

        // NOTE: Add split to the mem order list
        {
            Split->MemOrderPrev = Found;
            Split->MemOrderNext = Found->MemOrderNext;

            Split->MemOrderPrev->MemOrderNext = Split;
            Split->MemOrderNext->MemOrderPrev = Split;
        }

        // NOTE: Insert split into free list
        {
            Split->Prev = &Allocator->FreeList;
            Split->Next = Allocator->FreeList.Next;

            Split->Prev->Next = Split;
            Split->Next->Prev = Split;
        }
    }
    
    // NOTE: Remove header from free list
    {
        Found->Prev->Next = Found->Next;
        Found->Next->Prev = Found->Prev;
    }

    // NOTE: Insert header into mem list
    {
        Found->Prev = &Allocator->MemList;
        Found->Next = Allocator->MemList.Next;

        Found->Prev->Next = Found;
        Found->Next->Prev = Found;
    }

    Assert(Found->IsFreed);
    Found->IsFreed = false;

    return Result;
}

inline void FreeAlloc(list_allocator* Allocator, void* Mem)
{
    list_alloc_header* Header = (list_alloc_header*)((u8*)Mem - sizeof(list_alloc_header));
    Assert(!Header->IsFreed);
    Header->IsFreed = true;
    
    // NOTE: Remove header from mem list
    {
        Header->Prev->Next = Header->Next;
        Header->Next->Prev = Header->Prev;
    }

    // NOTE: Check if we can merge this cell with a neighbour
    b32 IsLeftFree = Header->MemOrderPrev->IsFreed;
    b32 IsRightFree = Header->MemOrderNext->IsFreed;
    if (IsLeftFree && IsRightFree)
    {
        // NOTE: Prev header becomes the new header here
        list_alloc_header* PrevHeader = Header->MemOrderPrev;
        list_alloc_header* NextHeader = Header->MemOrderNext;
        PrevHeader->Size += sizeof(list_alloc_header)*2 + Header->Size + NextHeader->Size;

        // NOTE: Update new header in mem order list
        {
            PrevHeader->MemOrderNext = NextHeader->MemOrderNext;
            NextHeader->MemOrderNext->MemOrderPrev = PrevHeader;
        }

        // NOTE: Remove next from list
        {
            NextHeader->Prev->Next = NextHeader->Next;
            NextHeader->Next->Prev = NextHeader->Prev;
        }

        // NOTE: Remove header from list
        {
            Header->Prev->Next = Header->Next;
            Header->Next->Prev = Header->Prev;
        }
    }
    else if (IsLeftFree)
    {
        // NOTE: Prev header becomes the new header here
        list_alloc_header* PrevHeader = Header->MemOrderPrev;
        PrevHeader->Size += sizeof(list_alloc_header) + Header->Size;

        // NOTE: Update new header in mem order list
        {
            PrevHeader->MemOrderNext = Header->MemOrderNext;
            Header->MemOrderNext->MemOrderPrev = PrevHeader;
        }

        // NOTE: Remove header from list
        {
            Header->Prev->Next = Header->Next;
            Header->Next->Prev = Header->Prev;
        }
    }
    else if (IsRightFree)
    {
        // NOTE: Header becomes the new header here
        list_alloc_header* NextHeader = Header->MemOrderNext;
        Header->Size += sizeof(list_alloc_header) + NextHeader->Size;

        // NOTE: Update header in mem order list
        {
            Header->MemOrderNext = NextHeader->MemOrderNext;
            NextHeader->MemOrderNext->MemOrderPrev = Header;
        }

        // NOTE: Remove next header from list
        {
            NextHeader->Prev->Next = NextHeader->Next;
            NextHeader->Next->Prev = NextHeader->Prev;
        }

        // NOTE: Insert header into free list
        {
            Header->Prev = &Allocator->FreeList;
            Header->Next = Allocator->FreeList.Next;

            Header->Prev->Next = Header;
            Header->Next->Prev = Header;
        }
    }
    else
    {
        // NOTE: Insert header into free list
        {
            Header->Prev = &Allocator->FreeList;
            Header->Next = Allocator->FreeList.Next;

            Header->Prev->Next = Header;
            Header->Next->Prev = Header;
        }
    }
}

//
// NOTE: Pool allocator
//

//
// NOTE: Thread safe memory allocator
//

struct mem_block
{
    mem_block* Next;
    mem_block* Prev;
};

#define MEM_POOL_BLOCK_SIZE (MegaBytes(10))
struct mem_pool_arena
{
    u64 Size;
    u64 NumBlocks;
    void* Base;
    
    mem_block FreeSentinel;
};

inline mem_pool_arena InitPoolArena(u64 Size, void* Base)
{
    mem_pool_arena Arena = {};
    Arena.Size = Size;
    Arena.Base = Base;

    // NOTE: Build the free list
    Assert(IsDivisible(Size, MEM_POOL_BLOCK_SIZE + sizeof(mem_block)));
    Arena.NumBlocks = Size / (MEM_POOL_BLOCK_SIZE + sizeof(mem_block));
    u8* CurrByte = (u8*)Arena.Base;

    Arena.FreeSentinel.Prev = &Arena.FreeSentinel;
    Arena.FreeSentinel.Next = &Arena.FreeSentinel;

    mem_block* CurrBlock = (mem_block*)Arena.Base;
    for (u64 BlockId = 0; BlockId < Arena.NumBlocks; ++BlockId)
    {
        CurrBlock->Prev = &Arena.FreeSentinel;
        CurrBlock->Next = Arena.FreeSentinel.Next;

        CurrBlock->Prev->Next = CurrBlock;
        CurrBlock->Next->Prev = CurrBlock;

        CurrBlock += 1;
    }

    return Arena;
}

internal void* PushSize(mem_pool_arena* Arena, u64 Size)
{
    void* Result = 0;
    
    if (Size > (MEM_POOL_BLOCK_SIZE - sizeof(mem_block)))
    {
        InvalidCodePath;
    }
    else
    {
        // NOTE: All blocks are taken, out of memory
        Assert(Arena->FreeSentinel.Next != &Arena->FreeSentinel);

        mem_block* Block = Arena->FreeSentinel.Next;
        u64 BlockIndex = ((u64)Block - (u64)Arena->Base) / sizeof(mem_block);
        u64 BlockMemOffset = Arena->NumBlocks*sizeof(mem_block) + BlockIndex*MEM_POOL_BLOCK_SIZE;
        Result = (void*)((u8*)Arena->Base + BlockMemOffset);

        Block->Prev->Next = Block->Next;
        Block->Next->Prev = Block->Prev;

        Block->Prev = 0;
        Block->Next = 0;
    }

    return Result;
}

internal void Free(mem_pool_arena* Arena, void* Data)
{
    u64 BlockMemOffset = (u64)Data - (u64)Arena->Base;
    u64 BlockId = (BlockMemOffset - Arena->NumBlocks*sizeof(mem_block)) / MEM_POOL_BLOCK_SIZE;

    mem_block* Block = (mem_block*)((u8*)Arena->Base + sizeof(mem_block)*BlockId);
    Block->Prev = &Arena->FreeSentinel;
    Block->Next = Arena->FreeSentinel.Next;

    Block->Prev->Next = Block;
    Block->Next->Prev = Block;
}

//
// NOTE: Linear allocator
// 

inline mem_arena InitArena(void* Mem, mm Size)
{
    mem_arena Result = {};
    Result.Size = Size;
    Result.Used = 0;
    Result.Mem = (u8*)Mem;

    return Result;
}

inline mem_double_arena InitDoubleArena(void* Mem, mm Size)
{
    mem_double_arena Result = {};
    Result.Size = Size;
    Result.UsedTop = 0;
    Result.UsedBot = 0;
    Result.Mem = (u8*)Mem;

    return Result;
}

inline void ClearArena(mem_arena* Arena)
{
    Arena->Used = 0;
}

inline void ClearArena(mem_double_arena* Arena)
{
    Arena->UsedTop = 0;
    Arena->UsedBot = 0;
}

inline temp_mem BeginTempMem(mem_arena* Arena)
{
    // NOTE: This function lets us take all memory allocated past this point and later
    // free it
    temp_mem TempMem = {};
    TempMem.Arena = Arena;
    TempMem.Used = Arena->Used;

    return TempMem;
}

inline void EndTempMem(temp_mem TempMem)
{
    TempMem.Arena->Used = TempMem.Used;
}

inline temp_double_mem BeginTempMem(mem_double_arena* Arena)
{
    // NOTE: This function lets us take all memory allocated past this point and later
    // free it
    temp_double_mem TempMem = {};
    TempMem.Arena = Arena;
    TempMem.UsedTop = Arena->UsedTop;
    TempMem.UsedBot = Arena->UsedBot;

    return TempMem;
}

inline void EndTempMem(temp_double_mem TempMem)
{
    TempMem.Arena->UsedTop = TempMem.UsedTop;
    TempMem.Arena->UsedBot = TempMem.UsedBot;
}

#define PushStruct(Arena, type) (type*)PushSize(Arena, sizeof(type))
#define PushArray(Arena, type, count) (type*)PushSize(Arena, sizeof(type)*count)
inline void* PushSize(mem_arena* Arena, mm Size)
{
    Assert((Arena->Used + Size) <= Arena->Size);
    void* Result = Arena->Mem + Arena->Used;
    Arena->Used += Size;
    
    return Result;
}

inline void* PushSize(mem_double_arena* Arena, mm Size)
{
    Assert(Arena->UsedTop + Arena->UsedBot + Size <= Arena->Size);
    void* Result = Arena->Mem + Arena->UsedTop;
    Arena->UsedTop += Size;
    
    return Result;
}

#define BotPushStruct(Arena, type) (type*)BotPushSize(Arena, sizeof(type))
#define BotPushArray(Arena, type, count) (type*)BotPushSize(Arena, sizeof(type)*count)
inline void* BotPushSize(mem_double_arena* Arena, mm Size)
{
    Assert(Arena->UsedTop + Arena->UsedBot + Size <= Arena->Size);
    Arena->UsedBot += Size;
    void* Result = Arena->Mem + Arena->Size - Arena->UsedBot;
    
    return Result;
}

inline mem_arena SubArena(mem_arena* Arena, mm Size)
{
    mem_arena Result = {};
    Result.Size = Size;
    Result.Used = 0;
    Result.Mem = (u8*)PushSize(Arena, Size);

    return Result;
}

inline void ClearMem(void* Mem, mm Size)
{
    u8* CurrByte = (u8*)Mem;
    for (mm Byte = 0; Byte < Size; ++Byte)
    {
        *CurrByte++ = 0;
    }
}

inline void Copy(void* Mem, void* Dest, mm Size)
{
    u8* CurrentByte = (u8*)Mem;
    u8* DestByte = (u8*)Dest;
    for (mm Byte = 0; Byte < Size; ++Byte)
    {
        *DestByte++ = *CurrentByte++;
    }
}

inline void ZeroMem(void* Mem, mm NumBytes)
{
    u8* ByteMem = (u8*)Mem;
    for (mm ByteId = 0; ByteId < NumBytes; ++ByteId)
    {
        *ByteMem = 0;
        ByteMem++;
    }
}

#define ShiftPtrByBytes(Ptr, Step, Type) (Type*)ShiftPtrByBytes_((u8*)Ptr, Step)
inline u8* ShiftPtrByBytes_(u8* Ptr, mm Step)
{
    u8* Result = Ptr + Step;
    return Result;
}
