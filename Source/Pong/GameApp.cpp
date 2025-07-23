#include <cstdio>
#include <cmath>
#include <cassert>
#include <new>
#include <chrono>
#include "3rdParty/json.hpp"
#include "GameApp.h"
#include "Debugging/Logger.h"

using namespace DirectX;

#define RELEASE_COM(x) { if (x != nullptr) { x->Release(); x = nullptr; } }

GameApp* g_pApp = nullptr;

GameApp::GameApp()
{
    g_pApp		            = this;

    m_hInst		            = nullptr;

    ZeroMemory(&m_rcclient, sizeof(RECT));
    m_hwnd		            = nullptr;

    m_pd3dDevice			= nullptr;
    m_pd3dDeviceContext		= nullptr;
    m_pDXGISwapChain		= nullptr;
    m_pRenderTargetView		= nullptr;
    ZeroMemory(&m_viewport, sizeof(D3D11_VIEWPORT));

    m_pVertexLayout			= nullptr;
    m_pVertexShader			= nullptr;
    m_pPixelShader			= nullptr;
    
    m_pTextVertexLayout     = nullptr;
    m_pTextVertexShader     = nullptr;
    m_pTextPixelShader      = nullptr;

    m_pVerts                = nullptr;
    m_pIndices              = nullptr;
    m_numVerts              = 0;
    m_numPolys              = 0;
    m_pVertexBuffer         = nullptr;
    m_pIndexBuffer          = nullptr;
    
    m_pTextVerts            = nullptr;
    m_pTextIndices          = nullptr;
    m_numTextVerts          = 0;
    m_numTextPolys          = 0;
    m_pTextVertexBuffer     = nullptr;
    m_pTextIndexBuffer      = nullptr;

    m_pcbPerFrame           = nullptr;
    m_pcbPerObject          = nullptr;

    m_pDirectSound          = nullptr;
    m_pPrimarySoundBuffer   = nullptr;
    m_pWallHitSoundBuffer   = nullptr;
    m_pPaddleHitSoundBuffer = nullptr;

    m_state                 = (GameState)0;

    m_paddleScore1 = 0;
    m_paddleScore2 = 0;

    ZeroMemory(&m_key, sizeof(m_key));
}

bool GameApp::Initialize()
{
    if (m_hInst == nullptr)
        m_hInst = GetModuleHandle(nullptr);

    WNDCLASS wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MsgProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = m_hInst;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = L"PongWindowClass";
    if (!RegisterClass(&wc))
        return false;

    SetRect(&m_rcclient, 0, 0, 640, 480);
    AdjustWindowRect(&m_rcclient, WS_OVERLAPPEDWINDOW, FALSE);

    m_hwnd = CreateWindow(L"PongWindowClass", L"Pong", WS_OVERLAPPEDWINDOW, 
        CW_USEDEFAULT, CW_USEDEFAULT, m_rcclient.right - m_rcclient.left, m_rcclient.bottom - m_rcclient.top, 
        nullptr, nullptr, m_hInst, nullptr);
    if (m_hwnd == nullptr)
        return false;

    if (!InitDevice())
        return false;

    if (!InitSound())
        return false;

    m_state = GameState::LoadingGameEnvironment;

    return true;
}

void GameApp::Run()
{
    if (!IsWindowVisible(m_hwnd))
        ShowWindow(m_hwnd, SW_SHOW);

    std::chrono::high_resolution_clock::time_point lastTime =
        std::chrono::high_resolution_clock::now();

    MSG msg;
    ZeroMemory(&msg, sizeof(MSG));
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            std::chrono::high_resolution_clock::time_point currentTime =
                std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<float>(currentTime - lastTime);
            lastTime = currentTime;

            float deltaTime = duration.count();
            Update(deltaTime);

            Render();
        }
    }
}

LRESULT GameApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        case WM_KEYDOWN:
            g_pApp->OnKeyDown((char)wParam);
            break;

        case WM_KEYUP:
            g_pApp->OnKeyUp((char)wParam);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

void GameApp::OnKeyDown(char c)
{
    m_key[(int)c] = true;
}

void GameApp::OnKeyUp(char c)
{
    m_key[(int)c] = false;

    if (c == 'E')
    {
        ++m_paddleScore1;
    }
    else if (c == 'R')
    {
        ++m_paddleScore2;
    }
}

void GameApp::Uninitialize()
{
    if (m_pWallHitSoundBuffer != nullptr)
        m_pWallHitSoundBuffer->Stop();

    if (m_pPaddleHitSoundBuffer != nullptr)
        m_pPaddleHitSoundBuffer->Stop();

    RELEASE_COM(m_pPaddleHitSoundBuffer);
    RELEASE_COM(m_pWallHitSoundBuffer);
    RELEASE_COM(m_pPrimarySoundBuffer);
    RELEASE_COM(m_pDirectSound);

    if (m_pVerts != nullptr)
        delete[] m_pVerts;
    if (m_pIndices != nullptr)
        delete[] m_pIndices;

    if (m_pTextVerts != nullptr)
        delete[] m_pTextVerts;
    if (m_pTextIndices != nullptr)
        delete[] m_pTextIndices;

    if (m_pd3dDeviceContext != nullptr)
        m_pd3dDeviceContext->ClearState();

    RELEASE_COM(m_pcbPerObject);
    RELEASE_COM(m_pcbPerFrame);

    RELEASE_COM(m_pTextIndexBuffer);
    RELEASE_COM(m_pTextVertexBuffer);

    RELEASE_COM(m_pIndexBuffer);
    RELEASE_COM(m_pVertexBuffer);

    RELEASE_COM(m_pTextPixelShader);
    RELEASE_COM(m_pTextVertexShader);
    RELEASE_COM(m_pTextVertexLayout);

    RELEASE_COM(m_pPixelShader);
    RELEASE_COM(m_pVertexShader);
    RELEASE_COM(m_pVertexLayout);
    
    RELEASE_COM(m_pRenderTargetView);
    RELEASE_COM(m_pDXGISwapChain);
    RELEASE_COM(m_pd3dDeviceContext);
    RELEASE_COM(m_pd3dDevice);
}

static HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint,
    LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;

    // Disable optimizations to further improve shader debugging
    dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ID3DBlob* pErrorBlob = nullptr;
    hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel,
        dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
    if (FAILED(hr))
    {
        if (pErrorBlob)
        {
            LOG("CompileShaderFromFile", Error, (const char*)pErrorBlob->GetBufferPointer());
            pErrorBlob->Release();
        }

        return hr;
    }

    if (pErrorBlob)
        pErrorBlob->Release();

    return S_OK;
}

bool GameApp::InitDevice()
{
    HRESULT hr = S_OK;

    UINT width = m_rcclient.right - m_rcclient.left;
    UINT height = m_rcclient.bottom - m_rcclient.top;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
    swapChainDesc.BufferDesc.Width = width;
    swapChainDesc.BufferDesc.Height = height;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 1;
    swapChainDesc.OutputWindow = m_hwnd;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapChainDesc.Flags = 0;

    D3D_FEATURE_LEVEL featureLevel;
    hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, nullptr, 0, D3D11_SDK_VERSION,
        &swapChainDesc, &m_pDXGISwapChain, &m_pd3dDevice, &featureLevel, &m_pd3dDeviceContext);
    if (FAILED(hr))
        return false;

    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = m_pDXGISwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (FAILED(hr))
        return false;

    hr = m_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_pRenderTargetView);
    pBackBuffer->Release();
    if (FAILED(hr))
        return false;

    m_viewport.TopLeftX = 0.0f;
    m_viewport.TopLeftY = 0.0f;
    m_viewport.Width = (FLOAT)width;
    m_viewport.Height = (FLOAT)height;
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;

    {
        ID3DBlob* pVSBlob = nullptr;
        hr = CompileShaderFromFile(L"Data/Shader.hlsl", "VSMain", "vs_4_0", &pVSBlob);
        if (FAILED(hr))
            return false;

        D3D11_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        UINT numElements = ARRAYSIZE(inputElementDescs);
        hr = m_pd3dDevice->CreateInputLayout(inputElementDescs, numElements, pVSBlob->GetBufferPointer(),
            pVSBlob->GetBufferSize(), &m_pVertexLayout);
        if (FAILED(hr))
            return false;

        hr = m_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(),
            nullptr, &m_pVertexShader);
        pVSBlob->Release();
        if (FAILED(hr))
            return false;

        ID3DBlob* pPSBlob = nullptr;
        hr = CompileShaderFromFile(L"Data/Shader.hlsl", "PSMain", "ps_4_0", &pPSBlob);
        if (FAILED(hr))
            return false;
        
        hr = m_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(),
            nullptr, &m_pPixelShader);
        pPSBlob->Release();
        if (FAILED(hr))
            return false;
    }

    {
        ID3DBlob* pVSBlob = nullptr;
        hr = CompileShaderFromFile(L"Data/TextShader.hlsl", "VSMain", "vs_4_0", &pVSBlob);
        if (FAILED(hr))
            return false;

        D3D11_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        UINT numElements = ARRAYSIZE(inputElementDescs);
        hr = m_pd3dDevice->CreateInputLayout(inputElementDescs, numElements, pVSBlob->GetBufferPointer(),
            pVSBlob->GetBufferSize(), &m_pTextVertexLayout);
        if (FAILED(hr))
            return false;

        hr = m_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(),
            nullptr, &m_pTextVertexShader);
        pVSBlob->Release();
        if (FAILED(hr))
            return false;

        ID3DBlob* pPSBlob = nullptr;
        hr = CompileShaderFromFile(L"Data/TextShader.hlsl", "PSMain", "ps_4_0", &pPSBlob);
        if (FAILED(hr))
            return false;

        hr = m_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(),
            nullptr, &m_pTextPixelShader);
        pPSBlob->Release();
        if (FAILED(hr))
            return false;
    }

    {
        // A --- B
        // |   / |
        // | /   | 
        // C --- D

        m_numVerts = 4;
        m_pVerts = new (std::nothrow) Vertex[m_numVerts];
        assert(m_pVerts != nullptr && "Out of memory in GameApp::InitDevice()");

        m_pVerts[0] = { XMFLOAT3( 0.5f, 0.5f, 0.0f) };
        m_pVerts[1] = { XMFLOAT3(-0.5f, 0.5f, 0.0f) };
        m_pVerts[2] = { XMFLOAT3( 0.5f,-0.5f, 0.0f) };
        m_pVerts[3] = { XMFLOAT3(-0.5f,-0.5f, 0.0f) };

        D3D11_BUFFER_DESC bufferDesc;
        ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
        bufferDesc.ByteWidth = sizeof(Vertex) * m_numVerts;
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.CPUAccessFlags = 0;
        D3D11_SUBRESOURCE_DATA initialData;
        ZeroMemory(&initialData, sizeof(D3D11_SUBRESOURCE_DATA));
        initialData.pSysMem = m_pVerts;
        hr = m_pd3dDevice->CreateBuffer(&bufferDesc, &initialData, &m_pVertexBuffer);
        if (FAILED(hr))
            return false;

        // A --- B
        // |   / |
        // | /   | 
        // C --- D

        m_numPolys = 2;
        m_pIndices = new (std::nothrow) WORD[m_numPolys * 3];
        assert(m_pIndices != nullptr && "Out of memory in GameApp::InitDevice()");

        // Triangle #1: ACB
        m_pIndices[0] = 0;
        m_pIndices[1] = 2;
        m_pIndices[2] = 1;

        // Triangle #2: BCD
        m_pIndices[3] = 1;
        m_pIndices[4] = 2;
        m_pIndices[5] = 3;

        bufferDesc.ByteWidth = sizeof(WORD) * m_numPolys * 3;
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        bufferDesc.CPUAccessFlags = 0;
        initialData.pSysMem = m_pIndices;
        hr = m_pd3dDevice->CreateBuffer(&bufferDesc, &initialData, &m_pIndexBuffer);
        if (FAILED(hr))
            return false;
    }

    {
        // A --- B
        // |   / |
        // | /   | 
        // C --- D

        m_numTextVerts = 4;
        m_pTextVerts = new (std::nothrow) TextVertex[m_numTextVerts];
        assert(m_pTextVerts != nullptr && "Out of memory in GameApp::InitDevice()");

        m_pTextVerts[0] = { XMFLOAT3( 0.5f, 0.5f, 0.0f), XMFLOAT2(1.0f, 0.0f) };
        m_pTextVerts[1] = { XMFLOAT3(-0.5f, 0.5f, 0.0f), XMFLOAT2(0.0f, 0.0f) };
        m_pTextVerts[2] = { XMFLOAT3( 0.5f,-0.5f, 0.0f), XMFLOAT2(1.0f, 1.0f) };
        m_pTextVerts[3] = { XMFLOAT3(-0.5f,-0.5f, 0.0f), XMFLOAT2(0.0f, 1.0f) };

        D3D11_BUFFER_DESC bufferDesc;
        ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
        bufferDesc.ByteWidth = sizeof(TextVertex) * m_numTextVerts;
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.CPUAccessFlags = 0;
        D3D11_SUBRESOURCE_DATA initialData;
        ZeroMemory(&initialData, sizeof(D3D11_SUBRESOURCE_DATA));
        initialData.pSysMem = m_pTextVerts;
        hr = m_pd3dDevice->CreateBuffer(&bufferDesc, &initialData, &m_pTextVertexBuffer);
        if (FAILED(hr))
            return false;   
            
        // A --- B
        // |   / |
        // | /   | 
        // C --- D

        m_numTextPolys = 2;
        m_pTextIndices = new (std::nothrow) WORD[m_numTextPolys * 3];
        assert(m_pTextIndices != nullptr && "Out of memory in GameApp::InitDevice()");

        // Triangle #1: ACB
        m_pTextIndices[0] = 0;
        m_pTextIndices[1] = 2;
        m_pTextIndices[2] = 1;

        // Triangle #2: BCD
        m_pTextIndices[3] = 1;
        m_pTextIndices[4] = 2;
        m_pTextIndices[5] = 3;

        bufferDesc.ByteWidth = sizeof(WORD) * m_numTextPolys * 3;
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        bufferDesc.CPUAccessFlags = 0;
        initialData.pSysMem = m_pTextIndices;
        hr = m_pd3dDevice->CreateBuffer(&bufferDesc, &initialData, &m_pTextIndexBuffer);
        if (FAILED(hr))
            return false;
    }

    D3D11_BUFFER_DESC bufferDesc;
    ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
    bufferDesc.ByteWidth = sizeof(ConstantBuffer_PerFrame);
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = m_pd3dDevice->CreateBuffer(&bufferDesc, nullptr, &m_pcbPerFrame);
    if (FAILED(hr))
        return false;

    bufferDesc.ByteWidth = sizeof(ConstantBuffer_PerObject);
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = m_pd3dDevice->CreateBuffer(&bufferDesc, nullptr, &m_pcbPerObject);
    if (FAILED(hr))
        return false;

    return true;
}

bool GameApp::InitSound()
{
    HRESULT hr = S_OK;

    hr = DirectSoundCreate8(nullptr, &m_pDirectSound, nullptr);
    if (FAILED(hr))
        return false;

    hr = m_pDirectSound->SetCooperativeLevel(m_hwnd, DSSCL_PRIORITY);
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

    return true;
}

bool GameApp::LoadWavFile(const char* name, LPDIRECTSOUNDBUFFER& soundBuffer)
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

    unsigned char*  pLockedSoundBuffer = nullptr;
    DWORD           lockedSoundBufferSize = 0;
    hr = soundBuffer->Lock(0, waveDataSize, (void**)&pLockedSoundBuffer, (DWORD*)&lockedSoundBufferSize, nullptr, 0, 0);
    if (FAILED(hr))
        return false;

    CopyMemory(pLockedSoundBuffer, pWaveData, waveDataSize);
    
    hr = soundBuffer->Unlock((void*)pLockedSoundBuffer, lockedSoundBufferSize, nullptr, 0);
    if (FAILED(hr))
        return false;

    return true;
}

void GameApp::Update(float deltaTime)
{  
    switch (m_state)
    {        
        case GameState::LoadingGameEnvironment:
        {
            m_ball.pos.x = m_viewport.Width / 2.0f;
            m_ball.pos.y = m_viewport.Height / 2.0f;
            m_ball.scale.x = 10.0f;
            m_ball.scale.y = 10.0f;
            m_ball.velocity.x = -350.0f;
            m_ball.velocity.y = 300.0f;

            m_paddle1.pos.x = m_viewport.Width * 0.1f;
            m_paddle1.pos.y = m_viewport.Height / 2.0f;
            m_paddle1.scale.x = 10.0f;
            m_paddle1.scale.y = 60.0f;

            m_paddle2.pos.x = m_viewport.Width * 0.9f;
            m_paddle2.pos.y = m_viewport.Height / 2.0f;
            m_paddle2.scale.x = 10.0f;
            m_paddle2.scale.y = 60.0f;

            m_state = GameState::WaitingForPlayers;

            break;
        }

        case GameState::WaitingForPlayers:
        {
            LOG("GameApp", Info, "Press SPACE to start");

            if (m_key[' '])
                m_state = GameState::Running;

            if (m_paddleScore1 >= 5 || m_paddleScore2 >= 5)
            {
                PostQuitMessage(0);
            }

            break;
        }

        case GameState::Running:
        {
            // Set paddle 1's velocity based on player input
            if (m_key['W'])
            {
                m_paddle1.velocity.y = 300.0f;
            }
            else if (m_key['S'])
            {
                m_paddle1.velocity.y = -300.0f;
            }
            else
            {
                m_paddle1.velocity.y = 0.0f;
            }

            // Set paddle 2's velocity based on AI logic
            if (m_paddle2.pos.y < m_ball.pos.y)
            {
                m_paddle2.velocity.y = 290.0f;
            }
            else if (m_paddle2.pos.y > m_ball.pos.y)
            {
                m_paddle2.velocity.y = -290.0f;
            }
            else
            {
                m_paddle2.velocity.y = 0.0f;
            }

            // Update paddle positions based on their velocities
            m_paddle1.pos.x += m_paddle1.velocity.x * deltaTime;
            m_paddle1.pos.y += m_paddle1.velocity.y * deltaTime;
            m_paddle2.pos.x += m_paddle2.velocity.x * deltaTime;
            m_paddle2.pos.y += m_paddle2.velocity.y * deltaTime;

            // Clamp the paddle's Y position to ensure it stays within the top and bottom edges of the viewport
            if (m_paddle1.pos.y + m_paddle1.scale.y / 2.0f > m_viewport.Height)
            {
                m_paddle1.pos.y = m_viewport.Height - m_paddle1.scale.y / 2.0f;
            }
            else if (m_paddle1.pos.y - m_paddle1.scale.y / 2.0f < 0.0f)
            {
                m_paddle1.pos.y = m_paddle1.scale.y / 2.0f;
            }

            if (m_paddle2.pos.y + m_paddle2.scale.y / 2.0f > m_viewport.Height)
            {
                m_paddle2.pos.y = m_viewport.Height - m_paddle2.scale.y / 2.0f;
            }
            else if (m_paddle2.pos.y - m_paddle2.scale.y / 2.0f < 0.0f)
            {
                m_paddle2.pos.y = m_paddle2.scale.y / 2.0f;
            }

            // Move the ball based on its velocity
            m_ball.pos.x += m_ball.velocity.x * deltaTime;
            m_ball.pos.y += m_ball.velocity.y * deltaTime;

            // Bounce the ball off the top and bottom edges of the viewport
            if (m_ball.pos.y + m_ball.scale.y / 2.0f > m_viewport.Height)
            {
                m_ball.velocity.y = -m_ball.velocity.y;
                m_pWallHitSoundBuffer->Play(0, 0, 0);
            }
            else if (m_ball.pos.y + m_ball.scale.y / 2.0f < 0.0f)
            {
                m_ball.velocity.y = -m_ball.velocity.y;
                m_pWallHitSoundBuffer->Play(0, 0, 0);
            }

            // Bounce the ball off the left and right paddles
            float deltaPosY = std::abs(m_ball.pos.y - m_paddle2.pos.y);
            if (m_ball.pos.x + m_ball.scale.x / 2.0f > m_paddle2.pos.x - m_paddle2.scale.x / 2.0f &&
                deltaPosY <= m_paddle2.scale.y / 2.0f)
            {
                m_ball.velocity.x = -m_ball.velocity.x;
                m_pPaddleHitSoundBuffer->Play(0, 0, 0);
            }

            deltaPosY = std::abs(m_ball.pos.y - m_paddle1.pos.y);
            if (m_ball.pos.x - m_ball.scale.x / 2.0f < m_paddle1.pos.x + m_paddle1.scale.x / 2.0f &&
                deltaPosY <= m_paddle1.scale.y / 2.0f)
            {
                m_ball.velocity.x = -m_ball.velocity.x;
                m_pPaddleHitSoundBuffer->Play(0, 0, 0);
            }

            // Check if the ball has passed beyond the left or right edge — update the score accordingly
            if (m_ball.pos.x < 0.0f)
            {
                ++m_paddleScore2;
                m_state = GameState::LoadingGameEnvironment;
            }
            else if (m_ball.pos.x > m_viewport.Width)
            {
                ++m_paddleScore1;
                m_state = GameState::LoadingGameEnvironment;
            }

            break;
        }

        default:
            assert(0 && "Unrecognized state");
    }
}

void GameApp::Render()
{
    {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));

        m_pd3dDeviceContext->Map(m_pcbPerFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

        ConstantBuffer_PerFrame* pPerFrame = (ConstantBuffer_PerFrame*)mappedResource.pData;
        XMStoreFloat4x4(&pPerFrame->view, XMMatrixTranspose(XMMatrixIdentity()));
        XMStoreFloat4x4(&pPerFrame->projection,
            XMMatrixTranspose(
                XMMatrixOrthographicOffCenterLH(
                    0.0f, m_viewport.Width, 0.0f, m_viewport.Height, -1.0f, 1.0f
                    )
                )
            );

        m_pd3dDeviceContext->Unmap(m_pcbPerFrame, 0);
    }

    float clearColor[] = { 0.0f, 0.125f, 0.25f, 1.0f };
    m_pd3dDeviceContext->ClearRenderTargetView(m_pRenderTargetView, clearColor);

    m_pd3dDeviceContext->RSSetViewports(1, &m_viewport);
    m_pd3dDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, nullptr);

    m_pd3dDeviceContext->IASetInputLayout(m_pVertexLayout);

    m_pd3dDeviceContext->VSSetShader(m_pVertexShader, nullptr, 0);
    m_pd3dDeviceContext->VSSetConstantBuffers(0, 1, &m_pcbPerFrame);
    m_pd3dDeviceContext->VSSetConstantBuffers(1, 1, &m_pcbPerObject);
    m_pd3dDeviceContext->PSSetShader(m_pPixelShader, nullptr, 0);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    m_pd3dDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
    m_pd3dDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    m_pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    // Draw the ball
    RenderQuad(m_ball.pos, m_ball.scale);

    // Draw the left (1) paddle
    RenderQuad(m_paddle1.pos, m_paddle2.scale);

    // Draw the right (2) paddle
    RenderQuad(m_paddle2.pos, m_paddle2.scale);

    // Draw the score for the paddle (1)
    for (int i = 0; i < m_paddleScore1; ++i)
    {
        float offsetX = 10.0f * (float)i;
        XMFLOAT2 pos;
        pos.x = m_viewport.Width * 0.4f + offsetX;
        pos.y = m_viewport.Height * 0.8f;

        RenderQuad(pos, XMFLOAT2(6.0f, 6.0f));
    }

    // Draw the score for the paddle (2)
    for (int i = 0; i < m_paddleScore2; ++i)
    {
        float offsetX = 10.0f * (float)i;
        XMFLOAT2 pos;
        pos.x = m_viewport.Width * 0.6f + offsetX;
        pos.y = m_viewport.Height * 0.8f;

        RenderQuad(pos, XMFLOAT2(6.0f, 6.0f));
    }

    // Render texts:

    m_pd3dDeviceContext->IASetInputLayout(m_pTextVertexLayout);

    m_pd3dDeviceContext->VSSetShader(m_pTextVertexShader, nullptr, 0);
    m_pd3dDeviceContext->VSSetConstantBuffers(0, 1, &m_pcbPerFrame);
    m_pd3dDeviceContext->VSSetConstantBuffers(1, 1, &m_pcbPerObject);
    m_pd3dDeviceContext->PSSetShader(m_pTextPixelShader, nullptr, 0);

    stride = sizeof(TextVertex);
    offset = 0;
    m_pd3dDeviceContext->IASetVertexBuffers(0, 1, &m_pTextVertexBuffer, &stride, &offset);
    m_pd3dDeviceContext->IASetIndexBuffer(m_pTextIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    m_pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    RenderQuad(XMFLOAT2(m_viewport.Width / 2.0f, m_viewport.Height / 2.0f), XMFLOAT2(64.0f, 64.0f));

    m_pDXGISwapChain->Present(0, 0);
}

void GameApp::RenderQuad(const DirectX::XMFLOAT2& pos, const DirectX::XMFLOAT2& scale)
{
    {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));

        m_pd3dDeviceContext->Map(m_pcbPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

        ConstantBuffer_PerObject* pPerObject = (ConstantBuffer_PerObject*)mappedResource.pData;
        XMStoreFloat4x4(&pPerObject->world, XMMatrixTranspose(
                XMMatrixScalingFromVector(XMLoadFloat2(&scale)) *
                XMMatrixTranslationFromVector(XMLoadFloat2(&pos))
            )
        );

        m_pd3dDeviceContext->Unmap(m_pcbPerObject, 0);
    }
    
    m_pd3dDeviceContext->DrawIndexed(m_numPolys * 3, 0, 0);
}
