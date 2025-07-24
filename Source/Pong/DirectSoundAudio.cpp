#include <cstdio>
#include <cassert>
#include <new>
#include "DirectSoundAudio.h"

#define RELEASE_COM(x) { if (x != nullptr) { x->Release(); x = nullptr; } }

DirectSoundAudio::DirectSoundAudio()
{
    m_pDirectSound          = nullptr;
    m_pPrimarySoundBuffer   = nullptr;
    m_pWallHitSoundBuffer   = nullptr;
    m_pPaddleHitSoundBuffer = nullptr;
}

bool DirectSoundAudio::Initialize(HWND hwnd)
{
        HRESULT hr = S_OK;

    hr = DirectSoundCreate8(nullptr, &m_pDirectSound, nullptr);
    if (FAILED(hr))
        return false;

    //hr = m_pDirectSound->SetCooperativeLevel(m_hwnd, DSSCL_PRIORITY);
    hr = m_pDirectSound->SetCooperativeLevel(hwnd, DSSCL_PRIORITY);
    if (FAILED(hr))
        return false;

    DSBUFFERDESC bufferDesc;
    ZeroMemory(&bufferDesc, sizeof(DSBUFFERDESC));
    bufferDesc.dwSize = sizeof(DSBUFFERDESC);
    bufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
    hr = m_pDirectSound->CreateSoundBuffer(&bufferDesc, &m_pPrimarySoundBuffer, nullptr);
    if (FAILED(hr))
        return false;

    WAVEFORMATEX waveformat;
    ZeroMemory(&waveformat, sizeof(WAVEFORMATEX));
    waveformat.wFormatTag = WAVE_FORMAT_PCM;     
    waveformat.nChannels = 2;      
    waveformat.nSamplesPerSec = 44100; 
    waveformat.wBitsPerSample = 16; 
    waveformat.nBlockAlign = (waveformat.wBitsPerSample / 8) * waveformat.nChannels;    
    waveformat.nAvgBytesPerSec = waveformat.nSamplesPerSec * waveformat.nBlockAlign;       
    hr = m_pPrimarySoundBuffer->SetFormat(&waveformat);
    if (FAILED(hr))
        return false;

    if (!LoadWavFile("Data/WallHit.wav", m_pWallHitSoundBuffer))
        return false;

    if (!LoadWavFile("Data/PaddleHit.wav", m_pPaddleHitSoundBuffer))
        return false;

    if (FAILED(m_pWallHitSoundBuffer->SetVolume(-500)))
        return false;

    if (FAILED(m_pPaddleHitSoundBuffer->SetVolume(-500)))
        return false;

    // For some reason, this shit is necessary to stop the game from freezing on the first Play call...
    if (m_pWallHitSoundBuffer != nullptr)
        m_pWallHitSoundBuffer->Play(0, 0, 0);
    if (m_pPaddleHitSoundBuffer != nullptr)
        m_pPaddleHitSoundBuffer->Play(0, 0, 0);

    return true;
}

void DirectSoundAudio::Uninitialize()
{
    if (m_pWallHitSoundBuffer != nullptr)
        m_pWallHitSoundBuffer->Stop();

    if (m_pPaddleHitSoundBuffer != nullptr)
        m_pPaddleHitSoundBuffer->Stop();

    RELEASE_COM(m_pPaddleHitSoundBuffer);
    RELEASE_COM(m_pWallHitSoundBuffer);
    RELEASE_COM(m_pPrimarySoundBuffer);
    RELEASE_COM(m_pDirectSound);
}

bool DirectSoundAudio::LoadWavFile(const char* name, LPDIRECTSOUNDBUFFER& soundBuffer)
{
    FILE* fp = nullptr;
    if (fopen_s(&fp, name, "rb") != 0)
        return false;
    
    fseek(fp, 0, SEEK_END);
    size_t size = (size_t)ftell(fp);
    if (size < 1)
    {
        fclose(fp);
        return false;
    }
    rewind(fp);

    char* pBuffer = new (std::nothrow) char[size];
    if (pBuffer == nullptr)
    {
        fclose(fp);
        return false; 
    }
    if (fread_s(pBuffer, size, 1, size, fp) != size)
    {
        fclose(fp);
        delete[] pBuffer;
        return false;
    }

    fclose(fp);

    unsigned int type = 0;
    unsigned int length = 0;
    
    unsigned int pos = 0;

    type = *((unsigned int*)(pBuffer + pos));
    pos += sizeof(unsigned int);
    length = *((unsigned int*)(pBuffer + pos));
    pos += sizeof(unsigned int);
    if (type != mmioFOURCC('R', 'I', 'F', 'F'))
    {
        delete[] pBuffer;
        return false;
    }

    type = *((unsigned int*)(pBuffer + pos));
    pos += sizeof(unsigned int);
    if (type != mmioFOURCC('W', 'A', 'V', 'E'))
    {
        delete[] pBuffer;
        return false;
    }

    WAVEFORMATEX waveformat;
    ZeroMemory(&waveformat, sizeof(WAVEFORMATEX));

    unsigned char*  pWaveData = nullptr;
    DWORD           waveDataSize = 0;

    bool copiedWavData = false;
    while (true)
    {
        type = *((unsigned int*)(pBuffer + pos));
        pos += sizeof(unsigned int);
        length = *((unsigned int*)(pBuffer + pos));
        pos += sizeof(unsigned int);

        switch (type)
        {
            case mmioFOURCC('f', 'm', 't', ' '):
                CopyMemory(&waveformat, (pBuffer + pos), length);
                waveformat.cbSize = (WORD)length;
                break;

            case mmioFOURCC('d', 'a', 't', 'a'):
                waveDataSize = length;
                pWaveData = new unsigned char[waveDataSize];
                assert(pWaveData != nullptr && "Out of memory in LoadWavFile().");

                CopyMemory(pWaveData, (pBuffer + pos), length);

                copiedWavData = true;
                
                break;
        }

        pos += length;
        if (length & 1)
            ++pos;

        if (copiedWavData)
            break;
    }
    delete[] pBuffer;

    DSBUFFERDESC bufferDesc;
    ZeroMemory(&bufferDesc, sizeof(DSBUFFERDESC));
    bufferDesc.dwSize = sizeof(DSBUFFERDESC);
    bufferDesc.dwBufferBytes = waveDataSize;
    bufferDesc.dwReserved = 0;
    bufferDesc.lpwfxFormat = &waveformat;
    bufferDesc.guid3DAlgorithm = GUID_NULL;
    bufferDesc.dwFlags = DSBCAPS_CTRLVOLUME;
    HRESULT hr = m_pDirectSound->CreateSoundBuffer(&bufferDesc, &soundBuffer, nullptr);
    if (FAILED(hr))
        return false;

    DWORD status;
    hr = soundBuffer->GetStatus(&status);
    if (FAILED(hr))
        return false;
    if (status & DSBSTATUS_BUFFERLOST)
    {
        int count = 0;
        do 
        {
            hr = soundBuffer->Restore();
            if (hr == DSERR_BUFFERLOST)
                Sleep(10);
        }
        while ((hr = soundBuffer->Restore()) == DSERR_BUFFERLOST && ++count < 100);
    }

    unsigned char*  pLockedSoundBuffer = nullptr;
    DWORD           lockedSoundBufferSize = 0;
    hr = soundBuffer->Lock(0, waveDataSize, (void**)&pLockedSoundBuffer, (DWORD*)&lockedSoundBufferSize, nullptr, 0, 0);
    if (FAILED(hr))
        return false;

    CopyMemory(pLockedSoundBuffer, pWaveData, waveDataSize);
    
    hr = soundBuffer->Unlock((void*)pLockedSoundBuffer, lockedSoundBufferSize, nullptr, 0);
    if (FAILED(hr))
        return false;

    hr = soundBuffer->SetCurrentPosition(0);
    if (FAILED(hr))
        return false;

    delete[] pWaveData;
    return true;
}

void DirectSoundAudio::PlayWallHit()
{
    if (m_pWallHitSoundBuffer != nullptr)
    {
        m_pWallHitSoundBuffer->SetCurrentPosition(0);
        m_pWallHitSoundBuffer->Play(0, 0, 0);
    }
}

void DirectSoundAudio::PlayPaddleHit()
{
    if (m_pPaddleHitSoundBuffer != nullptr)
    {
        m_pPaddleHitSoundBuffer->SetCurrentPosition(0);
        m_pPaddleHitSoundBuffer->Play(0, 0, 0);
    }
}
