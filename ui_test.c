# include "ui.h"

#include <assert.h>
#include <stdlib.h>

int main()
{
    //
    // x) Allocate commands for that window (from a vertical layout context)
    // x) Process these commands
    //

    ui_window_params Params =
    {
        .FrameMemorySize = UI_KIB(16),
        .Scheduler       =
        {
            .TaskPayloadSize = UI_KIB(16), // TODO: Rename to frame memory size?
            .EntryCount      = 64,
        },
    };

    uint64_t   WindowSize   = UIWindowMemorySize(Params);
    void      *WindowMemory = malloc(WindowSize);
    ui_window *Window       = UIWindowMemoryInit(WindowMemory, WindowSize, Params);

    if(Window)
    {
        while(true)
        {
            //
            // This is a bit weirder than I imagined. So any node that has no layout attached to it, is a leaf
            // node, it's basically either controlled by its parent or by its content. Which makes me think
            // we need a general node parameter structure... Anyways, to start a layout from a node, we simply
            // need to pass it in the enter function right? What is node based vs what is layout based. Something
            // like spacing or padding is layout related, so it's just specific to what you want to build.
            //
            // LayoutContext = UIEnterVerticalLayout(NodeParams, LayoutParams)
            //  -NodeParams:    Sizing, MinSize, MaxSize, ...
            //  -LayoutParams:  Spacing, Padding, ...
            //  -LayoutContext: Holds enough information to schedule the appropriate commands on leave.
            //
            //  Node = UIPushVerticalNode(NodeParams, LayoutContext)
            //  -Node:          Some opaque reference to the newly created node.
            //  -NodeParams:    Sizing, MinSize, MaxSize, ...
            //  -LayoutContext: Holds enough information to schedule the appropriate commands on leave.
            //
            //  LayoutContext = UIEnterVerticalLayout(Node, LayoutParams)
            //  UILeaveVerticalLayout(LayoutContext)
            //
            // UILeaveVerticalLayout(LayoutContext)
            //

            //
            // RANDOM THOUGHTS:
            // x) We allow the child to overflow the parent, this should not be the base behavior (in the case where the
            // child can be resized). We simply cannot.. We are lacking a whole class of algorithms. We'd need something that runs
            // _after_ and checks its child sizes and resizes them as needed depending on the mode or whatever. Is it solveable
            // using the current ideas/model or do we have to do something differently. We must also think of things like parent
            // padding and spacing. Something like space distribution algorithms.. We are back to the one to many problem we had
            // before.
            // x) Perhaps node desc are stored internally as read-only references. In the case of space-distribution algorithms,
            // we can't only rely on the final computed size, we'd also have to know the min/max size of each node to correctly
            // distribute space. It wouldn't be a full node desc probably, but it feels like some algorithms may rely on data
            // that is not computed yet and on data that is already computed.
            // x) There's a missing bridge between the node desc and when we enter the vertical layout. The link between the node
            // and the layout is sort of "fragile", quite hard to explain. Basically, should UIPushVerticalLayoutNode return something
            // that identifies the created node?
            // x) If we need a one to many relationship for the space distribution stuff, is there ever a world where we delay
            // things like the scheduler allocation? The push function could be used to allocate some node within a linked list
            // in the layout context? And only as we leave the layout process the whole context at once?
            // x) Oof. But now even fixed computations are only complete when the layout resizing runs. So.. anything that
            // depends on it, actually now depends on the resizing pass running. Uh. We have to fully process the local layout
            // first in any case I guess. This is becoming really complex :/
            //

            ui_node_desc Container =
            {
                .SizingX = {.Type = UISizing_Fixed, .Fixed = 300.0f},
                .SizingY = {.Type = UISizing_Fixed, .Fixed = 500.0f},
                .MinSize = {.X = 300.0f, .Y = 500.0f},
                .MaxSize = {.X = 300.0f, .Y = 500.0f},
            };

            ui_node_desc TitleBar =
            {
                .SizingX = {.Type = UISizing_Fixed, .Fixed = 310.0f},
                .SizingY = {.Type = UISizing_Fixed, .Fixed = 500.0f},
                .MinSize = {.X = 100.0f, .Y = 250.0f},
                .MaxSize = {.X = 310.0f, .Y = 500.0f},
            };

            ui_vertical_layout LayoutContext_A = UIEnterVerticalLayout(Container);
            {
            };
            UILeaveVerticalLayout(&LayoutContext_A, Window);
        }
    }
}
