#include <windows.h>
#include "Game.h"

// ウィンドウプロシージャ
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

// エントリーポイント
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // COM初期化（WICテクスチャ読み込みに必要、MCIと互換性のあるモード）
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    
    // ウィンドウクラスの登録
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"DanmakuShooterClass";

    if (!RegisterClassEx(&wc)) {
        MessageBox(nullptr, L"ウィンドウクラスの登録に失敗しました", L"エラー", MB_OK | MB_ICONERROR);
        return -1;
    }

    // Window size calculation (Full HD - 1920x1080)
    RECT rect = { 0, 0, 1920, 1080 };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    int windowWidth = rect.right - rect.left;
    int windowHeight = rect.bottom - rect.top;

    // Create window
    HWND hWnd = CreateWindow(
        L"DanmakuShooterClass",
        L"Malt-Shoot ~ Danmaku Shooting Game",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowWidth, windowHeight,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hWnd) {
        MessageBox(nullptr, L"ウィンドウの作成に失敗しました", L"エラー", MB_OK | MB_ICONERROR);
        return -1;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // Initialize game
    Game game;
    if (!game.Initialize(hWnd, 1920, 1080)) {
        MessageBox(nullptr, L"ゲームの初期化に失敗しました", L"エラー", MB_OK | MB_ICONERROR);
        return -1;
    }

    // メインループ
    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            game.Update();
            game.Render();
        }
    }

    game.Shutdown();
    return static_cast<int>(msg.wParam);
}
