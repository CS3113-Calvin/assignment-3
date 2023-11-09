#pragma once
#include <SDL.h>
#include <SDL_opengl.h>

#include "ShaderProgram.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/mat4x4.hpp"

// Constants
// Window size
const float view_width  = 15.0f;
const float view_height = 7.5f;

enum Direction { LEFT,
                 RIGHT,
                 UP,
                 DOWN };

class Entity {
   private:
    bool m_is_active = true;

    // ����� PHYSICS (GRAVITY) ����� //
    glm::vec3 m_position;
    glm::vec3 m_velocity;
    glm::vec3 m_acceleration;

    // ����� TRANSFORMATIONS ����� //
    float     m_speed;
    glm::vec3 m_movement;
    glm::mat4 m_model_matrix;

    float     m_width    = 1;
    float     m_height   = 1;
    glm::vec3 m_scale    = glm::vec3(1.0f, 1.0f, 1.0f);
    float     m_rotation = 0.0f;

   public:
    // ����� STATIC VARIABLES ����� //
    static const int SECONDS_PER_FRAME = 4;
    static const int LEFT              = 0,
                     RIGHT             = 1,
                     UP                = 2,
                     DOWN              = 3;

    // ����� PHYSICS (COLLISIONS) ����� //
    bool m_collided_top    = false;
    bool m_collided_bottom = false;
    bool m_collided_left   = false;
    bool m_collided_right  = false;

    GLuint m_texture_id;

    bool m_mission_failed = false;
    bool m_mission_accomplished = false;

    // ����� METHODS ����� //
    Entity();
    ~Entity();

    // void       draw_sprite_from_texture_atlas(ShaderProgram* program, GLuint texture_id, int index);
    bool const check_collision(Entity* other) const;

    void update(float delta_time, Entity* collidable_entities, int collidable_entity_count, Entity* win_flag);
    void render(ShaderProgram* program);

    void move_left() { m_acceleration.x = glm::clamp(m_acceleration.x - 0.5f, -5.0f, 5.0f); };
    void move_right() { m_acceleration.x = glm::clamp(m_acceleration.x + 0.5f, -5.0f, 5.0f); };
    void move_up() { m_acceleration.y = glm::clamp(m_acceleration.y + 1.0f, -5.0f, 5.0f); };
    void move_down() { m_acceleration.y = glm::clamp(m_acceleration.y - 1.0f, -5.0f, 5.0f); };

    void activate() { m_is_active = true; };
    void deactivate() { m_is_active = false; };

    // ����� GETTERS ����� //
    glm::vec3 const get_position() const { return m_position; };
    glm::vec3 const get_velocity() const { return m_velocity; };
    glm::vec3 const get_acceleration() const { return m_acceleration; };
    glm::vec3 const get_movement() const { return m_movement; };
    float const     get_speed() const { return m_speed; };
    int const       get_width() const { return m_width; };
    int const       get_height() const { return m_height; };
    glm::vec3 const get_scale() { return m_scale; };
    float const     get_rotation() const { return m_rotation; };

    // ����� SETTERS ����� //
    void const set_position(glm::vec3 new_position) { m_position = new_position; };
    void const set_velocity(glm::vec3 new_velocity) { m_velocity = new_velocity; };
    void const set_acceleration(glm::vec3 new_acceleration) { m_acceleration = new_acceleration; };
    void const set_movement(glm::vec3 new_movement) { m_movement = new_movement; };
    void const set_speed(float new_speed) { m_speed = new_speed; };
    void const set_width(float new_width) { m_width = new_width; };
    void const set_height(float new_height) { m_height = new_height; };
    void const set_scale(glm::vec3 new_scale) { m_scale = new_scale; };
    void const set_rotation(float new_rotation) { m_rotation = new_rotation; };
};
