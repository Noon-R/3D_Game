#pragma once

#include <DirectXMath.h>

using namespace DirectX;

class Platform
{
public:
    Platform(const XMFLOAT3& position, const XMFLOAT3& size, const XMFLOAT4& color);
    ~Platform();

    XMFLOAT3 GetPosition() const { return m_position; }
    XMFLOAT3 GetSize() const { return m_size; }
    XMFLOAT4 GetColor() const { return m_color; }

    XMMATRIX GetWorldMatrix() const;

    bool CheckCollision(const XMFLOAT3& playerPos, const XMFLOAT3& playerSize) const;

private:
    XMFLOAT3 m_position;
    XMFLOAT3 m_size;
    XMFLOAT4 m_color;
};
