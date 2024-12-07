#include "GameObject.h"

void GobUpdate(GameObject* gob)
{
    uint8_t orgX = gob->x;
    // if(gob->x <  255 - gob->sx)
       gob->x += gob->sx;
    uint8_t orgY = gob->y;
    // if(gob->y <  255 - gob->sy)
       gob->y += gob->sy;   
}

void GobDraw(GameObject* gob)
{

}