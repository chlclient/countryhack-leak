#pragma once
#include "../../../../SDK/SDK.h"
#include "../../Render.h"
#include <ImGui/TextEditor.h>
#include <mutex>
#include <ImGui/imgui.h>

class CCustomUI
{
public:
    void Render();
    void RenderAimbot(int iTab);
    void RenderExploits();
    void RenderVisuals(int iTab);
    void RenderMisc();
    void RenderLogs(int iTab);
    void RenderSettings(int iTab);
};

ADD_FEATURE(CCustomUI, CustomUI);