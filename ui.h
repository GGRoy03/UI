# ifndef UI_HEADER
#   define UI_HEADER


// ==========================================================
// [SECTION] : INCLUDES
// ==========================================================


# include <stdint.h>
# include <assert.h>
# include <stdbool.h>
# include <stdlib.h>


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


# if defined(UI_COMPILER_CLANG)
#  define UI_ALIGN_OF(X) (__alignof__(X))
# elif defined(UI_COMPILER_MSVC)
#  define UI_ALIGN_OF(X) (alignof(X)) 
# else
#  error "UNIMPLEMENTED ALIGNOF MACRO"
# endif


// ==========================================================
// [SECTION] : COMMON MACROS | HELPERS
// ==========================================================


# define UI_KIB(X) ((X * 1024))
# define UI_MIB(X) ((UI_KIB(X) * 1024))
# define UI_GIB(X) ((UI_MIB(X) * 1024))

# define UI_MIN(A, B)              ((A <= B) ? A : B)
# define UI_MAX(A, B)              ((A >= B) ? A : B)
# define UI_CLAMP(Value, Min, Max) (UI_MIN(UI_MAX(Value, Min), Max))

# define UI_IS_VALID_ENUM(Value, MaxExclusive) ((Value >= 0) && (Value < MaxExclusive))
# define UI_ARRAY_COUNT(Array)                 ((sizeof(Array) / sizeof(Array[0])))

# define UI_BIT_MASK(BitCount)          ((1ull << BitCount) - 1ull)
# define UI_CHECK_BIT_MASK(Value, Mask) ((Value & Mask))
# define UI_IS_POWER_OF_TWO(X)          ((X) && ((X & (X - 1)) == 0))


// ==========================================================
// [SECTION] LAYOUT GRAPH
// ==========================================================
// [DESCRIPTION]
// ==========================================================


typedef struct
{
    uint32_t ID;
} ui_graph_node_handle;


typedef struct ui_layout_graph ui_layout_graph;


// ==========================================================
// [SECTION] LAYOUT RESOURCES
// ==========================================================
// [DESCRIPTION]
// ==========================================================


typedef struct
{
    uint32_t Start;
    uint32_t Count;
} ui_layout_resource_handle;


typedef struct
{
    ui_layout_resource_handle Value;
} ui_layout_read_handle;


typedef struct
{
    ui_layout_resource_handle Value;
} ui_layout_write_handle;


// ==========================================================
// [SECTION] LAYOUT COMMANDS
// ==========================================================
// [DESCRIPTION]
// ==========================================================


typedef enum
{
    UIJustify_Start        = 0,
    UIJustify_Center       = 1,
    UIJustify_End          = 2,
    UIJustify_SpaceBetween = 3,
    UIJustify_SpaceAround  = 4,
    UIJustify_SpaceEvenly  = 5,
} UIJustify;


typedef enum
{
    UIAlignment_Start  = 0,
    UIAlignment_Center = 1,
    UIAlignment_End    = 2,
} UIAlignment;


typedef struct
{
    float                   Spacing;
    float                   Padding;
    float                   BorderWidth;
    float                   BorderInset;
    UIJustify               Justify;
    ui_layout_read_handle   ParentAxisBox;
    ui_layout_read_handle  *ChildrenAxisBox;
    ui_layout_write_handle *ChildrenPosition;
} ui_place_major_axis_command;


typedef struct
{
    float                  Padding;
    float                  BorderWidth;
    float                  BorderInset;
    UIAlignment            Alignment;
    ui_layout_read_handle  ParentAxisBox;
    ui_layout_read_handle  ChildAxisBox;
    ui_layout_write_handle ChildPosition;
} ui_place_minor_axis_command;


#define UI_LAYOUT_COMMAND_TYPES(X) \
    X(ui_place_major_axis_command, UILayoutCommand_PlaceMajorAxis) \
    X(ui_place_minor_axis_command, UILayoutCommand_PlaceMinorAxis)


typedef enum
{
#define X(Type, Tag) Tag,
    UI_LAYOUT_COMMAND_TYPES(X)
#undef X
} UILayoutCommand;


# ifdef UI_IMPLEMENTATION
# endif



// ==========================================================
// [SECTION] : ...
// ==========================================================


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
UIIsValidLinearAllocator(const ui_linear_allocator *Allocator)
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
    const void *Data;
    uint32_t    Count;
    bool        IsValid;
} ui_linear_allocator_view;


static ui_linear_allocator_view
UILinearAllocatorBufferView(uint32_t KeySize, const ui_linear_allocator *Allocator)
{
    ui_linear_allocator_view Result = {};

    if(UIIsValidLinearAllocator(Allocator))
    {
        if(KeySize != 0)
        {
            assert(Allocator->At % KeySize == 0);

            Result.Data    = Allocator->Buffer;
            Result.Count   = Allocator->At / KeySize;
            Result.IsValid = true;
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
    float VisualSize;
    float ContentSize;
} ui_axis_box;


static float
UISizeFixedAxis(float FixedValue, float MinConstraint, float MaxConstraint)
{
    float Result = 0.0f;

    if(MinConstraint > MaxConstraint)
    {
        Result = UI_CLAMP(FixedValue, MinConstraint, MinConstraint);
    }
    else
    {
        Result = UI_CLAMP(FixedValue, MinConstraint, MaxConstraint);
    }

    return Result;
}


static ui_axis_box
UISizeAxisBox(float SelfSize, float PaddingA, float PaddingB, float BorderWidth, float BorderInset)
{
    //
    // The BorderInset value specifies how much of the border is inside the element visually.
    // For example, a border inset of 0.0f means the border is full outside the element and a border
    // inset of 1.0f means the border is fully inside the element.
    //

    float ClampedBorderInset = UI_CLAMP(BorderInset, 0.0f, 1.0f);
    float BorderConsumed     = BorderWidth * BorderInset;
    float BorderExtended     = BorderWidth * (1.0f - BorderInset);

    //
    // Visual Size:
    // The size that will most likely be rendered. The placer often uses this size when placing elements.
    // Though it should be possible to override that behavior and use the self size instead which may lead to
    // overlapping visual elements.
    //
    // Content Size:
    // The size that is allowed by the parent for children to be placed inside of it.
    //

    ui_axis_box Result =
    {
        .VisualSize  = SelfSize + (BorderExtended * 2.0f),
        .ContentSize = SelfSize - ((BorderConsumed * 2.0f) + (PaddingA + PaddingB)),
    };

    return Result;
}


static void
UIPlaceMajorAxis(float ContentSize, float *ChildSize, uint32_t ChildCount, float Spacing, float Padding, float BorderWidth, float BorderInset, UIJustify Justify, float *Result)
{
    //
    // TODO:
    // This is probably missing a bunch of safety clamps/checks
    // This has not been tested.
    //

    if(ChildCount > 0)
    {
        float ChildrenSize = 0.0f;
        for(uint32_t ChildIdx = 0; ChildIdx < ChildCount; ++ChildIdx)
        {
            ChildrenSize += ChildSize[ChildIdx];
        }

        float RemainingContent = ContentSize - ChildrenSize;
        if(RemainingContent > 0.0f)
        {
            float SpacingConsumed = Spacing * (ChildCount - 1);

            //
            // CursorStart represents the initial offset applied to the relative position.
            // CursorStep reprensets the step we need to take after laying out each element,
            // this step does not include the child's own size.
            //

            float CursorStart = 0.0f;
            float CursorStep  = 0.0f;
            switch(Justify)
            {

            case UIJustify_Start:
            {
                CursorStart = 0.0f;
                CursorStep  = Spacing;
            } break;

            case UIJustify_Center:
            {
                CursorStart = (RemainingContent - SpacingConsumed) * 0.5f;
                CursorStep  = Spacing;
            } break;

            case UIJustify_End:
            {
                CursorStart = (RemainingContent - SpacingConsumed);
                CursorStep  = Spacing;
            } break;

            case UIJustify_SpaceBetween:
            {
                CursorStart = 0.0f;
                CursorStep  = ChildCount > 1 ? (RemainingContent / (ChildCount - 1)) : 0.0f;
            } break;

            case UIJustify_SpaceAround:
            {
                CursorStart = (RemainingContent / (ChildCount * 2));
                CursorStep  = CursorStart * 2.0f;
            } break;

            case UIJustify_SpaceEvenly:
            {
                CursorStart = (RemainingContent / (ChildCount + 1));
                CursorStep  =  CursorStart;
            } break;

            }

            //
            // Since this functions returns positions relative to the parent's top-left position (which is considered to be (0, 0),
            // we need to apply an offset to each position: Padding (From where we start laying out element: A or B when reversed)
            // and the border-width along with the border inset since the border may or may not affect the position.
            //

            float ClampedBorderInset = UI_CLAMP(BorderInset, 0.0f, 1.0f);
            float CursorOffset       = Padding + (BorderWidth * ClampedBorderInset);
            float CursorPosition     = CursorStart;
            for(uint32_t ChildIdx = 0; ChildIdx < ChildCount; ++ChildIdx)
            {
                Result[ChildIdx] = CursorPosition + CursorOffset;
                CursorPosition  += ChildSize[ChildIdx] + CursorStep;
            }
        }
    }
}


static float
UIPlaceMinorAxis(float ContentSize, float ChildSize, float Padding, float BorderWidth, float BorderInset, UIAlignment Alignment)
{
    float Result = 0.0f;

    float RemainingContent = ContentSize - ChildSize;
    if(RemainingContent > 0.0f)
    {
        switch(Alignment)
        {

        case UIAlignment_Start:  Result = 0.0f;                    break;
        case UIAlignment_Center: Result = RemainingContent * 0.5f; break;
        case UIAlignment_End:    Result = RemainingContent;        break;

        }
    }
    else
    {
        //
        // TODO:
        // Uhm.. Unsure how to handle the overflow case.
        //

        assert(false);
    }

    return Result;
}

// ==========================================================
// [SECTION] LAYOUT COMMANDS
// ==========================================================

//
// This would be the "safe" wrappers? And we have the unsafe function doing the core work?
// I mean.. if I can get it to compile at all. Yeah, it wasn't compiled with C11.
// This is very very experimental.
//

#define UIAcquireLayoutCommand(Type, Allocator) ((Type *)_Generic((Type *)0, \
        ui_place_major_axis_command *: UIAcquireCommandMemoryFromType(sizeof(Type), UI_ALIGN_OF(Type), Allocator) \
        ui_place_minor_axis_command *: UIAcquireCommandMemoryFromType(sizeof(Type), UI_ALIGN_OF(Type), Allocator) \
    ))

#define UIAcquireLayoutCommandData(Type, Allocator) 0


typedef struct
{
    ui_linear_allocator FramePool;
} ui_layout_command_allocator;


static bool
UIIsValidLayoutCommandAllocator(const ui_layout_command_allocator *Allocator)
{
    bool Result = Allocator && UIIsValidLinearAllocator(&Allocator->FramePool);
    return Result;
}


static void *
UIAcquireUntypedCommandMemory(size_t Size, size_t Align, ui_layout_command_allocator *Allocator)
{
    void *Result = 0;

    if(UIIsValidLayoutCommandAllocator(Allocator))
    {
        assert(Size);
        assert(Align);

        Result = UIAllocateLinearAligned(Size, Align, &Allocator->FramePool);
    }

    return Result;
}


// ==========================================================
// [SECTION] LAYOUT GRAPH
// ==========================================================

//
// EXPERIMENTAL
//




typedef struct
{
    float Value;
} ui_float_layout_resource;


typedef struct
{
    ui_axis_box Value;
} ui_axis_box_layout_resource;


typedef struct
{
    union
    {
        ui_float_layout_resource     Float;
        ui_axis_box_layout_resource  AxisBox;
    };
} ui_layout_resource;



typedef struct ui_layout_resource_node ui_layout_resource_node;
struct ui_layout_resource_node
{
    ui_layout_resource_node  *Next;
    uint32_t                  ResourceID;
    uint32_t                  GraphNodeIndex;
};


typedef struct
{
    ui_layout_resource_node *First;
    ui_layout_resource_node *Last;
    uint32_t                 Count;
} ui_layout_resource_list;


//
// EXPERIMENTAL
//

typedef struct
{
    uint32_t TargetNodeIndex;
    uint32_t NextEdgeIndex;
} ui_graph_edge;


typedef struct
{
    uint32_t FirstEdgeIndex;
    uint32_t DependencyCount;

    UILayoutCommand CommandType;
    union
    {
        ui_place_major_axis_command *MajorAxis;
        void                        *Pointer;
    } Command;
} ui_graph_node;


struct ui_layout_graph
{
    ui_graph_node *Nodes;
    uint32_t       NodeCount;
    ui_graph_edge *Edges;
    uint32_t       EdgeCount;

    //
    // Experimental
    //

    ui_linear_allocator      FrameAllocator;
    ui_layout_resource_list  ReadList;
    ui_layout_resource_list  WriteList;
    ui_layout_resource       Resources[128];
    uint32_t                 ResourceCount;
};


static bool
UIIsValidLayoutGraph(const ui_layout_graph *Graph)
{
    bool Result = Graph && Graph->Nodes && Graph->Edges;
    return Result;
}


static ui_graph_node_handle
UIGraphNodeHandleFromIndex(uint32_t Index)
{
    assert(Index != 0);

    ui_graph_node_handle Result = {.ID = Index};
    return Result;
}


static ui_graph_node *
UIGetGraphNodeSentinel(const ui_layout_graph *Graph)
{
    assert(UIIsValidLayoutGraph(Graph));

    ui_graph_node *Result = Graph->Nodes;
    return Result;
}


static ui_graph_node *
UIGetGraphNode(uint32_t Index, const ui_layout_graph *Graph)
{
    assert(UIIsValidLayoutGraph(Graph));

    ui_graph_node *Result = 0;

    if(Index < Graph->NodeCount)
    {
        Result = Graph->Nodes + Index;
    }

    return Result;
}


static ui_graph_edge *
UIGetGraphEdgeSentinel(const ui_layout_graph *Graph)
{
    assert(UIIsValidLayoutGraph(Graph));

    ui_graph_edge *Result = Graph->Edges;
    return Result;
}


static ui_graph_edge *
UIGetGraphEdge(uint32_t Index, const ui_layout_graph *Graph)
{
    assert(UIIsValidLayoutGraph(Graph));

    ui_graph_edge *Result = 0;

    if(Index < Graph->EdgeCount)
    {
        Result = Graph->Edges + Index;
    }

    return Result;
}


static void
UIFreeGraphEdge(uint32_t EdgeIndex, ui_layout_graph *Graph)
{
    assert(UIIsValidLayoutGraph(Graph));

    ui_graph_edge *Sentinel = UIGetGraphEdgeSentinel(Graph);
    assert(Sentinel);

    ui_graph_edge *Edge = UIGetGraphEdge(EdgeIndex, Graph);
    if(Edge)
    {
        assert(Edge != Sentinel);

        Edge->TargetNodeIndex = 0;
        Edge->NextEdgeIndex   = Sentinel->NextEdgeIndex;

        Sentinel->NextEdgeIndex = EdgeIndex;
    }
}


static uint32_t
UIPopFreeGraphEdge(ui_layout_graph *Graph)
{
    assert(UIIsValidLayoutGraph(Graph));

    ui_graph_edge *Sentinel = UIGetGraphEdgeSentinel(Graph);
    assert(Sentinel);

    uint32_t       EdgeIndex = Sentinel->NextEdgeIndex;
    ui_graph_edge *Result    = UIGetGraphEdge(EdgeIndex, Graph);
    if(Result)
    {
        Sentinel->NextEdgeIndex = Result->NextEdgeIndex;
    }

    return EdgeIndex;
}


static uint32_t
UIPopFreeGraphNode(ui_layout_graph *Graph)
{
    assert(UIIsValidLayoutGraph(Graph));

    uint32_t Result = 0;

    ui_graph_node *Sentinel = UIGetGraphNodeSentinel(Graph);
    assert(Sentinel);

    uint32_t       EdgeIndex = Sentinel->FirstEdgeIndex;
    ui_graph_edge *Edge      = UIGetGraphEdge(EdgeIndex, Graph);
    if(Edge)
    {
        uint32_t       NodeIndex = Edge->TargetNodeIndex;
        ui_graph_node *Node      = UIGetGraphNode(NodeIndex, Graph);
        if(Node)
        {
            assert(Node != Sentinel);

            UIFreeGraphEdge(EdgeIndex, Graph);

            Result                   = NodeIndex;
            Sentinel->FirstEdgeIndex = Node->FirstEdgeIndex;
        }
    }

    return Result;
}


static ui_graph_node_handle
UIAddLayoutGraphNode(ui_layout_graph *Graph)
{
    ui_graph_node_handle Result = {};

    if(UIIsValidLayoutGraph(Graph))
    {
        uint32_t       NodeIndex = UIPopFreeGraphNode(Graph);
        ui_graph_node *Node      = UIGetGraphNode(NodeIndex, Graph);

        if(Node)
        {
            Node->FirstEdgeIndex   = 0;
            Node->DependencyCount      = 0;
        }

        Result = UIGraphNodeHandleFromIndex(NodeIndex);
    }

    return Result;
}


static void
UIAddGraphEdge(uint32_t Source, uint32_t Target, ui_layout_graph *Graph)
{
    if(UIIsValidLayoutGraph(Graph))
    {
        ui_graph_node *SourceNode = UIGetGraphNode(Source, Graph);
        ui_graph_node *TargetNode = UIGetGraphNode(Target, Graph);

        if(SourceNode && TargetNode)
        {
            //
            // Successor (Source->Target)
            //


            //
            // Predecessor (Target->Source)
            //

            uint32_t       EdgeIndex = UIPopFreeGraphEdge(Graph);
            ui_graph_edge *Edge      = UIGetGraphEdge(EdgeIndex, Graph);

            if(Edge)
            {
                Edge->TargetNodeIndex = Target;
                Edge->NextEdgeIndex   = SourceNode->FirstEdgeIndex;

                SourceNode->FirstEdgeIndex = EdgeIndex;
                TargetNode->DependencyCount    += 1;
            }
        }
    }
}


static void
UIAddGraphEdgeFromHandle(ui_graph_node_handle Source, ui_graph_node_handle Target, ui_layout_graph *Graph)
{
    UIAddGraphEdge(Source.ID, Target.ID, Graph);
}


typedef struct
{
    uint32_t NodeCount;
    uint32_t EdgeCount;
} ui_layout_graph_params;


static uint64_t
UILayoutGraphMemorySize(ui_layout_graph_params Params)
{
    ui_memory_size_counter Counter = {};
    {
        UIMemorySizeCountBuffer(ui_graph_node, Params.NodeCount, &Counter);
        UIMemorySizeCountBuffer(ui_graph_edge, Params.EdgeCount, &Counter);

        UIMemorySizeCountStruct(ui_layout_graph, &Counter);
    }

    uint64_t Result = UIMemorySizeCounterWorstCase(UI_ALIGN_OF(ui_layout_graph), Counter);
    return Result;
}


static ui_layout_graph *
UILayoutGraphMemoryInit(void *Memory, uint64_t Size, ui_layout_graph_params Params)
{
    ui_layout_graph *Result = 0;

    ui_linear_allocator Allocator = UILinearAllocator(Memory, Size);
    if(UIIsValidLinearAllocator(&Allocator))
    {
        ui_graph_node *Nodes = UIAllocateLinearArray(Params.NodeCount, ui_graph_node, &Allocator);
        ui_graph_edge *Edges = UIAllocateLinearArray(Params.EdgeCount, ui_graph_edge, &Allocator);

        ui_layout_graph *Graph = UIAllocateLinearStruct(ui_layout_graph, &Allocator);
        if(Graph)
        {
            Graph->Nodes     = Nodes;
            Graph->NodeCount = Params.NodeCount;
            Graph->Edges     = Edges;
            Graph->EdgeCount = Params.EdgeCount;

            //
            // Populate the edge free-list.
            //

            for(uint32_t EdgeIdx = 0; EdgeIdx < Params.EdgeCount; ++EdgeIdx)
            {
                ui_graph_edge *Edge = UIGetGraphEdge(EdgeIdx, Graph);
                assert(Edge);

                if(EdgeIdx != (Params.EdgeCount - 1))
                {
                    Edge->TargetNodeIndex = 0;
                    Edge->NextEdgeIndex   = EdgeIdx + 1;
                }
                else
                {
                    Edge->TargetNodeIndex = 0;
                    Edge->NextEdgeIndex   = 0;
                }
            }

            //
            // Populate the node free-list.
            //

            assert(Params.EdgeCount >= Params.EdgeCount);

            for (uint32_t NodeIdx = 0; NodeIdx < Params.NodeCount; ++NodeIdx)
            {
                ui_graph_node *Node = UIGetGraphNode(NodeIdx, Graph);
                Node->FirstEdgeIndex = 0;
                Node->DependencyCount         = 0;
            }

            for(uint32_t NodeIdx = 0; NodeIdx < Params.NodeCount; ++NodeIdx)
            {
                if((NodeIdx + 1) < Params.NodeCount)
                {
                    UIAddGraphEdge(NodeIdx, NodeIdx + 1, Graph);
                }
                else
                {
                    //
                    // TODO:
                    // I think this might be a waste.
                    //

                    UIAddGraphEdge(NodeIdx, 0, Graph);
                }
            }
        }

        Result = Graph;
    }

    return Result;
}


typedef struct
{
    uint32_t Indices[32];
    uint32_t HeadIndex;
    uint32_t TailIndex;
} ui_graph_iterator_worker;


typedef struct
{
    const ui_layout_graph *Graph;
    uint32_t                *NodeDependencyCount;
    uint32_t                 NodeCount;
    ui_graph_iterator_worker Worker;
} ui_graph_iterator;


static uint32_t
UIGraphIteratorNextIndex(uint32_t CurrentIndex, const ui_graph_iterator_worker *Worker)
{
    uint32_t Result = (CurrentIndex + 1) % UI_ARRAY_COUNT(Worker->Indices);
    return Result;
}


static uint32_t
UIGraphIteratorPrevIndex(uint32_t CurrentIndex, const ui_graph_iterator_worker *Worker)
{
    uint32_t Result = (CurrentIndex - 1) % UI_ARRAY_COUNT(Worker->Indices);
    return Result;
}


static void
UIPushNodeInIterator(uint32_t Index, ui_graph_iterator_worker *Worker)
{
    if(Worker)
    {
        uint32_t NextHeadIndex = UIGraphIteratorNextIndex(Worker->HeadIndex, Worker);
        if(NextHeadIndex != Worker->TailIndex)
        {
            Worker->Indices[Worker->HeadIndex] = Index; 
            Worker->HeadIndex                  = NextHeadIndex;
        }
    }
}


static uint32_t
UIPopNextNodeFromIterator(ui_graph_iterator_worker *Worker)
{
    uint32_t Result = 0;

    if(Worker)
    {
        if(Worker->HeadIndex != Worker->TailIndex)
        {
            uint32_t PrevIndex = UIGraphIteratorPrevIndex(Worker->HeadIndex, Worker);

            Result            = Worker->Indices[PrevIndex];
            Worker->HeadIndex = PrevIndex;
        }
    }

    return Result;
}


static uint32_t
UIPopLastNodeFromIterator(ui_graph_iterator_worker *Worker)
{
    uint32_t Result = {};

    if(Worker)
    {
        if(Worker->TailIndex != Worker->HeadIndex)
        {
            Result            = Worker->Indices[Worker->TailIndex];
            Worker->TailIndex = UIGraphIteratorNextIndex(Worker->TailIndex, Worker);
        }
    }

    return Result;
}


static ui_graph_iterator
UIBeginGraphIterator(const ui_layout_graph *Graph, ui_linear_allocator *Allocator)
{
    ui_graph_iterator Result = {};

    if(UIIsValidLayoutGraph(Graph) && UIIsValidLinearAllocator(Allocator))
    {
        ui_graph_iterator Iterator =
        {
            .Graph      = Graph,
            .NodeDependencyCount = UIAllocateLinearArray(Graph->NodeCount, uint32_t, Allocator),
            .NodeCount  = Graph->NodeCount,
        };

        for(uint32_t NodeIdx = 1; NodeIdx < Graph->NodeCount; ++NodeIdx)
        {
            ui_graph_node *Node = UIGetGraphNode(NodeIdx, Graph);
            assert(Node);

            if(Node->DependencyCount == 0)
            {
                UIPushNodeInIterator(NodeIdx, &Iterator.Worker);
            }
            else
            {
                Iterator.NodeDependencyCount[NodeIdx] = Node->DependencyCount;
            }
        }

        Result = Iterator;
    }

    return Result;
}


static bool
UIGraphIteratorNext(ui_graph_iterator *Iterator, ui_graph_node_handle *Result)
{
    bool CanContinue = false;

    const ui_layout_graph  *LayoutGraph = Iterator->Graph;
    ui_graph_iterator_worker *Worker      = &Iterator->Worker;

    uint32_t             NodeIndex = UIPopNextNodeFromIterator(Worker);
    const ui_graph_node *Node      = UIGetGraphNode(NodeIndex, LayoutGraph);
    if (Node && Node != UIGetGraphNodeSentinel(LayoutGraph))
    {
        uint32_t       EdgeIndex = Node->FirstEdgeIndex;
        ui_graph_edge *Edge      = UIGetGraphEdge(EdgeIndex, LayoutGraph);
        while(Edge && Edge != UIGetGraphEdgeSentinel(LayoutGraph))
        {
            uint32_t TargetIndex   = Edge->TargetNodeIndex;
            uint32_t CurrentDependencyCount = Iterator->NodeDependencyCount[TargetIndex];
            assert(CurrentDependencyCount > 0);

            CurrentDependencyCount -= 1;
            if (CurrentDependencyCount == 0)
            {
                UIPushNodeInIterator(TargetIndex, Worker);
            }

            Iterator->NodeDependencyCount[TargetIndex] = CurrentDependencyCount;

            EdgeIndex = Edge->NextEdgeIndex;
            Edge      = UIGetGraphEdge(EdgeIndex, LayoutGraph);
        }

        *Result     = UIGraphNodeHandleFromIndex(NodeIndex);
        CanContinue = true;
    }

    return CanContinue;
}


//
// Experimenting with resources.
//



static ui_layout_resource_handle
UIAcquireFloatLayoutResource(uint32_t Count, ui_layout_graph *Graph)
{
    ui_layout_resource_handle Result = {};

    if(UIIsValidLayoutGraph(Graph))
    {
    }

    return Result;
}


//
// This actually add the edges to the graph while parsing the read/write list.
//


static void
UIBuildLayoutGraph(ui_layout_graph *Graph)
{
    if(UIIsValidLayoutGraph(Graph))
    {
        //
        // Intermediate table
        //
        // TODO:
        // How big should this table be?
        //

        uint32_t *ResourceIDToProducerIndex = UIAllocateLinearArray(0, uint32_t, &Graph->FrameAllocator);

        //
        // Write-List
        //

        ui_layout_resource_list WriteList = Graph->WriteList;
        for(ui_layout_resource_node *Node = WriteList.First; Node != 0; Node = Node->Next)
        {
            ResourceIDToProducerIndex[Node->ResourceID] = Node->GraphNodeIndex;
        }

        //
        // Read-List
        //

        ui_layout_resource_list ReadList = Graph->ReadList;
        for(ui_layout_resource_node *Node = ReadList.First; Node != 0; Node = Node->Next)
        {
            uint32_t ProducerIndex = ResourceIDToProducerIndex[Node->ResourceID];
            if(ProducerIndex != 0)
            {
                uint32_t ConsumerIndex = Node->GraphNodeIndex;
                UIAddGraphEdge(ProducerIndex, ConsumerIndex, Graph);
            }
        }
    }
}


//
//
//


static void
UIAppendLayoutResourceNode(uint32_t NodeIndex, ui_layout_resource_handle Handle, ui_layout_resource_list *List, ui_linear_allocator *Allocator)
{
    assert(List);
    assert(UIIsValidLinearAllocator(Allocator));

    ui_layout_resource_node *ResourceNode = UIAllocateLinearStruct(ui_layout_resource_node, Allocator);
    if(ResourceNode)
    {
        if(!List->First)
        {
            List->First = ResourceNode;
            List->Last  = ResourceNode;
        }
        else if(List->Last)
        {
            List->Last->Next = ResourceNode;
            List->Last       = ResourceNode;
        }

        //
        // TODO:
        // Need to compute the resource unique ID.
        //

        ResourceNode->GraphNodeIndex = NodeIndex;
        ResourceNode->ResourceID     = 0;
    }
}


static ui_layout_read_handle
UIBindLayoutRead(ui_layout_resource_handle ResourceHandle, ui_graph_node_handle NodeHandle, ui_layout_graph *Graph)
{
    ui_layout_read_handle Result = {};

    if(UIIsValidLayoutGraph(Graph))
    {
        UIAppendLayoutResourceNode(NodeHandle.ID, ResourceHandle, &Graph->ReadList, &Graph->FrameAllocator);
        Result = (ui_layout_read_handle){.Value = ResourceHandle};
    }

    return Result;
}


static ui_layout_write_handle
UIBindLayoutWrite(ui_layout_resource_handle ResourceHandle, ui_graph_node_handle NodeHandle, ui_layout_graph *Graph)
{
    ui_layout_write_handle Result = {};

    if(UIIsValidLayoutGraph(Graph))
    {
        UIAppendLayoutResourceNode(NodeHandle.ID, ResourceHandle, &Graph->WriteList, &Graph->FrameAllocator);
        Result = (ui_layout_write_handle){.Value = ResourceHandle};
    }

    return Result;
}


static void *
UIBindLayoutCommand(ui_graph_node_handle Handle, UILayoutCommand Type, ui_layout_graph *Graph)
{
    assert(UIIsValidLayoutGraph(Graph));

    void *Result = 0;

    if(UIIsValidLayoutGraph(Graph))
    {
        ui_graph_node *Node = UIGetGraphNode(Handle.ID, Graph);
        if(Node && Node->Command.Pointer == 0)
        {
            Result = UIAcquireCommandMemoryFromType(Type, 0);
            if(Result)
            {
                Node->Command.Pointer = Result;
                Node->CommandType     = Type;
            }
        }
    }

    return Result;
}


static ui_place_major_axis_command *
UIBindPlaceMajorAxisCommand(uint32_t ChildCount, ui_graph_node_handle Handle, ui_layout_graph *Graph)
{
    ui_place_major_axis_command *Result = 0;

    if(UIIsValidLayoutGraph(Graph))
    {
        //
        // This is just stupid... man. Hours on this and still stuck on the same stupid problem.
        // Perhaps just reverse it... Hand the command pointer: UITryBindLayoutCommandToNode(Handle, CommandPtr, Graph)
        // if true:
        //    allocate the rest.
        //
        // That's fine. So just do that. It's not perfect, but we're wasting too much time on this insignificant piece of code.
        //
    }

    return Result;

    ui_place_major_axis_command *Result = (ui_place_major_axis_command *)UIBindLayoutCommand(Handle, UILayoutCommand_PlaceMajorAxis, Graph);
    if(Result)
    {
        //
        // It's just such a stupid problem, yet I've no clue how to fix it.
        // There's some common behavior each time we allocate a command on a node right? Which means this is a graph operation?
        // But the command itself also needs to do stuff to allocate the dynamic part of it. Which means it needs to allocate
        // something on the graph. If bind layout command is internal, we can assume parameters passed to it are correct...
        // and soooo. what's the problem exactly? Do we still want to deal with this X-Macro stuff? This weird table mapping
        // type -> size/align... is it even useful? Well... why not? I am confused.
        //
        //

        Result->ChildrenAxisBox;
        Result->ChildrenPosition;
    }
}


// ==========================================================
// [SECTION] : WINDOW
// ==========================================================


typedef struct
{
    ui_linear_allocator FrameAllocator;
    ui_layout_graph  *LayoutGraph;
} ui_window;


typedef struct
{
    uint64_t                 FrameMemorySize;
    ui_layout_graph_params LayoutGraph;
} ui_window_params;


static uint64_t
UIWindowMemorySize(ui_window_params Params)
{
    ui_memory_size_counter Counter = {};
    {
        Counter.Size += UILayoutGraphMemorySize(Params.LayoutGraph);

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
        uint64_t LayoutGraphSize   = UILayoutGraphMemorySize(Params.LayoutGraph);
        uint8_t *LayoutGraphMemory = UIAllocateLinearArray(LayoutGraphSize, uint8_t, &Allocator);

        uint8_t   *FrameMemory = UIAllocateLinearArray(Params.FrameMemorySize, uint8_t, &Allocator);
        ui_window *Window      = UIAllocateLinearStruct(ui_window, &Allocator);
        if(Window)
        {
            Window->LayoutGraph    = UILayoutGraphMemoryInit(LayoutGraphMemory, LayoutGraphSize, Params.LayoutGraph);
            Window->FrameAllocator = UILinearAllocator(FrameMemory, Params.FrameMemorySize);
        }

        Result = Window;
    }

    return Result;
}


static void
UIExecuteWindow(ui_window *Window)
{
    if(Window)
    {
        //
        // Layout
        //

        ui_layout_graph *LayoutGraph = Window->LayoutGraph;
        if(UIIsValidLayoutGraph(LayoutGraph))
        {
            //
            // Iterate the graph and execute the commands.
            //

            ui_graph_node_handle Handle   = {};
            ui_graph_iterator    Iterator = UIBeginGraphIterator(Window->LayoutGraph, &Window->FrameAllocator);
            while(UIGraphIteratorNext(&Iterator, &Handle))
            {
                //
                // TODO:
                // Use the handle to do _something_
                //
                // This is where we need to route the data. Now, the problem is:
                // We still don't allocate command/haven't specified what a command contains.
                // I'm struggling to figure out at what level that allocator actually sits.
                // It seems like something that's internal only. Which is somehow specific to a window.
                // Well the window has to use it. Okay first of all, relying on the stable ID to index into a table
                // is stupid and has a lot of downsides. A user key is much better from what I can tell. In any case.
                // I guess what I am trying to figure out is how much context the allocator actually needs. We need to map
                // a simple key to a command. I feel like it doesn't need very much context..
                //

            }
        }
    }
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
    float        BorderInset;
    UIAlignment  Align;
    UIJustify    Justify;
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
            Layout->BorderInset = 0.0f;
            Layout->Align       = UIAlignment_Start;
            Layout->Justify     = UIJustify_Start;
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

//
// This looks... fine? I still need a way to route the data to the execution pipeline though.
// So we probably need to define these commands data-wise. Still this question of dealing/routing
// the outputs... Uhm.
//


static void
UILeaveVerticalLayout(const ui_vertical_layout *Layout, ui_window *Window)
{
    ui_linear_allocator *Allocator   = &Window->FrameAllocator;
    ui_layout_graph   *LayoutGraph = Window->LayoutGraph;

    //
    //
    //

    uint32_t              ContentCount      = Layout->ContentCount;
    ui_graph_node_handle *ChildrenBoxModelX = UIAllocateLinearArray(ContentCount, ui_graph_node_handle, Allocator);
    ui_graph_node_handle *ChildrenBoxModelY = UIAllocateLinearArray(ContentCount, ui_graph_node_handle, Allocator);
    ui_graph_node_handle  ParentBoxModelX   = UIAddLayoutGraphNode(LayoutGraph);
    ui_graph_node_handle  ParentBoxModelY   = UIAddLayoutGraphNode(LayoutGraph);
    
    //
    // X-Axis Sizing (Minor Axis)
    //

    if(Layout->SizingX.Type == UISizing_Fixed)
    {
        ui_graph_node_handle ParentSizeX = UIAddLayoutGraphNode(LayoutGraph);
        {
            UIAddGraphEdgeFromHandle(ParentSizeX, ParentBoxModelX, LayoutGraph);
        }

        uint32_t ChildIndex = 0;
        for(ui_vertical_content_node *Node = Layout->FstContent; Node != 0; Node = Node->Next)
        {
            ui_vertical_content *Content = &Node->Content;

            if(Content->SizingX.Type == UISizing_Fixed)
            {
                ui_graph_node_handle ChildSizeX     = UIAddLayoutGraphNode(LayoutGraph);
                ui_graph_node_handle ChildBoxModelX = UIAddLayoutGraphNode(LayoutGraph);
                {
                    UIAddGraphEdgeFromHandle(ChildSizeX, ChildBoxModelX, LayoutGraph);
                }

                ChildrenBoxModelX[ChildIndex] = ChildBoxModelX;
                ChildIndex                   += 1;
            }
        }
    }

    //
    // Y-Axis Sizing (Major Axis)
    //

    if(Layout->SizingY.Type == UISizing_Fixed)
    {
        ui_graph_node_handle ParentSizeY = UIAddLayoutGraphNode(LayoutGraph);
        {
            UIAddGraphEdgeFromHandle(ParentSizeY, ParentBoxModelY, LayoutGraph);
        }

        uint32_t ChildIndex = 0;
        for(ui_vertical_content_node *Node = Layout->FstContent; Node != 0; Node = Node->Next)
        {
            ui_vertical_content *Content = &Node->Content;

            if(Content->SizingY.Type == UISizing_Fixed)
            {
                ui_graph_node_handle ChildSizeY     = UIAddLayoutGraphNode(LayoutGraph);
                ui_graph_node_handle ChildBoxModelY = UIAddLayoutGraphNode(LayoutGraph);
                {
                    UIAddGraphEdgeFromHandle(ChildSizeY, ChildBoxModelY, LayoutGraph);
                }

                ChildrenBoxModelY[ChildIndex] = ChildBoxModelY;
                ChildIndex                   += 1;
            }
        }
    }

    //
    // X-Axis Placing (Minor Axis)
    //

    {
        uint32_t ChildIndex = 0;
        for(ui_vertical_content_node *Node = Layout->FstContent; Node != 0; Node = Node->Next)
        {
            ui_graph_node_handle PlaceChildX = UIAddLayoutGraphNode(LayoutGraph);
            {
                UIAddGraphEdgeFromHandle(ParentBoxModelX              , PlaceChildX, LayoutGraph);
                UIAddGraphEdgeFromHandle(ChildrenBoxModelX[ChildIndex], PlaceChildX, LayoutGraph);
            }
        }
    }

    //
    // Y-Axis Placing (Major Axis)
    //

    ui_graph_node_handle PlaceAxisY = UIAddLayoutGraphNode(LayoutGraph);
    {
        UIAddGraphEdgeFromHandle(ParentBoxModelY, PlaceAxisY, LayoutGraph);

        uint32_t ChildIndex = 0;
        for(ui_vertical_content_node *Node = Layout->FstContent; Node != 0; Node = Node->Next)
        {
            UIAddGraphEdgeFromHandle(ChildrenBoxModelY[ChildIndex], PlaceAxisY, LayoutGraph);
        }
    }
}


static void
UILeaveVerticalLayout2(const ui_vertical_layout *Layout, ui_window *Window)
{
    ui_layout_graph     *LayoutGraph = Window->LayoutGraph;
    ui_linear_allocator *Allocator   = &Window->FrameAllocator;

    ui_layout_resource_handle LayoutSizeX     = UIAcquireFloatLayoutResource(1, LayoutGraph);
    ui_layout_resource_handle LayoutSizeY     = UIAcquireFloatLayoutResource(1, LayoutGraph);
    ui_layout_resource_handle ParentAxisBoxX  = UIAcquireFloatLayoutResource(1, LayoutGraph);
    ui_layout_resource_handle LayoutBoxModelY = UIAcquireFloatLayoutResource(1, LayoutGraph);

    ui_layout_resource_handle ContentSizeX     = UIAcquireFloatLayoutResource(Layout->ContentCount, LayoutGraph);
    ui_layout_resource_handle ContentSizeY     = UIAcquireFloatLayoutResource(Layout->ContentCount, LayoutGraph);
    ui_layout_resource_handle ContentBoxModelY = UIAcquireFloatLayoutResource(Layout->ContentCount, LayoutGraph);
    ui_layout_resource_handle ContentPositionX = UIAcquireFloatLayoutResource(Layout->ContentCount, LayoutGraph);
    ui_layout_resource_handle ContentPositionY = UIAcquireFloatLayoutResource(Layout->ContentCount, LayoutGraph);


    ui_layout_resource_handle *ChildrenAxisBoxX = UIAllocateLinearArray(Layout->ContentCount, ui_layout_resource_handle, Allocator);


    //
    // Place X (Minor Axis)
    //

    ui_graph_node_handle PlaceMinorAxisHandle = UIAddLayoutGraphNode(LayoutGraph);
    {
        uint32_t ChildIndex = 0;
        for(ui_vertical_content_node *Node = Layout->FstContent; Node != 0; Node = Node->Next, ChildIndex += 1)
        {
            ui_layout_resource_handle    Position = UIAcquireFloatLayoutResource(1, LayoutGraph);
            ui_place_minor_axis_command *Command  = UIAcquirePlaceMinorAxisCommand(PlaceMinorAxisHandle, LayoutGraph);
            if(Command)
            {
                Command->Padding       = Layout->Padding.Left;
                Command->BorderWidth   = Layout->BorderWidth;
                Command->BorderInset   = Layout->BorderInset;
                Command->Alignment     = Layout->Align;
                Command->ParentAxisBox = UIBindLayoutRead(ParentAxisBoxX, PlaceMinorAxisHandle, LayoutGraph);
                Command->ChildAxisBox  = UIBindLayoutRead(ChildrenAxisBoxX[ChildIndex], PlaceMinorAxisHandle, LayoutGraph);
                Command->ChildPosition = UIBindLayoutWrite(Position, PlaceMinorAxisHandle, LayoutGraph);
            }
        }
    }

    //
    // Place Y (Major Axis)
    //

    ui_graph_node_handle PlaceMajorAxisHandle = UIAddLayoutGraphNode(LayoutGraph);
    {
        ui_place_major_axis_command *Command = UIAcquirePlaceMajorAxisCommand(PlaceMajorAxisHandle, LayoutGraph);
        if(Command)
        {
            Command->Spacing         = Layout->Spacing;
            Command->Padding         = Layout->Padding.Left;
            Command->BorderWidth     = Layout->BorderWidth;
            Command->BorderInset     = Layout->BorderInset;
            Command->Justify         = Layout->Justify;
            Command->LayoutSize      = UIBindLayoutRead(LayoutSizeY , PlaceMajorAxisHandle, LayoutGraph);
            Command->ContentSize     = UIBindLayoutRead(ContentSizeX, PlaceMajorAxisHandle, LayoutGraph);
            Command->ContentPosition = UIBindLayoutWrite(ContentPositionY, PlaceMajorAxisHandle, LayoutGraph);
        }
    }
}


# endif // UI_HEADER
