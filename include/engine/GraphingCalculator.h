// GraphingCalculator.h
#pragma once
#include <string>
#include <vector>
#include <functional>
#include <cmath>
#include <algorithm>
#include "imgui.h"

struct FunctionEntry {
    std::string expression = "sin(x)";
    ImVec4 color = ImVec4(1, 1, 0, 1);
    bool visible = true;
    std::vector<ImVec2> cachedPoints;
    bool needsRecache = true;
};

struct ViewState {
    double centerX = 0.0;
    double centerY = 0.0;
    double scaleX = 1.0;  // units per pixel
    double scaleY = 1.0;
    
    // Convert between world (math) coords and screen coords
    ImVec2 worldToScreen(double wx, double wy, ImVec2 canvasPos, ImVec2 canvasSize) const {
        float sx = canvasPos.x + (float)((wx - centerX) / scaleX) + canvasSize.x * 0.5f;
        float sy = canvasPos.y - (float)((wy - centerY) / scaleY) + canvasSize.y * 0.5f;
        return ImVec2(sx, sy);
    }
    
    ImVec2 screenToWorld(ImVec2 screen, ImVec2 canvasPos, ImVec2 canvasSize) const {
        double wx = (screen.x - canvasPos.x - canvasSize.x * 0.5) * scaleX + centerX;
        double wy = -(screen.y - canvasPos.y - canvasSize.y * 0.5) * scaleY + centerY;
        return ImVec2((float)wx, (float)wy);
    }
};