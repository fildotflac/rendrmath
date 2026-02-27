
#include "app/Application.h"
#include "engine/GraphWidget.h"
#include "imgui.h"

namespace MyApp
{
    GraphWidget graphWidget;

    void RenderUI()
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("DockSpace", nullptr, 
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | 
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoBringToFrontOnFocus);

        ImGui::PopStyleVar(3);

        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

        ImGui::End();

        graphWidget.render();
    }
}