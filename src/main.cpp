#include <cmath>

#include "config.h"
#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

enum class GameState {
    MAIN_MENU,
    ROUND_START,
    PAUSED,
    PLAYING
};

enum class GameMode {
    SINGLE_PLAYER,
    MULTI_PLAYER
};

struct Ball {
    Vector2 position;
    Vector2 velocity;
    float radius;
};

void reset_game(Rectangle& paddle_left, Rectangle& paddle_right, Ball& ball) {
    paddle_left = {
        BORDER_OFFSET + PADDLE_OFFSET_FROM_BORDER,
        HEIGHT / 2.0 - PADDLE_HEIGHT / 2,
        PADDLE_WIDTH,
        PADDLE_HEIGHT};

    paddle_right = {
        WIDTH - BORDER_OFFSET - PADDLE_OFFSET_FROM_BORDER - PADDLE_WIDTH,
        HEIGHT / 2.0 - PADDLE_HEIGHT / 2,
        PADDLE_WIDTH,
        PADDLE_HEIGHT};

    ball = {{WIDTH / 2.0, HEIGHT / 2.0}, {0, 0}, BALL_RADIUS};
    float initial_angle = GetRandomValue(-45, 45) * DEG2RAD;
    float initial_direction = GetRandomValue(0, 1) == 0 ? -1.0f : 1.0f;  // left or right
    ball.velocity.x = cosf(initial_angle) * initial_direction * BALL_SPEED;
    ball.velocity.y = sinf(initial_angle) * BALL_SPEED;
}

void draw_border() {
    DrawRectangleLinesEx(Rectangle{BORDER_OFFSET, BORDER_OFFSET,
                                   WIDTH - 2 * BORDER_OFFSET, HEIGHT - 2 * BORDER_OFFSET},
                         2.0f, WHITE);
}

void draw_player_scores(int left, int right) {
    int font_size = 25;

    const char* left_text = TextFormat("Score: %d", left);
    DrawText(left_text, BORDER_OFFSET, 10, font_size, WHITE);

    const char* right_text = TextFormat("Score: %d", right);
    int text_width = MeasureText(right_text, font_size);

    DrawText(right_text,
             WIDTH - BORDER_OFFSET - text_width,
             10,
             font_size,
             WHITE);
}

void draw_paddles(Rectangle paddle_left, Rectangle paddle_right) {
    DrawRectangleRec(paddle_left, WHITE);
    DrawRectangleRec(paddle_right, WHITE);
}

void draw_ball(Ball ball) {
    DrawCircleV(ball.position, ball.radius, WHITE);
}

void main_menu(GameState& state, GameMode& mode) {
    int pong_title_width = MeasureText("PONG!", 100);
    DrawText("PONG!", WIDTH / 2.0f - (pong_title_width / 2.0f), 100, 100, WHITE);
    DrawLineEx({WIDTH / 2.0f - (pong_title_width / 2.0f) - 50, 220},
               {WIDTH / 2.0f + (pong_title_width / 2.0f) + 50, 220}, 2, WHITE);

    // +20 is for padding
    float button_width = MeasureText("SINGLE PLAYER", MENU_BUTTON_FONT_SIZE) + 20;
    float button_height = MENU_BUTTON_FONT_SIZE + 20;

    Rectangle singleplayer_button_bounds = {
        WIDTH / 2 - button_width / 2,
        (HEIGHT / 2) - (button_height / 2) - 50,
        button_width, button_height};

    Rectangle multiplayer_button_bounds = {
        WIDTH / 2 - button_width / 2,
        (HEIGHT / 2) - (button_height / 2) + 50,
        button_width, button_height};

    if (GuiButton(singleplayer_button_bounds, "SINGLE PLAYER")) {
        mode = GameMode::SINGLE_PLAYER;
        state = GameState::ROUND_START;
    }

    if (GuiButton(multiplayer_button_bounds, "MULTIPLAYER")) {
        mode = GameMode::MULTI_PLAYER;
        state = GameState::ROUND_START;
    }
}

void draw_pause() {
    int font_size = 50;
    DrawText("PAUSED", WIDTH / 2 - MeasureText("PAUSED", font_size) / 2,
             HEIGHT / 2 - font_size / 2, font_size, RED);
}

void draw_round_start() {
    int font_size = 50;
    DrawText("PRESS ENTER TO START", WIDTH / 2 - MeasureText("PRESS ENTER TO START", font_size) / 2,
             HEIGHT / 4 - font_size / 2, font_size, WHITE);
}

void update_ball(Ball& ball, Rectangle& paddle_left, Rectangle& paddle_right,
                 int& player_left_score, int& player_right_score, GameState& state) {
    ball.position.x += ball.velocity.x;
    ball.position.y += ball.velocity.y;

    if (CheckCollisionCircleRec(ball.position, ball.radius, paddle_left)) {
        float paddle_left_center_y = paddle_left.y + PADDLE_HEIGHT / 2;
        float hit_offset = ball.position.y - paddle_left_center_y;
        float normalized = hit_offset / (PADDLE_HEIGHT / 2.0f);
        float angle = normalized * MAX_BOUNCE_ANGLE;
        ball.velocity.x = cos(angle) * BALL_SPEED * 1;  // 1 = direction (left->right)
        ball.velocity.y = sin(angle) * BALL_SPEED;
    }

    if (CheckCollisionCircleRec(ball.position, ball.radius, paddle_right)) {
        float paddle_right_center_y = paddle_right.y + PADDLE_HEIGHT / 2;
        float hit_offset = ball.position.y - paddle_right_center_y;
        float normalized = hit_offset / (PADDLE_HEIGHT / 2.0f);
        float angle = normalized * MAX_BOUNCE_ANGLE;
        ball.velocity.x = cos(angle) * BALL_SPEED * -1;
        ball.velocity.y = sin(angle) * BALL_SPEED;  // -1 = direction (right->left)
    }

    Vector2 top_left = {BORDER_OFFSET, BORDER_OFFSET};
    Vector2 top_right = {WIDTH - BORDER_OFFSET, BORDER_OFFSET};
    Vector2 bottom_left = {BORDER_OFFSET, HEIGHT - BORDER_OFFSET};
    Vector2 bottom_right = {WIDTH - BORDER_OFFSET, HEIGHT - BORDER_OFFSET};

    if (CheckCollisionCircleLine(ball.position, ball.radius, top_left, top_right) ||
        CheckCollisionCircleLine(ball.position, ball.radius, bottom_left, bottom_right)) {
        ball.velocity.y *= -1;
    }

    if (CheckCollisionCircleLine(ball.position, ball.radius, top_left, bottom_left)) {
        player_right_score++;
        state = GameState::ROUND_START;
    }

    if (CheckCollisionCircleLine(ball.position, ball.radius, top_right, bottom_right)) {
        player_left_score++;
        state = GameState::ROUND_START;
    }
}

void handle_paddle_input_multiplayer(Rectangle& paddle_left, Rectangle& paddle_right) {
    if (IsKeyDown(KEY_W)) {
        if (paddle_left.y > BORDER_OFFSET + PADDLE_OFFSET_FROM_BORDER) {
            paddle_left.y -= PADDLE_SPEED;
        }
    }

    if (IsKeyDown(KEY_S)) {
        if (paddle_left.y + PADDLE_HEIGHT < HEIGHT - BORDER_OFFSET - PADDLE_OFFSET_FROM_BORDER) {
            paddle_left.y += PADDLE_SPEED;
        }
    }

    if (IsKeyDown(KEY_UP)) {
        if (paddle_right.y > BORDER_OFFSET + PADDLE_OFFSET_FROM_BORDER) {
            paddle_right.y -= PADDLE_SPEED;
        }
    }

    if (IsKeyDown(KEY_DOWN)) {
        if (paddle_right.y + PADDLE_HEIGHT < HEIGHT - BORDER_OFFSET - PADDLE_OFFSET_FROM_BORDER) {
            paddle_right.y += PADDLE_SPEED;
        }
    }
}

void handle_paddle_input_singleplayer(Rectangle& paddle_left) {
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) {
        if (paddle_left.y > BORDER_OFFSET + PADDLE_OFFSET_FROM_BORDER) {
            paddle_left.y -= PADDLE_SPEED;
        }
    }

    if (IsKeyDown(KEY_S) || (IsKeyDown(KEY_DOWN))) {
        if (paddle_left.y + PADDLE_HEIGHT < HEIGHT - BORDER_OFFSET - PADDLE_OFFSET_FROM_BORDER) {
            paddle_left.y += PADDLE_SPEED;
        }
    }
}

void cpu_move(Ball ball, Rectangle& paddle) {
    if ((ball.position.y > paddle.y + PADDLE_HEIGHT / 2) &&
        (paddle.y + PADDLE_HEIGHT < HEIGHT - BORDER_OFFSET - PADDLE_OFFSET_FROM_BORDER)) {
        paddle.y += PADDLE_SPEED;
    } else if ((ball.position.y < paddle.y + PADDLE_HEIGHT / 2) &&
               (paddle.y > BORDER_OFFSET + PADDLE_OFFSET_FROM_BORDER)) {
        paddle.y -= PADDLE_SPEED;
    }
}

int main() {
    InitWindow(WIDTH, HEIGHT, "Pong");
    SetTargetFPS(60);

    GuiLoadStyle("style/style_terminal.rgs");
    GuiSetStyle(DEFAULT, TEXT_SIZE, MENU_BUTTON_FONT_SIZE);

    GameState state = GameState::MAIN_MENU;
    GameMode mode;

    int player_left_score = 0;
    int player_right_score = 0;

    Rectangle paddle_left;
    Rectangle paddle_right;
    Ball ball;

    bool round_initialized = false;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        if (state != GameState::MAIN_MENU) {
            draw_border();
            draw_player_scores(player_left_score, player_right_score);
            draw_paddles(paddle_left, paddle_right);
            draw_ball(ball);
        }

        switch (state) {
            case GameState::MAIN_MENU:
                main_menu(state, mode);
                break;

            case GameState::ROUND_START:
                if (!round_initialized) {
                    reset_game(paddle_left, paddle_right, ball);
                    round_initialized = true;
                }

                draw_round_start();
                if (IsKeyPressed(KEY_ENTER)) {
                    round_initialized = false;
                    state = GameState::PLAYING;
                }
                break;

            case GameState::PLAYING:
                if (mode == GameMode::MULTI_PLAYER) {
                    handle_paddle_input_multiplayer(paddle_left, paddle_right);
                }

                if (mode == GameMode::SINGLE_PLAYER) {
                    handle_paddle_input_singleplayer(paddle_left);
                    cpu_move(ball, paddle_right);
                }

                update_ball(ball, paddle_left, paddle_right, player_left_score, player_right_score, state);

                if (IsKeyPressed(KEY_P)) state = GameState::PAUSED;
                break;

            case GameState::PAUSED:
                if (IsKeyPressed(KEY_ENTER)) state = GameState::PLAYING;
                draw_pause();
                break;
        }

        EndDrawing();
    }

    CloseWindow();
}