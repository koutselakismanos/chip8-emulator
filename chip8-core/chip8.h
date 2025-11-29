#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>

#define CHIP8_SCREEN_WIDTH  64
#define CHIP8_SCREEN_HEIGHT 32
#define CHIP8_TARGET_FRAME_TIME 16

#define CHIP8_MEMORY_ROM_OFFSET 0x200
#define CHIP8_MEMORY_FONTSET_OFFSET 0x50

#define CHIP8_MEMORY_SIZE 4096 // 4KB

typedef struct {
    uint8_t vx[16]; // X is a hexadecimal digit (0 through F). VF is used as a flag by some instructions
    uint16_t i; // generally used to store memory addresses, so only the lowest (rightmost) 12 bits are usually used

    uint8_t delay_timer;
    uint8_t sound_timer;

    uint16_t pc; // program counter
    uint8_t sp; // stack pointer
} chip8_registers_t;

typedef struct {
    bool is_pressed;
    char label;
} chip8_key;

typedef struct {
    uint8_t memory[CHIP8_MEMORY_SIZE];
    uint16_t stack[16]; // call stack
    chip8_registers_t registers;

    chip8_key keypad[16];
    uint8_t display[CHIP8_SCREEN_WIDTH * CHIP8_SCREEN_HEIGHT];
} chip8_t;



typedef void (*chip8_instruction_handler)(chip8_t* chip, uint16_t opcode);

// public
void chip8_init(chip8_t* chip);
void chip8_cycle(chip8_t* chip);
void chip8_tick_timers(chip8_t* chip);
void chip8_set_key(chip8_t* chip, uint8_t key_index, bool is_pressed);

/**
 * @brief Loads a raw ROM binary into the VM memory
 * @param chip pointer to the chip instance
 * @param buffer pointer to rom buffer
 * @param size buffer size
 * @return true Success
 * @return false Failed to load rom
 */
[[nodiscard]]
bool chip8_load_rom(chip8_t* chip, const uint8_t* buffer, size_t size);


#endif // CHIP8_H
