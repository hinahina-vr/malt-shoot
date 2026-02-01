#include "Input.h"

Input::Input()
    : m_hWnd(nullptr)
{
    memset(m_currentKeys, 0, sizeof(m_currentKeys));
    memset(m_previousKeys, 0, sizeof(m_previousKeys));
}

Input::~Input() {
}

bool Input::Initialize(HWND hWnd) {
    m_hWnd = hWnd;
    return true;
}

void Input::Update() {
    memcpy(m_previousKeys, m_currentKeys, sizeof(m_previousKeys));
    
    for (int i = 0; i < 256; i++) {
        m_currentKeys[i] = (GetAsyncKeyState(i) & 0x8000) != 0;
    }
}

bool Input::IsKeyDown(int key) const {
    return m_currentKeys[key];
}

bool Input::IsKeyPressed(int key) const {
    return m_currentKeys[key] && !m_previousKeys[key];
}

bool Input::IsKeyReleased(int key) const {
    return !m_currentKeys[key] && m_previousKeys[key];
}
