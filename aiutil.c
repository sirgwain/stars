#include "common.h"

int32_t vrgAiPacketDist[2] = {aiPacketDistanceSquaredSingleMassDriver, aiPacketDistanceSquaredTwoMassDrivers};
uint8_t vrgSBMacAisb[6] = {aiSbRecipeMacSpaceDock, aiSbRecipeMacSpaceStation,     aiSbRecipeMacUltraStationEarly,
                           aiSbRecipeSpaceStation, aiSbRecipeMacUltraStationLate, aiSbRecipeMacDeathStar};

int16_t FCreateAiShdef(int16_t ishdef, HulDef ihul, uint8_t *rgaip) {
    StringId ids;
    int32_t  grbitHull;
    int16_t  cItem;
    int16_t  ihs;
    HUL     *lphul;
    PART     part;
    SHDEF    shdef;

    memset(&shdef, 0, sizeof(SHDEF));
    if (FLookupPartX(&part, ishdef < 0 ? 0x400 : 0x4000, ihul) != mdPartAvailAvailable) {
        return FALSE;
    }
    lphul = part.phul;
    shdef.hul = *lphul;
    shdef.hul.ihuldef = lphul->ihuldef;
    for (ihs = 0; ihs < lphul->chs; ihs++) {
        if (!FGetAIPart(rgaip[ihs], &part)) {
            return FALSE;
        }
        part.hs.cItem = lphul->rghs[ihs].cItem;
        shdef.hul.rghs[ihs] = part.hs;
    }
    shdef.hul.chs = ihs;
    shdef.fFree = FALSE;
    if (ishdef < 0) {
        shdefBuild = shdef;
        return TRUE;
    }
    ids = idsUniverseDefinitionFileSeemsMissingCorrupt;
    grbitHull = 1 << ihul;
    if (grbitHull & 0x780) {
        ids = idsLyingBastard;
        cItem = 16;
    } else if (grbitHull & 0x40) {
        ids = idsScrapper;
        cItem = 16;
    } else if (grbitHull & 0x70) {
        ids = idsEasterBunny;
        cItem = 10;
    } else if (grbitHull & 0xf0000) {
        ids = idsPidgeon;
        cItem = 12;
    } else if (grbitHull & 0xf) {
        ids = idsGlovebox;
        cItem = 8;
    } else if (grbitHull & 0x1f00000) {
        ids = idsGroundHog;
        cItem = 8;
    } else if (grbitHull & 0x3800) {
        ids = idsPricklyPear;
        cItem = 8;
    } else if (grbitHull & 0xc000) {
        ids = idsEgg;
        cItem = 8;
    } else {
        ids = idsZombie;
        cItem = 8;
    }
    shdef.ishdef = ishdef;
    PickANameAndBmp(&shdef, ids, cItem, lphul->ibmp);
    return FChangeAiShdef(&shdef, ishdef);
}

int16_t FGetAIPart(AiPartPreference aip, PART *ppart) {
    int16_t cTry;
    int16_t iOffset;
    int16_t i;
    int16_t cItem;
    PART    part;

    iOffset = 0;
    for (i = 0; i < (int16_t)aip; i++) {
        iOffset += vrgcAiParts[i];
    }
    cTry = vrgcAiParts[aip];
    while (cTry-- > 0) {
        part.hs.grhst = 1 << vrgAiParts[iOffset].ibit;
        part.hs.iItem = vrgAiParts[iOffset].iItem;
        part.hs.cItem = 0;
        cItem = vrgAiParts[iOffset].cItem;
        while (cItem-- > 0) {
            if (FLookupPart(&part) == mdPartAvailAvailable) {
                *ppart = part;
                return TRUE;
            }
            part.hs.iItem--;
        }
        iOffset++;
    }
    return FALSE;
}

void PickANameAndBmp(SHDEF *pshdef, StringId ids, int16_t cids, int16_t ibmpStart) {
    int16_t i;
    int16_t rgfBmpUsed[4];
    int16_t ishdef;

    memset(rgfBmpUsed, 0, 8);
    if (pshdef->ishdef >= 16) {
        for (ishdef = 0; ishdef < 10; ishdef++) {
            if (!rglpshdefSB[idPlayer][ishdef].fFree && rglpshdefSB[idPlayer][ishdef].hul.ihuldef == pshdef->hul.ihuldef) {
                rgfBmpUsed[rglpshdefSB[idPlayer][ishdef].hul.ibmp + -ibmpStart] = TRUE;
            }
        }
    } else {
        for (ishdef = 0; ishdef < 16; ishdef++) {
            if (!rgshdef[ishdef].fFree && rgshdef[ishdef].hul.ihuldef == pshdef->hul.ihuldef) {
                rgfBmpUsed[rgshdef[ishdef].hul.ibmp + -ibmpStart] = TRUE;
            }
        }
    }
    for (i = 0; i < 4 && rgfBmpUsed[i] != 0; i++) {
    }
    if (i == 4) {
        i = Random(4);
    }
    pshdef->hul.ibmp = ibmpStart + i;
    for (i = 0; i < 20; i++) {
        CchGetString(Random(cids) + ids, pshdef->hul.szClass);
        if (pshdef->ishdef >= 16) {
            for (ishdef = 0; ishdef < 10 && (rglpshdefSB[idPlayer][ishdef].fFree || rglpshdefSB[idPlayer][ishdef].hul.ihuldef != pshdef->hul.ihuldef ||
                                             strcmp(pshdef->hul.szClass, rglpshdefSB[idPlayer][ishdef].hul.szClass) != 0);
                 ishdef++) {
            }
            if (ishdef == 10) {
                return;
            }
        } else {
            for (ishdef = 0; ishdef < 16 && (rgshdef[ishdef].fFree || strcmp(pshdef->hul.szClass, rgshdef[ishdef].hul.szClass) != 0); ishdef++) {
            }
            if (ishdef == 16) {
                return;
            }
        }
    }
    CchSprintf(pshdef->hul.szClass, "%s %d", PszGetCompressedString(Random(cids) + ids), Random(100));
    return;
}

int16_t FChangeAiShdef(SHDEF *pshdef, int16_t ishdef) {
    SHDEF  *lpshdefBase;
    int16_t iDir;
    int16_t ishdefWork;
    SHDEF   shdef;

    shdef = *pshdef;
    shdef.ishdef = ishdef;
    shdef.turn = game.turn;
    shdef.cExist = 0;
    shdef.cBuilt = 0;
    if (ishdef >= 16) {
        lpshdefBase = rglpshdefSB[idPlayer];
        ishdefWork = ishdef - 16;
    } else {
        lpshdefBase = rglpshdef[idPlayer];
        ishdefWork = ishdef;
    }
    UpdateShdefCost(&shdef);
    if (shdef.fFree != lpshdefBase[ishdefWork].fFree) {
        iDir = !shdef.fFree ? 1 : -1;
        if (ishdef >= 16) {
            rgplr[idPlayer].cshdefSB += iDir;
        } else {
            rgplr[idPlayer].cShDef += iDir;
        }
    } else if (!lpshdefBase[ishdefWork].fFree) {
        lpshdefBase[ishdefWork].fFree = TRUE;
        LogChangeShDef(lpshdefBase + ishdefWork);
    }
    lpshdefBase[ishdefWork] = shdef;
    LogChangeShDef(&shdef);
    return TRUE;
}

int16_t XferAiSupply(GrobjClass grobjSrc, int16_t idSrc, GrobjClass grobjDst, int16_t idDst, MineralType iSupply, int16_t cQuan) {
    int16_t dChg;
    int16_t iT;
    int32_t cAvailable;

    if (cQuan == 0) {
        return 0;
    }
    if (cQuan < 0) {
        iT = grobjSrc;
        grobjSrc = grobjDst;
        grobjDst = iT;
        iT = idSrc;
        idSrc = idDst;
        idDst = iT;
        cQuan = -cQuan;
    }
    cAvailable = ChgCargo(grobjSrc, idSrc, iSupply, 0, NULL);
    if (cQuan > cAvailable) {
        cQuan = LOWORD(cAvailable);
    }
    if (cQuan == 0) {
        return 0;
    }
    dChg = LOWORD(ChgCargo(grobjDst, idDst, iSupply, cQuan, NULL));
    if (dChg != 0) {
        ChgCargo(grobjSrc, idSrc, iSupply, (int16_t)-dChg, NULL);
    }
    return dChg;
}

int16_t XferAiTroopers(int16_t idSrc, int16_t idDst, int16_t cQuan) {
    int32_t cAvailable;

    if (cQuan == 0) {
        return 0;
    }
    cAvailable = ChgCargo(grobjFleet, idSrc, Colonists, 0, NULL);
    if (cQuan > cAvailable) {
        cQuan = LOWORD(cAvailable);
    }
    if (cQuan == 0) {
        return 0;
    }
    ChgCargo(grobjFleet, idSrc, Colonists, (int16_t)-cQuan, NULL);
    ChgCargo(grobjPlanet, idDst, Colonists, cQuan, NULL);
    return cQuan;
}

int16_t FColonizeAiFleet(FLEET *lpfl, int16_t idPlanet) {
    ORDER ord;

    ChangeMainObjSel(grobjFleet, lpfl->id);
    memset(&ord, 0, sizeof(ORDER));
    ord.pt = rgptPlan[idPlanet];
    ord.grobj = grobjPlanet;
    ord.id = idPlanet;
    ord.grTask = grTaskColonize;
    ord.fValidTask = TRUE;
    ord.iWarp = (uint16_t)IFindIdealWarp(lpfl, TRUE);
    if (!FMoveAiFleet(lpfl, &ord, FALSE)) {
        return FALSE;
    }
    lpfl->fMark = TRUE;
    return TRUE;
}

int16_t FGotoWormholeAiFleet(FLEET *lpfl, THING *lpthWorm) {
    ORDER ord;

    ChangeMainObjSel(grobjFleet, lpfl->id);
    memset(&ord, 0, sizeof(ORDER));
    ord.pt = lpthWorm->pt;
    ord.grobj = grobjThing;
    ord.id = lpthWorm->idFull;
    ord.grTask = grTaskNone;
    ord.fValidTask = FALSE;
    ord.iWarp = (uint16_t)IFindIdealWarp(lpfl, TRUE);
    if (!FMoveAiFleet(lpfl, &ord, FALSE)) {
        return FALSE;
    }
    lpfl->fMark = TRUE;
    return TRUE;
}

int16_t IdNearestColonizablePlanet(FLEET *lpflCol, THING **plpthWorm) {
    PLANET  *lpplMac;
    POINT16  pt;
    int32_t  dy;
    int32_t  d2;
    PLANET  *lppl;
    int16_t  idBest;
    int16_t  ifl;
    FLEET   *lpfl;
    int16_t  i;
    int16_t  iVal;
    int32_t  dx;
    uint8_t *lpb;
    int32_t  d2Cur;

    if (rgplr[idPlayer].idAi != idAiRobotoid && rgplr[idPlayer].idAi != idAiMacinti) {
        iVal = 16;
        if (fMarkedPlanets)
            goto LFindNearest;
    } else {
        iVal = 0;
    }
    lpb = vlpbAiPlanet + 15;
    i = 0;
    while (i < game.cPlanMax) {
        *lpb = iVal;
        i++;
        lpb += 16;
    }
    lppl = lpPlanets;
    lpplMac = lpPlanets + cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->iPlayer != iplrNone) {
            if (lppl->iPlayer == idPlayer) {
                iVal = 1;
            } else {
                iVal = 2;
            }
        } else if (rgplr[idPlayer].idAi != idAiRobotoid && rgplr[idPlayer].idAi != idAiMacinti && PctPlanetOptValue(lppl, idPlayer) < 0) {
            iVal = 8;
        } else {
            iVal = 0;
        }
        vlpbAiPlanet[lppl->id * 16 + 15] = iVal;
    }
    for (ifl = 0; ifl < cFleet; ifl++) {
        lpfl = rglpfl[ifl];
        if (!rglpfl[ifl])
            break;
        if (lpfl->iPlayer == idPlayer && lpfl->cord > 1 && lpfl != lpflCol && lpfl->lpplord->rgord[1].grobj == grobjPlanet &&
            ((rgplr[idPlayer].idAi != idAiRobotoid && rgplr[idPlayer].idAi != idAiMacinti) || lpfl->lpplord->rgord[1].grTask == grTaskColonize)) {
            vlpbAiPlanet[lpfl->lpplord->rgord[1].id * 16 + 15] = 4;
        }
    }
LFindNearest:
    lpb = vlpbAiPlanet + 15;
    d2 = 99999999;
    pt = lpflCol->pt;
    idBest = idplNone;
    i = 0;
    while (i < game.cPlanMax) {
        if (*lpb <= 0) {
            dx = abs(pt.x - rgptPlan[i].x);
            dy = abs(pt.y - rgptPlan[i].y);
            if (dx < d2 && dy < d2) {
                d2Cur = (uint32_t)(dx * dx);
                if ((int32_t)(uint32_t)(dx * dx) < d2) {
                    d2Cur += (uint32_t)(dy * dy);
                    if (d2Cur < d2) {
                        d2 = d2Cur;
                        idBest = i;
                    }
                }
            }
        }
        i++;
        lpb += 16;
    }
    fMarkedPlanets = TRUE;
    if (plpthWorm) {
        *plpthWorm = NULL;
        if (lpflCol->idPlanet != idPlanetDeepSpace && game.turn < 120) {
            lppl = LpplFromId(lpflCol->idPlanet);
            if (lppl && lppl->iPlayer == idPlayer) {
                *plpthWorm = LpthWormFind(&pt, d2);
            }
        }
        if (*plpthWorm) {
            idBest = idplNone;
        }
    }
    return idBest;
}

THING *LpthWormFind(POINT16 *ppt, int32_t d2) {
    int16_t  pctGood;
    int16_t  dy;
    int32_t  d2Worm;
    uint16_t grbitplr;
    THING   *lpth;
    THING   *lpthBest;
    int16_t  iVal;
    int16_t  dx;
    THING   *lpthMac;
    int32_t  d2Cur;

    lpthBest = NULL;
    grbitplr = 1 << idPlayer;
    d2Worm = 1000000;
    pctGood = 0;
    lpth = lpThings;
    lpthMac = lpThings + cThing;
    for (; lpth < lpthMac; lpth++) {
        if (lpth->ith == ithWormhole) {
            dx = abs(ppt->x - lpth->pt.x);
            dy = abs(ppt->y - lpth->pt.y);
            d2Cur = (int16_t)(dx * dx + dy * dy);
            if (d2Cur <= (int32_t)(d2 * 4) && d2Cur <= 46656) {
                if (lpth->thw.grbitPlrTrav & grbitplr) {
                    iVal = PctWormholeMoves(lpth);
                    iVal = 70 - 10 * iVal;
                } else if (d2Cur <= d2) {
                    iVal = 90;
                } else {
                    iVal = 50;
                }
                if (iVal > pctGood || (iVal == pctGood && d2Cur < d2Worm)) {
                    pctGood = iVal;
                    lpthBest = lpth;
                    d2Worm = d2Cur;
                }
            }
        }
    }
    if (lpthBest && Random(100) < pctGood) {
        return lpthBest;
    }
    return NULL;
}

uint32_t UlFleetPower(FLEET *lpfl) {
    uint32_t ul;
    int16_t  iplr;
    int16_t  ishdef;

    ul = 0;
    iplr = lpfl->iPlayer;
    for (ishdef = 0; ishdef < 16; ishdef++) {
        if (lpfl->rgcsh[ishdef] != 0) {
            ul += (uint32_t)(rglpshdef[iplr][ishdef].lPower * lpfl->rgcsh[ishdef]);
            if (ul & 0x80000000)
                break;
        }
    }
    return ul;
}

int16_t IdNearestUnknownPlanet(FLEET *lpfl, THING **plpthWorm) {
    POINT16  pt;
    int32_t  dy;
    int32_t  d2;
    int16_t  idBest;
    int16_t  i;
    int32_t  dx;
    uint8_t *lpb;
    int32_t  d2Cur;

    if (!fMarkedPlanets) {
        IdNearestColonizablePlanet(lpfl, NULL);
    }
    lpb = vlpbAiPlanet + 15;
    d2 = 99999999;
    pt = lpfl->pt;
    idBest = idplNone;
    i = 0;
    while (i < game.cPlanMax) {
        if (*lpb == 16) {
            dx = abs(pt.x - rgptPlan[i].x);
            dy = abs(pt.y - rgptPlan[i].y);
            if (dx < d2 && dy < d2) {
                d2Cur = (uint32_t)(dx * dx);
                if ((int32_t)(uint32_t)(dx * dx) < d2) {
                    d2Cur += (uint32_t)(dy * dy);
                    if (d2Cur < d2) {
                        d2 = d2Cur;
                        idBest = i;
                    }
                }
            }
        }
        i++;
        lpb += 16;
    }
    if (plpthWorm) {
        *plpthWorm = NULL;
        if (lpfl->idPlanet != idPlanetDeepSpace && Random(100) < 5) {
            *plpthWorm = LpthWormFind(&pt, d2);
        }
        if (*plpthWorm) {
            idBest = idplNone;
        }
    }
    return idBest;
}

void AddMinesToBlockedQueues() {
    PROD    prod;
    int32_t cMaxBuild;
    int16_t etaBetterAlchemy;
    int32_t cBuild;
    int16_t etaFirst;
    PLANET *lppl;
    int32_t cResMine;
    int32_t cRes;
    int16_t ipl;
    int32_t rgCost[4];
    PROD    rgprod[64];
    int16_t etaBetterMines;

    for (ipl = 0; ipl < vclpplAi; ipl++) {
        lppl = vrglpplAi[ipl];
        if (!vrglpplAi[ipl])
            break;
        if (lppl->lpplprod) {
            prod = lppl->lpplprod->rgprod[0];
            if (prod.grobj == grobjPlanet &&
                (prod.iItem == mdIdleMine || prod.iItem == iobjAlchemy || prod.iItem == mdIdleAlchemy || prod.iItem == mdIdleTerraform))
                continue;
            ChangeMainObjSel(grobjPlanet, lppl->id);
            PszProductionETA(&sel.pl, sel.pl.lpplprod, iobjMine, &etaFirst, NULL);
            if (etaFirst != 1) {
                if (etaFirst == -1) {
                    etaFirst = 600;
                }
                GetProductionCosts(lppl, &prod, rgCost, idPlayer, TRUE);
                cRes = CResourcesAtPlanet(&sel.pl, idPlayer);
                if (!sel.pl.fNoResearch) {
                    cRes -= (int32_t)(cRes * (int16_t)rgplr[idPlayer].pctResearch) / 100;
                }
                if (rgCost[3] <= (int32_t)(uint32_t)(cRes * (int16_t)(etaFirst - 1))) {
                    cMaxBuild = CMaxOperableMines(&sel.pl, idPlayer, TRUE) - sel.pl.cMines;
                    if (cMaxBuild < 0) {
                        cMaxBuild = 0;
                    }
                    cResMine = GetRaceStat(&rgplr[idPlayer], rsMineBuild);
                    if ((int32_t)(uint32_t)(cResMine * cMaxBuild) <= cRes) {
                        cBuild = cMaxBuild;
                    } else {
                        cBuild = (int32_t)(cRes / cResMine);
                    }
                    InitProduction(rgprod);
                    if (cBuild > 0) {
                        AddItemToQueue(mdIdleMine, LOWORD(cBuild), grobjPlanet, addItemFront);
                        FinishProduction(TRUE);
                        PszProductionETA(&sel.pl, sel.pl.lpplprod, iobjFactory, &etaBetterMines, NULL);
                        if (etaBetterMines == -1) {
                            etaBetterMines = 700;
                        }
                        sel.pl.lpplprod->rgprod[0].cItem = 1;
                        sel.pl.lpplprod->rgprod[0].iItem = iobjAlchemy;
                    } else {
                        etaBetterMines = 700;
                        AddItemToQueue(iobjAlchemy, 1, grobjPlanet, addItemFront);
                        FinishProduction(TRUE);
                    }
                    PszProductionETA(&sel.pl, sel.pl.lpplprod, iobjFactory, &etaBetterAlchemy, NULL);
                    if (etaBetterAlchemy == -1) {
                        etaBetterAlchemy = 700;
                    }
                    if ((etaBetterAlchemy >= etaFirst || etaBetterAlchemy >= etaBetterMines) && cBuild >= 1) {
                        sel.pl.lpplprod->rgprod[0].iItem = mdIdleMine;
                        if (etaFirst < etaBetterMines || cBuild <= 0) {
                            sel.pl.lpplprod->iprodMac--;
                            memmove(sel.pl.lpplprod->rgprod, &sel.pl.lpplprod->rgprod[1], sel.pl.lpplprod->iprodMac * sizeof(PROD));
                        } else {
                            sel.pl.lpplprod->rgprod[0].cItem = LOWORD(cBuild);
                        }
                    }
                }
            }
        }
    }
    return;
}

int16_t FFleetInField(FLEET *lpfl, THING *lpth) {
    int16_t dy;
    int16_t dx;
    int32_t dxy2;

    dx = abs(lpfl->pt.x - lpth->pt.x);
    dy = abs(lpfl->pt.y - lpth->pt.y);
    dxy2 = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
    if (dxy2 < lpth->thm.cMines) {
        return TRUE;
    }
    return FALSE;
}

void SetAiFleetIdealSpeed(FLEET *lpfl, int16_t wtFuelMax, int16_t cMinefields, THING **rglpth) {
    THING  *lpth;
    int16_t i;
    int16_t j;
    int16_t ith;
    THING  *lpthMac;
    int16_t fMinefield;

    fMinefield = FALSE;
    switch (cMinefields) {
    case 0:
        break;
    case -1:
        lpth = lpThings;
        lpthMac = lpThings + cThing;
        for (; lpth < lpthMac; lpth++) {
            if (lpth->ith == ithMinefield && lpth->iplr != idPlayer && lpth->thm.iType != mineSpeedBump && FFleetInField(lpfl, lpth)) {
                fMinefield = TRUE;
                goto LSelectFleet;
            }
        }
        break;
    default:
        for (ith = 0; ith < cMinefields; ith++) {
            if (FFleetInField(lpfl, rglpth[ith])) {
                lpth = rglpth[ith];
                fMinefield = TRUE;
                goto LSelectFleet;
            }
        }
        break;
    }
LSelectFleet:
    ChangeMainObjSel(grobjFleet, lpfl->id);
    if (fMinefield) {
        switch (lpth->thm.iType) {
        case mineHeavy:
            i = 6;
            break;
        case mineSpeedBump:
            i = 5;
            j = Random(10);
            if (j >= 7) {
                i++;
            }
            break;
        default:
            i = 4;
            j = Random(10);
            if (j >= 4) {
                i++;
            }
            break;
        }
        switch (GetRaceStat(&rgplr[idPlayer], rsMajorAdv)) {
        case raStealth:
            i++;
            break;
        }
        goto WarpSet;
    }
    i = IWarpBestForWaypoint(lpfl, &lpfl->lpplord->rgord[1]);
WarpSet:
    if (i != lpfl->lpplord->rgord[1].iWarp) {
        sel.fl.lpplord->rgord[1].iWarp = i;
        FLookupFleet(idWriteBack, &sel.fl);
    }
    return;
}

int16_t IdTargetAttack(FLEET *lpfl, FLEET *lpflAtk, FLEET *lpflEnemy, int16_t fOnlyHumans) {
    FLEET   *lpflClosest;
    FLEET   *lpflT;
    int32_t  lDistBest;
    PLANET  *lpplMac;
    POINT16  pt;
    int16_t  dy;
    PLANET  *lppl;
    int32_t  lDist;
    int16_t  idClosest;
    int16_t  i;
    int16_t  cShipsAtk;
    int16_t  cShipsDst;
    PLANET  *lpplClosest;
    uint8_t *lpb;
    int16_t  dx;
    FLEET   *lpflAtk2;
    ORDER    ord;

    cShipsDst = 0;
    cShipsAtk = 0;
    for (i = 0; i < 16; i++) {
        cShipsAtk += lpfl->rgcsh[i];
    }
    lpflClosest = NULL;
    lpflT = lpflEnemy;
    lDistBest = 1000000;
    pt = lpfl->pt;
    memset(&ord, 0, sizeof(ORDER));
    for (; lpflT; lpflT = lpflT->lpflNext) {
        if (fOnlyHumans && rgplr[lpflT->iPlayer].fAi)
            goto NextTarg;
        lpflAtk2 = lpflAtk;
        cShipsDst = 0;
        for (; lpflAtk2; lpflAtk2 = lpflAtk2->lpflNext) {
            if (lpflAtk2 != lpfl && lpflAtk2->cord >= 2 && lpflAtk2->lpplord->rgord[1].id == lpflT->id && lpflAtk2->lpplord->rgord[1].grobj == grobjFleet) {
                for (i = 0; i < 16; i++) {
                    cShipsDst += lpflAtk2->rgcsh[i];
                }
            }
        }
        if ((cShipsDst > 0 && Random(3) == 0) || (5 * cShipsDst > cShipsAtk && Random(15) == 0))
            goto NextTarg;
        dx = lpflT->pt.x - pt.x;
        dy = lpflT->pt.y - pt.y;
        lDist = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
        if (lDist < lDistBest) {
            lDistBest = lDist;
            lpflClosest = lpflT;
        }
    NextTarg:;
    }
    if (lDistBest < 32400) {
        ord.pt = lpflClosest->pt;
        ord.grobj = grobjFleet;
        ord.id = lpflClosest->id;
    } else {
        if (lpflClosest) {
            if (lpfl->rgwtMin[4] < (int32_t)(LGetFleetStat(lpfl, 1) / 2) && FMoveToNearestStarbase(lpfl, FALSE)) {
                return 0;
            }
            pt = lpflClosest->pt;
            lpplClosest = NULL;
            lDistBest = 1000000;
            lppl = lpPlanets;
            lpplMac = lpPlanets + cPlanet;
            for (; lppl < lpplMac; lppl++) {
                for (lpflAtk2 = lpflAtk; lpflAtk2 && (lpflAtk2 == lpfl || lpflAtk2->cord < 2 || lpflAtk2->lpplord->rgord[1].id != lppl->id ||
                                                      lpflAtk2->lpplord->rgord[1].grobj != grobjPlanet);
                     lpflAtk2 = lpflAtk2->lpflNext) {
                }
                if (!lpflAtk2) {
                    dx = rgptPlan[lppl->id].x - pt.x;
                    dy = rgptPlan[lppl->id].y - pt.y;
                    lDist = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                    if (lDist < lDistBest) {
                        lDistBest = lDist;
                        lpplClosest = lppl;
                    }
                }
            }
            if (lpplClosest && lpplClosest->id != lpfl->idPlanet) {
                ord.pt = rgptPlan[lpplClosest->id];
                ord.grobj = grobjPlanet;
                ord.id = lpplClosest->id;
                goto ThwakSumthin;
            }
        } else if (fOnlyHumans) {
            return IdTargetAttack(lpfl, lpflAtk, lpflEnemy, FALSE);
        }
        pt = lpfl->pt;
        lpplClosest = NULL;
        lDistBest = 1000000;
        lppl = lpPlanets;
        lpplMac = lpPlanets + cPlanet;
        for (; lppl < lpplMac; lppl++) {
            if (lppl->iPlayer != iplrNone && lppl->iPlayer != idPlayer) {
                dx = rgptPlan[lppl->id].x - pt.x;
                dy = rgptPlan[lppl->id].y - pt.y;
                lDist = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                if (lDist < lDistBest) {
                    lDistBest = lDist;
                    lpplClosest = lppl;
                }
            }
        }
        if (lpplClosest && lpplClosest->id != lpfl->idPlanet) {
            ord.pt = rgptPlan[lpplClosest->id];
            ord.grobj = grobjPlanet;
            ord.id = lpplClosest->id;
        } else {
            idClosest = idplNone;
            lDistBest = 1000000;
            lpb = vlpbAiPlanet + 9;
            i = 0;
            while (i < game.cPlanMax) {
                if (*lpb <= 0) {
                    dx = rgptPlan[i].x - pt.x;
                    dy = rgptPlan[i].y - pt.y;
                    lDist = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                    if (lDist < lDistBest) {
                        lDistBest = lDist;
                        idClosest = i;
                    }
                }
                i++;
                lpb += 16;
            }
            if (idClosest != idplNone) {
                ord.pt = rgptPlan[idClosest];
                ord.grobj = grobjPlanet;
                ord.id = idClosest;
            } else {
                ord.id = Random(game.cPlanMax);
                ord.grobj = grobjPlanet;
                ord.pt = rgptPlan[ord.id];
            }
        }
    }
ThwakSumthin:
    if (lpfl->cord > 1 && lpfl->lpplord->rgord[1].grobj == grobjPlanet && ord.grobj == grobjPlanet) {
        return -1;
    }
    ord.grTask = grTaskNone;
    ord.fValidTask = TRUE;
    ord.iWarp = 4;
    if (!FMoveAiFleet(lpfl, &ord, FALSE)) {
        return -1;
    }
    return 0;
}

int16_t IdTargetFreighter(FLEET *lpflFr, PLANET *lpplHome) {
    int32_t  lWorst2;
    int32_t  scoreBest;
    PLANET  *lpplMac;
    int32_t  score;
    POINT16  pt;
    int16_t  dy;
    int32_t  lWorst;
    int16_t  pctFull;
    int16_t  idBest;
    PLANET  *lppl;
    int32_t  wtPlanCargo;
    FLEET   *lpfl;
    int32_t  wtCargoMax;
    int16_t  ifl;
    int16_t  i;
    int16_t  iWorst2;
    int32_t  wtCargoFree;
    THING   *lpthBest;
    int16_t  iWorst = 0; // First mineral until the minimum search selects another.
    PLANET  *lpplBest;
    int16_t  ishFreighter;
    int16_t  pctHere;
    int16_t  dx;
    uint8_t *lpb;
    int16_t  fNeedy;
    ORDER    ord;
    int16_t  fSalvage;
    int32_t  l;

    fSalvage = FALSE;
    lpb = vlpbAiPlanet + 14;
    i = 0;
    while (i < game.cPlanMax) {
        *lpb = 0;
        i++;
        lpb += 16;
    }
    for (ishFreighter = 0; ishFreighter < 16 && lpflFr->rgcsh[ishFreighter] <= 0; ishFreighter++) {
    }
    lWorst = 1000000000;
    lWorst2 = 1000000000;
    for (i = 0; i < 3; i++) {
        if (lpplHome->rgwtMin[i] < lWorst) {
            lWorst2 = lWorst;
            iWorst2 = iWorst;
            lWorst = lpplHome->rgwtMin[i];
            iWorst = i;
        }
    }
    fNeedy = lWorst < (int32_t)(lWorst2 >> 1);
    if (lWorst < (int32_t)(lWorst2 >> 2)) {
        fNeedy++;
    }
    for (ifl = 0; ifl < cFleet; ifl++) {
        lpfl = rglpfl[ifl];
        if (!rglpfl[ifl])
            break;
        if (lpfl->iPlayer == idPlayer && lpfl->cord > 1 && lpfl != lpflFr && lpfl->lpplord->rgord[1].grobj == grobjPlanet && lpfl->rgcsh[ishFreighter] != 0 &&
            lpfl->lpplord->rgord[1].id != lpplHome->id) {
            vlpbAiPlanet[lpfl->lpplord->rgord[1].id * 16 + 14] = 1;
        }
    }
    wtCargoMax = LGetFleetStat(lpflFr, 2);
    wtCargoFree = GetCargoFree(lpflFr);
    if (wtCargoMax <= 0) {
        return idplNone;
    }
    pctFull = 100 - LOWORD((int32_t)((int32_t)(wtCargoFree * 100) / wtCargoMax));
    scoreBest = 0;
    idBest = idplNone;
    pt = lpflFr->pt;
    lppl = lpPlanets;
    lpplMac = lpPlanets + cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->id != lpflFr->idPlanet && vlpbAiPlanet[lppl->id * 16 + 14] == 0) {
            if (lppl->iPlayer == iplrNone && (vlpbAiPlanet[lppl->id * 16 + 1] & 0x80)) {
                dx = pt.x - rgptPlan[lppl->id].x;
                dy = pt.y - rgptPlan[lppl->id].y;
                l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                l = ((int32_t)sqrt((double)l) + 24) / 25;
                if (l < 1) {
                    l = 1;
                }
                score = (int32_t)((int16_t)((vlpbAiPlanet[lppl->id * 16 + 1] & 0x7f) * 0x1f4) / l);
            } else {
                if (lppl->iPlayer != idPlayer) {
                    if (rgplr[idPlayer].idAi == idAiRobotoid && lpplHome->id == lpflFr->idPlanet &&
                        ((lpplHome->rgwtMin[3] > 1100 && lppl->uPopGuess < 62) || (lpplHome->rgwtMin[3] > 300 && lppl->uPopGuess < 15))) {
                        pctHere = 65;
                        goto ScorePctHere;
                    }
                    if (vlpbAiPlanet[lppl->id * 16 + 3] == 0 || (vlpbAiPlanet[lppl->id * 16 + 3] & 0x80) || lpplHome->id != lpflFr->idPlanet)
                        continue;
                    score = 25000;
                }
                if (lppl == lpplHome) {
                    if (pctFull < 35)
                        continue;
                    if (pctFull == 100) {
                        score = 25000;
                        goto LScore;
                    }
                    dx = pt.x - rgptPlan[lppl->id].x;
                    dy = pt.y - rgptPlan[lppl->id].y;
                    l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                    l = ((int32_t)sqrt((double)l) + 24) / 25;
                    if (l == 0)
                        continue;
                    score = (int32_t)((int16_t)(20 * pctFull) / l);
                    goto LScore;
                }
                if (lppl->fStarbase || (lppl->lpplprod && lppl->lpplprod->rgprod[0].grobj == grobjFleet && lppl->lpplprod->rgprod[0].iItem >= iobjPacketGerm))
                    continue;
                if (vlpbAiPlanet[lppl->id * 16 + 2] != 0 && !(vlpbAiPlanet[lppl->id * 16 + 2] & 0x80) && lpplHome->id == lpflFr->idPlanet) {
                    score = 25000;
                    goto LScore;
                }
                if (fNeedy == 2) {
                    wtPlanCargo = lppl->rgwtMin[iWorst];
                } else {
                    wtPlanCargo = 0;
                    for (i = 0; i < 3; i++) {
                        if (fNeedy && i != iWorst) {
                            wtPlanCargo += (int32_t)(lppl->rgwtMin[i] >> 1);
                        } else {
                            wtPlanCargo += lppl->rgwtMin[i];
                        }
                    }
                }
                if (wtPlanCargo < 10)
                    continue;
                pctHere = LOWORD((int32_t)((int32_t)(wtPlanCargo * 100) / wtCargoMax));
                if (pctHere > 100 - pctFull) {
                    pctHere = 100 - pctFull;
                }
            ScorePctHere:
                dx = pt.x - rgptPlan[lppl->id].x;
                dy = pt.y - rgptPlan[lppl->id].y;
                l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                l = ((int32_t)sqrt((double)l) + 24) / 25;
                if (l < 1) {
                    l = 1;
                }
                score = (int32_t)((int16_t)(100 * pctHere) / l);
            }
        LScore:
            if (score > scoreBest) {
                scoreBest = score;
                lpplBest = lppl;
                idBest = lppl->id;
            }
        }
    }
    lpthBest = NULL;
    if (FSalvageTargetFreighter2(lpflFr, fNeedy, iWorst, pctFull, wtCargoMax, scoreBest, &lpthBest, &idBest) && GetCargoFree(lpflFr) == 0) {
        lpthBest = NULL;
        lpplBest = lpplHome;
        idBest = lpplHome->id;
    }
    if (idBest == idplNone) {
        return idplNone;
    }
    memset(&ord, 0, sizeof(ORDER));
    if (lpthBest) {
        ord.grobj = grobjThing;
        ord.pt = lpthBest->pt;
    } else {
        ord.grobj = grobjPlanet;
        ord.pt = rgptPlan[idBest];
    }
    ord.id = idBest;
    ord.grTask = grTaskXfer;
    ord.fValidTask = TRUE;
    ord.iWarp = 4;
    for (i = 0; i <= 2; i++) {
        ord.txp.rgia[i].iAction = !lpthBest && idBest == lpplHome->id ? 2 : 1;
    }
    if (rgplr[idPlayer].idAi == idAiTurinDrone) {
        if (lpplHome->id == lpflFr->idPlanet) {
            ChangeMainObjSel(grobjFleet, lpflFr->id);
            if (lpplHome->rgwtMin[3] > 1200 && !lpthBest && lpplBest->iPlayer != iplrNone && lpplBest->rgwtMin[3] < lpplHome->rgwtMin[3]) {
                XferAiSupply(grobjPlanet, lpflFr->idPlanet, grobjFleet, lpflFr->id, Colonists, 1000);
            }
            FLookupFleet(lpflFr->id, &sel.fl);
        }
        if (!lpthBest && lpplBest->iPlayer != iplrNone && lpplBest != lpplHome) {
            ord.txp.rgia[3].iAction = iActionUnloadAll;
        }
    } else if (rgplr[idPlayer].idAi == idAiRobotoid && lpplHome->id == lpflFr->idPlanet && !lpthBest && lpplBest->iPlayer != iplrNone) {
        if (lpplBest->iPlayer != idPlayer) {
            ChangeMainObjSel(grobjFleet, lpflFr->id);
            if (lpplHome->rgwtMin[3] > 900) {
                XferAiSupply(grobjPlanet, lpflFr->idPlanet, grobjFleet, lpflFr->id, Colonists, 300);
            } else {
                XferAiSupply(grobjPlanet, lpflFr->idPlanet, grobjFleet, lpflFr->id, Colonists, 100);
            }
            FLookupFleet(lpflFr->id, &sel.fl);
            ord.txp.rgia[3].iAction = iActionUnloadAll;
        } else if (lpplBest->rgwtMin[3] < 1000 && lpplHome->rgwtMin[3] > (int32_t)(lpplBest->rgwtMin[3] * 3) / 2 && lpplHome->rgwtMin[3] > 500) {
            if (lpplHome->rgwtMin[3] > 2000) {
                l = 10;
            } else if (lpplHome->rgwtMin[3] > 1500) {
                l = 8;
            } else if (lpplHome->rgwtMin[3] > 1000) {
                l = 5;
            } else {
                l = (int32_t)((lpplHome->rgwtMin[3] - 400) / 100);
            }
            l = (int32_t)(lpplHome->rgwtMin[3] * l) / 100;
            ChangeMainObjSel(grobjFleet, lpflFr->id);
            XferAiSupply(grobjPlanet, lpflFr->idPlanet, grobjFleet, lpflFr->id, Colonists, LOWORD(l));
            FLookupFleet(lpflFr->id, &sel.fl);
            ord.txp.rgia[3].iAction = iActionUnloadAll;
        }
    }
    if (lpthBest) {
        ord.grTask = grTaskNone;
        ord.fValidTask = FALSE;
    } else if (lpplHome->id != idBest && fNeedy) {
        if (fNeedy == 2 || lpplBest->rgwtMin[iWorst] >= wtCargoFree) {
            for (i = 0; i <= 2; i++) {
                if (i != iWorst) {
                    ord.txp.rgia[i].iAction = iActionNone;
                }
            }
        } else if (lpthBest || lpplBest->iPlayer != iplrNone) {
            for (i = 0; i <= 2; i++) {
                ord.txp.rgia[i].iAction = iActionFillPercent;
                if (i == iWorst) {
                    l = 66;
                } else {
                    l = 33;
                }
                ord.txp.rgia[i].cQuan = LOWORD(l);
            }
        }
    }
    if (!FMoveAiFleet(lpflFr, &ord, FALSE)) {
        return idplNone;
    }
    return idBest;
}

int16_t FSalvageTargetFreighter2(FLEET *lpflFr, int16_t fNeedy, int16_t iWorst, int16_t pctFull, int32_t wtCargoMax, int32_t scoreBest, THING **plpthBest,
                                 int16_t *pidBest) {
    POINT16 pt;
    int32_t score;
    int16_t dy;
    int16_t i;
    int32_t wtPlanCargo;
    THING  *lpth;
    int16_t pctHere;
    int16_t dx;
    THING  *lpthMac;
    int16_t fSalvage;
    int32_t l;

    fSalvage = FALSE;
    pt = lpflFr->pt;
    lpth = lpThings;
    lpthMac = lpThings + cThing;
    for (; lpth < lpthMac; lpth++) {
        if (lpth->ith == ithMineralPacket && lpth->thp.iWarp == 0) {
            dx = pt.x - lpth->pt.x;
            dy = pt.y - lpth->pt.y;
            if (dx == 0 && dy == 0) {
                if (fNeedy && lpth->thp.rgwtMin[iWorst] != 0) {
                    XferAiSupply(grobjThing, lpth->idFull, grobjFleet, lpflFr->id, iWorst, lpth->thp.rgwtMin[iWorst]);
                }
                if (fNeedy != 2) {
                    for (i = 0; i < 3; i++) {
                        if (lpth->thp.rgwtMin[i] != 0) {
                            XferAiSupply(grobjThing, lpth->idFull, grobjFleet, lpflFr->id, i, lpth->thp.rgwtMin[i]);
                        }
                    }
                }
                fSalvage = TRUE;
                FLookupFleet(lpflFr->id, &sel.fl);
            } else if ((dx <= dy ? dy : dx) <= 200) {
                if (fNeedy == 2) {
                    wtPlanCargo = lpth->thp.rgwtMin[iWorst];
                } else {
                    wtPlanCargo = 0;
                    for (i = 0; i < 3; i++) {
                        if (fNeedy && i != iWorst) {
                            wtPlanCargo += (int16_t)(lpth->thp.rgwtMin[i] >> 1);
                        } else {
                            wtPlanCargo += lpth->thp.rgwtMin[i];
                        }
                    }
                }
                if (wtPlanCargo >= 10) {
                    pctHere = LOWORD((int32_t)((int32_t)(wtPlanCargo * 100) / wtCargoMax));
                    if (pctHere > 100 - pctFull) {
                        pctHere = 100 - pctFull;
                    }
                    l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                    l = ((int32_t)sqrt((double)l) + 24) / 25;
                    if (l < 1) {
                        l = 1;
                    }
                    score = (int32_t)((int16_t)(100 * pctHere) / l);
                    if (score > scoreBest) {
                        scoreBest = score;
                        *plpthBest = lpth;
                        *pidBest = lpth->idFull;
                    }
                }
            }
        }
    }
    return fSalvage;
}

int16_t FMoveAiFleet(FLEET *lpfl, ORDER *pord, int16_t fAppend) {
    int16_t iord;

    ChangeMainObjSel(grobjFleet, lpfl->id);
    if (sel.fl.lpplord->iordMax == sel.fl.cord) {
        sel.fl.lpplord = (PLORD *)LpplReAlloc((PL *)sel.fl.lpplord, sel.fl.cord + 1);
    }
    iord = !fAppend ? 0 : sel.fl.cord - 1;
    if (pord->pt.x != sel.fl.lpplord->rgord[iord].pt.x || pord->pt.y != sel.fl.lpplord->rgord[iord].pt.y) {
        iord++;
    } else if (sel.fl.cord > 1) {
        sel.fl.cord = 1;
        FLookupFleet(idWriteBack, &sel.fl);
    }
    sel.fl.lpplord->rgord[iord] = *pord;
    sel.fl.cord = iord + 1;
    sel.fl.lpplord->iordMac = iord + 1;
    return FLookupFleet(idWriteBack, &sel.fl);
}

void AddItemToQueue(uint16_t iItem, uint16_t cItem, GrobjClass grobj, AddItemMode mdAddItem) {
    int16_t fSingle;
    int16_t iprod;
    int16_t i;
    PROD    rgprod[64];

    fSingle = FALSE;
    if (!sel.pl.lpplprod || sel.pl.lpplprod->iprodMac <= 200) {
        if (!lpplProdGlob) {
            fSingle = TRUE;
            InitProduction(rgprod);
            for (i = 0; i < cProdGlob && (rgprod[i].iItem != (uint32_t)iItem || rgprod[i].grobj != (uint32_t)grobj); i++) {
            }
            if (i >= cProdGlob) {
                cItem = 0;
            } else {
                cItem = cItem >= rgprod[i].cItem ? rgprod[i].cItem : cItem;
            }
        }
        if (cItem > 0) {
            if (mdAddItem != addItemReplace && lpplProdGlob->iprodMac == lpplProdGlob->iprodMax) {
                lpplProdGlob = (PLPROD *)LpplReAlloc((PL *)lpplProdGlob, lpplProdGlob->iprodMac + 3);
            }
            switch (mdAddItem) {
            case addItemReplace:
                lpplProdGlob->iprodMac = 0;
                /* fallthrough */
            case addItemEnd:
                iprod = lpplProdGlob->iprodMac;
                break;
            case addItemFront:
                iprod = 0;
                memmove(&lpplProdGlob->rgprod[1], lpplProdGlob->rgprod, lpplProdGlob->iprodMac * sizeof(PROD));
                memset(lpplProdGlob->rgprod, 0, 4);
            }
            lpplProdGlob->rgprod[iprod].cItem = cItem;
            lpplProdGlob->rgprod[iprod].iItem = iItem;
            lpplProdGlob->rgprod[iprod].grobj = grobj;
            lpplProdGlob->rgprod[iprod].pct = 0;
            lpplProdGlob->rgprod[iprod].unused = 0;
            lpplProdGlob->iprodMac++;
        }
        if (fSingle) {
            FinishProduction(cItem != 0);
        }
    }
    return;
}

int16_t IroEnsureAi(uint8_t *lpbRes, int16_t cRes, int16_t *pishdefSBLatest, int16_t pct) {
    int16_t iSmallest;
    int16_t i;
    int16_t pctTech;
    int16_t ilvl;

    if (pishdefSBLatest) {
        EnsureAiStarbaseDesigns();
        *pishdefSBLatest = IshdefAiSBLatest();
        ValidateStarbaseHistory();
    }
    rgplr[idPlayer].pctResearch = pct;
    for (i = 0; i < 6 && rgplr[idPlayer].rgTech[i] >= 24; i++) {
    }
    if (i == 6) {
        rgplr[idPlayer].pctResearch = 0;
        pctTech = rgplr[idPlayer].iTechCur * 256;
        WriteMemRt(rtLogResearch, 2, &pctTech);
    }
    for (i = 0; i < cRes; i++) {
        ilvl = rgplr[idPlayer].rgTech[lpbRes[i] >> 5];
        if (ilvl < (lpbRes[i] & 0x1f)) {
            rgplr[idPlayer].iTechNow = lpbRes[i] >> 5;
            if (i < cRes - 1 && ilvl + 1 == (lpbRes[i] & 0x1f)) {
                rgplr[idPlayer].iTechNext = lpbRes[i + 1] >> 5;
            }
            pctTech = rgplr[idPlayer].pctResearch + rgplr[idPlayer].iTechCur * 256;
            WriteMemRt(rtLogResearch, 2, &pctTech);
            return i;
        }
    }
    iSmallest = 0;
    for (i = 1; i < 6; i++) {
        if (rgplr[idPlayer].rgTech[i] < rgplr[idPlayer].rgTech[iSmallest]) {
            iSmallest = i;
        }
    }
    if (rgplr[idPlayer].iTechNow != iSmallest) {
        rgplr[idPlayer].iTechNow = iSmallest;
        if (rgplr[idPlayer].iTechNow == 0x1a) {
            rgplr[idPlayer].pctResearch = 0;
        }
        pctTech = rgplr[idPlayer].pctResearch + rgplr[idPlayer].iTechCur * 256;
        WriteMemRt(rtLogResearch, 2, &pctTech);
    }
    return 926;
}

void KeepFleetsMoving() {
    int16_t i;
    int16_t ifl;
    FLEET  *lpfl;
    THING  *lpth;
    THING  *rglpth[100];
    int16_t ith;
    THING  *lpthMac;

    ith = 0;
    lpth = lpThings;
    lpthMac = lpThings + cThing;
    for (; lpth < lpthMac; lpth++) {
        if (lpth->ith == ithMinefield && lpth->iplr != idPlayer && lpth->thm.iType != mineSpeedBump) {
            if (ith == 100) {
                ith = -1;
                goto LKeepMovn;
            }
            rglpth[ith++] = lpth;
        }
    }
LKeepMovn:
    for (ifl = 0; ifl < cFleet; ifl++) {
        lpfl = rglpfl[ifl];
        if (!rglpfl[ifl])
            break;
        if (lpfl->iPlayer == idPlayer && lpfl->cord > 1) {
            if (rgplr[idPlayer].idAi != idAiRobotoid && game.turn < 5) {
                i = 16;
            } else {
                i = -1;
                if (FIsAiTransport(lpfl)) {
                    i = 30;
                }
            }
            SetAiFleetIdealSpeed(lpfl, i, ith, rglpth);
        }
    }
    return;
}

int16_t FShouldWeBuildColonizers(int16_t *pcCol) {
    int16_t  iMax;
    int16_t  cColFl;
    uint32_t cBuilt;
    int16_t  iMin;
    int16_t  ifl;
    int16_t  i;
    FLEET   *lpfl;
    int16_t  rgish[16];
    int16_t  cColPl;

    cColFl = 0;
    cColPl = 0;
    cBuilt = 0;
    iMax = -1;
    iMin = 16;
    if (pcCol) {
        *pcCol = 0;
    }
    if (rgplr[idPlayer].lvlAi == lvlAiEasy && (game.turn & 1)) {
        return FALSE;
    }
    if (game.turn < 30) {
        return TRUE;
    }
    for (i = 0; i < 16; i++) {
        if (!rgshdef[i].fFree && (rgshdef[i].hul.ihuldef == ihuldefMiniColonyShip || rgshdef[i].hul.ihuldef == ihuldefColonyShip)) {
            rgish[i] = 1;
            if (i > iMax) {
                iMax = i;
            }
            if (i < iMin) {
                iMin = i;
            }
        } else {
            rgish[i] = 0;
        }
    }
    iMax++;
    if (iMax <= 0) {
        return FALSE;
    }
    for (ifl = 0; ifl < cFleet; ifl++) {
        lpfl = rglpfl[ifl];
        if (!rglpfl[ifl])
            break;
        if (lpfl->iPlayer == idPlayer) {
            for (i = iMin; i < iMax; i++) {
                if (rgish[i] > 0 && lpfl->rgcsh[i] > 0) {
                    cColFl++;
                    break;
                }
            }
        }
    }
    if (pcCol) {
        *pcCol = cColFl;
    }
    if (cColFl > 20 * game.mdSize + 10) {
        return FALSE;
    }
    for (i = 0; i < game.cPlayer; i++) {
        if (rgplr[i].fInclude) {
            cColPl += rgplr[i].cPlanet;
        }
    }
    if (cColFl + cColPl > (int16_t)(game.cPlanMax * 4) / 5) {
        return FALSE;
    }
    cBuilt = 0;
    for (i = iMin; i < iMax; i++) {
        if (rgish[i] > 0) {
            cBuilt += rgshdef[i].cBuilt;
        }
    }
    if (cBuilt - (int16_t)(cColPl + cColFl) > 25 && (cColFl > 4 || Random(2) != 0)) {
        return FALSE;
    }
    return TRUE;
}

int16_t FIsAiAttack(FLEET *lpfl) {
    HulDef  ihul;
    int16_t i;

    for (i = 0; i < 16; i++) {
        if (lpfl->rgcsh[i] > 0) {
            ihul = rgshdef[i].hul.ihuldef;
            if ((int16_t)ihul > ihuldefFrigate && (int16_t)ihul <= ihuldefDreadnought) {
                return TRUE;
            }
            switch (ihul) {
            case ihuldefFrigate:
                if (rglpshdef[idPlayer][i].lPower > 0) {
                    return TRUE;
                }
                return FALSE;
            case ihuldefMetaMorph:
            case ihuldefNubian:
                if (WtMaxShdefStat(&rgshdef[i], 2) < 500 && rglpshdef[idPlayer][i].lPower > 0) {
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

int16_t FIsTurinDroneAiAttack(FLEET *lpfl) {
    HulDef  ihul;
    int16_t i;

    for (i = 0; i < 16; i++) {
        if (lpfl->rgcsh[i] > 0) {
            ihul = rgshdef[i].hul.ihuldef;
            if ((int16_t)ihul >= ihuldefScout && (int16_t)ihul <= ihuldefDreadnought) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

int16_t FIsAiTransport(FLEET *lpfl) {
    HulDef  ihul;
    int16_t i;

    for (i = 0; i < 16; i++) {
        if (lpfl->rgcsh[i] > 0) {
            ihul = rgshdef[i].hul.ihuldef;
            if (((int16_t)ihul >= ihuldefSmallFreighter && (int16_t)ihul <= ihuldefSuperFreighter) ||
                ((int16_t)ihul >= ihuldefPrivateer && (int16_t)ihul <= ihuldefGalleon)) {
                return TRUE;
            }
            if (ihul == ihuldefMetaMorph && WtMaxShdefStat(&rgshdef[i], 2) >= 500 && rglpshdef[idPlayer][i].lPower > 0) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

void ValidateStarbaseHistory() {
    int16_t iWrite;
    int16_t iBest;
    PLANET *lpplMac;
    POINT16 pt;
    int16_t id;
    int16_t dy;
    int16_t cFr2;
    PLANET *lppl;
    int16_t ifl;
    FLEET  *lpfl;
    int16_t i;
    int16_t j;
    int16_t ipl;
    int16_t cFr;
    int16_t dx;
    int32_t lBest;
    int32_t l;

    if (rgplr[idPlayer].idAi != idAiCybertron && rgplr[idPlayer].idAi != idAiMacinti) {
        if (((AIHIST *)vlpbAiData)->cbAiHist <= 2) {
            ((AIHIST *)vlpbAiData)->cStarbase = 0;
        }
        if (game.turn >= 20) {
            iWrite = 0;
            if (((AIHIST *)vlpbAiData)->cStarbase < 0 || ((AIHIST *)vlpbAiData)->cStarbase > 64) {
                ((AIHIST *)vlpbAiData)->cStarbase = 0;
            }
            for (i = 0; i < ((AIHIST *)vlpbAiData)->cStarbase; i++) {
                if (((AIHIST *)vlpbAiData)->rgasb[i].cFreighter < 0 || ((AIHIST *)vlpbAiData)->rgasb[i].cFreighter > 8) {
                    ((AIHIST *)vlpbAiData)->rgasb[i].cFreighter = 0;
                }
                lppl = LpplFromId(((AIHIST *)vlpbAiData)->rgasb[i].idPlanet);
                if (lppl && lppl->iPlayer == idPlayer) {
                    if (iWrite != i) {
                        ((AIHIST *)vlpbAiData)->rgasb[iWrite] = ((AIHIST *)vlpbAiData)->rgasb[i];
                    }
                    iWrite++;
                }
            }
            ((AIHIST *)vlpbAiData)->cStarbase = iWrite;
            for (ipl = 0; ipl < vclpplAi; ipl++) {
                lppl = vrglpplAi[ipl];
                if (!vrglpplAi[ipl])
                    break;
                if (lppl->fStarbase) {
                    for (i = 0; i < ((AIHIST *)vlpbAiData)->cStarbase && ((AIHIST *)vlpbAiData)->rgasb[i].idPlanet != lppl->id; i++) {
                    }
                    if (i == ((AIHIST *)vlpbAiData)->cStarbase && i < 64) {
                        ((AIHIST *)vlpbAiData)->rgasb[i].idPlanet = lppl->id;
                        ((AIHIST *)vlpbAiData)->rgasb[i].cFreighter = 0;
                        ((AIHIST *)vlpbAiData)->cStarbase++;
                    }
                }
            }
            lppl = lpPlanets;
            lpplMac = lpPlanets + cPlanet;
            for (; lppl < lpplMac && ((AIHIST *)vlpbAiData)->cStarbase < 64; lppl++) {
                if (lppl->iPlayer == idPlayer && !lppl->fStarbase && lppl->rgwtMin[3] >= 80) {
                    l = 0;
                    for (i = 0; i < 3; i++) {
                        l += (int32_t)((uint32_t)((uint32_t)lppl->rgMinConc[i] * (uint32_t)lppl->rgMinConc[i]) * 4) + lppl->rgwtMin[i];
                    }
                    if (l >= 7000 && lppl->cMines >= 20 && lppl->cFactories >= 20) {
                        for (i = 0; i < ((AIHIST *)vlpbAiData)->cStarbase; i++) {
                            id = ((AIHIST *)vlpbAiData)->rgasb[i].idPlanet;
                            if (id == lppl->id)
                                break;
                            if (rgplr[idPlayer].idAi == idAiRobotoid) {
                                pt = rgptPlan[id];
                                dx = pt.x - rgptPlan[lppl->id].x;
                                dy = pt.y - rgptPlan[lppl->id].y;
                                l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                                if (l < 2500)
                                    break;
                            }
                        }
                        if (i >= ((AIHIST *)vlpbAiData)->cStarbase) {
                            ((AIHIST *)vlpbAiData)->rgasb[((AIHIST *)vlpbAiData)->cStarbase].idPlanet = lppl->id;
                            ((AIHIST *)vlpbAiData)->rgasb[((AIHIST *)vlpbAiData)->cStarbase].cFreighter = 0;
                            ((AIHIST *)vlpbAiData)->cStarbase++;
                        }
                    }
                }
            }
            for (ifl = 0; ifl < cFleet; ifl++) {
                lpfl = rglpfl[ifl];
                if (!rglpfl[ifl])
                    break;
                if (lpfl->iPlayer == idPlayer && FIsAiTransport(lpfl)) {
                    for (i = 0; i < ((AIHIST *)vlpbAiData)->cStarbase; i++) {
                        for (j = 0; j < ((AIHIST *)vlpbAiData)->rgasb[i].cFreighter && ((AIHIST *)vlpbAiData)->rgasb[i].rgflid[j] != lpfl->id; j++) {
                        }
                        if (j < ((AIHIST *)vlpbAiData)->rgasb[i].cFreighter)
                            break;
                    }
                    if (i == ((AIHIST *)vlpbAiData)->cStarbase) {
                        iBest = -1;
                        lBest = 10000000;
                        for (i = 0; i < ((AIHIST *)vlpbAiData)->cStarbase; i++) {
                            if (((AIHIST *)vlpbAiData)->rgasb[i].cFreighter < 8) {
                                pt = rgptPlan[((AIHIST *)vlpbAiData)->rgasb[i].idPlanet];
                                dx = pt.x - lpfl->pt.x;
                                dy = pt.y - lpfl->pt.y;
                                l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                                if (l < lBest) {
                                    lBest = l;
                                    iBest = i;
                                }
                            }
                        }
                        if (iBest != -1) {
                            ((AIHIST *)vlpbAiData)->rgasb[iBest].rgflid[((AIHIST *)vlpbAiData)->rgasb[iBest].cFreighter++] = lpfl->id;
                        }
                    }
                }
            }
            for (i = 0; i < ((AIHIST *)vlpbAiData)->cStarbase; i++) {
                cFr = ((AIHIST *)vlpbAiData)->rgasb[i].cFreighter;
                if (cFr < 4) {
                    for (j = 0; j < ((AIHIST *)vlpbAiData)->cStarbase; j++) {
                        cFr2 = ((AIHIST *)vlpbAiData)->rgasb[j].cFreighter;
                        if (cFr2 >= cFr + 2)
                            break;
                    }
                    if (j < ((AIHIST *)vlpbAiData)->cStarbase) {
                        ((AIHIST *)vlpbAiData)->rgasb[j].cFreighter--;
                        ((AIHIST *)vlpbAiData)->rgasb[i].rgflid[((AIHIST *)vlpbAiData)->rgasb[i].cFreighter++] =
                            ((AIHIST *)vlpbAiData)->rgasb[j].rgflid[((AIHIST *)vlpbAiData)->rgasb[j].cFreighter];
                    }
                }
            }
            ((AIHIST *)vlpbAiData)->cbAiHist = ((AIHIST *)vlpbAiData)->cStarbase * 20 + 4;
        }
    }
    return;
}

void GetResourcesAvailable(PLANET *lppl, int32_t *rgRes) {
    int16_t i;
    int32_t cRes;

    EstMineralsMined(lppl, rgRes, -1, FALSE);
    for (i = 0; i < 3; i++) {
        rgRes[i] += lppl->rgwtMin[i];
    }
    cRes = CResourcesAtPlanet(lppl, idPlayer);
    if (!sel.pl.fNoResearch) {
        cRes -= (int32_t)(cRes * (int16_t)rgplr[idPlayer].pctResearch) / 100;
    }
    rgRes[3] = cRes;
    return;
}

void GetProdQCost(PLANET *lppl, int32_t *rgCost) {
    int32_t rgCostCur[4];
    PLPROD *lpplprod;
    int16_t i;
    int16_t j;
    PROD   *lpprod;

    for (i = 0; i < 4; i++) {
        rgCost[i] = 0;
    }
    if (lppl->lpplprod) {
        lpplprod = lppl->lpplprod;
        i = 0;
        lpprod = lpplprod->rgprod;
        while (i < lpplprod->iprodMac) {
            GetProductionCosts(lppl, lpprod, rgCostCur, idPlayer, FALSE);
            for (j = 0; j < 4; j++) {
                rgCost[j] += rgCostCur[j];
            }
            i++;
            lpprod++;
        }
    }
    return;
}

void MergeAllShdefs(int16_t grbitish) {
    int16_t crglpflW;
    FLEET  *rglpflW[32];
    int16_t iMax;
    int16_t iMin;
    int16_t i;
    int16_t ifl;
    FLEET  *lpfl;
    int16_t grbit;
    int16_t rgish[16];
    FLEET  *lpflNextPass;
    int16_t iflNextPass;

    iMax = -1;
    iMin = 16;
    i = 0;
    grbit = 1;
    while (i < 16) {
        if (grbitish & grbit) {
            rgish[i] = 1;
            if (i > iMax) {
                iMax = i;
            }
            if (i < iMin) {
                iMin = i;
            }
        } else {
            rgish[i] = 0;
        }
        i++;
        grbit *= 2;
    }
    iMax++;
    if (iMax > 0) {
        lpflNextPass = NULL;
        crglpflW = 0;
        for (ifl = 0; ifl < cFleet; ifl++) {
            lpfl = rglpfl[ifl];
            if (!rglpfl[ifl])
                break;
        NextPass:
            if (lpfl->iPlayer == idPlayer) {
                for (i = iMin; i < iMax && (lpfl->rgcsh[i] <= 0 || rgish[i] <= 0); i++) {
                }
                if (i != iMax && CMineFromLpfl(lpfl) != 4000) {
                    for (i = 0; i < crglpflW && (rglpflW[i]->idPlanet != lpfl->idPlanet || rglpflW[i]->pt.x != lpfl->pt.x || rglpflW[i]->pt.y != lpfl->pt.y);
                         i++) {
                    }
                    if (i < crglpflW) {
                        Merge2Fleets(rglpflW[i], lpfl, FALSE);
                    } else if (crglpflW != 32) {
                        rglpflW[crglpflW++] = lpfl;
                    } else if (!lpflNextPass) {
                        lpflNextPass = lpfl;
                        iflNextPass = ifl;
                    }
                }
            }
        }
        if (lpflNextPass) {
            lpfl = lpflNextPass;
            lpflNextPass = NULL;
            ifl = iflNextPass;
            crglpflW = 0;
            goto NextPass;
        }
    }
    return;
}

FLEET *LpflFindClosestEnum(FLEET *lpfl, int16_t (*pfn)(FLEET *, FLEET *)) {
    FLEET  *lpflT;
    POINT16 pt;
    int16_t dy;
    int16_t ish;
    int16_t dx;
    FLEET  *lpflBest;
    int32_t l;
    int32_t lBest;

    lBest = 10000000;
    lpflBest = NULL;
    pt = lpfl->pt;
    for (ish = 0; ish < cFleet; ish++) {
        lpflT = rglpfl[ish];
        if (!rglpfl[ish])
            break;
        dx = pt.x - lpflT->pt.x;
        dy = pt.y - lpflT->pt.y;
        l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
        if (l < lBest && pfn(lpfl, lpflT) != 0) {
            lpflBest = lpflT;
            lBest = l;
        }
    }
    return lpflBest;
}

PLANET *LpplFindClosestEnum(PLANET *lppl, int16_t (*pfn)(PLANET *, PLANET *)) {
    POINT16 pt;
    int16_t dy;
    PLANET *lpplTMac;
    PLANET *lpplBest;
    PLANET *lpplT;
    int16_t dx;
    int32_t l;
    int32_t lBest;

    lBest = 10000000;
    lpplBest = NULL;
    pt = rgptPlan[lppl->id];
    lpplT = lpPlanets;
    lpplTMac = lpPlanets + cPlanet;
    for (; lpplT < lpplTMac; lpplT++) {
        dx = pt.x - rgptPlan[lpplT->id].x;
        dy = pt.y - rgptPlan[lpplT->id].y;
        l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
        if (l < lBest && pfn(lppl, lpplT) != 0) {
            lpplBest = lpplT;
            lBest = l;
        }
    }
    return lpplBest;
}

PLANET *LpplFindBestEnum(PLANET *lppl, int16_t (*pfn)(PLANET *, PLANET *)) {
    int16_t iBest;
    POINT16 pt;
    int16_t dy;
    PLANET *lpplTMac;
    PLANET *lpplBest;
    PLANET *lpplT;
    int16_t iCur;
    int16_t dx;
    int32_t l;
    int32_t lBest;

    iBest = 1;
    lBest = 10000000;
    lpplBest = NULL;
    pt = rgptPlan[lppl->id];
    lpplT = lpPlanets;
    lpplTMac = lpPlanets + cPlanet;
    for (; lpplT < lpplTMac; lpplT++) {
        dx = pt.x - rgptPlan[lpplT->id].x;
        dy = pt.y - rgptPlan[lpplT->id].y;
        l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
        iCur = pfn(lppl, lpplT);
        if (iCur > iBest || (iCur == iBest && l < lBest)) {
            iBest = iCur;
            lBest = l;
            lpplBest = lpplT;
        }
    }
    return lpplBest;
}

int16_t IdRandomPlanetNearby(POINT16 pt, int16_t cDist, int16_t fAvoidStarbases) {
    int16_t iChance;
    int32_t lDistMax;
    int32_t dy;
    int16_t idBest;
    int16_t i;
    int32_t dx;
    int16_t cExtraAttempts;
    int32_t d2Cur;
    PLANET *lppl;

    cExtraAttempts = !fAvoidStarbases ? 0 : 2;
LRetry:
    lDistMax = (uint32_t)(cDist * cDist);
    iChance = 1;
    idBest = idplNone;
    for (i = 0; i < game.cPlanMax; i++) {
        dx = abs(pt.x - rgptPlan[i].x);
        dy = abs(pt.y - rgptPlan[i].y);
        d2Cur = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
        if (d2Cur <= lDistMax && Random(iChance++) == 0) {
            idBest = i;
        }
    }
    if (idBest != idplNone && cExtraAttempts-- > 0) {
        lppl = LpplFromId(idBest);
        if (lppl && lppl->fStarbase)
            goto LRetry;
    }
    return idBest;
}

void ClearAiCurrentTask(FLEET *lpfl, int16_t fChangeSel) {
    if (fChangeSel) {
        ChangeMainObjSel(grobjFleet, lpfl->id);
    }
    sel.fl.lpplord->rgord[0].grTask = grTaskNone;
    FLookupFleet(idWriteBack, &sel.fl);
    return;
}

int16_t FEnumOurStarbase(PLANET *lpplSrc, PLANET *lpplTest) {
    if (lpplTest->iPlayer == idPlayer && lpplTest->fStarbase) {
        return TRUE;
    }
    return FALSE;
}

int16_t FFleetMightHaveTeeth(FLEET *lpfl) {
    HUL    *lphul;
    int16_t ishdef;

    for (ishdef = 0; ishdef < 16; ishdef++) {
        if (lpfl->rgcsh[ishdef] != 0) {
            lphul = &rglpshdef[lpfl->iplr][ishdef].hul;
            if (FHullHasTeeth(lphul)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

int16_t IdTargetScout(FLEET *lpfl, FLEET *lpflAtk, FLEET *lpflEnemy, int16_t fOnlyHumans, THING **plpthWorm) {
    FLEET  *lpflClosest;
    FLEET  *lpflT;
    int32_t lDistBest;
    PLANET *lpplMac;
    POINT16 pt;
    int16_t dy;
    PLANET *lppl;
    int32_t lDist;
    int16_t idClosest;
    PLANET *lpplClosest;
    int16_t dx;
    FLEET  *lpflAtk2;
    ORDER   ord;

    lpflClosest = NULL;
    lpflT = lpflEnemy;
    lDistBest = 1000000;
    pt = lpfl->pt;
    memset(&ord, 0, sizeof(ORDER));
    if (!FFleetMightHaveTeeth(lpfl))
        goto LFindPlanet;
    for (; lpflT; lpflT = lpflT->lpflNext) {
        if (fOnlyHumans && rgplr[lpflT->iPlayer].fAi)
            goto NextTarg;
        for (lpflAtk2 = lpflAtk; lpflAtk2 && (lpflAtk2 == lpfl || lpflAtk2->cord < 2 || lpflAtk2->lpplord->rgord[1].id != lpflT->id ||
                                              lpflAtk2->lpplord->rgord[1].grobj != grobjFleet);
             lpflAtk2 = lpflAtk2->lpflNext) {
        }
        if (lpflAtk2 && Random(3) == 0)
            goto NextTarg;
        dx = lpflT->pt.x - pt.x;
        dy = lpflT->pt.y - pt.y;
        lDist = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
        if (lDist < lDistBest) {
            lDistBest = lDist;
            lpflClosest = lpflT;
        }
    NextTarg:;
    }
    if (lDistBest < 32400) {
        ord.pt = lpflClosest->pt;
        ord.grobj = grobjFleet;
        ord.id = lpflClosest->id;
        goto ThwakSumthin;
    }
    if (lpflClosest) {
        if (lpfl->rgwtMin[4] < (int32_t)(LGetFleetStat(lpfl, 1) / 2) && FMoveToNearestStarbase(lpfl, FALSE)) {
            return 0;
        }
        pt = lpflClosest->pt;
        lpplClosest = NULL;
        lDistBest = 1000000;
        lppl = lpPlanets;
        lpplMac = lpPlanets + cPlanet;
        for (; lppl < lpplMac; lppl++) {
            for (lpflAtk2 = lpflAtk; lpflAtk2 && (lpflAtk2 == lpfl || lpflAtk2->cord < 2 || lpflAtk2->lpplord->rgord[1].id != lppl->id ||
                                                  lpflAtk2->lpplord->rgord[1].grobj != grobjPlanet);
                 lpflAtk2 = lpflAtk2->lpflNext) {
            }
            if (!lpflAtk2) {
                dx = rgptPlan[lppl->id].x - pt.x;
                dy = rgptPlan[lppl->id].y - pt.y;
                lDist = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                if (lDist < lDistBest) {
                    lDistBest = lDist;
                    lpplClosest = lppl;
                }
            }
        }
        if (lpplClosest && lpplClosest->id != lpfl->idPlanet) {
            ord.pt = rgptPlan[lpplClosest->id];
            ord.grobj = grobjPlanet;
            ord.id = lpplClosest->id;
            goto ThwakSumthin;
        }
    } else if (fOnlyHumans) {
        return IdTargetScout(lpfl, lpflAtk, lpflEnemy, FALSE, plpthWorm);
    }
LFindPlanet:
    idClosest = IdNearestUnknownPlanet(lpfl, plpthWorm);
    if (plpthWorm && *plpthWorm) {
        ord.pt = (*plpthWorm)->pt;
        ord.grobj = grobjThing;
        ord.id = (*plpthWorm)->idFull;
    } else {
        if (idClosest == idplNone) {
            if (rgplr[idPlayer].idPlanetHome == idplNone) {
                idClosest = Random(game.cPlanMax);
            } else {
                lpplClosest = LpplFindBestEnum(LpplFromId(rgplr[idPlayer].idPlanetHome), FEnumCalcArmadaDest);
                if (!lpplClosest) {
                    idClosest = Random(game.cPlanMax);
                } else {
                    idClosest = lpplClosest->id;
                }
            }
        }
        ord.id = idClosest;
        ord.grobj = grobjPlanet;
        ord.pt = rgptPlan[idClosest];
    }
ThwakSumthin:
    if (lpfl->cord > 1 && lpfl->lpplord->rgord[1].grobj == grobjPlanet && ord.grobj == grobjPlanet) {
        return -1;
    }
    if (ord.grobj == grobjPlanet) {
        vlpbAiPlanet[ord.id * 16 + 15] = 4;
    }
    ord.grTask = grTaskNone;
    ord.fValidTask = TRUE;
    ord.iWarp = (uint16_t)IFindIdealWarp(lpfl, TRUE);
    if (!FMoveAiFleet(lpfl, &ord, FALSE)) {
        return -1;
    }
    return 0;
}

void MarkPlanetsUnderAttack() {
    PLANET *lppl;
    int16_t i;
    int16_t ifl;
    FLEET  *lpfl;
    int16_t j;

    for (ifl = 0; ifl < cFleet; ifl++) {
        lpfl = rglpfl[ifl];
        if (!rglpfl[ifl])
            break;
        if (lpfl->iPlayer != idPlayer && lpfl->idPlanet != idPlanetDeepSpace) {
            j = lpfl->iPlayer;
            for (i = 0; i < 16 && (lpfl->rgcsh[i] <= 0 || (int16_t)rglpshdef[j][i].hul.ihuldef < ihuldefMiniBomber ||
                                   (int16_t)rglpshdef[j][i].hul.ihuldef > ihuldefB52Bomber);
                 i++) {
            }
            if (i != 16) {
                lppl = LpplFromId(lpfl->idPlanet);
                if (lppl->iPlayer == idPlayer) {
                    vlpbAiPlanet[lppl->id * 16 + 8] = 1;
                }
            }
        }
    }
    return;
}

void FixPlanetsUnderAttack(PROD *rgprod) {
    PLANET *lppl;
    int16_t ipl;

    for (ipl = 0; ipl < vclpplAi; ipl++) {
        lppl = vrglpplAi[ipl];
        if (!vrglpplAi[ipl])
            break;
        if (vlpbAiPlanet[lppl->id * 16 + 8] != 0) {
            QuickBuildDefenses(lppl, rgprod);
        }
    }
    return;
}

void QuickBuildDefenses(PLANET *lppl, PROD *rgprod) {
    int16_t cMax;
    int16_t cAlch;
    int16_t cCur;
    int16_t i;
    int16_t cRes;
    int32_t lVal;
    int16_t cDef;
    int32_t rgRes[4];
    PROD   *lpprod;

    if (lppl->lpplprod) {
        i = 0;
        lpprod = lppl->lpplprod->rgprod;
        while (i < lppl->lpplprod->iprodMac) {
            if (lpprod->grobj == grobjPlanet && lpprod->iItem == mdIdleDefense) {
                return;
            }
            i++;
            lpprod++;
        }
    }
    GetResourcesAvailable(lppl, rgRes);
    if (rgRes[3] >= 50) {
        ChangeMainObjSel(grobjPlanet, lppl->id);
        InitProduction(rgprod);
        for (i = 0; i < cProdGlob && (pProdGlob[i].grobj != grobjPlanet || pProdGlob[i].iItem != mdIdleDefense || pProdGlob[i].cItem < 1); i++) {
        }
        if (i == cProdGlob) {
            FinishProduction(FALSE);
        } else {
            cMax = pProdGlob[i].cItem;
            cCur = 100;
            for (i = 0; i < 3; i++) {
                lVal = (int32_t)(rgRes[i] / 5);
                if (lVal < cCur) {
                    cCur = LOWORD(lVal);
                }
            }
            cRes = LOWORD((int32_t)(rgRes[3] / 25));
            if (cRes > 5) {
                cRes -= cRes / 6;
            }
            rgRes[3] -= (int32_t)(rgRes[3] / 10);
            if (cRes > cMax) {
                cRes = cMax;
            }
            if (cRes > cCur) {
                cAlch = LOWORD((int32_t)((rgRes[3] - (int16_t)((GetRaceGrbit(&rgplr[idPlayer], ibitRaceMineralAlchemy) == 0 ? 100 : 25) * cCur)) / 150));
                if (cAlch < 0) {
                    cAlch = 0;
                }
                cDef = cCur + cAlch;
                cAlch = 5 * cAlch;
            } else {
                cAlch = 0;
                cDef = cRes;
            }
            if (cDef > 0) {
                AddItemToQueue(mdIdleDefense, cDef, grobjPlanet, addItemFront);
            }
            if (cAlch > 0) {
                AddItemToQueue(mdIdleAlchemy, cAlch, grobjPlanet, addItemFront);
            }
            FinishProduction(cDef > 0 || cAlch > 0);
        }
    }
    return;
}

int16_t IdplFindClosestStarbase(FLEET *lpfl, int16_t fBigOnes) {
    POINT16 pt;
    int16_t dy;
    PLANET *lpplTMac;
    PLANET *lpplBest;
    PLANET *lpplT;
    int16_t dx;
    int32_t l;
    int32_t lBest;

    lBest = 10000000;
    lpplBest = NULL;
    pt = lpfl->lpplord->rgord[0].pt;
    lpplT = lpPlanets;
    lpplTMac = lpPlanets + cPlanet;
    for (; lpplT < lpplTMac; lpplT++) {
        dx = pt.x - rgptPlan[lpplT->id].x;
        dy = pt.y - rgptPlan[lpplT->id].y;
        l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
        if (l < lBest && lpplT->iPlayer == idPlayer && lpplT->fStarbase && (!fBigOnes || lpplT->rgwtMin[3] > 250)) {
            lpplBest = lpplT;
            lBest = l;
        }
    }
    if (!lpplBest) {
        return idplNone;
    }
    return lpplBest->id;
}

int16_t FMoveToNearestStarbase(FLEET *lpfl, int16_t fBigOnes) {
    int16_t id;
    ORDER   ord;

    id = IdplFindClosestStarbase(lpfl, fBigOnes);
    if (id == idplNone) {
        return FALSE;
    }
    ord.id = id;
    ord.grobj = grobjPlanet;
    ord.pt = rgptPlan[id];
    ord.grTask = grTaskNone;
    ord.fValidTask = TRUE;
    ord.iWarp = 4;
    return FMoveAiFleet(lpfl, &ord, FALSE);
}

void MoveToNearestPlanetOrEnemy(FLEET *lpfl, int16_t dEnemyRange) {
    POINT16 pt;
    int16_t id;
    int16_t dy;
    PLANET *lpplTMac;
    PLANET *lpplBest;
    PLANET *lpplT;
    int16_t dx;
    ORDER   ord;
    int32_t l;
    int32_t lBest;
    SCAN    scan;

    lBest = 10000000;
    lpplBest = NULL;
    pt = lpfl->lpplord->rgord[0].pt;
    lpplT = lpPlanets;
    lpplTMac = lpPlanets + cPlanet;
    for (; lpplT < lpplTMac; lpplT++) {
        dx = pt.x - rgptPlan[lpplT->id].x;
        dy = pt.y - rgptPlan[lpplT->id].y;
        l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
        if (l < lBest && lpplT->iPlayer != idPlayer && lpplT->iPlayer != iplrNone) {
            lpplBest = lpplT;
            lBest = l;
        }
    }
    if (lpplBest && lBest <= (int32_t)(uint32_t)(dEnemyRange * dEnemyRange)) {
        id = lpplBest->id;
    } else {
        if (!FFindNearestObject(lpfl->pt, grobjPlanet | mdNoRecurse, &scan)) {
            return;
        }
        id = scan.idpl;
    }
    if (lpfl->idPlanet != id) {
        ord.id = id;
        ord.grobj = grobjPlanet;
        ord.pt = rgptPlan[id];
        ord.grTask = grTaskNone;
        ord.fValidTask = TRUE;
        ord.iWarp = 4;
        FMoveAiFleet(lpfl, &ord, FALSE);
    }
    return;
}

void EnsureAiStarbaseDesigns() {
    uint16_t wTurnLast;
    int16_t  iSetNew;
    int16_t  i;
    int16_t  iSetLast;

    if (rglpshdefSB[idPlayer][2].fFree) {
        FCreateAiStarbase(2, 2, aiSbRecipeAuto, isbhullAuto);
        FCreateAiStarbase(4, 3, aiSbRecipeAuto, isbhullAuto);
    }
    if (rglpshdefSB[idPlayer][1].fFree) {
        FCreateAiStarbase(1, 1, aiSbRecipeAuto, isbhullAuto);
    }
    if (rglpshdefSB[idPlayer][3].fFree) {
        FCreateAiStarbase(3, 2, aiSbRecipeAuto, isbhullAuto);
    }
    if (game.turn >= 50) {
        iSetLast = -1;
        wTurnLast = 0;
        for (i = 0; i <= 9; i += 2) {
            if (i == 6) {
                i--;
            }
            if (!rglpshdefSB[idPlayer][i].fFree && rglpshdefSB[idPlayer][i].turn >= wTurnLast) {
                wTurnLast = rglpshdefSB[idPlayer][i].turn;
                iSetLast = i;
            }
        }
        if (iSetLast != -1 && (uint16_t)(wTurnLast + 40) <= game.turn) {
            iSetNew = iSetLast <= 4 ? 5 : 0;
            for (i = iSetNew; i < iSetNew + 5; i += 2) {
                if (!rglpshdefSB[idPlayer][i].fFree && rglpshdefSB[idPlayer][i].cExist > 0)
                    goto LOrbital;
            }
            FCreateAiStarbase(iSetNew, 1, aiSbRecipeAuto, isbhullAuto);
            FCreateAiStarbase(iSetNew + 2, 2, aiSbRecipeAuto, isbhullAuto);
            FCreateAiStarbase(iSetNew + 4, 3, aiSbRecipeAuto, isbhullAuto);
        }
    LOrbital:
        iSetLast = -1;
        wTurnLast = 0;
        for (i = 1; i < 9; i += 2) {
            if (i == 5) {
                i++;
            }
            if (!rglpshdefSB[idPlayer][i].fFree && rglpshdefSB[idPlayer][i].turn >= wTurnLast) {
                wTurnLast = rglpshdefSB[idPlayer][i].turn;
                iSetLast = i;
            }
        }
        if (iSetLast != -1 && (uint16_t)(wTurnLast + 40) <= game.turn) {
            iSetNew = iSetLast <= 4 ? 6 : 1;
            if ((rglpshdefSB[idPlayer][iSetNew].fFree || rglpshdefSB[idPlayer][iSetNew].cExist <= 0) &&
                (rglpshdefSB[idPlayer][iSetNew + 2].fFree || rglpshdefSB[idPlayer][iSetNew + 2].cExist <= 0)) {
                FCreateAiStarbase(iSetNew, 1, aiSbRecipeAuto, isbhullAuto);
                FCreateAiStarbase(iSetNew + 2, 2, aiSbRecipeAuto, isbhullAuto);
            }
        }
    }
    return;
}

void EnsureMacintiStarbaseDesigns(uint8_t *rgSB) {
    int16_t k;
    int16_t iOld;
    int16_t cAge;
    int16_t i;
    int16_t j;
    int16_t iNew;

    *rgSB = 0;
    for (i = 1; i <= 3; i++) {
        if (rglpshdefSB[idPlayer][i].fFree || rglpshdefSB[idPlayer][i].cExist == 0) {
            if (FCreateAiStarbase(i, i >= 3 ? 2 : 1, vrgSBMacAisb[i - 1], i)) {
                rgSB[i] = 0;
            } else {
                rgSB[i] = 1;
            }
        } else {
            cAge = game.turn - rglpshdefSB[idPlayer][i].turn;
            if (cAge < 35) {
                if (game.turn > 25 && i == 1 && rglpshdefSB[idPlayer][i].hul.chs != 8) {
                    rgSB[i] = 3;
                } else {
                    rgSB[i] = 0;
                }
            } else if (cAge < 50) {
                rgSB[i] = 2;
            } else {
                rgSB[i] = 3;
            }
        }
    }
    iOld = ishdefNone;
    for (i = 1; i <= 3; i++) {
        if (rgSB[i] >= 2 && (iOld == ishdefNone || (int16_t)rgSB[i] > rgSB[iOld] ||
                             (rgSB[i] == rgSB[iOld] && rglpshdefSB[idPlayer][i].turn < rglpshdefSB[idPlayer][iOld].turn))) {
            iOld = i;
        }
    }
    for (i = 1; i <= 3; i++) {
        if (rgSB[i] >= 2 && i != iOld) {
            rgSB[i] = 0;
        }
    }
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 3 && (rglpshdefSB[idPlayer][3 * i + 4 + j].fFree || rglpshdefSB[idPlayer][3 * i + 4 + j].cExist <= 0); j++) {
        }
        if (j == 3) {
            for (j = 0; j < 3; j++) {
                for (k = 5; k >= 3 && FCreateAiStarbase(3 * i + 4 + j, j + 1, vrgSBMacAisb[k], k - 1) == 0; k--) {
                }
            }
        }
    }
    for (i = 4; i < 10; i++) {
        rgSB[i] = rglpshdefSB[idPlayer][i].fFree != 0;
    }
    if (rglpshdefSB[idPlayer][4].turn >= rglpshdefSB[idPlayer][7].turn) {
        iNew = 4;
        iOld = 7;
    } else {
        iNew = 7;
        iOld = 4;
    }
    if ((uint16_t)(game.turn - rglpshdefSB[idPlayer][iOld].turn) < 30) {
        j = 2;
    } else {
        j = 3;
    }
    for (i = iOld; i < iOld + 3; i++) {
        rgSB[i] = j;
    }
    i = rglpshdefSB[idPlayer][iNew].hul.ihuldef - ihuldefOrbitalFort;
    if (i < 4) {
        rgSB[3] = 2;
        if (i < 3) {
            rgSB[2] = 2;
        }
    }
    return;
}

int16_t FCreateAiStarbase(int16_t ishdef, int16_t iLevel, AiStarbaseRecipe aisb, isbhull isb) {
    int16_t i;
    SHDEF   shdef;
    HS     *lphs;

    if (aisb < aiSbRecipeSpaceStation) {
        switch (ishdef) {
        case 1:
        case 3:
        case 6:
        case 8:
            aisb = aiSbRecipeOrbitalFort;
            isb = isbhullOrbitalFort;
            if (iLevel != 2)
                break;
            iLevel++;
            break;
        default:
            aisb = aiSbRecipeSpaceStation;
            isb = isbhullSpaceStation;
        }
    }
    if (!FCreateAiShdef(ishdefNone, isb, (uint8_t *)&vrgSBAip[aisb])) {
        return FALSE;
    }
    shdef = shdefBuild;
    if (iLevel < 3) {
        for (i = 0; i < shdef.hul.chs; i++) {
            lphs = &shdef.hul.rghs[i];
            if (lphs->cItem >= 4) {
                lphs->cItem >>= 3 - iLevel;
            } else if (iLevel <= 1 && rgplr[idPlayer].idAi != idAiCybertron && (lphs->grhst == hstSpecialSB || lphs->cItem > 1)) {
                lphs->cItem += 0xff;
            }
        }
    }
    shdef.ishdef = ishdef + 16;
    PickANameAndBmp(&shdef, idsGuardianAngel, 13, shdef.hul.ibmp);
    return FChangeAiShdef(&shdef, ishdef + 16);
}

int16_t FAIFling(PLANET *lppl, int32_t *rgResAvail) {
    PLANET *lpplHit;
    POINT16 pt;
    int32_t dy;
    int16_t iT;
    int32_t d2;
    int32_t dBigAssPacket;
    int16_t i;
    int16_t fTwoMAs;
    int16_t iLevelBest;
    PLANET *lpplBest;
    int32_t dx;
    int16_t cFound;
    PLANET *lpplHitMac;
    int32_t l;
    PROD   *lpprod;
    int16_t iKeep;

    cFound = 0;
    iLevelBest = -1;
    if (rgplr[idPlayer].idAi == idAiCybertron) {
        return FALSE;
    }
    i = 0;
    lpprod = lpplProdGlob->rgprod;
    while (i < lpplProdGlob->iprodMac) {
        if (lpprod->grobj == grobjPlanet && lpprod->iItem >= iobjPacketIron && lpprod->iItem <= iobjPacketMixed) {
            return FALSE;
        }
        i++;
        lpprod++;
    }
    if (rgplr[idPlayer].lvlAi > lvlAiStandard && *rgResAvail + rgResAvail[1] + rgResAvail[2] > 3000 && lppl->fStarbase &&
        IWarpMAFromLppl(lppl, &fTwoMAs) >= 10 && Random(4) == 0) {
        pt = rgptPlan[lppl->id];
        for (iT = 0; iT <= 2 && lppl->rgwtMin[iT] <= 12500; iT++) {
        }
        dBigAssPacket = vrgAiPacketDist[fTwoMAs];
        if (iT <= 2) {
            dBigAssPacket = (uint32_t)(dBigAssPacket * 3);
        }
        lpplHit = lpPlanets;
        lpplHitMac = lpPlanets + cPlanet;
        for (; lpplHit < lpplHitMac; lpplHit++) {
            if (lpplHit->iPlayer != iplrNone && lpplHit->iPlayer != idPlayer && (uint16_t)(lpplHit->turn + 2) >= game.turn &&
                ((lpplHit->uDefGuess < 14 || lpplHit->uPopGuess < 750) && GetRaceStat(&rgplr[lpplHit->iPlayer], rsMajorAdv) != raMacintosh) &&
                (!lpplHit->fStarbase || GetRaceStat(&rgplr[lpplHit->iPlayer], rsMajorAdv) != raMassAccel)) {
                dx = (int16_t)(pt.x - rgptPlan[lpplHit->id].x);
                dy = (int16_t)(pt.y - rgptPlan[lpplHit->id].y);
                d2 = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                if (d2 <= dBigAssPacket) {
                    cFound++;
                    if (Random(cFound) == 0) {
                        iLevelBest = 13;
                        lpplBest = lpplHit;
                    }
                }
            }
        }
        if (iLevelBest >= 10) {
            sel.pl.iWarpFling = iLevelBest - 4;
            sel.pl.idFling = lpplBest->id + 1;
            FLookupPlanet(idWriteBack, &sel.pl);
            if (rgResAvail[2] > 20000 && Random(3) != 0) {
                AddItemToQueue(iobjPacketGerm, 80, grobjPlanet, addItemEnd);
            }
            if (*rgResAvail > 3000 && rgResAvail[1] > 4000 && rgResAvail[2] > 3000) {
                AddItemToQueue(iobjPacketMixed, 30, grobjPlanet, addItemEnd);
            } else if (*rgResAvail > 1500 && rgResAvail[1] > 2250 && rgResAvail[2] > 1500) {
                AddItemToQueue(iobjPacketMixed, 15, grobjPlanet, addItemEnd);
            } else {
                for (iT = 0; iT < 3; iT++) {
                    iKeep = iT == 1 ? 2500 : 1250;
                    if (rgResAvail[iT] > iKeep) {
                        l = (int32_t)((rgResAvail[iT] - iKeep) / 200);
                        if (25 < (l <= 1 ? 1 : l)) {
                            l = 25;
                        } else if (l <= 1) {
                            l = 1;
                        }
                        AddItemToQueue(iT + 14, LOWORD(l), grobjPlanet, addItemEnd);
                    }
                }
            }
            return TRUE;
        }
    }
    return FALSE;
}

int16_t IshdefAiSBLatestOF() {
    if (!rglpshdefSB[idPlayer][6].fFree && rglpshdefSB[idPlayer][6].turn > rglpshdefSB[idPlayer][1].turn) {
        return 6;
    }
    return 1;
}

int16_t IshdefAiSBLatest() {
    if (!rglpshdefSB[idPlayer][5].fFree && rglpshdefSB[idPlayer][5].turn > rglpshdefSB[idPlayer]->turn) {
        return 5;
    }
    return 0;
}

void QueueAiStarbases(PROD *rgprod, int16_t ishdefSBLatest) {
    PLANET *lpplMac;
    PLANET *lppl;
    int16_t i;
    PROD   *lpprod;

    if (ishdefSBLatest != ishdefNone && rgplr[idPlayer].idAi != idAiMacinti &&
        ((!gd.fTutorial && (game.lid != 9236297 || rgplr[idPlayer].idAi != idAiTurinDrone)) || game.turn <= 30)) {
        lppl = lpPlanets;
        lpplMac = lpPlanets + cPlanet;
        for (; lppl < lpplMac; lppl++) {
            if (rgplr[idPlayer].idAi == idAiCybertron) {
                if (lppl->iPlayer != idPlayer)
                    continue;
                ishdefSBLatest = iBuildCyberStarbase(lppl);
                if (ishdefSBLatest == ishdefNone)
                    continue;
            } else {
                if (lppl->iPlayer != idPlayer || ((lppl->fStarbase && lppl->isb <= 9) || lppl->rgwtMin[3] < 80 || vlpbAiPlanet[lppl->id * 16 + 2] != 0))
                    continue;
                for (i = 0; i < ((AIHIST *)vlpbAiData)->cStarbase && ((AIHIST *)vlpbAiData)->rgasb[i].idPlanet != lppl->id; i++) {
                }
                if (i == ((AIHIST *)vlpbAiData)->cStarbase)
                    continue;
            }
            ChangeMainObjSel(grobjPlanet, lppl->id);
            InitProduction(rgprod);
            i = 0;
            for (lpprod = lpplProdGlob->rgprod; i < lpplProdGlob->iprodMac && (lpprod->grobj != grobjFleet || lpprod->iItem < iobjPacketGerm); lpprod++) {
                i++;
            }
            if (i < lpplProdGlob->iprodMac) {
                FinishProduction(FALSE);
            } else {
                AddItemToQueue(ishdefSBLatest + 16, 1, grobjFleet, addItemEnd);
                FinishProduction(TRUE);
            }
        }
    }
    return;
}

int16_t FUpgradeAiStarbase(PLANET *lppl, int16_t ishdefSBLatest) {
    int16_t isbCur;
    int16_t iDesigns;
    int16_t i;
    int16_t pctUpg;
    int16_t isbNew;
    PROD   *lpprod;
    int16_t ishdef;

    if (ishdefSBLatest == ishdefNone) {
        return FALSE;
    }
    if (lppl->fStarbase) {
        i = 0;
        lpprod = lpplProdGlob->rgprod;
        while (i < lpplProdGlob->iprodMac) {
            if (lpprod->grobj == grobjFleet && lpprod->iItem >= iobjPacketGerm) {
                return FALSE;
            }
            i++;
            lpprod++;
        }
    }
    if (rgplr[idPlayer].idAi == idAiMacinti) {
        isbCur = lppl->isb;
        if (vAiMacRecycleSB[isbCur] == 3) {
        LDoMacUpgrade:
            if (isbCur < 4) {
                for (isbNew = isbCur + 1; isbNew < 9 && vAiMacRecycleSB[isbNew] != 0; isbNew++) {
                }
            } else {
                isbNew = isbCur + 3;
                if (isbNew >= 10) {
                    isbNew -= 6;
                }
            }
        LDoMacUpgrade2:
            AddItemToQueue(isbNew + 16, 1, grobjFleet, addItemEnd);
            return TRUE;
        }
        if (vAiMacRecycleSB[isbCur] == 2 && Random(100) < 10)
            goto LDoMacUpgrade;
        if (isbCur >= 4 && isbCur != 6 && isbCur != 9 && Random(100) < 8) {
            isbNew = isbCur + 1;
            goto LDoMacUpgrade2;
        }
        if (isbCur < 4) {
            i = PctPlanetCapacity(lppl);
            if (i > 15) {
                i = (i - 15) * 6;
                if (Random(100) < i)
                    goto LDoMacUpgrade;
            }
        }
        return FALSE;
    }
    if (rgplr[idPlayer].idAi == idAiCybertron && game.turn < 40) {
        return FALSE;
    }
    if (lppl->fStarbase) {
        switch (lppl->isb) {
        case 1:
        case 3:
        case 6:
        case 8:
            iDesigns = 2;
            ishdefSBLatest = IshdefAiSBLatestOF();
            break;
        default:
            iDesigns = 3;
        }
        if (lppl->isb < (uint16_t)ishdefSBLatest || lppl->isb > (uint16_t)(ishdefSBLatest + (iDesigns - 1) * 2)) {
            pctUpg = game.turn - rglpshdefSB[idPlayer][ishdefSBLatest].turn - 10;
            if (pctUpg < 0) {
                pctUpg = 0;
            } else if (pctUpg < 50) {
                pctUpg >>= 1;
            }
            pctUpg += 5;
            if (Random(100) < pctUpg) {
                switch (lppl->isb) {
                case 1:
                case 3:
                case 6:
                case 8:
                    ishdef = ishdefSBLatest + lppl->isb % 5 - 1;
                    break;
                default:
                    ishdef = ishdefSBLatest + lppl->isb % 5;
                }
                AddItemToQueue(ishdef + 16, 1, grobjFleet, addItemEnd);
                return TRUE;
            }
        } else if (lppl->isb % 5 < (iDesigns - 1) * 2 && !rglpshdefSB[idPlayer][lppl->isb + 2].fFree && Random(100) < 6) {
            for (i = 0; i < 3; i++) {
                if (lppl->rgwtMin[i] < 200) {
                    return FALSE;
                }
            }
            AddItemToQueue(lppl->isb + 18, 1, grobjFleet, addItemEnd);
            return TRUE;
        }
    }
    return FALSE;
}

int16_t FQueueAiTerraforming(PLANET *lppl, int32_t *rgResAvail, int32_t *rgResCost) {
    int16_t i;
    int16_t j;
    int16_t dEnv;
    int32_t rgItemCost[4];
    PROD   *lpprod;

    if (rgplr[idPlayer].idAi == idAiCybertron) {
        return FALSE;
    }
    if (lppl->rgwtMin[3] < 200) {
        return FALSE;
    }
    i = 0;
    lpprod = lpplProdGlob->rgprod;
    while (i < lpplProdGlob->iprodMac) {
        if (lpprod->grobj == grobjPlanet && lpprod->iItem == mdIdleTerraform) {
            return FALSE;
        }
        i++;
        lpprod++;
    }
    j = -1;
    dEnv = 0;
    for (i = 0; i < 3; i++) {
        if (abs(lppl->rgEnvVar[i] - rgplr[idPlayer].rgEnvVar[i]) > dEnv) {
            j = i;
            dEnv = abs(lppl->rgEnvVar[i] - rgplr[idPlayer].rgEnvVar[i]);
        }
    }
    if (j != -1) {
        for (i = 0; i < cProdGlob; i++) {
            if (pProdGlob[i].grobj == grobjPlanet && pProdGlob[i].iItem == mdIdleTerraform && pProdGlob[i].cItem >= 1) {
                AddItemToQueue(pProdGlob[i].iItem, 4 >= (int32_t)pProdGlob[i].cItem ? pProdGlob[i].cItem : 4, grobjPlanet, addItemEnd);
                pProdGlob[i].cItem = 0;
                GetProductionCosts(lppl, pProdGlob + i, rgItemCost, idPlayer, TRUE);
                for (j = 0; j < 4; j++) {
                    rgResCost[j] += (uint32_t)(rgItemCost[j] * (4 >= (int32_t)pProdGlob[i].cItem ? (int16_t)pProdGlob[i].cItem : 4));
                }
                return TRUE;
            }
        }
    }
    return FALSE;
}

int16_t FQueueAiScanner(PLANET *lppl, int32_t *rgResAvail, int32_t *rgResCost) {
    int16_t i;
    int16_t j;
    int32_t rgItemCost[4];
    PROD   *lpprod;

    i = 0;
    lpprod = lpplProdGlob->rgprod;
    while (i < lpplProdGlob->iprodMac) {
        if (lpprod->grobj == grobjPlanet && lpprod->iItem >= iobjPlanetaryScannerFirst && lpprod->iItem <= iobjPlanetaryScannerSnooper620X) {
            return FALSE;
        }
        i++;
        lpprod++;
    }
    for (i = cProdGlob - 1; i >= 0 && (pProdGlob[i].grobj != grobjPlanet || pProdGlob[i].iItem < iobjPlanetaryScannerFirst ||
                                       pProdGlob[i].iItem > iobjPlanetaryScannerSnooper620X);
         i--) {
    }
    if (i >= 0) {
        GetProductionCosts(lppl, pProdGlob + i, rgItemCost, idPlayer, TRUE);
        for (j = 0; j < 4 && (rgItemCost[j] <= 0 || rgItemCost[j] + rgResCost[j] <= rgResAvail[j]); j++) {
        }
        if (j == 4) {
            AddItemToQueue(pProdGlob[i].iItem, 1, grobjPlanet, addItemEnd);
            for (j = 0; j < 4; j++) {
                rgResCost[j] += rgItemCost[j];
            }
            return TRUE;
        }
    }
    return FALSE;
}

int16_t FQueueAiDefenses(PLANET *lppl, int32_t *rgResAvail, int32_t *rgResCost) {
    int16_t i;
    int16_t j;
    PROD   *lpprod;

    if (lppl->rgwtMin[3] < 1600 || (int32_t)(lppl->rgwtMin[3] / 80) <= (int32_t)lppl->cDefenses) {
        return FALSE;
    }
    i = 0;
    lpprod = lpplProdGlob->rgprod;
    while (i < lpplProdGlob->iprodMac) {
        if (lpprod->grobj == grobjPlanet && lpprod->iItem == mdIdleDefense) {
            return FALSE;
        }
        i++;
        lpprod++;
    }
    for (i = 0; i < cProdGlob && (pProdGlob[i].grobj != grobjPlanet || pProdGlob[i].iItem != mdIdleDefense || pProdGlob[i].cItem < 1); i++) {
    }
    if (i < cProdGlob) {
        j = pProdGlob[i].cItem;
        if (j > 4) {
            j = 4;
        }
        AddItemToQueue(mdIdleDefense, j, grobjPlanet, addItemEnd);
        return TRUE;
    }
    return FALSE;
}

void HandleBasicAiTasks(int16_t iroCur, PROD *rgprod, int16_t ishdefSBLatest, int32_t *rgResAvail, int32_t *rgResCost) {
    PLANET *lppl;
    int16_t i;
    int16_t ipl;
    int16_t fWrite;

    KeepFleetsMoving();
    QueueAiStarbases(rgprod, ishdefSBLatest);
    for (ipl = 0; ipl < vclpplAi; ipl++) {
        lppl = vrglpplAi[ipl];
        if (!vrglpplAi[ipl])
            break;
        if (rgplr[idPlayer].idAi == idAiRobotoid) {
            if (lppl->rgwtMin[3] < 40)
                continue;
        } else if (lppl->rgwtMin[3] < 60 && vlpbAiPlanet[lppl->id * 16 + 2] == 0) {
            continue;
        }
        GetResourcesAvailable(lppl, rgResAvail);
        GetProdQCost(lppl, rgResCost);
        if (rgplr[idPlayer].idAi != idAiMacinti) {
            for (i = 0; i < 4 && rgResAvail[i] >= rgResCost[i]; i++) {
            }
            if (i < 3)
                continue;
        }
        ChangeMainObjSel(grobjPlanet, lppl->id);
        InitProduction(rgprod);
        fWrite = FALSE;
        if (rgplr[idPlayer].idAi == idAiRobotoid || vlpbAiPlanet[lppl->id * 16 + 2] == 0) {
            if (FUpgradeAiStarbase(lppl, ishdefSBLatest) || FAIFling(lppl, rgResAvail) || FQueueAiScanner(lppl, rgResAvail, rgResCost) ||
                FQueueAiDefenses(lppl, rgResAvail, rgResCost)) {
                fWrite = TRUE;
            }
        } else if (rgplr[idPlayer].idAi == idAiCybertron && FUpgradeAiStarbase(lppl, ishdefSBLatest)) {
            fWrite = TRUE;
        }
        if (rgplr[idPlayer].idAi != idAiRobotoid && !fWrite && FQueueAiTerraforming(lppl, rgResAvail, rgResCost)) {
            fWrite = TRUE;
        }
        FinishProduction(fWrite);
    }
    if (!game.fTutorial && game.turn >= (uint16_t)(10 * game.mdSize + 20)) {
        FixPlanetsUnderAttack(rgprod);
    }
    AddMinesToBlockedQueues();
    return;
}

void SplitOutShdefs(uint8_t *rgbIsh) {
    int16_t iLast;
    int16_t iFirst;
    int16_t ifl;
    int16_t i;
    FLEET  *lpfl;
    int16_t fUnmarked;
    FLEET   flNew;
    int16_t fMarked;
    FLEET  *lpflNew;

    iLast = -1;
    iFirst = -1;
    for (i = 0; i < 16; i++) {
        if (rgbIsh[i] > 0) {
            iLast = i;
            if (iFirst == -1) {
                iFirst = i;
            }
        }
    }
    if (iFirst == -1)
        return;
LTopOfLoop:
    if (rgplr[idPlayer].cFleet > 500)
        return;
    for (ifl = 0; ifl < cFleet; ifl++) {
        lpfl = rglpfl[ifl];
        if (!rglpfl[ifl])
            break;
        if (lpfl->iPlayer == idPlayer && !lpfl->fDead) {
            fUnmarked = FALSE;
            fMarked = FALSE;
            for (i = 0; i < 16; i++) {
                if (lpfl->rgcsh[i] > 0) {
                    if (rgbIsh[i] != 0) {
                        if (fUnmarked) {
                        LDoTheSplit:
                            ChangeMainObjSel(grobjFleet, lpfl->id);
                            lpflNew = LpflNewSplit(&sel.fl);
                            flNew = *lpflNew;
                            for (i = 0; i < 16; i++) {
                                if (rgbIsh[i] > 0) {
                                    flNew.rgcsh[i] = sel.fl.rgcsh[i];
                                    sel.fl.rgcsh[i] = 0;
                                }
                            }
                            FleetTransferCargoBalance(&sel.fl, &flNew);
                            FLookupFleet(idWriteBack, &sel.fl);
                            FLookupFleet(idWriteBack, &flNew);
                            goto LTopOfLoop;
                        }
                        fMarked = TRUE;
                    } else {
                        if (fMarked)
                            goto LDoTheSplit;
                        fUnmarked = TRUE;
                    }
                }
            }
        }
    }
}

int16_t CheckAiShdefStatus(int16_t ishBeg, int16_t ishEnd, uint16_t cRecyclePeriod, int16_t *piLatest, uint8_t *rgbOld) {
    uint32_t cExist;
    int16_t  i;
    SHDEF    shdef;

    *piLatest = ishdefNone;
    cExist = 0;
    for (i = ishBeg; i <= ishEnd; i++) {
        if (!rgshdef[i].fFree) {
            cExist += rgshdef[i].cExist;
            if (*piLatest == ishdefNone || rgshdef[*piLatest].turn < rgshdef[i].turn) {
                *piLatest = i;
            }
            if ((uint16_t)(game.turn - rgshdef[i].turn) > cRecyclePeriod) {
                if (rgshdef[i].cExist == 0) {
                    shdef = rgshdef[i];
                    shdef.fFree = TRUE;
                    FChangeAiShdef(&shdef, i);
                } else {
                    rgbOld[i] = 1;
                }
            }
        }
    }
    if (cExist > 32000) {
        cExist = 32000;
    }
    return LOWORD(cExist);
}

void IncreaseAIMinefieldSizes() {
    THING  *lpth;
    int32_t cMines;
    THING  *lpthMac;

    lpth = lpThings;
    lpthMac = lpThings + cThing;
    for (; lpth < lpthMac; lpth++) {
        if (lpth->ith == ithMinefield) {
            cMines = (int32_t)((long double)sqrt((double)lpth->thm.cMines) + 10.5);
            lpth->thm.cMines = (uint32_t)(cMines * cMines);
        }
    }
    return;
}

int16_t FFindBuddyAndJoinUp(FLEET *lpfl, int16_t ishLo, int16_t ishHi, int32_t lMaxDist1, int32_t lMaxDist2) {
    int32_t lDistBest;
    int32_t lDist;
    int16_t i;
    int16_t ifl;
    FLEET  *lpflOther;
    FLEET  *lpflBest;
    ORDER   ord;

    lpflBest = NULL;
    lDistBest = 9999999;
    for (ifl = 0; ifl < cFleet; ifl++) {
        lpflOther = rglpfl[ifl];
        if (!rglpfl[ifl])
            break;
        if (lpflOther->iPlayer == idPlayer) {
            if (lpflOther == lpfl)
                break;
            for (i = ishLo; i <= ishHi; i++) {
                if (lpflOther->rgcsh[i] != 0) {
                    lDist = LDistance2(lpfl->pt, lpflOther->pt);
                    if (lDist < lDistBest) {
                        lpflBest = lpflOther;
                        lDistBest = lDist;
                    }
                    break;
                }
            }
        }
    }
    if (lpflBest && (lDistBest <= (int32_t)(uint32_t)(lMaxDist1 * lMaxDist1) || (lDistBest <= (int32_t)(uint32_t)(lMaxDist2 * lMaxDist2) && Random(2) != 0))) {
        ClearAiCurrentTask(lpfl, TRUE);
        ord.id = lpflBest->id;
        ord.grobj = grobjFleet;
        ord.pt = lpflBest->pt;
        ord.grTask = grTaskNone;
        ord.fValidTask = TRUE;
        ord.iWarp = 6;
        FMoveAiFleet(lpfl, &ord, FALSE);
        return TRUE;
    }
    return FALSE;
}

int16_t FShouldPlanetBuildColonizer(PLANET *lpplSrc) {
    POINT16  pt;
    int16_t  i;
    int32_t  lCur;
    uint8_t *lpb;
    int32_t  lBest;

    lBest = 10000000;
    pt = rgptPlan[lpplSrc->id];
    if (game.turn < 60) {
        return TRUE;
    }
    lpb = vlpbAiPlanet + 13;
    i = 0;
    while (i < game.cPlanMax) {
        if (*lpb == 0) {
            lCur = LDistance2(pt, rgptPlan[i]);
            if (lCur < lBest) {
                lBest = lCur;
            }
        }
        i++;
        lpb += 16;
    }
    if (lBest > 122500 || (lBest > 90000 && Random(2) != 0) || (lBest > 62500 && Random(2) != 0)) {
        return FALSE;
    }
    return TRUE;
}

void InitRandomPlanetList() {
    PLANET *lpplMac;
    int16_t iT;
    PLANET *lppl;
    int16_t i;

    vclpplAi = 0;
    lppl = lpPlanets;
    lpplMac = lpPlanets + cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->iPlayer == idPlayer) {
            vrglpplAi[vclpplAi++] = lppl;
        }
    }
    if (!gd.fTutorial) {
        for (i = 0; i < vclpplAi - 1; i++) {
            iT = Random(vclpplAi - i) + i;
            lppl = vrglpplAi[i];
            vrglpplAi[i] = vrglpplAi[iT];
            vrglpplAi[iT] = lppl;
        }
    }
    return;
}
