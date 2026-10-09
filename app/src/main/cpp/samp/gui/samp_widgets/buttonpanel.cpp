#include "../../main.h"
#include "../gui.h"
#include "../../game/game.h"
#include "../../net/netgame.h"
#include "../../net/localplayer.h"
#include "../../net/netgame.h"
#include "../../vendor/imgui/imgui.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <ctime>
#include <string>

extern UI* pUI;
extern CNetGame* pNetGame;
extern CGame* pGame;

bool bNeedEnterVehicle = false;
bool OpenButton = false;
int Tab = 0;

namespace {
constexpr int MODE_CLOSED = 0;
constexpr int MODE_WHEEL = 1;
constexpr int MODE_PHONE = 2;
constexpr int TARGET_NONE = 0;
constexpr int TARGET_PANEL_BUTTON = 1;
constexpr int TARGET_PHONE_BUTTON = 2;
constexpr int TARGET_MODAL = 3;
constexpr float PI_F = 3.14159265358979323846f;

struct HudRect {
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;

    bool contains(float x, float y) const {
        return x >= left && x <= right && y >= top && y <= bottom;
    }
};

struct ActionRects {
    HudRect panel;
    HudRect phone;
};

struct WheelLayout {
    ImVec2 center;
    float radius = 0.0f;
    float itemRadius = 0.0f;
    float itemDistance = 0.0f;
    ImVec2 itemCenters[4];
    HudRect closeButton;
};

struct PhoneLayout {
    HudRect frame;
    HudRect screen;
    HudRect closeButton;
    HudRect swipeZone;
    HudRect homeIndicator;
    HudRect appTiles[10];
    float scale = 1.0f;
};

const char* const WHEEL_LABELS[4] = {"Inventory", "Property", "Animasi", "Kendaraan"};
const char* const PHONE_APP_LABELS[10] = {
    "Inventory", "Property", "Animasi", "Kontak", "Pesan", "Pengaturan",
    "Telepon", "Kamera", "Peta", "Musik"
};

bool Hit(const HudRect& r, float x, float y) {
    return r.contains(x, y);
}

void AddCenteredText(ImDrawList* draw, const char* text, float centerX, float top,
                     ImU32 color, float size) {
    if (!draw || !text) return;
    ImFont* font = ImGui::GetFont();
    if (!font) return;
    size = std::max(7.0f, size);
    const ImVec2 textSize = font->CalcTextSizeA(size, FLT_MAX, 0.0f, text);
    draw->AddText(font, size, ImVec2(centerX - textSize.x * 0.5f, top), color, text);
}

void DrawPanelGlyph(ImDrawList* draw, const ImVec2& center, float s, ImU32 color) {
    const float r = 8.0f * s;
    draw->AddCircle(center, r, color, 32, 1.8f * s);
    draw->AddCircleFilled(center, 2.0f * s, color, 16);
    const ImVec2 spokes[4] = {
        ImVec2(center.x, center.y - r - 3.0f * s),
        ImVec2(center.x + r + 3.0f * s, center.y),
        ImVec2(center.x, center.y + r + 3.0f * s),
        ImVec2(center.x - r - 3.0f * s, center.y)
    };
    const ImVec2 ends[4] = {
        ImVec2(center.x, center.y - r + 1.0f * s),
        ImVec2(center.x + r - 1.0f * s, center.y),
        ImVec2(center.x, center.y + r - 1.0f * s),
        ImVec2(center.x - r + 1.0f * s, center.y)
    };
    for (int i = 0; i < 4; ++i) draw->AddLine(spokes[i], ends[i], color, 1.5f * s);
}

void DrawPhoneGlyph(ImDrawList* draw, const ImVec2& center, float s, ImU32 color) {
    const ImVec2 a(center.x - 8.0f * s, center.y - 14.0f * s);
    const ImVec2 b(center.x + 8.0f * s, center.y + 14.0f * s);
    draw->AddRect(a, b, color, 4.0f * s, 0, 1.8f * s);
    draw->AddLine(ImVec2(center.x - 3.0f * s, center.y - 9.0f * s),
                  ImVec2(center.x + 3.0f * s, center.y - 9.0f * s), color, 1.2f * s);
    draw->AddCircleFilled(ImVec2(center.x, center.y + 9.5f * s), 1.2f * s, color, 12);
}

void DrawWheelGlyph(ImDrawList* draw, int item, const ImVec2& c, float s, ImU32 color) {
    switch (item) {
        case 0: { // inventory bag
            draw->AddRect(ImVec2(c.x - 12*s, c.y - 4*s), ImVec2(c.x + 12*s, c.y + 15*s), color, 3*s, 0, 1.8f*s);
            draw->AddLine(ImVec2(c.x - 6*s, c.y - 4*s), ImVec2(c.x - 4*s, c.y - 11*s), color, 1.8f*s);
            draw->AddLine(ImVec2(c.x - 4*s, c.y - 11*s), ImVec2(c.x + 4*s, c.y - 11*s), color, 1.8f*s);
            draw->AddLine(ImVec2(c.x + 4*s, c.y - 11*s), ImVec2(c.x + 6*s, c.y - 4*s), color, 1.8f*s);
            draw->AddLine(ImVec2(c.x - 4*s, c.y + 2*s), ImVec2(c.x + 4*s, c.y + 2*s), color, 1.4f*s);
            break;
        }
        case 1: { // property
            draw->AddLine(ImVec2(c.x - 14*s, c.y - 1*s), ImVec2(c.x, c.y - 13*s), color, 2*s);
            draw->AddLine(ImVec2(c.x, c.y - 13*s), ImVec2(c.x + 14*s, c.y - 1*s), color, 2*s);
            draw->AddRect(ImVec2(c.x - 10*s, c.y - 1*s), ImVec2(c.x + 10*s, c.y + 14*s), color, 1*s, 0, 1.8f*s);
            draw->AddRectFilled(ImVec2(c.x - 3*s, c.y + 5*s), ImVec2(c.x + 3*s, c.y + 14*s), color, 1*s);
            break;
        }
        case 2: { // animation figure
            draw->AddCircle(c, 4.0f*s, color, 20, 1.8f*s);
            draw->AddLine(ImVec2(c.x, c.y + 4*s), ImVec2(c.x, c.y + 15*s), color, 1.8f*s);
            draw->AddLine(ImVec2(c.x, c.y + 7*s), ImVec2(c.x - 10*s, c.y + 12*s), color, 1.8f*s);
            draw->AddLine(ImVec2(c.x, c.y + 7*s), ImVec2(c.x + 10*s, c.y + 2*s), color, 1.8f*s);
            draw->AddLine(ImVec2(c.x, c.y + 15*s), ImVec2(c.x - 8*s, c.y + 24*s), color, 1.8f*s);
            draw->AddLine(ImVec2(c.x, c.y + 15*s), ImVec2(c.x + 9*s, c.y + 22*s), color, 1.8f*s);
            break;
        }
        default: { // vehicle
            draw->AddLine(ImVec2(c.x - 14*s, c.y + 5*s), ImVec2(c.x - 10*s, c.y - 5*s), color, 1.8f*s);
            draw->AddLine(ImVec2(c.x - 10*s, c.y - 5*s), ImVec2(c.x + 8*s, c.y - 5*s), color, 1.8f*s);
            draw->AddLine(ImVec2(c.x + 8*s, c.y - 5*s), ImVec2(c.x + 14*s, c.y + 5*s), color, 1.8f*s);
            draw->AddRect(ImVec2(c.x - 15*s, c.y + 4*s), ImVec2(c.x + 15*s, c.y + 13*s), color, 2*s, 0, 1.8f*s);
            draw->AddCircleFilled(ImVec2(c.x - 9*s, c.y + 14*s), 3*s, color, 16);
            draw->AddCircleFilled(ImVec2(c.x + 9*s, c.y + 14*s), 3*s, color, 16);
            break;
        }
    }
}

void DrawPhoneAppGlyph(ImDrawList* draw, int item, const ImVec2& c, float s, ImU32 color) {
    switch(item){
        case 0: DrawWheelGlyph(draw,0,c,s,color); break;
        case 1: DrawWheelGlyph(draw,1,c,s,color); break;
        case 2: DrawWheelGlyph(draw,2,c,s,color); break;
        case 3:
            draw->AddCircle(ImVec2(c.x,c.y-5*s),5*s,color,24,1.8f*s);
            draw->AddLine(ImVec2(c.x-10*s,c.y+9*s),ImVec2(c.x-8*s,c.y+2*s),color,1.8f*s);
            draw->AddLine(ImVec2(c.x-8*s,c.y+2*s),ImVec2(c.x-3*s,c.y-1*s),color,1.8f*s);
            draw->AddLine(ImVec2(c.x-3*s,c.y-1*s),ImVec2(c.x+3*s,c.y-1*s),color,1.8f*s);
            draw->AddLine(ImVec2(c.x+3*s,c.y-1*s),ImVec2(c.x+8*s,c.y+2*s),color,1.8f*s);
            draw->AddLine(ImVec2(c.x+8*s,c.y+2*s),ImVec2(c.x+10*s,c.y+9*s),color,1.8f*s);
            break;
        case 4:
            draw->AddRect(ImVec2(c.x-12*s,c.y-9*s),ImVec2(c.x+12*s,c.y+7*s),color,4*s,0,1.8f*s);
            draw->AddLine(ImVec2(c.x-6*s,c.y+7*s),ImVec2(c.x-10*s,c.y+12*s),color,1.8f*s);
            draw->AddLine(ImVec2(c.x-10*s,c.y+12*s),ImVec2(c.x+1*s,c.y+7*s),color,1.8f*s);
            draw->AddCircleFilled(ImVec2(c.x-5*s,c.y-1*s),1.2f*s,color,12);
            draw->AddCircleFilled(ImVec2(c.x,c.y-1*s),1.2f*s,color,12);
            draw->AddCircleFilled(ImVec2(c.x+5*s,c.y-1*s),1.2f*s,color,12);
            break;
        case 5:
            draw->AddCircle(c,6*s,color,28,2*s);
            draw->AddCircleFilled(c,2*s,color,16);
            for(int i=0;i<8;++i){const float a=PI_F*0.25f*i;draw->AddLine(ImVec2(c.x+std::cos(a)*8*s,c.y+std::sin(a)*8*s),ImVec2(c.x+std::cos(a)*12*s,c.y+std::sin(a)*12*s),color,2*s);}
            break;
        case 6: DrawPhoneGlyph(draw,c,s,color); break;
        case 7:
            draw->AddRect(ImVec2(c.x-13*s,c.y-9*s),ImVec2(c.x+13*s,c.y+10*s),color,4*s,0,1.8*s);
            draw->AddRect(ImVec2(c.x-7*s,c.y-12*s),ImVec2(c.x-1*s,c.y-9*s),color,1*s,0,1.5*s);
            draw->AddCircle(c,5*s,color,24,1.8*s);
            draw->AddCircleFilled(ImVec2(c.x+8*s,c.y-5*s),1.3*s,color,12);
            break;
        case 8:
            draw->AddLine(ImVec2(c.x-13*s,c.y-9*s),ImVec2(c.x-13*s,c.y+9*s),color,1.8*s);
            draw->AddLine(ImVec2(c.x-13*s,c.y-9*s),ImVec2(c.x-3*s,c.y-12*s),color,1.8*s);
            draw->AddLine(ImVec2(c.x-3*s,c.y-12*s),ImVec2(c.x+5*s,c.y-8*s),color,1.8*s);
            draw->AddLine(ImVec2(c.x+5*s,c.y-8*s),ImVec2(c.x+13*s,c.y-11*s),color,1.8*s);
            draw->AddLine(ImVec2(c.x+13*s,c.y-11*s),ImVec2(c.x+13*s,c.y+8*s),color,1.8*s);
            draw->AddLine(ImVec2(c.x+13*s,c.y+8*s),ImVec2(c.x+5*s,c.y+11*s),color,1.8*s);
            draw->AddLine(ImVec2(c.x+5*s,c.y+11*s),ImVec2(c.x-3*s,c.y+7*s),color,1.8*s);
            draw->AddLine(ImVec2(c.x-3*s,c.y+7*s),ImVec2(c.x-13*s,c.y+9*s),color,1.8*s);
            draw->AddLine(ImVec2(c.x-3*s,c.y-12*s),ImVec2(c.x-3*s,c.y+7*s),color,1.6*s);
            draw->AddLine(ImVec2(c.x+5*s,c.y-8*s),ImVec2(c.x+5*s,c.y+11*s),color,1.6*s);
            break;
        default:
            draw->AddLine(ImVec2(c.x-6*s,c.y-11*s),ImVec2(c.x+7*s,c.y-14*s),color,1.8*s);
            draw->AddLine(ImVec2(c.x+7*s,c.y-14*s),ImVec2(c.x+7*s,c.y+6*s),color,1.8*s);
            draw->AddCircleFilled(ImVec2(c.x-7*s,c.y+9*s),3*s,color,16);
            draw->AddCircleFilled(ImVec2(c.x+4*s,c.y+7*s),3*s,color,16);
            break;
    }
}

ActionRects BuildActionRects(const ImVec2& screen, const ImVec2& anchor, const ImVec2& anchorSize) {
    const float scale = std::max(0.55f, std::min(screen.y / 480.0f, screen.x / 640.0f));
    const float anchorWidth = std::max(anchorSize.x, 34.0f * scale);
    const float anchorHeight = std::max(anchorSize.y, 30.0f * scale);
    float side = std::max(anchorHeight * 1.45f, 58.0f * scale);
    side = std::min(side, std::max(62.0f * scale, screen.y * 0.14f));
    const float gap = 8.0f * scale;
    const float left = anchor.x + anchorWidth + gap;
    const float top = anchor.y + (anchorHeight - side) * 0.5f;
    ActionRects result;
    result.panel = {left, top, left + side, top + side};
    result.phone = {left + side + gap, top, left + side + gap + side, top + side};
    return result;
}

WheelLayout BuildWheelLayout(const ImVec2& screen) {
    WheelLayout layout{};
    const float h = std::max(1.0f, screen.y);
    const float w = std::max(1.0f, screen.x);
    const float scale = std::max(0.55f, std::min(h / 480.0f, w / 640.0f));
    layout.center = ImVec2(w * 0.5f, h * 0.5f);
    layout.radius = std::min(h * 0.365f, w * 0.235f);
    layout.radius = std::max(layout.radius, 75.0f * scale);
    layout.itemRadius = std::min(layout.radius * 0.275f, 48.0f * scale);
    layout.itemDistance = layout.radius * 0.86f;
    const float angles[4] = {-PI_F * 0.5f, 0.0f, PI_F * 0.5f, PI_F};
    for (int i = 0; i < 4; ++i) {
        layout.itemCenters[i] = ImVec2(layout.center.x + std::cos(angles[i]) * layout.itemDistance,
                                       layout.center.y + std::sin(angles[i]) * layout.itemDistance);
    }
    const float cr = 15.0f * scale;
    const ImVec2 cc(layout.center.x + layout.radius * 0.91f, layout.center.y - layout.radius * 0.91f);
    layout.closeButton = {cc.x-cr, cc.y-cr, cc.x+cr, cc.y+cr};
    return layout;
}

PhoneLayout BuildPhoneLayout(const ImVec2& screen) {
    PhoneLayout layout{};
    const float h = std::max(1.0f, screen.y);
    const float w = std::max(1.0f, screen.x);
    layout.scale = std::max(0.55f, std::min(h / 480.0f, w / 640.0f));
    const float phoneHeight = std::min(h * 0.86f, w * 1.48f);
    const float phoneWidth = phoneHeight * 0.55f;
    const float centeredLeft = (w - phoneWidth) * 0.5f;
    const float maxLeft = std::max(8.0f, w - phoneWidth - 8.0f);
    const float left = std::min(maxLeft, std::max(8.0f, centeredLeft + w * 0.12f));
    const float top = (h - phoneHeight) * 0.5f;
    layout.frame = {left, top, left + phoneWidth, top + phoneHeight};
    const float margin = 7.0f * layout.scale;
    layout.screen = {left + margin, top + margin, left + phoneWidth - margin, top + phoneHeight - margin};
    const float closeR = 10.0f * layout.scale;
    const float closeX = layout.frame.right - 21.0f * layout.scale;
    const float closeY = layout.frame.top + 22.0f * layout.scale;
    layout.closeButton = {closeX-closeR, closeY-closeR, closeX+closeR, closeY+closeR};
    const float sh = layout.screen.bottom - layout.screen.top;
    layout.swipeZone = {layout.screen.left, layout.screen.top + sh * 0.76f,
                        layout.screen.right, layout.screen.bottom};
    layout.homeIndicator = {layout.screen.left + (layout.screen.right-layout.screen.left)*0.34f,
                            layout.screen.bottom - 15.0f*layout.scale,
                            layout.screen.left + (layout.screen.right-layout.screen.left)*0.66f,
                            layout.screen.bottom - 4.0f*layout.scale};
    for (auto& tile : layout.appTiles) tile = {0, 0, 0, 0};
    const float sw = layout.screen.right - layout.screen.left;
    const float cellW = sw * 0.28f;
    const float cellH = sh * 0.19f;
    const float gridTop = layout.screen.top + sh * 0.28f;
    const float colStep = sw * 0.30f;
    const float rowStep = sh * 0.225f;
    for (int i = 0; i < 6; ++i) {
        const int col = i % 3;
        const int row = i / 3;
        const float x = layout.screen.left + sw * 0.08f + col * colStep;
        const float y = gridTop + row * rowStep;
        layout.appTiles[i] = {x, y, x + cellW, y + cellH};
    }
    const float dockTop = layout.screen.top + sh * 0.755f;
    const float dockH = sh * 0.12f;
    const float dockCellW = sw * 0.205f;
    const float dockStep = sw * 0.225f;
    for (int i = 0; i < 4; ++i) {
        const float x = layout.screen.left + sw * 0.05f + i * dockStep;
        layout.appTiles[6+i] = {x, dockTop, x + dockCellW, dockTop + dockH};
    }
    return layout;
}

int PhoneAppAt(const PhoneLayout& layout, float x, float y) {
    for (int i = 0; i < 10; ++i) if (Hit(layout.appTiles[i], x, y)) return i;
    return -1;
}

void DrawCloseIcon(ImDrawList* draw, const HudRect& rect, float scale) {
    const ImVec2 c((rect.left + rect.right) * 0.5f, (rect.top + rect.bottom) * 0.5f);
    const float d = std::max(4.0f, (rect.right - rect.left) * 0.23f);
    draw->AddLine(ImVec2(c.x-d, c.y-d), ImVec2(c.x+d, c.y+d), IM_COL32(255,255,255,240), 2.0f*scale);
    draw->AddLine(ImVec2(c.x+d, c.y-d), ImVec2(c.x-d, c.y+d), IM_COL32(255,255,255,240), 2.0f*scale);
}

void DrawWheel(ImDrawList* draw, const ImVec2& screen, int selected) {
    const WheelLayout layout = BuildWheelLayout(screen);
    const float scale = std::max(0.55f, std::min(screen.y / 480.0f, screen.x / 640.0f));
    const ImU32 gold = IM_COL32(241, 190, 91, 255);
    const ImU32 cyan = IM_COL32(105, 220, 226, 255);
    const ImU32 pale = IM_COL32(241, 246, 252, 255);
    draw->AddRectFilledMultiColor(ImVec2(0,0), screen, IM_COL32(4,9,18,224), IM_COL32(10,17,29,224),
                                  IM_COL32(5,12,23,224), IM_COL32(2,7,15,224));
    draw->AddCircleFilled(layout.center, layout.radius + 19.0f*scale, IM_COL32(5,11,20,105), 96);
    draw->AddCircleFilled(layout.center, layout.radius + 9.0f*scale, IM_COL32(10,18,30,225), 96);
    draw->AddCircle(layout.center, layout.radius + 9.0f*scale, IM_COL32(116,145,173,110), 96, 1.0f*scale);
    draw->AddCircle(layout.center, layout.radius * 0.72f, IM_COL32(107,217,222,32), 96, 1.1f*scale);

    for (int i = 0; i < 4; ++i) {
        draw->AddLine(layout.center, layout.itemCenters[i], IM_COL32(141,164,185,55), 1.0f*scale);
    }

    AddCenteredText(draw, "PANEL CEPAT", layout.center.x, layout.center.y-layout.radius-43.0f*scale,
                    pale, 14.0f*scale);
    AddCenteredText(draw, "VICE SIDE ROLEPLAY  /  AKSES UTAMA", layout.center.x,
                    layout.center.y-layout.radius-24.0f*scale, IM_COL32(170,190,207,235), 7.2f*scale);

    for (int i = 0; i < 4; ++i) {
        const ImVec2 c = layout.itemCenters[i];
        const float cardW = 112.0f*scale;
        const float cardH = 78.0f*scale;
        const ImVec2 lo(c.x-cardW*0.5f,c.y-cardH*0.5f);
        const ImVec2 hi(c.x+cardW*0.5f,c.y+cardH*0.5f);
        const bool active = i == selected;
        const ImU32 edge = active ? gold : IM_COL32(120,151,179,145);
        const ImU32 iconBg = active ? IM_COL32(116,79,34,255) : IM_COL32(28,45,62,255);
        draw->AddRectFilled(ImVec2(lo.x,lo.y+4.0f*scale),ImVec2(hi.x,hi.y+4.0f*scale),IM_COL32(0,0,0,95),13.0f*scale);
        draw->AddRectFilled(lo,hi,active?IM_COL32(31,34,38,250):IM_COL32(17,29,42,245),13.0f*scale);
        draw->AddLine(ImVec2(lo.x+14*scale,lo.y+1*scale),ImVec2(hi.x-14*scale,lo.y+1*scale),
                      active?IM_COL32(241,190,91,80):IM_COL32(155,190,214,55),1.0f*scale);
        draw->AddRect(lo,hi,edge,13.0f*scale,0,active?1.8f*scale:1.1f*scale);
        draw->AddCircleFilled(ImVec2(c.x,c.y-9.0f*scale),17.0f*scale,iconBg,40);
        draw->AddCircle(ImVec2(c.x,c.y-9.0f*scale),17.0f*scale,active?IM_COL32(255,220,151,210):IM_COL32(145,180,204,110),40,1.0f*scale);
        DrawWheelGlyph(draw,i,ImVec2(c.x,c.y-9.0f*scale),0.66f*scale,pale);
        AddCenteredText(draw,WHEEL_LABELS[i],c.x,c.y+20.0f*scale,pale,9.3f*scale);
        if (active) draw->AddCircleFilled(ImVec2(hi.x-10.0f*scale,lo.y+10.0f*scale),2.4f*scale,gold,16);
    }

    const float hubW = 138.0f*scale;
    const float hubH = 80.0f*scale;
    const ImVec2 hubMin(layout.center.x-hubW*0.5f,layout.center.y-hubH*0.5f);
    const ImVec2 hubMax(layout.center.x+hubW*0.5f,layout.center.y+hubH*0.5f);
    draw->AddRectFilled(ImVec2(hubMin.x,hubMin.y+4.0f*scale),ImVec2(hubMax.x,hubMax.y+4.0f*scale),IM_COL32(0,0,0,105),18.0f*scale);
    draw->AddRectFilled(hubMin,hubMax,IM_COL32(13,24,37,250),18.0f*scale);
    draw->AddRect(hubMin,hubMax,IM_COL32(107,217,222,150),18.0f*scale,0,1.0f*scale);
    AddCenteredText(draw,"VICE SIDE",layout.center.x,layout.center.y-22.0f*scale,gold,8.5f*scale);
    AddCenteredText(draw,selected>=0?WHEEL_LABELS[selected]:"PANEL UTAMA",layout.center.x,
                    layout.center.y-5.0f*scale,pale,11.0f*scale);
    AddCenteredText(draw,selected>=0?"Menu dipilih":"Pilih menu di sekeliling",layout.center.x,
                    layout.center.y+14.0f*scale,IM_COL32(174,192,207,245),7.0f*scale);

    const ImVec2 cc((layout.closeButton.left+layout.closeButton.right)*0.5f,
                    (layout.closeButton.top+layout.closeButton.bottom)*0.5f);
    const float closeR=(layout.closeButton.right-layout.closeButton.left)*0.5f;
    draw->AddCircleFilled(cc,closeR+3.0f*scale,IM_COL32(0,0,0,100),32);
    draw->AddCircleFilled(cc,closeR,IM_COL32(20,31,44,250),32);
    draw->AddCircle(cc,closeR,IM_COL32(241,190,91,220),32,1.2f*scale);
    DrawCloseIcon(draw,layout.closeButton,scale);
}

void DrawPhone(ImDrawList* draw, const ImVec2& screen, bool unlocked,
               float swipeOffset, const std::string& toast, double toastUntil) {
    const PhoneLayout layout = BuildPhoneLayout(screen);
    const float s = layout.scale;
    const float pw = layout.frame.right-layout.frame.left;
    const ImVec2 phoneMin(layout.frame.left,layout.frame.top);
    const ImVec2 phoneMax(layout.frame.right,layout.frame.bottom);
    const ImVec2 screenMin(layout.screen.left,layout.screen.top);
    const ImVec2 screenMax(layout.screen.right,layout.screen.bottom);
    ImFont* font=ImGui::GetFont();

    draw->AddRectFilled(ImVec2(0,0),screen,IM_COL32(2,7,15,190));
    draw->AddCircleFilled(ImVec2(screen.x*0.77f,screen.y*0.26f),screen.y*0.42f,IM_COL32(34,106,126,28),80);
    draw->AddRectFilled(ImVec2(phoneMin.x+6*s,phoneMin.y+9*s),ImVec2(phoneMax.x+6*s,phoneMax.y+9*s),IM_COL32(0,0,0,125),31*s);
    draw->AddRectFilled(phoneMin,phoneMax,IM_COL32(16,25,36,255),31*s);
    draw->AddRect(phoneMin,phoneMax,IM_COL32(186,202,215,240),31*s,0,1.5*s);
    const ImVec2 screenMinInset(screenMin.x+1.0f*s,screenMin.y+1.0f*s);
    const ImVec2 screenMaxInset(screenMax.x-1.0f*s,screenMax.y-1.0f*s);
    draw->AddRectFilled(screenMinInset,screenMaxInset,IM_COL32(24,39,67,255),23*s);

    draw->PushClipRect(screenMin,screenMax,true);
    const float sw=layout.screen.right-layout.screen.left;
    const float sh=layout.screen.bottom-layout.screen.top;
    draw->AddCircleFilled(ImVec2(screenMin.x+sw*0.83f,screenMin.y+sh*0.18f),sh*0.29f,IM_COL32(88,119,184,92),64);
    draw->AddCircleFilled(ImVec2(screenMin.x+sw*0.17f,screenMin.y+sh*0.63f),sh*0.36f,IM_COL32(153,73,155,82),64);
    draw->AddCircleFilled(ImVec2(screenMin.x+sw*0.86f,screenMin.y+sh*0.82f),sh*0.32f,IM_COL32(27,148,160,65),64);

    char timeText[16]="09:41";
    char dateText[48]="Vice Side Roleplay";
    std::time_t now=std::time(nullptr);
    std::tm localTime{};
#if defined(__ANDROID__) || defined(__linux__)
    localtime_r(&now,&localTime);
#else
    if(std::tm* tmNow=std::localtime(&now)) localTime=*tmNow;
#endif
    std::strftime(timeText,sizeof(timeText),"%H:%M",&localTime);
    std::strftime(dateText,sizeof(dateText),"%A, %d %B",&localTime);
    if(font) draw->AddText(font,8.0f*s,ImVec2(screenMin.x+13*s,screenMin.y+8*s),IM_COL32(255,255,255,245),timeText);
    const float islandW=52.0f*s;
    draw->AddRectFilled(ImVec2((screenMin.x+screenMax.x-islandW)*0.5f,screenMin.y+5*s),
                        ImVec2((screenMin.x+screenMax.x+islandW)*0.5f,screenMin.y+17*s),IM_COL32(2,4,8,245),8*s);
    const float signalX=screenMax.x-34*s;
    draw->AddLine(ImVec2(signalX,screenMin.y+14*s),ImVec2(signalX,screenMin.y+10*s),IM_COL32(255,255,255,230),1.4*s);
    draw->AddLine(ImVec2(signalX+4*s,screenMin.y+14*s),ImVec2(signalX+4*s,screenMin.y+7*s),IM_COL32(255,255,255,230),1.4*s);
    draw->AddLine(ImVec2(signalX+8*s,screenMin.y+14*s),ImVec2(signalX+8*s,screenMin.y+4*s),IM_COL32(255,255,255,230),1.4*s);
    draw->AddRect(ImVec2(screenMax.x-19*s,screenMin.y+7*s),ImVec2(screenMax.x-8*s,screenMin.y+14*s),IM_COL32(255,255,255,220),2*s,0,1*s);
    draw->AddRectFilled(ImVec2(screenMax.x-7*s,screenMin.y+9*s),ImVec2(screenMax.x-5*s,screenMin.y+12*s),IM_COL32(255,255,255,220),1*s);

    if(!unlocked){
        AddCenteredText(draw,dateText,(screenMin.x+screenMax.x)*0.5f,screenMin.y+sh*0.245f,IM_COL32(235,242,252,245),9.5f*s);
        AddCenteredText(draw,timeText,(screenMin.x+screenMax.x)*0.5f,screenMin.y+sh*0.285f,IM_COL32(255,255,255,255),37.0f*s);
        const HudRect notification{screenMin.x+sw*0.08f,screenMin.y+sh*0.46f,screenMax.x-sw*0.08f,screenMin.y+sh*0.585f};
        draw->AddRectFilled(ImVec2(notification.left,notification.top),ImVec2(notification.right,notification.bottom),IM_COL32(10,18,31,205),13*s);
        draw->AddRect(ImVec2(notification.left,notification.top),ImVec2(notification.right,notification.bottom),IM_COL32(255,255,255,48),13*s,0,1*s);
        draw->AddCircleFilled(ImVec2(notification.left+15*s,notification.top+15*s),5*s,IM_COL32(241,190,91,255),20);
        if(font){
            draw->AddText(font,8.3f*s,ImVec2(notification.left+26*s,notification.top+7*s),IM_COL32(241,190,91,255),"VICE SIDE ROLEPLAY");
            draw->AddText(font,7.0f*s,ImVec2(notification.left+10*s,notification.top+24*s),IM_COL32(235,240,248,245),"Handphone karakter siap digunakan.");
        }
        const float handleY=screenMax.y-34*s-swipeOffset;
        draw->AddRectFilled(ImVec2((screenMin.x+screenMax.x)*0.5f-17*s,handleY-12*s),
                            ImVec2((screenMin.x+screenMax.x)*0.5f+17*s,handleY+12*s),IM_COL32(255,255,255,33),12*s);
        draw->AddLine(ImVec2((screenMin.x+screenMax.x)*0.5f-5*s,handleY+2*s),ImVec2((screenMin.x+screenMax.x)*0.5f,handleY-3*s),IM_COL32(255,255,255,235),1.5*s);
        draw->AddLine(ImVec2((screenMin.x+screenMax.x)*0.5f,handleY-3*s),ImVec2((screenMin.x+screenMax.x)*0.5f+5*s,handleY+2*s),IM_COL32(255,255,255,235),1.5*s);
        AddCenteredText(draw,"GESER KE ATAS UNTUK BUKA",(screenMin.x+screenMax.x)*0.5f,screenMax.y-18*s,IM_COL32(232,238,247,235),6.7f*s);
    }else{
        if(font){
            draw->AddText(font,8.5f*s,ImVec2(screenMin.x+14*s,screenMin.y+32*s),IM_COL32(183,205,229,245),"VICE SIDE ROLEPLAY");
            draw->AddText(font,15.0f*s,ImVec2(screenMin.x+14*s,screenMin.y+48*s),IM_COL32(255,255,255,255),"Aplikasi");
        }
        const HudRect dock{screenMin.x+sw*0.045f,screenMin.y+sh*0.745f,screenMax.x-sw*0.045f,screenMin.y+sh*0.895f};
        draw->AddRectFilled(ImVec2(dock.left,dock.top),ImVec2(dock.right,dock.bottom),IM_COL32(232,240,250,43),14*s);
        draw->AddRect(ImVec2(dock.left,dock.top),ImVec2(dock.right,dock.bottom),IM_COL32(255,255,255,48),14*s,0,1*s);
        const ImU32 tileColors[10]={IM_COL32(204,142,64,245),IM_COL32(61,157,119,245),IM_COL32(96,112,204,245),
            IM_COL32(63,145,194,245),IM_COL32(184,83,121,245),IM_COL32(99,111,128,245),
            IM_COL32(47,159,112,245),IM_COL32(130,148,163,245),IM_COL32(57,126,174,245),IM_COL32(154,86,190,245)};
        for(int i=0;i<10;++i){
            const HudRect& tile=layout.appTiles[i];
            const float iconSize=std::min(tile.bottom-tile.top,tile.right-tile.left)*0.62f;
            const ImVec2 iconMin((tile.left+tile.right-iconSize)*0.5f,tile.top+2.0f*s);
            const ImVec2 iconMax(iconMin.x+iconSize,iconMin.y+iconSize);
            draw->AddRectFilled(ImVec2(iconMin.x,iconMin.y+2*s),ImVec2(iconMax.x,iconMax.y+2*s),IM_COL32(0,0,0,65),9*s);
            draw->AddRectFilled(iconMin,iconMax,tileColors[i],9*s);
            draw->AddRect(iconMin,iconMax,IM_COL32(255,255,255,80),9*s,0,0.8*s);
            const ImVec2 glyphCenter((iconMin.x+iconMax.x)*0.5f,(iconMin.y+iconMax.y)*0.5f);
            DrawPhoneAppGlyph(draw,i,glyphCenter,iconSize/38.0f,IM_COL32(255,255,255,250));
            AddCenteredText(draw,PHONE_APP_LABELS[i],(tile.left+tile.right)*0.5f,iconMax.y+3.0f*s,IM_COL32(249,250,255,250),7.0f*s);
        }
        draw->AddRectFilled(ImVec2(layout.homeIndicator.left,layout.homeIndicator.top),
                            ImVec2(layout.homeIndicator.right,layout.homeIndicator.bottom),IM_COL32(255,255,255,235),5*s);
    }
    draw->PopClipRect();

    const ImVec2 closeCenter((layout.closeButton.left+layout.closeButton.right)*0.5f,
                             (layout.closeButton.top+layout.closeButton.bottom)*0.5f);
    draw->AddCircleFilled(closeCenter,(layout.closeButton.right-layout.closeButton.left)*0.5f,IM_COL32(12,18,28,245),24);
    draw->AddCircle(closeCenter,(layout.closeButton.right-layout.closeButton.left)*0.5f,IM_COL32(255,255,255,90),24,1*s);
    DrawCloseIcon(draw,layout.closeButton,s);

    if(toastUntil>ImGui::GetTime()&&!toast.empty()){
        const float toastW=std::min(pw*0.88f,std::max(100.0f*s,toast.size()*6.0f*s));
        const float toastH=26.0f*s;
        const float tx=(screenMin.x+screenMax.x)*0.5f;
        const float ty=screenMax.y-52.0f*s;
        draw->AddRectFilled(ImVec2(tx-toastW*0.5f,ty),ImVec2(tx+toastW*0.5f,ty+toastH),IM_COL32(7,12,20,230),10*s);
        AddCenteredText(draw,toast.c_str(),tx,ty+7.0f*s,IM_COL32(255,255,255,245),7.0f*s);
    }
}
}

ButtonPanel::ButtonPanel()
    : Layout(Orientation::HORIZONTAL)
{
    CButton* m_bTab = new CButton("TAB", UISettings::fontSize() / 2);
    m_bOpen = new OButton(">>", UISettings::fontSize() / 2);
    CButton* m_bClose = new CButton("<<", UISettings::fontSize() / 2);
    CButton* m_bEsc = new CButton("ESC", UISettings::fontSize() / 2);
    CButton* m_bAlt = new CButton("ALT", UISettings::fontSize() / 2);
    m_bH = new CButton("H", UISettings::fontSize() / 2);
    CButton* m_bY = new CButton("Y", UISettings::fontSize() / 2);
    CButton* m_bN = new CButton("N", UISettings::fontSize() / 2);
    CButton* m_bF = new CButton("F", UISettings::fontSize() / 2);
    CButton* m_bP = new CButton("+", UISettings::fontSize() / 2);
    CButton* m_bG = new CButton("G", UISettings::fontSize() / 2);
    CButton* m_bFOOD = new CButton("FOOD", UISettings::fontSize() / 2);
    CButton* m_bGPS = new CButton("GPS", UISettings::fontSize() / 2);
    CButton* m_bD = new CButton("D", UISettings::fontSize() / 2);
    CButton* m_bUSE = new CButton("USE", UISettings::fontSize() / 2);
    CButton* m_b2 = new CButton("2", UISettings::fontSize() / 2);

    m_bTab->setCallback([]() {
        if (Tab == 0) {
            if (pUI) { pUI->playertablist()->show(); Tab = 1; }
        } else {
            Tab = 0;
        }
    });
    m_bOpen->setCallback([]() { OpenButton = true; });
    m_bClose->setCallback([]() { OpenButton = false; });
    m_bEsc->setCallback([m_bEsc]() {
        if (pNetGame && m_bEsc->visible()) {
            CTextDrawPool* pTextDrawPool = pNetGame->GetTextDrawPool();
            if (pTextDrawPool) pTextDrawPool->SetSelectState(false, 0);
        }
    });
    m_bAlt->setCallback([m_bAlt]() {
        if (m_bAlt->visible()) LocalPlayerKeys.bKeys[ePadKeys::KEY_WALK] = true;
    });
    m_bH->setCallback([this]() {
        if (m_bH->visible()) LocalPlayerKeys.bKeys[ePadKeys::KEY_CTRL_BACK] = true;
    });
    m_bY->setCallback([m_bY]() { if (m_bY->visible()) LocalPlayerKeys.bKeys[ePadKeys::KEY_YES] = true; });
    m_bN->setCallback([m_bN]() { if (m_bN->visible()) LocalPlayerKeys.bKeys[ePadKeys::KEY_NO] = true; });
    m_bF->setCallback([m_bF]() { if (m_bF->visible()) LocalPlayerKeys.bKeys[ePadKeys::KEY_SECONDARY_ATTACK] = true; });
    m_bP->setCallback([m_bP]() { if (m_bP->visible()) LocalPlayerKeys.bKeys[ePadKeys::KEY_SUBMISSION] = true; });
    m_bG->setCallback([m_bG]() { if (m_bG->visible()) bNeedEnterVehicle = true; });
    m_bGPS->setCallback([]() { if (pNetGame) pNetGame->SendChatCommand("/gps"); });
    m_bD->setCallback([]() { LocalPlayerKeys.bKeys[ePadKeys::KEY_SUBMISSION] = true; });
    m_bUSE->setCallback([]() { LocalPlayerKeys.bKeys[ePadKeys::KEY_ANALOG_LEFT] = true; });
    m_b2->setCallback([m_b2]() { if (m_b2->visible()) LocalPlayerKeys.bKeys[ePadKeys::KEY_SUBMISSION] = true; });

    this->addChild(m_bClose);
    this->addChild(m_bTab);
    this->addChild(m_bEsc);
    this->addChild(m_bAlt);
    this->addChild(m_bH);
    this->addChild(m_bF);
    this->addChild(m_bY);
    this->addChild(m_bN);
    this->addChild(m_bG);
    this->addChild(m_b2);
    this->addChild(m_bOpen);
    m_bOpen->performLayout();
    m_bOpen->setPosition(ImVec2(15.0f, 15.0f));
}

void ButtonPanel::drawLauncherUi() {
    if (!visible() || !pGame || pGame->IsGamePaused()) return;
    ImVec2 screen = ImGui::GetIO().DisplaySize;
    if (screen.x <= 0.0f || screen.y <= 0.0f) screen = ImVec2(640.0f, 480.0f);
    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    if (!draw) return;

    if (!OpenButton && m_bOpen) {
        const ActionRects rects = BuildActionRects(screen, m_bOpen->absolutePosition(), m_bOpen->size());
        const float side = rects.panel.right - rects.panel.left;
        const float scale = side / 42.0f;
        const ImU32 gold = IM_COL32(240,190,87,255);
        const ImU32 cyan = IM_COL32(107,217,222,255);
        draw->AddRectFilled(ImVec2(rects.panel.left,rects.panel.top+3*scale),ImVec2(rects.panel.right,rects.panel.bottom+3*scale),IM_COL32(0,0,0,105),side*0.26f);
        draw->AddRectFilled(ImVec2(rects.panel.left,rects.panel.top),ImVec2(rects.panel.right,rects.panel.bottom),
            m_pressedAction==TARGET_PANEL_BUTTON?IM_COL32(51,43,31,250):IM_COL32(18,29,43,245),side*0.26f);
        draw->AddRect(ImVec2(rects.panel.left,rects.panel.top),ImVec2(rects.panel.right,rects.panel.bottom),gold,side*0.26f,0,1.7f*scale);
        draw->AddRectFilled(ImVec2(rects.phone.left,rects.phone.top+3*scale),ImVec2(rects.phone.right,rects.phone.bottom+3*scale),IM_COL32(0,0,0,105),side*0.26f);
        draw->AddRectFilled(ImVec2(rects.phone.left,rects.phone.top),ImVec2(rects.phone.right,rects.phone.bottom),
            m_pressedAction==TARGET_PHONE_BUTTON?IM_COL32(22,63,68,250):IM_COL32(16,34,45,245),side*0.26f);
        draw->AddRect(ImVec2(rects.phone.left,rects.phone.top),ImVec2(rects.phone.right,rects.phone.bottom),cyan,side*0.26f,0,1.7f*scale);
        draw->AddCircleFilled(ImVec2((rects.panel.left+rects.panel.right)*0.5f,rects.panel.top+side*0.36f),side*0.25f,IM_COL32(63,48,29,245),32);
        draw->AddCircleFilled(ImVec2((rects.phone.left+rects.phone.right)*0.5f,rects.phone.top+side*0.36f),side*0.25f,IM_COL32(16,62,69,245),32);
        DrawPanelGlyph(draw, ImVec2((rects.panel.left+rects.panel.right)*0.5f, rects.panel.top+side*0.36f), scale*0.93f, gold);
        DrawPhoneGlyph(draw, ImVec2((rects.phone.left+rects.phone.right)*0.5f, rects.phone.top+side*0.36f), scale*0.92f, cyan);
        AddCenteredText(draw,"PANEL",(rects.panel.left+rects.panel.right)*0.5f,rects.panel.top+side*0.72f,IM_COL32(245,247,251,255),9.2f*scale);
        AddCenteredText(draw,"HP",(rects.phone.left+rects.phone.right)*0.5f,rects.phone.top+side*0.72f,IM_COL32(245,247,251,255),9.5f*scale);
    }

    if (m_launcherMode == MODE_WHEEL) {
        DrawWheel(draw, screen, m_selectedWheelItem);
    } else if (m_launcherMode == MODE_PHONE) {
        DrawPhone(draw, screen, m_phoneUnlocked, m_phoneSwipeOffset, m_toastMessage, m_toastUntil);
    }
}

bool ButtonPanel::handleLauncherTouch(int type, int pointer, int x, int y) {
    if (!visible() || !pGame || pGame->IsGamePaused()) return false;
    ImVec2 screen = ImGui::GetIO().DisplaySize;
    if (screen.x <= 0.0f || screen.y <= 0.0f) screen = ImVec2(640.0f, 480.0f);
    const float px = static_cast<float>(x);
    const float py = static_cast<float>(y);

    // Overlay mode owns every pointer so gameplay controls below it never fire.
    if (m_launcherMode != MODE_CLOSED) {
        if (pointer != 0) return true;
        if (type == 2) {
            m_touchCaptured = true;
            m_touchTarget = TARGET_MODAL;
            m_touchStart = ImVec2(px,py);
            m_phoneSwipeArmed = false;
            m_phoneSwipeOffset = 0.0f;
            if (m_launcherMode == MODE_PHONE && !m_phoneUnlocked) {
                const PhoneLayout phone = BuildPhoneLayout(screen);
                m_phoneSwipeArmed = Hit(phone.swipeZone,px,py);
            }
            return true;
        }
        if (m_touchCaptured && m_touchTarget == TARGET_MODAL) {
            if (type == 3) {
                if (m_launcherMode == MODE_PHONE && !m_phoneUnlocked && m_phoneSwipeArmed) {
                    m_phoneSwipeOffset = std::max(0.0f, std::min(screen.y*0.10f, m_touchStart.y-py));
                }
                return true;
            }
            if (type == 1) {
                if (m_launcherMode == MODE_WHEEL) {
                    const WheelLayout wheel = BuildWheelLayout(screen);
                    if (Hit(wheel.closeButton,px,py)) {
                        m_launcherMode = MODE_CLOSED;
                    } else {
                        int selected = -1;
                        const float wheelScale = std::max(0.55f, std::min(screen.y/480.0f, screen.x/640.0f));
                        const float hitHalfWidth = 56.0f*wheelScale;
                        const float hitHalfHeight = 39.0f*wheelScale;
                        for (int i = 0; i < 4; ++i) {
                            const ImVec2 c = wheel.itemCenters[i];
                            if (std::abs(px-c.x) <= hitHalfWidth && std::abs(py-c.y) <= hitHalfHeight) { selected = i; break; }
                        }
                        if (selected >= 0) m_selectedWheelItem = selected;
                        else if (px < wheel.center.x-wheel.radius*1.18f || px > wheel.center.x+wheel.radius*1.18f ||
                                 py < wheel.center.y-wheel.radius*1.18f || py > wheel.center.y+wheel.radius*1.18f) {
                            m_launcherMode = MODE_CLOSED;
                        }
                    }
                } else if (m_launcherMode == MODE_PHONE) {
                    const PhoneLayout phone = BuildPhoneLayout(screen);
                    if (Hit(phone.closeButton,px,py)) {
                        m_launcherMode = MODE_CLOSED;
                        m_phoneUnlocked = false;
                    } else if (!m_phoneUnlocked) {
                        if (m_phoneSwipeArmed && m_touchStart.y-py > screen.y*0.065f) {
                            m_phoneUnlocked = true;
                            m_toastMessage.clear();
                            m_toastUntil = 0.0;
                        } else if (!Hit(phone.frame,px,py)) {
                            m_launcherMode = MODE_CLOSED;
                        }
                    } else if (Hit(phone.homeIndicator,px,py)) {
                        m_phoneUnlocked = false;
                    } else {
                        const int app = PhoneAppAt(phone,px,py);
                        if (app >= 0) {
                            m_toastMessage = std::string(PHONE_APP_LABELS[app]) + " segera tersedia";
                            m_toastUntil = ImGui::GetTime() + 1.8;
                        } else if (!Hit(phone.frame,px,py)) {
                            m_launcherMode = MODE_CLOSED;
                        }
                    }
                }
                m_phoneSwipeArmed = false;
                m_phoneSwipeOffset = 0.0f;
                m_touchCaptured = false;
                m_touchTarget = TARGET_NONE;
                return true;
            }
            return true;
        }
        return true;
    }

    if (OpenButton || !m_bOpen) return false;
    const ActionRects rects = BuildActionRects(screen, m_bOpen->absolutePosition(), m_bOpen->size());
    if (type == 2 && pointer == 0) {
        if (Hit(rects.panel,px,py)) {
            m_touchCaptured = true;
            m_touchTarget = TARGET_PANEL_BUTTON;
            m_pressedAction = TARGET_PANEL_BUTTON;
            return true;
        }
        if (Hit(rects.phone,px,py)) {
            m_touchCaptured = true;
            m_touchTarget = TARGET_PHONE_BUTTON;
            m_pressedAction = TARGET_PHONE_BUTTON;
            return true;
        }
        return false;
    }
    if (m_touchCaptured && (m_touchTarget == TARGET_PANEL_BUTTON || m_touchTarget == TARGET_PHONE_BUTTON)) {
        if (type == 3) return true;
        if (type == 1) {
            const int target = m_touchTarget;
            if (target == TARGET_PANEL_BUTTON && Hit(rects.panel,px,py)) {
                m_launcherMode = MODE_WHEEL;
                m_selectedWheelItem = -1;
            } else if (target == TARGET_PHONE_BUTTON && Hit(rects.phone,px,py)) {
                m_launcherMode = MODE_PHONE;
                m_phoneUnlocked = false;
                m_phoneSwipeOffset = 0.0f;
                m_toastMessage.clear();
                m_toastUntil = 0.0;
            }
            m_touchCaptured = false;
            m_touchTarget = TARGET_NONE;
            m_pressedAction = TARGET_NONE;
            return true;
        }
        return true;
    }
    return false;
}
