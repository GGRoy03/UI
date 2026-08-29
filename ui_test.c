# include "ui.h"

#include <assert.h>
#include <stdlib.h>

int main()
{
    ui_directed_graph_params Params =
    {
        .NodeCount = 100,
        .EdgeCount = 200,
    };

    uint64_t           GraphSize = UIDirectedGraphMemorySize(Params);
    void              *Memory    = malloc(GraphSize);
    ui_directed_graph *Graph     = UIDirectedGraphMemoryInit(Memory, GraphSize, Params);

    ui_window *Window = 0;
    if(Window)
    {
        // while(true)
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

            ui_vertical_layout *Container = UIEnterVerticalLayout(0, Window);
            if(Container)
            {
                ui_vertical_content *TitleBar = UIPushVerticalContent(Container);
                if(TitleBar)
                {
                    //
                    // I'd like to make this nicer to write, but the idea seems correct.
                    //

                    TitleBar->SizingX = (ui_sizing){.Type = UISizing_Fixed, .Fixed = 250.0f};
                    TitleBar->SizingY = (ui_sizing){.Type = UISizing_Fixed, .Fixed = 450.0f};
                    TitleBar->Min     = (ui_size){.X = 250.0f, .Y = 450.0f};
                    TitleBar->Max     = (ui_size){.X = 250.0f, .Y = 450.0f};
//
                    // Now... The titlebar itself wants content. And so.. there's a root version (above) and there's a
                    // non-root version right? And both just work. Sort of... Ah it's just that the non-root is able to do things
                    // like: construct itself from the content. Obviously this would be horizontal.
                    //

                    ui_vertical_layout *TitleBarContainer = UIEnterVerticalLayout(TitleBar, Window);
                    if(TitleBarContainer)
                    {
                        //
                        // UIPushVerticalContent() // Some button
                        // UIPushVerticalContent() // Some button
                        // UIPushVerticalContent() // Some button
                        //
                    }
                }
            }


        }
    }
}
