
#include "app/Application.h"
#include "imgui.h"

namespace MyApp
{
    void RenderUI()
    {
        ImGui::Begin("Hello, world!");
        ImGui::Text("This is some useful text.");
        ImGui::End();
    }
}