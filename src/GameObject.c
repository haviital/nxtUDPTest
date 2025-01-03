#include "lib/zxn/zxnext_layer2.h"
#include "lib/zxn/zxnext_sprite.h"

#include "GameObject.h"

void GobUpdate(GameObject* gob)
{
    uint8_t orgX = gob->x;
    // if(gob->x <  255 - gob->sx)
       gob->x += gob->sx;
    //uint8_t orgY = gob->y;

    // Going down
    if(gob->sy > 0)
    {
        if(gob->y <  SCREEN_Y - gob->sy)
            gob->y += gob->sy;   
    //    else
    //        gob->y = 0;
    }

    // Going up
    if(gob->sy < 0)
    {
        if(gob->y > -gob->sy )
            gob->y += gob->sy;   
        //else
        //    gob->y = (uint8_t)(SCREEN_Y-1); 
    }
}

void GobDraw(GameObject* gob)
{
    // Update sprite position.
    set_sprite_slot(gob->spriteIndex);
    set_sprite_attributes_ext(gob->spritePatternIndex, gob->x, gob->y,  
        gob->spritePaletteOffset, 0, !gob->isHidden && gob->isActive);
}