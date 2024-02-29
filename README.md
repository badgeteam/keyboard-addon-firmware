# Keyboard addon firmware

## Usage

Shows up on the I2C bus at address `0x42`.

### Registers

| Register  | Description            |
|-----------|------------------------|
| 0         | Firmware version (LSB) |
| 1         | Firmware version (MSB) |
| 2         | Reserved               |
| 3         | Reserved               |
| 4         | Buttons                |
| 5         | LED                    |
| 6-255     | Reserved               |
