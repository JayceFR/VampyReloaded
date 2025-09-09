#ifndef GUN_H
#define GUN_H

// Each gun should have 
// cooldown -> between bullets 
// ammo 
// reload time -> between magazines 
typedef struct{
    float cooldown; 
    int maxAmmo; 
    float reloadTime; 
    Texture2D texture;

    // projectile
    float damage; 
    float speed; 
} Gun;


#endif