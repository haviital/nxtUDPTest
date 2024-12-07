#include "lib/zxn/zxnext_layer2.h"
#include "lib/zxn/zxnext_sprite.h"

#include "GameObject.h"

void GobUpdate(GameObject* gob)
{
    uint8_t orgX = gob->x;
    // if(gob->x <  255 - gob->sx)
       gob->x += gob->sx;
    uint8_t orgY = gob->y;
    if(gob->y <  SCREEN_Y - gob->sy)
       gob->y += gob->sy;   
    else
       gob->y = 0;
}

void GobDraw(GameObject* gob)
{
    // Update sprite position.
    set_sprite_slot(0);
    set_sprite_attributes_ext(gob->spriteIndex, gob->x, gob->y, 0, 0, !gob->isHidden);
}