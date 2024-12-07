#include <stdint.h>

typedef struct GameObject_ 
{
    uint8_t x; // X coordinate in pixels
    uint8_t y; // Y coordinate in pixels
    int8_t sx; // Horizontal speed in pixels
    int8_t sy; // Vertical speed in pixels
} GameObject;

void GobUpdate(GameObject* gob);
void GobDraw(GameObject* gob);
