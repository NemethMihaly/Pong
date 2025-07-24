#pragma once

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

    Renderer                m_renderer;
    Audio                   m_audio;

    GameState               m_state;
    DirectX::XMFLOAT2       m_worldBounds;
    Ball                    m_ball;
    Paddle                  m_paddle1;
    Paddle                  m_paddle2;
    int                     m_paddleScore1;
    int                     m_paddleScore2;

    bool                    m_key[256];

public:
    GameApp();

    bool Initialize();
    void Run();

    static LRESULT CALLBACK MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void OnKeyDown(char c);
    void OnKeyUp(char c);

    void Uninitialize();

private:
    void ChangeState(GameState newState);

    void Update(float deltaTime);
    void Render();
};

extern GameApp* g_pApp;
