#pragma once

#include <DirectXMath.h>

using namespace DirectX;

class Player
{
public:
    Player();
    ~Player();

    void Update(float deltaTime);
    void Jump();
    void MoveLeft(float deltaTime);
    void MoveRight(float deltaTime);
    void MoveForward(float deltaTime);
    void MoveBackward(float deltaTime);

    XMFLOAT3 GetPosition() const { return m_position; }
    XMFLOAT3 GetVelocity() const { return m_velocity; }
    XMFLOAT3 GetSize() const { return m_size; }

    void SetPosition(const XMFLOAT3& position) { m_position = position; }
    void SetVelocity(const XMFLOAT3& velocity) { m_velocity = velocity; }
    void SetGrounded(bool grounded) { m_isGrounded = grounded; }

    bool IsGrounded() const { return m_isGrounded; }

    XMMATRIX GetWorldMatrix() const;

private:
    XMFLOAT3 m_position;
    XMFLOAT3 m_velocity;
    XMFLOAT3 m_size;
    XMFLOAT4 m_color;

    float m_moveSpeed;
    float m_jumpForce;
    float m_gravity;

    bool m_isGrounded;
};
