#ifndef SDL_APP_H
#define SDL_APP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <SDL.h>

#include "cliffs.h"

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_AudioDeviceID audio_device;
    bool audio_ok;
    SDL_Texture *win_texture;
    int win_texture_width;
    int win_texture_height;
} App;

bool app_init(App *app, bool hidden, char *error, size_t error_size);
void app_shutdown(App *app);
bool app_load_win_image(App *app, const char *path, char *error, size_t error_size);
void app_play_intro_tune(App *app);
void app_stop_audio(App *app);
void app_render_instructions(App *app, const char *level_name);
void app_render_game(App *app, const Game *game, uint64_t now_ms);
void app_render_result(App *app, const Game *game);
void app_present(App *app);
void app_render_hook(void *userdata, const Game *game);
void app_delay_hook(void *userdata, uint32_t ms);

#endif
