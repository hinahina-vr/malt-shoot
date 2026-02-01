#pragma once

#include <windows.h>
#include <mmsystem.h>
#include <thread>
#include <string>

#pragma comment(lib, "winmm.lib")

class SoundManager {
public:
    SoundManager() : m_initialized(false), m_soundPath(L"") {}
    ~SoundManager() {}

    void Initialize() {
        m_initialized = true;
        // Get executable path for sounds
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        std::wstring exePath(path);
        size_t lastSlash = exePath.find_last_of(L"\\/");
        m_soundPath = exePath.substr(0, lastSlash) + L"\\..\\..\\assets\\sounds\\";
    }

    // Non-blocking sound effects using async threads
    void PlayShot() {
        std::thread([]() {
            Beep(1200, 20);
        }).detach();
    }

    void PlayEnemyHit() {
        std::thread([]() {
            Beep(600, 30);
        }).detach();
    }

    void PlayEnemyDestroy() {
        // Play hina_eyao.wav for enemy destruction!
        std::wstring soundFile = m_soundPath + L"hina_eyao.wav";
        PlaySoundW(soundFile.c_str(), nullptr, SND_FILENAME | SND_ASYNC);
    }

    void PlayPlayerHit() {
        // Play hina_buoo.wav when player gets hit
        std::wstring soundFile = m_soundPath + L"hina_buoo.wav";
        PlaySoundW(soundFile.c_str(), nullptr, SND_FILENAME | SND_ASYNC);
    }

    void PlayBomb() {
        std::thread([]() {
            for (int i = 0; i < 5; i++) {
                Beep(100 + i * 30, 30);
            }
        }).detach();
    }

    void PlayItemCollect() {
        std::thread([]() {
            Beep(1000, 15);
            Beep(1200, 15);
        }).detach();
    }

    void PlayGraze() {
        std::thread([]() {
            Beep(1500, 10);
        }).detach();
    }

    void PlaySpecialReady() {
        std::thread([]() {
            Beep(800, 50);
            Beep(1000, 50);
            Beep(1200, 80);
        }).detach();
    }

private:
    bool m_initialized;
    std::wstring m_soundPath;
};
