#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include "Entity.h"

Entity::Entity() {
    // ����� PHYSICS ����� //
    m_position     = glm::vec3(0.0f, 0.0f, 0.0f);
    m_velocity     = glm::vec3(0.0f, 0.0f, 0.0f);
    m_acceleration = glm::vec3(0.0f, 0.0f, 0.0f);

    // ����� TRANSLATION ����� //
    m_movement     = glm::vec3(0.0f, 0.0f, 0.0f);
    m_speed        = 0;
    m_model_matrix = glm::mat4(1.0f);
}

Entity::~Entity() {}

void Entity::move(int direction) {
    switch (direction) {
        case LEFT:
            m_acceleration.x += -1;
            break;
        case RIGHT:
            m_acceleration.x += 1;
            break;
        case UP:
            m_acceleration.y += 1;
            break;
        case DOWN:
            m_acceleration.y += -1;
            break;
    }
    // clamp acceleration by min and max values
    m_acceleration = glm::clamp(m_acceleration, glm::vec3(min_acceleration, min_acceleration, 0.0f), glm::vec3(max_acceleration, max_acceleration, 0.0f));
}

bool in_bounds(float value, float min, float max) {
    return value >= min && value <= max;
}

void Entity::update(float delta_time, Entity* collidable_entities, int collidable_entity_count, Entity* win_flag) {
    if (!m_is_active) return;

    m_collided_top    = false;
    m_collided_bottom = false;
    m_collided_left   = false;
    m_collided_right  = false;

    // ����� GRAVITY ����� //
    const float GRAVITY  = 2.0f;
    m_velocity          += m_acceleration * delta_time;
    m_velocity.y        -= GRAVITY * delta_time;

    for (int i = 0; i < collidable_entity_count; i++) {
        // STEP 1: For every entity that our player can collide with...
        Entity* collidable_entity = &collidable_entities[i];
        if (check_collision(collidable_entity)) {
            // STEP 2: Calculate the distance between its centre and our centre
            //         and use that to calculate the amount of overlap between
            //         both bodies.
            float y_distance = fabs(m_position.y - collidable_entity->m_position.y);
            float y_overlap  = fabs(y_distance - (m_height / 2.0f) - (collidable_entity->m_height / 2.0f));

            // STEP 3: "Unclip" ourselves from the other entity, and zero our
            //         vertical velocity.
            if (m_velocity.y > 0) {
                m_position.y   -= y_overlap;
                m_velocity.y    = 0;
                m_collided_top  = true;
            } else if (m_velocity.y < 0) {
                m_position.y      += y_overlap;
                m_velocity.y       = 0;
                m_collided_bottom  = true;
            }
            m_is_active = false;
            m_mission_failed  = true;
            return;
        }
    }

    m_position.y += m_velocity.y * delta_time;
    m_position.x += m_velocity.x * delta_time;

    // Slow acceleration over time
    if (m_acceleration.y > 0.001f) {
        m_acceleration.y *= 0.8;
    }
    if (m_acceleration.x > 0.001f) {
        m_acceleration.x *= 0.8;
    }
    m_acceleration = glm::clamp(m_acceleration, glm::vec3(-5.0f, -5.0f, 0.0f), glm::vec3(5.0f, 5.0f, 0.0f));
    // std::cout << "acceleration" << m_acceleration.x << std::endl;
    // std::cout << "velocity" << m_velocity.x << std::endl;
    // std::cout << "position" << m_position.x << std::endl;

    // ����� TRANSFORMATIONS ����� //
    m_model_matrix = glm::mat4(1.0f);
    m_model_matrix = glm::translate(m_model_matrix, m_position);
    m_model_matrix = glm::scale(m_model_matrix, m_scale);
    m_model_matrix = glm::rotate(m_model_matrix, glm::radians(m_rotation), glm::vec3(0.0f, 0.0f, 1.0f));

    // Check if player has won
    if (win_flag && check_collision(win_flag)) {
        win_flag->m_is_active = false;
        m_is_active           = false;
        m_mission_accomplished  = true;
    }

    // Check if player is still within bounds
    if (!in_bounds(m_position.x, -view_width, view_width) || !in_bounds(m_position.y, -view_height, view_height)) {
        std::cout << "NOT IN BOUNDS" << m_position.x << std::endl;
        m_is_active = false;
        m_mission_failed  = true;
    }
}

void Entity::render(ShaderProgram* program) {
    program->set_model_matrix(m_model_matrix);

    float vertices[]   = {-0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5};
    float tex_coords[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};

    glBindTexture(GL_TEXTURE_2D, m_texture_id);

    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->get_position_attribute());
    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, tex_coords);
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}

bool const Entity::check_collision(Entity* other) const {
    // If either entity is inactive, there shouldn't be any collision
    if (!m_is_active || !other->m_is_active) return false;

    // use circle-circle collision
    // float distance  = glm::distance(m_position, other->m_position);
    // float sum_radii = (m_width + other->m_width);
    // return distance < sum_radii;

    float x_distance = fabs(m_position.x - other->m_position.x) - ((m_width + other->m_width) / 2.0f);
    float y_distance = fabs(m_position.y - other->m_position.y) - ((m_height + other->m_height) / 2.0f);

    return x_distance < 0.0f && y_distance < 0.0f;
}
