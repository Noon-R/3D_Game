#include "../include/Platform.h"

Platform::Platform(const XMFLOAT3& position, const XMFLOAT3& size, const XMFLOAT4& color)
    : m_position(position)
    , m_size(size)
    , m_color(color)
{
}

Platform::~Platform()
{
}

XMMATRIX Platform::GetWorldMatrix() const
{
    XMMATRIX scale = XMMatrixScaling(m_size.x, m_size.y, m_size.z);
    XMMATRIX translation = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
    return scale * translation;
}

bool Platform::CheckCollision(const XMFLOAT3& playerPos, const XMFLOAT3& playerSize) const
{
    float playerMinX = playerPos.x - playerSize.x * 0.5f;
    float playerMaxX = playerPos.x + playerSize.x * 0.5f;
    float playerMinY = playerPos.y - playerSize.y * 0.5f;
    float playerMaxY = playerPos.y + playerSize.y * 0.5f;
    float playerMinZ = playerPos.z - playerSize.z * 0.5f;
    float playerMaxZ = playerPos.z + playerSize.z * 0.5f;

    float platformMinX = m_position.x - m_size.x * 0.5f;
    float platformMaxX = m_position.x + m_size.x * 0.5f;
    float platformMinY = m_position.y - m_size.y * 0.5f;
    float platformMaxY = m_position.y + m_size.y * 0.5f;
    float platformMinZ = m_position.z - m_size.z * 0.5f;
    float platformMaxZ = m_position.z + m_size.z * 0.5f;

    bool collisionX = playerMaxX >= platformMinX && playerMinX <= platformMaxX;
    bool collisionY = playerMaxY >= platformMinY && playerMinY <= platformMaxY;
    bool collisionZ = playerMaxZ >= platformMinZ && playerMinZ <= platformMaxZ;

    return collisionX && collisionY && collisionZ;
}
