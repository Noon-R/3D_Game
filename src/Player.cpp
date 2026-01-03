#include "../include/Player.h"

Player::Player()
    : m_position(0.0f, 5.0f, 0.0f)
    , m_velocity(0.0f, 0.0f, 0.0f)
    , m_size(1.0f, 1.0f, 1.0f)
    , m_color(0.0f, 0.8f, 0.2f, 1.0f)
    , m_moveSpeed(5.0f)
    , m_jumpForce(10.0f)
    , m_gravity(20.0f)
    , m_isGrounded(false)
{
}

Player::~Player()
{
}

void Player::Update(float deltaTime)
{
    if (!m_isGrounded)
    {
        m_velocity.y -= m_gravity * deltaTime;
    }

    m_position.x += m_velocity.x * deltaTime;
    m_position.y += m_velocity.y * deltaTime;
    m_position.z += m_velocity.z * deltaTime;

    m_velocity.x *= 0.9f;
    m_velocity.z *= 0.9f;
}

void Player::Jump()
{
    if (m_isGrounded)
    {
        m_velocity.y = m_jumpForce;
        m_isGrounded = false;
    }
}

void Player::MoveLeft(float deltaTime)
{
    m_velocity.x = -m_moveSpeed;
}

void Player::MoveRight(float deltaTime)
{
    m_velocity.x = m_moveSpeed;
}

void Player::MoveForward(float deltaTime)
{
    m_velocity.z = m_moveSpeed;
}

void Player::MoveBackward(float deltaTime)
{
    m_velocity.z = -m_moveSpeed;
}

XMMATRIX Player::GetWorldMatrix() const
{
    XMMATRIX scale = XMMatrixScaling(m_size.x, m_size.y, m_size.z);
    XMMATRIX translation = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
    return scale * translation;
}
