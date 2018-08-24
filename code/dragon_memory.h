#if !defined(COCO_MEMORY_H)

struct mem_arena
{
    mm Size;
    mm Used;
    u8* Mem;
};

struct temp_mem
{
    mem_arena* Arena;
    mm Used;
};

struct mem_double_arena
{
    mm Size;
    mm UsedTop;
    mm UsedBot;
    u8* Mem;
};

struct temp_double_mem
{
    mem_double_arena* Arena;
    mm UsedTop;
    mm UsedBot;
};

struct stack_alloc_header
{
    stack_alloc_header* Prev;
};

struct stack_allocator
{
    mm Size;
    mm Used;
    u8* Base;
    stack_alloc_header* TopHeader;
};

struct list_alloc_header
{
    b32 IsFreed;
    mm Size;
    list_alloc_header* Next;
    list_alloc_header* Prev;

    list_alloc_header* MemOrderNext;
    list_alloc_header* MemOrderPrev;
};

struct list_allocator
{
    list_alloc_header MemList;
    list_alloc_header FreeList;
    list_alloc_header MemOrderList;
};

/*
  TODO: For free list, we have a bunch of possible optimizations:
      - Merge cells on the free list (requires global ordering)

*/

#define COCO_MEMORY_H
#endif
