#include "ItemManager.h"
#include "Graphics.h"
#include <cmath>
#include <cstdlib>

constexpr float PI = 3.14159265358979f;

ItemManager::ItemManager() {
}

ItemManager::~ItemManager() {
}

void ItemManager::Initialize() {
    m_items.reserve(MAX_ITEMS);
}

void ItemManager::Update(float deltaTime, XMFLOAT2 playerPos, int screenWidth, int screenHeight) {
    for (size_t idx = 0; idx < m_items.size(); idx++) {
        Item& item = m_items[idx];
        if (!item.isActive) continue;

        if (item.isBeingCollected) {
            float dx = playerPos.x - item.position.x;
            float dy = playerPos.y - item.position.y;
            float dist = sqrtf(dx * dx + dy * dy);
            
            if (dist > 5.0f) {
                item.collectSpeed += 2000.0f * deltaTime;
                item.position.x += (dx / dist) * item.collectSpeed * deltaTime;
                item.position.y += (dy / dist) * item.collectSpeed * deltaTime;
            }
        } else {
            item.velocity.y += 200.0f * deltaTime;
            if (item.velocity.y > 150.0f) item.velocity.y = 150.0f;
            
            item.position.x += item.velocity.x * deltaTime;
            item.position.y += item.velocity.y * deltaTime;
        }

        item.lifetime -= deltaTime;
        if (item.lifetime <= 0 || item.position.y > screenHeight + 50) {
            item.isActive = false;
        }
    }
}

void ItemManager::Render(Graphics* graphics) {
    for (size_t idx = 0; idx < m_items.size(); idx++) {
        const Item& item = m_items[idx];
        if (!item.isActive) continue;

        XMFLOAT4 color = GetItemColor(item.type);
        
        if (item.lifetime < 2.0f) {
            float blink = sinf(item.lifetime * 20.0f);
            if (blink < 0) continue;
        }

        // シンプルな塗りつぶし円のみ（グロー削除）
        graphics->DrawCircle(item.position.x, item.position.y, item.radius, color);
    }
}

void ItemManager::Clear() {
    for (size_t idx = 0; idx < m_items.size(); idx++) {
        m_items[idx].isActive = false;
    }
}

void ItemManager::SpawnDrops(float x, float y, int enemyType) {
    int powerCount = 2 + enemyType;
    int pointCount = 3 + enemyType * 2;

    for (int i = 0; i < powerCount; i++) {
        float offsetX = static_cast<float>((rand() % 60) - 30);
        float offsetY = static_cast<float>((rand() % 40) - 20);
        SpawnItem(x + offsetX, y + offsetY, ItemType::WhiskyShot);
    }

    for (int i = 0; i < pointCount; i++) {
        float offsetX = static_cast<float>((rand() % 80) - 40);
        float offsetY = static_cast<float>((rand() % 50) - 25);
        SpawnItem(x + offsetX, y + offsetY, ItemType::BarrelDrop);
    }

    if (rand() % 100 < 5) {
        SpawnItem(x, y, ItemType::IceCube);
    }
    if (rand() % 100 < 1) {
        SpawnItem(x, y, ItemType::GoldenBottle);
    }
}

void ItemManager::SpawnItem(float x, float y, ItemType type) {
    Item* item = nullptr;
    for (size_t idx = 0; idx < m_items.size(); idx++) {
        if (!m_items[idx].isActive) {
            item = &m_items[idx];
            break;
        }
    }
    
    if (!item && m_items.size() < MAX_ITEMS) {
        m_items.push_back(Item());
        item = &m_items.back();
    }
    
    if (!item) return;

    item->position = { x, y };
    item->velocity = { 
        static_cast<float>((rand() % 100) - 50),
        -100.0f - static_cast<float>(rand() % 100)
    };
    item->type = type;
    item->radius = GetItemRadius(type);
    item->lifetime = 10.0f;
    item->isActive = true;
    item->isBeingCollected = false;
    item->collectSpeed = 0.0f;
}

ItemManager::CollectedItems ItemManager::CollectItems(XMFLOAT2 playerPos, float collectRadius, bool autoCollect) {
    CollectedItems collected = { 0, 0, 0, 0, false };

    for (size_t idx = 0; idx < m_items.size(); idx++) {
        Item& item = m_items[idx];
        if (!item.isActive) continue;

        float dx = playerPos.x - item.position.x;
        float dy = playerPos.y - item.position.y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (autoCollect && !item.isBeingCollected) {
            item.isBeingCollected = true;
            item.collectSpeed = 100.0f;
        }

        if (dist < collectRadius + item.radius) {
            item.isActive = false;

            switch (item.type) {
                case ItemType::WhiskyShot: collected.power += 1; break;
                case ItemType::MaltGrain: collected.power += 8; break;
                case ItemType::BarrelDrop: collected.points += 100; break;
                case ItemType::LabelStar: collected.points += 500; break;
                case ItemType::IceCube: collected.bombs += 1; break;
                case ItemType::GoldenBottle: collected.lives += 1; break;
                case ItemType::FullCask: collected.fullPower = true; break;
            }
        }
        else if (dist < collectRadius * 3) {
            item.isBeingCollected = true;
            item.collectSpeed = 200.0f;
        }
    }

    return collected;
}

XMFLOAT4 ItemManager::GetItemColor(ItemType type) {
    switch (type) {
        case ItemType::WhiskyShot: return { 0.9f, 0.6f, 0.2f, 1.0f };   // アンバー
        case ItemType::MaltGrain: return { 0.7f, 0.4f, 0.1f, 1.0f };    // ディープアンバー
        case ItemType::BarrelDrop: return { 0.8f, 0.5f, 0.15f, 1.0f };  // ゴールデンブラウン
        case ItemType::LabelStar: return { 1.0f, 0.9f, 0.4f, 1.0f };    // イエローゴールド
        case ItemType::IceCube: return { 0.5f, 0.9f, 1.0f, 1.0f };      // アイスブルー
        case ItemType::GoldenBottle: return { 1.0f, 0.85f, 0.3f, 1.0f };// ゴールド
        case ItemType::FullCask: return { 0.95f, 0.7f, 0.25f, 1.0f };   // リッチアンバー
        default: return { 1.0f, 1.0f, 1.0f, 1.0f };
    }
}

float ItemManager::GetItemRadius(ItemType type) {
    switch (type) {
        case ItemType::WhiskyShot: return 24.0f;    // 3x size
        case ItemType::MaltGrain: return 36.0f;
        case ItemType::BarrelDrop: return 20.0f;
        case ItemType::LabelStar: return 30.0f;
        case ItemType::IceCube: return 30.0f;
        case ItemType::GoldenBottle: return 40.0f;
        case ItemType::FullCask: return 48.0f;
        default: return 20.0f;
    }
}
