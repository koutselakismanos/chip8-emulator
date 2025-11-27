#include "chip8.h"

#include <assert.h>
#include <string.h>

static const uint8_t FONTSET[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80 // F
};

static const char KEYBAD_LABELS[16] = {
    '1', '2', '3', 'C',
    '4', '5', '6', 'D',
    '7', '8', '9', 'E',
    'A', '0', 'B', 'F'
};

void chip8_init(chip8_t* chip) {
    memset(chip, 0, sizeof(chip8_t));

    chip->registers.pc = CHIP8_MEMORY_ROM_OFFSET;


    memcpy(&chip->memory[0x50], FONTSET, sizeof(FONTSET));

    for (size_t i = 0; i < 16; i++) {
        chip->keypad[i].label = KEYBAD_LABELS[i];
    }
}

bool chip8_load_rom(chip8_t* chip, const uint8_t* buffer, const size_t size) {
    // this should be constexpr, but I am not sure if I will switch to c17 or stay on c23
    const size_t max_size = CHIP8_MEMORY_SIZE - CHIP8_MEMORY_ROM_OFFSET;
    if (size > max_size) {
        return false;
    }

    memcpy(&chip->memory[CHIP8_MEMORY_ROM_OFFSET], buffer, size);

    return true;
}

static void op_0_sys(chip8_t* chip, const uint16_t opcode) {
    switch (opcode & 0x00FF) {
    case 0xE0:
        // Clear the display
        memset(&chip->display, 0, sizeof(chip->display));
        break;
    case 0xEE:
        chip->registers.pc = chip->stack[chip->registers.sp];
        chip->registers.sp--;
        break;
    default:
        // Docs say to ignore the SYS addr instruction
        break;
    }
}

static void op_1_jump(chip8_t* chip, const uint16_t opcode) {
    const uint16_t nnn = opcode & 0x0FFF;
    chip->registers.pc = nnn;
}

static void op_2_call(chip8_t* chip, const uint16_t opcode) {
    const uint16_t nnn = opcode & 0x0FFF;
    chip->registers.sp++;
    // Maybe we should have a safety wrap here

    chip->stack[chip->registers.sp] = chip->registers.pc;
    chip->registers.pc = nnn;
}

static void op_3_se(chip8_t* chip, const uint16_t opcode) {
    const uint8_t kk = opcode & 0xFF;
    const uint8_t x = opcode >> 8 & 0xF;
    if (chip->registers.vx[x] == kk) {
        chip->registers.pc += 2;
    }
}

static void op_4_sne(chip8_t* chip, const uint16_t opcode) {
    const uint8_t kk = opcode & 0xFF;
    const uint8_t x = opcode >> 8 & 0xF;
    if (chip->registers.vx[x] != kk) {
        chip->registers.pc += 2;
    }
}

static void op_5_se(chip8_t* chip, const uint16_t opcode) {
    const uint8_t x_register = opcode >> 8 & 0xF;
    const uint8_t y_register = opcode >> 4 & 0xF;
    if (chip->registers.vx[x_register] == chip->registers.vx[y_register]) {
        chip->registers.pc += 2;
    }
}

static void op_6_ld(chip8_t* chip, const uint16_t opcode) {
    const uint8_t x_register = opcode >> 8 & 0x0F;
    chip->registers.vx[x_register] = opcode & 0x00FF;
}


static void op_7_add(chip8_t* chip, const uint16_t opcode) {
    const uint8_t kk = opcode & 0xFF;
    const uint8_t x = opcode >> 8 & 0xF;
    chip->registers.vx[x] = chip->registers.vx[x] + kk;
}

static void op_8_math(chip8_t* chip, const uint16_t opcode) {
    const uint8_t x_register = opcode >> 8 & 0x0F;
    const uint8_t y_register = opcode >> 4 & 0x0F;
    const uint8_t vx = chip->registers.vx[x_register];
    const uint8_t vy = chip->registers.vx[y_register];

    switch (opcode & 0x0F) {
    case 0x0:
        chip->registers.vx[x_register] = vy;
        break;
    case 0x1:
        chip->registers.vx[x_register] = vx | vy;
        break;
    case 0x2:
        chip->registers.vx[x_register] = vx & vy;
        break;
    case 0x3:
        chip->registers.vx[x_register] = vx ^ vy;
        break;
    case 0x4:
        const uint16_t result = vx + vy;
        chip->registers.vx[0xF] = 1;
        if (result > 255) {
            chip->registers.vx[0xF] = 1;
        }
        chip->registers.vx[x_register] = result & 0xFF;
        break;
    case 0x5:
        chip->registers.vx[0xF] = 0;
        if (vx > vy) {
            chip->registers.vx[0xF] = 1;
        }
        chip->registers.vx[x_register] = vx - vy;
        break;
    case 0x6:
        chip->registers.vx[0xF] = 0;
        if (vx & 0x1) {
            chip->registers.vx[0xF] = 1;
        }
        chip->registers.vx[x_register] = vx >> 1;
        break;
    case 0x7:
        chip->registers.vx[0xF] = 0;
        if (vy > vx) {
            chip->registers.vx[0xF] = 1;
        }

        chip->registers.vx[x_register] = vy - vx;
        break;
    case 0xE:
        chip->registers.vx[0xF] = 0;
        if (vx >> 7 & 0x1) {
            chip->registers.vx[0xF] = 1;
        }
        chip->registers.vx[x_register] = vx << 1;
    default: ;
        // we do nothing :D
    }
}

static void op_9_sne(chip8_t* chip, const uint16_t opcode) {
    const uint8_t x_register = opcode >> 8 & 0x0F;
    const uint8_t y_register = opcode >> 4 & 0x0F;
    if (chip->registers.vx[x_register] != chip->registers.vx[y_register]) {
        chip->registers.pc += 2;
    }
}

static void op_a_ld_i(chip8_t* chip, const uint16_t opcode) {
    chip->registers.i = opcode & 0x0FFF;
}

static void op_b_jump_v0(chip8_t* chip, uint16_t opcode) {
    assert(false);
}

static void op_c_rnd(chip8_t* chip, uint16_t opcode) {
    assert(false);
}

static void op_d_drw(chip8_t* chip, const uint16_t opcode) {
    const uint8_t vx_index = opcode >> 8 & 0x0F;
    const uint8_t vy_index = opcode >> 4 & 0x0F;
    const uint8_t height = opcode & 0x0F;

    const uint8_t x_pos = chip->registers.vx[vx_index] % CHIP8_SCREEN_WIDTH;
    const uint8_t y_pos = chip->registers.vx[vy_index] % CHIP8_SCREEN_HEIGHT;

    chip->registers.vx[0xF] = 0;

    for (size_t row = 0; row < height; row++) {
        const uint8_t sprite_byte = chip->memory[chip->registers.i + row];

        for (size_t col = 0; col < 8; col++) {
            const uint8_t sprite_pixel = sprite_byte & (0x80 >> col);

            if (sprite_pixel == 0) continue;

            const uint8_t screen_x = x_pos + col;
            const uint8_t screen_y = y_pos + row;

            if (screen_x >= CHIP8_SCREEN_WIDTH || screen_y >= CHIP8_SCREEN_HEIGHT) {
                continue;
            }

            // calculate screen index for 1D display array
            const uint16_t screen_index = (screen_y * CHIP8_SCREEN_WIDTH) + screen_x;

            if (chip->display[screen_index] == 1) {
                chip->registers.vx[0xF] = 1;
            }

            chip->display[screen_index] ^= 1;
        }
    }
}

static void op_e_key(chip8_t* chip, const uint16_t opcode) {
    const uint8_t x_register = opcode >> 8 & 0x0F;
    const chip8_key key = chip->keypad[chip->registers.vx[x_register]];
    switch (opcode & 0xFF) {
    case 0x9E:
        if (!key.is_pressed) {
            break;
        }
        chip->registers.pc += 2;
        break;
    case 0xA1:
        if (key.is_pressed) {
            break;
        }
        chip->registers.pc += 2;
        break;
    default:
        break;
    }
}

static void op_f_misc(chip8_t* chip, const uint16_t opcode) {
    const uint8_t x_register = opcode >> 8 & 0x0F;
    uint8_t* const vx = &chip->registers.vx[x_register];
    switch (opcode & 0xFF) {
    case 0x07:
        *vx = chip->registers.delay_timer;
        break;
    case 0x0A:
        bool key_pressed = false;
        for (int i = 0; i < 16; i++) {
            if (chip->keypad[i].is_pressed) {
                *vx = i;
                key_pressed = true;
                break;
            }
        }

        if (!key_pressed) {
            chip->registers.pc -= 2;
        }
        break;
    case 0x15:
        chip->registers.delay_timer = *vx;
        break;
    case 0x18:
        chip->registers.sound_timer = *vx;
        break;
    case 0x1E:
        chip->registers.i += *vx;
        break;
    case 0x29:
        chip->registers.i = CHIP8_MEMORY_FONTSET_OFFSET + (*vx & 0xF) * 5;
        break;
    case 0x33:
        // hundreds
        chip->memory[chip->registers.i] = *vx / 100;
        // tens
        chip->memory[chip->registers.i + 1] = *vx / 10 % 10;
        // ones
        chip->memory[chip->registers.i + 2] = *vx % 10;
        break;
    case 0x55:
        memcpy(&chip->memory[chip->registers.i], chip->registers.vx, x_register + 1);
        assert("I should debug this when it occurs");
        break;
    case 0x65:
        memcpy(chip->registers.vx, &chip->memory[chip->registers.i], x_register + 1);
        assert("I should debug this when it occurs");
        break;
    default:
        // do nothing :D

    }
}

static const chip8_instruction_handler opcode_table[16] = {
    op_0_sys, // 0
    op_1_jump, // 1
    op_2_call, // 2
    op_3_se, // 3
    op_4_sne, // 4
    op_5_se, // 5
    op_6_ld, // 6
    op_7_add, // 7
    op_8_math, // 8 (Complex)
    op_9_sne, // 9
    op_a_ld_i, // A
    op_b_jump_v0, // B
    op_c_rnd, // C
    op_d_drw, // D
    op_e_key, // E (Complex)
    op_f_misc // F (Complex)
};

void chip8_cycle(chip8_t* chip) {
    const uint8_t opcode_high_byte = chip->memory[chip->registers.pc];
    const uint8_t opcode_low_byte = chip->memory[chip->registers.pc + 1];

    const uint16_t opcode = (uint16_t)opcode_high_byte << 8 | opcode_low_byte;

    const uint8_t opcode_index = opcode_high_byte >> 4;

    chip->registers.pc += 2;

    opcode_table[opcode_index](chip, opcode);
}

void chip8_tick_timers(chip8_t* chip) {
    assert(false);
}
