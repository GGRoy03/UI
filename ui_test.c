# include "ui.h"

#include <assert.h>
#include <stdlib.h>

int main()
{
    ui_window_params Params =
    {
        .FrameMemorySize = UI_KIB(16),

        .LayoutGraph =
        {
            .NodeCount   = 100,
            .EdgeCount   = 200,
            .ScratchSize = UI_KIB(16),
        },
    };

    uint64_t   WindowSize = UIWindowMemorySize(Params);
    void      *Memory     = calloc(WindowSize, 1);
    ui_window *Window     = UIWindowMemoryInit(Memory, WindowSize, Params);

    if(Window)
    {
        ui_layout_graph *LayoutGraph = Window->LayoutGraph;
        {
            ui_layout_content Container =
            {
                .SizingX = UIFixedSizing(450.0f),
                .SizingY = UIFixedSizing(800.0f),
                .Min     = UISize(300.0f, 700.0f),
                .Max     = UISize(500.0f, 850.0f),
            };

            ui_vertical_layout *VBox = UIEnterVerticalLayout(&Container, Window);
            if(VBox)
            {
                ui_layout_content *Header = UIPushVerticalContent(VBox);
                if(Header)
                {
                    Header->SizingX = UIFixedSizing(430.0f);
                    Header->SizingY = UIFixedSizing(150.0f);
                    Header->Min     = UISize(430.0f, 150.0f);
                    Header->Max     = UISize(430.0f, 150.0f);
                }

                ui_layout_content *Area = UIPushVerticalContent(VBox);
                if(Area)
                {
                    Area->SizingX = UIFixedSizing(430.0f);
                    Area->SizingY = UIFixedSizing(600.0f);
                    Area->Min     = UISize(430.0f, 600.0f);
                    Area->Max     = UISize(430.0f, 600.0f);
                }
            }
            UILeaveVerticalLayout(VBox, Window);

            UIBuildLayoutGraph(LayoutGraph);

            ui_graph_node_handle Handle   = {};
            ui_graph_iterator    Iterator = UIBeginGraphIterator(LayoutGraph, &Window->FrameAllocator);
            while(UIGraphIteratorNext(&Iterator, &Handle))
            {
                ui_graph_node *Node = UIGetGraphNode(Handle.ID, LayoutGraph);
                assert(Node);
                assert(Node->CommandPointer);

                switch(Node->CommandType)
                {

                case UILayoutCommand_FixedSizing:    break;
                case UILayoutCommand_AxisBoxSizing:  break;
                case UILayoutCommand_PlaceMajorAxis: break;
                case UILayoutCommand_PlaceMinorAxis: break;

                }
            }
        }

    }

    if(Window)
    {
    }
}
