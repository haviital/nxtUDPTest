#include <stdint.h>

#define SCREEN_X 256
#define SCREEN_Y 192

typedef struct GameObject_ 
{
    uint8_t x; // X coordinate in pixels
    uint8_t y; // Y coordinate in pixels
    int8_t sx; // Horizontal speed in pixels
    int8_t sy; // Vertical speed in pixels

    uint8_t spriteIndex;  // Sprite index.
    uint8_t spritePatternIndex; // Srite bitmap data slot.
    bool isHidden;  // True is the game objec is not visible.

} GameObject;

void GobUpdate(GameObject* gob);
void GobDraw(GameObject* gob);
