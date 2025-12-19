#include <stdio.h>
#include <stdlib.h>
#include <raylib.h>
#include <raygui.h>

#include "dynamic_array.h"

const int SCREEN_WIDTH = 1080;
const int SCREEN_HEIGHT = 720;
const int ZOOM_LEVEL = 10;

 // The side menu occupies 1/3 of the screen width (right-sided)
const double SIDE_MENU_RATIO = 1.0 / 3.0;
const double MAIN_SCREEN_WIDTH = SCREEN_WIDTH * (1 - SIDE_MENU_RATIO);
const double SIDE_SCREEN_WIDTH = SCREEN_WIDTH * SIDE_MENU_RATIO;

Vector2 mouse_position = { 0 };

// X AND Y OFFSETS
int vert_line_offset = 0;
int horz_line_offset = 0;
int nb_tiles = 10;
int tile_width;

CREATE_DYNAMIC_ARRAY(Vector2);

DynamicArray_Vector2 balls = { 0 };

static float vector_distance2(Vector2 *p1, Vector2 *p2)
{
    Vector2 d = {
        .x = p2->x - p1->x,
        .y = p2->y - p1->y
    };
    return d.x * d.x + d.y * d.y;
}

static float float_distance2(float x1, float y1, float x2, float y2)
{
    Vector2 v1 = {
        .x = x1,
        .y = y1
    };
    Vector2 v2 = {
        .x = x2,
        .y = y2
    };
    return vector_distance2(&v1, &v2);
}

static int is_in_screen(Vector2 *point)
{
    return !(point->x < 0
             || point->x > MAIN_SCREEN_WIDTH
             || point->y < 0
             || point->y > SCREEN_HEIGHT);
}

static void draw_grid(void)
{
    static double mouse_prev_x = 0;
    static double mouse_prev_y = 0;
    
    tile_width = MAIN_SCREEN_WIDTH / nb_tiles;

    if (IsKeyPressed(KEY_A)) {
        nb_tiles += ZOOM_LEVEL;
    }
    
    if (IsKeyPressed(KEY_Q)) {
        if (nb_tiles - ZOOM_LEVEL > 0) {
            nb_tiles -= ZOOM_LEVEL;
        }
    }

    for (int t = 0; t < nb_tiles; t++) {
        int line_pos = t * tile_width;

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            mouse_prev_x = mouse_position.x;
            mouse_prev_y = mouse_position.y;
        }
        
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            int mouse_dx = mouse_position.x - mouse_prev_x;
            int mouse_dy = mouse_position.y - mouse_prev_y;

            vert_line_offset += (int)mouse_dx%tile_width;
            horz_line_offset += (int)mouse_dy%tile_width;
            
            mouse_prev_x = mouse_position.x;
            mouse_prev_y = mouse_position.y;
        }

        int vert_line_pos = (int)(vert_line_offset + line_pos) % (int) MAIN_SCREEN_WIDTH;
        int horz_line_pos = (int)(horz_line_offset + line_pos) % SCREEN_HEIGHT;

        if (vert_line_pos < 0) {
            vert_line_pos = abs((int)MAIN_SCREEN_WIDTH - vert_line_pos);
            vert_line_pos = MAIN_SCREEN_WIDTH - vert_line_pos % (int) MAIN_SCREEN_WIDTH;
        }
        
        if (horz_line_pos < 0) {
            horz_line_pos = abs(SCREEN_HEIGHT - horz_line_pos);
            horz_line_pos = SCREEN_HEIGHT - horz_line_pos %  SCREEN_HEIGHT;
        }
        
        // Vertical line
        DrawLine(vert_line_pos, 0, vert_line_pos, GetScreenHeight(), WHITE);

        // Horizontal line
        DrawLine(0, horz_line_pos, MAIN_SCREEN_WIDTH, horz_line_pos, WHITE);
    }
    DrawLine((int) MAIN_SCREEN_WIDTH, 0, (int) MAIN_SCREEN_WIDTH, GetScreenHeight(), WHITE);
}


int main(void)
{
    SetTraceLogLevel(LOG_NONE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "DOOM LEVEL EDITOR");

    SetTargetFPS(60);

    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        BeginDrawing();
        {
            ClearBackground(Fade(BLACK, 0.6f));
            mouse_position = GetMousePosition();
            draw_grid();

            if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
                // POSITION IN REAL-WORLD (ms_ix, ms_iy)
                int ms_ix = (int) mouse_position.x - vert_line_offset;
                int ms_iy = (int) mouse_position.y - horz_line_offset;

                int corner_x;
                int corner_y;

                if (ms_ix < 0) {
                    corner_x = ms_ix % tile_width < -tile_width/2 ? -1 : 0;
                } else {
                    corner_x = ms_ix % tile_width > tile_width/2;
                }

                if (ms_iy < 0) {
                    corner_y = ms_iy % tile_width < -tile_width/2 ? -1 : 0;
                } else {
                    corner_y = ms_iy % tile_width > tile_width/2;
                }

                Vector2 ball = {
                    .x = tile_width * (corner_x + ms_ix / tile_width),
                    .y = tile_width * (corner_y + ms_iy / tile_width)
                };

                if (float_distance2(ball.x + vert_line_offset, ball.y + horz_line_offset, mouse_position.x, mouse_position.y) < tile_width*tile_width/5) {
                    da_append(&balls, ball);
                }
            }

            // RENDERING POTENTIAL BALL (COLOUR GREEN)
            {
                int ms_ix = (int) mouse_position.x - vert_line_offset;
                int ms_iy = (int) mouse_position.y - horz_line_offset;

                int corner_x;
                int corner_y;
                
                if (ms_ix < 0) {
                    corner_x = ms_ix % tile_width < -tile_width/2 ? -1 : 0;
                } else {
                    corner_x = ms_ix % tile_width > tile_width/2;
                }
                
                if (ms_iy < 0) {
                    corner_y = ms_iy % tile_width < -tile_width/2 ? -1 : 0;
                } else {
                    corner_y = ms_iy % tile_width > tile_width/2;
                }
                
                Vector2 potential_ball = {
                    .x = tile_width * (corner_x + ms_ix / tile_width) + vert_line_offset,
                    .y = tile_width * (corner_y + ms_iy / tile_width) + horz_line_offset
                };

                if (is_in_screen(&potential_ball) && vector_distance2(&potential_ball, &mouse_position) < tile_width*tile_width/5) {
                    DrawCircle((int) potential_ball.x,
                               (int) potential_ball.y,
                               5.0f, GREEN);
                }
            }

            // RENDERING EVERY BALLS
            for (int i = 0; i < balls.count; i++) {
                Vector2 *ball = &(balls.values[i]);
                Vector2 rendered_ball = {
                    .x = (int) ball->x + vert_line_offset,
                    .y = (int) ball->y + horz_line_offset
                };
                if (is_in_screen(&rendered_ball)) {
                    DrawCircle(rendered_ball.x,
                               rendered_ball.y,
                               5.0f, RED);
                }
            }

            DrawFPS(10, 10);
        }
        EndDrawing();
    }
    CloseWindow();

    da_free(&balls);

    return 0;
}
