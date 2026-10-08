#pragma once
#include "../../game/game.h"
#include "../../game/util.h"

#include "../../main.h"

#include "../../settings.h"
#include "util/CUtil.h"


extern CSettings* pSettings;

class VoiceButton : public Button
{
public:
    VoiceButton() : Button("TALK", UISettings::fontSize() / 2) {
        m_recording = false;
        /* 5:3 aspect ratio */
        m_texture_micro_on = (RwTexture*)CUtil::LoadTextureFromDB("samp", "voiceactive");
        m_texture_micro_off = (RwTexture*)CUtil::LoadTextureFromDB("samp", "voicepassive");
    }

    virtual void draw(ImGuiRenderer* renderer) override
    {
        if (!renderer || !pSettings || !pSettings->Get().bVoiceChatEnable) return;

        if (countdown > 0 && recording() == 1) countdown--;
        if (countdown == 0 && recording() == 1) setRecording(0);

        RwTexture* texture = recording() ? m_texture_micro_on : m_texture_micro_off;
        if (!texture || !texture->raster) {
            // Some game-data packs do not contain the optional voice button textures.
            // Keep the control usable and visible instead of dereferencing a null texture.
            Button::draw(renderer);
            return;
        }

        renderer->drawImage(absolutePosition(), absolutePosition() + size(), texture->raster);
    }

    void touchPopEvent() override
    {
        countdown = 500;
        setRecording(0);
    }

    void touchPushEvent() override
    {
        setRecording(1);
        countdown = 500;
    }

    void setRecording(bool recording)
    {
        if (recording == 1) countdown = 200;
        m_recording = recording;
        this->setCaptionColor(m_recording ? ImColor(1.0f, 0.0f, 0.0f) : ImColor(1.0f, 1.0f, 1.0f));
    }

    bool recording() const { return m_recording; }

private:
    bool m_recording;
    RwTexture* m_texture_micro_on;
    RwTexture* m_texture_micro_off;

public:
    int countdown = 1000;
};
