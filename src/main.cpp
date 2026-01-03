#include "../include/DX12Core.h"
#include "../include/Player.h"
#include "../include/Platform.h"
#include <windows.h>
#include <vector>
#include <memory>

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;

DX12Core* g_dx12Core = nullptr;
Player* g_player = nullptr;
std::vector<std::unique_ptr<Platform>> g_platforms;

bool g_keys[256] = { false };

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_KEYDOWN:
        if (wParam < 256)
            g_keys[wParam] = true;
        return 0;

    case WM_KEYUP:
        if (wParam < 256)
            g_keys[wParam] = false;
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void ProcessInput(float deltaTime)
{
    if (g_keys['A'])
        g_player->MoveLeft(deltaTime);

    if (g_keys['D'])
        g_player->MoveRight(deltaTime);

    if (g_keys['W'])
        g_player->MoveForward(deltaTime);

    if (g_keys['S'])
        g_player->MoveBackward(deltaTime);

    if (g_keys[VK_SPACE])
        g_player->Jump();

    if (g_keys[VK_ESCAPE])
        PostQuitMessage(0);
}

void UpdatePhysics(float deltaTime)
{
    g_player->Update(deltaTime);

    XMFLOAT3 playerPos = g_player->GetPosition();
    XMFLOAT3 playerSize = g_player->GetSize();
    XMFLOAT3 playerVel = g_player->GetVelocity();

    bool wasGrounded = g_player->IsGrounded();
    g_player->SetGrounded(false);

    for (const auto& platform : g_platforms)
    {
        if (platform->CheckCollision(playerPos, playerSize))
        {
            float playerBottom = playerPos.y - playerSize.y * 0.5f;
            float platformTop = platform->GetPosition().y + platform->GetSize().y * 0.5f;
            float platformBottom = platform->GetPosition().y - platform->GetSize().y * 0.5f;

            if (playerVel.y <= 0 && playerBottom <= platformTop && playerBottom >= platformBottom)
            {
                playerPos.y = platformTop + playerSize.y * 0.5f;
                g_player->SetPosition(playerPos);
                g_player->SetVelocity(XMFLOAT3(playerVel.x, 0.0f, playerVel.z));
                g_player->SetGrounded(true);
            }
        }
    }

    if (playerPos.y < -10.0f)
    {
        playerPos.y = 10.0f;
        g_player->SetPosition(playerPos);
        g_player->SetVelocity(XMFLOAT3(0.0f, 0.0f, 0.0f));
    }
}

void Render()
{
    g_dx12Core->BeginFrame();

    XMVECTOR eyePos = XMVectorSet(0.0f, 10.0f, -20.0f, 1.0f);
    XMVECTOR focusPos = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
    XMVECTOR upDir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX viewMatrix = XMMatrixLookAtLH(eyePos, focusPos, upDir);
    g_dx12Core->SetViewMatrix(viewMatrix);

    XMMATRIX projectionMatrix = XMMatrixPerspectiveFovLH(
        XM_PIDIV4,
        static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT),
        0.1f,
        100.0f
    );
    g_dx12Core->SetProjectionMatrix(projectionMatrix);

    g_dx12Core->RenderCube(g_player->GetWorldMatrix(), XMFLOAT4(0.0f, 0.8f, 0.2f, 1.0f));

    for (const auto& platform : g_platforms)
    {
        g_dx12Core->RenderCube(platform->GetWorldMatrix(), platform->GetColor());
    }

    g_dx12Core->EndFrame();
}

void InitializeGame()
{
    g_player = new Player();

    g_platforms.push_back(std::make_unique<Platform>(
        XMFLOAT3(0.0f, 0.0f, 0.0f),
        XMFLOAT3(10.0f, 1.0f, 10.0f),
        XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f)
    ));

    g_platforms.push_back(std::make_unique<Platform>(
        XMFLOAT3(-5.0f, 2.0f, 0.0f),
        XMFLOAT3(3.0f, 1.0f, 3.0f),
        XMFLOAT4(0.8f, 0.3f, 0.3f, 1.0f)
    ));

    g_platforms.push_back(std::make_unique<Platform>(
        XMFLOAT3(5.0f, 4.0f, 0.0f),
        XMFLOAT3(3.0f, 1.0f, 3.0f),
        XMFLOAT4(0.3f, 0.3f, 0.8f, 1.0f)
    ));

    g_platforms.push_back(std::make_unique<Platform>(
        XMFLOAT3(0.0f, 6.0f, 5.0f),
        XMFLOAT3(3.0f, 1.0f, 3.0f),
        XMFLOAT4(0.3f, 0.8f, 0.8f, 1.0f)
    ));
}

void CleanupGame()
{
    delete g_player;
    g_platforms.clear();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"DirectX12_3DPlatformer";

    RegisterClassEx(&wc);

    RECT rect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindowEx(
        0,
        L"DirectX12_3DPlatformer",
        L"DirectX 12 - 3D Platformer",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hwnd)
        return 1;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    g_dx12Core = new DX12Core();
    if (!g_dx12Core->Initialize(hwnd, WINDOW_WIDTH, WINDOW_HEIGHT))
    {
        MessageBox(NULL, L"Failed to initialize DirectX 12", L"Error", MB_OK);
        delete g_dx12Core;
        return 1;
    }

    InitializeGame();

    LARGE_INTEGER frequency;
    LARGE_INTEGER lastTime;
    LARGE_INTEGER currentTime;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&lastTime);

    MSG msg = {};
    bool running = true;

    while (running)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                running = false;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (running)
        {
            QueryPerformanceCounter(&currentTime);
            float deltaTime = static_cast<float>(currentTime.QuadPart - lastTime.QuadPart) / static_cast<float>(frequency.QuadPart);
            lastTime = currentTime;

            if (deltaTime > 0.1f)
                deltaTime = 0.1f;

            ProcessInput(deltaTime);
            UpdatePhysics(deltaTime);
            Render();
        }
    }

    CleanupGame();

    delete g_dx12Core;

    return static_cast<int>(msg.wParam);
}
