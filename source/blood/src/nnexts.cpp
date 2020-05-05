//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT
Copyright (C) NoOne

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------


///////////////////////////////////////////////////////////////////
// This file provides modern features for mappers.
// For full documentation please visit http://cruo.bloodgame.ru/xxsystem
///////////////////////////////////////////////////////////////////


#include "nnexts.h"
#ifdef NOONE_EXTENSIONS
#include <random>
#include "aiunicult.h"
#include "triggers.h"
#include "sectorfx.h"
#include "globals.h"
#include "endgame.h"
#include "weapon.h"
#include "mmulti.h"
#include "view.h"
#include "tile.h"
#include "trig.h"
#include "sfx.h"
#include "seq.h"
#include "ai.h"


bool gAllowTrueRandom = false;
SPRITEMASS gSpriteMass[];   // cache for getSpriteMassBySize();
short gProxySpritesList[];  // list of additional sprites which can be triggered by Proximity
short gProxySpritesCount;   // current count
short gSightSpritesList[];  // list of additional sprites which can be triggered by Sight
short gSightSpritesCount;   // current count
short gPhysSpritesList[];   // list of additional sprites which can be affected by physics
short gPhysSpritesCount;    // current count
TRPLAYERCTRL gPlayerCtrl[kMaxPlayers];

std::default_random_engine gStdRandom;

VECTORINFO_EXTRA gVectorInfoExtra[] = {
    1207,1207,      1001,1001,      4001,4002,
    431,431,        1002,1002,      359,359,
    521,521,        513,513,        499,499,
    9012,9014,      1101,1101,      1207,1207,
    499,495,        495,496,        9013,499,
    1307,1308,      499,499,        499,499,
    499,499,        499,499,        351,351,
    0,0,            357,499
};

MISSILEINFO_EXTRA gMissileInfoExtra[] = {
    1207, 1207, false, false, false, false, false, true, false,
    420, 420,   false, true, true, false, false, false, false,
    471, 471,   false, false, false, false, false, false, true,
    421, 421,   false, true, false, true, false, false, false,
    1309, 351,  false, true, false, false, false, false, false,
    480, 480,   false, true, false, true, false, false, false,
    470, 470,   false, false, false, false, false, false, true,
    489, 490,   false, false, false, false, false, true, false,
    462, 351,   false, true, false, false, false, false, false,
    1203, 172,  false, false, true, false, false, false, false,
    0,0,        false, false, true, false, false, false, false,
    1457, 249,  false, false, false, false, false, true, false,
    480, 489,   false, true, false, true, false, false, false,
    480, 489,   false, false, false, true, false, false, false,
    480, 489,   false, false, false, true, false, false, false,
    491, 491,   true, true, true, true, true, true, true,
    520, 520,   false, false, false, false, false, true, false,
    520, 520,   false, false, false, false, false, true, false
};

THINGINFO_EXTRA gThingInfoExtra[] = {
    true,   true,   true,   false,  false,
    false,  false,  false,  false,  false,
    false,  false,  false,  false,  false,
    true,   false,  false,  true,   true,
    true,   true,   false,  false,  false,
    false,  false,  true,   true,   true,
    true,   true,   true,   true,   true,
    true,
};

// for actor.cpp
//-------------------------------------------------------------------------

bool nnExtIsImmune(spritetype* pSprite, int dmgType, int minScale) {

    if (dmgType >= kDmgFall && dmgType < kDmgMax && pSprite->extra >= 0 && xsprite[pSprite->extra].locked != 1) {
        if (pSprite->type >= kThingBase && pSprite->type < kThingMax)
            return (thingInfo[pSprite->type - kThingBase].dmgControl[dmgType] <= minScale);
        else if (IsDudeSprite(pSprite)) {
            if (IsPlayerSprite(pSprite)) return (gPlayer[pSprite->type - kDudePlayer1].damageControl[dmgType] <= minScale);
            else if (pSprite->type == kDudeModernCustom) return (gGenDudeExtra[pSprite->index].dmgControl[dmgType] <= minScale);
            else return (getDudeInfo(pSprite->type)->at70[dmgType] <= minScale);
        }
    }

    return true;
}

bool nnExtEraseModernStuff(spritetype* pSprite, XSPRITE* pXSprite) {
    
    bool erased = false;
    switch (pSprite->type) {
        // erase all modern types if the map is not extended
        case kModernCustomDudeSpawn:
        case kModernRandomTX:
        case kModernSequentialTX:
        case kModernSeqSpawner:
        case kModernObjPropertiesChanger:
        case kModernObjPicnumChanger:
        case kModernObjSizeChanger:
        case kModernDudeTargetChanger:
        case kModernSectorFXChanger:
        case kModernObjDataChanger:
        case kModernSpriteDamager:
        case kModernObjDataAccumulator:
        case kModernEffectSpawner:
        case kModernWindGenerator:
        case kModernPlayerControl:
            pSprite->type = kSpriteDecoration;
            erased = true;
            break;
        case kItemModernMapLevel:
        case kDudeModernCustom:
        case kDudeModernCustomBurning:
        case kModernThingTNTProx:
        case kModernThingEnemyLifeLeech:
            pSprite->type = kSpriteDecoration;
            changespritestat(pSprite->index, kStatDecoration);
            erased = true;
            break;
        // also erase some modernized vanilla types which was not active
        case kMarkerWarpDest:
            if (pSprite->statnum == kStatMarker) break;
            pSprite->type = kSpriteDecoration;
            erased = true;
            break;
    }

    if (pXSprite->Sight) {
        pXSprite->Sight = false; // it does not work in vanilla at all
        erased = true;
    }

    if (pXSprite->Proximity) {
        // proximity works only for things and dudes in vanilla
        switch (pSprite->statnum) {
            case kStatThing:
            case kStatDude:
                break;
            default:
                pXSprite->Proximity = false;
                erased = true;
                break;
        }
    }

    return erased;
}

void nnExtInitModernStuff(bool bSaveLoad) {
    
    gAllowTrueRandom = false;
    // use true random only for single player mode, otherwise use Blood's default one.
    if (gGameOptions.nGameType == 0 && !VanillaMode() && !DemoRecordStatus()) {
        
        gStdRandom.seed(std::random_device()());

        // since true random is not working if compiled with old mingw versions, we should
        // check if it works in game and if not - switch to using in-game random function.
        for (int i = kMaxRandomizeRetries; i >= 0; i--) {
            std::uniform_int_distribution<int> dist_a_b(0, 100);
            if (gAllowTrueRandom || i <= 0) break;
            else if (dist_a_b(gStdRandom) != 0)
                gAllowTrueRandom = true;
        }

    }

    if (!gAllowTrueRandom)
        initprintf("> True randomness is not available, using in-game random function(s)");

    // reset counters
    gProxySpritesCount = gSightSpritesCount = gPhysSpritesCount = 0;

    // fill arrays with negative values to avoid index 0 situation
    memset(gSightSpritesList, -1, sizeof(gSightSpritesList));
    memset(gProxySpritesList, -1, sizeof(gProxySpritesList));
    memset(gPhysSpritesList, -1, sizeof(gPhysSpritesList));

    for (int i = 0; i < kMaxXSprites; i++) {

        if (xsprite[i].reference < 0) continue;
        XSPRITE* pXSprite = &xsprite[i];  spritetype* pSprite = &sprite[pXSprite->reference];

        switch (pSprite->type) {
            case kDudeModernCustom:
            case kDudeModernCustomBurning:
                getSpriteMassBySize(pSprite); // create mass cache
                break;
        }

        // init after loading save file
        if (bSaveLoad) {

            // add in list of physics affected sprites
            if (pXSprite->physAttr != 0) {
                //xvel[pSprite->index] = yvel[pSprite->index] = zvel[pSprite->index] = 0;

                gPhysSpritesList[gPhysSpritesCount++] = pSprite->index; // add sprite index
                getSpriteMassBySize(pSprite); // create mass cache
            }

            if (pXSprite->data3 != pXSprite->sysData1) {
                switch (pSprite->statnum) {
                case kStatDude:
                    switch (pSprite->type) {
                    case kDudeModernCustom:
                    case kDudeModernCustomBurning:
                        pXSprite->data3 = pXSprite->sysData1; // move sndStartId back from sysData1 to data3 
                        break;
                    }
                    break;
                }
            }

        } else {
            
            switch (pSprite->type) {
                case kModernWindGenerator:
                    pSprite->cstat &= ~CSTAT_SPRITE_BLOCK;
                    break;
                case kModernDudeTargetChanger:
                case kModernObjDataAccumulator:
                case kModernRandom:
                case kModernRandom2:
                    pSprite->cstat &= ~CSTAT_SPRITE_BLOCK;
                    pSprite->cstat |= CSTAT_SPRITE_INVISIBLE;
                    switch (pSprite->type) {
                        // add statnum for faster dude searching
                        case kModernDudeTargetChanger:
                            changespritestat(pSprite->index, kStatModernDudeTargetChanger);
                            if (pXSprite->busyTime <= 0) pXSprite->busyTime = 5;
                            break;
                        // remove kStatItem status from random item generators
                        case kModernRandom:
                        case kModernRandom2:
                            changespritestat(pSprite->index, kStatDecoration);
                            break;
                    }
                    break;
                case kModernThingTNTProx:
                    pXSprite->Proximity = true;
                    break;
            }

            // very quick fix for floor sprites with Touch trigger flag if their Z is equals sector floorz / ceilgz
            if ((pSprite->cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR) && pSprite->sectnum >= 0 && pSprite->extra >= 0 && xsprite[pSprite->extra].Touch) {
                if (pSprite->z == sector[pSprite->sectnum].floorz) pSprite->z--;
                else if (pSprite->z == sector[pSprite->sectnum].ceilingz) pSprite->z++;
            }
        }

        // make Proximity flag work not just for dudes and things...
        if (pXSprite->Proximity && gProxySpritesCount < kMaxSuperXSprites) {
            switch (pSprite->statnum) {
            // exceptions
            case kStatThing: // things already treated in their functions
            case kStatDude: // enemies already treated in their functions
                // senseless to have sight and proximity together
                if (pXSprite->Sight && pXSprite->DudeLockout) pXSprite->Proximity = false;
                break;
            case kStatFX:           case kStatExplosion:            case kStatItem:
            case kStatPurge:        case kStatSpares:               case kStatFlare:
            case kStatInactive:     case kStatFree:                 case kStatMarker:
            case kStatPathMarker:
                break;
            default:
                // senseless to have sight and proximity together
                if (pXSprite->Sight && pXSprite->DudeLockout) pXSprite->Proximity = false;
                else {
                    gProxySpritesList[gProxySpritesCount++] = pSprite->index;
                    if (gProxySpritesCount == kMaxSuperXSprites)
                        ThrowError("Max (%d) *additional* Proximity sprites reached!", kMaxSuperXSprites);
                }
                break;
            }
        }

        // make Sight flag work not just for dudes and things...
        if (pXSprite->Sight && gSightSpritesCount < kMaxSuperXSprites) {
            switch (pSprite->statnum) {
            // exceptions
            case kStatFX:           case kStatExplosion:            case kStatItem:
            case kStatPurge:        case kStatSpares:               case kStatFlare:
            case kStatInactive:     case kStatFree:                 case kStatMarker:
            case kStatPathMarker:
                break;
            default:
                gSightSpritesList[gSightSpritesCount++] = pSprite->index;
                if (gSightSpritesCount == kMaxSuperXSprites)
                    ThrowError("Max (%d) Sight sprites reached!", kMaxSuperXSprites);
                break;
            }
        }
    }
}


// The following functions required for random event features
//-------------------------
int nnExtRandom(int a, int b) {
    if (gAllowTrueRandom) return Random(b - a) + a;
    
    // used for better randomness in single player
    std::uniform_int_distribution<int> dist_a_b(a, b);
    return dist_a_b(gStdRandom);
}

int GetDataVal(spritetype* pSprite, int data) {
    if (pSprite->extra >= 0) {
        switch (data) {
            case 0: return xsprite[pSprite->extra].data1;
            case 1: return xsprite[pSprite->extra].data2;
            case 2: return xsprite[pSprite->extra].data3;
            case 3: return xsprite[pSprite->extra].data4;
        }
    }

    return -1;
}

// tries to get random data field of sprite
int randomGetDataValue(XSPRITE* pXSprite, int randType) {
    if (pXSprite == NULL) return -1;
    int random = 0; int bad = 0; int maxRetries = kMaxRandomizeRetries;

    int rData[4];
    rData[0] = pXSprite->data1; rData[2] = pXSprite->data3;
    rData[1] = pXSprite->data2; rData[3] = pXSprite->data4;
    // randomize only in case if at least 2 data fields fits.
    for (int i = 0; i < 4; i++) {
        switch (randType) {
        case kRandomizeItem:
            if (rData[i] >= kItemWeaponBase && rData[i] < kItemMax) break;
            else bad++;
            break;
        case kRandomizeDude:
            if (rData[i] >= kDudeBase && rData[i] < kDudeMax) break;
            else bad++;
            break;
        case kRandomizeTX:
            if (rData[i] > kChannelZero && rData[i] < kChannelUserMax) break;
            else bad++;
            break;
        default:
            bad++;
            break;
        }
    }

    if (bad < 3) {
        // try randomize few times
        while (maxRetries > 0) {
            random = nnExtRandom(0, 3);
            if (rData[random] > 0) return rData[random];
            else maxRetries--;
        }
    }

    return -1;
}

// this function drops random item using random pickup generator(s)
spritetype* randomDropPickupObject(spritetype* pSource, short prevItem) {
    spritetype* pSprite2 = NULL; int selected = -1; int maxRetries = 9;
    if (xspriRangeIsFine(pSource->extra)) {
        XSPRITE* pXSource = &xsprite[pSource->extra];
        while ((selected = randomGetDataValue(pXSource, kRandomizeItem)) == prevItem) if (maxRetries-- <= 0) break;
        if (selected > 0) {
            pSprite2 = actDropObject(pSource, selected);
            if (pSprite2 != NULL) {

                pXSource->dropMsg = pSprite2->type; // store dropped item type in dropMsg
                pSprite2->x = pSource->x;
                pSprite2->y = pSource->y;
                pSprite2->z = pSource->z;

                if ((pSource->flags & kModernTypeFlag1) && (pXSource->txID > 0 || (pXSource->txID != 3 && pXSource->lockMsg > 0)) &&
                    dbInsertXSprite(pSprite2->index) > 0) {

                    XSPRITE* pXSprite2 = &xsprite[pSprite2->extra];

                    // inherit spawn sprite trigger settings, so designer can send command when item picked up.
                    pXSprite2->txID = pXSource->txID;
                    pXSprite2->command = pXSource->command;
                    pXSprite2->triggerOn = pXSource->triggerOn;
                    pXSprite2->triggerOff = pXSource->triggerOff;

                    pXSprite2->Pickup = true;

                }
            }
        }
    }
    return pSprite2;
}

// this function spawns random dude using dudeSpawn
spritetype* randomSpawnDude(spritetype* pSource) {
    spritetype* pSprite2 = NULL; int selected = -1;
    if (xspriRangeIsFine(pSource->extra)) {
        XSPRITE* pXSource = &xsprite[pSource->extra];
        if ((selected = randomGetDataValue(pXSource, kRandomizeDude)) > 0)
            pSprite2 = actSpawnDude(pSource, selected, -1, 0);
    }
    return pSprite2;
}
//-------------------------

void nnExtProcessSuperSprites() {
        
    // process additional proximity sprites
    if (gProxySpritesCount > 0) {
        for (int i = 0; i < gProxySpritesCount; i++) {
            if (sprite[gProxySpritesList[i]].extra < 0) continue;

            XSPRITE* pXProxSpr = &xsprite[sprite[gProxySpritesList[i]].extra];
            if (!pXProxSpr->Proximity || (!pXProxSpr->Interrutable && pXProxSpr->state != pXProxSpr->restState) || pXProxSpr->locked == 1
                || pXProxSpr->isTriggered) continue;  // don't process locked or triggered sprites

            int x = sprite[gProxySpritesList[i]].x;	int y = sprite[gProxySpritesList[i]].y;
            int z = sprite[gProxySpritesList[i]].z;	int index = sprite[gProxySpritesList[i]].index;
            int sectnum = sprite[gProxySpritesList[i]].sectnum;

            if (!pXProxSpr->DudeLockout) {

                for (int nAffected = headspritestat[kStatDude]; nAffected >= 0; nAffected = nextspritestat[nAffected]) {

                    if ((sprite[nAffected].flags & 32) || xsprite[sprite[nAffected].extra].health <= 0) continue;
                    else if (CheckProximity(&sprite[nAffected], x, y, z, sectnum, 96)) {
                        trTriggerSprite(index, pXProxSpr, kCmdSpriteProximity);
                        break;
                    }
                }

            } else {

                for (int a = connecthead; a >= 0; a = connectpoint2[a]) {
                    if (gPlayer[a].pXSprite->health > 0 && CheckProximity(gPlayer[a].pSprite, x, y, z, sectnum, 96)) {
                        trTriggerSprite(index, pXProxSpr, kCmdSpriteProximity);
                        break;
                    }
                }

            }
        }
    }

    // process sight sprites (for players only)
    if (gSightSpritesCount > 0) {
        for (int i = 0; i < gSightSpritesCount; i++) {
            if (sprite[gSightSpritesList[i]].extra < 0) continue;

            XSPRITE* pXSightSpr = &xsprite[sprite[gSightSpritesList[i]].extra];
            if (!pXSightSpr->Sight || (!pXSightSpr->Interrutable && pXSightSpr->state != pXSightSpr->restState) || pXSightSpr->locked == 1 ||
                pXSightSpr->isTriggered) continue; // don't process locked or triggered sprites

            int x = sprite[gSightSpritesList[i]].x;	int y = sprite[gSightSpritesList[i]].y;
            int z = sprite[gSightSpritesList[i]].z;	int index = sprite[gSightSpritesList[i]].index;
            int sectnum = sprite[gSightSpritesList[i]].sectnum;

            for (int a = connecthead; a >= 0; a = connectpoint2[a]) {
                spritetype* pPlaySprite = gPlayer[a].pSprite;
                if (gPlayer[a].pXSprite->health > 0 && cansee(x, y, z, sectnum, pPlaySprite->x, pPlaySprite->y, pPlaySprite->z, pPlaySprite->sectnum)) {
                    trTriggerSprite(index, pXSightSpr, kCmdSpriteSight);
                    break;
                }
            }
        }
    }

    // process Debris sprites for movement
    if (gPhysSpritesCount > 0) {
        //viewSetSystemMessage("PHYS COUNT: %d", gPhysSpritesCount);
        for (int i = 0; i < gPhysSpritesCount; i++) {
            if (gPhysSpritesList[i] == -1) continue;
            else if (sprite[gPhysSpritesList[i]].statnum == kStatFree || (sprite[gPhysSpritesList[i]].flags & kHitagFree) != 0) {
                gPhysSpritesList[i] = -1;
                continue;
            }

            XSPRITE* pXDebris = &xsprite[sprite[gPhysSpritesList[i]].extra];
            if (!(pXDebris->physAttr & kPhysMove) && !(pXDebris->physAttr & kPhysGravity)) {
                gPhysSpritesList[i] = -1;
                continue;
            }

            spritetype* pDebris = &sprite[gPhysSpritesList[i]];
            XSECTOR* pXSector = (sector[pDebris->sectnum].extra >= 0) ? &xsector[sector[pDebris->sectnum].extra] : NULL;
            viewBackupSpriteLoc(pDebris->index, pDebris);
            int airVel = gSpriteMass[pDebris->extra].airVel;
            if (pXSector != NULL) {
                if (pXSector->Underwater) airVel <<= 6;
                if (pXSector->panVel != 0) {
                    int top, bottom;
                    GetSpriteExtents(pDebris, &top, &bottom);

                    if (getflorzofslope(pDebris->sectnum, pDebris->x, pDebris->y) <= bottom)
                    {
                        int angle = pXSector->panAngle;
                        int speed = 0;
                        if (pXSector->panAlways || pXSector->state || pXSector->busy)
                        {
                            speed = pXSector->panVel << 9;
                            if (!pXSector->panAlways && pXSector->busy)
                                speed = mulscale16(speed, pXSector->busy);
                        }
                        if (sector[pDebris->sectnum].floorstat & 64)
                            angle = (angle + GetWallAngle(sector[pDebris->sectnum].wallptr) + 512) & 2047;
                        int dx = mulscale30(speed, Cos(angle));
                        int dy = mulscale30(speed, Sin(angle));
                        xvel[pDebris->index] += dx;
                        yvel[pDebris->index] += dy;
                    }
                }
            }

            actAirDrag(pDebris, airVel);

            if (((pDebris->index >> 8) & 15) == (gFrame & 15) && (pXDebris->physAttr & kPhysGravity))
                pXDebris->physAttr |= kPhysFalling;

            if ((pXDebris->physAttr & 4) == 0 && xvel[pDebris->index] == 0 && yvel[pDebris->index] == 0 &&
                zvel[pDebris->index] == 0 && velFloor[pDebris->sectnum] == 0 && velCeil[pDebris->sectnum] == 0)
                continue;

            debrisMove(i);

        }
    }

}

// this function plays sound predefined in missile info
void sfxPlayMissileSound(spritetype* pSprite, int missileId) {
    MISSILEINFO_EXTRA* pMissType = &gMissileInfoExtra[missileId - kMissileBase];
    sfxPlay3DSound(pSprite, Chance(0x5000) ? pMissType->fireSound[0] : pMissType->fireSound[1], -1, 0);
}

// this function plays sound predefined in vector info
void sfxPlayVectorSound(spritetype* pSprite, int vectorId) {
    VECTORINFO_EXTRA* pVectorData = &gVectorInfoExtra[vectorId];
    sfxPlay3DSound(pSprite, Chance(0x5000) ? pVectorData->fireSound[0] : pVectorData->fireSound[1], -1, 0);
}

int getSpriteMassBySize(spritetype* pSprite) {
    int mass = 0; int seqId = -1; int clipDist = pSprite->clipdist; Seq* pSeq = NULL;
    if (IsDudeSprite(pSprite)) {

        switch (pSprite->type) {
        case kDudePodMother: // fake dude, no seq
            break;
        case kDudeModernCustom:
        case kDudeModernCustomBurning:
            seqId = xsprite[pSprite->extra].data2;
            clipDist = gGenDudeExtra[pSprite->index].initVals[2];
            break;
        default:
            seqId = getDudeInfo(pSprite->type)->seqStartID;
            break;
        }

    } else if (pSprite->extra >= 0) {

        seqId = seqGetID(3, pSprite->extra);

    }

    SPRITEMASS* cached = &gSpriteMass[pSprite->extra];
    if (((seqId >= 0 && seqId == cached->seqId) || pSprite->picnum == cached->picnum) && pSprite->xrepeat == cached->xrepeat &&
        pSprite->yrepeat == cached->yrepeat && clipDist == cached->clipdist) {
        return cached->mass;
    }

    short picnum = pSprite->picnum;
    short massDiv = 30;  short addMul = 2; short subMul = 2;

    if (seqId >= 0) {
        DICTNODE* hSeq = gSysRes.Lookup(seqId, "SEQ");
        if (hSeq)
        {
            pSeq = (Seq*)gSysRes.Load(hSeq);
            picnum = seqGetTile(&pSeq->frames[0]);
        } else
            picnum = pSprite->picnum;
    }

    clipDist = ClipLow(pSprite->clipdist, 1);
    short x = tilesiz[picnum].x;        short y = tilesiz[picnum].y;
    short xrepeat = pSprite->xrepeat; 	short yrepeat = pSprite->yrepeat;

    // take surface type into account
    switch (tileGetSurfType(pSprite->index + 0xc000)) {
        case 1:  massDiv = 16; break; // stone
        case 2:  massDiv = 18; break; // metal
        case 3:  massDiv = 21; break; // wood
        case 4:  massDiv = 25; break; // flesh
        case 5:  massDiv = 28; break; // water
        case 6:  massDiv = 26; break; // dirt
        case 7:  massDiv = 27; break; // clay
        case 8:  massDiv = 35; break; // snow
        case 9:  massDiv = 22; break; // ice
        case 10: massDiv = 37; break; // leaves
        case 11: massDiv = 33; break; // cloth
        case 12: massDiv = 36; break; // plant
        case 13: massDiv = 24; break; // goo
        case 14: massDiv = 23; break; // lava
    }

    mass = ((x + y) * (clipDist / 2)) / massDiv;

    if (xrepeat > 64) mass += ((xrepeat - 64) * addMul);
    else if (xrepeat < 64 && mass > 0) {
        for (int i = 64 - xrepeat; i > 0; i--) {
            if ((mass -= subMul) <= 100 && subMul-- <= 1) {
                mass -= i;
                break;
            }
        }
    }

    if (yrepeat > 64) mass += ((yrepeat - 64) * addMul);
    else if (yrepeat < 64 && mass > 0) {
        for (int i = 64 - yrepeat; i > 0; i--) {
            if ((mass -= subMul) <= 100 && subMul-- <= 1) {
                mass -= i;
                break;
            }
        }
    }

    if (mass <= 0) cached->mass = 1 + Random(10);
    else cached->mass = ClipRange(mass, 1, 65535);

    cached->airVel = ClipRange(400 - cached->mass, 32, 400);
    cached->fraction = ClipRange(60000 - (cached->mass << 7), 8192, 60000);

    cached->xrepeat = pSprite->xrepeat;             cached->yrepeat = pSprite->yrepeat;
    cached->picnum = pSprite->picnum;               cached->seqId = seqId;
    cached->clipdist = pSprite->clipdist;

    return cached->mass;
}

int debrisGetIndex(int nSprite) {
    if (sprite[nSprite].extra < 0 || xsprite[sprite[nSprite].extra].physAttr == 0)
        return -1;

    for (int i = 0; i < gPhysSpritesCount; i++) {
        if (gPhysSpritesList[i] != nSprite) continue;
        return i;
    }

    return -1;
}

int debrisGetFreeIndex(void) {
    for (int i = 0; i < kMaxSuperXSprites; i++) {
        if (gPhysSpritesList[i] == -1 || sprite[gPhysSpritesList[i]].statnum == kStatFree) return i;

        else if ((sprite[gPhysSpritesList[i]].flags & kHitagFree) || sprite[gPhysSpritesList[i]].extra < 0) return i;
        else if (xsprite[sprite[gPhysSpritesList[i]].extra].physAttr == 0) return i;
    }

    return -1;
}

void debrisConcuss(int nOwner, int listIndex, int x, int y, int z, int dmg) {
    spritetype* pSprite = (gPhysSpritesList[listIndex] >= 0) ? &sprite[gPhysSpritesList[listIndex]] : NULL;
    if (pSprite != NULL && pSprite->extra >= 0 && pSprite->extra < kMaxXSprites) {
        int dx = pSprite->x - x; int dy = pSprite->y - y; int dz = (pSprite->z - z) >> 4;
        dmg = scale(0x40000, dmg, 0x40000 + dx * dx + dy * dy + dz * dz);

        int size = (tilesiz[pSprite->picnum].x * pSprite->xrepeat * tilesiz[pSprite->picnum].y * pSprite->yrepeat) >> 1;
        if (xsprite[pSprite->extra].physAttr & kPhysDebrisExplode) {
            if (gSpriteMass[pSprite->extra].mass > 0) {
                int t = scale(dmg, size, gSpriteMass[pSprite->extra].mass);

                xvel[pSprite->index] += mulscale16(t, dx);
                yvel[pSprite->index] += mulscale16(t, dy);
                zvel[pSprite->index] += mulscale16(t, dz);
            }


            if (pSprite->type >= kThingBase && pSprite->type < kThingMax)
                //actPostSprite(pSprite->index, kStatThing); // !!! (does not working here) if it was a thing, return it's statnum back
                changespritestat(pSprite->index, kStatThing);
        }


        actDamageSprite(nOwner, pSprite, DAMAGE_TYPE_3, dmg);
        return;
    }
}

void debrisMove(int listIndex) {
    if (!(sprite[gPhysSpritesList[listIndex]].extra > 0 && sprite[gPhysSpritesList[listIndex]].extra < kMaxXSprites)) {
        gPhysSpritesList[listIndex] = -1;
        return;
    } else if (!(sprite[gPhysSpritesList[listIndex]].sectnum >= 0 && sprite[gPhysSpritesList[listIndex]].sectnum < kMaxSectors)) {
        gPhysSpritesList[listIndex] = -1;
        return;
    }

    int nSprite = gPhysSpritesList[listIndex];
    int nXSprite = sprite[nSprite].extra;       XSPRITE* pXDebris = &xsprite[nXSprite];
    spritetype* pSprite = &sprite[nSprite];     int nSector = pSprite->sectnum;

    int top, bottom;	GetSpriteExtents(pSprite, &top, &bottom);

    int moveHit = 0;
    //int floorDist = (bottom - pSprite->z) / 4;
    //int ceilDist = (pSprite->z - top) / 4;
    //int clipDist = pSprite->clipdist << 2;

    int tmpFraction = gSpriteMass[pSprite->extra].fraction;
    if (sector[nSector].extra >= 0 && xsector[sector[nSector].extra].Underwater)
        tmpFraction >>= 1;

    if (xvel[pSprite->index] != 0 || yvel[pSprite->index] != 0) {

        short oldcstat = pSprite->cstat;
        pSprite->cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);

        moveHit = gSpriteHit[nXSprite].hit = ClipMove((int*)&pSprite->x, (int*)&pSprite->y, (int*)&pSprite->z, &nSector, xvel[nSprite] >> 12,
            yvel[nSprite] >> 12, pSprite->clipdist << 2, (pSprite->z - top) / 4, (bottom - pSprite->z) / 4, CLIPMASK0);

        pSprite->cstat = oldcstat;

        dassert(nSector >= 0);

        if (pSprite->sectnum != nSector) {
            dassert(nSector >= 0 && nSector < kMaxSectors);
            ChangeSpriteSect(nSprite, nSector);
        }

        if ((gSpriteHit[nXSprite].hit & 0xc000) == 0x8000) {
            int nHitWall = gSpriteHit[nXSprite].hit & 0x3fff;
            actWallBounceVector((int*)&xvel[nSprite], (int*)&yvel[nSprite], nHitWall, tmpFraction);
        }

    } else {
        dassert(nSector >= 0 && nSector < kMaxSectors);
        FindSector(pSprite->x, pSprite->y, pSprite->z, &nSector);
    }

    if (zvel[nSprite])
        pSprite->z += zvel[nSprite] >> 8;

    int ceilZ, ceilHit, floorZ, floorHit;
    GetZRange(pSprite, &ceilZ, &ceilHit, &floorZ, &floorHit, pSprite->clipdist << 2, CLIPMASK0);
    GetSpriteExtents(pSprite, &top, &bottom);

    if ((pXDebris->physAttr & kPhysGravity) && bottom < floorZ) {
        pSprite->z += 455;
        zvel[nSprite] += 58254;
    }
    int warp = CheckLink(pSprite);
    if (warp != 0) {
        GetZRange(pSprite, &ceilZ, &ceilHit, &floorZ, &floorHit, pSprite->clipdist << 2, CLIPMASK0);
        if (!(pSprite->cstat & CSTAT_SPRITE_INVISIBLE)) {
            switch (warp) {
            case kMarkerUpWater:
            case kMarkerUpGoo:
                int pitch = (150000 - (gSpriteMass[pSprite->extra].mass << 9)) + Random3(8192);
                sfxPlay3DSoundCP(pSprite, 720, -1, 0, pitch, 75 - Random(40));

                if (sector[pSprite->sectnum].extra < 0 || !xsector[sector[pSprite->sectnum].extra].Underwater)
                    evKill(pSprite->index, 3, kCallbackEnemeyBubble);
                else {
                    if (Chance(0x8000))
                        evPost(pSprite->index, 3, 0, kCallbackEnemeyBubble);

                    for (int i = 2; i <= 5; i++) {
                        if (Chance(0x3000 * i))
                            evPost(pSprite->index, 3, 0, kCallbackEnemeyBubble);
                    }
                }
                break;
            }
        }
    }

    GetSpriteExtents(pSprite, &top, &bottom);

    if ((floorHit & 0xe000) == 0xc000) {
        if ((sprite[floorHit & 0x1fff].cstat & 0x30) == 0x20)
            if (klabs(bottom - floorZ) < 1024) floorZ -= 1024;
    }

    if (bottom >= floorZ) {

        gSpriteHit[nXSprite].florhit = floorHit;
        pSprite->z += floorZ - bottom;
        int v20 = zvel[nSprite] - velFloor[pSprite->sectnum];
        if (v20 > 0) {

            pXDebris->physAttr |= kPhysFalling;
            actFloorBounceVector((int*)&xvel[nSprite], (int*)&yvel[nSprite], (int*)&v20, pSprite->sectnum, tmpFraction);
            zvel[nSprite] = v20;

            if (velFloor[pSprite->sectnum] == 0 && klabs(zvel[nSprite]) < 0x10000) {
                zvel[nSprite] = 0;
                pXDebris->physAttr &= ~kPhysFalling;
            }

            moveHit = 0x4000 | nSector;

        } else if (zvel[nSprite] == 0)
            pXDebris->physAttr &= ~kPhysFalling;

    } else {

        gSpriteHit[nXSprite].florhit = 0;
        if (pXDebris->physAttr & kPhysGravity)
            pXDebris->physAttr |= kPhysFalling;
    }

    if (top <= ceilZ) {

        gSpriteHit[nXSprite].ceilhit = ceilHit;
        pSprite->z += ClipLow(ceilZ - top, 0);
        if (zvel[nSprite] < 0)
        {
            xvel[nSprite] = mulscale16(xvel[nSprite], 0xc000);
            yvel[nSprite] = mulscale16(yvel[nSprite], 0xc000);
            zvel[nSprite] = mulscale16(-zvel[nSprite], 0x4000);
        }

    } else {

        gSpriteHit[nXSprite].ceilhit = 0;

    }

    if (bottom >= floorZ) {
        int nVel = approxDist(xvel[nSprite], yvel[nSprite]);
        int nVelClipped = ClipHigh(nVel, 0x11111);

        if ((floorHit & 0xc000) == 0xc000) {
            int nHitSprite = floorHit & 0x3fff;
            if ((sprite[nHitSprite].cstat & 0x30) == 0)
            {
                xvel[nSprite] += mulscale(4, pSprite->x - sprite[nHitSprite].x, 2);
                yvel[nSprite] += mulscale(4, pSprite->y - sprite[nHitSprite].y, 2);
                moveHit = gSpriteHit[nXSprite].hit;
            }
        }
        if (nVel > 0)
        {
            int t = divscale16(nVelClipped, nVel);
            xvel[nSprite] -= mulscale16(t, xvel[nSprite]);
            yvel[nSprite] -= mulscale16(t, yvel[nSprite]);
        }
    }

    if (xvel[nSprite] || yvel[nSprite])
        pSprite->ang = getangle(xvel[nSprite], yvel[nSprite]);

    if (moveHit != 0 && pXDebris->Impact && pXDebris->locked != 1 && !pXDebris->isTriggered) {
        if (!pXDebris->Interrutable && pXDebris->state != pXDebris->restState) return;

        if (pSprite->type >= kThingBase && pSprite->type < kThingMax)
            // if thing was turned in debris, change it's stat back so it will do on impact what it supposed to do...
            //actPostSprite(nSprite, kStatThing); // !!!! not working here for some reason
            changespritestat(nSprite, kStatThing);


        if (pXDebris->state == 1) trTriggerSprite(pSprite->index, pXDebris, kCmdOff);
        else trTriggerSprite(pSprite->index, pXDebris, kCmdOn);
    }
}

bool ceilIsTooLow(spritetype* pSprite) {
    if (pSprite != NULL) {

        sectortype* pSector = &sector[pSprite->sectnum];
        int a = pSector->ceilingz - pSector->floorz;
        int top, bottom;
        GetSpriteExtents(pSprite, &top, &bottom);
        int b = top - bottom;
        if (a > b) return true;
    }

    return false;
}

void aiSetGenIdleState(spritetype* pSprite, XSPRITE* pXSprite) {
    switch (pSprite->type) {
    case kDudeModernCustom:
    case kDudeModernCustomBurning:
        aiGenDudeNewState(pSprite, &genIdle);
        break;
    default:
        aiNewState(pSprite, pXSprite, &genIdle);
        break;
    }
}

// this function stops wind on all TX sectors affected by WindGen after it goes off state.
void windGenStopWindOnSectors(XSPRITE* pXSource) {
    spritetype* pSource = &sprite[pXSource->reference];

    if (pXSource->txID <= 0) {

        if (sector[pSource->sectnum].extra >= 0)
            xsector[sector[pSource->sectnum].extra].windVel = 0;

        return;
    }

    for (int i = bucketHead[pXSource->txID]; i < bucketHead[pXSource->txID + 1]; i++) {
        if (rxBucket[i].type != OBJ_SECTOR) continue;
        XSECTOR* pXSector = &xsector[sector[rxBucket[i].index].extra];
        if ((pXSector->state == 1 && !pXSector->windAlways) || (sprite[pXSource->reference].flags & kModernTypeFlag1))
            pXSector->windVel = 0;
    }
}

void trPlayerCtrlStartScene(XSPRITE* pXSource, PLAYER* pPlayer) {

    int nSource = sprite[pXSource->reference].index; TRPLAYERCTRL* pCtrl = &gPlayerCtrl[pPlayer->nPlayer];
    QAV* pQav = playerQavSceneLoad(pXSource->data2);
    if (pQav != NULL) {

        // save current weapon
        pXSource->dropMsg = pPlayer->curWeapon;

        short nIndex = pCtrl->qavScene.index;
        if (nIndex > -1 && nIndex != nSource && sprite[nIndex].extra >= 0)
            pXSource->dropMsg = xsprite[sprite[nIndex].extra].dropMsg;

        if (nIndex < 0)
            WeaponLower(pPlayer);

        pXSource->sysData1 = ClipLow((pQav->at10 * pXSource->waitTime) / 4, 0); // how many times animation should be played

        pCtrl->qavScene.index = nSource;
        pCtrl->qavScene.qavResrc = pQav;
        pCtrl->qavScene.dummy = -1;

        pCtrl->qavScene.qavResrc->Preload();

        pPlayer->sceneQav = pXSource->data2;
        pPlayer->weaponTimer = pCtrl->qavScene.qavResrc->at10;
        pPlayer->qavCallback = (pXSource->data3 > 0) ? ClipRange(pXSource->data3 - 1, 0, 32) : -1;
        pPlayer->qavLoop = false;

    }

}

void trPlayerCtrlStopScene(XSPRITE* pXSource, PLAYER* pPlayer) {

    TRPLAYERCTRL* pCtrl = &gPlayerCtrl[pPlayer->nPlayer];
    //viewSetSystemMessage("OFF %d", pCtrl->qavScene.index);

    pXSource->sysData1 = 0;
    pCtrl->qavScene.index = -1;
    pCtrl->qavScene.qavResrc = NULL;
    pPlayer->sceneQav = -1;

    // restore weapon
    if (pPlayer->pXSprite->health > 0) {
        int oldWeapon = (pXSource->dropMsg != 0) ? pXSource->dropMsg : 1;
        pPlayer->input.newWeapon = pPlayer->curWeapon = oldWeapon;
        WeaponRaise(pPlayer);
    }

}

void trPlayerCtrlLink(XSPRITE* pXSource, PLAYER* pPlayer) {

    pPlayer->pXSprite->txID = pXSource->txID;
    pPlayer->pXSprite->command = pXSource->data2;
    pPlayer->pXSprite->triggerOn = pXSource->triggerOn;
    pPlayer->pXSprite->triggerOff = pXSource->triggerOff;
    pPlayer->pXSprite->busyTime = pXSource->busyTime;
    pPlayer->pXSprite->waitTime = pXSource->waitTime;
    pPlayer->pXSprite->restState = pXSource->restState;

    pPlayer->pXSprite->Push = pXSource->Push;
    pPlayer->pXSprite->Impact = pXSource->Impact;
    pPlayer->pXSprite->Vector = pXSource->Vector;
    pPlayer->pXSprite->Touch = pXSource->Touch;
    pPlayer->pXSprite->Sight = pXSource->Sight;
    pPlayer->pXSprite->Proximity = pXSource->Proximity;

    pPlayer->pXSprite->Decoupled = pXSource->Decoupled;
    pPlayer->pXSprite->Interrutable = pXSource->Interrutable;
    pPlayer->pXSprite->DudeLockout = pXSource->DudeLockout;

    //pPlayer->pXSprite->data1            = pXSource->data1;
    //pPlayer->pXSprite->data2            = pXSource->data2;
    //pPlayer->pXSprite->data3            = pXSource->data3;
    //pPlayer->pXSprite->data4            = pXSource->data4;

    pPlayer->pXSprite->key = pXSource->key;
    pPlayer->pXSprite->dropMsg = pXSource->dropMsg;

}

void trPlayerCtrlSetRace(XSPRITE* pXSource, PLAYER* pPlayer) {
    playerSetRace(pPlayer, pXSource->data2);
    switch (pPlayer->lifeMode) {
        case kModeHuman:
        case kModeBeast:
            playerSizeReset(pPlayer);
            break;
        case kModeHumanShrink:
            playerSizeShrink(pPlayer, 2);
            break;
        case kModeHumanGrown:
            playerSizeGrow(pPlayer, 2);
            break;
    }
}

void trPlayerCtrlSetMoveSpeed(XSPRITE* pXSource, PLAYER* pPlayer) {
    
    int speed = pXSource->data2 << 1;
    for (int i = 0; i < kModeMax; i++) {
        for (int a = 0; a < kPostureMax; a++) {
            POSTURE* curPosture = &pPlayer->pPosture[i][a]; POSTURE defPosture = gPostureDefaults[i][a];
            if (pXSource->data2 == 100) {
                curPosture->frontAccel = defPosture.frontAccel;
                curPosture->sideAccel = defPosture.sideAccel;
                curPosture->backAccel = defPosture.backAccel;
            } else if (speed >= 0) {
                curPosture->frontAccel = ClipRange(mulscale8(defPosture.frontAccel, speed), 0, 65535);
                curPosture->sideAccel = ClipRange(mulscale8(defPosture.sideAccel, speed), 0, 65535);
                curPosture->backAccel = ClipRange(mulscale8(defPosture.backAccel, speed), 0, 65535);
            }
        }
    }
    //viewSetSystemMessage("MOVEMENT: %d %d %d", pXSprite->rxID,pSprite->index, gPosture[0][0].frontAccel);

}

void trPlayerCtrlSetJumpHeight(XSPRITE* pXSource, PLAYER* pPlayer) {
    
    int jump = pXSource->data3 * 3;
    for (int i = 0; i < kModeMax; i++) {
        POSTURE* curPosture = &pPlayer->pPosture[i][kPostureStand]; POSTURE defPosture = gPostureDefaults[i][kPostureStand];
        if (pXSource->data3 == 100) {
            curPosture->normalJumpZ = defPosture.normalJumpZ;
            curPosture->pwupJumpZ = defPosture.pwupJumpZ;
        } else if (jump >= 0) {
            curPosture->normalJumpZ = ClipRange(mulscale8(defPosture.normalJumpZ, jump), -0x200000, 0);
            curPosture->pwupJumpZ = ClipRange(mulscale8(defPosture.pwupJumpZ, jump), -0x200000, 0);
        }
    }

    //viewSetSystemMessage("JUMPING: %d", gPosture[0][0].normalJumpZ);
}

void trPlayerCtrlSetScreenEffect(XSPRITE* pXSource, PLAYER* pPlayer) {
    
    switch (pXSource->data2) {
        case 1: // tilting
            pPlayer->tiltEffect = ClipRange(pXSource->data3, 0, 220);
            break;
        case 2: // pain
            pPlayer->painEffect = pXSource->data3;
            break;
        case 3: // blind
            pPlayer->blindEffect = pXSource->data3;
            break;
        case 4: // pickup
            pPlayer->pickupEffect = pXSource->data3;
            break;
        case 5: // quakeEffect
            pPlayer->quakeEffect = pXSource->data3;
            break;
        case 6: // visibility
            pPlayer->visibility = pXSource->data3;
            break;
        case 7: // delirium
            pPlayer->pwUpTime[kPwUpDeliriumShroom] = ClipHigh(pXSource->data3 << 1, 432000);
            break;
    }

}

void trPlayerCtrlSetLookAngle(XSPRITE* pXSource, PLAYER* pPlayer) {
    
    CONSTEXPR int upAngle = 289; CONSTEXPR int downAngle = -347;
    CONSTEXPR double lookStepUp = 4.0 * upAngle / 60.0;
    CONSTEXPR double lookStepDown = -4.0 * downAngle / 60.0;

    int look = pXSource->data2 << 5;
    if (look > 0) pPlayer->q16look = fix16_min(mulscale8(F16(lookStepUp), look), F16(upAngle));
    else if (look < 0) pPlayer->q16look = -fix16_max(mulscale8(F16(lookStepDown), abs(look)), F16(downAngle));
    else pPlayer->q16look = 0;

}

void trPlayerCtrlEraseStuff(XSPRITE* pXSource, PLAYER* pPlayer) {
    
    switch (pXSource->data2) {
        case 0: // erase all
            fallthrough__;
        case 1: // erase weapons
            WeaponLower(pPlayer);

            for (int i = 0; i < 14; i++) {
                pPlayer->hasWeapon[i] = false;
                // also erase ammo
                if (i < 12) pPlayer->ammoCount[i] = 0;
            }

            pPlayer->hasWeapon[1] = true;
            pPlayer->curWeapon = 0;
            pPlayer->nextWeapon = 1;

            WeaponRaise(pPlayer);
            if (pXSource->data2) break;
            fallthrough__;
        case 2: // erase all armor
            for (int i = 0; i < 3; i++) pPlayer->armor[i] = 0;
            if (pXSource->data2) break;
            fallthrough__;
        case 3: // erase all pack items
            for (int i = 0; i < 5; i++) {
                pPlayer->packSlots[i].isActive = false;
                pPlayer->packSlots[i].curAmount = 0;
                pPlayer->packItemId = -1;
            }
            if (pXSource->data2) break;
            fallthrough__;
        case 4: // erase all keys
            for (int i = 0; i < 8; i++) pPlayer->hasKey[i] = false;
            if (pXSource->data2) break;
    }

}

void trPlayerCtrlGiveStuff(XSPRITE* pXSource, PLAYER* pPlayer, TRPLAYERCTRL* pCtrl) {
    switch (pXSource->data2) {
        case 1: // give N weapon and default ammo for it
        case 2: // give just N ammo for selected weapon
            if (pXSource->data3 <= 0 || pXSource->data3 > 12) break;
            for (int i = 0; i < 12; i++) {
                if (gWeaponItemData[i].type != pXSource->data3) continue;

                WEAPONITEMDATA* pWeaponData = &gWeaponItemData[i]; int nAmmoType = pWeaponData->ammoType;
                if (pXSource->data2 == 1) {
                    pPlayer->ammoCount[nAmmoType] = ClipHigh(pPlayer->ammoCount[nAmmoType] + pWeaponData->count, gAmmoInfo[nAmmoType].max);
                } else {
                    pPlayer->ammoCount[nAmmoType] = ClipHigh(pPlayer->ammoCount[nAmmoType] + pXSource->data4, gAmmoInfo[nAmmoType].max);
                    break;
                }

                pPlayer->hasWeapon[pXSource->data3] = true;

                if (pXSource->data4 == 0) { // switch on it
                    pPlayer->nextWeapon = 0;

                    if (pPlayer->sceneQav >= 0 && spriRangeIsFine(pCtrl->qavScene.index)) {
                        XSPRITE* pXScene = &xsprite[sprite[pCtrl->qavScene.index].extra];
                        pXScene->dropMsg = pXSource->data3;
                    } else if (pPlayer->curWeapon != pXSource->data3) {
                        pPlayer->input.newWeapon = pXSource->data3;
                        WeaponRaise(pPlayer);
                    }
                }
                break;
            }
            break;
    }
}

void trPlayerCtrlUsePackItem(XSPRITE* pXSource, PLAYER* pPlayer) {
    unsigned int invItem = pXSource->data2 - 1;
    packUseItem(pPlayer, invItem);

    // force remove after use
    if (pXSource->data4 == 1) {
        pPlayer->packSlots[invItem].isActive = false;
        pPlayer->packSlots[invItem].curAmount = 0;
    }
}

void useObjResizer(XSPRITE* pXSource, short objType, int objIndex) {
    switch (objType) {
    // for sectors
    case 6:
        if (valueIsBetween(pXSource->data1, -1, 32767))
            sector[objIndex].floorxpanning = ClipRange(pXSource->data1, 0, 255);

        if (valueIsBetween(pXSource->data2, -1, 32767))
            sector[objIndex].floorypanning = ClipRange(pXSource->data2, 0, 255);

        if (valueIsBetween(pXSource->data3, -1, 32767))
            sector[objIndex].ceilingxpanning = ClipRange(pXSource->data3, 0, 255);

        if (valueIsBetween(pXSource->data4, -1, 65535))
            sector[objIndex].ceilingypanning = ClipRange(pXSource->data4, 0, 255);
        break;
    // for sprites
    case 3:

        // resize by seq scaling
        if (sprite[pXSource->reference].flags & kModernTypeFlag1) {
            if (valueIsBetween(pXSource->data1, -255, 32767)) {
                int mulDiv = (valueIsBetween(pXSource->data2, 0, 257)) ? pXSource->data2 : 256;
                if (pXSource->data1 > 0) xsprite[sprite[objIndex].extra].scale = mulDiv * ClipHigh(pXSource->data1, 25);
                else if (pXSource->data1 < 0) xsprite[sprite[objIndex].extra].scale = mulDiv / ClipHigh(abs(pXSource->data1), 25);
                else xsprite[sprite[objIndex].extra].scale = 0;

                // request properties update for custom dude
                switch (sprite[objIndex].type) {
                case kDudeModernCustom:
                case kDudeModernCustomBurning:
                    gGenDudeExtra[objIndex].updReq[kGenDudePropertySpriteSize] = true;
                    gGenDudeExtra[objIndex].updReq[kGenDudePropertyAttack] = true;
                    gGenDudeExtra[objIndex].updReq[kGenDudePropertyMass] = true;
                    gGenDudeExtra[objIndex].updReq[kGenDudePropertyDmgScale] = true;
                    evPost(objIndex, 3, kGenDudeUpdTimeRate, kCallbackGenDudeUpdate);
                    break;
                }
            }

        // resize by repeats
        } else {

            if (valueIsBetween(pXSource->data1, -1, 32767))
                sprite[objIndex].xrepeat = ClipRange(pXSource->data1, 0, 255);

            if (valueIsBetween(pXSource->data2, -1, 32767))
                sprite[objIndex].yrepeat = ClipRange(pXSource->data2, 0, 255);

        }

        if (valueIsBetween(pXSource->data3, -1, 32767))
            sprite[objIndex].xoffset = ClipRange(pXSource->data3, 0, 255);

        if (valueIsBetween(pXSource->data4, -1, 65535))
            sprite[objIndex].yoffset = ClipRange(pXSource->data4, 0, 255);
        break;
    case OBJ_WALL:
        if (valueIsBetween(pXSource->data1, -1, 32767))
            wall[objIndex].xrepeat = ClipRange(pXSource->data1, 0, 255);

        if (valueIsBetween(pXSource->data2, -1, 32767))
            wall[objIndex].yrepeat = ClipRange(pXSource->data2, 0, 255);

        if (valueIsBetween(pXSource->data3, -1, 32767))
            wall[objIndex].xpanning = ClipRange(pXSource->data3, 0, 255);

        if (valueIsBetween(pXSource->data4, -1, 65535))
            wall[objIndex].ypanning = ClipRange(pXSource->data4, 0, 255);
        break;
    }

}

void usePropertiesChanger(XSPRITE* pXSource, short objType, int objIndex) {

    spritetype* pSource = &sprite[pXSource->reference];

    switch (objType) {
        case OBJ_WALL: {
            walltype* pWall = &wall[objIndex]; int old = -1;

            // data3 = set wall hitag
            if (valueIsBetween(pXSource->data3, -1, 32767)) {
                if ((pSource->flags & kModernTypeFlag1)) pWall->hitag = pWall->hitag |= pXSource->data3;
                else pWall->hitag = pXSource->data3;
            }

            // data4 = set wall cstat
            if (valueIsBetween(pXSource->data4, -1, 65535)) {
                old = pWall->cstat;

                // set new cstat
                if ((pSource->flags & kModernTypeFlag1)) pWall->cstat = pWall->cstat |= pXSource->data4; // relative
                else pWall->cstat = pXSource->data4; // absolute

                // and hanlde exceptions
                if ((old & 0x2) && !(pWall->cstat & 0x2)) pWall->cstat |= 0x2; // kWallBottomSwap
                if ((old & 0x4) && !(pWall->cstat & 0x4)) pWall->cstat |= 0x4; // kWallBottomOrg, kWallOutsideOrg
                if ((old & 0x20) && !(pWall->cstat & 0x20)) pWall->cstat |= 0x20; // kWallOneWay

                if (old & 0xc000) {

                    if (!(pWall->cstat & 0xc000))
                        pWall->cstat |= 0xc000; // kWallMoveMask

                    if ((old & 0x0) && !(pWall->cstat & 0x0)) pWall->cstat |= 0x0; // kWallMoveNone
                    else if ((old & 0x4000) && !(pWall->cstat & 0x4000)) pWall->cstat |= 0x4000; // kWallMoveForward
                    else if ((old & 0x8000) && !(pWall->cstat & 0x8000)) pWall->cstat |= 0x8000; // kWallMoveBackward

                }
            }
        }
        break;
        case OBJ_SPRITE: {
            spritetype* pSprite = &sprite[objIndex]; bool thing2debris = false;
            XSPRITE* pXSprite = &xsprite[pSprite->extra]; int old = -1;

            // data3 = set sprite hitag
            if (valueIsBetween(pXSource->data3, -1, 32767)) {
                old = pSprite->flags;

                // set new hitag
                if ((pSource->flags & kModernTypeFlag1)) pSprite->flags = pSource->flags |= pXSource->data3; // relative
                else pSprite->flags = pXSource->data3;  // absolute

                // and handle exceptions
                if ((old & kHitagFree) && !(pSprite->flags & kHitagFree)) pSprite->flags |= kHitagFree;
                if ((old & kHitagRespawn) && !(pSprite->flags & kHitagRespawn)) pSprite->flags |= kHitagRespawn;

                // prepare things for different (debris) physics.
                if (pSprite->statnum == kStatThing && debrisGetFreeIndex() >= 0) thing2debris = true;

            }

            // data2 = sprite physics settings
            if ((pXSource->data2 >= 0 && pXSource->data3 <= 33) || thing2debris) {
                switch (pSprite->statnum) {
                case kStatDude: // dudes already treating in game
                case kStatFree:
                case kStatMarker:
                case kStatPathMarker: // path marker
                    break;
                default:
                    // store physics attributes in xsprite to avoid setting hitag for modern types!
                    int flags = (pXSprite->physAttr != 0) ? pXSprite->physAttr : 0;

                    if (thing2debris) {

                        // converting thing to debris
                        if ((pSprite->flags & kPhysMove) != 0) flags |= kPhysMove;
                        else flags &= ~kPhysMove;

                        if ((pSprite->flags & kPhysGravity) != 0) flags |= (kPhysGravity | kPhysFalling);
                        else flags &= ~(kPhysGravity | kPhysFalling);

                        pSprite->flags &= ~(kPhysMove | kPhysGravity | kPhysFalling);
                        xvel[objIndex] = yvel[objIndex] = zvel[objIndex] = 0;  pXSprite->restState = pXSprite->state;

                    } else {

                        // first digit of data2: set main physics attributes
                        switch (pXSource->data2) {
                            case 0:
                                flags &= ~kPhysMove;
                                flags &= ~(kPhysGravity | kPhysFalling);
                                break;

                            case 1: case 10: case 11: case 12: case 13:
                                flags |= kPhysMove;
                                flags &= ~(kPhysGravity | kPhysFalling);
                                break;

                            case 2: case 20: case 21: case 22: case 23:
                                flags &= ~kPhysMove;
                                flags |= (kPhysGravity | kPhysFalling);
                                break;

                            case 3: case 30: case 31: case 32: case 33:
                                flags |= kPhysMove;
                                flags |= (kPhysGravity | kPhysFalling);
                                break;
                        }

                        // second digit of data2: set physics flags
                        switch (pXSource->data2) {
                            case 0: case 1: case 2: case 3:
                            case 10: case 20: case 30:
                                flags &= ~kPhysDebrisVector;
                                flags &= ~kPhysDebrisExplode;
                                break;

                            case 11: case 21: case 31:
                                flags |= kPhysDebrisVector;
                                flags &= ~kPhysDebrisExplode;
                                break;

                            case 12: case 22: case 32:
                                flags &= ~kPhysDebrisVector;
                                flags |= kPhysDebrisExplode;
                                break;

                            case 13: case 23: case 33:
                                flags |= kPhysDebrisVector;
                                flags |= kPhysDebrisExplode;
                                break;
                        }

                    }

                    int nIndex = debrisGetIndex(objIndex); // check if there is no sprite in list

                    // adding physics sprite in list
                    if ((flags & kPhysGravity) != 0 || (flags & kPhysMove) != 0) {

                        if (nIndex != -1) pXSprite->physAttr = flags; // just update physics attributes
                        else if ((nIndex = debrisGetFreeIndex()) < 0)
                            viewSetSystemMessage("Max (%d) Physics affected sprites reached!", kMaxSuperXSprites);
                        else {

                            pXSprite->physAttr = flags; // update physics attributes

                            // allow things to became debris, so they use different physics...
                            if (pSprite->statnum == kStatThing) changespritestat(objIndex, 0);
                            //actPostSprite(nDest, kStatDecoration); // !!!! not working here for some reason

                            gPhysSpritesList[nIndex] = objIndex;
                            if (nIndex >= gPhysSpritesCount) gPhysSpritesCount++;
                            getSpriteMassBySize(pSprite); // create physics cache

                        }

                    // removing physics from sprite in list (don't remove sprite from list)
                    } else if (nIndex != -1) {

                        pXSprite->physAttr = flags;
                        xvel[objIndex] = yvel[objIndex] = zvel[objIndex] = 0;
                        if (pSprite->lotag >= kThingBase && pSprite->lotag < kThingMax)
                            changespritestat(objIndex, kStatThing);  // if it was a thing - restore statnum

                    }

                    break;
                }
            }

            // data4 = sprite cstat
            if (valueIsBetween(pXSource->data4, -1, 65535)) {

                old = pSprite->cstat;

                // set new cstat
                if ((pSource->flags & kModernTypeFlag1)) pSprite->cstat = pSprite->cstat |= pXSource->data4; // relative
                else pSprite->cstat = pXSource->data4; // absolute

                                                       // and handle exceptions
                if ((old & 0x1000) && !(pSprite->cstat & 0x1000)) pSprite->cstat |= 0x1000; //kSpritePushable
                if ((old & 0x80) && !(pSprite->cstat & 0x80)) pSprite->cstat |= 0x80; // kSpriteOriginAlign

                if (old & 0x6000) {

                    if (!(pSprite->cstat & 0x6000))
                        pSprite->cstat |= 0x6000; // kSpriteMoveMask

                    if ((old & 0x0) && !(pSprite->cstat & 0x0)) pSprite->cstat |= 0x0; // kSpriteMoveNone
                    else if ((old & 0x2000) && !(pSprite->cstat & 0x2000)) pSprite->cstat |= 0x2000; // kSpriteMoveForward, kSpriteMoveFloor
                    else if ((old & 0x4000) && !(pSprite->cstat & 0x4000)) pSprite->cstat |= 0x4000; // kSpriteMoveReverse, kSpriteMoveCeiling

                }

            }
        }
        break;
        case OBJ_SECTOR: {

            XSECTOR* pXSector = &xsector[sector[objIndex].extra];

            // data1 = sector underwater status and depth level
            if (pXSource->data1 == 0) pXSector->Underwater = false;
            else if (pXSource->data1 == 1) pXSector->Underwater = true;
            else if (pXSource->data1 > 9) pXSector->Depth = 7;
            else if (pXSource->data1 > 1) pXSector->Depth = pXSource->data1 - 2;


            // data2 = sector visibility
            if (valueIsBetween(pXSource->data2, -1, 32767)) {
                if (pXSource->data2 > 234) sector[objIndex].visibility = 234;
                else sector[objIndex].visibility = pXSource->data2;
            }

            // data3 = sector ceil cstat
            if (valueIsBetween(pXSource->data3, -1, 32767)) {
                if ((pSource->flags & kModernTypeFlag1)) sector[objIndex].ceilingstat = sector[objIndex].ceilingstat |= pXSource->data3;
                else sector[objIndex].ceilingstat = pXSource->data3;
            }

            // data4 = sector floor cstat
            if (valueIsBetween(pXSource->data4, -1, 65535)) {
                if ((pSource->flags & kModernTypeFlag1)) sector[objIndex].floorstat = sector[objIndex].floorstat |= pXSource->data4;
                else sector[objIndex].floorstat = pXSource->data4;
            }
        }
        break;
        // no TX id
        case -1:
            // data2 = global visibility
            if (valueIsBetween(pXSource->data2, -1, 32767))
                gVisibility = ClipRange(pXSource->data2, 0, 4096);
        break;
    }

}

void useTeleportTarget(XSPRITE* pXSource, spritetype* pSprite) {
    spritetype* pSource = &sprite[pXSource->reference]; PLAYER* pPlayer = getPlayerById(pSprite->type);
    XSECTOR* pXSector = (sector[pSource->sectnum].extra >= 0) ? &xsector[sector[pSource->sectnum].extra] : NULL;

    pSprite->x = pSource->x; pSprite->y = pSource->y;
    pSprite->z += (sector[pSource->sectnum].floorz - sector[pSprite->sectnum].floorz);

    if (pSource->flags & kModernTypeFlag1) // force telefrag
        TeleFrag(pSprite->index, pSource->sectnum);

    changespritesect(pSprite->index, pSource->sectnum);
    if (pXSector && pXSector->Underwater) {
        spritetype* pLink = (gLowerLink[pSource->sectnum]) ? &sprite[gLowerLink[pSource->sectnum]] : NULL;
        if (pLink) {

            // must be sure we found exact same upper link
            for (int i = 0; i < kMaxSectors; i++) {
                if (xsprite[sprite[gUpperLink[i]].extra].data1 != xsprite[pLink->extra].data1) continue;
                pLink = &sprite[gUpperLink[i]];
                break;
            }

        }

        if (pLink)
            xsprite[pSprite->extra].medium = (pLink->type == kMarkerUpGoo) ? kMediumGoo : kMediumWater;

        if (pPlayer) {
            int waterPal = kMediumWater;
            if (pLink) {
                if (xsprite[pLink->extra].data2 > 0) waterPal = xsprite[pLink->extra].data2;
                else if (pLink->type == kMarkerUpGoo) waterPal = kMediumGoo;
            }

            pPlayer->nWaterPal = waterPal;
            pPlayer->posture = kPostureSwim;
            pPlayer->pXSprite->burnTime = 0;
        }

    } else {

        xsprite[pSprite->extra].medium = kMediumNormal;
        if (pPlayer) {
            pPlayer->posture = (!pPlayer->input.buttonFlags.crouch) ? kPostureStand : kPostureCrouch;
            pPlayer->nWaterPal = 0;
        }

    }

    if (pXSource->data2 == 1) {
        
        if (pPlayer) pPlayer->q16ang = fix16_from_int(pSource->ang);
        else if (IsDudeSprite(pSprite)) xsprite[pSprite->extra].goalAng = pSprite->ang = pSource->ang;
        else pSprite->ang = pSource->ang;
    }

    if (pXSource->data3 == 1)
        xvel[pSprite->index] = yvel[pSprite->index] = zvel[pSprite->index] = 0;

    viewBackupSpriteLoc(pSprite->index, pSprite);

    if (pXSource->data4 > 0)
        sfxPlay3DSound(pSource, pXSource->data4, -1, 0);

    if (pPlayer) {
        playerResetInertia(pPlayer);
        if (pXSource->data2 == 1)
            pPlayer->zViewVel = pPlayer->zWeaponVel = 0;
    }
}


void useEffectGen(XSPRITE* pXSource, spritetype* pSprite) {
    if (pSprite == NULL) pSprite = &sprite[pXSource->reference];
    if (pSprite->extra < 0) return;

    int fxId = pXSource->data2 + Random(pXSource->data3);
    int pos, top, bottom; GetSpriteExtents(pSprite, &top, &bottom); spritetype* pEffect = NULL;

    // select where exactly effect should be spawned
    switch (pXSource->data4) {
    case 1:
        pos = bottom; // bottom of sprite
        break;
    default:
        pos = top; // top of sprite
        break;
    }

    if (fxId > 0 && fxId < 57 && (pEffect = gFX.fxSpawn((FX_ID)fxId, pSprite->sectnum, pSprite->x, pSprite->y, pos, 0)) != NULL) {

        if (pEffect->cstat & CSTAT_SPRITE_ONE_SIDED) pEffect->cstat &= ~CSTAT_SPRITE_ONE_SIDED;

        if (pSprite->flags & kModernTypeFlag1) {
            pEffect->pal = pSprite->pal;
            pEffect->xrepeat = pSprite->xrepeat;
            pEffect->yrepeat = pSprite->yrepeat;
            pEffect->shade = pSprite->shade;
        }
    }
}


void useSectorWindGen(XSPRITE* pXSource, sectortype* pSector) {

    spritetype* pSource = &sprite[pXSource->reference];
    XSECTOR* pXSector = NULL; bool forceWind = false;
    int nXSector = 0;
    if (pSector == NULL) {

        if (sector[pSource->sectnum].extra < 0) {
            int nXSector = dbInsertXSector(pSource->sectnum);
            if (nXSector > 0) pXSector = &xsector[nXSector];
            else return;

            forceWind = true;

        } else {
            pXSector = &xsector[sector[pSource->sectnum].extra];
            nXSector = sector[pXSector->reference].extra;
        }

    } else {
        pXSector = &xsector[pSector->extra];
        nXSector = sector[pXSector->reference].extra;
    }

    if (pSource->flags) {
        pXSector->panAlways = 1;
        pXSector->windAlways = 1;
    } else if (forceWind)
        pXSector->windAlways = 1;

    if (pXSource->data2 > 32766) pXSource->data2 = 32767;

    if (pXSource->data1 == 1 || pXSource->data1 == 3) pXSector->windVel = Random(pXSource->data2);
    else pXSector->windVel = pXSource->data2;

    if (pXSource->data1 == 2 || pXSource->data1 == 3) {
        short ang = pSource->ang;
        while (pSource->ang == ang)
            pSource->ang = Random3(kAng360);
    }

    pXSector->windAng = pSource->ang;

    if (pXSource->data3 > 0 && pXSource->data3 < 4) {
        switch (pXSource->data3) {
        case 1:
            pXSector->panFloor = true;
            pXSector->panCeiling = false;
            break;
        case 2:
            pXSector->panFloor = false;
            pXSector->panCeiling = true;
            break;
        case 3:
            pXSector->panFloor = true;
            pXSector->panCeiling = true;
            break;
        }

        short oldPan = pXSector->panVel;
        pXSector->panAngle = pXSector->windAng;
        pXSector->panVel = pXSector->windVel;

        // add to panList if panVel was set to 0 previously
        if (oldPan == 0 && pXSector->panVel != 0 && panCount < kMaxXSprites) {

            int i;
            for (i = 0; i < panCount; i++) {
                if (panList[i] != nXSector) continue;
                break;
            }

            if (i == panCount)
                panList[panCount++] = nXSector;
        }

    }
}



void useSpriteDamager(XSPRITE* pXSource, spritetype* pSprite) {
    spritetype* pSource = &sprite[pXSource->reference];
    if (pSprite != NULL && xspriRangeIsFine(pSprite->extra) && xsprite[pSprite->extra].health > 0) {
        DAMAGE_TYPE dmgType = (DAMAGE_TYPE)ClipRange(pXSource->data2, kDmgFall, kDmgElectric);
        int dmg = (pXSource->data3 == 0) ? 65535 : ClipRange(pXSource->data3 << 1, 1, 65535);
        if (pXSource->data2 >= 0) actDamageSprite(pSource->index, pSprite, dmgType, dmg);
        else if (pXSource->data2 == -1 && IsDudeSprite(pSprite)) {
            PLAYER* pPlayer = getPlayerById(pSprite->type);
            if (pPlayer == NULL || !pPlayer->godMode) {
                xsprite[pSprite->extra].health = ClipLow(xsprite[pSprite->extra].health - dmg, 0);
                if (xsprite[pSprite->extra].health == 0) {
                    if (pPlayer == NULL) actKillDude(pSource->index, pSprite, DAMAGE_TYPE_0, 4);
                    else playerDamageSprite(pSource->index, pPlayer, DAMAGE_TYPE_0, 4);
                }
            }
        }
    }
}

void useSeqSpawnerGen(XSPRITE* pXSource, int objType, int index) {
    if (pXSource->data2 > 0 && !gSysRes.Lookup(pXSource->data2, "SEQ")) {
        consoleSysMsg("Missing sequence #%d", pXSource->data2);
        return;
    }

    switch (objType) {
        case OBJ_SECTOR:
            if (pXSource->data2 <= 0) {
                if (pXSource->data3 == 3 || pXSource->data3 == 1)
                    seqKill(2, sector[index].extra);
                if (pXSource->data3 == 3 || pXSource->data3 == 2)
                    seqKill(1, sector[index].extra);
            } else {
                if (pXSource->data3 == 3 || pXSource->data3 == 1)
                    seqSpawn(pXSource->data2, 2, sector[index].extra, -1);
                if (pXSource->data3 == 3 || pXSource->data3 == 2)
                    seqSpawn(pXSource->data2, 1, sector[index].extra, -1);
            }
            return;
        case OBJ_WALL:
            if (pXSource->data2 <= 0) {
                if (pXSource->data3 == 3 || pXSource->data3 == 1)
                    seqKill(0, wall[index].extra);
                if ((pXSource->data3 == 3 || pXSource->data3 == 2) && (wall[index].cstat & CSTAT_WALL_MASKED))
                    seqKill(4, wall[index].extra);
            } else {

                if (pXSource->data3 == 3 || pXSource->data3 == 1)
                    seqSpawn(pXSource->data2, 0, wall[index].extra, -1);
                if (pXSource->data3 == 3 || pXSource->data3 == 2) {

                    if (wall[index].nextwall < 0) {
                        if (pXSource->data3 == 3)
                            seqSpawn(pXSource->data2, 0, wall[index].extra, -1);

                    } else {
                        if (!(wall[index].cstat & CSTAT_WALL_MASKED))
                            wall[index].cstat |= CSTAT_WALL_MASKED;

                        seqSpawn(pXSource->data2, 4, wall[index].extra, -1);
                    }
                }

                if (pXSource->data4 > 0) {

                    int cx, cy, cz;
                    cx = (wall[index].x + wall[wall[index].point2].x) >> 1;
                    cy = (wall[index].y + wall[wall[index].point2].y) >> 1;
                    int nSector = sectorofwall(index);
                    int32_t ceilZ, floorZ;
                    getzsofslope(nSector, cx, cy, &ceilZ, &floorZ);
                    int32_t ceilZ2, floorZ2;
                    getzsofslope(wall[index].nextsector, cx, cy, &ceilZ2, &floorZ2);
                    ceilZ = ClipLow(ceilZ, ceilZ2);
                    floorZ = ClipHigh(floorZ, floorZ2);
                    cz = (ceilZ + floorZ) >> 1;

                    sfxPlay3DSound(cx, cy, cz, pXSource->data4, nSector);

                }

            }
            return;
        case OBJ_SPRITE:
            if (pXSource->data2 <= 0) seqKill(3, sprite[index].extra);
            else {
                seqSpawn(pXSource->data2, 3, sprite[index].extra, -1);
                if (pXSource->data4 > 0) sfxPlay3DSound(&sprite[index], pXSource->data4, -1, 0);
            }
            return;
    }
}

bool valueIsBetween(int val, int min, int max) {
    return (val > min&& val < max);
}

char modernTypeSetSpriteState(int nSprite, XSPRITE* pXSprite, int nState) {
    if ((pXSprite->busy & 0xffff) == 0 && pXSprite->state == nState)
        return 0;

    pXSprite->busy = nState << 16; pXSprite->state = nState;
    
    evKill(nSprite, 3);
    if (pXSprite->restState != nState && pXSprite->waitTime > 0)
        evPost(nSprite, 3, (pXSprite->waitTime * 120) / 10, pXSprite->restState ? kCmdOn : kCmdOff);

    if (pXSprite->txID != 0 && ((pXSprite->triggerOn && pXSprite->state) || (pXSprite->triggerOff && !pXSprite->state)))
        modernTypeSendCommand(nSprite, pXSprite->txID, (COMMAND_ID)pXSprite->command);

    return 1;
}

void modernTypeSendCommand(int nSprite, int destChannel, COMMAND_ID command) {
    switch (command) {
    case kCmdLink:
        evSend(nSprite, 3, destChannel, kCmdModernUse); // just send command to change properties
        return;
    case kCmdUnlock:
        evSend(nSprite, 3, destChannel, command); // send normal command first
        evSend(nSprite, 3, destChannel, kCmdModernUse);  // then send command to change properties
        return;
    default:
        evSend(nSprite, 3, destChannel, kCmdModernUse); // send first command to change properties
        evSend(nSprite, 3, destChannel, command); // then send normal command
        return;
    }
}

// this function used by various new modern types.
void modernTypeTrigger(int destObjType, int destObjIndex, EVENT event) {

    if (event.type != OBJ_SPRITE) return;
    spritetype* pSource = &sprite[event.index];

    if (!xspriRangeIsFine(pSource->extra)) return;
    XSPRITE* pXSource = &xsprite[pSource->extra];

    switch (destObjType) {
        case OBJ_SECTOR:
            if (!xsectRangeIsFine(sector[destObjIndex].extra)) return;
            break;
        case OBJ_WALL:
            if (!xwallRangeIsFine(wall[destObjIndex].extra)) return;
            break;
        case OBJ_SPRITE:
            if (!xspriRangeIsFine(sprite[destObjIndex].extra)) return;
            if (sprite[destObjIndex].flags & 32) return;
            break;
        default:
            return;
    }

    switch (pSource->type) {
        // allows teleport any sprite from any location to the source destination
        case kMarkerWarpDest:
            if (destObjType != OBJ_SPRITE) break;
            useTeleportTarget(pXSource, &sprite[destObjIndex]);
            break;
        case kModernSpriteDamager:
        // damages xsprite via TX ID
            if (destObjType != OBJ_SPRITE) break;
            useSpriteDamager(pXSource, &sprite[destObjIndex]);
            break;
        // can spawn any effect passed in data2 on it's or txID sprite
        case kModernEffectSpawner:
            if (destObjType != OBJ_SPRITE || pXSource->data2 < 0 || pXSource->data2 >= kFXMax) break;
            useEffectGen(pXSource, &sprite[destObjIndex]);
            break;
        // takes data2 as SEQ ID and spawns it on it's or TX ID object
        case kModernSeqSpawner:
            useSeqSpawnerGen(pXSource, destObjType, destObjIndex);
            break;
        // creates wind on TX ID sector
        case kModernWindGenerator:
            if (destObjType != OBJ_SECTOR || pXSource->data2 < 0) break;
            useSectorWindGen(pXSource, &sector[destObjIndex]);
            break;
        // size and pan changer of sprite/wall/sector via TX ID
        case kModernObjSizeChanger:
            useObjResizer(pXSource, destObjType, destObjIndex);
            break;
        // iterate data filed value of destination object
        case kModernObjDataAccumulator:
            useIncDecGen(pXSource, destObjType, destObjIndex);
            break;
        // change data field value of destination object
        case kModernObjDataChanger:
            useDataChanger(pXSource, destObjType, destObjIndex);
            break;
        // change sector lighting dynamically
        case kModernSectorFXChanger:
            if (destObjType != OBJ_SECTOR) break;
            useSectorLigthChanger(pXSource, &xsector[sector[destObjIndex].extra]);
            break;
        // change target of dudes and make it fight
        case kModernDudeTargetChanger:
            if (destObjType != OBJ_SPRITE) break;
            useTargetChanger(pXSource, &sprite[destObjIndex]);
            break;
        // change picture and palette of TX ID object
        case kModernObjPicnumChanger:
            usePictureChanger(pXSource, destObjType, destObjIndex);
            break;
        // change various properties
        case kModernObjPropertiesChanger:
            usePropertiesChanger(pXSource, destObjType, destObjIndex);
            break;
    }
}

// the following functions required for kModernDudeTargetChanger
//---------------------------------------
spritetype* aiFightGetTargetInRange(spritetype* pSprite, int minDist, int maxDist, short data, short teamMode) {
    DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type); XSPRITE* pXSprite = &xsprite[pSprite->extra];
    spritetype* pTarget = NULL; XSPRITE* pXTarget = NULL; spritetype* cTarget = NULL;
    for (int nSprite = headspritestat[kStatDude]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
        pTarget = &sprite[nSprite];  pXTarget = &xsprite[pTarget->extra];
        if (!aiFightDudeCanSeeTarget(pXSprite, pDudeInfo, pTarget)) continue;

        int dist = aiFightGetTargetDist(pSprite, pDudeInfo, pTarget);
        if (dist < minDist || dist > maxDist) continue;
        else if (pXSprite->target == pTarget->index) return pTarget;
        else if (!IsDudeSprite(pTarget) || pTarget->index == pSprite->index || IsPlayerSprite(pTarget)) continue;
        else if (IsBurningDude(pTarget) || !IsKillableDude(pTarget) || pTarget->owner == pSprite->index) continue;
        else if ((teamMode == 1 && aiFightIsMateOf(pXSprite, pXTarget)) || aiFightMatesHaveSameTarget(pXSprite, pTarget, 1)) continue;
        else if (data == 666 || pXTarget->data1 == data) {

            if (pXSprite->target > 0) {
                cTarget = &sprite[pXSprite->target];
                int fineDist1 = aiFightGetFineTargetDist(pSprite, cTarget);
                int fineDist2 = aiFightGetFineTargetDist(pSprite, pTarget);
                if (fineDist1 < fineDist2)
                    continue;
            }
            return pTarget;
        }
    }

    return NULL;
}

spritetype* aiFightTargetIsPlayer(XSPRITE* pXSprite) {

    if (pXSprite->target >= 0) {
        if (IsPlayerSprite(&sprite[pXSprite->target]))
            return &sprite[pXSprite->target];
    }

    return NULL;
}
spritetype* aiFightGetMateTargets(XSPRITE* pXSprite) {
    int rx = pXSprite->rxID; spritetype* pMate = NULL; XSPRITE* pXMate = NULL;

    for (int i = bucketHead[rx]; i < bucketHead[rx + 1]; i++) {
        if (rxBucket[i].type == OBJ_SPRITE) {
            pMate = &sprite[rxBucket[i].index];
            if (pMate->extra < 0 || pMate->index == sprite[pXSprite->reference].index || !IsDudeSprite(pMate))
                continue;

            pXMate = &xsprite[pMate->extra];
            if (pXMate->target > -1) {
                if (!IsPlayerSprite(&sprite[pXMate->target]))
                    return &sprite[pXMate->target];
            }

        }
    }

    return NULL;
}

bool aiFightMatesHaveSameTarget(XSPRITE* pXLeader, spritetype* pTarget, int allow) {
    int rx = pXLeader->rxID; spritetype* pMate = NULL; XSPRITE* pXMate = NULL;

    for (int i = bucketHead[rx]; i < bucketHead[rx + 1]; i++) {

        if (rxBucket[i].type != OBJ_SPRITE)
            continue;

        pMate = &sprite[rxBucket[i].index];
        if (pMate->extra < 0 || pMate->index == sprite[pXLeader->reference].index || !IsDudeSprite(pMate))
            continue;

        pXMate = &xsprite[pMate->extra];
        if (pXMate->target == pTarget->index && allow-- <= 0)
            return true;
    }

    return false;

}

bool aiFightDudeCanSeeTarget(XSPRITE* pXDude, DUDEINFO* pDudeInfo, spritetype* pTarget) {
    spritetype* pDude = &sprite[pXDude->reference];
    int dx = pTarget->x - pDude->x; int dy = pTarget->y - pDude->y;

    //viewSetSystemMessage("zzzz");
    // check target
    if (approxDist(dx, dy) < pDudeInfo->seeDist) {
        int eyeAboveZ = pDudeInfo->eyeHeight * pDude->yrepeat << 2;

        // is there a line of sight to the target?
        if (cansee(pDude->x, pDude->y, pDude->z, pDude->sectnum, pTarget->x, pTarget->y, pTarget->z - eyeAboveZ, pTarget->sectnum)) {
            /*int nAngle = getangle(dx, dy);
            int losAngle = ((1024 + nAngle - pDude->ang) & 2047) - 1024;

            // is the target visible?
            if (klabs(losAngle) < 2048) // 360 deg periphery here*/
            return true;
        }

    }

    return false;

}

// this function required if monsters in genIdle ai state. It wakes up monsters
// when kModernDudeTargetChanger goes to off state, so they won't ignore the world.
void aiFightActivateDudes(int rx) {
    for (int i = bucketHead[rx]; i < bucketHead[rx + 1]; i++) {
        if (rxBucket[i].type != OBJ_SPRITE) continue;
        spritetype* pDude = &sprite[rxBucket[i].index]; XSPRITE* pXDude = &xsprite[pDude->extra];
        if (!IsDudeSprite(pDude) || pXDude->aiState->stateType != kAiStateGenIdle) continue;
        aiInitSprite(pDude);
    }
}


// this function sets target to -1 for all dudes that hunting for nSprite
void aiFightFreeTargets(int nSprite) {
    for (int nTarget = headspritestat[kStatDude]; nTarget >= 0; nTarget = nextspritestat[nTarget]) {
        if (!IsDudeSprite(&sprite[nTarget]) || sprite[nTarget].extra < 0) continue;
        else if (xsprite[sprite[nTarget].extra].target == nSprite)
            aiSetTarget(&xsprite[sprite[nTarget].extra], sprite[nTarget].x, sprite[nTarget].y, sprite[nTarget].z);
    }

    return;
}

// this function sets target to -1 for all targets that hunting for dudes affected by selected kModernDudeTargetChanger
void aiFightFreeAllTargets(XSPRITE* pXSource) {
    if (pXSource->txID <= 0) return;
    for (int i = bucketHead[pXSource->txID]; i < bucketHead[pXSource->txID + 1]; i++) {
        if (rxBucket[i].type == OBJ_SPRITE && sprite[rxBucket[i].index].extra >= 0)
            aiFightFreeTargets(rxBucket[i].index);
    }

    return;
}

bool aiFightDudeIsAffected(XSPRITE* pXDude) {
    if (pXDude->rxID <= 0 || pXDude->locked == 1) return false;
    for (int nSprite = headspritestat[kStatModernDudeTargetChanger]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
        XSPRITE* pXSprite = (sprite[nSprite].extra >= 0) ? &xsprite[sprite[nSprite].extra] : NULL;
        if (pXSprite == NULL || pXSprite->txID <= 0 || pXSprite->state != 1) continue;
        for (int i = bucketHead[pXSprite->txID]; i < bucketHead[pXSprite->txID + 1]; i++) {
            if (rxBucket[i].type != OBJ_SPRITE) continue;

            spritetype* pSprite = &sprite[rxBucket[i].index];
            if (pSprite->extra < 0 || !IsDudeSprite(pSprite)) continue;
            else if (pSprite->index == sprite[pXDude->reference].index) return true;
        }
    }
    return false;
}

// this function tells if there any dude found for kModernDudeTargetChanger
bool aiFightGetDudesForBattle(XSPRITE* pXSprite) {
    for (int i = bucketHead[pXSprite->txID]; i < bucketHead[pXSprite->txID + 1]; i++) {
        if (rxBucket[i].type != OBJ_SPRITE) continue;
        else if (IsDudeSprite(&sprite[rxBucket[i].index]) &&
            xsprite[sprite[rxBucket[i].index].extra].health > 0) return true;
    }

    return false;
}

void aiFightAlarmDudesInSight(spritetype* pSprite, int max) {
    spritetype* pDude = NULL; XSPRITE* pXDude = NULL;
    XSPRITE* pXSprite = &xsprite[pSprite->extra];
    DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type);
    for (int nSprite = headspritestat[kStatDude]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
        pDude = &sprite[nSprite];
        if (pDude->index == pSprite->index || !IsDudeSprite(pDude) || pDude->extra < 0)
            continue;
        pXDude = &xsprite[pDude->extra];
        if (aiFightDudeCanSeeTarget(pXSprite, pDudeInfo, pDude)) {
            if (pXDude->target != -1 || pXDude->rxID > 0)
                continue;

            aiSetTarget(pXDude, pDude->x, pDude->y, pDude->z);
            aiActivateDude(pDude, pXDude);
            if (max-- < 1)
                break;
        }
    }
}

bool aiFightIsAnnoyingUnit(spritetype* pDude) {
    switch (pDude->type) {
        case kDudeHand:             case kDudeSpiderBrown:              case kDudeSpiderRed:
        case kDudeSpiderBlack:      case kDudeSpiderMother:             case kDudeBoneEel:
        case kDudeBat:              case kDudeRat:                      case kDudePodGreen:
        case kDudeTentacleGreen:    case kDudeTentacleFire:             case kDudeTentacleMother:
        case kDudePodFire:
            return true;
        default:
            return false;
    }
}

bool aiFightUnitCanFly(spritetype* pDude) {
    switch (pDude->type) {
        case kDudeBat:                  case kDudeGargoyleFlesh:
        case kDudeGargoyleStone:        case kDudePhantasm:
            return true;
        default:
            return false;
    }
}

bool aiFightIsMeleeUnit(spritetype* pDude) {
    switch (pDude->type) {
        case kDudeZombieAxeNormal:          case kDudeZombieAxeBuried:          case kDudeGargoyleFlesh:
        case kDudeHand:                     case kDudeSpiderBrown:              case kDudeSpiderRed:
        case kDudeSpiderBlack:              case kDudeSpiderMother:             case kDudeGillBeast:
        case kDudeBoneEel:                  case kDudeBat:                      case kDudeRat:
        case kDudeTentacleGreen:            case kDudeTentacleFire:             case kDudeTentacleMother:
        case kDudeZombieAxeLaying:          case kDudeInnocent:                 case kDudeTinyCaleb:
        case kDudeBeast:
            return true;
        case kDudeModernCustom:
            return (pDude->extra >= 0 && dudeIsMelee(&xsprite[pDude->extra]));
        default:
            return false;
    }
}

int aiFightGetTargetDist(spritetype* pSprite, DUDEINFO* pDudeInfo, spritetype* pTarget) {
    int x = pTarget->x; int y = pTarget->y;
    int dx = x - pSprite->x; int dy = y - pSprite->y;

    int dist = approxDist(dx, dy);
    if (dist <= pDudeInfo->meleeDist) return 0;
    if (dist >= pDudeInfo->seeDist) return 13;
    if (dist <= pDudeInfo->seeDist / 12) return 1;
    if (dist <= pDudeInfo->seeDist / 11) return 2;
    if (dist <= pDudeInfo->seeDist / 10) return 3;
    if (dist <= pDudeInfo->seeDist / 9) return 4;
    if (dist <= pDudeInfo->seeDist / 8) return 5;
    if (dist <= pDudeInfo->seeDist / 7) return 6;
    if (dist <= pDudeInfo->seeDist / 6) return 7;
    if (dist <= pDudeInfo->seeDist / 5) return 8;
    if (dist <= pDudeInfo->seeDist / 4) return 9;
    if (dist <= pDudeInfo->seeDist / 3) return 10;
    if (dist <= pDudeInfo->seeDist / 2) return 11;
    return 12;
}

int aiFightGetFineTargetDist(spritetype* pSprite, spritetype* pTarget) {
    int x = pTarget->x; int y = pTarget->y;
    int dx = x - pSprite->x; int dy = y - pSprite->y;

    int dist = approxDist(dx, dy);
    return dist;
}

bool modernTypeOperateSprite(int nSprite, spritetype* pSprite, XSPRITE* pXSprite, EVENT event) {
    
    switch (pSprite->type) {
        default:
            return false; // no modern type found to work with, go normal OperateSprite();
        case kDudePlayer1:      case kDudePlayer2:
        case kDudePlayer3:      case kDudePlayer4:
        case kDudePlayer5:      case kDudePlayer6:
        case kDudePlayer7:      case kDudePlayer8:
            switch (event.cmd) {
                case kCmdOff:
                    SetSpriteState(nSprite, pXSprite, 0);
                    break;
                case kCmdOn:
                    SetSpriteState(nSprite, pXSprite, 1);
                    break;
                default:
                    SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1);
                    break;
            }
            return true;
        // add linking for path markers and stacks feature
        case kMarkerLowWater:       case kMarkerUpWater:    case kMarkerUpGoo:
        case kMarkerLowGoo:         case kMarkerUpLink:     case kMarkerLowLink:
        case kMarkerUpStack:        case kMarkerLowStack:   case kMarkerPath:
            if (pXSprite->txID > 0 && pXSprite->command == kCmdLink)
                evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command);
            return true;
        // add spawn random dude feature - works only if at least 2 data fields are not empty.
        case kMarkerDudeSpawn:
            if (gGameOptions.nMonsterSettings && pXSprite->data1 >= kDudeBase && pXSprite->data1 < kDudeVanillaMax) {

                spritetype* pSpawn = NULL;
                if ((pSpawn = randomSpawnDude(pSprite)) == NULL) 
                    return false; // go normal OperateSprite();
            
                XSPRITE* pXSpawn = &xsprite[pSpawn->extra];
                gKillMgr.sub_263E0(1);
                switch (pXSprite->data1) {
                    case kDudeBurningInnocent:
                    case kDudeBurningCultist:
                    case kDudeBurningZombieButcher:
                    case kDudeBurningTinyCaleb:
                    case kDudeBurningBeast:
                        pXSpawn->health = getDudeInfo(pXSprite->data1)->startHealth << 4;
                        pXSpawn->burnTime = 10;
                        pXSpawn->target = -1;
                        aiActivateDude(pSpawn, pXSpawn);
                        break;
                    default:
                        if (pSprite->flags & kModernTypeFlag3) aiActivateDude(pSpawn, pXSpawn);
                        break;
                }
            }
            return true;
        // Random Event Switch takes random data field and uses it as TX ID
        case kModernRandomTX: {

            int tx = 0; int maxRetries = kMaxRandomizeRetries;
            // set range of TX ID if data2 and data3 is empty.
            if (pXSprite->data1 > 0 && pXSprite->data2 <= 0 && pXSprite->data3 <= 0 && pXSprite->data4 > 0) {

                // data1 must be less than data4
                if (pXSprite->data1 > pXSprite->data4) {
                    short tmp = pXSprite->data1;
                    pXSprite->data1 = pXSprite->data4;
                    pXSprite->data4 = tmp;
                }

                //int total = pXSprite->data4 - pXSprite->data1;
                while (maxRetries > 0) {
                    
                    //if (gGameOptions.nGameType == 0 && !VanillaMode() && !DemoRecordStatus()) tx = randomSTD(pXSprite->data1, pXSprite->data4);
                    //else tx = Random(total) + pXSprite->data1;

                    if ((tx = nnExtRandom(pXSprite->data1, pXSprite->data4)) != pXSprite->txID) break;
                    maxRetries--;
                }

            } else {
                while (maxRetries > 0) {
                    if ((tx = randomGetDataValue(pXSprite, kRandomizeTX)) > 0 && tx != pXSprite->txID) break;
                    maxRetries--;
                }
            }

            pXSprite->txID = (tx > 0 && tx < kChannelUserMax) ? tx : 0;
            SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1);
        }
        return true;
        // Sequential Switch takes values from data fields starting from data1 and uses it as TX ID
        case kModernSequentialTX: {
            bool range = false; int cnt = 3; int tx = 0;
            // set range of TX ID if data2 and data3 is empty.
            if (pXSprite->data1 > 0 && pXSprite->data2 <= 0 && pXSprite->data3 <= 0 && pXSprite->data4 > 0) {

                // data1 must be less than data4
                if (pXSprite->data1 > pXSprite->data4) {
                    short tmp = pXSprite->data1;
                    pXSprite->data1 = (short)pXSprite->data4;
                    pXSprite->data4 = tmp;
                }

                // force send command to all TX id in a range
                if (pSprite->flags & kModernTypeFlag1) {
                    for (pXSprite->txID = pXSprite->data1; pXSprite->txID <= pXSprite->data4; pXSprite->txID++) {
                        if (pXSprite->txID > 0)
                            evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command);
                    }

                    pXSprite->txID = pXSprite->sysData1 = 0;
                    return true;
                }

                // Make sure txIndex is correct as we store current index of TX ID here.
                if (pXSprite->sysData1 < pXSprite->data1) pXSprite->sysData1 = pXSprite->data1;
                else if (pXSprite->sysData1 > pXSprite->data4) pXSprite->sysData1 = pXSprite->data4;

                range = true;

            } else {

                // force send command to all TX id specified in data
                if (pSprite->flags & kModernTypeFlag1) {
                    for (int i = 0; i <= 3; i++) {
                        if ((pXSprite->txID = GetDataVal(pSprite, i)) > 0)
                            evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command);
                    }

                    pXSprite->txID = pXSprite->sysData1 = 0;
                    return true;
                }

                // Make sure txIndex is correct as we store current index of data field here.
                if (pXSprite->sysData1 > 3) pXSprite->sysData1 = 0;
                else if (pXSprite->sysData1 < 0) pXSprite->sysData1 = 3;

            }

            switch (event.cmd) {
            case kCmdOff:
                if (range == false) {
                    while (cnt-- >= 0) { // skip empty data fields
                        pXSprite->sysData1--;
                        if (pXSprite->sysData1 < 0) pXSprite->sysData1 = 3;
                        tx = GetDataVal(pSprite, pXSprite->sysData1);
                        if (tx < 0) ThrowError(" -- Current data index is negative");
                        if (tx > 0) break;
                        continue;
                    }
                } else {
                    pXSprite->sysData1--;
                    if (pXSprite->sysData1 < pXSprite->data1) {
                        pXSprite->sysData1 = pXSprite->data4;
                    }
                    tx = pXSprite->sysData1;
                }
                break;
            default:
                if (range == false) {
                    while (cnt-- >= 0) { // skip empty data fields
                        if (pXSprite->sysData1 > 3) pXSprite->sysData1 = 0;
                        tx = GetDataVal(pSprite, pXSprite->sysData1);
                        if (tx < 0) ThrowError(" ++ Current data index is negative");
                        pXSprite->sysData1++;
                        if (tx > 0) break;
                        continue;
                    }
                } else {
                    tx = pXSprite->sysData1;
                    if (pXSprite->sysData1 >= pXSprite->data4) {
                        pXSprite->sysData1 = pXSprite->data1;
                        break;
                    }
                    pXSprite->sysData1++;
                }
                break;
            }

            pXSprite->txID = (tx > 0 && tx < kChannelUserMax) ? tx : 0;
            SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1);
        }
        return true;
        case kMarkerWarpDest:
        case kModernSpriteDamager:
            if (pXSprite->txID <= 0) {
                PLAYER* pPlayer = getPlayerById(pXSprite->data1);
                if (pPlayer != NULL && SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1) == 1 && pXSprite->data1 > 0) {
                    switch (pSprite->type) {
                        case kMarkerWarpDest:
                            useTeleportTarget(pXSprite, pPlayer->pSprite);
                            break;
                        case kModernSpriteDamager:
                            useSpriteDamager(pXSprite, pPlayer->pSprite);
                            break;
                    }
                }
                return true;
            }
            modernTypeSetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1);
            return true;
        case kModernObjPropertiesChanger:
            if (pXSprite->txID <= 0) {
                if (SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1) == 1)
                    usePropertiesChanger(pXSprite, -1, -1);
                return true;
            }
            modernTypeSetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1);
            return true;
        case kModernObjSizeChanger:
        case kModernObjPicnumChanger:
        case kModernSectorFXChanger:
        case kModernObjDataChanger:
            modernTypeSetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1);
            return true;
        case kModernCustomDudeSpawn:
            if (gGameOptions.nMonsterSettings && genDudeSpawn(pSprite, -1) != NULL) gKillMgr.sub_263E0(1);
            return true;
        case kModernSeqSpawner:
        case kModernEffectSpawner:
            switch (event.cmd) {
                case kCmdOff:
                    if (pXSprite->state == 1) SetSpriteState(nSprite, pXSprite, 0);
                    break;
                case kCmdOn:
                    evKill(nSprite, 3); // queue overflow protect
                    if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
                    fallthrough__;
                case kCmdRepeat:
                    if (pXSprite->txID > 0) modernTypeSendCommand(nSprite, pXSprite->txID, (COMMAND_ID)pXSprite->command);
                    else if (pSprite->type == kModernSeqSpawner) useSeqSpawnerGen(pXSprite, 3, pSprite->index);
                    else useEffectGen(pXSprite, NULL);
            
                    if (pXSprite->busyTime > 0)
                        evPost(nSprite, 3, ClipLow((int(pXSprite->busyTime) + Random2(pXSprite->data1)) * 120 / 10, 0), kCmdRepeat);
                    break;
                default:
                    if (pXSprite->state == 0) evPost(nSprite, 3, 0, kCmdOn);
                    else evPost(nSprite, 3, 0, kCmdOff);
                    break;
            }
            return true;
        case kModernWindGenerator:
            switch (event.cmd) {
                case kCmdOff:
                    windGenStopWindOnSectors(pXSprite);
                    if (pXSprite->state == 1) SetSpriteState(nSprite, pXSprite, 0);
                    break;
                case kCmdOn:
                    evKill(nSprite, 3); // queue overflow protect
                    if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
                    fallthrough__;
                case kCmdRepeat:
                    if (pXSprite->txID > 0) modernTypeSendCommand(nSprite, pXSprite->txID, (COMMAND_ID)pXSprite->command);
                    else useSectorWindGen(pXSprite, NULL);

                    if (pXSprite->busyTime > 0) evPost(nSprite, 3, pXSprite->busyTime, kCmdRepeat);
                    break;
                default:
                    if (pXSprite->state == 0) evPost(nSprite, 3, 0, kCmdOn);
                    else evPost(nSprite, 3, 0, kCmdOff);
                    break;
            }
            return true;
        case kModernDudeTargetChanger:

            // this one is required if data4 of generator was dynamically changed
            // it turns monsters in normal idle state instead of genIdle, so they not ignore the world.
            if (pXSprite->dropMsg == 3 && 3 != pXSprite->data4)
                aiFightActivateDudes(pXSprite->txID);

            switch (event.cmd) {
                case kCmdOff:
                    if (pXSprite->data4 == 3) aiFightActivateDudes(pXSprite->txID);
                    if (pXSprite->state == 1) SetSpriteState(nSprite, pXSprite, 0);
                    break;
                case kCmdOn:
                    evKill(nSprite, 3); // queue overflow protect
                    if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
                    fallthrough__;
                case kCmdRepeat:
                    if (pXSprite->txID <= 0 || !aiFightGetDudesForBattle(pXSprite)) {
                        aiFightFreeAllTargets(pXSprite);
                        evPost(nSprite, 3, 0, kCmdOff);
                        break;
                    } else {
                        modernTypeSendCommand(nSprite, pXSprite->txID, (COMMAND_ID)pXSprite->command);
                    }

                    if (pXSprite->busyTime > 0) evPost(nSprite, 3, pXSprite->busyTime, kCmdRepeat);
                    break;
                default:
                    if (pXSprite->state == 0) evPost(nSprite, 3, 0, kCmdOn);
                    else evPost(nSprite, 3, 0, kCmdOff);
                    break;
            }
            pXSprite->dropMsg = pXSprite->data4;
            return true;
        case kModernObjDataAccumulator:
            switch (event.cmd) {
                case kCmdOff:
                    if (pXSprite->state == 1) SetSpriteState(nSprite, pXSprite, 0);
                    break;
                case kCmdOn:
                    evKill(nSprite, 3); // queue overflow protect
                    if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
                    fallthrough__;
                case kCmdRepeat:
                    // force OFF after *all* TX objects reach the goal value
                    if (pSprite->flags == kModernTypeFlag0 && incDecGoalValueIsReached(pXSprite)) {
                        evPost(nSprite, 3, 0, kCmdOff);
                        break;
                    }

                    if (pXSprite->txID > 0 && pXSprite->data1 > 0 && pXSprite->data1 <= 4) {
                        modernTypeSendCommand(nSprite, pXSprite->txID, (COMMAND_ID)pXSprite->command);
                        if (pXSprite->busyTime > 0) evPost(nSprite, 3, pXSprite->busyTime, kCmdRepeat);
                    }
                    break;
                default:
                    if (pXSprite->state == 0) evPost(nSprite, 3, 0, kCmdOn);
                    else evPost(nSprite, 3, 0, kCmdOff);
                    break;
            }
            return true;
        case kModernRandom:
        case kModernRandom2:
            switch (event.cmd) {
                case kCmdOff:
                    if (pXSprite->state == 1) SetSpriteState(nSprite, pXSprite, 0);
                    break;
                case kCmdOn:
                    evKill(nSprite, 3); // queue overflow protect
                    if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
                    fallthrough__;
                case kCmdRepeat:
                    useRandomItemGen(pSprite, pXSprite);
                    if (pXSprite->busyTime > 0)
                        evPost(nSprite, 3, (120 * pXSprite->busyTime) / 10, kCmdRepeat);
                    break;
                default:
                    if (pXSprite->state == 0) evPost(nSprite, 3, 0, kCmdOn);
                    else evPost(nSprite, 3, 0, kCmdOff);
                    break;
            }
            return true;
        case kModernThingTNTProx:
            if (pSprite->statnum != kStatRespawn) {
                switch (event.cmd) {
                case kCmdSpriteProximity:
                    if (pXSprite->state) break;
                    sfxPlay3DSound(pSprite, 452, 0, 0);
                    evPost(nSprite, 3, 30, kCmdOff);
                    pXSprite->state = 1;
                    fallthrough__;
                case kCmdOn:
                    sfxPlay3DSound(pSprite, 451, 0, 0);
                    pXSprite->Proximity = 1;
                    break;
                default:
                    actExplodeSprite(pSprite);
                    break;
                }
            }
            return true;
        case kModernThingEnemyLifeLeech:
            dudeLeechOperate(pSprite, pXSprite, event);
            return true;
        case kModernPlayerControl: { // WIP
            PLAYER* pPlayer = NULL; int nPlayer = pXSprite->data1; int oldCmd = -1;
            if ((pPlayer = getPlayerById(nPlayer)) == NULL || pPlayer->pXSprite->health <= 0)  return true;
            else if (pXSprite->command < kCmdNumberic + 3 && pXSprite->command > kCmdNumberic + 4
                && !modernTypeSetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1)) return true;

            TRPLAYERCTRL* pCtrl = &gPlayerCtrl[pPlayer->nPlayer];
            if (event.cmd >= kCmdNumberic) {
                switch (event.cmd) {
                    case kCmdNumberic + 3: // start playing qav scene
                        if (pCtrl->qavScene.index != nSprite || pXSprite->Interrutable)
                            trPlayerCtrlStartScene(pXSprite, pPlayer);
                        return true;
                    case kCmdNumberic + 4: { // stop playing qav scene
                        int scnIndex = pCtrl->qavScene.index;
                        if (spriRangeIsFine(scnIndex) && (scnIndex == nSprite || event.type != OBJ_SPRITE || sprite[event.index].type != kModernPlayerControl)) {
                            if (scnIndex != nSprite) pXSprite = &xsprite[sprite[scnIndex].extra];
                            trPlayerCtrlStopScene(pXSprite, pPlayer);
                        }
                        return true;
                    }
                    default:
                        oldCmd = pXSprite->command;
                        pXSprite->command = event.cmd; // convert event command to current sprite command
                        break;
                }
            }

            /// !!! COMMANDS OF THE CURRENT SPRITE, NOT OF THE EVENT !!! ///
            switch (pXSprite->command) {
                case kCmdLink: // copy properties of sprite to player
                    trPlayerCtrlLink(pXSprite, pPlayer);
                    break;
                case kCmdNumberic: // player life form
                    if (pXSprite->data2 < kModeHuman || pXSprite->data2 > kModeHumanShrink) break;
                    else trPlayerCtrlSetRace(pXSprite, pPlayer);
                    break;
                case kCmdNumberic + 1: // 65 (move speed and jump height)
                    // player movement speed (for all races and postures)
                    if (valueIsBetween(pXSprite->data2, -1, 32767))
                        trPlayerCtrlSetMoveSpeed(pXSprite, pPlayer);

                    // player jump height (for all races and stand posture only)
                    if (valueIsBetween(pXSprite->data3, -1, 32767))
                        trPlayerCtrlSetJumpHeight(pXSprite, pPlayer);
                    break;
                case kCmdNumberic + 2: // 66 (player screen effects)
                    if (pXSprite->data3 < 0) break;
                    else trPlayerCtrlSetScreenEffect(pXSprite, pPlayer);
                    break;
                case kCmdNumberic + 3: // 67 (start playing qav scene)
                    if (pCtrl->qavScene.index == nSprite && !pXSprite->Interrutable) break;
                    else trPlayerCtrlStartScene(pXSprite, pPlayer);
                    break;
                case kCmdNumberic + 4: // 68 (stop playing qav scene)
                    if (pCtrl->qavScene.index != nSprite) break;
                    else trPlayerCtrlStopScene(pXSprite, pPlayer);
                    break;
                case kCmdNumberic + 5: // 69 (set player sprite and look angles)
                    //data4 is reserved
                    if (pXSprite->data4 != 0) break;
                    
                    // look angle
                    if (valueIsBetween(pXSprite->data2, -128, 128))
                        trPlayerCtrlSetLookAngle(pXSprite, pPlayer);

                    // sprite angle (TO-DO: if tx > 0, take a look on TX ID sprite)
                    if (pSprite->flags & kModernTypeFlag1) pPlayer->q16ang = fix16_from_int(pSprite->ang);
                    else if (valueIsBetween(pXSprite->data3, -kAng360, kAng360))
                        pPlayer->q16ang = fix16_from_int(pXSprite->data3);

                    break;
                case kCmdNumberic + 6: // 70 (erase player stuff...)
                    if (pXSprite->data2 < 0) break;
                    else trPlayerCtrlEraseStuff(pXSprite, pPlayer);
                    break;
                case kCmdNumberic + 7: // 71 (give something to player...)
                    if (pXSprite->data2 <= 0) break;
                    else trPlayerCtrlGiveStuff(pXSprite, pPlayer, pCtrl);
                    break;
                case kCmdNumberic + 8: // 72 (use inventory item)
                    if (pXSprite->data2 < 1 || pXSprite->data2 > 5) break;
                    else trPlayerCtrlUsePackItem(pXSprite, pPlayer);
                    break;
            }
            if (oldCmd > -1) pXSprite->command = oldCmd;
        }
        return true;
        case kGenModernMissileUniversal:
        case kGenModernSound:
            switch (event.cmd) {
                case kCmdOff:
                    if (pXSprite->state == 1) SetSpriteState(nSprite, pXSprite, 0);
                    break;
                case kCmdOn:
                    evKill(nSprite, 3); // queue overflow protect
                    if (pXSprite->state == 0) SetSpriteState(nSprite, pXSprite, 1);
                    fallthrough__;
                case kCmdRepeat:
                    switch (pSprite->type) {
                        case kGenModernMissileUniversal:
                            useUniMissileGen(3, pSprite->extra);
                            break;
                        case kGenModernSound:
                            useSoundGen(pSprite, pXSprite);
                            break;
                    }
                    if (pXSprite->txID) evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command);
                    if (pXSprite->busyTime > 0) evPost(nSprite, 3, (120 * pXSprite->busyTime) / 10, kCmdRepeat);
                    break;
                default:
                    if (pXSprite->state == 0) evPost(nSprite, 3, 0, kCmdOn);
                    else evPost(nSprite, 3, 0, kCmdOff);
                    break;
            }
        return true;
    }
}

bool modernTypeLinkSprite(spritetype* pSprite, XSPRITE* pXSprite, EVENT event) {
    switch (pSprite->type) {
        // these can be linked too now, so it's possible to change palette, underwater status and more...
        case kMarkerLowWater:
        case kMarkerUpWater:
        case kMarkerUpGoo:
        case kMarkerLowGoo:
        case kMarkerUpLink:
        case kMarkerLowLink:
        case kMarkerUpStack:
        case kMarkerLowStack: {
            if (event.type != OBJ_SPRITE) break;
            spritetype* pSprite2 = &sprite[event.index];
            if (pSprite2->extra < 0) break;
            XSPRITE* pXSprite2 = &xsprite[pSprite2->extra];

            // Only lower to lower and upper to upper linking allowed.
            switch (pSprite->type) {
                case kMarkerLowWater:
                case kMarkerLowLink:
                case kMarkerLowStack:
                case kMarkerLowGoo:
                    switch (pSprite2->type) {
                        case kMarkerLowWater:
                        case kMarkerLowLink:
                        case kMarkerLowStack:
                        case kMarkerLowGoo:
                            break;
                        default:
                            return true;
                    }
                    break;
                case kMarkerUpWater:
                case kMarkerUpLink:
                case kMarkerUpStack:
                case kMarkerUpGoo:
                    switch (pSprite2->type) {
                        case kMarkerUpWater:
                        case kMarkerUpLink:
                        case kMarkerUpStack:
                        case kMarkerUpGoo:
                            break;
                        default:
                            return true;
                    }
                    break;
            }

            // swap link location
            /*short tmp1 = pXSprite2.data1;*/
            /*pXSprite2.data1 = pXSprite.data1;*/
            /*pXSprite.data1 = tmp1;*/

            if (pXSprite->data2 < kMaxPAL)
            {
                // swap medium
                int tmp2 = pXSprite2->data2;
                pXSprite2->data2 = pXSprite->data2;
                pXSprite->data2 = tmp2;
            }


            // swap link type                       // swap link owners (sectors)
            short tmp3 = pSprite2->type;			//short tmp7 = pSprite2.owner;
            pSprite2->type = pSprite->type;			//pSprite2.owner = pSprite.owner;
            pSprite->type = tmp3;					//pSprite.owner = tmp7;

            // Deal with linked sectors
            sectortype* pSector = &sector[pSprite->sectnum];
            sectortype* pSector2 = &sector[pSprite2->sectnum];

            // Check for underwater
            XSECTOR* pXSector = NULL;	XSECTOR* pXSector2 = NULL;
            if (pSector->extra > 0) pXSector = &xsector[pSector->extra];
            if (pSector2->extra > 0) pXSector2 = &xsector[pSector2->extra];
            if (pXSector != NULL && pXSector2 != NULL) {
                bool tmp6 = pXSector->Underwater;
                pXSector->Underwater = pXSector2->Underwater;
                pXSector2->Underwater = tmp6;
            }

            // optionally swap floorpic
            if (pXSprite2->data3 == 1) {
                short tmp4 = pSector->floorpicnum;
                pSector->floorpicnum = pSector2->floorpicnum;
                pSector2->floorpicnum = tmp4;
            }

            // optionally swap ceilpic
            if (pXSprite2->data4 == 1) {
                short tmp5 = pSector->ceilingpicnum;
                pSector->ceilingpicnum = pSector2->ceilingpicnum;
                pSector2->ceilingpicnum = tmp5;
            }
        }
        return true;
        // add a way to link between path markers, so path sectors can change their path on the fly.
        case kMarkerPath:
            // only path marker to path marker link allowed
            if (event.type == OBJ_SPRITE) {
                int nXSprite2 = sprite[event.index].extra;
                // get master path marker data fields
                pXSprite->data1 = xsprite[nXSprite2].data1;
                pXSprite->data2 = xsprite[nXSprite2].data2;
                pXSprite->data3 = xsprite[nXSprite2].data3; // include soundId(?)

                // get master path marker busy and wait times
                pXSprite->busyTime = xsprite[nXSprite2].busyTime;
                pXSprite->waitTime = xsprite[nXSprite2].waitTime;

            }
            return true;
    }

    return false;
}

bool modernTypeOperateWall(int nWall, walltype* pWall, XWALL* pXWall, EVENT event) {
    
    switch (pWall->type) {
        case kSwitchOneWay:
            switch (event.cmd) {
                case kCmdOff:
                    SetWallState(nWall, pXWall, 0);
                    break;
                case kCmdOn:
                    SetWallState(nWall, pXWall, 1);
                    break;
                default:
                    SetWallState(nWall, pXWall, pXWall->restState ^ 1);
                    break;
            }
            return true;
        default:
            return false; // no modern type found to work with, go normal OperateWall();
    }
    
}

void useRandomItemGen(spritetype* pSource, XSPRITE* pXSource) {
    // let's first search for previously dropped items and remove it
    if (pXSource->dropMsg > 0) {
        for (short nItem = headspritestat[kStatItem]; nItem >= 0; nItem = nextspritestat[nItem]) {
            spritetype* pItem = &sprite[nItem];
            if ((unsigned int)pItem->type == pXSource->dropMsg && pItem->x == pSource->x && pItem->y == pSource->y && pItem->z == pSource->z) {
                gFX.fxSpawn((FX_ID)29, pSource->sectnum, pSource->x, pSource->y, pSource->z, 0);
                deletesprite(nItem);
                break;
            }
        }
    }

    // then drop item
    spritetype* pDrop = randomDropPickupObject(pSource, pXSource->dropMsg);

    // check if generator affected by physics
    if (pDrop != NULL && debrisGetIndex(pSource->index) != -1 && (pDrop->extra >= 0 || dbInsertXSprite(pDrop->index) > 0)) {
        int nIndex = debrisGetFreeIndex();
        if (nIndex >= 0) {
            xsprite[pDrop->extra].physAttr |= kPhysMove | kPhysGravity | kPhysFalling; // must fall always
            pSource->cstat &= ~CSTAT_SPRITE_BLOCK;

            gPhysSpritesList[nIndex] = pDrop->index;
            if (nIndex >= gPhysSpritesCount) gPhysSpritesCount++;
            getSpriteMassBySize(pDrop); // create mass cache
        }
    }
}

void useUniMissileGen(int, int nXSprite) {

    XSPRITE* pXSprite = &xsprite[nXSprite]; int dx = 0, dy = 0, dz = 0;
    spritetype* pSprite = &sprite[pXSprite->reference];

    if (pXSprite->data1 < kMissileBase || pXSprite->data1 >= kMissileMax)
        return;

    if (pSprite->cstat & 32) {
        if (pSprite->cstat & 8) dz = 0x4000;
        else dz = -0x4000;
    } else {
        dx = Cos(pSprite->ang) >> 16;
        dy = Sin(pSprite->ang) >> 16;
        dz = pXSprite->data3 << 6; // add slope controlling
        if (dz > 0x10000) dz = 0x10000;
        else if (dz < -0x10000) dz = -0x10000;
    }

    spritetype* pMissile = NULL;
    pMissile = actFireMissile(pSprite, 0, 0, dx, dy, dz, pXSprite->data1);
    if (pMissile != NULL) {

        // inherit some properties of the generator
        if (pSprite->flags & kModernTypeFlag1) {

            pMissile->xrepeat = pSprite->xrepeat;
            pMissile->yrepeat = pSprite->yrepeat;

            pMissile->pal = pSprite->pal;
            pMissile->shade = pSprite->shade;

        }

        // add velocity controlling
        if (pXSprite->data2 > 0) {

            int velocity = pXSprite->data2 << 12;
            xvel[pMissile->index] = mulscale(velocity, dx, 14);
            yvel[pMissile->index] = mulscale(velocity, dy, 14);
            zvel[pMissile->index] = mulscale(velocity, dz, 14);

        }

        // add bursting for missiles
        if (pMissile->type != kMissileFlareAlt && pXSprite->data4 > 0)
            evPost(pMissile->index, 3, (pXSprite->data4 > 500) ? 500 : pXSprite->data4 - 1, kCallbackMissileBurst);

    }

}

void useSoundGen(spritetype* pSource, XSPRITE* pXSource) {
    int pitch = pXSource->data4 << 1; if (pitch < 2000) pitch = 0;
    sfxPlay3DSoundCP(pSource, pXSource->data2, -1, 0, pitch, pXSource->data3);
}

void useIncDecGen(XSPRITE* pXSource, short objType, int objIndex) {
    int data = getDataFieldOfObject(objType, objIndex, pXSource->data1);
    if (data == -65535) return;
    
    spritetype* pSource = &sprite[pXSource->reference];
    if (pXSource->data2 < pXSource->data3) {

        if (data < pXSource->data2) data = pXSource->data2;
        if (data > pXSource->data3) data = pXSource->data3;

        if ((data += pXSource->data4) >= pXSource->data3) {
            switch (pSource->flags) {
                case kModernTypeFlag0:
                case kModernTypeFlag1:
                    if (data > pXSource->data3) data = pXSource->data3;
                    break;
                case kModernTypeFlag2: {
                    if (data > pXSource->data3) data = pXSource->data3;
                    if (!incDecGoalValueIsReached(pXSource)) break;
                    short tmp = pXSource->data3;
                    pXSource->data3 = pXSource->data2;
                    pXSource->data2 = tmp;
                }
                    break;
                case kModernTypeFlag3:
                    if (data > pXSource->data3) data = pXSource->data2;
                    break;
            }
        }

    } else if (pXSource->data2 > pXSource->data3) {

        if (data > pXSource->data2) data = pXSource->data2;
        if (data < pXSource->data3) data = pXSource->data3;

        if ((data -= pXSource->data4) <= pXSource->data3) {
            switch (pSource->flags) {
                case kModernTypeFlag0:
                case kModernTypeFlag1:
                    if (data < pXSource->data3) data = pXSource->data3;
                    break;
                case kModernTypeFlag2: {
                    if (data < pXSource->data3) data = pXSource->data3;
                    if (!incDecGoalValueIsReached(pXSource)) break;
                    short tmp = pXSource->data3;
                    pXSource->data3 = pXSource->data2;
                    pXSource->data2 = tmp;
                }
                    break;
                case kModernTypeFlag3:
                    if (data < pXSource->data3) data = pXSource->data2;
                    break;
            }
        }
    }

    setDataValueOfObject(objType, objIndex, pXSource->data1, data);

}

void useDataChanger(XSPRITE* pXSource, int objType, int objIndex) {
    
    spritetype* pSource = &sprite[pXSource->reference];
    switch (objType) {
        case OBJ_SECTOR:
            if ((pSource->flags & kModernTypeFlag1) || (pXSource->data1 != -1 && pXSource->data1 != 32767))
                setDataValueOfObject(objType, objIndex, 1, pXSource->data1);
            break;
        case OBJ_SPRITE:
            if ((pSource->flags & kModernTypeFlag1) || (pXSource->data1 != -1 && pXSource->data1 != 32767))
                setDataValueOfObject(objType, objIndex, 1, pXSource->data1);

            if ((pSource->flags & kModernTypeFlag1) || (pXSource->data2 != -1 && pXSource->data2 != 32767))
                setDataValueOfObject(objType, objIndex, 2, pXSource->data2);

            if ((pSource->flags & kModernTypeFlag1) || (pXSource->data3 != -1 && pXSource->data3 != 32767))
                setDataValueOfObject(objType, objIndex, 3, pXSource->data3);

            if ((pSource->flags & kModernTypeFlag1) || pXSource->data4 != 65535)
                setDataValueOfObject(objType, objIndex, 4, pXSource->data4);
            break;
        case OBJ_WALL:
            if ((pSource->flags & kModernTypeFlag1) || (pXSource->data1 != -1 && pXSource->data1 != 32767))
                setDataValueOfObject(objType, objIndex, 1, pXSource->data1);
            break;
    }
}

void useSectorLigthChanger(XSPRITE* pXSource, XSECTOR* pXSector) {
    
    spritetype* pSource = &sprite[pXSource->reference];
    if (valueIsBetween(pXSource->data1, -1, 32767))
        pXSector->wave = (pXSource->data1 > 11) ? 11 : pXSource->data1;

    int oldAmplitude = pXSector->amplitude;
    if (pXSource->data2 >= 0) pXSector->amplitude = (pXSource->data2 > 127) ? 127 : pXSource->data2;
    else if (pXSource->data2 < -1) pXSector->amplitude = (pXSource->data2 < -127) ? -127 : pXSource->data2;

    if (valueIsBetween(pXSource->data3, -1, 32767))
        pXSector->freq = (pXSource->data3 > 255) ? 255 : pXSource->data3;

    if (valueIsBetween(pXSource->data4, -1, 65535))
        pXSector->phase = (pXSource->data4 > 255) ? 255 : pXSource->data4;

    // force shadeAlways
    if (pSource->flags & kModernTypeFlag1)
        pXSector->shadeAlways = true;

    // add to shadeList if amplitude was set to 0 previously
    if (oldAmplitude == 0 && pXSector->amplitude != 0 && shadeCount < kMaxXSectors) {

        bool found = false;
        for (int i = 0; i < shadeCount; i++) {
            if (shadeList[i] != sector[pXSector->reference].extra) continue;
            found = true;
            break;
        }

        if (!found)
            shadeList[shadeCount++] = sector[pXSector->reference].extra;
    }
}

void useTargetChanger(XSPRITE* pXSource, spritetype* pSprite) {
    
    
    if (!IsDudeSprite(pSprite) || pSprite->statnum != kStatDude) {
        switch (pSprite->type) { // can be dead dude turned in gib
            // make current target and all other dudes not attack this dude anymore
        case kThingBloodBits:
        case kThingBloodChunks:
            aiFightFreeTargets(pSprite->index);
            return;
        default:
            return;
        }
    }
    
    
    spritetype* pSource = &sprite[pXSource->reference]; XSPRITE* pXSprite = &xsprite[pSprite->extra];
    spritetype* pTarget = NULL; XSPRITE* pXTarget = NULL; int receiveHp = 33 + Random(33);
    DUDEINFO* pDudeInfo = getDudeInfo(pSprite->type); int matesPerEnemy = 1;

    // dude is burning?
    if (pXSprite->burnTime > 0 && spriRangeIsFine(pXSprite->burnSource)) {

        if (IsBurningDude(pSprite)) return;
        else {
            spritetype* pBurnSource = &sprite[pXSprite->burnSource];
            if (pBurnSource->extra >= 0) {
                if (pXSource->data2 == 1 && aiFightIsMateOf(pXSprite, &xsprite[pBurnSource->extra])) {
                    pXSprite->burnTime = 0;

                    // heal dude a bit in case of friendly fire
                    if (pXSprite->data4 > 0 && pXSprite->health < pXSprite->data4)
                        actHealDude(pXSprite, receiveHp, pXSprite->data4);
                    else if (pXSprite->health < pDudeInfo->startHealth)
                        actHealDude(pXSprite, receiveHp, pDudeInfo->startHealth);
                } else if (xsprite[pBurnSource->extra].health <= 0) {
                    pXSprite->burnTime = 0;
                }
            }
        }
    }

    spritetype* pPlayer = aiFightTargetIsPlayer(pXSprite);
    // special handling for player(s) if target changer data4 > 2.
    if (pPlayer != NULL) {
        if (pXSource->data4 == 3) {
            aiSetTarget(pXSprite, pSprite->x, pSprite->y, pSprite->z);
            aiSetGenIdleState(pSprite, pXSprite);
            if (pSprite->type == kDudeModernCustom && leechIsDropped(pSprite))
                removeLeech(leechIsDropped(pSprite));
        } else if (pXSource->data4 == 4) {
            aiSetTarget(pXSprite, pPlayer->x, pPlayer->y, pPlayer->z);
            if (pSprite->type == kDudeModernCustom && leechIsDropped(pSprite))
                removeLeech(leechIsDropped(pSprite));
        }
    }

    int maxAlarmDudes = 8 + Random(8);
    if (pXSprite->target > -1 && sprite[pXSprite->target].extra > -1 && pPlayer == NULL) {
        pTarget = &sprite[pXSprite->target]; pXTarget = &xsprite[pTarget->extra];

        if (aiFightUnitCanFly(pSprite) && aiFightIsMeleeUnit(pTarget) && !aiFightUnitCanFly(pTarget))
            pSprite->flags |= 0x0002;
        else if (aiFightUnitCanFly(pSprite))
            pSprite->flags &= ~0x0002;

        if (!IsDudeSprite(pTarget) || pXTarget->health < 1 || !aiFightDudeCanSeeTarget(pXSprite, pDudeInfo, pTarget)) {
            aiSetTarget(pXSprite, pSprite->x, pSprite->y, pSprite->z);
        }
        // dude attack or attacked by target that does not fit by data id?
        else if (pXSource->data1 != 666 && pXTarget->data1 != pXSource->data1) {
            if (aiFightDudeIsAffected(pXTarget)) {

                // force stop attack target
                aiSetTarget(pXSprite, pSprite->x, pSprite->y, pSprite->z);
                if (pXSprite->burnSource == pTarget->index) {
                    pXSprite->burnTime = 0;
                    pXSprite->burnSource = -1;
                }

                // force stop attack dude
                aiSetTarget(pXTarget, pTarget->x, pTarget->y, pTarget->z);
                if (pXTarget->burnSource == pSprite->index) {
                    pXTarget->burnTime = 0;
                    pXTarget->burnSource = -1;
                }
            }

        }
        // instantly kill annoying spiders, rats, hands etc if dude is big enough
        else if (aiFightIsAnnoyingUnit(pTarget) && !aiFightIsAnnoyingUnit(pSprite) && tilesiz[pSprite->picnum].y >= 60 &&
            aiFightGetTargetDist(pSprite, pDudeInfo, pTarget) < 2) {

            actKillDude(pSource->index, pTarget, DAMAGE_TYPE_0, 65535);
            aiSetTarget(pXSprite, pSprite->x, pSprite->y, pSprite->z);

        } else if (pXSource->data2 == 1 && aiFightIsMateOf(pXSprite, pXTarget)) {
            spritetype* pMate = pTarget; XSPRITE* pXMate = pXTarget;

            // heal dude
            if (pXSprite->data4 > 0 && pXSprite->health < pXSprite->data4)
                actHealDude(pXSprite, receiveHp, pXSprite->data4);
            else if (pXSprite->health < pDudeInfo->startHealth)
                actHealDude(pXSprite, receiveHp, pDudeInfo->startHealth);

            // heal mate
            if (pXMate->data4 > 0 && pXMate->health < pXMate->data4)
                actHealDude(pXMate, receiveHp, pXMate->data4);
            else {
                DUDEINFO* pTDudeInfo = getDudeInfo(pMate->type);
                if (pXMate->health < pTDudeInfo->startHealth)
                    actHealDude(pXMate, receiveHp, pTDudeInfo->startHealth);
            }

            if (pXMate->target > -1 && sprite[pXMate->target].extra >= 0) {
                pTarget = &sprite[pXMate->target];
                // force mate stop attack dude, if he does
                if (pXMate->target == pSprite->index) {
                    aiSetTarget(pXMate, pMate->x, pMate->y, pMate->z);
                } else if (!aiFightIsMateOf(pXSprite, &xsprite[pTarget->extra])) {
                    // force dude to attack same target that mate have
                    aiSetTarget(pXSprite, pTarget->index);
                    return;

                } else {
                    // force mate to stop attack another mate
                    aiSetTarget(pXMate, pMate->x, pMate->y, pMate->z);
                }
            }

            // force dude stop attack mate, if target was not changed previously
            if (pXSprite->target == pMate->index)
                aiSetTarget(pXSprite, pSprite->x, pSprite->y, pSprite->z);


        }
        // check if targets aims player then force this target to fight with dude
        else if (aiFightTargetIsPlayer(pXTarget) != NULL) {
            aiSetTarget(pXTarget, pSprite->index);
        }

        int mDist = 3; if (aiFightIsMeleeUnit(pSprite)) mDist = 2;
        if (pXSprite->target >= 0 && aiFightGetTargetDist(pSprite, pDudeInfo, &sprite[pXSprite->target]) < mDist) {
            if (!isActive(pSprite->index)) aiActivateDude(pSprite, pXSprite);
            return;
        }
        // lets try to look for target that fits better by distance
        else if (((int)gFrameClock & 256) != 0 && (pXSprite->target < 0 || aiFightGetTargetDist(pSprite, pDudeInfo, pTarget) >= mDist)) {
            pTarget = aiFightGetTargetInRange(pSprite, 0, mDist, pXSource->data1, pXSource->data2);
            if (pTarget != NULL) {
                pXTarget = &xsprite[pTarget->extra];

                // Make prev target not aim in dude
                if (pXSprite->target > -1) {
                    spritetype* prvTarget = &sprite[pXSprite->target];
                    aiSetTarget(&xsprite[prvTarget->extra], prvTarget->x, prvTarget->y, prvTarget->z);
                    if (!isActive(pTarget->index))
                        aiActivateDude(pTarget, pXTarget);
                }

                // Change target for dude
                aiSetTarget(pXSprite, pTarget->index);
                if (!isActive(pSprite->index))
                    aiActivateDude(pSprite, pXSprite);

                // ...and change target of target to dude to force it fight
                if (pXSource->data3 > 0 && pXTarget->target != pSprite->index) {
                    aiSetTarget(pXTarget, pSprite->index);
                    if (!isActive(pTarget->index))
                        aiActivateDude(pTarget, pXTarget);
                }
                return;
            }
        }
    }

    if ((pXSprite->target < 0 || pPlayer != NULL) && ((int)gFrameClock & 32) != 0) {
        // try find first target that dude can see
        for (int nSprite = headspritestat[kStatDude]; nSprite >= 0; nSprite = nextspritestat[nSprite]) {
            
            pTarget = &sprite[nSprite]; pXTarget = &xsprite[pTarget->extra];

            if (pXTarget->target == pSprite->index) {
                aiSetTarget(pXSprite, pTarget->index);
                return;
            }

            // skip non-dudes and players
            if (!IsDudeSprite(pTarget) || (IsPlayerSprite(pTarget) && pXSource->data4 > 0) || pTarget->owner == pSprite->index) continue;
            // avoid self aiming, those who dude can't see, and those who dude own
            else if (!aiFightDudeCanSeeTarget(pXSprite, pDudeInfo, pTarget) || pSprite->index == pTarget->index) continue;
            // if Target Changer have data1 = 666, everyone can be target, except AI team mates.
            else if (pXSource->data1 != 666 && pXSource->data1 != pXTarget->data1) continue;
            // don't attack immortal, burning dudes and mates
            if (IsBurningDude(pTarget) || !IsKillableDude(pTarget) || (pXSource->data2 == 1 && aiFightIsMateOf(pXSprite, pXTarget)))
                continue;

            if (pXSource->data2 == 0 || (pXSource->data2 == 1 && !aiFightMatesHaveSameTarget(pXSprite, pTarget, matesPerEnemy))) {

                // Change target for dude
                aiSetTarget(pXSprite, pTarget->index);
                if (!isActive(pSprite->index))
                    aiActivateDude(pSprite, pXSprite);

                // ...and change target of target to dude to force it fight
                if (pXSource->data3 > 0 && pXTarget->target != pSprite->index) {
                    aiSetTarget(pXTarget, pSprite->index);
                    if (!isActive(pTarget->index))
                        aiActivateDude(pTarget, pXTarget);

                    if (pXSource->data3 == 2)
                        aiFightAlarmDudesInSight(pTarget, maxAlarmDudes);
                }
                
                return;
            }
            
            break;
        }
    }

    // got no target - let's ask mates if they have targets
    if ((pXSprite->target < 0 || pPlayer != NULL) && pXSource->data2 == 1 && ((int)gFrameClock & 64) != 0) {
        spritetype* pMateTarget = NULL;
        if ((pMateTarget = aiFightGetMateTargets(pXSprite)) != NULL && pMateTarget->extra > 0) {
            XSPRITE* pXMateTarget = &xsprite[pMateTarget->extra];
            if (aiFightDudeCanSeeTarget(pXSprite, pDudeInfo, pMateTarget)) {
                if (pXMateTarget->target < 0) {
                    aiSetTarget(pXMateTarget, pSprite->index);
                    if (IsDudeSprite(pMateTarget) && !isActive(pMateTarget->index))
                        aiActivateDude(pMateTarget, pXMateTarget);
                }

                aiSetTarget(pXSprite, pMateTarget->index);
                if (!isActive(pSprite->index))
                    aiActivateDude(pSprite, pXSprite);
                return;

                // try walk in mate direction in case if not see the target
            } else if (pXMateTarget->target >= 0 && aiFightDudeCanSeeTarget(pXSprite, pDudeInfo, &sprite[pXMateTarget->target])) {
                spritetype* pMate = &sprite[pXMateTarget->target];
                pXSprite->target = pMateTarget->index;
                pXSprite->targetX = pMate->x;
                pXSprite->targetY = pMate->y;
                pXSprite->targetZ = pMate->z;
                if (!isActive(pSprite->index))
                    aiActivateDude(pSprite, pXSprite);
                return;
            }
        }
    }
}

void usePictureChanger(XSPRITE* pXSource, int objType, int objIndex) {
    
    spritetype* pSource = &sprite[pXSource->reference];
    switch (objType) {
        case OBJ_SECTOR: {
            if (valueIsBetween(pXSource->data1, -1, 32767))
                sector[objIndex].floorpicnum = pXSource->data1;

            if (valueIsBetween(pXSource->data2, -1, 32767))
                sector[objIndex].ceilingpicnum = pXSource->data2;

            XSECTOR* pXSector = &xsector[sector[objIndex].extra];
            if (valueIsBetween(pXSource->data3, -1, 32767)) {
                sector[objIndex].floorpal = pXSource->data3;
                if (pSource->flags & kModernTypeFlag1)
                    pXSector->floorpal = pXSource->data3;
            }

            if (valueIsBetween(pXSource->data4, -1, 65535)) {
                sector[objIndex].ceilingpal = pXSource->data4;
                if (pSource->flags & kModernTypeFlag1)
                    pXSector->ceilpal = pXSource->data4;
            }
            break;
        }
        case OBJ_SPRITE:
            if (valueIsBetween(pXSource->data1, -1, 32767))
                sprite[objIndex].picnum = pXSource->data1;

            if (pXSource->data2 >= 0) sprite[objIndex].shade = (pXSource->data2 > 127) ? 127 : pXSource->data2;
            else if (pXSource->data2 < -1) sprite[objIndex].shade = (pXSource->data2 < -127) ? -127 : pXSource->data2;

            if (valueIsBetween(pXSource->data3, -1, 32767))
                sprite[objIndex].pal = pXSource->data3;
            break;
        case OBJ_WALL:
            if (valueIsBetween(pXSource->data1, -1, 32767))
                wall[objIndex].picnum = pXSource->data1;

            if (valueIsBetween(pXSource->data2, -1, 32767))
                wall[objIndex].overpicnum = pXSource->data2;

            if (valueIsBetween(pXSource->data3, -1, 32767))
                wall[objIndex].pal = pXSource->data3;
            break;
        }
}

//---------------------------------------

// player related
QAV* playerQavSceneLoad(int qavId) {
    QAV* pQav = NULL; DICTNODE* hQav = gSysRes.Lookup(qavId, "QAV");

    if (hQav) pQav = (QAV*)gSysRes.Lock(hQav);
    else viewSetSystemMessage("Failed to load QAV animation #%d", qavId);

    return pQav;
}

void playerQavSceneProcess(PLAYER* pPlayer, QAVSCENE* pQavScene) {
    int nIndex = pQavScene->index;
    if (sprite[nIndex].extra >= 0) {
        XSPRITE* pXSprite = &xsprite[sprite[nIndex].extra];
        if (pXSprite->waitTime > 0 && --pXSprite->sysData1 <= 0) {
            if (pXSprite->txID > 0)
                evSend(nIndex, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command);
            if (pXSprite->locked) trPlayerCtrlStopScene(pXSprite, pPlayer);
            else evPost(nIndex, 3, 0, (COMMAND_ID)(kCmdNumberic + 4));
        } else {
            playerQavScenePlay(pPlayer);
            pPlayer->weaponTimer = ClipLow(pPlayer->weaponTimer -= 4, 0);
        }
    } else {
        pQavScene->index = pPlayer->sceneQav = -1;
        pQavScene->qavResrc = NULL;
    }
}

void playerQavSceneDraw(PLAYER* pPlayer, int a2, int a3, int a4, int a5) {
    if (pPlayer == NULL || pPlayer->sceneQav == -1) return;

    QAVSCENE* pQavScene = &gPlayerCtrl[pPlayer->nPlayer].qavScene;
    spritetype* pSprite = &sprite[pQavScene->index];

    if (pQavScene->qavResrc != NULL) {

        QAV* pQAV = pQavScene->qavResrc;
        int v4 = (pPlayer->weaponTimer == 0) ? (int)totalclock % pQAV->at10 : pQAV->at10 - pPlayer->weaponTimer;

        int flags = 2; int nInv = powerupCheck(pPlayer, kPwUpShadowCloak);
        if (nInv >= 120 * 8 || (nInv != 0 && ((int)totalclock & 32))) {
            a2 = -128; flags |= 1;
        }

        // draw as weapon
        if (!(pSprite->flags & kModernTypeFlag1)) {

            pQAV->x = a3; pQAV->y = a4;
            pQAV->Draw(v4, flags, a2, a5);

            // draw fullscreen (currently 4:3 only)
        } else {

            int wx1 = windowxy1.x, wy1 = windowxy1.y, wx2 = windowxy2.x, wy2 = windowxy2.y;

            windowxy2.x = xdim - 1; windowxy2.y = ydim - 1;
            windowxy1.x = windowxy1.y = 0;

            pQAV->Draw(v4, flags, a2, a5);

            windowxy1.x = wx1; windowxy1.y = wy1;
            windowxy2.x = wx2; windowxy2.y = wy2;

        }

    }

}

void playerQavScenePlay(PLAYER* pPlayer) {
    if (pPlayer == NULL || pPlayer->sceneQav == -1) return;

    QAVSCENE* pQavScene = &gPlayerCtrl[pPlayer->nPlayer].qavScene;
    if (pQavScene->qavResrc != NULL) {
        QAV* pQAV = pQavScene->qavResrc;
        pQAV->nSprite = pPlayer->pSprite->index;
        int nTicks = pQAV->at10 - pPlayer->weaponTimer;
        pQAV->Play(nTicks - 4, nTicks, pPlayer->qavCallback, pPlayer);
    }
}

void playerQavSceneReset(PLAYER* pPlayer) {
    QAVSCENE* pQavScene = &gPlayerCtrl[pPlayer->nPlayer].qavScene;
    pQavScene->index = pQavScene->dummy = pPlayer->sceneQav = -1;
    pQavScene->qavResrc = NULL;
}

bool playerSizeShrink(PLAYER* pPlayer, int divider) {
    pPlayer->pXSprite->scale = 256 / divider;
    playerSetRace(pPlayer, kModeHumanShrink);
    return true;
}

bool playerSizeGrow(PLAYER* pPlayer, int multiplier) {
    pPlayer->pXSprite->scale = 256 * multiplier;
    playerSetRace(pPlayer, kModeHumanGrown);
    return true;
}

bool playerSizeReset(PLAYER* pPlayer) {
    playerSetRace(pPlayer, kModeHuman);
    pPlayer->pXSprite->scale = 0;
    return true;
}

void playerDeactivateShrooms(PLAYER* pPlayer) {
    powerupDeactivate(pPlayer, kPwUpGrowShroom);
    pPlayer->pwUpTime[kPwUpGrowShroom] = 0;

    powerupDeactivate(pPlayer, kPwUpShrinkShroom);
    pPlayer->pwUpTime[kPwUpShrinkShroom] = 0;
}



PLAYER* getPlayerById(short id) {

    // relative to connected players
    if (id >= 1 && id <= kMaxPlayers) {
        id = id - 1;
        for (int i = connecthead; i >= 0; i = connectpoint2[i]) {
            if (id == gPlayer[i].nPlayer)
                return &gPlayer[i];
        }

    // absolute sprite type
    } else if (id >= kDudePlayer1 && id <= kDudePlayer8) {
        for (int i = connecthead; i >= 0; i = connectpoint2[i]) {
            if (id == gPlayer[i].pSprite->type)
                return &gPlayer[i];
        }
    }

    //viewSetSystemMessage("There is no player id #%d", id);
    return NULL;
}

// misc functions
bool IsBurningDude(spritetype* pSprite) {
    if (pSprite == NULL) return false;
    switch (pSprite->type) {
    case kDudeBurningInnocent:
    case kDudeBurningCultist:
    case kDudeBurningZombieAxe:
    case kDudeBurningZombieButcher:
    case kDudeBurningTinyCaleb:
    case kDudeBurningBeast:
    case kDudeModernCustomBurning:
        return true;
    }

    return false;
}

bool IsKillableDude(spritetype* pSprite) {
    switch (pSprite->type) {
    case kDudeGargoyleStatueFlesh:
    case kDudeGargoyleStatueStone:
        return false;
    default:
        if (!IsDudeSprite(pSprite) || xsprite[pSprite->extra].locked == 1) return false;
        return true;
    }
}

bool isGrown(spritetype* pSprite) {
    if (powerupCheck(&gPlayer[pSprite->type - kDudePlayer1], kPwUpGrowShroom) > 0) return true;
    else if (pSprite->extra >= 0 && xsprite[pSprite->extra].scale >= 512) return true;
    else return false;
}

bool isShrinked(spritetype* pSprite) {
    if (powerupCheck(&gPlayer[pSprite->type - kDudePlayer1], kPwUpShrinkShroom) > 0) return true;
    else if (pSprite->extra >= 0 && xsprite[pSprite->extra].scale > 0 && xsprite[pSprite->extra].scale <= 128) return true;
    else return false;
}

bool isActive(int nSprite) {
    if (sprite[nSprite].extra < 0 || sprite[nSprite].extra >= kMaxXSprites)
        return false;

    XSPRITE* pXDude = &xsprite[sprite[nSprite].extra];
    switch (pXDude->aiState->stateType) {
    case kAiStateIdle:
    case kAiStateGenIdle:
    case kAiStateSearch:
    case kAiStateMove:
    case kAiStateOther:
        return false;
    default:
        return true;
    }
}

int getDataFieldOfObject(int objType, int objIndex, int dataIndex) {
    int data = -65535;
    switch (objType) {
    case OBJ_SPRITE:
        switch (dataIndex) {
        case 1:
            return xsprite[sprite[objIndex].extra].data1;
        case 2:
            return xsprite[sprite[objIndex].extra].data2;
        case 3:
            switch (sprite[objIndex].type) {
            case kDudeModernCustom:
                return xsprite[sprite[objIndex].extra].sysData1;
            default:
                return xsprite[sprite[objIndex].extra].data3;
            }
        case 4:
            return xsprite[sprite[objIndex].extra].data4;
        default:
            return data;
        }
    case OBJ_SECTOR:
        return xsector[sector[objIndex].extra].data;
    case OBJ_WALL:
        return xwall[wall[objIndex].extra].data;
    default:
        return data;
    }
}

bool setDataValueOfObject(int objType, int objIndex, int dataIndex, int value) {
    switch (objType) {
        case OBJ_SPRITE: {
            XSPRITE* pXSprite = &xsprite[sprite[objIndex].extra];

            // exceptions
            if (IsDudeSprite(&sprite[objIndex]) && pXSprite->health <= 0) return true;
            /*switch (sprite[objIndex].type) {
                case kThingBloodBits:
                case kThingBloodChunks:
                case kThingZombieHead:
                case kThingObjectGib:
                case kThingObjectExplode:
                    if (pXSprite->data1 > 0 || pXSprite->data2 > 0 || pXSprite->data3 > 0 || pXSprite->data4 > 0) return true;
                    break;
            }*/

            switch (dataIndex) {
                case 1:
                    xsprite[sprite[objIndex].extra].data1 = value;
                    switch (sprite[objIndex].type) {
                    case kSwitchCombo:
                        if (value == xsprite[sprite[objIndex].extra].data2) SetSpriteState(objIndex, &xsprite[sprite[objIndex].extra], 1);
                        else SetSpriteState(objIndex, &xsprite[sprite[objIndex].extra], 0);
                        break;
                    case kDudeModernCustom:
                    case kDudeModernCustomBurning:
                        gGenDudeExtra[objIndex].updReq[kGenDudePropertyWeapon] = true;
                        gGenDudeExtra[objIndex].updReq[kGenDudePropertyDmgScale] = true;
                        evPost(objIndex, 3, kGenDudeUpdTimeRate, kCallbackGenDudeUpdate);
                        break;
                    }
                    return true;
                case 2:
                    xsprite[sprite[objIndex].extra].data2 = value;
                    switch (sprite[objIndex].type) {
                    case kDudeModernCustom:
                    case kDudeModernCustomBurning:
                        gGenDudeExtra[objIndex].updReq[kGenDudePropertySpriteSize] = true;
                        gGenDudeExtra[objIndex].updReq[kGenDudePropertyMass] = true;
                        gGenDudeExtra[objIndex].updReq[kGenDudePropertyDmgScale] = true;
                        gGenDudeExtra[objIndex].updReq[kGenDudePropertyStates] = true;
                        gGenDudeExtra[objIndex].updReq[kGenDudePropertyAttack] = true;
                        evPost(objIndex, 3, kGenDudeUpdTimeRate, kCallbackGenDudeUpdate);
                        break;
                    }
                    return true;
                case 3:
                    xsprite[sprite[objIndex].extra].data3 = value;
                    switch (sprite[objIndex].type) {
                    case kDudeModernCustom:
                    case kDudeModernCustomBurning:
                        xsprite[sprite[objIndex].extra].sysData1 = value;
                        break;
                    }
                    return true;
                case 4:
                    xsprite[sprite[objIndex].extra].data4 = value;
                    return true;
                default:
                    return false;
            }
        }
        case OBJ_SECTOR:
            xsector[sector[objIndex].extra].data = value;
            return true;
        case OBJ_WALL:
            xwall[wall[objIndex].extra].data = value;
            return true;
        default:
            return false;
    }
}

// this function checks if all TX objects have the same value
bool incDecGoalValueIsReached(XSPRITE* pXSprite) {
    for (int i = bucketHead[pXSprite->txID]; i < bucketHead[pXSprite->txID + 1]; i++) {
        if (getDataFieldOfObject(rxBucket[i].type, rxBucket[i].index, pXSprite->data1) != pXSprite->data3)
            return false;
    }
    return true;
}

// this function can be called via sending numbered command to TX kChannelModernEndLevelCustom
// it allows to set custom next level instead of taking it from INI file.
void levelEndLevelCustom(int nLevel) {

    gGameOptions.uGameFlags |= 1;

    if (nLevel >= 16 || nLevel < 0) {
        gGameOptions.uGameFlags |= 2;
        gGameOptions.nLevel = 0;
        return;
    }

    gNextLevel = nLevel;
}

void callbackUniMissileBurst(int nSprite) // 22
{
    dassert(nSprite >= 0 && nSprite < kMaxSprites);
    if (sprite[nSprite].statnum != kStatProjectile) return;
    spritetype* pSprite = &sprite[nSprite];
    int nAngle = getangle(xvel[nSprite], yvel[nSprite]);
    int nRadius = 0x55555;

    for (int i = 0; i < 8; i++)
    {
        spritetype* pBurst = actSpawnSprite(pSprite, 5);

        pBurst->type = pSprite->type;
        pBurst->shade = pSprite->shade;
        pBurst->picnum = pSprite->picnum;

        pBurst->cstat = pSprite->cstat;
        if ((pBurst->cstat & CSTAT_SPRITE_BLOCK)) {
            pBurst->cstat &= ~CSTAT_SPRITE_BLOCK; // we don't want missiles impact each other
            evPost(pBurst->index, 3, 100, kCallbackMissileSpriteBlock); // so set blocking flag a bit later
        }

        pBurst->pal = pSprite->pal;
        pBurst->clipdist = pSprite->clipdist / 4;
        pBurst->flags = pSprite->flags;
        pBurst->xrepeat = pSprite->xrepeat / 2;
        pBurst->yrepeat = pSprite->yrepeat / 2;
        pBurst->ang = ((pSprite->ang + missileInfo[pSprite->type - kMissileBase].angleOfs) & 2047);
        pBurst->owner = pSprite->owner;

        actBuildMissile(pBurst, pBurst->extra, pSprite->index);

        int nAngle2 = (i << 11) / 8;
        int dx = 0;
        int dy = mulscale30r(nRadius, Sin(nAngle2));
        int dz = mulscale30r(nRadius, -Cos(nAngle2));
        if (i & 1)
        {
            dy >>= 1;
            dz >>= 1;
        }
        RotateVector(&dx, &dy, nAngle);
        xvel[pBurst->index] += dx;
        yvel[pBurst->index] += dy;
        zvel[pBurst->index] += dz;
        evPost(pBurst->index, 3, 960, kCallbackRemove);
    }
    evPost(nSprite, 3, 0, kCallbackRemove);
}


void callbackMakeMissileBlocking(int nSprite) // 23
{
    dassert(nSprite >= 0 && nSprite < kMaxSprites);
    if (sprite[nSprite].statnum != kStatProjectile) return;
    sprite[nSprite].cstat |= CSTAT_SPRITE_BLOCK;
}

void callbackGenDudeUpdate(int nSprite) // 24
{
    if (spriRangeIsFine(nSprite))
        genDudeUpdate(&sprite[nSprite]);
}
#endif

///////////////////////////////////////////////////////////////////
// This file provides modern features for mappers.
// For full documentation please visit http://cruo.bloodgame.ru/xxsystem
///////////////////////////////////////////////////////////////////