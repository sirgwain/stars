#include "common.h"

int16_t FStargateJump(FLEET *lpfl, int16_t isbsSrc, int16_t isbsDst, int16_t dDist) {
    int16_t   dpPerShdefNew;
    int16_t   dpShdef;
    POINT16   pt;
    int16_t   id;
    FLEET     flSrc;
    int16_t   cshT;
    uint8_t   pctKill;
    int16_t   i;
    int32_t   cshOrig;
    MessageId idm;
    int32_t   cshKill;
    int16_t   pct;
    int16_t   rgpct[16];
    int16_t   cshdef;
    int16_t   ishdef;
    int32_t   dp;
    int16_t   dpPerShdefOld;
    int16_t   cshDamagedOld;
    FLEET     flDead;

    cshdef = 0;
    cshKill = 0;
    cshOrig = 0;
    memset(rgpct, 0, 32);
    flSrc = *lpfl;
    if (flSrc.lpplord->rgord[1].grobj == grobjPlanet) {
        id = flSrc.lpplord->rgord[1].id;
    } else {
        pt = flSrc.lpplord->rgord[1].pt;
        for (id = 0; id < game.cPlanMax && (pt.x != rgptPlan[id].x || pt.y != rgptPlan[id].y); id++) {
        }
    }
    for (ishdef = 0; ishdef < 16; ishdef++) {
        if (flSrc.rgcsh[ishdef] != 0) {
            cshdef++;
            cshOrig += flSrc.rgcsh[ishdef];
            switch (MdCalcStargateDamage(isbsSrc, isbsDst, dDist, rglpshdef[flSrc.iPlayer][ishdef].hul.wtEmpty, &rgpct[ishdef])) {
            case -1:
                FSendPlrMsg(flSrc.iPlayer, idmAttemptedUseStargateReachCouldBecauseDestination, flSrc.id | 0x8000, flSrc.id, flSrc.idPlanet, id, 0, 0, 0, 0);
                return FALSE;
            case -2:
                FSendPlrMsg(flSrc.iPlayer, idmAttemptedUseStargateReachCouldBecauseShips, flSrc.id | 0x8000, flSrc.id, flSrc.idPlanet, id, ishdef, 0, 0, 0);
                return FALSE;
            case 0:
                cshdef--;
                break;
            case 1:
                flSrc.fNoHeal = TRUE;
            }
        }
    }
    if (cshdef == 0) {
    LKilledEmAll:
        lpfl->fDead = TRUE;
        FSendPlrMsg(flSrc.iPlayer, idmHeedlessDangerAttemptedUseStargateReachFleet, flSrc.id | 0x8000, flSrc.id, flSrc.idPlanet, id, 0, 0, 0, 0);
        return FALSE;
    }
    memset(&flDead, 0, sizeof(FLEET));
    for (ishdef = 0; ishdef < 16; ishdef++) {
        if (flSrc.rgcsh[ishdef] != 0 && rgpct[ishdef] != 0) {
            if (rgpct[ishdef] == 100) {
                cshKill += flSrc.rgcsh[ishdef];
                flSrc.rgcsh[ishdef] = 0;
                flSrc.rgdv[ishdef].dp = 0;
                cshdef--;
            } else {
                cshT = flSrc.rgcsh[ishdef];
                if (GetRaceStat(&rgplr[lpfl->iPlayer], rsMajorAdv) == raStargate) {
                    pctKill = 0;
                } else {
                    pctKill = rgpct[ishdef] / 3;
                }
                dpShdef = rglpshdef[lpfl->iPlayer][ishdef].hul.dp;
                if (flSrc.rgdv[ishdef].dp != 0) {
                    cshDamagedOld = LOWORD((int32_t)(cshT * (int16_t)flSrc.rgdv[ishdef].pctSh) / 100);
                    if (cshDamagedOld == 0) {
                        cshDamagedOld = 1;
                    }
                } else {
                    cshDamagedOld = 0;
                }
                if (pctKill > 0) {
                    for (i = 0; i < flSrc.rgcsh[ishdef]; i++) {
                        if (Random(100) < pctKill) {
                            cshT--;
                            if (cshDamagedOld != 0 && (uint16_t)Random(500) < flSrc.rgdv[ishdef].pctDp) {
                                cshDamagedOld--;
                            }
                        }
                    }
                    cshKill += (int16_t)(flSrc.rgcsh[ishdef] - cshT);
                }
                if (cshT != 0) {
                    if (flSrc.rgdv[ishdef].dp != 0) {
                        dpPerShdefOld = LOWORD((int32_t)(dpShdef * (int16_t)flSrc.rgdv[ishdef].pctDp) / 500);
                        if (dpPerShdefOld == 0) {
                            dpPerShdefOld = 1;
                        }
                    } else {
                        dpPerShdefOld = 0;
                    }
                    dpPerShdefNew = LOWORD((int32_t)(dpShdef * rgpct[ishdef]) / 100);
                    if (dpPerShdefNew == 0) {
                        dpPerShdefNew = 1;
                    }
                    if (cshDamagedOld != 0 && dpPerShdefNew + dpPerShdefOld >= dpShdef) {
                        cshKill += cshDamagedOld;
                        cshT -= cshDamagedOld;
                    }
                    if (cshT != 0) {
                        dp = (uint32_t)(dpPerShdefNew * cshT) + (uint32_t)(dpPerShdefOld * cshDamagedOld);
                        pct = LOWORD((int32_t)((int32_t)(dp / cshT) * 500 / dpShdef));
                        if (pct == 0) {
                            pct = 1;
                        }
                        flSrc.rgdv[ishdef].pctDp = pct;
                        flSrc.rgdv[ishdef].pctSh = 100;
                    }
                }
                flSrc.rgcsh[ishdef] = cshT;
                if (cshT == 0) {
                    flSrc.rgdv[ishdef].dp = 0;
                    cshdef--;
                }
            }
            flDead.rgcsh[ishdef] = lpfl->rgcsh[ishdef] - flSrc.rgcsh[ishdef];
        }
    }
    if (cshdef == 0)
        goto LKilledEmAll;
    if (cshKill != 0) {
        if (cshKill & 0xffff0000) {
            FSendPlrMsg(lpfl->iPlayer, idmUsedStargateReachLosingUnbelievableShipsJump, lpfl->id | 0x8000, lpfl->id, lpfl->idPlanet, id, LOWORD(cshKill),
                        HIWORD(cshKill), 0, 0);
        } else {
            if (cshKill < (int32_t)(cshOrig >> 2)) {
                idm = idmUsedStargateReachLosingShipsTreacherousVoid;
            } else if (cshKill > (int32_t)(cshOrig >> 1)) {
                idm = idmUsedStargateReachUnfortunatelyLosingShipsGreat;
            } else {
                idm = idmUsedStargateReachLosingShipsUnforgivingVoid;
            }
            FSendPlrMsg(lpfl->iPlayer, idm, lpfl->id | 0x8000, lpfl->id, lpfl->idPlanet, id, LOWORD(cshKill), 0, 0, 0);
        }
        flDead.iPlayer = flSrc.iPlayer;
        flDead.fDead = TRUE;
        flDead.det = detAll;
        FleetTransferCargoBalance(&flSrc, &flDead);
    }
    *lpfl = flSrc;
    return TRUE;
}

int16_t MdCalcStargateDamage(int16_t isbsSrc, int16_t isbsDst, int16_t dDist, int16_t wt, int16_t *ppctDmg) {
    int32_t dBaseDistance;
    PART    partDst;
    PART    partSrc;
    int32_t pctSurviveT;
    int32_t pctSurvive;

    pctSurvive = 10000;
    partDst.hs.grhst = hstSpecialSB;
    partSrc.hs.grhst = hstSpecialSB;
    partSrc.hs.iItem = isbsSrc;
    partDst.hs.iItem = isbsDst;
    FLookupPart(&partSrc);
    FLookupPart(&partDst);
    dBaseDistance = partSrc.pspecialsb->grAbility2;
    if (dBaseDistance == -1) {
        dBaseDistance = 8000;
    }
    if ((int32_t)dDist > (int32_t)(uint32_t)(dBaseDistance * 5)) {
        return -1;
    }
    if ((partSrc.pspecialsb->grAbility > 0 && (int32_t)wt > (int32_t)(uint32_t)(partSrc.pspecialsb->grAbility * 5)) ||
        (partDst.pspecialsb->grAbility > 0 && (int32_t)wt > (int32_t)(uint32_t)(partDst.pspecialsb->grAbility * 5))) {
        return -2;
    }
    if (dDist > dBaseDistance) {
        pctSurvive = (int32_t)((int32_t)(((uint32_t)(dBaseDistance * 5) - dDist) * 2500) / dBaseDistance);
        if (pctSurvive <= 0)
            goto TotalDeath;
    }
    if (wt > partSrc.pspecialsb->grAbility && partSrc.pspecialsb->grAbility > 0) {
        pctSurviveT = (int32_t)((int32_t)(((uint32_t)(partSrc.pspecialsb->grAbility * 5) - wt) * 2500) / partSrc.pspecialsb->grAbility);
        if (pctSurviveT <= 0)
            goto TotalDeath;
        pctSurvive = (int32_t)(pctSurvive * pctSurviveT) / 10000;
    }
    if (wt > partDst.pspecialsb->grAbility && partDst.pspecialsb->grAbility > 0) {
        pctSurviveT = (int32_t)((int32_t)(((uint32_t)(partDst.pspecialsb->grAbility * 5) - wt) * 2500) / partDst.pspecialsb->grAbility);
        if (pctSurviveT <= 0)
            goto TotalDeath;
        pctSurvive = (int32_t)(pctSurvive * pctSurviveT) / 10000;
    }
    *ppctDmg = LOWORD((int32_t)((10000 - pctSurvive) / 100));
    return 1;
TotalDeath:
    *ppctDmg = 100;
    return 0;
}

void KillUsedWaypoints() {
    int16_t j;
    int16_t i;
    FLEET  *lpfl;
    int16_t fRep;
    PLANET *lppl;

    if (cFleet > 0) {
        for (i = 0; i < cFleet; i++) {
            lpfl = rglpfl[i];
            if (!rglpfl[i])
                break;
            if (lpfl->cord > 1) {
                if (lpfl->fMark) {
                    DeleteWpFar(lpfl, 1, FALSE);
                    FSendPlrMsg(lpfl->iPlayer, idmHasExecutedOrdersFollowFleetAwaitsFurther, lpfl->id | 0x8000, lpfl->id, 0, 0, 0, 0, 0, 0);
                } else {
                    for (j = 1; j < lpfl->cord; j++) {
                        if (lpfl->lpplord->rgord[j].grobj == grobjFleet) {
                            lpfl->lpflNext = LpflFromId(lpfl->lpplord->rgord[j].id);
                            if (lpfl->lpflNext) {
                                if (!lpfl->lpplord->rgord[j].fNoAutoTrack) {
                                    lpfl->lpplord->rgord[j].pt = lpfl->lpflNext->pt;
                                }
                                lpfl->lpflNext = NULL;
                            } else {
                                lpfl->lpplord->rgord[j].grobj = grobjOther;
                                lpfl->lpplord->rgord[j].id = 0;
                            }
                        }
                    }
                    if (lpfl->pt.x == lpfl->lpplord->rgord[1].pt.x && lpfl->pt.y == lpfl->lpplord->rgord[1].pt.y) {
                        lpfl->lpplord->rgord[0] = lpfl->lpplord->rgord[1];
                        if (lpfl->lpplord->rgord[0].grobj != grobjFleet)
                            goto NoOrdFixupYet;
                        if (lpfl->lpplord->rgord[0].grTask == grTaskXfer)
                            goto NoOrdFixupYet;
                        if (lpfl->lpplord->rgord[0].grTask == grTaskMerge)
                            goto NoOrdFixupYet;
                        if (lpfl->idPlanet == idPlanetDeepSpace) {
                            lpfl->lpplord->rgord[0].grobj = grobjOther;
                            lpfl->lpplord->rgord[0].id = 0;
                            goto NoOrdFixupYet;
                        }
                        lpfl->lpplord->rgord[0].grobj = grobjPlanet;
                        lpfl->lpplord->rgord[0].id = lpfl->idPlanet;
                    NoOrdFixupYet:
                        if (lpfl->lpplord->rgord[1].grTask != grTaskPatrol || lpfl->lpplord->rgord[1].grobj != grobjFleet) {
                            fRep = lpfl->fRepOrders;
                        } else {
                            fRep = FALSE;
                        }
                        DeleteWpFar(lpfl, 1, fRep);
                        if (lpfl->cord == 1) {
                            switch (lpfl->lpplord->rgord[0].grTask) {
                            case grTaskXfer:
                            case grTaskColonize:
                            case grTaskMine:
                            case grTaskScrap:
                            case grTaskLayMines:
                            case grTaskPatrol:
                                break;
                            case grTaskAutoRoute:
                                if (lpfl->idPlanet != idPlanetDeepSpace) {
                                    lppl = LpplFromId(lpfl->idPlanet);
                                    if (lppl->iPlayer == lpfl->iPlayer && lppl->idRoute != 0)
                                        break;
                                }
                                /* fallthrough */
                            default:
                                FSendPlrMsg(lpfl->iPlayer, idmHasCompletedAssignedOrders, lpfl->id | 0x8000, lpfl->id, 0, 0, 0, 0, 0, 0);
                            }
                        }
                    }
                }
            }
        }
    }
    return;
}

void NoAutoTrackFleet(FLEET *lpflTarget) {
    int16_t iplr;
    int16_t idTarget;
    int16_t i;
    ORDER  *lpord;
    int16_t ifl;
    FLEET  *lpfl;

    iplr = lpflTarget->iPlayer;
    idTarget = lpflTarget->id;
    for (ifl = 0; ifl < cFleet; ifl++) {
        lpfl = rglpfl[ifl];
        if (!rglpfl[ifl])
            break;
        if (lpfl->iPlayer != iplr && lpfl->cord > 1) {
            lpord = &lpfl->lpplord->rgord[1];
            i = 1;
            while (i < lpfl->cord) {
                if (lpord->grobj == grobjFleet && lpord->id == idTarget) {
                    lpord->fNoAutoTrack = TRUE;
                    lpord->pt = lpflTarget->pt;
                }
                i++;
                lpord++;
            }
        }
    }
    return;
}

void AutoRouteFleet(FLEET *lpfl, PLANET *lppl) {
    int32_t dTravel;
    int16_t iWarp;
    int16_t pctDmg;
    int16_t wt;
    int32_t cTurns;
    int16_t i;
    ORDER  *lpord;
    PLANET *lpplRoute;
    int16_t isbsDst;
    int16_t wtBig;
    int16_t ishdef;
    int16_t ishdefBig;
    int16_t isbsSrc;

    lpfl->cord = 2;
    lpfl->lpplord->iordMac = 2;
    lpord = &lpfl->lpplord->rgord[1];
    lpord->grTask = grTaskAutoRoute;
    lpord->grobj = grobjPlanet;
    lpord->id = lppl->idRoute - 1;
    lpplRoute = LpplFromId(lppl->idRoute - 1);
    lpord->pt = rgptPlan[lppl->idRoute - 1];
    lpord->fValidTask = TRUE;
    iWarp = IFindIdealWarp(lpfl, FALSE);
    dTravel = Sf80ToI32((Sf80Add(Sf80From64(DGetDistance(lpfl->pt.x, lpfl->pt.y, lpord->pt.x, lpord->pt.y)), Sf80From64(0.999))));
    if (lppl->iPlayer == lpplRoute->iPlayer && lppl->fStarbase && lpplRoute->fStarbase) {
        isbsDst = IStargateFromLppl(lpplRoute);
        isbsSrc = IStargateFromLppl(lppl);
        for (i = 0; i < 4 && lpfl->rgwtMin[i] == 0; i++) {
        }
        if (i == 4 && isbsDst != -1 && isbsSrc != -1) {
            wt = 0;
            wtBig = 0;
            ishdefBig = ishdefNone;
            for (ishdef = 0; ishdef < 16; ishdef++) {
                if (lpfl->rgcsh[ishdef] != 0) {
                    wt = rglpshdef[lpfl->iPlayer][ishdef].hul.wtEmpty;
                    if (wt > wtBig) {
                        wtBig = wt;
                        ishdefBig = ishdef;
                    }
                }
            }
            if (MdCalcStargateDamage(isbsSrc, isbsDst, LOWORD(dTravel), wtBig, &pctDmg) == 1 && pctDmg == 0) {
                iWarp = 11;
            }
        }
        if (iWarp < 9 && LphuldefFromId(rglpshdefSB[lpfl->iPlayer][lpplRoute->isb].hul.ihuldef)->hul.wtCargoMax != 0) {
            for (iWarp = 9; iWarp > 0 && EstFuelUse(lpfl, 0, iWarp, -1, TRUE) < dTravel; iWarp--) {
            }
            lpord->iWarp = iWarp;
        }
    }
    if (iWarp < 11 && iWarp != 0) {
        cTurns = (int32_t)((int32_t)(dTravel / iWarp) / iWarp);
        while (iWarp > 2) {
            iWarp--;
            if ((int32_t)((int32_t)(dTravel / iWarp) / iWarp) > cTurns) {
                iWarp++;
                break;
            }
        }
        for (; iWarp != 0 && EstFuelUse(lpfl, 0, iWarp, dTravel, FALSE) > lpfl->rgwtMin[4]; iWarp--) {
        }
    }
    lpord->iWarp = iWarp;
    return;
}

int16_t FColonizer(FLEET *lpfl) {
    int16_t i;
    int32_t l;

    for (i = 0; i < 16; i++) {
        if (lpfl->rgcsh[i] != 0) {
            l = 1 << rglpshdef[lpfl->iPlayer][i].hul.ihuldef;
            if (l & 0xc000) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

int16_t FScout(FLEET *lpfl) {
    int16_t i;
    int32_t l;

    for (i = 0; i < 16; i++) {
        if (lpfl->rgcsh[i] != 0) {
            l = 1 << rglpshdef[lpfl->iPlayer][i].hul.ihuldef;
            if (l & 0x70) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

void AutoFleetOrder(FLEET *lpfl, PLANET *lppl) {
    int32_t cMine;
    int16_t ifl;
    ORDER  *lpord;
    FLEET  *lpflT;
    int16_t fFoundFleet;

    fFoundFleet = FALSE;
    lpord = lpfl->lpplord->rgord;
    if ((lppl->iPlayer == iplrNone || (GetRaceStat(&rgplr[lpfl->iPlayer], rsMajorAdv) == raMacintosh && lppl->iPlayer == lpfl->iPlayer)) &&
        CMineFromLpfl(lpfl) != 0) {
        if (lppl->iPlayer == iplrNone) {
            for (ifl = 0; ifl < cFleet; ifl++) {
                lpflT = rglpfl[ifl];
                if (!rglpfl[ifl])
                    break;
                if (lpflT->iPlayer >= lpfl->iPlayer && !lpflT->fDead && lpfl != lpflT) {
                    if (lpflT->iPlayer > lpfl->iPlayer)
                        break;
                    if (lpflT->pt.x == lpfl->pt.x && lpflT->pt.y == lpfl->pt.y) {
                        cMine = CMineFromLpfl(lpflT);
                        if (cMine > 0 && cMine < 4000) {
                            fFoundFleet = TRUE;
                            break;
                        }
                    }
                }
            }
        }
        if (fFoundFleet) {
            lpord->grTask = grTaskMerge;
            lpord->grobj = grobjFleet;
            lpord->id = lpflT->id;
        } else {
            lpord->grTask = grTaskMine;
        }
    }
    lpfl->iplan = 0;
    return;
}

int32_t CMineFromLpfl(FLEET *lpfl) {
    int32_t cMine;
    int16_t j;
    int16_t i;
    HUL    *lphuldef;
    PART    part;
    int32_t cMineTot;
    int16_t chs;
    HS     *lphs;

    cMineTot = 0;
    for (i = 0; i < 16; i++) {
        if (lpfl->rgcsh[i] > 0) {
            lphuldef = &rglpshdef[lpfl->iPlayer][i].hul;
            chs = lphuldef->chs;
            cMine = 0;
            j = 0;
            lphs = lphuldef->rghs;
            while (j < chs) {
                if (lphs->grhst == hstMining && lphs->iItem >= iminingRoboMidgetMiner && lphs->iItem <= iminingAlienMiner) {
                    part.hs = *lphs;
                    FLookupPart(&part);
                    cMine += (uint32_t)(lphs->cItem * part.pmining->grAbility);
                }
                j++;
                lphs++;
            }
            cMineTot += (uint32_t)(cMine * lpfl->rgcsh[i]);
        }
    }
    if (cMineTot < 4000) {
        return cMineTot;
    }
    return 4000;
}

int32_t PctTerraFromLpfl(FLEET *lpfl) {
    int16_t j;
    int32_t pctTot;
    int16_t i;
    int32_t pct;
    HUL    *lphuldef;
    int16_t chs;
    HS     *lphs;

    pctTot = 0;
    for (i = 0; i < 16; i++) {
        if (lpfl->rgcsh[i] > 0) {
            lphuldef = &rglpshdef[lpfl->iPlayer][i].hul;
            chs = lphuldef->chs;
            pct = 0;
            j = 0;
            lphs = lphuldef->rghs;
            while (j < chs) {
                if (lphs->grhst == hstMining && lphs->iItem == iminingOrbitalAdjuster) {
                    pct += lphs->cItem;
                }
                j++;
                lphs++;
            }
            pctTot += (uint32_t)(pct * lpfl->rgcsh[i]);
        }
    }
    return pctTot;
}

int32_t CLayMinesFromLpfl(FLEET *lpfl, MineFieldType iType, int16_t ishdef) {
    uint16_t iMin;
    uint16_t iMax;
    int32_t  cMine;
    int16_t  j;
    int16_t  i;
    HUL     *lphul;
    PART     part;
    int32_t  cMineTot;
    int16_t  chs;
    HS      *lphs;

    cMineTot = 0;
    switch (iType) {
    case mineAll:
    default:
        iMin = 0;
        iMax = 9;
        break;
    case mineStandard:
        iMin = 0;
        iMax = 3;
        break;
    case mineHeavy:
        iMin = 4;
        iMax = 6;
        break;
    case mineSpeedBump:
        iMin = 7;
        iMax = 9;
    }
    for (i = 0; i < 16; i++) {
        if (lpfl->rgcsh[i] > 0 && (ishdef == ishdefAll || i == ishdef)) {
            lphul = &rglpshdef[lpfl->iPlayer][i].hul;
            chs = lphul->chs;
            cMine = 0;
            j = 0;
            lphs = lphul->rghs;
            while (j < chs) {
                if (lphs->grhst == hstMines && lphs->iItem >= iMin && lphs->iItem <= iMax) {
                    part.hs = *lphs;
                    FLookupPart(&part);
                    cMine += (uint32_t)(lphs->cItem * part.pmines->grAbility);
                } else if ((int16_t)iType <= mineStandard && lphs->grhst == hstBeam && lphs->iItem == ibeamMultiContainedMunition) {
                    cMine += (int32_t)(lphs->cItem * 4);
                }
                j++;
                lphs++;
            }
            if (lphul->ihuldef == ihuldefMiniMineLayer || lphul->ihuldef == ihuldefSuperMineLayer) {
                cMine = (int32_t)(cMine * 2);
            }
            cMineTot += (uint32_t)(cMine * lpfl->rgcsh[i]);
        }
    }
    if (cMineTot > 10000000 || cMineTot < 0) {
        return 100000000;
    }
    return (uint32_t)(cMineTot * 10);
}

int32_t CMineSweepFromLpfl(FLEET *lpfl) {
    int32_t lPowTot;
    int16_t i;
    HUL    *lphul;
    int32_t lPow;

    lPowTot = 0;
    for (i = 0; i < 16; i++) {
        if (lpfl->rgcsh[i] > 0) {
            lphul = &rglpshdef[lpfl->iPlayer][i].hul;
            lPow = CMineSweepFromLphul(lphul);
            lPowTot += (uint32_t)(lPow * lpfl->rgcsh[i]);
        }
    }
    if (lPowTot <= 0) {
        return 0;
    }
    return lPowTot;
}

int32_t CMineSweepFromLphul(HUL *lphul) {
    int16_t chs;
    HS     *lphs;
    int32_t lRange;
    int16_t j;
    int16_t fStarbase;
    int32_t lPow;
    PART    part;

    fStarbase = (int16_t)lphul->ihuldef >= ihuldefOrbitalFort;
    chs = lphul->chs;
    lPow = 0;
    for (j = 0, lphs = lphul->rghs; j < chs; j++, lphs++) {
        if (lphs->grhst == hstBeam) {
            part.hs = *lphs;
            FLookupPart(&part);
            if (part.pbeam->grfAbilities & beamGatling) {
                lRange = 4;
            } else {
                if (part.pbeam->grfAbilities & beamSapper)
                    continue;
                lRange = part.pbeam->dRangeMax;
            }
            if (fStarbase) {
                lRange++;
            }
            lPow += (uint32_t)((uint32_t)((uint32_t)(lRange * lRange) * lphs->cItem) * part.pbeam->dp);
        }
    }
    if (lPow <= 0) {
        return 0;
    }
    return lPow;
}

int16_t PctCloakFromLpfl(FLEET *lpfl) {
    int16_t j;
    double  dcPts;
    double  dwtFleet;
    int16_t i;
    int32_t cPtsCur;
    int16_t fUseFloat;
    HUL    *lphul;
    int32_t wtFleet;
    int16_t cScore;
    int32_t cPts;
    int32_t wtFleetCur;
    int16_t chs;
    HS     *lphs;

    wtFleet = 0;
    cPts = 0;
    dwtFleet = 0.0;
    dcPts = 0.0;
    fUseFloat = FALSE;
    for (i = 0; i < 16; i++) {
        if (lpfl->rgcsh[i] > 0) {
            lphul = &rglpshdef[lpfl->iPlayer][i].hul;
            chs = lphul->chs;
            wtFleetCur = (uint32_t)(lpfl->rgcsh[i] * (uint32_t)lphul->wtEmpty);
            cPtsCur = 0;
            if (GetRaceStat(&rgplr[lpfl->iPlayer], rsMajorAdv) == raStealth) {
                cPtsCur += 300;
            }
            j = 0;
            lphs = lphul->rghs;
            while (j < chs) {
                cPtsCur += CPtsCloakFromLphs(lphs);
                j++;
                lphs++;
            }
            if (cPtsCur > 0) {
                if (!fUseFloat && (cPtsCur > 4000 || wtFleetCur > 500000 || cPts > 100000000 || wtFleet > 50000000)) {
                    dcPts = Sf64FromI32(cPts);
                    dwtFleet = Sf64FromI32(wtFleet);
                    fUseFloat = TRUE;
                }
                if (fUseFloat) {
                    dcPts = Sf64From80((Sf80Add(Sf80From64(dcPts), Sf80Mul(Sf80FromI32(cPtsCur), Sf80FromI32(wtFleetCur)))));
                } else {
                    cPts += (uint32_t)(cPtsCur * wtFleetCur);
                }
            }
            if (fUseFloat) {
                dwtFleet = Sf64From80((Sf80Add(Sf80From64(dwtFleet), Sf80FromI32(wtFleetCur))));
            } else {
                wtFleet += wtFleetCur;
            }
        }
    }
    if (!fUseFloat && cPts == 0) {
        return 0;
    }
    if (GetRaceStat(&rgplr[lpfl->iPlayer], rsMajorAdv) != raStealth) {
        if (fUseFloat) {
            for (i = 0; i <= 3; i++) {
                dwtFleet = Sf64From80((Sf80Add(Sf80From64(dwtFleet), Sf80FromI32(lpfl->rgwtMin[i]))));
            }
        } else {
            for (i = 0; i <= 3; i++) {
                wtFleet += lpfl->rgwtMin[i];
            }
        }
    }
    if (fUseFloat) {
        cPts = Sf80ToI32((Sf80Div(Sf80From64(dcPts), Sf80From64(dwtFleet))));
    } else {
        cPts = (int32_t)(cPts / wtFleet);
    }
    if (cPts < 0) {
        return 0;
    }
    cScore = LOWORD(cPts);
    if (cScore <= 100) {
        return cScore >> 1;
    }
    cScore -= 100;
    if (cScore <= 200) {
        return (cScore >> 3) + 0x32;
    }
    cScore -= 200;
    if (cScore <= 312) {
        return cScore / 24 + 75;
    }
    cScore -= 312;
    if (cScore <= 512) {
        return (cScore >> 6) + 0x58;
    }
    if (cScore < 1000) {
        return (cScore >= 768) + 96;
    }
    return 98;
}

int16_t CPtsCloakFromLphs(HS *lphs) {
    int16_t cPts;
    PART    part;

    cPts = 0;
    if (lphs->cItem <= 0) {
        return 0;
    }
    switch (lphs->grhst) {
    case hstSpecialE:
        if (lphs->iItem < ispecialETransportCloaking || lphs->iItem > ispecialEMultiFunctionPod)
            break;
        part.hs = *lphs;
        FLookupPart(&part);
        cPts = part.pspecial->grAbility;
        break;
    case hstSpecialM:
        if (lphs->iItem != ispecialMMultiCargoPod)
            break;
        cPts = 20;
        break;
    case hstScanner:
        if (lphs->iItem != iscannerChameleonScanner)
            break;
        cPts = 40;
        break;
    case hstArmor:
        if (lphs->iItem == iarmorDepletedNeutronium) {
            cPts = 50;
            break;
        }
        if (lphs->iItem != iarmorMegaPolyShell)
            break;
        cPts = 40;
        break;
    case hstShield:
        if (lphs->iItem == ishieldShadowShield) {
            cPts = 70;
            break;
        }
        if (lphs->iItem != ishieldLangstonShell)
            break;
        cPts = 20;
        break;
    case hstBeam:
        if (lphs->iItem != ibeamMultiContainedMunition)
            break;
        cPts = 20;
        break;
    case hstEngine:
        if (lphs->iItem != iengineEnigmaPulsar)
            break;
        cPts = 20;
        break;
    case hstMining:
        if (lphs->iItem == iminingAlienMiner) {
            cPts = 60;
        } else if (lphs->iItem == iminingOrbitalAdjuster) {
            cPts = 50;
        }
    }
    if (lphs->cItem > 1) {
        cPts *= lphs->cItem;
    }
    return cPts;
}

void MarkTechsSeen(HUL *lphul, int16_t iplr) {
    int16_t iplrSav;
    int16_t iTech;
    int16_t ihs;
    PART    part;

    iplrSav = idPlayer;
    idPlayer = iplr;
    part.hs.grhst = hstHull;
    part.hs.iItem = lphul->ihuldef;
    FLookupPart(&part);
    for (iTech = 0; iTech < 6; iTech++) {
        rgTechBattle[iTech] = (int16_t)rgTechBattle[iTech] <= part.phul->rgTech[iTech] ? part.phul->rgTech[iTech] : rgTechBattle[iTech];
    }
    for (ihs = 0; ihs < lphul->chs; ihs++) {
        if (lphul->rghs[ihs].cItem != 0) {
            part.hs = lphul->rghs[ihs];
            FLookupPart(&part);
            for (iTech = 0; iTech < 6; iTech++) {
                rgTechBattle[iTech] = (int16_t)rgTechBattle[iTech] <= part.pcom->rgTech[iTech] ? part.pcom->rgTech[iTech] : rgTechBattle[iTech];
            }
            iTech = -1;
            switch (part.hs.grhst) {
            case hstSpecialM:
                if (part.hs.iItem == ispecialMMultiCargoPod) {
                    iTech = 0;
                    break;
                }
                if (part.hs.iItem != ispecialMJumpGate)
                    break;
                iTech = 11;
                break;
            case hstSpecialE:
                if (part.hs.iItem != ispecialEMultiFunctionPod)
                    break;
                iTech = 1;
                break;
            case hstShield:
                if (part.hs.iItem != ishieldLangstonShell)
                    break;
                iTech = 2;
                break;
            case hstArmor:
                if (part.hs.iItem != iarmorMegaPolyShell)
                    break;
                iTech = 3;
                break;
            case hstMining:
                if (part.hs.iItem != iminingAlienMiner)
                    break;
                iTech = 4;
                break;
            case hstBomb:
                if (part.hs.iItem != ibombHushABoom)
                    break;
                iTech = 5;
                break;
            case hstBeam:
                if (part.hs.iItem != ibeamMultiContainedMunition)
                    break;
                iTech = 7;
                break;
            case hstTorp:
                if (part.hs.iItem != itorpAntiMatterTorpedo)
                    break;
                iTech = 6;
                break;
            case hstEngine:
                if (part.hs.iItem == iengineEnigmaPulsar) {
                    iTech = 9;
                }
            }
            if (iTech != -1 && rgTechTrader[iTech] < 25) {
                rgTechTrader[iTech] += part.hs.cItem;
                if (rgTechTrader[iTech] > 25) {
                    rgTechTrader[iTech] = 25;
                }
            }
        }
    }
    idPlayer = iplrSav;
    return;
}
