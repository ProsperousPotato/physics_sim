#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_hints.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    float x;
    float y;
} Vector;

typedef struct {
    Vector position;
    Vector velocity;
    SDL_Color color;
} Circle;

#define SCREEN_WIDTH 720
#define SCREEN_HEIGHT 720

bool init_SDL(SDL_Window **window, SDL_Renderer **renderer) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    *window = SDL_CreateWindow("Physics", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (*window == NULL) {
        fprintf(stderr, "SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (*renderer == NULL) {
        fprintf(stderr, "SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    return true;
}

#define GRAVITY 750.0f
#define AIR_RESISTANCE 0.90f
#define CIRCLE_WINDOW_RADIUS SCREEN_HEIGHT / 2
#define BALL_RADIUS CIRCLE_WINDOW_RADIUS / 50


void update_balls(Circle *ball, Circle *wall, float delta_time) {

    ball->velocity.y += GRAVITY * delta_time;

    ball->position.x += ball->velocity.x * AIR_RESISTANCE * delta_time;
    ball->position.y += ball->velocity.y * AIR_RESISTANCE * delta_time;

    Vector direction_to_center = {
        ball->position.x - wall->position.x,
        ball->position.y - wall->position.y
    };

    float distance= sqrtf(direction_to_center.x * direction_to_center.x + direction_to_center.y * direction_to_center.y);
    if (distance + BALL_RADIUS >= CIRCLE_WINDOW_RADIUS) {
        float normal = 1.0f / distance;
        direction_to_center.x *= normal;
        direction_to_center.y *= normal;

        float reflection_velocity = ball->velocity.x * direction_to_center.x + ball->velocity.y * direction_to_center.y;
        ball->velocity.x -= 2.0f * reflection_velocity * direction_to_center.x;
        ball->velocity.y -= 2.0f * reflection_velocity * direction_to_center.y;

        float rev_wall_clip = distance + BALL_RADIUS - CIRCLE_WINDOW_RADIUS;
        ball->position.x -= direction_to_center.x * rev_wall_clip;
        ball->position.y -= direction_to_center.y * rev_wall_clip;
    }

}

void render_circle(SDL_Renderer *renderer, Circle *ball, int radius) {
    SDL_SetRenderDrawColor(renderer, ball->color.r, ball->color.g, ball->color.b, ball->color.a);

    for (int w = 0; w < radius * 2; w++) {
        for (int h = 0; h < radius * 2; h++) {
            int dx = radius - w;
            int dy = radius - h;
            if ((dx * dx + dy * dy) <= (radius * radius)) {
                SDL_RenderDrawPoint(renderer, ball->position.x + dx, ball->position.y + dy);
            }
        }
    }
}

#define MAX_BALLS 2048

int main() {
    srand((unsigned int)time(NULL));
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    if (!init_SDL(&window, &renderer)) {
        return 1;
    }

    const int n = 1000;

    Circle balls[MAX_BALLS];
    int ball_count = 1;

    Circle ball = {
        .position = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2},
        .velocity = {(rand() % (2 * n + 1) - n), 0},
        .color = {(rand() % 256), (rand() % 256), (rand() % 256), 0}
    };

    Circle circle_window = {
        .position = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2},
        .velocity = {0, 0},
        .color = {20, 20, 20, 0}
    };

    bool running = true;
    SDL_Event event;
    float frame_count = 0;
    Uint32 last_tick = SDL_GetTicks();
    Uint32 fps_time = last_tick;

    while (running) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_KEYDOWN:
                    switch (event.key.keysym.scancode) {
                        case SDL_SCANCODE_R:
                            ball.position.x = SCREEN_WIDTH / 2;
                            ball.position.y = SCREEN_HEIGHT / 2;
                            ball.velocity.y = 0;
                            ball.velocity.x = (rand() % (2 * n) -n);
                            ball_count = 1;
                            break;

                        case SDL_SCANCODE_Q:
                            running = false;
                        
                        case SDL_SCANCODE_SPACE:
                            while (ball_count < MAX_BALLS) {
                                balls[ball_count++] = (Circle) {
                                    .position = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2},
                                    .velocity =  {(rand() % (2 * n + 1) - n), 0},
                                    .color = {(rand() % 256), (rand() % 256), (rand() % 256), 0}
                                };
                            }

                        default:
                            break;
                    }
            }
        }
        Uint32 cur_tick = SDL_GetTicks();
        frame_count++;
        float delta_time = (cur_tick - last_tick) / 1000.0f;

        update_balls(&ball, &circle_window, delta_time);

        last_tick = cur_tick;

        for (int i = 1; i < ball_count; i++) {
            update_balls(&balls[i], &circle_window, delta_time);
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);

        render_circle(renderer, &circle_window, CIRCLE_WINDOW_RADIUS);
        render_circle(renderer, &ball, BALL_RADIUS);

        for (int i = 1; i < ball_count; i++) {
            render_circle(renderer, &balls[i], BALL_RADIUS);
        }

        if (cur_tick - fps_time >= 1000) {
            float fps = (float)frame_count;
            frame_count = 0;
            fps_time = cur_tick;

            char title[64];
            snprintf(title, sizeof(title), "Physics | FPS: %.0f", fps);
            SDL_SetWindowTitle(window, title);
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
