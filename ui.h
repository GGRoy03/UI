# ifndef UI_HEADEhhh
#   define UI_HEADER
# endif

# ifdef UI_HEADER


// ==========================================================
// [SECTION] : INCLUDES
// ==========================================================


# include <stdint.h>
# include <assert.h>
# include <stdbool.h>


// ==========================================================
// [SECTION] : CONTEXT CRACKING | HELPERS
// ==========================================================


# if defined(__clang__)
#   define UI_COMPILER_CLANG
# elif defined(__GNUC__)
#   define UI_COMPILER_GNU
# elif defined(_MSC_VER)
#   define UI_COMPILER_MSVC
# else
#   error "UNKNOWN COMPILER"
# endif


# ifdef UI_COMPILER_CLANG
#  define UI_ALIGN_OF(X) (__alignof__(X))
# else
#  error "UNIMPLEMENTED ALIGNOF MACRO"
# endif


// ==========================================================
// [SECTION] : COMMON MACROS | HELPERS
// ==========================================================


# define UI_KIB(X) ((X * 1024))
# define UI_MIB(X) ((UI_KIB(X) * 1024))
# define UI_GIB(X) ((UI_MIB(X) * 1024))

# define UI_MIN(A, B) ((A <= B) ? A : B)
# define UI_MAX(A, B) ((A >= B) ? A : B)

# define UI_VALIDATE_ENUM(Value, MaxExclusive) ((Value >= 0) && (Value < MaxExclusive))
# define UI_ARRAY_COUNT(Array)                 ((sizeof(Array) / sizeof(Array[0])))

# define UI_BIT_MASK(BitCount)          ((1ull << BitCount) - 1ull)
# define UI_CHECK_BIT_MASK(Value, Mask) ((Value & Mask))


// ==========================================================
// [SECTION] : ...
// ==========================================================


typedef enum
{
    UIAlignment_Start  = 0,
    UIAlignment_Center = 1,
    UIAlignment_End    = 2,
} UIAlignment;


typedef enum
{
    UISizing_Fixed   = 0,
    UISizing_Percent = 1,
    UISizing_Fit     = 2,
} UISizing;


typedef struct
{
    UISizing Type;
    union
    {
        float Fixed;
        float Percent;
    };
} ui_sizing;


typedef struct
{
    float X;
    float Y;
} ui_size;


typedef struct
{
    float Left;
    float Top;
    float Right;
    float Bottom;
} ui_padding;


//
// "Describes" children, or I guess an element within a vertical context?
// Could it be the parent itself? I don't think so, because the parent itself has just
// more responsability in most cases?
//



// ==========================================================
// [SECTION] : ALLOCATORS
// ==========================================================


static const size_t UI_LINEAR_ALLOCATOR_DEFAULT_ALIGNMENT = 64;
static const size_t UI_CHUNK_ALLOCATOR_CHUNK_SIZE         = UI_KIB(16);


//
// TODO: Align Of (Compiler Detection?)
//

#define UIAllocateLinearArray(Count, Type, Allocator) (Type *)UIAllocateLinearAligned((Count * sizeof(Type)), UI_ALIGN_OF(Type), Allocator)
#define UIAllocateLinearStruct(Type, Allocator)       (Type *)UIAllocateLinearAligned((sizeof(Type)), UI_ALIGN_OF(Type), Allocator)


typedef struct
{
    uint8_t *Buffer;
    size_t   BufferSize;
    size_t   At;
} ui_linear_allocator;


static bool
UIIsPowerOfTwo(uintptr_t Value)
{
    bool Result = Value != 0 && (Value & (Value - 1)) == 0;
    return Result;
}


static uintptr_t
UIAlignForward(uintptr_t Pointer, uintptr_t Align)
{
    assert(UIIsPowerOfTwo(Align));

    uintptr_t Result = Pointer;

    uintptr_t Remainder = (Pointer & (Align - 1));
    if(Remainder != 0)
    {
        Result -= Align - Remainder;
    }

    return Result;
}


static bool
UIIsValidLinearAllocator(ui_linear_allocator *Allocator)
{
    bool Result = Allocator && Allocator->Buffer;
    return Result;
}


static ui_linear_allocator
UILinearAllocator(void *BackingBuffer, size_t Size)
{
    ui_linear_allocator Result =
    {
        .Buffer     = (uint8_t *)BackingBuffer,
        .BufferSize = Size,
        .At         = 0,
    };

    return Result;
}


static void *
UIAllocateLinearAligned(size_t Size, size_t Align, ui_linear_allocator *Allocator)
{
    void *Result = 0;

    if(UIIsValidLinearAllocator(Allocator))
    {
        uintptr_t CurrentPointer = (uintptr_t)(Allocator->Buffer + Allocator->At);
        uintptr_t AlignedPointer = UIAlignForward(CurrentPointer, Align);
        uintptr_t RelativeOffset = AlignedPointer - (uintptr_t)(Allocator->Buffer);

        if(RelativeOffset + Size < Allocator->BufferSize)
        {
            Result = Allocator->Buffer + RelativeOffset;
            Allocator->At = RelativeOffset + Size;
        }
    }

    return Result;
}


static void *
UIAllocateLinear(size_t Size, ui_linear_allocator *Allocator)
{
    void *Result = UIAllocateLinearAligned(Size, UI_LINEAR_ALLOCATOR_DEFAULT_ALIGNMENT, Allocator);
    return Result;
}


typedef struct
{
    uint32_t Count;
    uint64_t SizeOfType;
    uint64_t Align;
} ui_linear_array_desc;


static uint64_t
UISizeOfLinearArray(const ui_linear_array_desc *Descriptions, uint32_t Count)
{
    uint64_t Result = 0;

    if(Descriptions)
    {
        uint64_t Current = 0;
        for(uint32_t DescIdx = 0; DescIdx < Count; ++DescIdx)
        {
            const ui_linear_array_desc *Desc = Descriptions + DescIdx;

            uintptr_t Aligned    = UIAlignForward(Current, Desc->Align);
            uint64_t  Difference = Aligned - Current;
            uint64_t  ArraySize  = Desc->Count * Desc->SizeOfType;

            Current += (Difference + ArraySize);
        }
    }

    return Result;
}


typedef struct
{
    uint64_t Size;
} ui_memory_size_counter;


static void
UIMemorySizeCounterAppend(uint64_t BufferSize, uint32_t Align, ui_memory_size_counter *Counter)
{
    if(Counter)
    {
        uintptr_t Aligned    = UIAlignForward(Counter->Size, Align);
        uint64_t  Difference = Aligned - Counter->Size;

        Counter->Size += Difference + BufferSize;
    }
}


static uint64_t
UIMemorySizeCounterWorstCase(uint32_t Align, ui_memory_size_counter Counter)
{
    assert(Align > 0);

    uint64_t Result = Counter.Size + (Align - 1);
    return Result;
}


#define UIMemorySizeCountBuffer(Type, Count, Counter) UIMemorySizeCounterAppend((Count * sizeof(Type)), (UI_ALIGN_OF(Type)), Counter)
#define UIMemorySizeCountStruct(Type, Counter)        UIMemorySizeCounterAppend((sizeof(Type)), (UI_ALIGN_OF(Type)), Counter)


// ==========================================================
// [SECTION] : LAYOUT ALGORITHMS
// ==========================================================


typedef struct
{
    float Size;
    float MinSize;
    float MaxSize;
} ui_fixed_sizing_task;


static void
UILayoutComputeFixedSizing(const ui_fixed_sizing_task *Tasks, uint32_t Count, float *OutSize)
{
    //
    // TODO: Validate Inputs Or Something
    //

    for(uint32_t TaskIdx = 0; TaskIdx < Count; ++TaskIdx)
    {
        ui_fixed_sizing_task Task = Tasks[TaskIdx];

        float MinSize = UI_MAX(Task.MinSize, 0.0f);
        float MaxSize = UI_MAX(Task.MaxSize, 0.0f);
        if(MinSize <= MaxSize)
        {
            OutSize[TaskIdx] = UI_MIN(UI_MAX(Task.Size, MinSize), MaxSize);
        }
        else
        {
            OutSize[TaskIdx] = 0.0f;
        }
    }
}


typedef struct
{
    float    ParentSpace;
    float   *ChildSize;
    uint32_t ChildCount;
} ui_naive_space_sharing;


static void
UILayoutComputeNaiveSpaceSharing(const ui_naive_space_sharing *Tasks, uint32_t Count, float *OutSize)
{
    //
    // TODO: Validate Inputs Or Something
    //

    for(uint32_t TaskIdx = 0; TaskIdx < Count; ++TaskIdx)
    {
        ui_naive_space_sharing Task = Tasks[TaskIdx];

        float TotalChildSize = 0.0f;
        for(uint32_t ChildIdx = 0; ChildIdx < Task.ChildCount; ++ChildIdx)
        {
            TotalChildSize += Task.ChildSize[ChildIdx];
        }

        float Scale = Task.ParentSpace / TotalChildSize;
        for(uint32_t ChildIdx = 0; ChildIdx < Task.ChildCount; ++ChildIdx)
        {
            OutSize[ChildIdx] = Task.ChildSize[ChildIdx] * Scale;
        }
    }
}


// ==========================================================
// [SECTION] : LAYOUT SCHEDULER
// ========================================================


typedef enum
{
    UIAxis_X = 0,
    UIAxis_Y = 1,
} UIAxis;


typedef enum
{
    UILayoutBucket_FixedSizingX = 0,
    UILayoutBucket_FixedSizingY = 1,

    UILayoutBucket_Count,
    UILayoutBucket_Unknown,
} UILayoutBucket;

//
// TODO:
// There's really no reason to store pointers instead of indices.
// Also, we might break this data into different arrays depending on our access patterns.
// It's only packed into one structure for simplicity.
//

typedef struct ui_scheduler_entry ui_scheduler_entry;
struct ui_scheduler_entry
{
    UILayoutBucket      Bucket;
    uint32_t            Counter;
    uint32_t            ByteOffset;
    ui_scheduler_entry *Head;
    ui_scheduler_entry *Next;
};


typedef struct
{
    ui_scheduler_entry *Entries;
    uint32_t            EntryCount;
    uint32_t            EntrySize;
    ui_linear_allocator PayloadAllocator;
} ui_scheduler;


typedef struct
{
    uint32_t Value;
} ui_scheduler_task;


static bool
UIIsValidScheduler(const ui_scheduler *Scheduler)
{
    bool Result = (Scheduler != 0);
    return Result;
}


static ui_scheduler_entry *
UIGetSchedulerEntryFromTask(ui_scheduler_task Task, const ui_scheduler *Scheduler)
{
    ui_scheduler_entry *Result = 0;

    if(Task.Value < Scheduler->EntryCount)
    {
        Result = (ui_scheduler_entry *)Scheduler->Entries + Task.Value;
    }

    return Result;
}


static ui_scheduler_task
UIAcquireSchedulerTaskEntry(ui_scheduler *Scheduler)
{
    //
    // TODO:
    // Write this code properly.
    //

    ui_scheduler_task Result = {};

    if(Scheduler)
    {
        if(Scheduler->EntryCount < Scheduler->EntrySize)
        {
            Result.Value           = Scheduler->EntryCount;
            Scheduler->EntryCount += 1;
        }
    }

    return Result;
}


static void
UIInjectSchedulerDependency(ui_scheduler_task Dependent, ui_scheduler_task Dependency, ui_scheduler *Scheduler)
{
    //
    // TODO:
    // Write this code properly.
    //

    if(Scheduler)
    {
        ui_scheduler_entry *DependentEntry  = UIGetSchedulerEntryFromTask(Dependent , Scheduler);
        ui_scheduler_entry *DependencyEntry = UIGetSchedulerEntryFromTask(Dependency, Scheduler);

        if(DependentEntry && DependencyEntry)
        {
            //
            // Something like that...
            //

            DependentEntry->Counter += 1;

            if(DependencyEntry->Head)
            {
                DependencyEntry->Head->Next = DependentEntry;
            }
            DependencyEntry->Head = DependencyEntry;
        }
    }
}


static void *
UIAcquireSchedulerPayloadSlice(uint64_t Size, ui_scheduler_task Task, ui_scheduler *Scheduler)
{
    //
    // TODO:
    // Write this code properly.
    //

    void *Result = 0;

    if(Scheduler)
    {
        //
        // BUG:
        // 0 is not a valid check since the first command write will be at the 0 offset which would
        // allow one to write multiple times to the same slot.
        //

        ui_scheduler_entry *Entry = UIGetSchedulerEntryFromTask(Task, Scheduler);
        if(Entry && Entry->ByteOffset == 0)
        {
            //
            // TODO:
            // We should care about alignment... right? Meh, maybe not for now since this might change allocator anyway.
            //

            void *Data = UIAllocateLinear(Size, &Scheduler->PayloadAllocator);
            if(Data)
            {
                Entry->ByteOffset = Scheduler->PayloadAllocator.At - Size;
                Result            = Data;
            }
        }
    }

    return Result;
}


//
// NOTE:
// Could we just be taking in the command? Do we want to depend on that type?
// We internally already do depend on this type. Should the outer code? Why not?
//


static void
UIPushSchedulerFixedSizingTaskData(ui_scheduler_task Task, float Size, float MinSize, float MaxSize, ui_scheduler *Scheduler)
{
    if(UIIsValidScheduler(Scheduler))
    {
        //
        // TODO:
        // This is not a valid way to check if an entry has data already mapped to it.
        //

        ui_scheduler_entry *Entry = UIGetSchedulerEntryFromTask(Task, Scheduler);
        if(Entry && Entry->ByteOffset == 0)
        {
            ui_fixed_sizing_task *Task = UIAllocateLinearStruct(ui_fixed_sizing_task, &Scheduler->PayloadAllocator);
            if(Task)
            {
                Task->Size    = Size;
                Task->MinSize = MinSize;
                Task->MaxSize = MaxSize;
            }
        }
    }
}





typedef struct
{
    uint64_t TaskPayloadSize;
    uint32_t EntryCount;
} ui_scheduler_params;


static uint64_t
UISchedulerMemorySize(ui_scheduler_params Params)
{
    ui_memory_size_counter Counter = {};
    {
        UIMemorySizeCountBuffer(ui_scheduler_entry, Params.EntryCount, &Counter);
        UIMemorySizeCountBuffer(uint8_t, Params.TaskPayloadSize, &Counter);
        UIMemorySizeCountStruct(ui_scheduler, &Counter);
    }

    uint64_t Result = UIMemorySizeCounterWorstCase(UI_ALIGN_OF(ui_scheduler), Counter);
    return Result;
}


static ui_scheduler *
UISchedulerMemoryInit(void *Memory, uint64_t Size, ui_scheduler_params Params)
{
    ui_scheduler *Result = 0;

    ui_linear_allocator Allocator = UILinearAllocator(Memory, Size);
    if(UIIsValidLinearAllocator(&Allocator))
    {
        ui_scheduler_entry *Entries   = UIAllocateLinearArray(Params.EntryCount, ui_scheduler_entry, &Allocator);
        uint8_t            *Payload   = UIAllocateLinearArray(Params.TaskPayloadSize, uint8_t, &Allocator);
        ui_scheduler       *Scheduler = UIAllocateLinearStruct(ui_scheduler, &Allocator);

        if(Scheduler)
        {
            Scheduler->PayloadAllocator = UILinearAllocator(Payload, Params.TaskPayloadSize);
            Scheduler->Entries          = Entries;
            Scheduler->EntryCount       = 0;
            Scheduler->EntrySize        = Params.EntryCount;

            for(uint32_t EntryIdx = 0; EntryIdx < Params.EntryCount; ++EntryIdx)
            {
                ui_scheduler_entry *Entry = Scheduler->Entries + EntryIdx;
                Entry->Counter    = 0;
                Entry->Bucket     = UILayoutBucket_Unknown;
                Entry->ByteOffset = 0;
                Entry->Head       = 0;
                Entry->Next       = 0;
            }
        }

        Result = Scheduler;
    }

    return Result;
}


// ==========================================================
// [SECTION] : WINDOW
// ==========================================================


typedef struct
{
    ui_linear_allocator FrameAllocator;
    ui_scheduler       *Scheduler;
} ui_window;


typedef struct
{
    uint64_t            FrameMemorySize;
    ui_scheduler_params Scheduler;
} ui_window_params;


static uint64_t
UIWindowMemorySize(ui_window_params Params)
{
    ui_memory_size_counter Counter = {};
    {
        uint64_t SchedulerSize = UISchedulerMemorySize(Params.Scheduler);
        {
            UIMemorySizeCountBuffer(uint8_t, SchedulerSize, &Counter);
        }

        UIMemorySizeCountBuffer(uint8_t, Params.FrameMemorySize, &Counter);
        UIMemorySizeCountStruct(ui_window, &Counter);
    }

    uint64_t Result = UIMemorySizeCounterWorstCase(UI_ALIGN_OF(ui_window), Counter);
    return Result;
}


static ui_window *
UIWindowMemoryInit(void *Memory, uint64_t Size, ui_window_params Params)
{
    ui_window *Result = 0;

    ui_linear_allocator Allocator = UILinearAllocator(Memory, Size);
    if(UIIsValidLinearAllocator(&Allocator))
    {
        uint64_t SchedulerSize = UISchedulerMemorySize(Params.Scheduler);

        uint8_t   *SchedulerMemory = UIAllocateLinearArray(SchedulerSize, uint8_t, &Allocator);
        uint8_t   *FrameMemory     = UIAllocateLinearArray(Params.FrameMemorySize, uint8_t, &Allocator);
        ui_window *Window          = UIAllocateLinearStruct(ui_window, &Allocator);

        if(Window)
        {
            Window->FrameAllocator = UILinearAllocator(FrameMemory, Params.FrameMemorySize);
            Window->Scheduler      = UISchedulerMemoryInit(SchedulerMemory, SchedulerSize, Params.Scheduler);
        }

        Result = Window;
    }

    return Result;
}


// ==========================================================
// [SECTION] : TEMPORARY
// ==========================================================


typedef struct
{
    ui_sizing SizingX;
    ui_sizing SizingY;
    ui_size   MinSize;
    ui_size   MaxSize;
} ui_node_desc;

// ==========================================================
// [SECTION] : VERTICAL LAYOUT | WINDOW
// ==========================================================


typedef struct
{
    float        Spacing;
    ui_padding   Padding;
    float        BorderWidth;
    UIAlignment  XAlignment;
    UIAlignment  YAlignment;
    ui_node_desc Parent;
    uint32_t     ChildCount;
} ui_vertical_layout;


//
// TODO:
// Add more parameters.
//

static ui_vertical_layout
UIEnterVerticalLayout(ui_node_desc Parent)
{
    ui_vertical_layout Result =
    {
        .Spacing     = 5.0f,
        .Padding     = {},
        .BorderWidth = 0.0f,
        .XAlignment  = UIAlignment_Start,
        .YAlignment  = UIAlignment_Start,
        .Parent      = Parent,
    };

    return Result;
}


static void
UIPushVeritcalNode(ui_node_desc Node, ui_vertical_layout *Layout, ui_window *Window)
{
    //
    // TODO:
    // Write this code correctly.
    //

    if(Layout && Window)
    {
        //
        // TODO:
        // Remove the duplicated logic once we get it right.
        //

        if(Node.SizingX.Type == UISizing_Fixed)
        {
            ui_scheduler_task Task = UIAcquireSchedulerTaskEntry(Window->Scheduler);
            {
                UIPushSchedulerFixedSizingTaskData(Task, Node.SizingX.Fixed, Node.MinSize.X, Node.MaxSize.X, Window->Scheduler);
            }
        }

        if(Node.SizingY.Type == UISizing_Fixed)
        {
            ui_scheduler_task Task = UIAcquireSchedulerTaskEntry(Window->Scheduler);
            {
                UIPushSchedulerFixedSizingTaskData(Task, Node.SizingY.Fixed, Node.MinSize.Y, Node.MaxSize.Y, Window->Scheduler);
            }
        }
    }
}


static void
UILeaveVerticalLayout(const ui_vertical_layout *Layout, ui_window *Window)
{
    if(Layout && Window)
    {
        //
        // Okay.. and again, how do we schedule ours tasks. This is the part that's missing.
        //
    }
}


# endif // UI_HEADER
