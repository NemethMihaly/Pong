#include <cstdio>
#include <cstring>
#include <cmath>
#include <cassert>
#include <new>
#include <chrono>
#include "GameApp.h"
#include "Debugging/Logger.h"

using namespace DirectX;

GameApp* g_pApp = nullptr;

GameApp::GameApp()
{
    g_pApp		            = this;

    m_hInst		            = nullptr;

    ZeroMemory(&m_rcclient, sizeof(RECT));
    m_hwnd		            = nullptr;

    m_state                 = (GameState)0;
    m_paddleScore1          = 0;
    m_paddleScore2          = 0;

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

    GetClientRect(m_hwnd, &m_rcclient);

    if (!m_renderer.Initialize(m_hwnd))
        return false;
    if (!m_renderer.LoadFontMetaData())
        return false;

    if (!m_directSoundAudio.Initialize(m_hwnd))
        return false;
    if (!m_directSoundAudio.LoadSound("Data/WallHit.wav", SoundEvent::WallHit))
        return false;
    if (!m_directSoundAudio.LoadSound("Data/PaddleHit.wav", SoundEvent::PaddleHit))
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
    m_directSoundAudio.Uninitialize();
    m_renderer.Uninitialize();
}

void GameApp::Update(float deltaTime)
{  
    switch (m_state)
    {        
        case GameState::LoadingGameEnvironment:
        {
            m_worldBounds.x = 640.0f;
            m_worldBounds.y = 480.0f;

            m_ball.pos.x = m_worldBounds.x / 2.0f;
            m_ball.pos.y = m_worldBounds.y / 2.0f;
            m_ball.scale.x = 10.0f;
            m_ball.scale.y = 10.0f;
            m_ball.velocity.x = -350.0f;
            m_ball.velocity.y = 300.0f;

            m_paddle1.pos.x = m_worldBounds.x * 0.05f;
            m_paddle1.pos.y = m_worldBounds.y / 2.0f;
            m_paddle1.scale.x = 10.0f;
            m_paddle1.scale.y = 60.0f;

            m_paddle2.pos.x = m_worldBounds.x * 0.95f;
            m_paddle2.pos.y = m_worldBounds.y / 2.0f;
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
            if (m_paddle1.pos.y + m_paddle1.scale.y / 2.0f > m_worldBounds.y)
            {
                m_paddle1.pos.y = m_worldBounds.y - m_paddle1.scale.y / 2.0f;
            }
            else if (m_paddle1.pos.y - m_paddle1.scale.y / 2.0f < 0.0f)
            {
                m_paddle1.pos.y = m_paddle1.scale.y / 2.0f;
            }

            if (m_paddle2.pos.y + m_paddle2.scale.y / 2.0f > m_worldBounds.y)
            {
                m_paddle2.pos.y = m_worldBounds.y - m_paddle2.scale.y / 2.0f;
            }
            else if (m_paddle2.pos.y - m_paddle2.scale.y / 2.0f < 0.0f)
            {
                m_paddle2.pos.y = m_paddle2.scale.y / 2.0f;
            }

            // Move the ball based on its velocity
            m_ball.pos.x += m_ball.velocity.x * deltaTime;
            m_ball.pos.y += m_ball.velocity.y * deltaTime;

            // Bounce the ball off the top and bottom edges of the viewport
            if (m_ball.pos.y + m_ball.scale.y / 2.0f > m_worldBounds.y &&
                m_ball.velocity.y > 0.0f)
            {
                m_ball.velocity.y = -m_ball.velocity.y;
                m_directSoundAudio.Play(SoundEvent::WallHit);
            }
            else if (m_ball.pos.y + m_ball.scale.y / 2.0f < 0.0f &&
                m_ball.velocity.y < 0.0f)
            {
                m_ball.velocity.y = -m_ball.velocity.y;
                m_directSoundAudio.Play(SoundEvent::WallHit);
            }

            // Bounce the ball off the left and right paddles
            float deltaPosY = std::abs(m_ball.pos.y - m_paddle2.pos.y);
            if (m_ball.pos.x + m_ball.scale.x / 2.0f > m_paddle2.pos.x - m_paddle2.scale.x / 2.0f &&
                m_ball.pos.x - m_ball.scale.x / 2.0f < m_paddle2.pos.x + m_paddle2.scale.x / 2.0f &&
                deltaPosY <= m_paddle2.scale.y / 2.0f &&
                m_ball.velocity.x > 0.0f)
            {
                m_ball.velocity.x = -m_ball.velocity.x;
                m_directSoundAudio.Play(SoundEvent::PaddleHit);
            }

            deltaPosY = std::abs(m_ball.pos.y - m_paddle1.pos.y);
            if (m_ball.pos.x - m_ball.scale.x / 2.0f < m_paddle1.pos.x + m_paddle1.scale.x / 2.0f &&
                m_ball.pos.x + m_ball.scale.x / 2.0f > m_paddle1.pos.x - m_paddle1.scale.x / 2.0f &&
                deltaPosY <= m_paddle1.scale.y / 2.0f &&
                m_ball.velocity.x < 0.0f)
            {
                m_ball.velocity.x = -m_ball.velocity.x;
                m_directSoundAudio.Play(SoundEvent::PaddleHit);
            }

            // Check if the ball has passed beyond the left or right edge — update the score accordingly
            if (m_ball.pos.x < 0.0f)
            {
                ++m_paddleScore2;
                m_state = GameState::LoadingGameEnvironment;
            }
            else if (m_ball.pos.x > m_worldBounds.x)
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
    m_renderer.PreRender();

    m_renderer.PrepareQuadPass();

    // Draw the ball
    m_renderer.RenderQuad(m_ball.pos, m_ball.scale);

    // Draw the left (1) paddle
    m_renderer.RenderQuad(m_paddle1.pos, m_paddle2.scale);

    // Draw the right (2) paddle
    m_renderer.RenderQuad(m_paddle2.pos, m_paddle2.scale);

    // Render texts:
    m_renderer.PrepareTextPass();

    m_renderer.RenderText(std::to_string(m_paddleScore1), XMFLOAT2(m_worldBounds.x * 0.3f, m_worldBounds.y * 0.8f), 48.0f);
    m_renderer.RenderText(std::to_string(m_paddleScore2), XMFLOAT2(m_worldBounds.x * 0.6f, m_worldBounds.y * 0.8f), 48.0f);

    m_renderer.RenderText("Hello, World!", XMFLOAT2(m_worldBounds.x * 0.2f, m_worldBounds.y * 0.6f), 12.0f);

    m_renderer.PostRender();
}
