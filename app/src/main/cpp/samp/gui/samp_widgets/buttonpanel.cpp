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

ActionRects BuildActionRects(const ImVec2& screen, const ImVec2& anchor, const ImVec2& anchorSize) {
    const float scale = std::max(0.55f, screen.y / 480.0f);
    const float anchorWidth = std::max(anchorSize.x, 34.0f * scale);
    const float anchorHeight = std::max(anchorSize.y, 30.0f * scale);
    float side = std::max(anchorHeight * 0.78f, 27.0f * scale);
    side = std::min(side, std::max(28.0f * scale, screen.y * 0.12f));
    const float gap = 5.0f * scale;
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
    layout.itemDistance = layout.radius * 0.69f;
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
    const float phoneHeight = std::min(h * 0.80f, w * 1.40f);
    const float phoneWidth = phoneHeight * 0.52f;
    const float left = (w - phoneWidth) * 0.5f;
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
    const ImU32 gold = IM_COL32(240, 190, 87, 245);
    const ImU32 pale = IM_COL32(243, 238, 224, 255);
    draw->AddRectFilled(ImVec2(0,0), screen, IM_COL32(3, 8, 17, 168));
    draw->AddCircleFilled(layout.center, layout.radius + 10.0f*scale, IM_COL32(8, 14, 24, 222), 96);
    draw->AddCircle(layout.center, layout.radius + 8.0f*scale, IM_COL32(77, 97, 119, 170), 96, 1.0f*scale);
    draw->AddCircle(layout.center, layout.radius * 0.78f, IM_COL32(240, 190, 87, 110), 96, 1.0f*scale);
    draw->AddCircle(layout.center, layout.radius * 0.54f, IM_COL32(114, 137, 163, 80), 96, 1.0f*scale);

    for (int i = 0; i < 4; ++i) {
        const ImVec2 c = layout.itemCenters[i];
        const float r = layout.itemRadius;
        const ImU32 fill = (i == selected) ? IM_COL32(95, 70, 37, 248) : IM_COL32(15, 24, 37, 248);
        const ImU32 edge = (i == selected) ? gold : IM_COL32(115, 137, 158, 210);
        draw->AddCircleFilled(c, r + 3.0f*scale, fill, 64);
        draw->AddCircle(c, r + 3.0f*scale, edge, 64, 1.6f*scale);
        DrawWheelGlyph(draw, i, ImVec2(c.x, c.y - 7.0f*scale), scale, pale);
        AddCenteredText(draw, WHEEL_LABELS[i], c.x, c.y + r*0.43f, pale, 10.5f*scale);
        if (i == selected) {
            draw->AddCircleFilled(ImVec2(c.x + r*0.72f, c.y - r*0.72f), 3.5f*scale, gold, 18);
        }
    }

    const float centerR = layout.radius * 0.36f;
    draw->AddCircleFilled(layout.center, centerR, IM_COL32(10, 17, 28, 250), 72);
    draw->AddCircle(layout.center, centerR, IM_COL32(240, 190, 87, 190), 72, 1.4f*scale);
    AddCenteredText(draw, "VICE SIDE", layout.center.x, layout.center.y - 22.0f*scale,
                    IM_COL32(240,190,87,255), 10.0f*scale);
    AddCenteredText(draw, selected >= 0 ? WHEEL_LABELS[selected] : "PANEL UTAMA",
                    layout.center.x, layout.center.y - 2.0f*scale, pale, 13.0f*scale);
    AddCenteredText(draw, selected >= 0 ? "UI fitur menyusul" : "Pilih salah satu menu",
                    layout.center.x, layout.center.y + 20.0f*scale,
                    IM_COL32(169,183,198,245), 8.5f*scale);
    draw->AddCircleFilled(ImVec2((layout.closeButton.left+layout.closeButton.right)*0.5f,
                                 (layout.closeButton.top+layout.closeButton.bottom)*0.5f),
                          (layout.closeButton.right-layout.closeButton.left)*0.5f,
                          IM_COL32(18,27,39,245), 32);
    draw->AddCircle(ImVec2((layout.closeButton.left+layout.closeButton.right)*0.5f,
                           (layout.closeButton.top+layout.closeButton.bottom)*0.5f),
                    (layout.closeButton.right-layout.closeButton.left)*0.5f,
                    IM_COL32(240,190,87,210), 32, 1.3f*scale);
    DrawCloseIcon(draw, layout.closeButton, scale);
}

void DrawPhone(ImDrawList* draw, const ImVec2& screen, bool unlocked,
               float swipeOffset, const std::string& toast, double toastUntil) {
    const PhoneLayout layout = BuildPhoneLayout(screen);
    const float s = layout.scale;
    const float pw = layout.frame.right - layout.frame.left;
    const float ph = layout.frame.bottom - layout.frame.top;
    const ImVec2 phoneMin(layout.frame.left, layout.frame.top);
    const ImVec2 phoneMax(layout.frame.right, layout.frame.bottom);
    const ImVec2 screenMin(layout.screen.left, layout.screen.top);
    const ImVec2 screenMax(layout.screen.right, layout.screen.bottom);
    ImFont* font = ImGui::GetFont();

    draw->AddRectFilled(ImVec2(0,0), screen, IM_COL32(3, 8, 17, 178));
    draw->AddRectFilled(phoneMin, phoneMax, IM_COL32(9, 13, 21, 255), 30.0f*s);
    draw->AddRect(phoneMin, phoneMax, IM_COL32(181, 193, 204, 235), 30.0f*s, 0, 1.4f*s);
    draw->AddRectFilled(screenMin, screenMax, unlocked ? IM_COL32(28, 35, 59, 255) : IM_COL32(31, 51, 83, 255), 23.0f*s);

    draw->PushClipRect(screenMin, screenMax, true);
    const float sw = layout.screen.right-layout.screen.left;
    const float sh = layout.screen.bottom-layout.screen.top;
    // Abstract blue/violet glass wallpaper.
    draw->AddCircleFilled(ImVec2(screenMin.x + sw*0.83f, screenMin.y + sh*0.19f), sh*0.29f,
                          IM_COL32(74, 99, 156, 120), 64);
    draw->AddCircleFilled(ImVec2(screenMin.x + sw*0.20f, screenMin.y + sh*0.66f), sh*0.37f,
                          IM_COL32(118, 62, 145, 105), 64);
    draw->AddCircleFilled(ImVec2(screenMin.x + sw*0.84f, screenMin.y + sh*0.82f), sh*0.34f,
                          IM_COL32(25, 133, 153, 85), 64);

    // Status row and dynamic-island detail.
    char timeText[16] = "09:41";
    char dateText[48] = "Vice Side Roleplay";
    std::time_t now = std::time(nullptr);
    std::tm localTime{};
#if defined(__ANDROID__) || defined(__linux__)
    localtime_r(&now, &localTime);
#else
    if (std::tm* tmNow = std::localtime(&now)) localTime = *tmNow;
#endif
    std::strftime(timeText, sizeof(timeText), "%H:%M", &localTime);
    std::strftime(dateText, sizeof(dateText), "%A, %d %B", &localTime);
    if (font) {
        draw->AddText(font, 8.0f*s, ImVec2(screenMin.x+13*s, screenMin.y+8*s), IM_COL32(255,255,255,245), timeText);
    }
    const float islandW = 52.0f*s;
    draw->AddRectFilled(ImVec2((screenMin.x+screenMax.x-islandW)*0.5f, screenMin.y+5*s),
                        ImVec2((screenMin.x+screenMax.x+islandW)*0.5f, screenMin.y+17*s),
                        IM_COL32(2,4,8,255), 8.0f*s);
    const float signalX = screenMax.x-23.0f*s;
    draw->AddLine(ImVec2(signalX,screenMin.y+14*s),ImVec2(signalX,screenMin.y+10*s),IM_COL32(255,255,255,230),1.4f*s);
    draw->AddLine(ImVec2(signalX+4*s,screenMin.y+14*s),ImVec2(signalX+4*s,screenMin.y+7*s),IM_COL32(255,255,255,230),1.4f*s);
    draw->AddLine(ImVec2(signalX+8*s,screenMin.y+14*s),ImVec2(signalX+8*s,screenMin.y+4*s),IM_COL32(255,255,255,230),1.4f*s);

    if (!unlocked) {
        AddCenteredText(draw, dateText, (screenMin.x+screenMax.x)*0.5f,
                        screenMin.y + sh*0.245f, IM_COL32(242,246,255,245), 10.0f*s);
        AddCenteredText(draw, timeText, (screenMin.x+screenMax.x)*0.5f,
                        screenMin.y + sh*0.285f, IM_COL32(255,255,255,255), 39.0f*s);
        const HudRect notification{screenMin.x+sw*0.08f, screenMin.y+sh*0.46f,
                                   screenMax.x-sw*0.08f, screenMin.y+sh*0.57f};
        draw->AddRectFilled(ImVec2(notification.left,notification.top),ImVec2(notification.right,notification.bottom),
                            IM_COL32(14,21,33,184), 12.0f*s);
        draw->AddRect(ImVec2(notification.left,notification.top),ImVec2(notification.right,notification.bottom),
                      IM_COL32(255,255,255,45), 12.0f*s, 0, 1.0f*s);
        if (font) {
            draw->AddText(font, 9.0f*s, ImVec2(notification.left+10*s, notification.top+8*s), IM_COL32(240,190,87,255), "VICE SIDE ROLEPLAY");
            draw->AddText(font, 7.5f*s, ImVec2(notification.left+10*s, notification.top+22*s), IM_COL32(235,240,248,245), "Handphone karakter siap digunakan.");
        }
        const float handleY = screenMax.y - 34.0f*s - swipeOffset;
        draw->AddRectFilled(ImVec2((screenMin.x+screenMax.x)*0.5f-15*s, handleY-11*s),
                            ImVec2((screenMin.x+screenMax.x)*0.5f+15*s, handleY+11*s),
                            IM_COL32(255,255,255,35), 11*s);
        draw->AddLine(ImVec2((screenMin.x+screenMax.x)*0.5f-5*s,handleY+2*s),
                      ImVec2((screenMin.x+screenMax.x)*0.5f,handleY-3*s), IM_COL32(255,255,255,235),1.4f*s);
        draw->AddLine(ImVec2((screenMin.x+screenMax.x)*0.5f,handleY-3*s),
                      ImVec2((screenMin.x+screenMax.x)*0.5f+5*s,handleY+2*s), IM_COL32(255,255,255,235),1.4f*s);
        AddCenteredText(draw, "Geser ke atas untuk buka", (screenMin.x+screenMax.x)*0.5f,
                        screenMax.y-18.0f*s, IM_COL32(232,238,247,235), 7.0f*s);
    } else {
        if (font) {
            draw->AddText(font, 9.5f*s, ImVec2(screenMin.x+14*s,screenMin.y+34*s), IM_COL32(255,255,255,250), "VICE SIDE ROLEPLAY");
            draw->AddText(font, 15.0f*s, ImVec2(screenMin.x+14*s,screenMin.y+51*s), IM_COL32(255,255,255,255), "Aplikasi");
        }
        const HudRect dock{screenMin.x+sw*0.045f, screenMin.y+sh*0.74f,
                           screenMax.x-sw*0.045f, screenMin.y+sh*0.89f};
        draw->AddRectFilled(ImVec2(dock.left,dock.top),ImVec2(dock.right,dock.bottom),IM_COL32(230,239,249,42),12*s);
        draw->AddRect(ImVec2(dock.left,dock.top),ImVec2(dock.right,dock.bottom),IM_COL32(255,255,255,38),12*s,0,1*s);
        const ImU32 tileColors[10] = {
            IM_COL32(204,142,64,235),IM_COL32(76,157,119,235),IM_COL32(101,117,205,235),
            IM_COL32(70,143,195,235),IM_COL32(187,91,122,235),IM_COL32(103,111,126,235),
            IM_COL32(61,166,119,235),IM_COL32(142,153,164,235),IM_COL32(63,133,174,235),IM_COL32(157,92,192,235)
        };
        for (int i = 0; i < 10; ++i) {
            const HudRect& tile = layout.appTiles[i];
            const float iconSize = std::min(tile.bottom-tile.top, tile.right-tile.left) * 0.59f;
            const ImVec2 iconMin((tile.left+tile.right-iconSize)*0.5f, tile.top+2.0f*s);
            const ImVec2 iconMax(iconMin.x+iconSize, iconMin.y+iconSize);
            draw->AddRectFilled(iconMin,iconMax,tileColors[i],8.0f*s);
            const char initial[2] = {PHONE_APP_LABELS[i][0], '\0'};
            AddCenteredText(draw, initial, (iconMin.x+iconMax.x)*0.5f,
                            iconMin.y+iconSize*0.25f, IM_COL32(255,255,255,255), 12.0f*s);
            AddCenteredText(draw, PHONE_APP_LABELS[i], (tile.left+tile.right)*0.5f,
                            iconMax.y+3.0f*s, IM_COL32(249,250,255,245), 6.2f*s);
        }
        draw->AddRectFilled(ImVec2(layout.homeIndicator.left,layout.homeIndicator.top),
                            ImVec2(layout.homeIndicator.right,layout.homeIndicator.bottom),
                            IM_COL32(255,255,255,225), 5*s);
    }
    draw->PopClipRect();

    // Close control stays above the wallpaper.
    const ImVec2 closeCenter((layout.closeButton.left+layout.closeButton.right)*0.5f,
                             (layout.closeButton.top+layout.closeButton.bottom)*0.5f);
    draw->AddCircleFilled(closeCenter, (layout.closeButton.right-layout.closeButton.left)*0.5f,
                          IM_COL32(12,18,28,240), 24);
    DrawCloseIcon(draw, layout.closeButton, s);

    if (toastUntil > ImGui::GetTime() && !toast.empty()) {
        const float toastW = std::min(pw*0.88f, std::max(100.0f*s, toast.size()*6.0f*s));
        const float toastH = 26.0f*s;
        const float tx = (screenMin.x+screenMax.x)*0.5f;
        const float ty = screenMax.y - 52.0f*s;
        draw->AddRectFilled(ImVec2(tx-toastW*0.5f,ty),ImVec2(tx+toastW*0.5f,ty+toastH),IM_COL32(7,12,20,225),10*s);
        AddCenteredText(draw, toast.c_str(), tx, ty+7.0f*s, IM_COL32(255,255,255,245), 7.0f*s);
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
        const ImU32 panelFill = m_pressedAction == TARGET_PANEL_BUTTON ? IM_COL32(64,54,38,248) : IM_COL32(13,21,33,232);
        const ImU32 phoneFill = m_pressedAction == TARGET_PHONE_BUTTON ? IM_COL32(30,65,72,248) : IM_COL32(13,21,33,232);
        draw->AddRectFilled(ImVec2(rects.panel.left,rects.panel.top),ImVec2(rects.panel.right,rects.panel.bottom),panelFill,side*0.25f);
        draw->AddRect(ImVec2(rects.panel.left,rects.panel.top),ImVec2(rects.panel.right,rects.panel.bottom),gold,side*0.25f,0,1.4f*scale);
        draw->AddRectFilled(ImVec2(rects.phone.left,rects.phone.top),ImVec2(rects.phone.right,rects.phone.bottom),phoneFill,side*0.25f);
        draw->AddRect(ImVec2(rects.phone.left,rects.phone.top),ImVec2(rects.phone.right,rects.phone.bottom),cyan,side*0.25f,0,1.4f*scale);
        DrawPanelGlyph(draw, ImVec2((rects.panel.left+rects.panel.right)*0.5f, rects.panel.top+side*0.36f), scale*0.72f, gold);
        DrawPhoneGlyph(draw, ImVec2((rects.phone.left+rects.phone.right)*0.5f, rects.phone.top+side*0.36f), scale*0.69f, cyan);
        AddCenteredText(draw,"PANEL",(rects.panel.left+rects.panel.right)*0.5f,rects.panel.top+side*0.68f,IM_COL32(239,243,248,255),7.2f*scale);
        AddCenteredText(draw,"HP",(rects.phone.left+rects.phone.right)*0.5f,rects.phone.top+side*0.68f,IM_COL32(239,243,248,255),7.5f*scale);
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
                        const float r = wheel.itemRadius * 1.12f;
                        for (int i = 0; i < 4; ++i) {
                            const ImVec2 c = wheel.itemCenters[i];
                            if (std::abs(px-c.x) <= r && std::abs(py-c.y) <= r) { selected = i; break; }
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
