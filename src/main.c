#include <stdio.h>
#include <stdlib.h>
#include <raylib.h>
#include <raygui.h>

#include "dynamic_array.h"

CREATE_DYNAMIC_ARRAY(Vector2);

typedef enum Edit_Mode { NONE, SECTOR } Edit_Mode;

typedef struct Sector {
    DynamicArray_Vector2 anchors;
} Sector; 

const char *GUI_STYLE = "styles/style_candy.rgs";
const int SCREEN_WIDTH = 1080;
const int SCREEN_HEIGHT = 720;
const int ZOOM_LEVEL = 10;

// The side menu occupies 1/3 of the screen width (right-sided)
const double SIDE_MENU_RATIO = 1.0 / 3.0;
const double MAIN_SCREEN_WIDTH = SCREEN_WIDTH * (1 - SIDE_MENU_RATIO);
const double SIDE_SCREEN_WIDTH = SCREEN_WIDTH * SIDE_MENU_RATIO;

Vector2 mouse_position = {0};
Edit_Mode edit_mode = NONE;

// X AND Y OFFSETS
int vert_line_offset = 0;
int horz_line_offset = 0;
int nb_tiles = 10;
int tile_width;

DynamicArray_Vector2 sector_anchors = {0};

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

static Vector2 vector2_offset(Vector2 v, Vector2 o)
{
    Vector2 ret = (Vector2) {
        .x = v.x + o.x,
        .y = v.y + o.y
    };

    return ret;
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

    /* if (IsKeyPressed(KEY_A)) { */
    /*     nb_tiles += ZOOM_LEVEL; */
    /* } */
    
    /* if (IsKeyPressed(KEY_Q)) { */
    /*     if (nb_tiles - ZOOM_LEVEL > 0) { */
    /*         nb_tiles -= ZOOM_LEVEL; */
    /*     } */
    /* } */

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


static Vector2 get_point_on_the_grid(Vector2 position_on_screen) {
    // POSITION IN REAL-WORLD (ms_ix, ms_iy)
    int ms_ix = (int) position_on_screen.x - vert_line_offset;
    int ms_iy = (int) position_on_screen.y - horz_line_offset;

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
    
    return ball;
}

static void fill_polygon(DynamicArray_Vector2 *anchors)
{
    if (anchors->count <= 0) return;

    Vector2 offset = (Vector2) {
        .x = vert_line_offset,
        .y = horz_line_offset
    };
    
    for (int i = 0; i < anchors->count-1; i++) {
        DrawTriangle(vector2_offset(anchors->values[0], offset),
                     vector2_offset(anchors->values[i], offset),
                     vector2_offset(anchors->values[i+1], offset),
                     Fade(ORANGE, 0.5f));
    }
}

static void render_sector(DynamicArray_Vector2 *anchors)
{
    Vector2 prev_anchor = { 0 };
    int prev_anchor_valid = 0;
    for (int i = 0; i < sector_anchors.count; i++) {
        Vector2 *anchor = &(sector_anchors.values[i]);
        Vector2 rendered_anchor = {
            .x = (int) anchor->x + vert_line_offset,
            .y = (int) anchor->y + horz_line_offset
        };

        int cur_anchor_backup_x = rendered_anchor.x;
        int cur_anchor_backup_y = rendered_anchor.y;
        // Draw a segment joining two walls
        if (prev_anchor_valid) {
            
            int prev_anchor_backup_x = prev_anchor.x;
            int prev_anchor_backup_y = prev_anchor.y;
            
            if (prev_anchor.x > MAIN_SCREEN_WIDTH) {
                double t = (MAIN_SCREEN_WIDTH - cur_anchor_backup_x) / (prev_anchor_backup_x - cur_anchor_backup_x);
                prev_anchor.x = MAIN_SCREEN_WIDTH;
                prev_anchor.y = cur_anchor_backup_y + t * (prev_anchor_backup_y - cur_anchor_backup_y);
            }

            if (rendered_anchor.x > MAIN_SCREEN_WIDTH) {
                double t = (MAIN_SCREEN_WIDTH - cur_anchor_backup_x) / (prev_anchor_backup_x - cur_anchor_backup_x);
                rendered_anchor.x = MAIN_SCREEN_WIDTH;
                rendered_anchor.y = cur_anchor_backup_y + t * (prev_anchor_backup_y - cur_anchor_backup_y);
            }

            DrawLine(prev_anchor.x, prev_anchor.y, rendered_anchor.x, rendered_anchor.y, BLUE);
        }
                
        rendered_anchor.x = cur_anchor_backup_x;
        rendered_anchor.y = cur_anchor_backup_y;
        
        if (is_in_screen(&rendered_anchor)) {
            DrawCircle(rendered_anchor.x,
                       rendered_anchor.y,
                       5.0f, RED);
        }

            
        prev_anchor = rendered_anchor;
        prev_anchor_valid = 1;
    }
    fill_polygon(anchors);
}

int main(void)
{
    SetTraceLogLevel(LOG_NONE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "DOOM LEVEL EDITOR");
    GuiLoadStyle(GUI_STYLE);

    SetTargetFPS(60);

    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        BeginDrawing();
        {
            ClearBackground(Fade(BLACK, 0.6f));
            mouse_position = GetMousePosition();
            draw_grid();

            if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && edit_mode == SECTOR) {
                Vector2 anchor = get_point_on_the_grid(mouse_position);

                if (float_distance2(anchor.x + vert_line_offset, anchor.y + horz_line_offset, mouse_position.x, mouse_position.y) < tile_width*tile_width/5) {
                    da_append(&sector_anchors, anchor);
                }
            }

            // RENDERING POTENTIAL BALL (COLOUR GREEN)
            {
                Vector2 potential_anchor = get_point_on_the_grid(mouse_position);
                potential_anchor.x += vert_line_offset;
                potential_anchor.y += horz_line_offset;

                if (is_in_screen(&potential_anchor) && vector_distance2(&potential_anchor, &mouse_position) < tile_width*tile_width/5) {
                    DrawCircle((int) potential_anchor.x,
                               (int) potential_anchor.y,
                               5.0f, GREEN);
                }
            }

            // RENDERING EVERY BALLS
            render_sector(&sector_anchors);

            char mode_text[1024];
            switch (edit_mode) {
            case NONE:
                if (GuiButton((Rectangle) { MAIN_SCREEN_WIDTH + 100, SCREEN_HEIGHT / 2, 200, 50 }, "AJOUTER UN SECTEUR")) {
                    edit_mode = SECTOR;
                }
                sprintf(mode_text, "AUCUN MODE N'EST ACTIVÉ");
                break;
            case SECTOR:
                if (GuiButton((Rectangle) { MAIN_SCREEN_WIDTH + 100, SCREEN_HEIGHT / 2, 200, 50 }, "ARRÊTER LE MODE D'ÉDITION")) {
                    edit_mode = NONE;
                }
                sprintf(mode_text, "MODE: Ajouter un nouveau secteur");
                break;
            }


            GuiLabel((Rectangle){ MAIN_SCREEN_WIDTH + 100, 100, 200, 50 }, mode_text);
            
            DrawFPS(10, 10);
        }
        EndDrawing();
    }
    CloseWindow();

    da_free(&sector_anchors);

    return 0;
}
