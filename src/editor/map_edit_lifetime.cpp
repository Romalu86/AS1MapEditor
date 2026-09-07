#include "mapedit/runtime.hpp"

MAP_EDIT::MAP_EDIT(HINSTANCE__* instance,HINSTANCE__* prev,const STRING* command_line,int sw,GRAPH_INIT* init)
    : MAP(instance,prev,command_line,sw,init), selectedSprites(), undo(), editFileName()
{
    if (!IsInitSuccess()) return;

    insertZ=0.0f; spriteType=2;
    hToolBar=0; hControlPanel=0;
    optDrawMap=0; optDrawGroup=0; optTacticMode=0;
    optGround0=1; optSnap=0; optChessSnap=0;
    optRandomDir=0; optDelete=0; optSortVid=0;
    optAirBrush=0; optAirBrushSize=10; optAirBrushDensity=20;
    optShiftSnapX=0; optShiftSnapY=0;
    optDrawMap=Registry->GetInt(STRING("DrawMap"),(int)optDrawMap);
    optBigStepZ=(unsigned int)Registry->GetInt(STRING("BigStepZ"),90);
    optControlPanel=0;
    curRegion=0;
    savedVid=0;

    // 34 initializers are present in original source/codegen; CreateToolbarEx receives 33.
    // Preserve that asymmetry exactly instead of "fixing" it.
    TBBUTTON_OLD buttons[34]={
        {0,45087,4,0,{0,0},0,-1}, {1,40001,4,0,{0,0},0,-1}, {2,40002,4,0,{0,0},0,-1},
        {0,0,4,1,{0,0},0,-1}, {3,40078,4,0,{0,0},0,-1}, {4,45092,4,0,{0,0},0,-1},
        {0,0,4,1,{0,0},0,-1}, {5,40025,4,2,{0,0},0,-1}, {6,40034,4,2,{0,0},0,-1},
        {7,40053,4,2,{0,0},0,-1}, {8,45085,5,2,{0,0},0,-1}, {9,45097,4,2,{0,0},0,-1},
        {10,45099,4,2,{0,0},0,-1}, {0,0,4,1,{0,0},0,-1}, {11,40047,4,6,{0,0},0,-1},
        {12,40048,5,6,{0,0},0,-1}, {13,40049,4,6,{0,0},0,-1}, {14,40050,4,6,{0,0},0,-1},
        {15,45091,4,6,{0,0},0,-1}, {16,40051,4,6,{0,0},0,-1}, {17,45090,4,6,{0,0},0,-1},
        {0,0,4,1,{0,0},0,-1}, {18,40032,4,2,{0,0},0,-1}, {0,0,4,1,{0,0},0,-1},
        {19,40067,4,2,{0,0},0,-1}, {0,0,4,1,{0,0},0,-1}, {20,40073,4,0,{0,0},0,-1},
        {0,0,4,1,{0,0},0,-1}, {21,40061,4,0,{0,0},0,-1}, {22,40062,4,0,{0,0},0,-1},
        {0,0,4,1,{0,0},0,-1}, {23,45098,4,0,{0,0},0,-1}, {24,41107,4,0,{0,0},0,-1},
        {0,0,4,1,{0,0},0,-1}
    };

    hToolBar=CreateToolbarEx(m_hWnd,0x50800100u,0x73,25,instance,0x73,buttons,33,0,0,0,0,sizeof(TBBUTTON_OLD));
    RECT_OLD rect;
    GetWindowRect(hToolBar,&rect);
    Graph->SetViewPort(Graph->ViewXMin(),Graph->ViewYMin()+float(rect.bottom-rect.top),Graph->ViewXMax(),Graph->ViewYMax());

    SetControlPanel(Registry->GetInt(STRING("ControlPanel"),0));
    const int no_hide=Registry->GetInt(STRING("NoHideVids"),0);
    if (no_hide) {
        unsigned int hide[2048];
        Registry->GetData(STRING("HideVids"),hide,(unsigned long)(no_hide*4));
        for (int i=0;i<no_hide;i++)
            if (m_vids[hide[i]]) m_vids[hide[i]]->SetPropHide(1);
    }
    Load(STRING(m_startupLoad));
}

MAP_EDIT::~MAP_EDIT()
{
    if (!IsInitSuccess()) return;

    Registry->SetInt(STRING("DrawMap"),(int)optDrawMap);
    Registry->SetInt(STRING("BigStepZ"),(int)optBigStepZ);
    Registry->SetInt(STRING("ControlPanel"),(int)optControlPanel);

    unsigned int no_hide=0;
    unsigned int hide[2048];
    for (int i=0;i<m_noVid;i++) {
        if (m_vids[i] && m_vids[i]->PropHide()) hide[no_hide++]=(unsigned int)i;
    }
    Registry->SetInt(STRING("NoHideVids"),(int)no_hide);
    Registry->SetData(STRING("HideVids"),hide,no_hide*4u);
    Release();
}
