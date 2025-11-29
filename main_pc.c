#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "chip8-core/chip8.h"
#include "lvgl/lvgl.h"

#define WINDOW_ZOOM 1000
#define SCALE_FACTOR 10

#define COLOR_ON  0xFFFFA500 // Orange, Full Alpha
#define COLOR_OFF 0xFF402900 // Dark Brown, Full Alpha

int main(void) {
    srand(time(NULL));
    chip8_t chip;
    chip8_init(&chip);

    const char* rom_filename = "../test-roms/maze.ch8";
    FILE* fptr = fopen(rom_filename, "rb");

    if (fptr == NULL) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    fseek(fptr, 0, SEEK_END);
    const size_t rom_size = ftell(fptr);
    rewind(fptr);

    if (rom_size <= 0) {
        perror("ROM is empty or failed to reading size");
        return EXIT_FAILURE;
    }

    uint8_t* rom_buffer = malloc(rom_size);
    if (rom_buffer == NULL) {
        perror("Error allocating memory");
        return EXIT_FAILURE;
    }

    const size_t read_result = fread(rom_buffer, 1, rom_size, fptr);
    if (read_result != rom_size) {
        perror("Failed to read ROM");
        free(rom_buffer);
        return EXIT_FAILURE;
    }

    const bool load_result = chip8_load_rom(&chip, rom_buffer, rom_size);
    if (!load_result) {
        perror("Failed to load ROM in chip8");
        return EXIT_FAILURE;
    }

    free(rom_buffer);
    fclose(fptr);

    static uint32_t display_buffer[CHIP8_SCREEN_WIDTH * SCALE_FACTOR * CHIP8_SCREEN_HEIGHT * SCALE_FACTOR];

    lv_init();

    lv_display_t* display = lv_windows_create_display(
        L"CHIP-8 Emulator",
        CHIP8_SCREEN_WIDTH * SCALE_FACTOR, CHIP8_SCREEN_HEIGHT * SCALE_FACTOR,
        LV_WINDOWS_ZOOM_BASE_LEVEL, false, false
    );

    if (!display) return EXIT_FAILURE;

    lv_display_set_buffers(display, display_buffer, NULL, sizeof(display_buffer), LV_DISPLAY_RENDER_MODE_PARTIAL);

    LV_DRAW_BUF_DEFINE_STATIC(draw_buf, CHIP8_SCREEN_WIDTH, CHIP8_SCREEN_HEIGHT, LV_COLOR_FORMAT_ARGB8888);
    LV_DRAW_BUF_INIT_STATIC(draw_buf);

    lv_obj_t* canvas = lv_canvas_create(lv_screen_active());
    lv_canvas_set_draw_buf(canvas, &draw_buf);

    lv_image_set_scale(canvas, 256 * SCALE_FACTOR);
    lv_image_set_antialias(canvas, false);

    lv_canvas_fill_bg(canvas, lv_color_hex(0xFFA500), LV_OPA_COVER);
    lv_obj_center(canvas);

    for (uint8_t x = 0; x < CHIP8_SCREEN_WIDTH; x++) {
        lv_canvas_set_px(canvas, x, 10, lv_color_hex(0xFF0000), LV_OPA_COVER); // Red Line
    }

    const uint8_t instructions_per_frame = 10;
    for (;;) {
        uint32_t start_tick = lv_tick_get();

        for (uint32_t i = 0; i < instructions_per_frame; i++) {
            chip8_cycle(&chip);
        }
        chip8_tick_timers(&chip);

        uint32_t*raw_buffer = (uint32_t*)lv_canvas_get_buf(canvas);
        for (size_t i = 0; i < CHIP8_SCREEN_WIDTH * CHIP8_SCREEN_HEIGHT; i++) {
            uint8_t pixel = chip.display[i];

            raw_buffer[i] = pixel == 0 ? COLOR_ON : COLOR_OFF;
        }

        for (uint32_t y = 0; y < CHIP8_SCREEN_HEIGHT; y++) {
            for (uint32_t x = 0; x < CHIP8_SCREEN_WIDTH; x++) {
                const size_t index = y * CHIP8_SCREEN_WIDTH + x;

                const uint8_t pixel_on = chip.display[index];

                const lv_color_t color = pixel_on ? lv_color_hex(0xFFA500) : lv_color_hex(0x402900);

                lv_canvas_set_px(canvas, x, y, color, LV_OPA_COVER);
            }
        }

        lv_obj_invalidate(canvas);

        lv_timer_handler();

        uint32_t elapsed = lv_tick_get() - start_tick;

        if (elapsed < CHIP8_TARGET_FRAME_TIME) {
            lv_sleep_ms(CHIP8_TARGET_FRAME_TIME - elapsed);
        }
        else {
            lv_sleep_ms(1);
        }
    }


    return EXIT_SUCCESS;
}
