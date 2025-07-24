#pragma once

#include <Windows.h>
#include <mmsystem.h>
#include <dsound.h>
#include <unordered_map>

#pragma comment(lib, "dsound.lib")

enum class SoundEvent
{
    WallHit,
    PaddleHit,
};

class DirectSoundAudio
{
    LPDIRECTSOUND8          m_pDirectSound;
    LPDIRECTSOUNDBUFFER     m_pPrimarySoundBuffer;
    std::unordered_map<SoundEvent, LPDIRECTSOUNDBUFFER> m_sounds;

public:
    DirectSoundAudio();

    bool Initialize(HWND hwnd);
    void Uninitialize();

    bool LoadWavFile(const char* name, LPDIRECTSOUNDBUFFER& outSoundBuffer);
    bool LoadSound(const char* name, SoundEvent event);

    void Play(SoundEvent event);
};
