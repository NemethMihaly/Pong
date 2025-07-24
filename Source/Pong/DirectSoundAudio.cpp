#include <cstdio>
#include <cassert>
#include <new>
#include "DirectSoundAudio.h"
#include "Debugging/Logger.h"

#define RELEASE_COM(x) { if (x != nullptr) { x->Release(); x = nullptr; } }

DirectSoundAudio::DirectSoundAudio()
{
    m_pDirectSound          = nullptr;
    m_pPrimarySoundBuffer   = nullptr;
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

    return true;
}

void DirectSoundAudio::Uninitialize()
{
    for (auto it = m_sounds.begin(); it != m_sounds.end(); ++it)
    {
        LPDIRECTSOUNDBUFFER pSoundBuffer = it->second;
        if (pSoundBuffer != nullptr)
            pSoundBuffer->Stop();

        RELEASE_COM(pSoundBuffer);
    }
    m_sounds.clear();

    RELEASE_COM(m_pPrimarySoundBuffer);
    RELEASE_COM(m_pDirectSound);
}

bool DirectSoundAudio::LoadWavFile(const char* name, LPDIRECTSOUNDBUFFER& outSoundBuffer)
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
    HRESULT hr = m_pDirectSound->CreateSoundBuffer(&bufferDesc, &outSoundBuffer, nullptr);
    if (FAILED(hr))
        return false;

    DWORD status;
    hr = outSoundBuffer->GetStatus(&status);
    if (FAILED(hr))
        return false;
    if (status & DSBSTATUS_BUFFERLOST)
    {
        int count = 0;
        do 
        {
            hr = outSoundBuffer->Restore();
            if (hr == DSERR_BUFFERLOST)
                Sleep(10);
        }
        while ((hr = outSoundBuffer->Restore()) == DSERR_BUFFERLOST && ++count < 100);
    }

    unsigned char*  pLockedSoundBuffer = nullptr;
    DWORD           lockedSoundBufferSize = 0;
    hr = outSoundBuffer->Lock(0, waveDataSize, (void**)&pLockedSoundBuffer, (DWORD*)&lockedSoundBufferSize, nullptr, 0, 0);
    if (FAILED(hr))
        return false;

    CopyMemory(pLockedSoundBuffer, pWaveData, waveDataSize);
    
    hr = outSoundBuffer->Unlock((void*)pLockedSoundBuffer, lockedSoundBufferSize, nullptr, 0);
    if (FAILED(hr))
        return false;

    hr = outSoundBuffer->SetCurrentPosition(0);
    if (FAILED(hr))
        return false;

    delete[] pWaveData;
    return true;
}

bool DirectSoundAudio::LoadSound(const char* name, SoundEvent event)
{
    LPDIRECTSOUNDBUFFER pSoundBuffer = nullptr;
    if (!LoadWavFile(name, pSoundBuffer))
        return false;
    
    m_sounds[event] = pSoundBuffer;

    return true;
}

void DirectSoundAudio::Play(SoundEvent event)
{
    auto findIt = m_sounds.find(event);
    if (findIt != m_sounds.end())
    {
        LPDIRECTSOUNDBUFFER pSoundBuffer = findIt->second;
        if (pSoundBuffer)
            pSoundBuffer->Play(0, 0, 0);
    }
}
