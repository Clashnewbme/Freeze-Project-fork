#include "vga.h"
#include "serial.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((volatile uint16_t*)0xB8000)
#define SCROLLBACK_LINES 1000

volatile uint16_t* vga = VGA_MEMORY;

uint16_t screen_buffer[SCROLLBACK_LINES][VGA_WIDTH];

int row = 0;
int col = 0;
int buffer_row = 0;
int view_offset = 0;

uint8_t color = 0x02;

static inline void outb(unsigned short port, unsigned char val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static void update_cursor() {
    int visible_row = VGA_HEIGHT - 1 - view_offset;
    if (visible_row < 0) visible_row = 0;

    unsigned short pos = visible_row * VGA_WIDTH + col;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((pos >> 8) & 0xFF));
}

void render() {
    int start = buffer_row - view_offset - (VGA_HEIGHT - 1);
    if (start < 0) start = 0;

    for (int y = 0; y < VGA_HEIGHT; y++) {
        int buf_y = start + y;

        for (int x = 0; x < VGA_WIDTH; x++) {
            if (buf_y >= 0 && buf_y < SCROLLBACK_LINES)
                vga[y * VGA_WIDTH + x] = screen_buffer[buf_y][x];
            else
                vga[y * VGA_WIDTH + x] = (color << 8) | ' ';
        }
    }

    update_cursor();
}

void putc(char c) {
    if (c == '\n') {
        col = 0;
        buffer_row++;
        if (buffer_row >= SCROLLBACK_LINES)
            buffer_row = SCROLLBACK_LINES - 1;

        view_offset = 0;
        render();

        serial_putc('\r');
        serial_putc('\n');
        return;
    }

    if (c == '\r') {
        col = 0;
        render();
        return;
    }

    screen_buffer[buffer_row][col] = (color << 8) | (uint8_t)c;
    serial_putc(c);

    col++;

    if (col >= VGA_WIDTH) {
        col = 0;
        buffer_row++;
        if (buffer_row >= SCROLLBACK_LINES)
            buffer_row = SCROLLBACK_LINES - 1;
    }

    view_offset = 0;
    render();
}

void scroll_up() {
    if (view_offset < SCROLLBACK_LINES - VGA_HEIGHT)
        view_offset++;
    render();
}

void scroll_down() {
    if (view_offset > 0)
        view_offset--;
    render();
}

void handle_ansi(int code) {
    switch (code) {
        case 0:  color = 0x07; break;
        case 30: color = 0x00; break;
        case 31: color = 0x04; break;
        case 32: color = 0x02; break;
        case 33: color = 0x06; break;
        case 34: color = 0x01; break;
        case 35: color = 0x05; break;
        case 36: color = 0x03; break;
        case 37: color = 0x07; break;
        case 90: color = 0x08; break;
        case 91: color = 0x0C; break;
        case 92: color = 0x0A; break;
        case 93: color = 0x0E; break;
        case 94: color = 0x09; break;
        case 95: color = 0x0D; break;
        case 96: color = 0x0B; break;
        case 97: color = 0x0F; break;
    }
}

void print(const char* s) {
    for (int i = 0; s[i]; i++) {
        if (s[i] == '\x1b' && s[i+1] == '[') {
            serial_putc(s[i++]);
            serial_putc(s[i++]);

            int code = 0;

            while (s[i] >= '0' && s[i] <= '9') {
                code = code * 10 + (s[i] - '0');
                serial_putc(s[i]);
                i++;
            }

            if (s[i] == 'm') {
                handle_ansi(code);
                serial_putc('m');
            }
        } else {
            putc(s[i]);
        }
    }
}

void clear() {
    for (int y = 0; y < SCROLLBACK_LINES; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            screen_buffer[y][x] = (color << 8) | ' ';
        }
    }

    row = 0;
    col = 0;
    buffer_row = 0;
    view_offset = 0;

    render();

    serial_print("\x1b[2J\x1b[H");
}

void erase_last_char() {
    if (col > 0) {
        col--;
    } else if (buffer_row > 0) {
        buffer_row--;
        col = VGA_WIDTH - 1;
    } else {
        return;
    }

    screen_buffer[buffer_row][col] = (color << 8) | ' ';

    render();

    serial_putc('\b');
    serial_putc(' ');
    serial_putc('\b');
}

void print_int(int v) {
    char buf[16];
    int i = 0;

    if (v == 0) {
        putc('0');
        return;
    }

    if (v < 0) {
        putc('-');
        v = -v;
    }

    while (v > 0) {
        buf[i++] = '0' + (v % 10);
        v /= 10;
    }

    for (int j = i - 1; j >= 0; j--) {
        putc(buf[j]);
    }
}

void print_hex(unsigned int v) {
    const char* hex = "0123456789ABCDEF";
    char buf[9];

    for (int i = 0; i < 8; i++) {
        buf[7 - i] = hex[v & 0xF];
        v >>= 4;
    }

    buf[8] = 0;

    print("0x");
    print(buf);
}
