#pragma once

#include <xaudio2.h>
#include <wrl/client.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <Windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

// 音声データ
struct AudioData {
    std::vector<BYTE> buffer;
    WAVEFORMATEX format;
};

// XAudio2 + Media Foundation ベースのオーディオマネージャー
class AudioManager {
public:
    AudioManager() : m_xaudio2(nullptr), m_masterVoice(nullptr), m_mfInitialized(false) {}
    ~AudioManager() { Shutdown(); }

    bool Initialize() {
        // Media Foundation初期化
        HRESULT hr = MFStartup(MF_VERSION);
        if (SUCCEEDED(hr)) {
            m_mfInitialized = true;
        }

        // XAudio2初期化
        hr = XAudio2Create(&m_xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
        if (FAILED(hr)) return false;

        hr = m_xaudio2->CreateMasteringVoice(&m_masterVoice);
        if (FAILED(hr)) return false;

        // 実行ファイルのパスを取得
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        std::wstring exePath(path);
        size_t lastSlash = exePath.find_last_of(L"\\/");
        m_soundPath = exePath.substr(0, lastSlash) + L"\\..\\..\\assets\\sounds\\";

        // 音声ファイルをプリロード（WAVとMP3両対応）
        LoadAudio(L"shot", m_soundPath + L"shot.mp3");
        LoadAudio(L"hit", m_soundPath + L"hina_eyao.wav");
        LoadAudio(L"destroy", m_soundPath + L"hina_eyao.wav");
        LoadAudio(L"player_hit", m_soundPath + L"hina_buoo.wav");
        LoadAudio(L"bomb", m_soundPath + L"bomb.mp3");
        LoadAudio(L"cursor", m_soundPath + L"cursor.mp3");
        LoadAudio(L"confirm", m_soundPath + L"confirm.mp3");
        LoadAudio(L"item", m_soundPath + L"hina_eyao.wav");

        return true;
    }

    void Shutdown() {
        for (auto& voice : m_activeVoices) {
            if (voice) {
                voice->Stop();
                voice->DestroyVoice();
            }
        }
        m_activeVoices.clear();

        if (m_masterVoice) {
            m_masterVoice->DestroyVoice();
            m_masterVoice = nullptr;
        }
        if (m_xaudio2) {
            m_xaudio2->Release();
            m_xaudio2 = nullptr;
        }
        if (m_mfInitialized) {
            MFShutdown();
            m_mfInitialized = false;
        }
    }

    // 汎用オーディオ読み込み（WAV/MP3対応）
    bool LoadAudio(const std::wstring& name, const std::wstring& filepath) {
        // 拡張子を確認
        std::wstring ext = filepath.substr(filepath.find_last_of(L".") + 1);
        for (auto& c : ext) c = towlower(c);

        if (ext == L"wav") {
            return LoadWav(name, filepath);
        } else if (ext == L"mp3" || ext == L"m4a" || ext == L"wma" || ext == L"flac") {
            return LoadWithMediaFoundation(name, filepath);
        }
        return false;
    }

    void PlaySound(const std::wstring& name, float volume = 1.0f) {
        auto it = m_sounds.find(name);
        if (it == m_sounds.end()) return;

        const AudioData& audio = it->second;

        IXAudio2SourceVoice* sourceVoice = nullptr;
        HRESULT hr = m_xaudio2->CreateSourceVoice(&sourceVoice, &audio.format);
        if (FAILED(hr)) return;

        XAUDIO2_BUFFER buffer = { 0 };
        buffer.AudioBytes = static_cast<UINT32>(audio.buffer.size());
        buffer.pAudioData = audio.buffer.data();
        buffer.Flags = XAUDIO2_END_OF_STREAM;

        sourceVoice->SetVolume(volume);
        sourceVoice->SubmitSourceBuffer(&buffer);
        sourceVoice->Start();

        m_activeVoices.push_back(sourceVoice);
        CleanupVoices();
    }

    void PlayShot(float volume = 0.5f) { PlaySound(L"shot", volume); }
    void PlayEnemyHit(float volume = 0.7f) { PlaySound(L"hit", volume); }
    void PlayEnemyDestroy(float volume = 0.8f) { PlaySound(L"destroy", volume); }
    void PlayPlayerHit(float volume = 1.0f) { PlaySound(L"player_hit", volume); }
    void PlayBomb(float volume = 1.0f) { PlaySound(L"bomb", volume); }
    void PlayItemCollect(float volume = 0.6f) { PlaySound(L"item", volume); }
    void PlayGraze(float volume = 0.4f) { PlaySound(L"hit", volume); }
    void PlaySpecialReady(float volume = 0.8f) { PlaySound(L"item", volume); }
    void PlayCursor(float volume = 0.6f) { PlaySound(L"cursor", volume); }
    void PlayConfirm(float volume = 1.0f) { PlaySound(L"confirm", volume); }

private:
    // WAVファイル読み込み
    bool LoadWav(const std::wstring& name, const std::wstring& filepath) {
        HANDLE file = CreateFileW(filepath.c_str(), GENERIC_READ, FILE_SHARE_READ,
                                   nullptr, OPEN_EXISTING, 0, nullptr);
        if (file == INVALID_HANDLE_VALUE) return false;

        DWORD fileSize = GetFileSize(file, nullptr);
        std::vector<BYTE> fileData(fileSize);
        DWORD bytesRead;
        ReadFile(file, fileData.data(), fileSize, &bytesRead, nullptr);
        CloseHandle(file);

        if (fileSize < 44) return false;

        // WAVヘッダー解析
        BYTE* ptr = fileData.data();
        if (memcmp(ptr, "RIFF", 4) != 0) return false;
        if (memcmp(ptr + 8, "WAVE", 4) != 0) return false;

        // fmtチャンクを探す
        ptr = fileData.data() + 12;
        BYTE* end = fileData.data() + fileSize;
        WAVEFORMATEX fmt = { 0 };

        while (ptr < end - 8) {
            DWORD chunkSize = *reinterpret_cast<DWORD*>(ptr + 4);
            if (memcmp(ptr, "fmt ", 4) == 0) {
                fmt.wFormatTag = *reinterpret_cast<WORD*>(ptr + 8);
                fmt.nChannels = *reinterpret_cast<WORD*>(ptr + 10);
                fmt.nSamplesPerSec = *reinterpret_cast<DWORD*>(ptr + 12);
                fmt.nAvgBytesPerSec = *reinterpret_cast<DWORD*>(ptr + 16);
                fmt.nBlockAlign = *reinterpret_cast<WORD*>(ptr + 20);
                fmt.wBitsPerSample = *reinterpret_cast<WORD*>(ptr + 22);
                fmt.cbSize = 0;
            } else if (memcmp(ptr, "data", 4) == 0) {
                AudioData audio;
                audio.format = fmt;
                audio.buffer.assign(ptr + 8, ptr + 8 + chunkSize);
                m_sounds[name] = std::move(audio);
                return true;
            }
            ptr += 8 + chunkSize;
        }
        return false;
    }

    // Media Foundationを使ったMP3/その他形式の読み込み
    bool LoadWithMediaFoundation(const std::wstring& name, const std::wstring& filepath) {
        if (!m_mfInitialized) return false;

        IMFSourceReader* reader = nullptr;
        HRESULT hr = MFCreateSourceReaderFromURL(filepath.c_str(), nullptr, &reader);
        if (FAILED(hr)) return false;

        // PCM形式に変換する出力メディアタイプを設定
        IMFMediaType* outputType = nullptr;
        MFCreateMediaType(&outputType);
        outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        outputType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
        outputType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
        outputType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, 2);
        outputType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100);

        hr = reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, outputType);
        outputType->Release();
        if (FAILED(hr)) {
            reader->Release();
            return false;
        }

        // 実際の出力フォーマットを取得
        IMFMediaType* actualType = nullptr;
        reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &actualType);
        
        UINT32 channels = 0, sampleRate = 0, bitsPerSample = 0;
        actualType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channels);
        actualType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &sampleRate);
        actualType->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &bitsPerSample);
        actualType->Release();

        // オーディオデータを読み込む
        std::vector<BYTE> audioBuffer;
        while (true) {
            IMFSample* sample = nullptr;
            DWORD flags = 0;
            hr = reader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr, &flags, nullptr, &sample);
            
            if (flags & MF_SOURCE_READERF_ENDOFSTREAM) break;
            if (FAILED(hr) || !sample) break;

            IMFMediaBuffer* buffer = nullptr;
            sample->ConvertToContiguousBuffer(&buffer);
            
            BYTE* data = nullptr;
            DWORD length = 0;
            buffer->Lock(&data, nullptr, &length);
            audioBuffer.insert(audioBuffer.end(), data, data + length);
            buffer->Unlock();
            
            buffer->Release();
            sample->Release();
        }
        reader->Release();

        if (audioBuffer.empty()) return false;

        // AudioDataに格納
        AudioData audio;
        audio.buffer = std::move(audioBuffer);
        audio.format.wFormatTag = WAVE_FORMAT_PCM;
        audio.format.nChannels = static_cast<WORD>(channels);
        audio.format.nSamplesPerSec = sampleRate;
        audio.format.wBitsPerSample = static_cast<WORD>(bitsPerSample);
        audio.format.nBlockAlign = audio.format.nChannels * audio.format.wBitsPerSample / 8;
        audio.format.nAvgBytesPerSec = audio.format.nSamplesPerSec * audio.format.nBlockAlign;
        audio.format.cbSize = 0;

        m_sounds[name] = std::move(audio);
        return true;
    }

    void CleanupVoices() {
        auto it = m_activeVoices.begin();
        while (it != m_activeVoices.end()) {
            XAUDIO2_VOICE_STATE state;
            (*it)->GetState(&state);
            if (state.BuffersQueued == 0) {
                (*it)->DestroyVoice();
                it = m_activeVoices.erase(it);
            } else {
                ++it;
            }
        }

        while (m_activeVoices.size() > 32) {
            m_activeVoices.front()->Stop();
            m_activeVoices.front()->DestroyVoice();
            m_activeVoices.erase(m_activeVoices.begin());
        }
    }

    IXAudio2* m_xaudio2;
    IXAudio2MasteringVoice* m_masterVoice;
    std::wstring m_soundPath;
    std::unordered_map<std::wstring, AudioData> m_sounds;
    std::vector<IXAudio2SourceVoice*> m_activeVoices;
    bool m_mfInitialized;
};
