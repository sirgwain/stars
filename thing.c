#include "common.h"

THING *LpthNew(int16_t iplr, ThingType ith) {
    int16_t iItem;
    int16_t i;
    THING  *lpth;
    THING   thNew;

    if (cThing >= 4050) {
        return NULL;
    }
    memset(&thNew, 0, sizeof(THING));
    thNew.iplr = iplr;
    thNew.ith = ith;
    lpth = lpThings;
    i = 0;
    for (; i < cThing && thNew.idFull > lpth->idFull; lpth++) {
        i++;
    }
    if (i < cThing && thNew.idFull == lpth->idFull) {
        iItem = lpth->id;
        while (i < cThing) {
            if (iItem >= 511) {
                return NULL;
            }
            if (lpth->idFull != thNew.idFull)
                break;
            i++;
            thNew.id++;
            lpth++;
            iItem++;
        }
    }
    if (cThing >= cThingAlloc) {
        cThingAlloc += 10;
        if (!lpThings) {
            lpThings = LpAlloc(cThingAlloc * sizeof(THING), htThings);
        } else {
            lpThings = LpReAlloc(lpThings, cThingAlloc * sizeof(THING), htThings);
        }
        lpth = lpThings + i;
    }
    if (i < cThing) {
        memmove(lpth + 1, lpth, (cThing - i) * sizeof(THING));
    }
    cThing++;
    memcpy(lpth, &thNew, sizeof(THING));
    return lpth;
}

void FreeLpth(THING *lpth) {
    if (lpth < lpThings + (cThing - 1)) {
        memmove(lpth, lpth + 1, (cThing - (int16_t)((uint8_t *)lpth - (uint8_t *)lpThings) / 18 - 1) * sizeof(THING));
    }
    cThing--;
    return;
}

int16_t CPlanetsInCircle(POINT16 pt, int32_t r2) {
    int16_t  xStart;
    POINT16 *ppt;
    int16_t  yEnd;
    int16_t  dy;
    POINT16 *pptEnd;
    int16_t  yStart;
    int16_t  i;
    int16_t  r;
    int16_t  cPl;
    int16_t  dx;
    int16_t  xEnd;

    r = LOWORD(Sf80ToI32((Sf80Add(Sf80From64(Sf64Sqrt(Sf64FromI32(r2))), Sf80From64(0.9999)))));
    xStart = pt.x - r;
    xEnd = pt.x + r;
    yStart = pt.y - r;
    yEnd = pt.y + r;
    cPl = 0;
    pptEnd = &rgptPlan[game.cPlanMax];
    dx = rgptPlan[game.cPlanMax - 1].x - rgptPlan[0].x;
    i = LOWORD((int32_t)((int32_t)((int16_t)(pt.x - rgptPlan[0].x) * game.cPlanMax) / dx));
    if (i >= game.cPlanMax) {
        i = game.cPlanMax - 1;
    }
    if (i < 0) {
        i = 0;
    }
    for (ppt = &rgptPlan[i]; ppt->x >= xStart && ppt > rgptPlan; ppt--) {
    }
    for (; ppt->x <= xEnd && ppt < pptEnd; ppt++) {
        if (ppt->x >= xStart && ppt->y >= yStart && ppt->y <= yEnd) {
            dx = ppt->x - pt.x;
            dy = ppt->y - pt.y;
            if ((uint32_t)(dx * dx) + (uint32_t)(dy * dy) <= r2) {
                cPl++;
            }
        }
    }
    return cPl;
}

int16_t IValidateWormholePos(THING *lpthWorm) {
    int16_t iRet;
    POINT16 pt;
    int32_t dy;
    THING  *lpthMac;
    FLEET  *lpfl;
    int16_t ifl;
    int16_t i;
    THING  *lpth;
    int16_t dUni;
    int32_t dx;
    int32_t l;

    iRet = 0;
    dUni = 400 * game.mdSize + 1400;
    pt = lpthWorm->pt;
    if (pt.x < 1000 || pt.y < 1000) {
        return 15;
    }
    if (pt.x > dUni || pt.y > dUni) {
        return 15;
    }
    lpth = lpThings;
    lpthMac = lpThings + cThing;
    for (; lpth < lpthMac; lpth++) {
        if (pt.x == lpth->pt.x && pt.y == lpth->pt.y && lpth != lpthWorm) {
            return 15;
        }
    }
    for (i = 0; i < game.cPlanMax; i++) {
        if (pt.x == rgptPlan[i].x && pt.y == rgptPlan[i].y) {
            return 15;
        }
    }
    for (ifl = 0; ifl < cFleet; ifl++) {
        lpfl = rglpfl[ifl];
        if (!rglpfl[ifl])
            break;
        if (pt.x == lpfl->pt.x && pt.y == lpfl->pt.y) {
            return 15;
        }
    }
    if (pt.x < 1010 || pt.y < 1010 || pt.x > dUni - 10 || pt.y > dUni - 10) {
        iRet |= 4;
    }
    lpth = lpThings;
    lpthMac = lpThings + cThing;
    for (; lpth < lpthMac; lpth++) {
        if (lpth->ith == ithWormhole && lpth != lpthWorm) {
            dx = (int16_t)(pt.x - lpth->pt.x);
            dy = (int16_t)(pt.y - lpth->pt.y);
            l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
            if (lpth->idFull == lpthWorm->thw.idPartner) {
                if (l < 4900) {
                    if (l < 25) {
                        iRet |= 8;
                    } else if (l < 100) {
                        iRet |= 4;
                    } else if (l < 900) {
                        iRet |= 2;
                    } else {
                        iRet |= 1;
                    }
                }
            } else if (l < 900) {
                if (l < 16) {
                    iRet |= 8;
                } else if (l < 64) {
                    iRet |= 4;
                } else if (l < 225) {
                    iRet |= 2;
                } else {
                    iRet |= 1;
                }
            }
        }
    }
    for (i = 0; i < game.cPlanMax; i++) {
        dx = (int16_t)(pt.x - rgptPlan[i].x);
        dy = (int16_t)(pt.y - rgptPlan[i].y);
        l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
        if (l < 784) {
            if (l < 25) {
                iRet |= 8;
            } else if (l < 100) {
                iRet |= 4;
            } else if (l < 400) {
                iRet |= 2;
            } else {
                iRet |= 1;
            }
        }
    }
    return iRet;
}

int16_t PctWormholeMoves(THING *lpth) {
    int16_t pct;

    pct = (int16_t)lpth->thw.cLastMove / 5;
    pct -= 2 - lpth->thw.iStable;
    if (pct < 0) {
        pct = 0;
    } else if (pct > 6) {
        pct = 6;
    }
    return pct;
}

void DoThingInteractions(int16_t fPostMove) {
    int32_t     wtThreshhold;
    uint16_t    grbitPlrTrader;
    int16_t     iplr;
    int32_t     wtMin;
    POINT16     pt;
    int16_t     iplrSav;
    uint8_t     rgTech[6];
    int32_t     wtNext;
    int32_t     dy;
    THING      *lpthMac;
    PLANET     *lpplMac;
    PLANET     *lppl;
    int16_t     i;
    int16_t     ifl;
    FLEET      *lpfl;
    THING      *lpth;
    MessageId   idm;
    int16_t     cPlrTrueMaxTech;
    int32_t     dx;
    int16_t     fMaxTech;
    int32_t     l;
    int32_t     cTech;
    int16_t     iLowest;
    int16_t     cTechCur;
    int32_t     lSpent;
    int16_t     iGoto;
    GrbitTrader grbitTrader;
    int16_t     cTry;
    int16_t     iOffset;
    int16_t     ish;
    SHDEF       shdef;
    SHDEF      *lpshdefDest;
    FLEET      *lpflNew;
    int16_t     cGive;
    int16_t     iLvl;
    int16_t     iPass;

    if (fPostMove) {
        lpth = lpThings;
        lpthMac = lpThings + cThing;
        for (; lpth < lpthMac; lpth++) {
            if (lpth->ith == ithMysteryTrader) {
                pt = lpth->pt;
                for (ifl = 0; ifl < cFleet; ifl++) {
                    lpfl = rglpfl[ifl];
                    if (!rglpfl[ifl])
                        break;
                    if (!lpfl->fDead && lpfl->pt.x == pt.x && lpfl->pt.y == pt.y) {
                        wtMin = 0;
                        for (i = 0; i <= 2; i++) {
                            wtMin += lpfl->rgwtMin[i];
                        }
                        if (wtMin < 5000) {
                            if (!lpfl->fHereAllTurn) {
                                FSendPlrMsg2(lpfl->iplr, idmMysteryTraderHasRefusedGiveCaptainAudience, lpfl->id | 0x8000, lpfl->id, 0);
                            }
                        } else {
                            cPlrTrueMaxTech = !rgplr[lpfl->iPlayer].fCrippled ? 26 : 10;
                            for (i = 0; i < 6 && rgplr[lpfl->iPlayer].rgTech[i] >= cPlrTrueMaxTech; i++) {
                            }
                            fMaxTech = i == 6;
                            grbitPlrTrader = rgplr[lpfl->iPlayer].grbitTrader;
                            iplr = lpfl->iPlayer;
                            if (1 << iplr & lpth->tht.grbitPlr) {
                                FSendPlrMsg2(lpfl->iplr, idmMysteryTraderEyesCaptainSuspiciouslySuggestsHe, lpfl->id | 0x8000, lpfl->id, 0);
                            } else {
                                lpth->tht.grbitPlr |= 1 << iplr;
                                FRemovePlayerMessage(iplr, idmHasCompletedAssignedOrders, lpfl->id | 0x8000);
                                lpfl->fDead = TRUE;
                                if (lpth->tht.grbitTrader == grbitTraderNone || (lpth->tht.grbitTrader & grbitPlrTrader)) {
                                    if (fMaxTech) {
                                        if (Random(5) != 0)
                                            goto LGivePart;
                                        idm = idmHasAbsorbedMysteryTraderHoweverTraderUnable2;
                                        FSendPlrMsg2(lpfl->iplr, idm, gotoNone, WFromLpfl(lpfl), 0);
                                        continue;
                                    } else {
                                        cTechCur = 0;
                                        cTech = (int32_t)((wtMin - 5000) / 1200) + 6;
                                        for (i = 0; i < 6; i++) {
                                            cTechCur += rgplr[lpfl->iPlayer].rgTech[i];
                                        }
                                        if (cTech > 10) {
                                            cTech = 10;
                                        }
                                        if (cTechCur >= 108) {
                                            cTech = 1;
                                        } else if (cTechCur >= 96) {
                                            cTech = 2;
                                        } else if (cTechCur >= 84) {
                                            cTech -= 3;
                                        } else if (cTechCur >= 72) {
                                            cTech -= 2;
                                        } else if (cTechCur >= 60) {
                                            cTech--;
                                        }
                                        if ((grbitPlrTrader & 0x1fff) != 0x1fff) {
                                            idm = idmHasAbsorbedMysteryTraderTraderHasGiven;
                                        } else {
                                            idm = idmHasAbsorbedMysteryTraderReturnTraderHas;
                                        }
                                        FSendPlrMsg(lpfl->iplr, idm, gotoNone, WFromLpfl(lpfl), LOWORD(cTech), 0, 0, 0, 0, 0);
                                        while (cTech-- != 0) {
                                            if (Random(4) < 3) {
                                                iLowest = Random(6);
                                                if (rgplr[iplr].rgTech[iLowest] < cPlrTrueMaxTech)
                                                    goto LGiveITech;
                                            }
                                            iLowest = 0;
                                            for (i = 1; i < 6; i++) {
                                                if (rgplr[iplr].rgTech[i] < rgplr[iplr].rgTech[iLowest]) {
                                                    iLowest = i;
                                                }
                                            }
                                            if (rgplr[iplr].rgTech[iLowest] >= cPlrTrueMaxTech)
                                                break;
                                        LGiveITech:
                                            memcpy(rgTech, rgplr[iplr].rgTech, 6);
                                            rgTech[iLowest]++;
                                            iplrSav = idPlayer;
                                            idPlayer = iplr;
                                            wtNext = CostOfDevelopingItem(rgTech);
                                            idPlayer = iplrSav;
                                            lSpent = rgplr[iplr].rgResSpent[iLowest];
                                            lSpent = (int32_t)(lSpent * 2);
                                            if (wtNext > 0) {
                                                lSpent += wtNext;
                                            }
                                            rgplr[iplr].rgResSpent[iLowest] = lSpent;
                                            UpdateResearchStatus(FALSE);
                                        }
                                        continue;
                                    }
                                }
                            LGivePart:
                                cTry = 25;
                                grbitTrader = lpth->tht.grbitTrader;
                                if (grbitTrader == grbitTraderNone) {
                                    grbitTrader = 1 << Random(13);
                                }
                                for (; (grbitTrader & grbitPlrTrader) != 0 && cTry-- > 0; grbitTrader = 1 << Random(13)) {
                                }
                                if (cTry <= 0) {
                                    grbitTrader = grbitTraderLifeboat;
                                }
                                lpth->tht.grbitPlr |= 1 << iplr;
                                if (grbitTrader == grbitTraderLifeboat) {
                                    if (!rgplr[iplr].fAi) {
                                        iOffset = Random(4 - (game.turn > 100));
                                        if (iOffset >= 1) {
                                            iOffset = Random(2) + 1;
                                        }
                                        memmove(&shdef, LpshdefT() + (iOffset + 19), sizeof(SHDEF));
                                        ish = IshFindSimilarDesign(&shdef.hul, iplr);
                                        if (ish < 0) {
                                            do {
                                                ish++;
                                            } while (ish < 16 && !rglpshdef[iplr][ish].fFree);
                                        }
                                        if (ish >= 16 || rgplr[lpfl->iplr].cFleet >= 0x200) {
                                        LNoLifeboat:
                                            FSendPlrMsg2(lpfl->iplr, idmHasAbsorbedMysteryTraderReturnTraderTried, gotoNone, WFromLpfl(lpfl), 0);
                                            continue;
                                        }
                                        if (Random(3) == 0) {
                                            cGive = 2;
                                        } else {
                                            cGive = 1;
                                        }
                                        if (game.turn > 100 && !game.fSinglePlr) {
                                            cGive += Random((uint32_t)game.turn / 100 + 1);
                                        }
                                        if (cGive > 5) {
                                            cGive = 5;
                                        }
                                        if (iOffset > 0) {
                                            cGive += Random(cGive + 1);
                                        }
                                        lpflNew = LpflNew(iplr, lpfl->idPlanet);
                                        if (!lpflNew)
                                            goto LNoLifeboat;
                                        FSendPlrMsg2(lpfl->iplr, idmHasAbsorbedMysteryTraderReturnHaveGiven3, lpflNew->id | 0x8000, WFromLpfl(lpfl), cGive);
                                        if (lpflNew->id < lpfl->id) {
                                            ifl++;
                                        }
                                        lpflNew->pt = lpfl->pt;
                                        lpflNew->lpplord->rgord[0].pt = lpfl->pt;
                                        lpflNew->fHereAllTurn = TRUE;
                                        lpshdefDest = rglpshdef[iplr] + ish;
                                        if (lpshdefDest->fFree) {
                                            *lpshdefDest = shdef;
                                            lpshdefDest->ishdef = ish;
                                            lpshdefDest->fGift = TRUE;
                                            lpshdefDest->cBuilt = 0;
                                            lpshdefDest->cExist = 0;
                                            rgplr[iplr].cShDef++;
                                            idPlayer = lpfl->iplr;
                                            UpdateShdefCost(lpshdefDest);
                                            idPlayer = iplrNone;
                                        }
                                        lpshdefDest->cBuilt += cGive;
                                        lpshdefDest->cExist += cGive;
                                        lpflNew->rgcsh[ish] = cGive;
                                        lpflNew->rgwtMin[4] = LGetFleetStat(lpflNew, 1);
                                        continue;
                                    }
                                } else {
                                    idm = IdmGiveTraderPart(grbitTrader, iplr, &iGoto);
                                    FSendPlrMsg2(lpfl->iplr, idm, iGoto, WFromLpfl(lpfl), 0);
                                }
                            }
                        }
                    }
                }
                lppl = lpPlanets;
                lpplMac = lpPlanets + cPlanet;
                for (; lppl < lpplMac; lppl++) {
                    if (lppl->iPlayer != iplrNone && lppl->fStarbase && rgplr[lppl->iPlayer].fAi && rgplr[lppl->iPlayer].lvlAi >= lvlAiTough &&
                        !(1 << lppl->iPlayer & lpth->tht.grbitPlr)) {
                        dx = (int16_t)(rgptPlan[lppl->id].x - pt.x);
                        if (dx > 100)
                            break;
                        dy = (int16_t)(rgptPlan[lppl->id].y - pt.y);
                        l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                        if (l <= 10000) {
                            wtNext = 0;
                            for (i = 0; i < 3; i++) {
                                wtNext += lppl->rgwtMin[i];
                            }
                            iplr = lppl->iPlayer;
                            wtThreshhold = (uint32_t)(rgplr[iplr].lvlAi == lvlAiTough ? 3500 : 5000);
                            if (wtNext >= wtThreshhold) {
                                if (lpth->tht.grbitTrader == grbitTraderNone)
                                    goto LAutoTech;
                                cTry = 50;
                                for (grbitTrader = lpth->tht.grbitTrader; (grbitTrader & rgplr[iplr].grbitTrader) != 0 && cTry-- > 0;
                                     grbitTrader = 1 << Random(13)) {
                                }
                                if (cTry <= 0)
                                    goto LAutoTech;
                                rgplr[iplr].grbitTrader |= grbitTrader;
                            LChgMin:
                                lpth->tht.grbitPlr |= 1 << iplr;
                                for (i = 2; wtNext > 0 && i >= 0; i--) {
                                    wtMin = wtNext < lppl->rgwtMin[i] ? wtNext : lppl->rgwtMin[i];
                                    lppl->rgwtMin[i] -= wtMin;
                                    wtNext -= wtMin;
                                }
                                continue;
                            LAutoTech:
                                iLvl = 0;
                                for (i = 0; i < 6; i++) {
                                    iLvl += rgplr[iplr].rgTech[i];
                                }
                                cPlrTrueMaxTech = !rgplr[lpfl->iPlayer].fCrippled ? 26 : 10;
                                if (iLvl >= 6 * cPlrTrueMaxTech - 6)
                                    continue;
                                for (iPass = 0; iPass < 6; iPass++) {
                                    iLvl = 0;
                                    for (i = 1; i < 6; i++) {
                                        if (rgplr[iplr].rgTech[i] < rgplr[iplr].rgTech[iLvl]) {
                                            iLvl = i;
                                        }
                                    }
                                    rgplr[iplr].rgTech[iLvl]++;
                                }
                                wtNext = wtThreshhold;
                                goto LChgMin;
                            }
                        }
                    }
                }
            }
        }
    }
    return;
}

int16_t IdmGiveTraderPart(GrbitTrader grbitTrader, int16_t iplr, uint16_t *piGoto) {
    uint16_t  iGoto;
    MessageId idm;

    rgplr[iplr].grbitTrader |= grbitTrader;
    idm = idmHasAbsorbedMysteryTraderHaveGivenPlans;
    switch (grbitTrader) {
    case grbitTraderCargo:
    default:
        iGoto = 0xcc04;
        break;
    case grbitTraderSpecial:
        iGoto = 0xcb04;
        break;
    case grbitTraderShield:
        iGoto = 0xc206;
        break;
    case grbitTraderArmor:
        iGoto = 0xc309;
        break;
    case grbitTraderMiner:
        iGoto = 0xc706;
        break;
    case grbitTraderBomb:
        iGoto = 0xc608;
        break;
    case grbitTraderBeam:
        iGoto = 0xc412;
        break;
    case grbitTraderTorp:
        iGoto = 0xc507;
        break;
    case grbitTraderHull:
        idm = idmHasAbsorbedMysteryTraderReturnHaveGiven;
        iGoto = 0xce1e;
        break;
    case grbitTraderEngine:
        iGoto = 0xc008;
        break;
    case grbitTraderGenesis:
        idm = idmHasAbsorbedMysteryTraderReturnHaveGiven2;
        iGoto = 0xcf0e;
        break;
    case grbitTraderJumpgate:
        iGoto = 0xcc09;
    }
    *piGoto = iGoto;
    return idm;
}
