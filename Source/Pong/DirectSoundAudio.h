#pragma once

#include <Windows.h>
#include <mmsystem.h>
#include <dsound.h>

#pragma comment(lib, "dsound.lib")

class DirectSoundAudio
{
    LPDIRECTSOUND8          m_pDirectSound;
    LPDIRECTSOUNDBUFFER     m_pPrimarySoundBuffer;
    LPDIRECTSOUNDBUFFER     m_pWallHitSoundBuffer;
    LPDIRECTSOUNDBUFFER     m_pPaddleHitSoundBuffer;

public:
    DirectSoundAudio();

    bool Initialize(HWND hwnd);
    void Uninitialize();

    bool LoadWavFile(const char* name, LPDIRECTSOUNDBUFFER& soundBuffer);

    void PlayWallHit();
    void PlayPaddleHit();
};
