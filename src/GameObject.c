#include <stdio.h>

#include "lib/zxn/zxnext_layer2.h"
#include "lib/zxn/zxnext_sprite.h"

#include "defines.h"
#include "GameObject.h"

// Move the game object.
void GobUpdate(GameObject* gob)
{
    uint8_t orgX = gob->x;
    // if(gob->x <  255 - gob->sx)
       gob->x += gob->sx;
    //uint8_t orgY = gob->y;

    // Going down
    if(gob->sy > 0)
    {
        if(gob->y <  SCREEN_H - gob->sy)
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
        //    gob->y = (uint8_t)(SCREEN_H-1); 
    }
}

// Setup drawing of the game object.
void GobDraw(GameObject* gob)
{
     // Update sprite position.
    set_sprite_slot(gob->spriteIndex);
    set_sprite_attributes_ext(gob->spritePatternIndex, gob->x, gob->y,  
        gob->spritePaletteOffset, 0, !gob->isHidden && gob->isActive);
}