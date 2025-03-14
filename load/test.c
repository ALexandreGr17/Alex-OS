#include <stdint.h>
uint8_t* g_ScreenBuffer = (uint8_t*)0xB8000;

const unsigned SCREEN_WIDTH = 80;
const unsigned SCREEN_HEIGHT = 25;

void putchr(int x, int y, char c)
{
    g_ScreenBuffer[2 * (y * SCREEN_WIDTH + x)] = c;
}

int main(void) {
    putchr(0, 0, 'H');
    putchr(1, 0, 'e');
    putchr(2, 0, 'l');
    putchr(3, 0, 'l');
    putchr(4, 0, 'o');
    // for(;;);
    return 0;
}
