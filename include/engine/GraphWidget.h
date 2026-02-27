// GraphWidget.h
#pragma once
#include "ExpressionParser.h"
#include "GraphingCalculator.h"
#include "imgui.h"
#include <vector>
#include <string>
#include <cmath>
#include <cstdio>
#include <algorithm>

class GraphWidget {
public:
    std::vector<FunctionEntry> functions;
    ViewState view;
    bool showGrid = true;
    bool showCrosshair = true;
    int samplesPerPixel = 2; // oversampling for smoother curves

    GraphWidget() {
        functions.push_back(FunctionEntry());
        view.centerX = view.centerY = 0;
        view.scaleX = view.scaleY = 0.02;
    }

    void render() {
        renderControlPanel();
        renderGraph();
    }

private:
    void renderControlPanel() {
        ImGui::Begin("Functions");

        for (int i = 0; i < (int)functions.size(); i++) {
            ImGui::PushID(i);
            
            ImGui::Checkbox("##vis", &functions[i].visible);
            ImGui::SameLine();
            ImGui::ColorEdit4("##color", &functions[i].color.x, 
                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
            ImGui::SameLine();
            
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 25);
            char exprBuffer[512];
            std::snprintf(exprBuffer, sizeof(exprBuffer), "%s", functions[i].expression.c_str());
            if (ImGui::InputText("##expr", exprBuffer, IM_ARRAYSIZE(exprBuffer))) {
                functions[i].expression = exprBuffer;
                functions[i].needsRecache = true;
            }
            
            ImGui::SameLine();
            if (ImGui::Button("X")) {
                functions.erase(functions.begin() + i);
                i--;
                ImGui::PopID();
                continue;
            }
            
            ImGui::PopID();
        }

        if (ImGui::Button("+ Add Function")) {
            FunctionEntry f;
            // Cycle through some default colors
            ImVec4 colors[] = {
                {1,1,0,1}, {0,1,1,1}, {1,0.5,0,1}, 
                {0.5,1,0,1}, {1,0,1,1}, {0.3,0.7,1,1}
            };
            f.color = colors[functions.size() % 6];
            functions.push_back(f);
        }

        ImGui::Separator();
        if (ImGui::Button("Reset View")) {
            view.centerX = view.centerY = 0;
            view.scaleX = view.scaleY = 0.02;
            invalidateAll();
        }
        ImGui::Checkbox("Grid", &showGrid);
        ImGui::SameLine();
        ImGui::Checkbox("Crosshair", &showCrosshair);

        ImGui::End();
    }

    void invalidateAll() {
        for (auto& f : functions) f.needsRecache = true;
    }

    // Compute nice grid spacing (1, 2, 5 multiples of powers of 10)
    double niceGridStep(double roughStep) {
        double pow10 = std::pow(10.0, std::floor(std::log10(roughStep)));
        double fraction = roughStep / pow10;
        
        if (fraction <= 1.0) return pow10;
        if (fraction <= 2.0) return 2.0 * pow10;
        if (fraction <= 5.0) return 5.0 * pow10;
        return 10.0 * pow10;
    }

    void renderGraph() {
        ImGui::Begin("Graph");

        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        ImVec2 canvasSize = ImGui::GetContentRegionAvail();
        
        if (canvasSize.x < 50 || canvasSize.y < 50) {
            ImGui::End();
            return;
        }

        // Create an invisible button to capture input
        ImGui::InvisibleButton("canvas", canvasSize,
            ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
        bool isHovered = ImGui::IsItemHovered();
        bool isActive  = ImGui::IsItemActive();

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // Background
        drawList->AddRectFilled(canvasPos, 
            ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
            IM_COL32(25, 25, 30, 255));
        
        // Clip to canvas
        drawList->PushClipRect(canvasPos, 
            ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), true);

        // ---- Handle Input ----
        handleInput(isHovered, isActive, canvasPos, canvasSize);

        // ---- Draw Grid ----
        if (showGrid) {
            drawGrid(drawList, canvasPos, canvasSize);
        }

        // ---- Draw Axes ----
        drawAxes(drawList, canvasPos, canvasSize);

        // ---- Draw Functions ----
        for (auto& func : functions) {
            if (!func.visible) continue;
            drawFunction(drawList, func, canvasPos, canvasSize);
        }

        // ---- Crosshair at mouse ----
        if (showCrosshair && isHovered) {
            drawCrosshair(drawList, canvasPos, canvasSize);
        }

        drawList->PopClipRect();
        ImGui::End();
    }

    void handleInput(bool isHovered, bool isActive, ImVec2 canvasPos, ImVec2 canvasSize) {
        ImGuiIO& io = ImGui::GetIO();

        // Pan with left mouse drag
        if (isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            ImVec2 delta = io.MouseDelta;
            view.centerX -= delta.x * view.scaleX;
            view.centerY += delta.y * view.scaleY;
            invalidateAll();
        }

        // Zoom with scroll wheel
        if (isHovered && io.MouseWheel != 0) {
            // Zoom toward mouse position
            ImVec2 mouseWorld = view.screenToWorld(io.MousePos, canvasPos, canvasSize);
            
            double zoomFactor = io.MouseWheel > 0 ? 0.85 : 1.0 / 0.85;
            
            view.scaleX *= zoomFactor;
            view.scaleY *= zoomFactor;

            // Clamp scale
            view.scaleX = std::clamp(view.scaleX, 1e-10, 1e10);
            view.scaleY = std::clamp(view.scaleY, 1e-10, 1e10);

            // Adjust center so mouse position stays fixed
            ImVec2 mouseWorldAfter = view.screenToWorld(io.MousePos, canvasPos, canvasSize);
            view.centerX -= (mouseWorldAfter.x - mouseWorld.x);
            view.centerY -= (mouseWorldAfter.y - mouseWorld.y);

            invalidateAll();
        }
    }

    void drawGrid(ImDrawList* drawList, ImVec2 canvasPos, ImVec2 canvasSize) {
        // Determine grid spacing (aim for ~80px between lines)
        double gridStepX = niceGridStep(view.scaleX * 80.0);
        double gridStepY = niceGridStep(view.scaleY * 80.0);

        ImVec2 worldTL = view.screenToWorld(canvasPos, canvasPos, canvasSize);
        ImVec2 worldBR = view.screenToWorld(
            ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), 
            canvasPos, canvasSize);

        float minWorldX = std::min(worldTL.x, worldBR.x);
        float maxWorldX = std::max(worldTL.x, worldBR.x);
        float minWorldY = std::min(worldTL.y, worldBR.y);
        float maxWorldY = std::max(worldTL.y, worldBR.y);

        ImU32 gridColor = IM_COL32(60, 60, 70, 255);
        ImU32 labelColor = IM_COL32(180, 180, 180, 255);

        // Vertical grid lines
        double startX = std::floor(minWorldX / gridStepX) * gridStepX;
        for (double wx = startX; wx <= maxWorldX; wx += gridStepX) {
            ImVec2 top = view.worldToScreen(wx, maxWorldY, canvasPos, canvasSize);
            ImVec2 bot = view.worldToScreen(wx, minWorldY, canvasPos, canvasSize);
            drawList->AddLine(top, bot, gridColor);
            
            // Label
            char buf[32];
            snprintf(buf, sizeof(buf), "%.6g", wx);
            ImVec2 origin = view.worldToScreen(wx, 0, canvasPos, canvasSize);
            origin.y = std::clamp(origin.y, canvasPos.y, canvasPos.y + canvasSize.y - 15.0f);
            drawList->AddText(ImVec2(top.x + 3, origin.y + 2), labelColor, buf);
        }

        // Horizontal grid lines
        double startY = std::floor(minWorldY / gridStepY) * gridStepY;
        for (double wy = startY; wy <= maxWorldY; wy += gridStepY) {
            ImVec2 left  = view.worldToScreen(minWorldX, wy, canvasPos, canvasSize);
            ImVec2 right = view.worldToScreen(maxWorldX, wy, canvasPos, canvasSize);
            drawList->AddLine(left, right, gridColor);
            
            char buf[32];
            snprintf(buf, sizeof(buf), "%.6g", wy);
            ImVec2 origin = view.worldToScreen(0, wy, canvasPos, canvasSize);
            origin.x = std::clamp(origin.x, canvasPos.x, canvasPos.x + canvasSize.x - 40.0f);
            drawList->AddText(ImVec2(origin.x + 3, left.y + 2), labelColor, buf);
        }
    }

    void drawAxes(ImDrawList* drawList, ImVec2 canvasPos, ImVec2 canvasSize) {
        ImU32 axisColor = IM_COL32(200, 200, 200, 255);

        ImVec2 worldTL = view.screenToWorld(canvasPos, canvasPos, canvasSize);
        ImVec2 worldBR = view.screenToWorld(
            ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), 
            canvasPos, canvasSize);

        // X axis (y=0)
        ImVec2 xLeft  = view.worldToScreen(worldTL.x, 0, canvasPos, canvasSize);
        ImVec2 xRight = view.worldToScreen(worldBR.x, 0, canvasPos, canvasSize);
        drawList->AddLine(xLeft, xRight, axisColor, 2.0f);

        // Y axis (x=0)
        ImVec2 yTop = view.worldToScreen(0, worldTL.y, canvasPos, canvasSize);
        ImVec2 yBot = view.worldToScreen(0, worldBR.y, canvasPos, canvasSize);
        drawList->AddLine(yTop, yBot, axisColor, 2.0f);
    }

    void drawFunction(ImDrawList* drawList, FunctionEntry& func, 
                       ImVec2 canvasPos, ImVec2 canvasSize) {
        int numSamples = (int)canvasSize.x * samplesPerPixel;
        if (numSamples < 2) return;

        ImU32 color = ImGui::ColorConvertFloat4ToU32(func.color);

        // Sample the function across visible x range
        ImVec2 worldTL = view.screenToWorld(canvasPos, canvasPos, canvasSize);
        ImVec2 worldBR = view.screenToWorld(
            ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), 
            canvasPos, canvasSize);

        double xMin = worldTL.x;
        double xMax = worldBR.x;
        double dx = (xMax - xMin) / numSamples;

        // Build polyline segments (break on NaN/Inf for discontinuities)
        std::vector<ImVec2> segment;
        segment.reserve(numSamples);

        for (int i = 0; i <= numSamples; i++) {
            double x = xMin + i * dx;
            double y = ExpressionParser::evaluate(func.expression, x);

            if (std::isfinite(y)) {
                ImVec2 screen = view.worldToScreen(x, y, canvasPos, canvasSize);
                
                // Clamp extreme values to avoid rendering issues
                // but still allow lines to go off-screen smoothly
                if (screen.y > canvasPos.y - 5000 && screen.y < canvasPos.y + canvasSize.y + 5000) {
                    segment.push_back(screen);
                } else {
                    // Way off screen - flush segment
                    if (segment.size() >= 2) {
                        drawList->AddPolyline(segment.data(), (int)segment.size(), 
                            color, ImDrawFlags_None, 2.0f);
                    }
                    segment.clear();
                }
            } else {
                // Discontinuity - flush segment
                if (segment.size() >= 2) {
                    drawList->AddPolyline(segment.data(), (int)segment.size(), 
                        color, ImDrawFlags_None, 2.0f);
                }
                segment.clear();
            }
        }

        // Flush remaining segment
        if (segment.size() >= 2) {
            drawList->AddPolyline(segment.data(), (int)segment.size(), 
                color, ImDrawFlags_None, 2.0f);
        }
    }

    void drawCrosshair(ImDrawList* drawList, ImVec2 canvasPos, ImVec2 canvasSize) {
        ImGuiIO& io = ImGui::GetIO();
        ImVec2 mouseWorld = view.screenToWorld(io.MousePos, canvasPos, canvasSize);
        
        ImU32 crossColor = IM_COL32(255, 255, 255, 80);
        drawList->AddLine(
            ImVec2(canvasPos.x, io.MousePos.y),
            ImVec2(canvasPos.x + canvasSize.x, io.MousePos.y), crossColor);
        drawList->AddLine(
            ImVec2(io.MousePos.x, canvasPos.y),
            ImVec2(io.MousePos.x, canvasPos.y + canvasSize.y), crossColor);

        // Coordinate label
        char buf[64];
        snprintf(buf, sizeof(buf), "(%.4f, %.4f)", mouseWorld.x, mouseWorld.y);
        drawList->AddText(ImVec2(io.MousePos.x + 15, io.MousePos.y - 20),
            IM_COL32(255, 255, 255, 200), buf);
    }
};