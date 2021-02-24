#include <cstdio>
#include <algorithm>
#include <chrono>
#include <math.h>

#include "display.h"

const uint8_t bar_bitmap[] = {
    0b11111000,
    0b11111000,
    0b11111000,
    0b11111000,
    0b11111000,
    0b11111000,
    0b11111000,
    0b11111000,
    0b11111000,
    0b11111000,
    0b11111000,
    0b11111000,
    0b11111000,
    0b11111000,
    0b11111000,
    0b11111000,
};

const uint8_t ball_bitmap[] = {
    0b1110000,
    0b1110000,
    0b1110000,
};

int rand_range(int min, int max) {
    return rand()%(max-min + 1) + min;
}

float calc_slope(float x1, float y1, float x2, float y2) {
    return (y2 - y1) / (x2 - x1);
}

void init_i2c() {
	i2c_init(I2C_PORT, 100 * 1000);
	gpio_set_function(16, GPIO_FUNC_I2C);
	gpio_set_function(17, GPIO_FUNC_I2C);
	gpio_pull_up(16);
	gpio_pull_up(17);
}

#define BUTTON1_PIN 14
#define BUTTON2_PIN 15

void init_buttons() {
    gpio_init(BUTTON1_PIN);
    gpio_set_dir(BUTTON1_PIN, GPIO_IN);
    gpio_pull_up(BUTTON1_PIN);

    gpio_init(BUTTON2_PIN);
    gpio_set_dir(BUTTON2_PIN, GPIO_IN);
    gpio_pull_up(BUTTON2_PIN);
}

int main() {
    stdio_init_all();

	init_i2c();
	init_display();
    init_buttons();

    srand(std::time(NULL));

    int score = 0;
    int enemy_score = 0;

    int paddle_speed = 3;

    int y = 24;

    int enemy_y = 24;
    int enemy_last_y = enemy_y;
    int enemy_next_y = rand_range(0, 64-16);

    float ball_speed = 12;
    float ball_x = (128-3)/2;
    float ball_y = (64-3)/2;
    float ball_next_x = (0-3);
    float ball_next_y = 64;
    bool ball_dir = false;
    float ball_diff_x = abs(ball_x - ball_next_x);
    float ball_diff_y = abs(ball_y - ball_next_y);
    float ball_distance = sqrt(ball_diff_x*ball_diff_x + ball_diff_y*ball_diff_y);

    auto reset_positions = [&]() {
        y = 24;
        enemy_y = 24;

        ball_x = (128-3)/2;
        ball_y = (64-3)/2;
    };

    auto hit_ball = [&]() {
        ball_next_x = ball_dir ? (128+ball_speed) : (0-ball_speed);
        ball_next_y = rand_range(0, 64);
        ball_diff_x = abs(ball_x - ball_next_x);
        ball_diff_y = abs(ball_y - ball_next_y);
        ball_distance = sqrt(ball_diff_x*ball_diff_x + ball_diff_y*ball_diff_y);
    };

    char score_board[16];

    while (true) {
        fill_scr(0);

        // my movement logic
        // if (!gpio_get(BUTTON1_PIN)) {
        //     y -= paddle_speed;
        // }
        // if (!gpio_get(BUTTON2_PIN)) {
        //     y += paddle_speed;
        // }
        // y = std::min(std::max(0, y), 64-16);
        y = (ball_next_y - 8);

        // enemy movement logic
        enemy_last_y = enemy_y;
        if (enemy_y > enemy_next_y) {
            enemy_y -= paddle_speed;
        }
        if (enemy_y < enemy_next_y) {
            enemy_y += paddle_speed;
        }
        enemy_y = std::min(std::max(0, enemy_y), 64-16);
        if (enemy_last_y == enemy_y && ball_dir == true) {
            // enemy_next_y = rand_range(0, 64-16);
            enemy_next_y = (ball_next_y - 8);
        }

        // ball movement logic
        if (ball_x < ball_next_x) {
            ball_x += (ball_diff_x / ball_distance) * ball_speed;
        }
        if (ball_y < ball_next_y) {
            ball_y += (ball_diff_y / ball_distance) * ball_speed;
        }
        if (ball_x > ball_next_x) {
            ball_x -= (ball_diff_x / ball_distance) * ball_speed;
        }
        if (ball_y > ball_next_y) {
            ball_y -= (ball_diff_y / ball_distance) * ball_speed;
        }

        // ball bounce back logic
        if (ball_dir == false && ball_x <= (0+3)) {
            if (ball_y >= y && ball_y <= y+16) { // currently only checking if the top left pixel is touching the bar
                ball_dir = true;
                hit_ball();
            }
        } else if (ball_dir == true && ball_x >= (128-3-3)) {
            if (ball_y >= enemy_y && ball_y <= enemy_y+16) {
                ball_dir = false;
                hit_ball();
            }
        }

        // scoring logic
        if (ball_x <= 0 || ball_x >= 128) {
            if (ball_dir == false) {
                enemy_score++;
            } else {
                score++;
            }

            reset_positions();
            hit_ball();
        }

        // score board
        snprintf(score_board, 16, "%i - %i", score, enemy_score);
        set_cursor((128 - (8 * strlen(score_board)))/2, 0);
        ssd1306_print(score_board);
        
        // game objects
        drawBitmap(0, y, bar_bitmap, 2, 16, 1);
        drawBitmap(128-2, enemy_y, bar_bitmap, 2, 16, 1);
        drawBitmap(ball_x, ball_y, ball_bitmap, 3, 3, 1);
        
        // set bitmap to screen
        show_scr();
    }

	return 0;
}