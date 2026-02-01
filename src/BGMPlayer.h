#pragma once

#include <windows.h>
#include <mmsystem.h>
#include <string>

#pragma comment(lib, "winmm.lib")

class BGMPlayer {
public:
    BGMPlayer() : m_initialized(false), m_isPlaying(false) {}
    ~BGMPlayer() { Stop(); }

    void Initialize() {
        // Get executable path for sounds
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        std::wstring exePath(path);
        size_t lastSlash = exePath.find_last_of(L"\\/");
        m_soundPath = exePath.substr(0, lastSlash) + L"\\..\\..\\assets\\sounds\\";
        m_initialized = true;
    }

    void PlayStageBGM() {
        PlayBGM(L"bgm_stage1_normal.mp3");
    }

    void PlayBossBGM() {
        PlayBGM(L"bgm_stage1_boss.mp3");
    }

    void PlayTitleBGM() {
        PlayBGM(L"bgm_op.mp3");
    }

    void PlayScoreBGM() {
        PlayBGM(L"bgm_ed.mp3");
    }

    void Stop() {
        if (m_isPlaying) {
            mciSendStringW(L"stop bgm", nullptr, 0, nullptr);
            mciSendStringW(L"close bgm", nullptr, 0, nullptr);
            m_isPlaying = false;
        }
    }

    void SetVolume(int volume) {
        // Volume: 0-1000
        m_volume = volume;  // 保存
        wchar_t cmd[256];
        swprintf_s(cmd, L"setaudio bgm volume to %d", volume);
        mciSendStringW(cmd, nullptr, 0, nullptr);
    }

private:
    void PlayBGM(const std::wstring& filename) {
        Stop();
        
        std::wstring fullPath = m_soundPath + filename;
        
        // Open the audio file
        std::wstring openCmd = L"open \"" + fullPath + L"\" type mpegvideo alias bgm";
        MCIERROR err = mciSendStringW(openCmd.c_str(), nullptr, 0, nullptr);
        if (err != 0) return;
        
        // Set volume to saved value（保存した音量を適用）
        wchar_t cmd[256];
        swprintf_s(cmd, L"setaudio bgm volume to %d", m_volume);
        mciSendStringW(cmd, nullptr, 0, nullptr);
        
        // Play with repeat
        mciSendStringW(L"play bgm repeat", nullptr, 0, nullptr);
        m_isPlaying = true;
    }

    bool m_initialized;
    bool m_isPlaying;
    std::wstring m_soundPath;
    int m_volume = 500;  // デフォルト50%
};
