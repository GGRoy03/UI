# ifndef UI_HEADER
#   define UI_HEADER


// ==========================================================
// [SECTION] : INCLUDES
// ==========================================================


# include <stdint.h>
# include <assert.h>
# include <stdbool.h>


// ==========================================================
// [SECTION] BIT TABLE
// ==========================================================
// [DESCRIPTION]
// ==========================================================


typedef struct ui_bit_table ui_bit_table;


typedef struct
{
    uint32_t EntryCount;
} ui_bit_table_params;

// ==========================================================
// [SECTION] DIRECTED GRAPH
// ==========================================================
// [DESCRIPTION]
// ==========================================================


typedef struct
{
    uint32_t ID;
} ui_graph_node_handle;


typedef struct ui_directed_graph ui_directed_graph;


# ifdef UI_IMPLEMENTATION
# endif


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

# define UI_IS_VALID_ENUM(Value, MaxExclusive) ((Value >= 0) && (Value < MaxExclusive))
# define UI_ARRAY_COUNT(Array)                 ((sizeof(Array) / sizeof(Array[0])))

# define UI_BIT_MASK(BitCount)          ((1ull << BitCount) - 1ull)
# define UI_CHECK_BIT_MASK(Value, Mask) ((Value & Mask))
# define UI_IS_POWER_OF_TWO(X)          ((X) && ((X & (X - 1)) == 0))


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
    float Size;
    float MinSize;
    float MaxSize;
} ui_fixed_sizing_task;


static float
UILayoutComputeFixedSizing(float Size, float MinSize, float MaxSize)
{
    float Result = 0.0f;

    float Min = UI_MAX(MinSize, 0.0f);
    float Max = UI_MAX(MaxSize, 0.0f);
    if(MinSize <= MaxSize)
    {
        Result = UI_MIN(UI_MAX(Size, MinSize), MaxSize);
    }
    else
    {
        Result = 0.0f;
    }

    return Result;
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
// [SECTION] BITSET
// ==========================================================
// [HISTORY]
// ==========================================================
// [TODO]
// - Test
// - Write the API
// - Write the init code
// ==========================================================


struct ui_bit_table
{
    uint64_t *Chunks;
    uint32_t  ChunkCount;
    uint32_t  EntryCount;
    uint32_t  BitPerChunk;
};


typedef struct
{
    uint64_t *Chunk;
    uint64_t  BitMask;
    bool      IsValid;
} ui_bit_table_entry;


static bool
UIIsValidBitTable(const ui_bit_table *BitTable)
{
    bool Result = BitTable && BitTable->Chunks;
    return Result;
}


static ui_bit_table
UIBitTable(uint64_t *Chunks, uint32_t ChunkCount)
{
    ui_bit_table Result =
    {
    };

    return Result;
}


static ui_bit_table_entry
UIBitTableGetEntry(uint32_t Index, const ui_bit_table *BitTable)
{
    assert(UIIsValidBitTable(BitTable));

    ui_bit_table_entry Result = {};

    if(Index < BitTable->EntryCount)
    {
        assert(UI_IS_POWER_OF_TWO(BitTable->BitPerChunk));

        uint32_t ChunkIndex = Index / BitTable->BitPerChunk;
        uint32_t BitIndex   = Index % BitTable->BitPerChunk;

        assert(ChunkIndex < BitTable->ChunkCount);

        Result.Chunk   = BitTable->Chunks + ChunkIndex;
        Result.BitMask = (1ULL << BitIndex);
        Result.IsValid = true;
    }

    return Result;
}


static void
UIBitTableSet(uint32_t Index, ui_bit_table *BitTable)
{
    if(UIIsValidBitTable(BitTable))
    {
        ui_bit_table_entry Entry = UIBitTableGetEntry(Index, BitTable);
        if(Entry.IsValid)
        {
            *Entry.Chunk |= Entry.BitMask;
        }
    }
}


static void
UIBitTableClear(uint32_t Index, ui_bit_table *BitTable)
{
    if(UIIsValidBitTable(BitTable))
    {
        ui_bit_table_entry Entry = UIBitTableGetEntry(Index, BitTable);
        if(Entry.IsValid)
        {
            *Entry.Chunk &= ~Entry.BitMask;
        }
    }
}


static bool
UIBitTableCheck(uint32_t Index, const ui_bit_table *BitTable)
{
    bool Result = false;

    if(UIIsValidBitTable(BitTable))
    {
        ui_bit_table_entry Entry = UIBitTableGetEntry(Index, BitTable);
        if(Entry.IsValid)
        {
            Result = *Entry.Chunk & Entry.BitMask;
        }
    }

    return Result;
}


// ==========================================================
// [SECTION] DIRECTED GRAPH
// ==========================================================
// [HISTORY]
// [8/27/2026]: Basic Directed Graph Implementation
// ==========================================================


typedef struct
{
    uint32_t TargetNodeIndex;
    uint32_t NextEdgeIndex;
} ui_graph_edge;


typedef struct
{
    uint32_t FirstEdgeIndex;
    uint32_t Degree;
} ui_graph_node;


struct ui_directed_graph
{
    ui_graph_node *Nodes;
    uint32_t       NodeCount;
    ui_graph_edge *Edges;
    uint32_t       EdgeCount;
};


static bool
UIIsValidDirectedGraph(const ui_directed_graph *Graph)
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
UIGetGraphNodeSentinel(const ui_directed_graph *Graph)
{
    assert(UIIsValidDirectedGraph(Graph));

    ui_graph_node *Result = Graph->Nodes;
    return Result;
}


static ui_graph_node *
UIGetGraphNode(uint32_t Index, const ui_directed_graph *Graph)
{
    assert(UIIsValidDirectedGraph(Graph));

    ui_graph_node *Result = 0;

    if(Index < Graph->NodeCount)
    {
        Result = Graph->Nodes + Index;
    }

    return Result;
}


static ui_graph_node *
UIGetGraphNodeFromHandle(ui_graph_node_handle Handle, const ui_directed_graph *Graph)
{
    ui_graph_node *Result = UIGetGraphNode(Handle.ID, Graph);
    return Result;
}


static ui_graph_edge *
UIGetGraphEdgeSentinel(const ui_directed_graph *Graph)
{
    assert(UIIsValidDirectedGraph(Graph));

    ui_graph_edge *Result = Graph->Edges;
    return Result;
}


static ui_graph_edge *
UIGetGraphEdge(uint32_t Index, const ui_directed_graph *Graph)
{
    assert(UIIsValidDirectedGraph(Graph));

    ui_graph_edge *Result = 0;

    if(Index < Graph->EdgeCount)
    {
        Result = Graph->Edges + Index;
    }

    return Result;
}


static ui_graph_node *
UIGetGraphNodeFromEdge(uint32_t Index, const ui_directed_graph *Graph)
{
    assert(Graph);

    ui_graph_node *Result = 0;

    ui_graph_edge *Edge = UIGetGraphEdge(Index, Graph);
    if(Edge)
    {
        Result = UIGetGraphNode(Edge->TargetNodeIndex, Graph);
    }

    return Result;
}


static uint32_t
UIGetNextGraphEdgeFromEdge(uint32_t Index, const ui_directed_graph *Graph)
{
    assert(UIIsValidDirectedGraph(Graph));

    uint32_t Result = {};

    ui_graph_edge *Edge = UIGetGraphEdge(Index, Graph);
    if(Edge)
    {
        Result = Edge->NextEdgeIndex;
    }

    return Result;
}


static void
UIFreeGraphEdge(uint32_t EdgeIndex, ui_directed_graph *Graph)
{
    assert(UIIsValidDirectedGraph(Graph));

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
UIPopFreeGraphEdge(ui_directed_graph *Graph)
{
    assert(UIIsValidDirectedGraph(Graph));

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
UIPopFreeGraphNode(ui_directed_graph *Graph)
{
    assert(UIIsValidDirectedGraph(Graph));

    uint32_t Result = 0;

    ui_graph_node *Sentinel = UIGetGraphNodeSentinel(Graph);
    assert(Sentinel);

    uint32_t       EdgeIndex = Sentinel->FirstEdgeIndex;
    ui_graph_edge *Edge      = UIGetGraphEdge(Sentinel->FirstEdgeIndex, Graph);
    if(Edge)
    {
        uint32_t       NodeIndex = Edge->TargetNodeIndex;
        ui_graph_node *Node      = UIGetGraphNode(Edge->TargetNodeIndex, Graph);
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


static uint32_t
UIAddGraphNode(ui_directed_graph *Graph)
{
    uint32_t Result = 0;
    if(UIIsValidDirectedGraph(Graph))
    {
        uint32_t       NodeIndex = UIPopFreeGraphNode(Graph);
        ui_graph_node *Node      = UIGetGraphNode(NodeIndex, Graph);

        if(Node)
        {
            Node->Degree         = 0;
            Node->FirstEdgeIndex = 0;
        }

        Result = NodeIndex;
    }

    return Result;
}


static void
UIRemoveGraphNode(ui_graph_node_handle Handle, ui_directed_graph *Graph)
{
    if(UIIsValidDirectedGraph(Graph))
    {
        ui_graph_node *Node = UIGetGraphNodeFromHandle(Handle, Graph);
        if(Node)
        {
            assert(Node != UIGetGraphNodeSentinel(Graph));

            uint32_t       EdgeIndex = Node->FirstEdgeIndex;
            ui_graph_edge *Edge      = UIGetGraphEdge(EdgeIndex, Graph);

            while(Edge)
            {
                ui_graph_node *TargetNode = UIGetGraphNode(Edge->TargetNodeIndex, Graph);
                if(TargetNode)
                {
                    assert(TargetNode->Degree > 0);
                    TargetNode->Degree -= 1;
                }

                UIFreeGraphEdge(EdgeIndex, Graph);

                EdgeIndex = Edge->NextEdgeIndex;
                Edge      = UIGetGraphEdge(EdgeIndex, Graph);
            }
        }
    }
}


static void
UIAddGraphEdge(uint32_t Source, uint32_t Target, ui_directed_graph *Graph)
{
    if(UIIsValidDirectedGraph(Graph))
    {
        ui_graph_node *SourceNode = UIGetGraphNode(Source, Graph);
        ui_graph_node *TargetNode = UIGetGraphNode(Target, Graph);
        if(SourceNode && TargetNode)
        {
            uint32_t       EdgeIndex = UIPopFreeGraphEdge(Graph);
            ui_graph_edge *Edge      = UIGetGraphEdge(EdgeIndex, Graph);
            if(Edge)
            {
                Edge->TargetNodeIndex = Target;
                Edge->NextEdgeIndex   = SourceNode->FirstEdgeIndex;

                SourceNode->FirstEdgeIndex = EdgeIndex;
                TargetNode->Degree        += 1;
            }
        }
    }
}


typedef struct
{
    uint32_t NodeCount;
    uint32_t EdgeCount;
} ui_directed_graph_params;


static uint64_t
UIDirectedGraphMemorySize(ui_directed_graph_params Params)
{
    ui_memory_size_counter Counter = {};
    {
        UIMemorySizeCountBuffer(ui_graph_node, Params.NodeCount, &Counter);
        UIMemorySizeCountBuffer(ui_graph_edge, Params.EdgeCount, &Counter);
        UIMemorySizeCountStruct(ui_directed_graph, &Counter);
    }

    uint64_t Result = UIMemorySizeCounterWorstCase(UI_ALIGN_OF(ui_directed_graph), Counter);
    return Result;
}


static ui_directed_graph *
UIDirectedGraphMemoryInit(void *Memory, uint64_t Size, ui_directed_graph_params Params)
{
    ui_directed_graph *Result = 0;

    ui_linear_allocator Allocator = UILinearAllocator(Memory, Size);
    if(UIIsValidLinearAllocator(&Allocator))
    {
        ui_graph_node     *Nodes = UIAllocateLinearArray(Params.NodeCount, ui_graph_node, &Allocator);
        ui_graph_edge     *Edges = UIAllocateLinearArray(Params.EdgeCount, ui_graph_edge, &Allocator);
        ui_directed_graph *Graph = UIAllocateLinearStruct(ui_directed_graph, &Allocator);

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

            for(uint32_t NodeIdx = 0; NodeIdx < Params.NodeCount; ++NodeIdx)
            {
                ui_graph_node *Node = UIGetGraphNode(NodeIdx, Graph);
                assert(Node);
                Node->FirstEdgeIndex = 0;
                Node->Degree         = 0;

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
    ui_directed_graph   *Graph;
    uint32_t             WaveCount;
    ui_graph_node_handle NextWaveHandles[16];
    uint32_t             NextWaveCount;
    bool                 IsValid;
} ui_graph_iterator;


static bool
UIIsValidGraphIterator(const ui_graph_iterator *Iterator)
{
    bool Result = Iterator && Iterator->Graph;
    return Result;
}


static ui_graph_iterator
UIGraphIteratorBegin(ui_directed_graph *Graph)
{
    ui_graph_iterator Iterator =
    {
        .Graph         = Graph,
        .WaveCount     = 0,
        .NextWaveCount = 0,
        .IsValid       = UIIsValidDirectedGraph(Graph),
    };

    return Iterator;
}


static bool
UIGraphIteratorNextWave(ui_graph_iterator *Iterator)
{
    bool CanContinue = false;

    if(Iterator && Iterator->IsValid)
    {
        ui_directed_graph *Graph = Iterator->Graph;

        if(Iterator->WaveCount == 0)
        {
            //
            // This is wrong, NodeCount is the total amount of nodes. If most nodes are unused, we're going to find
            // a bunch of degree 0 nodes which are useless to us. The graph should track the amount of nodes that are used.
            // But this problem is deeper, because there's simply no way to tell which are the valid nodes even if we know there
            // are 10 nodes, we don't know where they are stored. This also checks the sentinel. There are obvious solutions, but
            // they're not pretty. Uhm, this should be simple... Are we fine with the iterator having internal knowledge of the 
            // graph structure?
            //

            for(uint32_t NodeIdx = 0; NodeIdx < Graph->NodeCount; ++NodeIdx)
            {
            }
        }
        else
        {
            for(uint32_t WaveIdx = 0; WaveIdx < Iterator->NextWaveCount; ++WaveIdx)
            {
                //
                // Right... But the problem with that is that I simply don't know which nodes have hit the zero degree.
                // So I still have to do it manually :) Uhm. Weirdly linked to the first issue we have in this function
                // where it's hard to know which nodes are 0 degree/find them. I'd have to write the same code.
                // I think the iterator should be part of the graph API. With that being said...
                //

                ui_graph_node_handle NodeHandle = Iterator->NextWaveHandles[WaveIdx];
                ui_graph_node       *Node       = UIGetGraphNodeFromHandle(NodeHandle, Graph);

                if(Node)
                {
                    UIRemoveGraphNode(NodeHandle, Graph);
                }
            }
        }

        CanContinue = Iterator->IsValid;
    }

    return CanContinue;
}


// ==========================================================
// [SECTION] : WINDOW
// ==========================================================


typedef struct
{
    ui_linear_allocator FrameAllocator;
} ui_window;


typedef struct
{
    uint64_t FrameMemorySize;
} ui_window_params;


static uint64_t
UIWindowMemorySize(ui_window_params Params)
{
    ui_memory_size_counter Counter = {};
    {
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
        uint8_t   *FrameMemory         = UIAllocateLinearArray(Params.FrameMemorySize, uint8_t, &Allocator);
        ui_window *Window              = UIAllocateLinearStruct(ui_window, &Allocator);

        if(Window)
        {
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
        // Unsure what this would do really. Probably return some sort of draw commands for the caller.
        // We still need to fully execute the layout context. Caching/Building/Computing or whatever.
        // We don't really want to do this here. But _something_ has to do it and I wonder if we just cram everything
        // in the layout context. Does that make any sort of sense? Maybe? Why not? It doesn't have to own the output, does it?
        // It doesn't know how to crystalize. There's weird parallelism between contexts. Uhm... If we let the context execute...
        // Well, one thing is certain: We want some sort of mapping between stuff, wheter it's animations, style, layout, there's still
        // this concept of a "thing" (should probably find a name for that...) which I think the window should handle. So the other systems
        // style/layout/animations should allow the user to record a key alongisde their data. Something like that? We can try it. Key is a simple
        // number I think, fully opaque. Wrap it in a type so it's user overridable.
        //
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
    ui_directed_graph *Graph = 0;

    if(Layout->SizingX.Type == UISizing_Fixed)
    {
        for(ui_vertical_content_node *Node = Layout->FstContent; Node != 0; Node = Node->Next)
        {
            ui_vertical_content *Content = &Node->Content;

            if(Content->SizingX.Type == UISizing_Fixed)
            {
                //
                // Layout
                // x) Allocate Command
                // x) Fill Command
                // x) Allocate Node
                // x) Set Edge(s)
                //

            }
            else if(Content->SizingX.Type == UISizing_Percent)
            {
                //
                // Layout
                // x) Allocate Command
                // x) Fill Command
                // x) Allocate Node
                // x) Set Edge(s)
                //
                // Problem:
                // The percent-size command relies on the natural output of the parent.
                // Find a way to fill the command data, the percent command needs a reference to the parents natural size output, and how does
                // that fit into the execution context is quite unclear.
                //
                // Execution Context:
                // We iterate the graph in some way, which tells us which command to execute. It's always a command right, I mean it's technically
                // opaque... but, it's a dependency graph so like. I guess it could be used for other things now that I think about it. Anyways,
                // in our case, we would use one for the layout in the window context. The context then... Let's say we stick with the command
                // allocator idea. Allocate a command, this gives us an offset, map a node to that offset, as we iterate the graph we get offsets
                // back into commands, read them, execute them. The problem with this are the delayed commands. When commands depend on the output
                // from a previous commands. For example, the % case, the command needs to know what the parent's natural size was. Either, the %
                // command has a reference to the parent's natural size output or the parent's natural size writes into the % command once done.
                //
            }
            else if(Content->SizingX.Type == UISizing_Fit)
            {
            }
        }
    }
}


# endif // UI_HEADER
