# ifndef UI_HEADER
#   define UI_HEADER


// ==========================================================
// [SECTION] : TODOS
// ==========================================================
// X) Think about how we want to deal with resource
//    allocation. Currently the call-site and the internal
//    code is quite ugly. There's also performance concerns
//    that could matter, but are really hard to grasp fully
//    right now.
//
// PRIORITY
// X) Less verbose building API, currently it's hell. That's
//    fine as we are focusing on something that should work,
//    once we find the correct direction, clean/simplify the
//    API.
//
// X) The layout graph is hard to debug/reason through at
//    a low-level. Could we embed and print some debug stuff
//    into the nodes?
//
// X) The graph iterator currently returns a node handle
//    which is basically useless. Should mostly return the
//    command data or something.
//
// X) The LeaveVerticalLayout function is currently a mess.
//    It is our main prototype, so it'd be nice if it
//    actually looked nice :). There's probably huge
//    simplifications we could do for the API.
//
// PRIORITY
// X) There's currently a bug in the way we enqueue commands.
//    The root is basically never handled, since only parent
//    to child is handled when trying to pipe the outputs.
//    This seems to be only true for the roots and for the
//    final write. In any case, the root needs some special
//    casing, that's for sure.
//

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
#  define UI_ALIGN_OF(X) (_Alignof(X)) 
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
// [SECTION] LAYOUT RESOURCES
// ==========================================================
// [DESCRIPTION]
// ==========================================================


typedef struct
{
    void    *Data;
    uint32_t ID;
} ui_layout_resource_handle;


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


typedef enum
{
    UILayoutCommand_PlaceMajorAxis = 0,
    UILayoutCommand_PlaceMinorAxis = 1,
    UILayoutCommand_FixedSizing    = 2,
    UILayoutCommand_AxisBoxSizing  = 3,
    UILayoutCommand_ConstructRect  = 4,
} UILayoutCommand;


typedef union
{
    struct
    {
        float Left;
        float Right;
    };

    struct
    {
        float Top;
        float Bot;
    };
} ui_axis_padding;


typedef struct
{
    ui_layout_resource_handle Value;
} ui_layout_read_handle;


typedef struct
{
    ui_layout_resource_handle Value;
} ui_layout_write_handle;


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


typedef struct
{
    float                  FixedSize;
    float                  MinSize;
    float                  MaxSize;
    ui_layout_write_handle OutputSize;
} ui_fixed_sizing_command;


typedef struct
{
    ui_axis_padding        Padding;
    float                  BorderWidth;
    float                  BorderInset;
    ui_layout_read_handle  InputSize;
    ui_layout_write_handle OutputAxisBox;
} ui_axis_box_sizing_command;


typedef struct
{
    ui_layout_read_handle InputPosX;
    ui_layout_read_handle InputPosY;
    ui_layout_read_handle InputSizeX;
    ui_layout_read_handle InputSizeY;
    uint32_t              OutputIndex;
} ui_construct_rect_command;


// ==========================================================
// [SECTION] LAYOUT GRAPH
// ==========================================================
// [DESCRIPTION]
// ==========================================================


typedef struct ui_layout_graph ui_layout_graph;


typedef struct
{
    uint32_t ID;
} ui_graph_node_handle;


// ==========================================================
// [SECTION] WINDOW
// ==========================================================
// [DESCRIPTION]
// ==========================================================


typedef struct ui_window ui_window;


// ==========================================================
// [SECTION] WINDOW LAYOUT
// ==========================================================
// [DESCRIPTION]
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
    ui_sizing       SizingX;
    ui_sizing       SizingY;
    ui_size         Min;
    ui_size         Max;
    float           Spacing;
    float           BorderWidth;
    float           BorderInset;
    ui_axis_padding PaddingX;
    ui_axis_padding PaddingY;
    UIAlignment     Alignment;
    UIJustify       Justify;
    uint32_t        StableIndex;
} ui_layout_content;


static ui_sizing UIFixedSizing  (float Value);
static ui_size   UISize         (float X, float Y);


// ==========================================================
// [SECTION] VERTICAL LAYOUT | WINDOW
// ==========================================================
// [DESCRIPTION]
// ==========================================================


typedef struct ui_vertical_layout ui_vertical_layout;


static ui_vertical_layout * UIEnterVerticalLayout  (const ui_layout_content *Content, ui_window *Window);
static ui_layout_content  * UIPushVerticalContent  (ui_vertical_layout *Layout);
static void                 UILeaveVerticalLayout  (const ui_vertical_layout *Layout, ui_window *Window);


# ifdef UI_IMPLEMENTATION
# endif


// ==========================================================
// [SECTION] : ALLOCATOR HELPERS
// ==========================================================


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
        Result += Align - Remainder;
    }

    return Result;
}


// ==========================================================
// [SECTION] : LINEAR ALLOCATOR
// ==========================================================


static const size_t UI_LINEAR_ALLOCATOR_DEFAULT_ALIGNMENT = 64;


typedef struct
{
    uint8_t *Buffer;
    size_t   BufferSize;
    size_t   At;
} ui_linear_allocator;


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


#define UIAllocateLinearArray(Count, Type, Allocator) (Type *)UIAllocateLinearAligned((Count * sizeof(Type)), UI_ALIGN_OF(Type), Allocator)
#define UIAllocateLinearStruct(Type, Allocator)       (Type *)UIAllocateLinearAligned((sizeof(Type)), UI_ALIGN_OF(Type), Allocator)


// ==========================================================
// [SECTION] : MEMORY COUNTER
// ==========================================================


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

    return Result;
}


// ==========================================================
// [SECTION] LAYOUT GRAPH
// ==========================================================


typedef struct ui_layout_resource_node ui_layout_resource_node;
struct ui_layout_resource_node
{
    ui_layout_resource_node *Next;
    uint32_t                 ResourceID;
    uint32_t                 GraphNodeIndex;
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
    uint32_t        FirstEdgeIndex;
    uint32_t        DependencyCount;
    UILayoutCommand CommandType;
    void           *CommandPointer;
} ui_graph_node;


struct ui_layout_graph
{
    ui_graph_node           *Nodes;
    uint32_t                 NodeCount;
    ui_graph_edge           *Edges;
    uint32_t                 EdgeCount;
    ui_linear_allocator      FrameAllocator;
    ui_layout_resource_list  ReadList;
    ui_layout_resource_list  WriteList;
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
            uint32_t EdgeIndex = UIPopFreeGraphEdge(Graph);
            if(EdgeIndex != 0)
            {
                ui_graph_edge *Edge = UIGetGraphEdge(EdgeIndex, Graph);
                assert(Edge);

                Edge->TargetNodeIndex = Target;
                Edge->NextEdgeIndex   = SourceNode->FirstEdgeIndex;

                SourceNode->FirstEdgeIndex = EdgeIndex;
                TargetNode->DependencyCount    += 1;
            }
        }
    }
}


typedef struct
{
    uint32_t NodeCount;
    uint32_t EdgeCount;
    uint32_t ScratchSize;
} ui_layout_graph_params;


static uint64_t
UILayoutGraphMemorySize(ui_layout_graph_params Params)
{
    ui_memory_size_counter Counter = {};
    {
        UIMemorySizeCountBuffer(ui_graph_node, Params.NodeCount, &Counter);
        UIMemorySizeCountBuffer(ui_graph_edge, Params.EdgeCount, &Counter);
        UIMemorySizeCountBuffer(uint8_t, Params.ScratchSize, &Counter);

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
        ui_graph_node *Nodes   = UIAllocateLinearArray(Params.NodeCount, ui_graph_node, &Allocator);
        ui_graph_edge *Edges   = UIAllocateLinearArray(Params.EdgeCount, ui_graph_edge, &Allocator);
        uint8_t       *Scratch = UIAllocateLinearArray(Params.ScratchSize, uint8_t, &Allocator);

        ui_layout_graph *Graph = UIAllocateLinearStruct(ui_layout_graph, &Allocator);
        if(Graph)
        {
            Graph->Nodes          = Nodes;
            Graph->NodeCount      = Params.NodeCount;
            Graph->Edges          = Edges;
            Graph->EdgeCount      = Params.EdgeCount;
            Graph->FrameAllocator = UILinearAllocator(Scratch, Params.ScratchSize);

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
    const ui_layout_graph   *Graph;
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

        uint32_t *ResourceIDToProducerIndex = UIAllocateLinearArray(128, uint32_t, &Graph->FrameAllocator);

        //
        // Write-List
        //

        ui_layout_resource_list WriteList = Graph->WriteList;
        for(ui_layout_resource_node *Node = WriteList.First; Node != 0; Node = Node->Next)
        {
            assert(Node->ResourceID < 128);

            ResourceIDToProducerIndex[Node->ResourceID] = Node->GraphNodeIndex;
        }

        //
        // Read-List
        //

        ui_layout_resource_list ReadList = Graph->ReadList;
        for(ui_layout_resource_node *Node = ReadList.First; Node != 0; Node = Node->Next)
        {
            assert(Node->ResourceID < 128);

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

        ResourceNode->GraphNodeIndex = NodeIndex;
        ResourceNode->ResourceID     = Handle.ID;
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
UIBindCommand(ui_graph_node_handle Handle, uint64_t Size, uint64_t Align, UILayoutCommand Type, ui_layout_graph *Graph)
{
    void *Result = 0;

    if(UIIsValidLayoutGraph(Graph))
    {
        ui_graph_node *Node = UIGetGraphNode(Handle.ID, Graph);
        if(Node && Node->CommandPointer == 0)
        {
            Result = UIAllocateLinearAligned(Size, Align, &Graph->FrameAllocator);

            Node->CommandPointer = Result;
            Node->CommandType    = Type;
        }
    }

    return Result;
}


// ==========================================================
// [SECTION] : WINDOW
// ==========================================================


struct ui_window
{
    ui_linear_allocator FrameAllocator;
    ui_layout_graph    *LayoutGraph;
    uint32_t            NextStableIndex;
    uint32_t            NextContentStableIndex;
};


typedef struct
{
    uint64_t               FrameMemorySize;
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



static ui_layout_resource_handle
UIWindowPushLayoutResource(uint64_t Size, uint64_t Align, ui_window *Window)
{
    ui_layout_resource_handle Result = {};

    if(Window)
    {
        void *Data = UIAllocateLinearAligned(Size, Align, &Window->FrameAllocator);
        if(Data)
        {
            Result.Data = Data;
            Result.ID   = Window->NextStableIndex;

            ++Window->NextStableIndex;
        }
    }

    return Result;
}


// ==========================================================
// [SECTION] WINDOW LAYOUT
// ==========================================================


typedef struct ui_layout_content_node ui_layout_content_node;
struct ui_layout_content_node
{
    ui_layout_content       Content;
    ui_layout_content_node *Next;
};


static ui_sizing
UIFixedSizing(float Value)
{
    ui_sizing Result =
    {
        .Type  = UISizing_Fixed,
        .Fixed = Value,
    };

    return Result;
}


static ui_size
UISize(float X, float Y)
{
    ui_size Result =
    {
        .X = X,
        .Y = Y,
    };

    return Result;
}

// ==========================================================
// [SECTION] : VERTICAL LAYOUT | WINDOW
// ==========================================================


struct ui_vertical_layout
{
    ui_layout_content       Parent;
    ui_layout_content_node *FstContent;
    ui_layout_content_node *LstContent;
    uint32_t                ContentCount;
    ui_window              *Window;
};


static ui_vertical_layout *
UIEnterVerticalLayout(const ui_layout_content *Content, ui_window *Window)
{
    ui_vertical_layout *Result = 0;

    if(Window)
    {
        ui_vertical_layout *Layout = UIAllocateLinearStruct(ui_vertical_layout, &Window->FrameAllocator);
        if(Layout)
        {
            if(Content)
            {
                Layout->Parent = *Content;
            }

            Layout->Window = Window;
        }

        Result = Layout;
    }

    return Result;
}


static ui_layout_content *
UIPushVerticalContent(ui_vertical_layout *Layout)
{
    ui_layout_content *Result = 0;

    if(Layout)
    {
        ui_window *Window = Layout->Window;
        assert(Window);

        ui_layout_content_node *Node = UIAllocateLinearStruct(ui_layout_content_node, &Window->FrameAllocator);
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
            Result->SizingX     = (ui_sizing){};
            Result->SizingY     = (ui_sizing){};
            Result->Min         = (ui_size){};
            Result->Max         = (ui_size){};
            Result->StableIndex = Window->NextContentStableIndex;

            ++Window->NextContentStableIndex;
        }
    }

    return Result;
}


//
// NOTE: DO NOT PANIC! (250 lines long :)..)
// This is a monster function and also a very rough V0.1, it will be refactored many times to be nicer to work with.
//


static void
UILeaveVerticalLayout(const ui_vertical_layout *Layout, ui_window *Window)
{
    ui_layout_graph     *LayoutGraph = Window->LayoutGraph;
    ui_linear_allocator *Allocator   = &Window->FrameAllocator;


    ui_layout_resource_handle *ChildrenAxisBoxX = UIAllocateLinearArray(Layout->ContentCount, ui_layout_resource_handle, Allocator);
    ui_layout_resource_handle *ChildrenAxisBoxY = UIAllocateLinearArray(Layout->ContentCount, ui_layout_resource_handle, Allocator);

    //
    // Sizing X
    //

    ui_layout_content        *Parent         = (ui_layout_content *)&Layout->Parent;
    ui_layout_resource_handle ParentAxisBoxX = UIWindowPushLayoutResource(sizeof(ui_axis_box), UI_ALIGN_OF(ui_axis_box), Window);

    if(Parent->SizingX.Type == UISizing_Fixed)
    {
        //
        // Parent Sizing
        //

        ui_graph_node_handle      ParentSizeHandle = UIAddLayoutGraphNode(LayoutGraph);
        ui_layout_resource_handle ParentSizeX      = UIWindowPushLayoutResource(sizeof(float), UI_ALIGN_OF(float), Window);

        {
            ui_fixed_sizing_command *Command = (ui_fixed_sizing_command *)UIBindCommand(ParentSizeHandle, sizeof(ui_fixed_sizing_command), UI_ALIGN_OF(ui_fixed_sizing_command), UILayoutCommand_FixedSizing, LayoutGraph);
            if(Command)
            {
                Command->FixedSize  = Parent->SizingX.Fixed;
                Command->MinSize    = Parent->Min.X;
                Command->MaxSize    = Parent->Max.X;
                Command->OutputSize = UIBindLayoutWrite(ParentSizeX, ParentSizeHandle, LayoutGraph);
            }
        }

        ui_graph_node_handle        ParentAxisBoxHandle = UIAddLayoutGraphNode(LayoutGraph);
        ui_axis_box_sizing_command *Command             = (ui_axis_box_sizing_command *)UIBindCommand(ParentAxisBoxHandle, sizeof(ui_axis_box_sizing_command), UI_ALIGN_OF(ui_axis_box_sizing_command), UILayoutCommand_AxisBoxSizing, LayoutGraph);
        {
            if(Command)
            {
                Command->Padding       = Parent->PaddingX;
                Command->BorderWidth   = Parent->BorderWidth;
                Command->BorderInset   = Parent->BorderInset;
                Command->InputSize     = UIBindLayoutRead(ParentSizeX, ParentAxisBoxHandle, LayoutGraph);
                Command->OutputAxisBox = UIBindLayoutWrite(ParentAxisBoxX, ParentAxisBoxHandle, LayoutGraph);
            }
        }


        uint32_t ChildIndex = 0;
        for(ui_layout_content_node *Node = Layout->FstContent; Node != 0; Node = Node->Next)
        {
            ui_layout_content *Content = &Node->Content;

            //
            // Child Sizing
            //

            ui_graph_node_handle      ChildSizeHandle = UIAddLayoutGraphNode(LayoutGraph);
            ui_layout_resource_handle ChildSizeX      = UIWindowPushLayoutResource(sizeof(float), UI_ALIGN_OF(float), Window);

            if(Content->SizingX.Type == UISizing_Fixed)
            {
                ui_fixed_sizing_command *Command = (ui_fixed_sizing_command *)UIBindCommand(ChildSizeHandle, sizeof(ui_fixed_sizing_command), UI_ALIGN_OF(ui_fixed_sizing_command), UILayoutCommand_FixedSizing, LayoutGraph);
                if(Command)
                {
                    Command->FixedSize  = Parent->SizingX.Fixed;
                    Command->MinSize    = Parent->Min.X;
                    Command->MaxSize    = Parent->Max.X;
                    Command->OutputSize = UIBindLayoutWrite(ChildSizeX, ChildSizeHandle, LayoutGraph);
                }
            }

            //
            // Child Box Model
            //

            ChildrenAxisBoxX[ChildIndex] = UIWindowPushLayoutResource(sizeof(float), UI_ALIGN_OF(float), Window);

            ui_graph_node_handle ChildBoxModelHandle = UIAddLayoutGraphNode(LayoutGraph);
            {
                ui_axis_box_sizing_command *Command = (ui_axis_box_sizing_command *)UIBindCommand(ChildBoxModelHandle, sizeof(ui_axis_box_sizing_command), UI_ALIGN_OF(ui_axis_box_sizing_command), UILayoutCommand_AxisBoxSizing, LayoutGraph);
                if(Command)
                {
                    Command->Padding       = Parent->PaddingX;
                    Command->BorderWidth   = Parent->BorderWidth;
                    Command->BorderInset   = Parent->BorderInset;
                    Command->InputSize     = UIBindLayoutRead(ChildSizeX, ChildBoxModelHandle, LayoutGraph);
                    Command->OutputAxisBox = UIBindLayoutWrite(ChildrenAxisBoxX[ChildIndex], ChildBoxModelHandle, LayoutGraph);
                }
            }
        }
    }

    //
    // Sizing Y
    //

    ui_layout_resource_handle ParentSizeY    = UIWindowPushLayoutResource(sizeof(float)      , UI_ALIGN_OF(float)      , Window);
    ui_layout_resource_handle ParentAxisBoxY = UIWindowPushLayoutResource(sizeof(ui_axis_box), UI_ALIGN_OF(ui_axis_box), Window);

    if(Parent->SizingY.Type == UISizing_Fixed)
    {
        //
        // Parent Sizing
        //

        ui_graph_node_handle      ParentSizeHandle = UIAddLayoutGraphNode(LayoutGraph);
        ui_layout_resource_handle ParentSizeY      = UIWindowPushLayoutResource(sizeof(float), UI_ALIGN_OF(float), Window);

        {
            ui_fixed_sizing_command *Command = (ui_fixed_sizing_command *)UIBindCommand(ParentSizeHandle, sizeof(ui_fixed_sizing_command), UI_ALIGN_OF(ui_fixed_sizing_command), UILayoutCommand_FixedSizing, LayoutGraph);
            if(Command)
            {
                Command->FixedSize  = Parent->SizingY.Fixed;
                Command->MinSize    = Parent->Min.Y;
                Command->MaxSize    = Parent->Max.Y;
                Command->OutputSize = UIBindLayoutWrite(ParentSizeY, ParentSizeHandle, LayoutGraph);
            }
        }

        ui_graph_node_handle        ParentAxisBoxHandle = UIAddLayoutGraphNode(LayoutGraph);
        ui_axis_box_sizing_command *Command             = (ui_axis_box_sizing_command *)UIBindCommand(ParentAxisBoxHandle, sizeof(ui_axis_box_sizing_command), UI_ALIGN_OF(ui_axis_box_sizing_command), UILayoutCommand_AxisBoxSizing, LayoutGraph);
        {
            if(Command)
            {
                Command->Padding       = Parent->PaddingY;
                Command->BorderWidth   = Parent->BorderWidth;
                Command->BorderInset   = Parent->BorderInset;
                Command->InputSize     = UIBindLayoutRead(ParentSizeY, ParentAxisBoxHandle, LayoutGraph);
                Command->OutputAxisBox = UIBindLayoutWrite(ParentAxisBoxY, ParentAxisBoxHandle, LayoutGraph);
            }
        }

        uint32_t ChildIndex = 0;
        for(ui_layout_content_node *Node = Layout->FstContent; Node != 0; Node = Node->Next)
        {
            ui_layout_content *Content = &Node->Content;

            ui_graph_node_handle      ChildSizeHandle = UIAddLayoutGraphNode(LayoutGraph);
            ui_layout_resource_handle ChildSizeY      = UIWindowPushLayoutResource(sizeof(float), UI_ALIGN_OF(float), Window);

            if(Content->SizingY.Type == UISizing_Fixed)
            {
                ui_fixed_sizing_command *Command = (ui_fixed_sizing_command *)UIBindCommand(ChildSizeHandle, sizeof(ui_fixed_sizing_command), UI_ALIGN_OF(ui_fixed_sizing_command), UILayoutCommand_FixedSizing, LayoutGraph);
                if(Command)
                {
                    Command->FixedSize  = Parent->SizingY.Fixed;
                    Command->MinSize    = Parent->Min.Y;
                    Command->MaxSize    = Parent->Max.Y;
                    Command->OutputSize = UIBindLayoutWrite(ChildSizeY, ChildSizeHandle, LayoutGraph);
                }
            }

            //
            // Child Box Model
            //

            ChildrenAxisBoxY[ChildIndex] = UIWindowPushLayoutResource(sizeof(float), UI_ALIGN_OF(float), Window);

            ui_graph_node_handle ChildBoxModelHandle = UIAddLayoutGraphNode(LayoutGraph);
            {
                ui_axis_box_sizing_command *Command = (ui_axis_box_sizing_command *)UIBindCommand(ChildBoxModelHandle, sizeof(ui_axis_box_sizing_command), UI_ALIGN_OF(ui_axis_box_sizing_command), UILayoutCommand_AxisBoxSizing, LayoutGraph);
                if(Command)
                {
                    Command->Padding       = Parent->PaddingY;
                    Command->BorderWidth   = Parent->BorderWidth;
                    Command->BorderInset   = Parent->BorderInset;
                    Command->InputSize     = UIBindLayoutRead(ChildSizeY, ChildBoxModelHandle, LayoutGraph);
                    Command->OutputAxisBox = UIBindLayoutWrite(ChildrenAxisBoxY[ChildIndex], ChildBoxModelHandle, LayoutGraph);
                }
            }
        }
    }


    //
    // Place X (Minor Axis)
    //

    ui_layout_resource_handle *ChildrenPositionX = UIAllocateLinearArray(Layout->ContentCount, ui_layout_resource_handle, Allocator);
    if(ChildrenPositionX)
    {
        for(uint32_t ChildIdx = 0; ChildIdx < Layout->ContentCount; ++ChildIdx)
        {
            ChildrenPositionX[ChildIdx] = UIWindowPushLayoutResource(sizeof(float), UI_ALIGN_OF(float), Window);
        }

        uint32_t ChildIndex = 0;
        for(ui_layout_content_node *Node = Layout->FstContent; Node != 0; Node = Node->Next, ChildIndex += 1)
        {
            ui_graph_node_handle         PlaceMinorAxisHandle = UIAddLayoutGraphNode(LayoutGraph);
            ui_place_minor_axis_command *Command              = (ui_place_minor_axis_command *)UIBindCommand(PlaceMinorAxisHandle, sizeof(ui_place_minor_axis_command), UI_ALIGN_OF(ui_place_minor_axis_command), UILayoutCommand_PlaceMinorAxis, LayoutGraph);

            if(Command)
            {
                Command->Padding       = Parent->PaddingX.Left;
                Command->BorderWidth   = Parent->BorderWidth;
                Command->BorderInset   = Parent->BorderInset;
                Command->Alignment     = Parent->Alignment;
                Command->ParentAxisBox = UIBindLayoutRead(ParentAxisBoxX, PlaceMinorAxisHandle, LayoutGraph);
                Command->ChildAxisBox  = UIBindLayoutRead(ChildrenAxisBoxX[ChildIndex], PlaceMinorAxisHandle, LayoutGraph);
                Command->ChildPosition = UIBindLayoutWrite(ChildrenPositionX[ChildIndex], PlaceMinorAxisHandle, LayoutGraph);
            }
        }
    }

    //
    // Place Y (Major Axis)
    //

    ui_layout_resource_handle *ChildrenPositionY = UIAllocateLinearArray(Layout->ContentCount, ui_layout_resource_handle, Allocator);
    if(ChildrenPositionY)
    {
        for(uint32_t ChildIdx = 0; ChildIdx < Layout->ContentCount; ++ChildIdx)
        {
            ChildrenPositionY[ChildIdx] = UIWindowPushLayoutResource(sizeof(float), UI_ALIGN_OF(float), Window);
        }

        ui_graph_node_handle PlaceMajorAxisHandle = UIAddLayoutGraphNode(LayoutGraph);
        {
            ui_place_major_axis_command *Command = (ui_place_major_axis_command *)UIBindCommand(PlaceMajorAxisHandle, sizeof(ui_place_major_axis_command), UI_ALIGN_OF(ui_place_major_axis_command), UILayoutCommand_PlaceMajorAxis, LayoutGraph);
            if(Command)
            {
                Command->Spacing          = Parent->Spacing;
                Command->Padding          = Parent->PaddingY.Top;
                Command->BorderWidth      = Parent->BorderWidth;
                Command->BorderInset      = Parent->BorderInset;
                Command->Justify          = Parent->Justify;
                Command->ParentAxisBox    = UIBindLayoutRead(ParentAxisBoxY , PlaceMajorAxisHandle, LayoutGraph);
                Command->ChildrenAxisBox  = UIAllocateLinearArray(Layout->ContentCount, ui_layout_read_handle , Allocator);
                Command->ChildrenPosition = UIAllocateLinearArray(Layout->ContentCount, ui_layout_write_handle, Allocator);
     
                uint32_t ChildIndex = 0;
                for(ui_layout_content_node *Node = Layout->FstContent; Node != 0; Node = Node->Next, ChildIndex += 1)
                {
                    Command->ChildrenAxisBox[ChildIndex]  = UIBindLayoutRead(ChildrenAxisBoxY[ChildIndex], PlaceMajorAxisHandle, LayoutGraph);
                    Command->ChildrenPosition[ChildIndex] = UIBindLayoutWrite(ChildrenPositionY[ChildIndex], PlaceMajorAxisHandle, LayoutGraph);
                }
            }
        }
    }

    //
    // Construct Children
    //

    for(uint32_t ChildIdx = 0; ChildIdx < Layout->ContentCount; ++ChildIdx)
    {
        ui_graph_node_handle       ChildRectHandle = UIAddLayoutGraphNode(LayoutGraph);
        ui_construct_rect_command *Command         = (ui_construct_rect_command *)UIBindCommand(ChildRectHandle, sizeof(ui_construct_rect_command), UI_ALIGN_OF(ui_construct_rect_command), UILayoutCommand_ConstructRect, LayoutGraph);
        if(Command)
        {
            Command->InputPosX  = UIBindLayoutRead(ChildrenPositionX[ChildIdx], ChildRectHandle, LayoutGraph);
            Command->InputPosY  = UIBindLayoutRead(ChildrenPositionY[ChildIdx], ChildRectHandle, LayoutGraph);
            Command->InputSizeX = UIBindLayoutRead(ChildrenAxisBoxX[ChildIdx] , ChildRectHandle, LayoutGraph);
            Command->InputSizeY = UIBindLayoutRead(ChildrenAxisBoxY[ChildIdx] , ChildRectHandle, LayoutGraph);
        }
    }

}


# endif // UI_HEADER
