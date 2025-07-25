#pragma once

#define NOMINMAX
#include <Windows.h>
#include <DirectXMath.h>
#include "Renderer.h"
#include "Audio.h"

enum class GameState
{
    Invalid = 0,
    Initializing,
    LoadingGameEnvironment,
    WaitingForPlayers,
    Running,
};

struct BoundingBox
{
    DirectX::XMFLOAT2 min;
    DirectX::XMFLOAT2 max;

    BoundingBox() { ZeroMemory(this, sizeof(BoundingBox)); }

    bool Intersects(const BoundingBox& box) const;
};

struct Ball
{
    DirectX::XMFLOAT2   pos;
    DirectX::XMFLOAT2   scale;

    DirectX::XMFLOAT2   velocity;
    BoundingBox         bounds;

    Ball() { ZeroMemory(this, sizeof(Ball)); }
};

struct Paddle
{
    DirectX::XMFLOAT2   pos;
    DirectX::XMFLOAT2   scale;

    DirectX::XMFLOAT2   velocity;
    BoundingBox         bounds;

    Paddle() { ZeroMemory(this, sizeof(Paddle)); }
};

class GameApp
{
    HINSTANCE               m_hInst;

    RECT                    m_rcclient;
    HWND                    m_hwnd;

    Renderer                m_renderer;
    Audio                   m_audio;

    bool                    m_key[256];

    GameState               m_state;
    DirectX::XMFLOAT2       m_worldBounds;
    Paddle                  m_paddles[2];
    Ball                    m_ball;
    int                     m_paddleScore1;
    int                     m_paddleScore2;

public:
    GameApp();

    bool Initialize();
    void Run();

    static LRESULT CALLBACK StaticMsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void Uninitialize();

private:
    bool InitWindow();

    void ChangeState(GameState newState);

    void Update(float deltaTime);
    void UpdatePaddle(DirectX::XMFLOAT2& pos, const DirectX::XMFLOAT2& scale, 
        DirectX::XMFLOAT2& velocity, BoundingBox& bounds, float deltaTime);
    void UpdateBall(DirectX::XMFLOAT2& pos, const DirectX::XMFLOAT2& scale, DirectX::XMFLOAT2& velocity, 
        BoundingBox& bounds, Paddle* paddles, size_t numPaddles, float deltaTime);

    void Render();
};

extern GameApp* g_pApp;
