#include "ch32v003fun.h"
#include "i2c_slave.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Firmware version
#define FW_VERSION 1

// I2C registers
#define I2C_REG_FW_VERSION_0 0  // LSB
#define I2C_REG_FW_VERSION_1 1  // MSB
#define I2C_REG_RESERVED_0   2
#define I2C_REG_RESERVED_1   3
#define I2C_REG_BUTTONS      4
#define I2C_REG_LED          5

const uint32_t button_poll_interval = 100 * DELAY_MS_TIME;
uint32_t buttons_previous = 0;

uint8_t i2c_registers[255] = {0};

uint8_t curr_i2c_registers[sizeof(i2c_registers)] = {0};
uint8_t prev_i2c_registers[sizeof(i2c_registers)] = {0};

bool i2c_changed = false;
bool i2c_buttons_read = false;

uint8_t prev_buttons = 0;

void set_irq(bool active) {
    //GPIOC->BSHR |= 1 << (4 + (active ? 16 : 0)); // Pull PC4 low when active, else float high
}

void i2c_read_callback(uint8_t reg) {
    if (reg == I2C_REG_BUTTONS) {
        i2c_buttons_read = true;
    }
}

void i2c_stop_callback(uint8_t reg, uint8_t length) {
    i2c_changed = true;
}

uint8_t read_buttons() {
    // Button 1: PC7
    // Button 2: PD2
    // Button 3: PD5
    // Button 4: PD6

    uint8_t c = ~(GPIOC->INDR);
    uint8_t d = ~(GPIOD->INDR);

    return ((c >> 7) & 1) | (((d >> 2) & 1) << 1) | (((d >> 5) & 3) << 2);
}

void write_led(bool value) {
    // LED 1: PC0
    GPIOC->BSHR  |= 1 << (0 + (value ? 16 : 0));
}

int main() __attribute__((optimize("O0")));
int main() {
    SystemInit();

    // Enable GPIO ports A, C and D
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD;

    // Initialize I2C in peripheral mode on pins PC1 (SDA) and PC2 (SCL)
    SetupI2CSlave(0x42, 0, i2c_registers, sizeof(i2c_registers), i2c_stop_callback, i2c_read_callback);

    // Configure switch 1 (PC7), switch 2 (PD2), switch 3 (PD5) and switch 4 (PD6) as inputs
    GPIOC->CFGLR &= ~(0xf<<(4*7));
    GPIOC->CFGLR |= (GPIO_Speed_In | GPIO_CNF_IN_FLOATING) << (4 * 7);
    GPIOD->CFGLR &= ~(0xf<<(4*2));
    GPIOD->CFGLR |= (GPIO_Speed_In | GPIO_CNF_IN_FLOATING) << (4 * 2);
    GPIOD->CFGLR &= ~(0xf<<(4*5));
    GPIOD->CFGLR |= (GPIO_Speed_In | GPIO_CNF_IN_FLOATING) << (4 * 5);
    GPIOD->CFGLR &= ~(0xf<<(4*6));
    GPIOD->CFGLR |= (GPIO_Speed_In | GPIO_CNF_IN_FLOATING) << (4 * 6);

    // Configure LED (PC0) as output
    GPIOC->CFGLR &= ~(0xf<<(4*0));
    GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 0);
    GPIOC->BSHR  |= 1 << 0;

    while (1) {
        if (i2c_changed) {
            memcpy(curr_i2c_registers, i2c_registers, sizeof(i2c_registers));
            i2c_changed = false;

            write_led(i2c_registers[I2C_REG_LED]);
        }

        if (i2c_buttons_read) {
            set_irq(false);
            i2c_buttons_read = false;
        }

        // Write to registers
        i2c_registers[I2C_REG_FW_VERSION_0] = (FW_VERSION     ) & 0xFF;
        i2c_registers[I2C_REG_FW_VERSION_1] = (FW_VERSION >> 8) & 0xFF;

        uint32_t now = SysTick->CNT;

        if (now - buttons_previous >= button_poll_interval) {
            buttons_previous = now;
            uint8_t buttons = read_buttons();
            if (prev_buttons != buttons) {
                i2c_registers[I2C_REG_BUTTONS] = buttons;
                set_irq(true);
            }
        }

        memcpy(prev_i2c_registers, curr_i2c_registers, sizeof(i2c_registers));
    }
}
