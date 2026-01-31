#pragma once

#include <vector>
#include <DirectXMath.h>

using namespace DirectX;

// Item types like Touhou Eiyashou
enum class ItemType {
    Power,      // P - Power up item (red)
    Point,      // Point item (blue)
    BigPower,   // Big power (large red)
    Bomb,       // Bomb item (green)
    Life,       // 1UP (pink)
    FullPower,  // Full power item (rainbow)
    Star        // Star item for bonus (yellow)
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

    void Initialize();
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
    
    XMFLOAT4 GetItemColor(ItemType type);
    float GetItemRadius(ItemType type);
};
