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

    m_state                 = GameState::Initializing;
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

    if (!m_audio.Initialize(m_hwnd))
        return false;
    if (!m_audio.LoadSound("Data/WallHit.wav", SoundEvent::WallHit))
        return false;
    if (!m_audio.LoadSound("Data/PaddleHit.wav", SoundEvent::PaddleHit))
        return false;

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
}

void GameApp::Uninitialize()
{
    m_audio.Uninitialize();
    m_renderer.Uninitialize();
}

void GameApp::ChangeState(GameState newState)
{
    m_state = newState;
}

void GameApp::Update(float deltaTime)
{  
    switch (m_state)
    {        
        case GameState::Initializing:
            ChangeState(GameState::LoadingGameEnvironment);
            break;

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
            m_ball.bounds.min.x = m_ball.pos.x - (m_ball.scale.x / 2.0f);
            m_ball.bounds.min.y = m_ball.pos.y - (m_ball.scale.y / 2.0f);
            m_ball.bounds.max.x = m_ball.pos.x + (m_ball.scale.x / 2.0f);
            m_ball.bounds.max.y = m_ball.pos.y + (m_ball.scale.y / 2.0f);

            m_paddle1.pos.x = m_worldBounds.x * 0.05f;
            m_paddle1.pos.y = m_worldBounds.y / 2.0f;
            m_paddle1.scale.x = 10.0f;
            m_paddle1.scale.y = 60.0f;
            m_paddle1.bounds.min.x = m_paddle1.pos.x - (m_paddle1.scale.x / 2.0f);
            m_paddle1.bounds.min.y = m_paddle1.pos.y - (m_paddle1.scale.y / 2.0f);
            m_paddle1.bounds.max.x = m_paddle1.pos.x + (m_paddle1.scale.x / 2.0f);
            m_paddle1.bounds.max.y = m_paddle1.pos.y + (m_paddle1.scale.y / 2.0f);

            m_paddle2.pos.x = m_worldBounds.x * 0.95f;
            m_paddle2.pos.y = m_worldBounds.y / 2.0f;
            m_paddle2.scale.x = 10.0f;
            m_paddle2.scale.y = 60.0f;
            m_paddle2.bounds.min.x = m_paddle2.pos.x - (m_paddle2.scale.x / 2.0f);
            m_paddle2.bounds.min.y = m_paddle2.pos.y - (m_paddle2.scale.y / 2.0f);
            m_paddle2.bounds.max.x = m_paddle2.pos.x + (m_paddle2.scale.x / 2.0f);
            m_paddle2.bounds.max.y = m_paddle2.pos.y + (m_paddle2.scale.y / 2.0f);

            ChangeState(GameState::WaitingForPlayers);

            break;
        }

        case GameState::WaitingForPlayers:
        {
            if (m_key[' '])
                ChangeState(GameState::Running);

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

            UpdatePaddle(m_paddle1.pos, m_paddle1.scale, m_paddle1.velocity, m_paddle1.bounds, deltaTime);
            UpdatePaddle(m_paddle2.pos, m_paddle2.scale, m_paddle2.velocity, m_paddle2.bounds, deltaTime);
            UpdateBall(m_ball.pos, m_ball.scale, m_ball.velocity, m_ball.bounds, deltaTime);

            break;
        }

        default:
            assert(false && "Unrecognized state");
    }
}

void GameApp::UpdatePaddle(DirectX::XMFLOAT2& pos, const DirectX::XMFLOAT2& scale, 
    DirectX::XMFLOAT2& velocity, BoundingBox& bounds, float deltaTime)
{
    pos.x += velocity.x * deltaTime;
    pos.y += velocity.y * deltaTime;

    bounds.min.x = pos.x - (scale.x / 2.0f);
    bounds.min.y = pos.y - (scale.y / 2.0f);
    bounds.max.x = pos.x + (scale.x / 2.0f);
    bounds.max.y = pos.y + (scale.y / 2.0f);

    // Clamp the paddle's Y position to ensure it stays within the top and bottom edges of the world bounds
    if (pos.y + (scale.y / 2.0f) > m_worldBounds.y)
    {
        pos.y = m_worldBounds.y - (scale.y / 2.0f);
    }
    else if (pos.y - (scale.y / 2.0f) < 0.0f)
    {
        pos.y = (scale.y / 2.0f);
    }
}

void GameApp::UpdateBall(DirectX::XMFLOAT2& pos, const DirectX::XMFLOAT2& scale, 
    DirectX::XMFLOAT2& velocity, BoundingBox& bounds, float deltaTime)
{
    pos.x += velocity.x * deltaTime;
    pos.y += velocity.y * deltaTime;

    bounds.min.x = pos.x - (scale.x / 2.0f);
    bounds.min.y = pos.y - (scale.y / 2.0f);
    bounds.max.x = pos.x + (scale.x / 2.0f);
    bounds.max.y = pos.y + (scale.y / 2.0f);

    // Bounce the ball off the top and bottom edges of the world bounds
    if (pos.y + (scale.y / 2.0f) > m_worldBounds.y)
    {
        float penY = (pos.y + (scale.y / 2.0f)) - m_worldBounds.y; // Penetration depth along the Y-axis
        pos.y -= penY;

        bounds.min.x -= penY;
        bounds.min.y -= penY;
        bounds.max.x -= penY;
        bounds.max.y -= penY;

        velocity.y = -velocity.y;
        m_audio.Play(SoundEvent::WallHit);
    }
    else if (pos.y + (scale.y / 2.0f) < 0.0f)
    {
        float penY = 0.0f - (pos.y - (scale.y / 2.0f)); // Penetration depth along the Y-axis
        pos.y += penY;

        bounds.min.x += penY;
        bounds.min.y += penY;
        bounds.max.x += penY;
        bounds.max.y += penY;

        velocity.y = -velocity.y;
        m_audio.Play(SoundEvent::WallHit);
    }

    // Bounce the ball off the left and right paddles
    if (bounds.Intersects(m_paddle1.bounds))
    {
        XMFLOAT2 penetration;

        XMFLOAT2 overlap;
        overlap.x = std::min(bounds.max.x, m_paddle1.bounds.max.x) - std::max(bounds.min.x, m_paddle1.bounds.min.x);
        overlap.y = std::min(bounds.max.y, m_paddle1.bounds.max.y) - std::max(bounds.min.y, m_paddle1.bounds.min.y);
        if (overlap.x > 0.0f && overlap.y > 0.0f)
        {
            // Resolve along the axis of least penetration
            if (overlap.x < overlap.y)
            {
                float direction = (bounds.min.x + bounds.max.x < m_paddle1.bounds.min.x + m_paddle1.bounds.max.x) ? -1.0f : 1.0f;
                penetration.x = direction * overlap.x;
                penetration.y = 0.0f;
            }
            else
            {
                float direction = (bounds.min.y + bounds.max.y < m_paddle1.bounds.min.y + m_paddle1.bounds.max.y) ? -1.0f : 1.0f;
                penetration.x = 0.0f;
                penetration.y = direction * overlap.y;
            }
        }
        else
        {
            penetration.x = 0.0f;
            penetration.y = 0.0f;
        }

        pos.x += penetration.x;
        pos.y += penetration.y;

        bounds.min.x += penetration.x;
        bounds.min.y += penetration.y;
        bounds.max.x += penetration.x;
        bounds.max.y += penetration.y;

        velocity.x = -velocity.x;
        m_audio.Play(SoundEvent::PaddleHit);
    }
    else if (bounds.Intersects(m_paddle2.bounds))
    {
        XMFLOAT2 penetration;

        XMFLOAT2 overlap;
        overlap.x = std::min(bounds.max.x, m_paddle1.bounds.max.x) - std::max(bounds.min.x, m_paddle1.bounds.min.x);
        overlap.y = std::min(bounds.max.y, m_paddle1.bounds.max.y) - std::max(bounds.min.y, m_paddle1.bounds.min.y);
        if (overlap.x > 0.0f && overlap.y > 0.0f)
        {
            // Resolve along the axis of least penetration
            if (overlap.x < overlap.y)
            {
                float direction = (bounds.min.x + bounds.max.x < m_paddle1.bounds.min.x + m_paddle1.bounds.max.x) ? -1.0f : 1.0f;
                penetration.x = direction * overlap.x;
                penetration.y = 0.0f;
            }
            else
            {
                float direction = (bounds.min.y + bounds.max.y < m_paddle1.bounds.min.y + m_paddle1.bounds.max.y) ? -1.0f : 1.0f;
                penetration.x = 0.0f;
                penetration.y = direction * overlap.y;
            }
        }
        else
        {
            penetration.x = 0.0f;
            penetration.y = 0.0f;
        }

        pos.x += penetration.x;
        pos.y += penetration.y;

        bounds.min.x += penetration.x;
        bounds.min.y += penetration.y;
        bounds.max.x += penetration.x;
        bounds.max.y += penetration.y;

        velocity.x = -velocity.x;
        m_audio.Play(SoundEvent::PaddleHit);
    }

    // Check if the ball has passed beyond the left or right edge — update the score accordingly
    if (pos.x < 0.0f)
    {
        ++m_paddleScore2;
        ChangeState(GameState::LoadingGameEnvironment);
    }
    else if (pos.x > m_worldBounds.x)
    {
        ++m_paddleScore1;
        ChangeState(GameState::LoadingGameEnvironment);
    }
}

void GameApp::Render()
{
    m_renderer.PreRender();

    m_renderer.PrepareQuadPass();
    {
        m_renderer.RenderQuad(m_ball.pos, m_ball.scale);
        m_renderer.RenderQuad(m_paddle1.pos, m_paddle2.scale);
        m_renderer.RenderQuad(m_paddle2.pos, m_paddle2.scale);
    }

    m_renderer.PrepareTextPass();
    {

        m_renderer.RenderText(std::to_string(m_paddleScore1), XMFLOAT2(m_worldBounds.x * 0.3f, m_worldBounds.y * 0.8f), 48.0f);
        m_renderer.RenderText(std::to_string(m_paddleScore2), XMFLOAT2(m_worldBounds.x * 0.6f, m_worldBounds.y * 0.8f), 48.0f);

        if (m_state != GameState::Running)
            m_renderer.RenderText("Press SPACE to start", XMFLOAT2(m_worldBounds.x * 0.2f, m_worldBounds.y * 0.6f), 12.0f);
    }

    m_renderer.PostRender();
}

bool BoundingBox::Intersects(const BoundingBox& box) const
{
    return (
        max.x > box.min.x &&
        min.x < box.max.x &&
        max.y > box.min.y &&
        min.y < box.max.y
        );
}   
