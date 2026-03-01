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
    std::vector<PointEntry> points;
    ViewState view;
    bool showGrid = true;
    bool showCrosshair = true;
    int samplesPerPixel = 1; // 1 = one sample per pixel (disable oversampling for performance)

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
        ImGui::Begin("Expressions");

        // Display Functions
        for (int i = 0; i < (int)functions.size(); i++) {
            ImGui::PushID(i * 10000);  // Avoid ID collision with points
            
            ImGui::Checkbox("##vis", &functions[i].visible);
            ImGui::SameLine();
            ImGui::ColorEdit4("##color", &functions[i].color.x, 
                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
            ImGui::SameLine();
            
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 25);
            char exprBuffer[512];
            std::snprintf(exprBuffer, sizeof(exprBuffer), "%s", functions[i].expression.c_str());
            if (ImGui::InputText("##expr", exprBuffer, IM_ARRAYSIZE(exprBuffer))) {
                // Check if user changed it to point format
                std::string label;
                ImVec2 coords;
                if (tryParsePoint(exprBuffer, label, coords)) {
                    // Convert function to point
                    PointEntry p;
                    p.label = label;
                    p.coords = coords;
                    p.color = functions[i].color;
                    points.push_back(p);
                    functions.erase(functions.begin() + i);
                    ImGui::PopID();
                    continue;  // Skip the rest of this iteration
                } else {
                    // It's a function
                    functions[i].expression = ExpressionParser::toDisplayExpression(exprBuffer);
                    functions[i].needsRecache = true;
                }
            }
            
            ImGui::SameLine();
            if (ImGui::Button("X")) {
                functions.erase(functions.begin() + i);
                ImGui::PopID();
                continue;
            }
            
            ImGui::PopID();
        }

        // Display Points
        for (int i = 0; i < (int)points.size(); i++) {
            ImGui::PushID(i + 50000);  // Avoid ID collision with functions
            
            ImGui::Checkbox("##vis", &points[i].visible);
            ImGui::SameLine();
            ImGui::ColorEdit4("##color", &points[i].color.x, 
                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
            ImGui::SameLine();
            
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 25);
            char pointBuffer[512];
            std::snprintf(pointBuffer, sizeof(pointBuffer), "%s=(%.4g,%.4g)", 
                         points[i].label.c_str(), points[i].coords.x, points[i].coords.y);
            if (ImGui::InputText("##point", pointBuffer, IM_ARRAYSIZE(pointBuffer))) {
                // Try to parse as point
                std::string label;
                ImVec2 coords;
                if (tryParsePoint(pointBuffer, label, coords)) {
                    points[i].label = label;
                    points[i].coords = coords;
                }
                // If parsing fails, just keep the current values (don't convert to function)
            }
            
            ImGui::SameLine();
            if (ImGui::Button("X")) {
                points.erase(points.begin() + i);
                ImGui::PopID();
                continue;
            }
            
            ImGui::PopID();
        }

        ImGui::Separator();

        // Add new entry
        static char newEntryBuffer[512] = "";
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 80);
        ImGui::InputText("##new", newEntryBuffer, IM_ARRAYSIZE(newEntryBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
        
        ImGui::SameLine();
        if (ImGui::Button("Add") || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
            if (newEntryBuffer[0] != '\0') {
                std::string input(newEntryBuffer);
                std::string label;
                ImVec2 coords;
                std::string tangentExpr;
                
                // Check for Tangent(expr, point)
                if (tryParseTangent(input, tangentExpr)) {
                    FunctionEntry f;
                    f.expression = tangentExpr;
                    ImVec4 colors[] = {
                        {1,1,0,1}, {0,1,1,1}, {1,0.5,0,1}, 
                        {0.5,1,0,1}, {1,0,1,1}, {0.3,0.7,1,1}
                    };
                    f.color = colors[(functions.size() + points.size()) % 6];
                    functions.push_back(f);
                }
                // Check for P(x, expr) - point on function
                else if (tryParsePointCreator(input, label, coords)) {
                    PointEntry p;
                    // Generate a unique label P1, P2, etc.
                    int pointNum = 1;
                    for (const auto& existing : points) {
                        if (existing.label.size() == 2 && existing.label[0] == 'P' && std::isdigit(existing.label[1])) {
                            int num = std::stoi(existing.label.substr(1));
                            pointNum = std::max(pointNum, num + 1);
                        }
                    }
                    char buf[16];
                    snprintf(buf, sizeof(buf), "P%d", pointNum);
                    p.label = buf;
                    p.coords = coords;
                    ImVec4 colors[] = {
                        {1,1,0,1}, {0,1,1,1}, {1,0.5,0,1}, 
                        {0.5,1,0,1}, {1,0,1,1}, {0.3,0.7,1,1}
                    };
                    p.color = colors[(functions.size() + points.size()) % 6];
                    points.push_back(p);
                }
                // Check for regular point format Label=(x,y)
                else if (tryParsePoint(input, label, coords)) {
                    PointEntry p;
                    p.label = label;
                    p.coords = coords;
                    ImVec4 colors[] = {
                        {1,1,0,1}, {0,1,1,1}, {1,0.5,0,1}, 
                        {0.5,1,0,1}, {1,0,1,1}, {0.3,0.7,1,1}
                    };
                    p.color = colors[(functions.size() + points.size()) % 6];
                    points.push_back(p);
                }
                // Add as function
                else {
                    FunctionEntry f;
                    f.expression = ExpressionParser::toDisplayExpression(input);
                    ImVec4 colors[] = {
                        {1,1,0,1}, {0,1,1,1}, {1,0.5,0,1}, 
                        {0.5,1,0,1}, {1,0,1,1}, {0.3,0.7,1,1}
                    };
                    f.color = colors[(functions.size() + points.size()) % 6];
                    functions.push_back(f);
                }
                newEntryBuffer[0] = '\0';
            }
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

    // Calculate numerical derivative of expression at x
    double numericalDerivative(const std::string& expr, double x, double h = 1e-5) {
        double f_plus = ExpressionParser::evaluate(expr, x + h);
        double f_minus = ExpressionParser::evaluate(expr, x - h);
        if (!std::isfinite(f_plus) || !std::isfinite(f_minus)) return 0.0;
        return (f_plus - f_minus) / (2.0 * h);
    }

    // Check if string starts with prefix
    bool startsWith(const std::string& str, const std::string& prefix) {
        return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;
    }

    // Parse Tangent(expr, pointLabel) -> creates tangent line at point
    bool tryParseTangent(const std::string& input, std::string& outFuncExpr) {
        if (!startsWith(input, "Tangent(")) return false;
        
        // Find matching closing paren and comma
        int parenCount = 0;
        int commaPos = -1;
        for (size_t i = 8; i < input.size(); i++) {
            if (input[i] == '(') parenCount++;
            else if (input[i] == ')') {
                if (parenCount == 0) {
                    if (commaPos == -1) return false;
                    
                    std::string funcExpr = input.substr(8, commaPos - 8);
                    std::string pointName = input.substr(commaPos + 1, i - commaPos - 1);
                    
                    // Trim whitespace
                    while (!funcExpr.empty() && std::isspace(funcExpr.back())) funcExpr.pop_back();
                    while (!pointName.empty() && std::isspace(pointName.back())) pointName.pop_back();
                    size_t start = funcExpr.find_first_not_of(" \t");
                    if (start != std::string::npos) funcExpr = funcExpr.substr(start);
                    start = pointName.find_first_not_of(" \t");
                    if (start != std::string::npos) pointName = pointName.substr(start);
                    
                    // Find the point
                    PointEntry* point = nullptr;
                    for (auto& p : points) {
                        if (p.label == pointName) {
                            point = &p;
                            break;
                        }
                    }
                    if (!point) return false;
                    
                    // Calculate tangent line
                    double x0 = point->coords.x;
                    double y0 = point->coords.y;
                    double m = numericalDerivative(funcExpr, x0);
                    
                    if (!std::isfinite(m)) return false;
                    
                    // Tangent line: y = m(x - x0) + y0
                    char buf[512];
                    snprintf(buf, sizeof(buf), "%.10g*(x-%.10g)+%.10g", m, x0, y0);
                    outFuncExpr = buf;
                    return true;
                }
                parenCount--;
            } else if (input[i] == ',' && parenCount == 0) {
                commaPos = i;
            }
        }
        return false;
    }

    // Parse P(x, expr) -> creates point on function
    bool tryParsePointCreator(const std::string& input, std::string& outLabel, ImVec2& outCoords) {
        if (!startsWith(input, "P(")) return false;
        
        // Find matching closing paren and comma
        int parenCount = 0;
        int commaPos = -1;
        for (size_t i = 2; i < input.size(); i++) {
            if (input[i] == '(') parenCount++;
            else if (input[i] == ')') {
                if (parenCount == 0) {
                    if (commaPos == -1) return false;
                    
                    std::string xStr = input.substr(2, commaPos - 2);
                    std::string yExpr = input.substr(commaPos + 1, i - commaPos - 1);
                    
                    // Trim whitespace
                    while (!xStr.empty() && std::isspace(xStr.back())) xStr.pop_back();
                    while (!yExpr.empty() && std::isspace(yExpr.back())) yExpr.pop_back();
                    size_t start = xStr.find_first_not_of(" \t");
                    if (start != std::string::npos) xStr = xStr.substr(start);
                    start = yExpr.find_first_not_of(" \t");
                    if (start != std::string::npos) yExpr = yExpr.substr(start);
                    
                    try {
                        double x = std::stod(xStr);
                        double y = ExpressionParser::evaluate(yExpr, x);
                        if (!std::isfinite(y)) return false;
                        
                        outLabel = "P";
                        outCoords = ImVec2((float)x, (float)y);
                        return true;
                    } catch (...) {
                        return false;
                    }
                }
                parenCount--;
            } else if (input[i] == ',' && parenCount == 0) {
                commaPos = i;
            }
        }
        return false;
    }

    // Try to parse point format: "Label=(x,y)" or "Label=(x;y)"
    // Returns true if successfully parsed, false otherwise
    bool tryParsePoint(const std::string& input, std::string& outLabel, ImVec2& outCoords) {
        // Find '='
        size_t eqPos = input.find('=');
        if (eqPos == std::string::npos || eqPos == 0) return false;
        
        outLabel = input.substr(0, eqPos);
        // Trim whitespace from label
        while (!outLabel.empty() && std::isspace(outLabel.back())) outLabel.pop_back();
        
        std::string rest = input.substr(eqPos + 1);
        // Trim leading whitespace
        size_t start = 0;
        while (start < rest.size() && std::isspace(rest[start])) start++;
        rest = rest.substr(start);
        
        // Check for '(' and ')'
        if (rest.empty() || rest.front() != '(' || rest.back() != ')') return false;
        
        rest = rest.substr(1, rest.size() - 2);
        
        // Find separator ',' or ';'
        size_t sepPos = rest.find(',');
        if (sepPos == std::string::npos) {
            sepPos = rest.find(';');
        }
        if (sepPos == std::string::npos) return false;
        
        try {
            float x = std::stof(rest.substr(0, sepPos));
            float y = std::stof(rest.substr(sepPos + 1));
            outCoords = ImVec2(x, y);
            return true;
        } catch (...) {
            return false;
        }
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

        // ---- Draw Points ----
        for (auto& point : points) {
            if (!point.visible) continue;
            drawPoint(drawList, point, canvasPos, canvasSize);
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
            // Don't invalidate - let drawFunction handle cache intelligently
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
        }

        // Place point with right-click
        if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImVec2 mouseWorld = view.screenToWorld(io.MousePos, canvasPos, canvasSize);
            
            PointEntry p;
            p.coords = mouseWorld;
            ImVec4 colors[] = {
                {1,1,0,1}, {0,1,1,1}, {1,0.5,0,1}, 
                {0.5,1,0,1}, {1,0,1,1}, {0.3,0.7,1,1}
            };
            p.color = colors[points.size() % 6];
            char buf[32];
            snprintf(buf, sizeof(buf), "P%d", (int)points.size() + 1);
            p.label = buf;
            points.push_back(p);
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
        // Use higher sampling for better high-frequency function capture
        int numSamples = (int)canvasSize.x * samplesPerPixel * 4;
        if (numSamples < 2) return;

        ImU32 color = ImGui::ColorConvertFloat4ToU32(func.color);

        // Get visible world range
        ImVec2 worldTL = view.screenToWorld(canvasPos, canvasPos, canvasSize);
        ImVec2 worldBR = view.screenToWorld(
            ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), 
            canvasPos, canvasSize);

        double visibleXMin = std::min(worldTL.x, worldBR.x);
        double visibleXMax = std::max(worldTL.x, worldBR.x);
        double visibleWidth = visibleXMax - visibleXMin;

        // Create cache margin: cache 3x the visible width for smooth panning
        const double CACHE_MARGIN = 1.5;  // 3x total (each side gets 1.5x)
        double cachedXMin = visibleXMin - visibleWidth * CACHE_MARGIN;
        double cachedXMax = visibleXMax + visibleWidth * CACHE_MARGIN;

        // Check if we need to recalculate:
        // 1. Expression changed
        // 2. Zoom level changed significantly
        // 3. Pan moved outside cached region
        bool zoomChanged = std::abs(func.cachedZoom - view.scaleX) / func.cachedZoom > 0.05;  // 5% change threshold
        bool panOutsideCache = (visibleXMin < func.cachedXMin || visibleXMax > func.cachedXMax);
        
        if (func.needsRecache || zoomChanged || panOutsideCache) {
            // Recalculate function cache with expanded bounds
            func.cachedPoints.clear();
            func.cachedXMin = cachedXMin;
            func.cachedXMax = cachedXMax;
            func.cachedZoom = view.scaleX;

            double dx = (cachedXMax - cachedXMin) / numSamples;

            std::vector<ImVec2> segment;
            segment.reserve(numSamples);

            for (int i = 0; i <= numSamples; i++) {
                double x = cachedXMin + i * dx;
                double y = ExpressionParser::evaluate(func.expression, x);

                if (std::isfinite(y)) {
                    // Store in world coordinates (not screen)
                    // Clamp y to prevent memory issues, but keep valid range large
                    if (y > -1e8 && y < 1e8) {
                        segment.push_back(ImVec2((float)x, (float)y));
                    } else {
                        if (segment.size() >= 2) {
                            func.cachedPoints.push_back(segment);
                        }
                        segment.clear();
                    }
                } else {
                    // Discontinuity
                    if (segment.size() >= 2) {
                        func.cachedPoints.push_back(segment);
                    }
                    segment.clear();
                }
            }

            if (segment.size() >= 2) {
                func.cachedPoints.push_back(segment);
            }

            func.needsRecache = false;
        }

        // Transform world-space cache to screen and draw
        for (const auto& worldSegment : func.cachedPoints) {
            std::vector<ImVec2> screenSegment;
            screenSegment.reserve(worldSegment.size());

            for (const auto& worldPoint : worldSegment) {
                ImVec2 screenPoint = view.worldToScreen(worldPoint.x, worldPoint.y, canvasPos, canvasSize);
                
                // Cull if way off screen
                if (screenPoint.y > canvasPos.y - 5000 && screenPoint.y < canvasPos.y + canvasSize.y + 5000) {
                    // Deduplicate: only add if it's different from the last point (prevents rendering glitches)
                    if (screenSegment.empty() || 
                        std::abs(screenPoint.x - screenSegment.back().x) >= 0.5f ||
                        std::abs(screenPoint.y - screenSegment.back().y) >= 0.5f) {
                        screenSegment.push_back(screenPoint);
                    }
                }
            }

            if (screenSegment.size() >= 2) {
                drawList->AddPolyline(screenSegment.data(), (int)screenSegment.size(), 
                    color, ImDrawFlags_None, 2.0f);
            }
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

    void drawPoint(ImDrawList* drawList, PointEntry& point, ImVec2 canvasPos, ImVec2 canvasSize) {
        // Convert world coords to screen coords
        ImVec2 screenPos = view.worldToScreen(point.coords.x, point.coords.y, canvasPos, canvasSize);
        
        // Check if point is visible on screen
        if (screenPos.x < canvasPos.x - 10 || screenPos.x > canvasPos.x + canvasSize.x + 10 ||
            screenPos.y < canvasPos.y - 10 || screenPos.y > canvasPos.y + canvasSize.y + 10) {
            return;
        }

        ImU32 color = ImGui::ColorConvertFloat4ToU32(point.color);
        
        // Draw point as a filled circle
        const float POINT_RADIUS = 5.0f;
        drawList->AddCircleFilled(screenPos, POINT_RADIUS, color, 12);
        
        // Draw point outline
        drawList->AddCircle(screenPos, POINT_RADIUS, IM_COL32(255, 255, 255, 255), 12, 1.5f);
        
        // Draw label
        char labelBuf[64];
        snprintf(labelBuf, sizeof(labelBuf), "%s (%.3f, %.3f)", point.label.c_str(), point.coords.x, point.coords.y);
        drawList->AddText(ImVec2(screenPos.x + POINT_RADIUS + 5, screenPos.y - 8), color, labelBuf);
    }
};