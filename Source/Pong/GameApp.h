#pragma once

#include <Windows.h>
#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

struct Vertex
{
    DirectX::XMFLOAT3 pos;
};

struct ConstantBuffer_PerFrame
{
    DirectX::XMFLOAT4X4 view;
    DirectX::XMFLOAT4X4 projection;
};

struct ConstantBuffer_PerObject
{
    DirectX::XMFLOAT4X4 world;
};

class GameApp
{
    HINSTANCE   m_hInst;

    RECT        m_rcclient;
    HWND        m_hwnd;

    ID3D11Device*           m_pd3dDevice;
    ID3D11DeviceContext*    m_pd3dDeviceContext;
    IDXGISwapChain*         m_pDXGISwapChain;
    ID3D11RenderTargetView* m_pRenderTargetView;
    D3D11_VIEWPORT          m_viewport;
    ID3D11InputLayout*      m_pVertexLayout;
    ID3D11VertexShader*     m_pVertexShader;
    ID3D11PixelShader*      m_pPixelShader;
    Vertex*                 m_pVerts;
    UINT                    m_numVerts;
    ID3D11Buffer*           m_pVertexBuffer;
    ID3D11Buffer*           m_pcbPerFrame;
    ID3D11Buffer*           m_pcbPerObject;

public:
    GameApp();

    bool Initialize();
    void Run();

    static LRESULT CALLBACK MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void Uninitialize();

private:
    bool InitDevice();

    void Render();
};

extern GameApp* g_pApp;
