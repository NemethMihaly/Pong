#pragma once

#include <Windows.h>
#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include "DDSTextureLoader11.h"
#include <DirectXMath.h>
#include <mmsystem.h>
#include <dsound.h>
#include <string>
#include <unordered_map>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

#pragma comment(lib, "dsound.lib")

struct Vertex
{
    DirectX::XMFLOAT3 pos;
};

struct TextVertex
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT2 texCoord;
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

struct Glyph
{
    uint32_t    unicode;
    float       advance;

    float       planeLeft;
    float       planeBottom;
    float       planeRight;
    float       planeTop;

    float       atlasLeft;
    float       atlasBottom;
    float       atlasRight; 
    float       atlasTop;
};

struct FontAtlas
{
    uint32_t size;
    uint32_t width;
    uint32_t height;
    std::unordered_map<uint32_t, Glyph> glyphs;
};

enum class GameState
{
    Invalid = 0,
    LoadingGameEnvironment,
    WaitingForPlayers,
    Running,
};

struct Ball
{
    DirectX::XMFLOAT2 pos;
    DirectX::XMFLOAT2 scale;

    DirectX::XMFLOAT2 velocity;

    Ball() { ZeroMemory(this, sizeof(Ball)); }
};

struct Paddle
{
    DirectX::XMFLOAT2 pos;
    DirectX::XMFLOAT2 scale;

    DirectX::XMFLOAT2 velocity;

    Paddle() { ZeroMemory(this, sizeof(Paddle)); }
};

class GameApp
{
    HINSTANCE               m_hInst;

    RECT                    m_rcclient;
    HWND                    m_hwnd;

    ID3D11Device*           m_pd3dDevice;
    ID3D11DeviceContext*    m_pd3dDeviceContext;
    IDXGISwapChain*         m_pDXGISwapChain;
    ID3D11RenderTargetView* m_pRenderTargetView;
    D3D11_VIEWPORT          m_viewport;

    ID3D11InputLayout*      m_pVertexLayout;
    ID3D11VertexShader*     m_pVertexShader;
    ID3D11PixelShader*      m_pPixelShader;
    
    ID3D11InputLayout*      m_pTextVertexLayout;
    ID3D11VertexShader*     m_pTextVertexShader;
    ID3D11PixelShader*      m_pTextPixelShader;

    Vertex*                 m_pVerts;
    WORD*                   m_pIndices;
    UINT                    m_numVerts;
    UINT                    m_numPolys;
    ID3D11Buffer*           m_pVertexBuffer;
    ID3D11Buffer*           m_pIndexBuffer;
    
    TextVertex*             m_pTextVerts;
    WORD*                   m_pTextIndices;
    UINT                    m_numTextVerts;
    UINT                    m_numTextPolys;
    ID3D11Buffer*           m_pTextVertexBuffer;
    ID3D11Buffer*           m_pTextIndexBuffer;
    ID3D11ShaderResourceView* m_pFontAtlasTextureRV;
    ID3D11SamplerState*     m_pSamplerLinear;

    ID3D11Buffer*           m_pcbPerFrame;
    ID3D11Buffer*           m_pcbPerObject;

    LPDIRECTSOUND8          m_pDirectSound;
    LPDIRECTSOUNDBUFFER     m_pPrimarySoundBuffer;
    LPDIRECTSOUNDBUFFER     m_pWallHitSoundBuffer;
    LPDIRECTSOUNDBUFFER     m_pPaddleHitSoundBuffer;

    GameState               m_state;

    Ball                    m_ball;
    Paddle                  m_paddle1;
    Paddle                  m_paddle2;

    int                     m_paddleScore1;
    int                     m_paddleScore2;

    bool                    m_key[256];

    FontAtlas               m_fontAtlas;

public:
    GameApp();

    bool Initialize();
    void Run();

    static LRESULT CALLBACK MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void OnKeyDown(char c);
    void OnKeyUp(char c);

    void Uninitialize();

private:
    bool InitDevice();
    bool InitSound();
    
    bool LoadFontMetaData();
    bool LoadWavFile(const char* name, LPDIRECTSOUNDBUFFER& soundBuffer);

    void Update(float deltaTime);
    
    void Render();
    void RenderQuad(const DirectX::XMFLOAT2& pos, const DirectX::XMFLOAT2& scale);
    void RenderText(const std::string& str, const DirectX::XMFLOAT2& pos, float size);
};

extern GameApp* g_pApp;
