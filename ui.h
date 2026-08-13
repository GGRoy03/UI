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
    UIAlignment_Start        = 0,
    UIAlignment_Center       = 1,
    UIAlignment_End          = 2,
    UIAlignment_SpaceBetween = 3,
    UIAlignment_SpaceAround  = 4,
    UIAlignment_SpaceEvenly  = 5,
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
//
// NOTE:
// x) The task naming is not really appropriate as a task
// is associated with the scheduler while this has nothing
// to do with it.
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
    float       ParentSize;
    ui_padding  Padding;
    float       Spacing;
    UIAlignment Alignment;
    float      *ChildSize;
    uint32_t    ChildCount;
} ui_place_major_axis_task;


//
// TODO:
// Current writing this function as a main-axis placer only. Is it possible to generalize or do I
// need another function? I think i'd be hard to generalize without adding extra branching?
//

static void
UILayoutPlaceMajorAxis(const ui_place_major_axis_task *Tasks, uint32_t Count, float *OutPosition)
{
    //
    // TODO: Validate Inputs Or Something
    //

    uint32_t OutputOffset = 0;
    for(uint32_t TaskIdx = 0; TaskIdx < Count; ++TaskIdx)
    {
        const ui_place_major_axis_task *Task = Tasks + TaskIdx;

        //
        // Compute the total space occupied by the children logically.
        //
        // NOTE:
        // Something like that is slightly more complex, because we might have to account for some other data:
        // 1) Border-Width (in some cases the border width may affect the layout depending on what the user decides)
        // 2) Margin (spacing around the child as I understand it)
        //

        float ChildrenSize = 0.0f;
        for(uint32_t ChildIdx = 0; ChildIdx < Task->ChildCount; ++ChildIdx)
        {
            ChildrenSize += Task->ChildSize[ChildIdx];
        }

        //
        // Compute the alignment offset for the cursor:
        // 1) Where the cursor should start placing.
        // 2) How the cursor should change after placing a child.
        //
        // NOTE:
        // 1) Are there other cases we need to handle or is this sufficient?
        //
        // TODO:
        // 1) Probably made a bunch of mistakes/missed edge cases in the actual value.
        // 2) We are not accounting for the padding (both space and cursor offset)
        //

        float ParentSpace = Task->ParentSize;
        if(Task->ChildCount > 1)
        {
            ParentSpace = Task->ParentSize - (ChildrenSize + ((Task->ChildCount - 1) * Task->Spacing));
        }

        float StartOffset = 0.0f;
        float PlaceOffset = 0.0f;
        if(ParentSpace > 0.0f)
        {
            switch(Task->Alignment)
            {
    
            case UIAlignment_Center:
            {
                StartOffset = ParentSpace * 0.5f;
                PlaceOffset = 0.0f;
            } break;
     
            case UIAlignment_End:
            {
                StartOffset = ParentSpace;
                PlaceOffset = 0.0f;
            } break;

            case UIAlignment_SpaceBetween:
            {
                StartOffset = 0.0f;
                PlaceOffset = Task->ChildCount > 1 ? (ParentSpace / (Task->ChildCount - 1)) : 0.0f;
            } break;

            case UIAlignment_SpaceAround:
            {
                StartOffset = Task->ChildCount > 1 ? (ParentSpace / (Task->ChildCount * 2)) : 0.0f;
                PlaceOffset = StartOffset * 2.0f;
            } break;

            case UIAlignment_SpaceEvenly:
            {
                StartOffset = (ParentSpace / (Task->ChildCount + 1));
                PlaceOffset = StartOffset;
            } break;

            default: break;
     
            }
        }
        else
        {
            //
            // TODO:
            // I don't really know how to handle the overflow case. I think it might just be a completely different
            // placing branch. We need a 2D cursor in that case...
            //
        }

        //
        // Write the resulting positions in the output buffer.
        //

        float Cursor = StartOffset;
        for(uint32_t ChildIdx = 0; ChildIdx < Task->ChildCount; ++ChildIdx)
        {
            OutPosition[ChildIdx + OutputOffset] = Cursor;
            Cursor                              += PlaceOffset;
        }

        OutputOffset += Task->ChildCount;
    }
}


// ==========================================================
// [SECTION] : LAYOUT SCHEDULER
// ========================================================


//
// TODO:
// There's really no reason to store pointers instead of indices.
// Also, we might break this data into different arrays depending on our access patterns.
// It's only packed into one structure for simplicity.
//

typedef struct ui_scheduler_entry ui_scheduler_entry;
struct ui_scheduler_entry
{
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


//
// Uhm, well this still fits the same model as above since the above code doens't take in any dependency (it doesn't have any)
//

static void
UIPushSchedulerSpaceDistributionTaskData(ui_scheduler_task Task)
{
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
    ui_sizing SizingX;
    ui_sizing SizingY;
    ui_size   Min;
    ui_size   Max;
} ui_vertical_content;


typedef struct ui_vertical_content_node ui_vertical_content_node;
struct ui_vertical_content_node
{
    ui_vertical_content       Content;
    ui_vertical_content_node *Next;
};


typedef struct
{
    float        Spacing;
    ui_padding   Padding;
    float        BorderWidth;
    UIAlignment  XAlignment;
    UIAlignment  YAlignment;
    uint32_t     ChildCount;
    ui_sizing    SizingX;
    ui_sizing    SizingY;
    ui_size      MinSize;
    ui_size      MaxSize;

    ui_vertical_content_node *FstContent;
    ui_vertical_content_node *LstContent;
    uint32_t                  ContentCount;

    //
    // NOTE:
    // Not sure this is correct. Sharing ressources implicitly between structures may be a mistake? Though it should
    // be sound logically.
    //

    ui_linear_allocator      *WindowAllocator;
} ui_vertical_layout;


static ui_vertical_layout *
UIEnterVerticalLayout(const ui_vertical_content *Content, ui_window *Window)
{
    ui_vertical_layout *Result = 0;

    if(Window)
    {
        ui_vertical_layout *Layout = UIAllocateLinearStruct(ui_vertical_layout, &Window->FrameAllocator);
        if(Layout)
        {
            Layout->Spacing     = 0.0f;
            Layout->Padding     = {};
            Layout->BorderWidth = 0.0f;
            Layout->XAlignment  = UIAlignment_Start;
            Layout->YAlignment  = UIAlignment_Start;
            Layout->SizingX     = (Content != 0) ? Content->SizingX : (ui_sizing){};
            Layout->SizingY     = (Content != 0) ? Content->SizingY : (ui_sizing){};
            Layout->MinSize     = (Content != 0) ? Content->Min     : (ui_size){};
            Layout->MaxSize     = (Content != 0) ? Content->Max     : (ui_size){};
        }

        Result = Layout;
    }

    return Result;
}


static ui_vertical_content *
UIPushVerticalContent(ui_vertical_layout *Layout)
{
    ui_vertical_content *Result = 0;

    if(Layout)
    {
        ui_vertical_content_node *Node = UIAllocateLinearStruct(ui_vertical_content_node, Layout->WindowAllocator);
        if(Node)
        {
            //
            // Link the content node in the internal content linked list.
            //

            if(!Layout->FstContent)
            {
                Layout->FstContent = Node;
                Layout->LstContent = Node;
            }
            else if(Layout->LstContent)
            {
                Layout->LstContent->Next = Node;
                Layout->LstContent       = Node;
            }
            ++Layout->ContentCount;

            //
            // Write the result.
            //

            Result = &Node->Content;
            Result->SizingX = {};
            Result->SizingY = {};
            Result->Min     = {};
            Result->Max     = {};
        }
    }

    return Result;
}


static void
UILeaveVerticalLayout(const ui_vertical_layout *Layout, ui_window *Window)
{
    if(Layout && Window)
    {
        ui_scheduler *Scheduler = Window->Scheduler;

        //
        // Okay.. and again, how do we schedule ours tasks. This is the part that's missing.
        // Yeah I don't know. This whole thing is quite elusive. What do I have to do for sure here:
        // x) Major Axis Placement -> Some sort of reference to children, vertical layout parameters, depends on:
        // x) Minor Axis Placement -> Some sort of reference to children, vertical layout parameters, depends on:
        // x) Major Axis Resizing  -> Some sort of reference to children, vertical layout parameters, depends on:
        //
        // Do I HAVE to do these here:
        // x) Child sizing task?
        // x) Child dependency injection?
        //
        // Think in layout and content.
        //
        // Okay, I have the strongest structure yet. It is still hard though. I have everything HERE to make decisions which
        // I think is the correct approach instead of scattering knowledge around. Let's say we were to use the same scheduler
        // approach.
        //

        //
        // Let's just try and figure out one axis by bruteforcing.
        //

        if(Layout->SizingX.Type == UISizing_Fixed)
        {
        }
        else if(Layout->SizingX.Type == UISizing_Percent)
        {
        }
        else if(Layout->SizingX.Type == UISizing_Fit)
        {
            //
            // This is already sort of incorrect. Well. Uhm. This is just weird. We have to weirdly reason through the dependency
            // chain which can't be the correct approach. For example, if I push some sort of resizing task, then I'm basically
            // wiring the same dependency chain twice. I could then only inject the dependency on the parent when there's no
            // resize task, but that requires understanding the full dependency chain. And it feels like the scheduler is not
            // really doing its job.
            //
            // Perhaps something else we could explore, is a dependency chain resolver, we specify what we might depend on and it
            // runs the magic (for example: For my computation to be exact I depend on my parent's natural size, I depend on these node's
            // final size, I depend on this node's final size. Can we somehow separate things such that the dependency wiring is much
            // easier? We want to do the minimum work here such that it's super easy to do stuff. This is simply too hard. I think
            // most of the structure around the code is correct, just this particular task/scheduler idea is incorrect, but maybe pointing
            // in the correct direction.
            //

            ui_scheduler_task ParentTask = UIAcquireSchedulerTaskEntry(Scheduler);
            for(ui_vertical_content_node *Node = Layout->FstContent; Node != 0; Node = Node->Next)
            {
                ui_vertical_content Content = Node->Content;

                if(Content.SizingX.Type == UISizing_Fixed)
                {
                    ui_scheduler_task ChildTask = UIAcquireSchedulerTaskEntry(Scheduler);
                    {
                        UIPushSchedulerFixedSizingTaskData(ChildTask, Content.SizingX.Fixed, Content.Min.X, Content.Max.X, Scheduler);
                        UIInjectSchedulerDependency(ParentTask, ChildTask, Scheduler);
                    }
                }
                else if(Content.SizingX.Type == UISizing_Percent)
                {
                }
                else if(Content.SizingX.Type == UISizing_Fit)
                {
                }
            }

            UIInjectSchedulerDependency(ui_scheduler_task Dependent, ui_scheduler_task Dependency, ui_scheduler *Scheduler);
        }

        for(ui_vertical_content_node *Node = Layout->FstContent; Node != 0; Node = Node->Next)
        {
        }
    }
}


# endif // UI_HEADER
