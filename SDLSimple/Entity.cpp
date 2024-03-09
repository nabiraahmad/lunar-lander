//    Author: Nabira Ahmad
//    Assignment: Lunar Lander
//    Date due: 2024-03-09, 11:59pm
//    I pledge that I have completed this assignment without
//    collaborating with anyone else, in conformance with the
//    NYU School of Engineering Policies and Procedures on
//    Academic Misconduct.

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "Entity.h"

// entity constructor
Entity::Entity()
{
    m_position = glm::vec3(0.0f);
    m_velocity = glm::vec3(0.0f);
    m_acceleration = glm::vec3(0.0f);
    m_speed = 0.0f;
    m_model_matrix = glm::mat4(1.0f);
}

// entity destructor
Entity::~Entity()
{
    delete[] m_animation_up;
    delete[] m_animation_down;
    delete[] m_animation_left;
    delete[] m_animation_right;
    delete[] m_walking;
}

// purpose: collisions functions
bool  Entity::check_collision(Entity* other)
{
    float x_distance = fabs(m_position.x - other->m_position.x) - ((m_width + other->m_width) / 2.0f);
    float y_distance = fabs(m_position.y - other->m_position.y) - ((m_height + other->m_height) / 2.0f);
    
    if (x_distance < 0.0f && y_distance < 0.0f){
        collides = other->entity_type;
        return true;
    }
    return false;
}


void Entity::check_collision_y(Entity* objects, int objectCount)
{
    for (int i = 0; i < objectCount; i++)
    {
        Entity* object = &objects[i];

        if (check_collision(object))
        {
            float y_distance = fabs(m_position.y - object->m_position.y);
            float y_diff = fabs(y_distance - (m_height / 2.0f) - (object->m_height / 2.0f));
            if (m_velocity.y > 0) {
                m_position.y -= y_diff;
                m_velocity.y = 0;
                m_collided_top = true;
            }
            else if (m_velocity.y < 0) {
                m_position.y += y_diff;
                m_velocity.y = 0;
                m_collided_bottom = true;
            }
        }
    }
}

void Entity::check_collision_x(Entity* objects, int objectCount)
{
    for (int i = 0; i < objectCount; i++)
    {
        Entity* object = &objects[i];

        if (check_collision(object))
        {
            float x_distance = fabs(m_position.x - object->m_position.x);
            float x_diff = fabs(x_distance - (m_width / 2.0f) - (object->m_width / 2.0f));
            if (m_velocity.x > 0) {
                m_position.x -= x_diff;
                m_velocity.x = 0;
                m_collided_right = true;
            }
            else if (m_velocity.x < 0) {
                m_position.x += x_diff;
                m_velocity.x = 0;
                m_collided_left = true;
            }
        }
    }
}

void Entity::update(float delta_time, Entity* collidable_entities, int collidable_entity_count)
{
    // if the entity is a bad platform or walls, return
    if (entity_type == BADPLATFORM || entity_type == WALLS) {
            return;
    }
    
    // updating position and velocity, calling check_collision functions
    m_velocity += m_acceleration * delta_time;
        
    m_position.y += m_velocity.y * delta_time;
    check_collision_y(collidable_entities, collidable_entity_count);

    m_position.x += m_velocity.x * delta_time;
    check_collision_x(collidable_entities, collidable_entity_count);
    
    // general transform stuff
    m_model_matrix = glm::mat4(1.0f);
    m_model_matrix = glm::translate(m_model_matrix, m_position);
}

void Entity::render(ShaderProgram* program)
{
    program->set_model_matrix(m_model_matrix);

    float vertices[]   = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
    float tex_coords[] = { 0.0,  1.0, 1.0,  1.0, 1.0, 0.0,  0.0,  1.0, 1.0, 0.0,  0.0, 0.0 };

    glBindTexture(GL_TEXTURE_2D, m_texture_id);

    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->get_position_attribute());
    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, tex_coords);
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}
