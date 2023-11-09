/**
 * Author: Calvin Tian
 * Assignment: Lunar Lander
 * Date due: 2023-11-08, 11:59pm
 * I pledge that I have completed this assignment without
 * collaborating with anyone else, in conformance with the
 * NYU School of Engineering Policies and Procedures on
 * Academic Misconduct.
 **/

#define LOG(argument) std::cout << argument << '\n'
#define STB_IMAGE_IMPLEMENTATION
#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define NUMBER_OF_ENEMIES 3
#define FIXED_TIMESTEP 0.0166666f
#define ACC_OF_GRAVITY -9.81f
#define ASTEROID_COUNT 60

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>

#include <ctime>
#include <vector>

#include "Entity.h"
#include "ShaderProgram.h"
#include "cmath"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/mat4x4.hpp"
#include "stb_image.h"

// ����� STRUCTS AND ENUMS �����//
struct GameState {
    Entity* player;
    Entity* asteroids;
    Entity* flag;
    Entity* mission_failed;
    Entity* mission_accomplished;
    Entity* background;
};

// ����� CONSTANTS ����� //
const float MILLISECONDS_IN_SECOND = 1000.0;

const int WINDOW_WIDTH  = 640 * 3,
          WINDOW_HEIGHT = 960;

const float BG_RED     = 0.1922f,
            BG_BLUE    = 0.549f,
            BG_GREEN   = 0.9059f,
            BG_OPACITY = 1.0f;

const int VIEWPORT_X      = 0,
          VIEWPORT_Y      = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const char PLAYER_FILEPATH[]               = "assets/ship_1.png",
           WIN_FILEPATH[]                  = "assets/win.png",
           ASTEROID_FILEPATH[]             = "assets/asteroid.png",
           MISSION_ACCOMPLISHED_FILEPATH[] = "assets/mission_accomplished.png",
           MISSION_FAILED_FILEPATH[]       = "assets/mission_failed.png",
           PLAYER_LOW_FUEL_FILEPATH[]      = "assets/ship_1_low_fuel.png",
           PLAYER_NO_FUEL_FILEPATH[]       = "assets/ship_1_no_fuel.png";

const int   NUMBER_OF_TEXTURES = 1;  // to be generated, that is
const GLint LEVEL_OF_DETAIL    = 0;  // base image level; Level n is the nth mipmap reduction image
const GLint TEXTURE_BORDER     = 0;  // this value MUST be zero

// ����� VARIABLES ����� //
GameState g_game_state;

SDL_Window* g_display_window;
bool        g_game_is_running = true;

ShaderProgram g_shader_program;
glm::mat4     g_view_matrix, g_projection_matrix;

float g_previous_ticks   = 0.0f;
float g_time_accumulator = 0.0f;

// ���� GENERAL FUNCTIONS ���� //
float rand_float_range(float a, float b) {
    return ((b - a) * ((float)rand() / RAND_MAX)) + a;
}

GLuint load_texture(const char* filepath) {
    int            width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL) {
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

void initialise() {
    srand(time(NULL));  // seed random time

    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Hello, Entities!",
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

    g_view_matrix       = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-view_width, view_width, -view_height, view_height, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    // ����� PLAYER ����� //
    // Existing
    g_game_state.player = new Entity();
    g_game_state.player->set_position(glm::vec3(rand_float_range(-view_width + 2, view_width - 2), view_height, 0.0f));
    g_game_state.player->set_movement(glm::vec3(0.0f));
    g_game_state.player->set_acceleration(glm::vec3(0.0f, 0.0f, 0.0f));
    g_game_state.player->m_texture_id = load_texture(PLAYER_FILEPATH);
    g_game_state.player->set_height(0.5f);
    g_game_state.player->set_width(0.5f);
    g_game_state.player->set_scale(glm::vec3(1.0f, 1.0f, 1.0f));
    g_game_state.player->m_low_fuel_texture_id = load_texture(PLAYER_LOW_FUEL_FILEPATH);
    g_game_state.player->m_no_fuel_texture_id = load_texture(PLAYER_NO_FUEL_FILEPATH);

    // ����� ASTEROIDS ����� //
    g_game_state.asteroids = new Entity[ASTEROID_COUNT];

    // Set asteroids at random locations on the screen
    for (int index = 0; index < ASTEROID_COUNT; ++index) {
        g_game_state.asteroids[index].m_texture_id = load_texture(ASTEROID_FILEPATH);

        // random asteroid location between view_width and view_height
        g_game_state.asteroids[index].set_position(glm::vec3(rand_float_range(-view_width, view_width), rand_float_range(-view_height, view_height - 3.0f), 0.0f));

        g_game_state.asteroids[index].set_scale(glm::vec3(1.5f, 1.5f, 1.0f));
        g_game_state.asteroids[index].set_width(0.75f);
        g_game_state.asteroids[index].set_height(1.0f);
        g_game_state.asteroids[index].update(0.0f, NULL, 0, NULL);
    }

    // Set up the win flag at the bottom of the screen at a random position
    g_game_state.flag               = new Entity();
    g_game_state.flag->m_texture_id = load_texture(WIN_FILEPATH);
    g_game_state.flag->set_position(glm::vec3(rand() % 10 - 5.0f, -6.0f, 0.0f));
    g_game_state.flag->set_scale(glm::vec3(2.0f, 2.0f, 1.0f));
    g_game_state.flag->set_width(1.5f);
    g_game_state.flag->set_height(1.5f);
    g_game_state.flag->update(0.0f, NULL, 0, NULL);

    // Game states
    g_game_state.mission_failed               = new Entity();
    g_game_state.mission_failed->m_texture_id = load_texture(MISSION_FAILED_FILEPATH);
    g_game_state.mission_failed->set_scale(glm::vec3(20.0f, 10.0f, 1.0f));
    g_game_state.mission_failed->update(0.0f, NULL, 0, NULL);

    g_game_state.mission_accomplished               = new Entity();
    g_game_state.mission_accomplished->m_texture_id = load_texture(MISSION_ACCOMPLISHED_FILEPATH);
    g_game_state.mission_accomplished->set_scale(glm::vec3(20.0f, 10.0f, 1.0f));
    g_game_state.mission_accomplished->update(0.0f, NULL, 0, NULL);

    // Background
    g_game_state.background               = new Entity();
    g_game_state.background->m_texture_id = load_texture("assets/background.png");
    g_game_state.background->set_scale(glm::vec3(30.0f, 30.0f, 1.0f));
    g_game_state.background->update(0.0f, NULL, 0, NULL);

    // ����� GENERAL ����� //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input() {
    // VERY IMPORTANT: If nothing is pressed, we don't want to go anywhere
    // g_game_state.player->set_movement(glm::vec3(0.0f));

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
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

    if (key_state[SDL_SCANCODE_LEFT]) {
        g_game_state.player->move_left();
        g_game_state.player->set_rotation(90.0f);
    } else if (key_state[SDL_SCANCODE_RIGHT]) {
        g_game_state.player->move_right();
        g_game_state.player->set_rotation(270.0f);
    } else if (key_state[SDL_SCANCODE_UP]) {
        g_game_state.player->move_up();
        g_game_state.player->set_rotation(0.0f);
    } else if (key_state[SDL_SCANCODE_DOWN]) {
        // g_game_state.player->move_down();
    }

    // This makes sure that the player can't move faster diagonally
    // if (glm::length(g_game_state.player->get_movement()) > 1.0f) {
    //     g_game_state.player->set_movement(glm::normalize(g_game_state.player->get_movement()));
    // }
}

void update() {
    // ����� DELTA TIME ����� //
    float ticks      = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;  // get the current number of ticks
    float delta_time = ticks - g_previous_ticks;                        // the delta time is the difference from the last frame
    g_previous_ticks = ticks;

    // ����� FIXED TIMESTEP ����� //
    // STEP 1: Keep track of how much time has passed since last step
    delta_time += g_time_accumulator;

    // STEP 2: Accumulate the ammount of time passed while we're under our fixed timestep
    if (delta_time < FIXED_TIMESTEP) {
        g_time_accumulator = delta_time;
        return;
    }

    // STEP 3: Once we exceed our fixed timestep, apply that elapsed time into the objects' update function invocation
    while (delta_time >= FIXED_TIMESTEP) {
        // Notice that we're using FIXED_TIMESTEP as our delta time
        g_game_state.player->update(FIXED_TIMESTEP, g_game_state.asteroids, ASTEROID_COUNT, g_game_state.flag);
        delta_time -= FIXED_TIMESTEP;
    }

    g_time_accumulator = delta_time;
}

void render() {
    // ����� GENERAL ����� //
    glClear(GL_COLOR_BUFFER_BIT);

    // ����� BACKGROUND ����� //
    g_game_state.background->render(&g_shader_program);

    // ����� PLAYER ����� //
    g_game_state.player->render(&g_shader_program);

    // ����� PLATFORM ����� //
    for (int i = 0; i < ASTEROID_COUNT; i++) g_game_state.asteroids[i].render(&g_shader_program);

    // ����� WIN FLAG ����� //
    g_game_state.flag->render(&g_shader_program);

    // ����� MISSION ACCOMPLISHED ����� //
    if (g_game_state.player->m_mission_accomplished) {
        g_game_state.mission_accomplished->render(&g_shader_program);
    }

    // ����� MISSION FAILED ����� //
    if (g_game_state.player->m_mission_failed) {
        g_game_state.mission_failed->render(&g_shader_program);
    }

    // ����� GENERAL ����� //
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }

// ����� DRIVER GAME LOOP ����� /
int main(int argc, char* argv[]) {
    initialise();

    while (g_game_is_running) {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
