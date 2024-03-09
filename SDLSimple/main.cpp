//    Author: Nabira Ahmad
//    Assignment: Lunar Lander
//    Date due: 2024-03-09, 11:59pm
//    I pledge that I have completed this assignment without
//    collaborating with anyone else, in conformance with the
//    NYU School of Engineering Policies and Procedures on
//    Academic Misconduct.

#define LOG(argument) std::cout << argument << '\n'
#define STB_IMAGE_IMPLEMENTATION
#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define NUMBER_OF_ENEMIES 3
#define FIXED_TIMESTEP 0.0166666f
#define ACC_OF_GRAVITY -0.01f

// count for every collidable entity
// in this game: bad platforms, good platforms, and walls are collidable entities

// to win:
    // avoid bad platforms and walls
#define BAD_PLATFORM_COUNT 17
#define GOOD_PLATFORM_COUNT 2
#define WALL_COUNT 4

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include "Entity.h"

// ————— STRUCTS AND ENUMS —————//
struct GameState
{
    Entity* player;
    Entity* bad_platform;
    Entity* good_platform;
    Entity* wall;
};

// ————— CONSTANTS ————— //
const int WINDOW_WIDTH = 640*1.5,
WINDOW_HEIGHT = 480*1.5;

const float BG_RED = 0.1922f,
            BG_BLUE = 0.549f,
            BG_GREEN = 0.9059f,
            BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND  = 1000.0;
const char  SPRITESHEET_FILEPATH[]  = "pinkbutterfly.gif",
            BAD_PLATFORM_FILEPATH[]  = "fire.png",
            GOOD_PLATFORM_FILEPATH[] = "flower.png",
            FONT_FILEPATH[] = "font1.png";

const int NUMBER_OF_TEXTURES = 1;  // to be generated, that is
const GLint LEVEL_OF_DETAIL  = 0;  // base image level; Level n is the nth mipmap reduction image
const GLint TEXTURE_BORDER   = 0;  // this value MUST be zero

// ————— VARIABLES ————— //

GameState g_game_state;
bool g_game_is_running = true;

// adding a timer to make it seem more like lunar lander :)
bool g_timer_running = true;
float g_timer_value = 0.0f;

SDL_Window* g_display_window;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_model_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_time_accumulator = 0.0f;
const int FONTBANK_SIZE = 16;

GLuint font_texture_id;

// ———— GENERAL FUNCTIONS ———— //

// purpose: draw text on the screen for UI messages
void DrawText(ShaderProgram *program, GLuint font_texture_id, std::string text, float screen_size, float spacing, glm::vec3 position)
{
    // Scale the size of the fontbank in the UV-plane
    // We will use this for spacing and positioning
    float width = 1.0f / FONTBANK_SIZE;
    float height = 1.0f / FONTBANK_SIZE;

    // Instead of having a single pair of arrays, we'll have a series of pairs—one for each character
    // Don't forget to include <vector>!
    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    // For every character...
    for (int i = 0; i < text.size(); i++) {
        // 1. Get their index in the spritesheet, as well as their offset (i.e. their position
        //    relative to the whole sentence)
        int spritesheet_index = (int) text[i];  // ascii value of character
        float offset = (screen_size + spacing) * i;
        
        // 2. Using the spritesheet index, we can calculate our U- and V-coordinates
        float u_coordinate = (float) (spritesheet_index % FONTBANK_SIZE) / FONTBANK_SIZE;
        float v_coordinate = (float) (spritesheet_index / FONTBANK_SIZE) / FONTBANK_SIZE;

        // 3. Inset the current pair in both vectors
        vertices.insert(vertices.end(), {
            offset + (-0.5f * screen_size), 0.5f * screen_size,
            offset + (-0.5f * screen_size), -0.5f * screen_size,
            offset + (0.5f * screen_size), 0.5f * screen_size,
            offset + (0.5f * screen_size), -0.5f * screen_size,
            offset + (0.5f * screen_size), 0.5f * screen_size,
            offset + (-0.5f * screen_size), -0.5f * screen_size,
        });

        texture_coordinates.insert(texture_coordinates.end(), {
            u_coordinate, v_coordinate,
            u_coordinate, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate + width, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate, v_coordinate + height,
        });
    }

    // 4. And render all of them using the pairs
    glm::mat4 g_model_matrix = glm::mat4(1.0f);
    g_model_matrix = glm::translate(g_model_matrix, position);
    
    g_shader_program.set_model_matrix(g_model_matrix);
    glUseProgram(g_shader_program.get_program_id());
    
    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices.data());
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());
    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates.data());
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    
    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int) (text.size() * 6));
    
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
}

// purpose: load texture to sprite
GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(image);

    return textureID;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Butterfly lander",
                                        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                        WINDOW_WIDTH, WINDOW_HEIGHT,
                                        SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);
    
    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    g_model_matrix = glm::mat4(1.0f);
    
    font_texture_id = load_texture(FONT_FILEPATH);
    
    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);
    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    // ————— PLAYER ————— //
    
    // creating a new player, setting the position, movement, acceleration, and speed
    g_game_state.player = new Entity();
    
    // initialize player on the top right like lunar lander
    g_game_state.player->set_position(glm::vec3(-3.0f, 3.0f, 0.0f));
    g_game_state.player->set_movement(glm::vec3(0.0f));
    g_game_state.player->set_acceleration(glm::vec3(0.0f, ACC_OF_GRAVITY * 0.01, 0.0f));
    g_game_state.player->set_speed(1.0f);
    g_game_state.player->m_texture_id = load_texture(SPRITESHEET_FILEPATH);
    
    // for collision measurements
    g_game_state.player->set_height(0.0f);
    g_game_state.player->set_width(0.4f);
    
    
    // ————— PLATFORM ————— //
    g_game_state.bad_platform = new Entity[BAD_PLATFORM_COUNT];
    g_game_state.good_platform = new Entity[GOOD_PLATFORM_COUNT];
    
    // Walls
    g_game_state.wall = new Entity[WALL_COUNT];
    
    // for each platform, load texture, set the position, update it, and declare its type
    for (int i=0; i< GOOD_PLATFORM_COUNT; i++){
        g_game_state.good_platform[i].m_texture_id = load_texture(GOOD_PLATFORM_FILEPATH);
        
        if (i == 0){
            g_game_state.good_platform[0].set_position(glm::vec3(3.5f, 1.0f, 0.0f));
        }
        else{
            g_game_state.good_platform[1].set_position(glm::vec3(4.0f, 1.0f, 0.0f));
        }
        g_game_state.good_platform[i].update(0.0f, NULL, 0);
        g_game_state.good_platform[i].entity_type = GOODPLATFORM;
    }
    
    for (int i = 0; i< BAD_PLATFORM_COUNT; i++){
        g_game_state.bad_platform[i].m_texture_id = load_texture(BAD_PLATFORM_FILEPATH);
        
        if (i == 0){
            g_game_state.bad_platform[i].set_position(glm::vec3(-2.5f, -3.0f, 0.0f));
        }
        else if (i == 1){
            g_game_state.bad_platform[i].set_position(glm::vec3(-2.7f, -3.0f, 0.0f));
        }
        else if (i == 2){
            g_game_state.bad_platform[i].set_position(glm::vec3(1.5f, 0.0f, 0.0f));
        }
        else if (i == 3){
            g_game_state.bad_platform[i].set_position(glm::vec3(1.3f, 0.0f, 0.0f));
        }
        else if (i == 4){
            g_game_state.bad_platform[i].set_position(glm::vec3(3.0f, -2.0f, 0.0f));
        }
        else if (i == 5){
            g_game_state.bad_platform[i].set_position(glm::vec3(2.8f, -2.0f, 0.0f));
        }
        else if (i == 6){
            g_game_state.bad_platform[i].set_position(glm::vec3(-1.5f, 0.5f, 0.0f));
        }
        else if (i == 7){
            g_game_state.bad_platform[i].set_position(glm::vec3(-1.3f, 0.5f, 0.0f));
        }
        else if (i == 8){
            g_game_state.bad_platform[i].set_position(glm::vec3(-4.2f, -1.0f, 0.0f));
        }
        else if (i == 9){
            g_game_state.bad_platform[i].set_position(glm::vec3(-4.4f, -1.0f, 0.0f));
        }
        else if (i == 10){
            g_game_state.bad_platform[i].set_position(glm::vec3(-4.6f, -1.0f, 0.0f));
        }
        else if (i == 11){
            g_game_state.bad_platform[i].set_position(glm::vec3(0.0f, 2.9f, 0.0f));
        }
        else if (i == 12){
            g_game_state.bad_platform[i].set_position(glm::vec3(0.2f, 2.9f, 0.0f));
        }
        else if (i == 13){
            g_game_state.bad_platform[i].set_position(glm::vec3(0.4f, 2.9f, 0.0f));
        }
        else if (i == 14){
            g_game_state.bad_platform[i].set_position(glm::vec3(-2.9f, -3.0f, 0.0f));
        }
        else if (i == 15){
            g_game_state.bad_platform[i].set_position(glm::vec3(2.5f, -2.0f, 0.0f));
        }
        else{
            g_game_state.bad_platform[i].set_position(glm::vec3(-1.7f, 0.5f, 0.0f));
        }
        g_game_state.bad_platform[i].update(0.0f, NULL, 0);
        g_game_state.bad_platform[i].entity_type = BADPLATFORM;
    }
    
    font_texture_id = load_texture(FONT_FILEPATH);
    
    g_game_state.player->entity_type = PLAYER;
    
    // setting the position and measurements for walls
    g_game_state.wall[0].set_position(glm::vec3(-6.0f, 0.0f, 0.0f));
    g_game_state.wall[1].set_position(glm::vec3(6.0f, 0.0f, 0.0f));
    g_game_state.wall[2].set_position(glm::vec3(0.0f, 4.5f, 0.0f));
    g_game_state.wall[3].set_position(glm::vec3(0.0f, -4.5f, 0.0f));
    
    // left and right walls: 
        // width-> 0.1, height-> viewport height
    
    // top and bottom walls:
        // width-> viewport height , height-> 0.1
    g_game_state.wall[0].set_width(0.1f);
    g_game_state.wall[1].set_width(0.1f);
    g_game_state.wall[2].set_height(0.1f);
    g_game_state.wall[3].set_height(0.1f);

    g_game_state.wall[2].set_width(VIEWPORT_WIDTH);
    g_game_state.wall[3].set_width(VIEWPORT_WIDTH);
    g_game_state.wall[0].set_height(VIEWPORT_HEIGHT);
    g_game_state.wall[1].set_height(VIEWPORT_HEIGHT);
    
    for (int i=0; i< WALL_COUNT; i++){
        g_game_state.wall[i].entity_type = WALLS;
    }
    g_game_state.good_platform->entity_type = GOODPLATFORM;
    
    // ————— GENERAL ————— //
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    // VERY IMPORTANT: If nothing is pressed, we don't want to go anywhere
    g_game_state.player->set_movement(glm::vec3(0.0f));

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            // End game
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_game_is_running = false;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_q:
                // Quit the game with a keystroke
                g_game_is_running = false;
                break;

            default:
                break;
            }

        default:
            break;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    // move left, if its not pressed, set x acceleration to 0
    if (key_state[SDL_SCANCODE_LEFT])
    {
        g_game_state.player->move_left();
    }
    else {
        g_game_state.player->set_acceleration_x(0.0f);
    }
    
    if (key_state[SDL_SCANCODE_RIGHT])
    {
        g_game_state.player->move_right();
    }
    
    // move up, if its not pressed, set y acceleration to the acc_of_gravity to keep it falling
    if (key_state[SDL_SCANCODE_UP]){
        g_game_state.player->move_up();
    }
    else{
        g_game_state.player->set_acceleration_y(ACC_OF_GRAVITY);
    }

    // This makes sure that the player can't move faster diagonally
    if (glm::length(g_game_state.player->get_movement()) > 1.0f)
    {
        g_game_state.player->set_movement(glm::normalize(g_game_state.player->get_movement()));
    }
}

void update()
{
    // delta time stuff
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    // fixed timestep stuff
    delta_time += g_time_accumulator;

    if (delta_time < FIXED_TIMESTEP)
    {
        g_time_accumulator = delta_time;
        return;
    }
    if (g_timer_running) {
            g_timer_value += FIXED_TIMESTEP;
        }
    
    while (delta_time >= FIXED_TIMESTEP)
    {
        // updating platforms based on time
        g_game_state.player->update(FIXED_TIMESTEP, g_game_state.bad_platform, BAD_PLATFORM_COUNT);
        g_game_state.player->update(FIXED_TIMESTEP, g_game_state.good_platform, GOOD_PLATFORM_COUNT);
        g_game_state.player->update(FIXED_TIMESTEP, g_game_state.wall, WALL_COUNT);
        delta_time -= FIXED_TIMESTEP;
    }
    g_time_accumulator = delta_time;
}

void render()
{
    // ————— GENERAL ————— //
    glClear(GL_COLOR_BUFFER_BIT);
    

    // player stuff
    g_game_state.player->render(&g_shader_program);

    // platform stuff
    for (int i = 0; i < BAD_PLATFORM_COUNT; i++) {
            g_game_state.bad_platform[i].render(&g_shader_program);
    }
    for (int i = 0; i < GOOD_PLATFORM_COUNT; i++) {
            g_game_state.good_platform[i].render(&g_shader_program);
    }
    
    // if it collides with a good platform, show a winner message
    if (g_game_state.player->collides == GOODPLATFORM) {
            DrawText(&g_shader_program, font_texture_id, "MISSION SUCCESSFUL :)", 0.6, -0.2f, glm::vec3(-3.5f, -1.0f, 0.0f));
            g_timer_running= false;
        }
    
    // if it collides with a bad platform or walls, show a loser message
    else if (g_game_state.player->collides == BADPLATFORM || g_game_state.player->collides == WALLS) {
        
            // perfect spacing for the message (after playing w the spacing and viewport size 1309298 times :p
            DrawText(&g_shader_program, font_texture_id, "MISSION FAILED :(", 0.6, -0.2f, glm::vec3(-3.0f,-1.0f,0.0f));
            g_timer_running = false;
        }
    
    // drawing the timer output
    std::string timerText = "TIME: " + std::to_string(g_timer_value);
    DrawText(&g_shader_program, font_texture_id, timerText, 0.4, -0.2f, glm::vec3(-4.4f, 3.5f, 0.0f));

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }

// driver game loop
int main(int argc, char* argv[])
{
    initialise();

    while (g_game_is_running)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
