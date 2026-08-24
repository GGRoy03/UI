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


static void
UILayoutPlaceMajorAxis(const ui_place_major_axis_task *Tasks, uint32_t Count, float *OutPosition)
{
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
                StartOffset = ParentSpace - ChildrenSize;
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
            // placing branch. We need a 2D cursor in that case... Well, this case would be the content overflows, but
            // the content isn't allowed to. It would depend on some kind of policy.
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


typedef struct
{
    float       ParentSize;
    ui_padding  Padding;
    UIAlignment Alignment;
    float      *ChildSizes;
    uint32_t    ChildCount;
} ui_place_minor_axis_task;


static void
UILayoutPlaceMinorAxis(const ui_place_minor_axis_task *Tasks, uint32_t Count, float *OutPosition)
{
    uint32_t TotalChildCount = 0;

    for(uint32_t TaskIdx = 0; TaskIdx < Count; ++TaskIdx)
    {
        const ui_place_minor_axis_task *Task = Tasks + TaskIdx;

        for(uint32_t ChildIdx = 0; ChildIdx < Task->ChildCount; ++ChildIdx)
        {
            //
            // Compute the available content space for the parent.
            //
            // TODO:
            // x) Does not handle padding
            // x) Does not handle border-width
            //

            float ChildSize   = Task->ChildSizes[ChildIdx];
            float ParentSpace = Task->ParentSize - ChildSize;

            //
            // Compute the child's relative offset position.
            //
            // NOTE:
            // Do we always want to compute the relative position or do we accept the parent's absolute position
            // in parameter? Both are somewhat equivalent as I understand it.
            //
            // TODO:
            // The position result does not account for the parent's padding.
            //
            
            float Offset = 0.0f;
            switch(Task->Alignment)
            {

            case UIAlignment_Start:        break;
            case UIAlignment_SpaceAround:  break;
            case UIAlignment_SpaceBetween: break;
            case UIAlignment_SpaceEvenly:  break;

            case UIAlignment_Center: Offset = (ParentSpace * 0.5f); break;
            case UIAlignment_End:    Offset = (ParentSpace - ChildSize);

            }

            //
            // Write the output.
            //
            // NOTE:
            // Still unclear if we really want relative positions.
            //

            OutPosition[TotalChildCount + ChildIdx] = Offset;
            TotalChildCount                        += 1;
        }
    }
}


// ==========================================================
// [SECTION] : LAYOUT COMMAND CONTEXT
// ==========================================================


typedef enum
{
    UIAxis_X = 0,
    UIAxis_Y = 1,
} UIAxis;


typedef enum
{
    UILayoutTaskBucket_FixedX      = 0,
    UILayoutTaskBucket_FixedY      = 1,
    UILayoutTaskBucket_FitX        = 2,
    UILayoutTaskBucket_FitY        = 3,
    UILayoutTaskBucket_DistributeY = 4,
    UILayoutTaskBucket_PlaceMajorY = 5,
    UILayoutTaskBucket_PlaceMinorX = 6,

    UILayoutTaskBucket_Count,
} UILayoutTaskBucket;


typedef struct
{
    ui_linear_allocator Pool;
} ui_layout_context_bucket;


typedef struct
{
    ui_layout_context_bucket Buckets[UILayoutTaskBucket_Count];
} ui_layout_context;


static bool
UIIsValidLayoutTaskContext(const ui_layout_context *Context)
{
    bool Result = (Context != 0);
    return Result;
}


static ui_fixed_sizing_task *
UIAcquireFixedLayoutTask(UIAxis Axis, ui_layout_context *Context)
{
    ui_fixed_sizing_task *Result = 0;

    if(UIIsValidLayoutTaskContext(Context))
    {
        ui_linear_allocator *BucketAllocator = 0;

        if(Axis == UIAxis_X)
        {
            BucketAllocator = &Context->Buckets[UILayoutTaskBucket_FixedX].Pool;
        }
        else if(Axis == UIAxis_Y)
        {
            BucketAllocator = &Context->Buckets[UILayoutTaskBucket_FixedY].Pool;
        }

        if(UIIsValidLinearAllocator(BucketAllocator))
        {
            Result = UIAllocateLinearStruct(ui_fixed_sizing_task, BucketAllocator);
        }
    }

    return Result;
}


typedef struct
{
    uint32_t BucketByteSize;
} ui_layout_context_params;


static uint64_t
UILayoutContextMemorySize(ui_layout_context_params Params)
{
    ui_memory_size_counter Counter = {};
    {
        UIMemorySizeCountStruct(ui_layout_context, &Counter);

        for(uint32_t BucketIdx = 0; BucketIdx < UILayoutTaskBucket_Count; ++BucketIdx)
        {
            UIMemorySizeCountBuffer(uint8_t, Params.BucketByteSize, &Counter);
        }
    }

    uint64_t Result = UIMemorySizeCounterWorstCase(UI_ALIGN_OF(ui_layout_context), Counter);
    return Result;
}


static ui_layout_context *
UILayoutContextMemoryInit(void *Memory, uint64_t Size, ui_layout_context_params Params)
{
    ui_layout_context *Result = 0;

    ui_linear_allocator Allocator = UILinearAllocator(Memory, Size);
    if(UIIsValidLinearAllocator(&Allocator))
    {
        ui_layout_context *Context = UIAllocateLinearStruct(ui_layout_context, &Allocator);

        ui_linear_allocator Allocators[UILayoutTaskBucket_Count] = {};
        for(uint32_t BucketIdx = 0; BucketIdx < UILayoutTaskBucket_Count; ++BucketIdx)
        {
            uint8_t *Buffer = (uint8_t *)UIAllocateLinearAligned(Params.BucketByteSize, UI_ALIGN_OF(uint8_t), &Allocator);
            if(Buffer)
            {
                Allocators[BucketIdx] = UILinearAllocator(Buffer, Params.BucketByteSize); 
            }
        }

        if(Context)
        {
            for(uint32_t BucketIdx = 0; BucketIdx < UILayoutTaskBucket_Count; ++BucketIdx)
            {
                Context->Buckets[BucketIdx].Pool = Allocators[BucketIdx];
            }
        }

        Result = Context;
    }

    return Result;
}


// ==========================================================
// [SECTION] : WINDOW
// ==========================================================


typedef struct
{
    ui_linear_allocator FrameAllocator;
    ui_layout_context  *LayoutContext;
} ui_window;


typedef struct
{
    uint64_t                 FrameMemorySize;
    ui_layout_context_params LayoutContext;
} ui_window_params;


static uint64_t
UIWindowMemorySize(ui_window_params Params)
{
    ui_memory_size_counter Counter = {};
    {
        uint64_t LayoutContextSize = UILayoutContextMemorySize(Params.LayoutContext);
        {
            UIMemorySizeCountBuffer(uint8_t, LayoutContextSize, &Counter);
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
        uint64_t LayoutContextSize = UILayoutContextMemorySize(Params.LayoutContext);

        uint8_t   *LayoutContextMemory = UIAllocateLinearArray(LayoutContextSize, uint8_t, &Allocator);
        uint8_t   *FrameMemory         = UIAllocateLinearArray(Params.FrameMemorySize, uint8_t, &Allocator);
        ui_window *Window              = UIAllocateLinearStruct(ui_window, &Allocator);

        if(Window)
        {
            Window->FrameAllocator = UILinearAllocator(FrameMemory, Params.FrameMemorySize);
            Window->LayoutContext  = UILayoutContextMemoryInit(LayoutContextMemory, LayoutContextSize, Params.LayoutContext);
        }

        Result = Window;
    }

    return Result;
}


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
            Layout->Padding     = (ui_padding){};
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
            Result->SizingX = (ui_sizing){};
            Result->SizingY = (ui_sizing){};
            Result->Min     = (ui_size){};
            Result->Max     = (ui_size){};
        }
    }

    return Result;
}


static void
UILeaveVerticalLayout(const ui_vertical_layout *Layout, ui_window *Window)
{
    //
    // TODO:
    // 1) input validation
    // 2) reduce code dup
    //

    if(Layout && Window)
    {
        //
        // Container
        //

        if(Layout->SizingX.Type == UISizing_Fixed)
        {
            ui_fixed_sizing_task *Task = UIAcquireFixedLayoutTask(UIAxis_X, Window->LayoutContext);
            if(Task)
            {
                Task->Size    = Layout->SizingX.Fixed;
                Task->MinSize = Layout->MinSize.X;
                Task->MaxSize = Layout->MaxSize.X;
            }
        }

        if(Layout->SizingY.Type == UISizing_Fixed)
        {
            ui_fixed_sizing_task *Task = UIAcquireFixedLayoutTask(UIAxis_Y, Window->LayoutContext);
            if(Task)
            {
                Task->Size    = Layout->SizingY.Fixed;
                Task->MinSize = Layout->MinSize.Y;
                Task->MaxSize = Layout->MaxSize.Y;
            }
        }

        //
        // Content
        //

        for(ui_vertical_content_node *Node = Layout->FstContent; Node != 0; Node = Node->Next)
        {
            ui_vertical_content *Content = &Node->Content;

            if(Content->SizingX.Type == UISizing_Fixed)
            {
                ui_fixed_sizing_task *Task = UIAcquireFixedLayoutTask(UIAxis_X, Window->LayoutContext);
                if(Task)
                {
                    Task->Size    = Content->SizingX.Fixed;
                    Task->MinSize = Content->Min.X;
                    Task->MaxSize = Content->Max.X;
                }
            }
    
            if(Content->SizingY.Type == UISizing_Fixed)
            {
                ui_fixed_sizing_task *Task = UIAcquireFixedLayoutTask(UIAxis_Y, Window->LayoutContext);
                if(Task)
                {
                    Task->Size    = Content->SizingY.Fixed;
                    Task->MinSize = Content->Min.Y;
                    Task->MaxSize = Content->Max.Y;
                }
            }
        }

        //
        // Place
        //
    }
}


# endif // UI_HEADER
