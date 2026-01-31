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

        graphics->DrawGlowCircle(item.position.x, item.position.y, item.radius, color, 2);

        XMFLOAT4 coreColor = { 1.0f, 1.0f, 1.0f, 0.8f };
        graphics->DrawCircle(item.position.x, item.position.y, item.radius * 0.5f, coreColor);
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
        SpawnItem(x + offsetX, y + offsetY, ItemType::Power);
    }

    for (int i = 0; i < pointCount; i++) {
        float offsetX = static_cast<float>((rand() % 80) - 40);
        float offsetY = static_cast<float>((rand() % 50) - 25);
        SpawnItem(x + offsetX, y + offsetY, ItemType::Point);
    }

    if (rand() % 100 < 5) {
        SpawnItem(x, y, ItemType::Bomb);
    }
    if (rand() % 100 < 1) {
        SpawnItem(x, y, ItemType::Life);
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
                case ItemType::Power: collected.power += 1; break;
                case ItemType::BigPower: collected.power += 8; break;
                case ItemType::Point: collected.points += 100; break;
                case ItemType::Star: collected.points += 500; break;
                case ItemType::Bomb: collected.bombs += 1; break;
                case ItemType::Life: collected.lives += 1; break;
                case ItemType::FullPower: collected.fullPower = true; break;
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
        case ItemType::Power: return { 1.0f, 0.3f, 0.3f, 1.0f };
        case ItemType::BigPower: return { 1.0f, 0.1f, 0.1f, 1.0f };
        case ItemType::Point: return { 0.3f, 0.5f, 1.0f, 1.0f };
        case ItemType::Star: return { 1.0f, 1.0f, 0.3f, 1.0f };
        case ItemType::Bomb: return { 0.3f, 1.0f, 0.3f, 1.0f };
        case ItemType::Life: return { 1.0f, 0.5f, 0.8f, 1.0f };
        case ItemType::FullPower: return { 1.0f, 0.8f, 0.2f, 1.0f };
        default: return { 1.0f, 1.0f, 1.0f, 1.0f };
    }
}

float ItemManager::GetItemRadius(ItemType type) {
    switch (type) {
        case ItemType::Power: return 8.0f;
        case ItemType::BigPower: return 14.0f;
        case ItemType::Point: return 6.0f;
        case ItemType::Star: return 10.0f;
        case ItemType::Bomb: return 12.0f;
        case ItemType::Life: return 14.0f;
        case ItemType::FullPower: return 16.0f;
        default: return 8.0f;
    }
}
