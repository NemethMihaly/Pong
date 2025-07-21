#include <cassert>
#include <new>
#include "GameApp.h"
#include "Debugging/Logger.h"

using namespace DirectX;

#define RELEASE_COM(x) { if (x != nullptr) { x->Release(); x = nullptr; } }

GameApp* g_pApp = nullptr;

GameApp::GameApp()
{
    g_pApp		= this;

    m_hInst		= nullptr;

    ZeroMemory(&m_rcclient, sizeof(RECT));
    m_hwnd		= nullptr;

    m_pd3dDevice			= nullptr;
    m_pd3dDeviceContext		= nullptr;
    m_pDXGISwapChain		= nullptr;
    m_pRenderTargetView		= nullptr;
    ZeroMemory(&m_viewport, sizeof(D3D11_VIEWPORT));
    m_pVertexLayout			= nullptr;
    m_pVertexShader			= nullptr;
    m_pPixelShader			= nullptr;
    m_pVerts                = nullptr;
    m_numVerts              = 0;
    m_pVertexBuffer         = nullptr;
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

    return true;
}

void GameApp::Run()
{
    if (!IsWindowVisible(m_hwnd))
        ShowWindow(m_hwnd, SW_SHOW);

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

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

void GameApp::Uninitialize()
{
    if (m_pVerts != nullptr)
        delete[] m_pVerts;

    if (m_pd3dDeviceContext != nullptr)
        m_pd3dDeviceContext->ClearState();

    RELEASE_COM(m_pVertexBuffer);
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
    swapChainDesc.BufferCount = 2;
    swapChainDesc.OutputWindow = m_hwnd;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
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

    m_viewport.TopLeftX = 0;
    m_viewport.TopLeftY = 0;
    m_viewport.Width = (FLOAT)width;
    m_viewport.Height = (FLOAT)height;
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;

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

    m_numVerts = 3;
    m_pVerts = new (std::nothrow) Vertex[m_numVerts];
    assert(m_pVerts != nullptr && "Out of memory in GameApp::InitDevice()");

    m_pVerts[0] = { XMFLOAT3( 0.0f, 0.5f, 0.0f) };
    m_pVerts[1] = { XMFLOAT3( 0.5f,-0.5f, 0.0f) };
    m_pVerts[2] = { XMFLOAT3(-0.5f,-0.5f, 0.0f) };

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

    return true;
}

void GameApp::Render()
{
    float clearColor[] = { 0.0f, 0.125f, 0.25f, 1.0f };
    m_pd3dDeviceContext->ClearRenderTargetView(m_pRenderTargetView, clearColor);

    m_pd3dDeviceContext->RSSetViewports(1, &m_viewport);
    m_pd3dDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, nullptr);

    m_pd3dDeviceContext->IASetInputLayout(m_pVertexLayout);

    m_pd3dDeviceContext->VSSetShader(m_pVertexShader, nullptr, 0);
    m_pd3dDeviceContext->PSSetShader(m_pPixelShader, nullptr, 0);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    m_pd3dDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
    m_pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pd3dDeviceContext->Draw(m_numVerts, 0);

    m_pDXGISwapChain->Present(0, 0);
}
