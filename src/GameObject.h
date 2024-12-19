#include <stdint.h>
#include "defines.h"

typedef struct GameObject_ 
{
    uint8_t x; // X coordinate in pixels
    uint8_t y; // Y coordinate in pixels
    int8_t sx; // Horizontal speed in pixels
    int8_t sy; // Vertical speed in pixels

    uint8_t spriteIndex;  // Sprite index.
    uint8_t spritePatternIndex; // Srite bitmap data slot.
    bool isHidden;  // True is the game objec is not visible.
    bool isActive;  // True is the game object is active. False , if it is not updated or drawn.

} GameObject;

void GobUpdate(GameObject* gob);
void GobDraw(GameObject* gob);
