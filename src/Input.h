#pragma once

#include <windows.h>

class Input {
public:
    Input();
    ~Input();

    bool Initialize(HWND hWnd);
    void Update();

    bool IsKeyDown(int key) const;
    bool IsKeyPressed(int key) const;
    bool IsKeyReleased(int key) const;

private:
    bool m_currentKeys[256];
    bool m_previousKeys[256];
    HWND m_hWnd;
};
