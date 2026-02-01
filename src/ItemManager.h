#pragma once

#include <vector>
#include <DirectXMath.h>
#include <d3d11.h>
#include <wrl/client.h>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

// Whisky-themed item types (モルトバトル)
enum class ItemType {
    WhiskyShot,   // ショット - Power up (amber)
    BarrelDrop,   // 樽のしずく - Point item (golden brown)
    MaltGrain,    // モルト粒 - Big power (deep amber)
    IceCube,      // 氷 - Bomb item (cyan/ice blue)
    GoldenBottle, // 金のボトル - 1UP (gold)
    FullCask,     // フルカスク - Full power (rainbow amber)
    LabelStar     // ラベルスター - Star bonus (yellow gold)
};

struct Item {
    XMFLOAT2 position;
    XMFLOAT2 velocity;
    ItemType type;
    float radius;
    float lifetime;
    bool isActive;
    bool isBeingCollected; // Auto-collect animation
    float collectSpeed;
};

class Graphics;

class ItemManager {
public:
    ItemManager();
    ~ItemManager();

    void Initialize(Graphics* graphics);
    void Update(float deltaTime, XMFLOAT2 playerPos, int screenWidth, int screenHeight);
    void Render(Graphics* graphics);
    void Clear();

    // Spawn items when enemy dies
    void SpawnDrops(float x, float y, int enemyType);
    
    // Spawn specific item
    void SpawnItem(float x, float y, ItemType type);

    // Check collection and return collected items
    struct CollectedItems {
        int power;
        int points;
        int bombs;
        int lives;
        bool fullPower;
    };
    CollectedItems CollectItems(XMFLOAT2 playerPos, float collectRadius, bool autoCollect);

private:
    std::vector<Item> m_items;
    static const int MAX_ITEMS = 200;
    
    // テクスチャ（全アイテムタイプ）
    ComPtr<ID3D11ShaderResourceView> m_whiskyTexture;    // ウイスキーショット
    ComPtr<ID3D11ShaderResourceView> m_maltTexture;      // モルト粒
    ComPtr<ID3D11ShaderResourceView> m_barrelTexture;    // 樽のしずく
    ComPtr<ID3D11ShaderResourceView> m_labelTexture;     // ラベルスター
    ComPtr<ID3D11ShaderResourceView> m_iceCubeTexture;   // 氷
    ComPtr<ID3D11ShaderResourceView> m_bottleTexture;    // 金のボトル
    ComPtr<ID3D11ShaderResourceView> m_caskTexture;      // フルカスク
    
    XMFLOAT4 GetItemColor(ItemType type);
    float GetItemRadius(ItemType type);
};
