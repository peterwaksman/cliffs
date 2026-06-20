#include <stdio.h>
#include <string.h>

#include <SDL.h>

#include "cliffs.h"
#include "sdl_app.h"

typedef enum {
    screen_instructions = 0,
    screen_playing,
    screen_result
} Screen;

static void usage(const char *argv0)
{
    fprintf(stderr, "usage: %s [level-name-or-path] [--smoke-test]\n", argv0);
}

static int map_key(const SDL_KeyboardEvent *key)
{
    switch (key->keysym.sym) {
        case SDLK_ESCAPE:
            return key_escape;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            return key_enter;
        case SDLK_UP:
            return key_arrow_up;
        case SDLK_DOWN:
            return key_arrow_down;
        case SDLK_LEFT:
            return key_arrow_left;
        case SDLK_RIGHT:
            return key_arrow_right;
        case SDLK_F1:
            return key_f1;
        case SDLK_F2:
            return key_f2;
        default:
            if (key->keysym.sym >= 0 && key->keysym.sym < 128) {
                return (int)key->keysym.sym;
            }
            return key_none;
    }
}

int main(int argc, char **argv)
{
    App app;
    Game game;
    RenderHooks hooks;
    Screen screen = screen_instructions;
    uint64_t now_ms;
    bool running = true;
    bool smoke_test = false;
    bool tune_started = false;
    const char *level_name = "ledges";
    char error[256];
    int i;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--smoke-test") == 0) {
            smoke_test = true;
        } else if (argv[i][0] == '-' && argv[i][1] != '\0') {
            usage(argv[0]);
            return 1;
        } else {
            level_name = argv[i];
        }
    }

    if (!app_init(&app, smoke_test, error, sizeof(error))) {
        fprintf(stderr, "%s\n", error);
        return 1;
    }

    app_load_win_image(&app, "assets/ev6.pik", error, sizeof(error));

    hooks.present = app_render_hook;
    hooks.delay = app_delay_hook;
    hooks.userdata = &app;

    now_ms = SDL_GetTicks64();
    if (!game_init(&game, level_name, &hooks, now_ms, error, sizeof(error))) {
        fprintf(stderr, "%s\n", error);
        app_shutdown(&app);
        return 1;
    }

    if (smoke_test) {
        app_render_game(&app, &game, now_ms);
        app_present(&app);
        app_shutdown(&app);
        return 0;
    }

    while (running) {
        SDL_Event event;

        now_ms = SDL_GetTicks64();

        if (screen == screen_instructions && !tune_started) {
            app_play_intro_tune(&app);
            tune_started = true;
        }

        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) {
                running = false;
                break;
            }

            if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
                int key = map_key(&event.key);

                if (screen == screen_instructions) {
                    if (key == key_escape) {
                        running = false;
                    } else if (key == key_enter) {
                        app_stop_audio(&app);
                        tune_started = false;
                        now_ms = SDL_GetTicks64();
                        if (!game_init(&game, level_name, &hooks, now_ms, error, sizeof(error))) {
                            fprintf(stderr, "%s\n", error);
                            running = false;
                        } else {
                            screen = screen_playing;
                        }
                    }
                } else if (screen == screen_playing) {
                    game_press_key(&game, key, now_ms);
                } else if (screen == screen_result) {
                    if (key == key_escape) {
                        running = false;
                    } else if (key == key_enter) {
                        screen = screen_instructions;
                    }
                }
            }
        }

        if (!running) {
            break;
        }

        if (screen == screen_playing) {
            game_update(&game, now_ms);
            if (game.round_over) {
                screen = screen_result;
            }
            app_render_game(&app, &game, now_ms);
        } else if (screen == screen_instructions) {
            app_render_instructions(&app, level_name);
        } else {
            app_render_result(&app, &game);
        }

        app_present(&app);
        SDL_Delay(16);
    }

    app_shutdown(&app);
    return 0;
}
