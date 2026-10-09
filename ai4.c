#include "common.h"

void DoCyberAiTurn(PROD *rgprod) {
    int32_t        rgResCost[4];
    int16_t        cSBDefenderFleets;
    int32_t        rgResAvail[4];
    FLEET         *lpflEnemy;
    int32_t        cExistColony;
    int16_t        cFlMineLayers;
    uint8_t        rgRecycleShdef[16];
    PLANET        *lppl;
    int16_t        cMineLayers;
    int16_t        ifl;
    int16_t        i;
    FLEET         *lpfl;
    int16_t        cFlDestroyers;
    int16_t        cFlArmadas;
    int16_t        iLatestSBDefender;
    int32_t        cExistCargo;
    int16_t        iroCur;
    int16_t        j;
    FLEET         *lpflAttack;
    int16_t        iLatestCargo;
    int16_t        pctValueIdeal;
    int16_t        ipl;
    int16_t       *lpiHistSize;
    int16_t        iBuilt;
    int16_t        iLatestDestroyer;
    uint16_t       cRecyclePeriod;
    int16_t        ishdefLatestSB;
    int16_t        pctValue;
    PLANET        *lpplMac;
    CYBERINFO     *lpciPlan;
    int32_t        lNewPop;
    int16_t        iLatestBattle;
    int16_t        dOffsetPlanTemp;
    int16_t        iAttackStr;
    CYBERINFOTEMP *lpciPlanTemp;
    int16_t        idPlanDst;
    int16_t        fWrite;
    int16_t        fScrap;
    SHDEF          shdef;
    uint8_t        rgRecycleSBShdef[16];
    int16_t        id;
    uint8_t       *lpb;
    int16_t        iStrDef;
    ORDER          ord;
    int16_t        cFr;
    int16_t        iSBDef;
    PLANET        *lpplEnemy;

    dOffsetPlanTemp = game.cPlanMax * 2 + 2;
    cSBDefenderFleets = 0;
    cFlMineLayers = 0;
    cMineLayers = 0;
    cFlArmadas = 0;
    cFlDestroyers = 0;
    lpiHistSize = (int16_t *)vlpbAiData;
    if (*lpiHistSize == 2) {
        memset(vlpbAiData, 0, 0x2000);
        *lpiHistSize = game.cPlanMax * 2 + 2;
    }
    fMarkedPlanets = FALSE;
    iroCur = IroEnsureAi(vrgAiCybertronResOrder, cAiCybertronResOrder, &ishdefLatestSB, 17);
    EnsureCyberAiShdefs(iroCur);
    MergeAllShdefs(1);
    MergeAllShdefs(48);
    MergeAllShdefs(-16384);
    MergeAllShdefs(960);
    MergeAllShdefs(15360);
    iAttackStr = 1;
    if (game.turn > 50) {
        iAttackStr += (uint32_t)(game.turn - 50) / 10;
    }
    if (game.turn > 100) {
        iAttackStr += (uint32_t)(game.turn - 100) / 10 * ((uint32_t)game.turn / 100);
    }
    j = 3;
    if (game.turn > 130) {
        j += (uint32_t)(game.turn - 120) / 20;
    }
    if (j > 50) {
        j = 50;
    }
    vrgAiCyberArmadaPotency[0] = j;
    vrgAiCyberArmadaPotency[1] = (int16_t)(j & 0xff) / 2;
    j = 6;
    if (game.turn > 115) {
        j += (uint32_t)(game.turn - 100) / 22;
    }
    if (j > 12) {
        j = 12;
    }
    vrgAiCyberArmadaPotency[2] = j;
    vrgAiCyberArmadaPotency[3] = 3 >= j / 2 - 1 ? j / 2 - 1 : 3;
    memset(rgRecycleShdef, 0, 16);
    if (game.turn < 120) {
        cRecyclePeriod = 50;
    } else {
        cRecyclePeriod = game.turn < 200 ? 70 : game.turn < 400 ? 100 : 300;
    }
    CheckAiShdefStatus(4, 5, cRecyclePeriod, &iLatestDestroyer, rgRecycleShdef);
    if (iLatestDestroyer != ishdefNone) {
        rgRecycleShdef[iLatestDestroyer] = 0;
    }
    CheckAiShdefStatus(14, 15, cRecyclePeriod, &iLatestSBDefender, rgRecycleShdef);
    if (iLatestSBDefender != ishdefNone) {
        rgRecycleShdef[iLatestSBDefender] = 0;
    }
    cExistCargo = CheckAiShdefStatus(2, 3, cRecyclePeriod, &iLatestCargo, rgRecycleShdef);
    if (iLatestCargo != ishdefNone) {
        rgRecycleShdef[iLatestCargo] = 0;
    }
    iLatestBattle = ishdefNone;
    for (i = 0; i <= 1; i++) {
        if (!rgshdef[i * 4 + 6].fFree && (uint16_t)(game.turn - rgshdef[i * 4 + 6].turn) > cRecyclePeriod) {
            fScrap = TRUE;
            for (j = i * 4 + 9; j >= i * 4 + 6; j--) {
                if (!rgshdef[j].fFree) {
                    if (rgshdef[j].cExist == 0 && (fScrap || j != i * 4 + 6)) {
                        shdef = rgshdef[j];
                        shdef.fFree = TRUE;
                        FChangeAiShdef(&shdef, j);
                    } else {
                        rgRecycleShdef[j] = 1;
                        fScrap = FALSE;
                    }
                }
            }
        }
        if (!rgshdef[i * 4 + 6].fFree && (iLatestBattle == ishdefNone || rgshdef[iLatestBattle * 4 + 6].turn < rgshdef[i * 4 + 6].turn)) {
            iLatestBattle = i;
        }
    }
    cExistColony = rgshdef[1].cExist;
    if (game.turn > 80) {
        SplitOutShdefs(rgRecycleShdef);
        memset(rgRecycleSBShdef, 0, 16);
        rgRecycleSBShdef[0] = 2;
        SplitOutShdefs(rgRecycleSBShdef);
        memset(rgRecycleSBShdef, 0, 16);
        rgRecycleSBShdef[1] = 2;
        SplitOutShdefs(rgRecycleSBShdef);
        memset(rgRecycleSBShdef, 0, 16);
        rgRecycleSBShdef[3] = 2;
        rgRecycleSBShdef[2] = 2;
        SplitOutShdefs(rgRecycleSBShdef);
    }
    lpciPlanTemp = (CYBERINFOTEMP *)(vlpbAiData + dOffsetPlanTemp);
    memset(lpciPlanTemp, 0, game.cPlanMax * sizeof(CYBERINFOTEMP));
    lppl = lpPlanets;
    lpplMac = lpPlanets + cPlanet;
    for (; lppl < lpplMac; lppl++) {
        lpciPlan = (CYBERINFO *)(vlpbAiData + (lppl->id * 2 + 2));
        lpciPlanTemp = (CYBERINFOTEMP *)(vlpbAiData + (dOffsetPlanTemp + lppl->id * 2));
        if (lpciPlan->iPktTarget > 0) {
            lpciPlan->iPktTarget += 0x7ff;
        }
        if (lppl->iPlayer != idPlayer && lppl->iPlayer != iplrNone) {
            i = lppl->uPopGuess / 250 + 1;
            if (i > 6) {
                i = 6;
            }
            if (lppl->fStarbase) {
                i++;
            }
            vlpbAiPlanet[lppl->id * 16 + 10] = i;
            vlpbAiPlanet[lppl->id * 16 + 9] = 1;
        } else if (lppl->iPlayer == idPlayer && lppl->fStarbase) {
            switch (lppl->isb) {
            case 1:
            case 3:
            case 6:
            case 8:
                if (lppl->rgwtMin[0] < 10) {
                    lpciPlanTemp->fNeedsMin1 = TRUE;
                }
                if (lppl->rgwtMin[1] < 10) {
                    lpciPlanTemp->fNeedsMin2 = TRUE;
                }
                if (lppl->rgwtMin[2] >= 10)
                    break;
                lpciPlanTemp->fNeedsMin3 = TRUE;
                break;
            default:
                if (lppl->rgwtMin[0] < 1000) {
                    lpciPlanTemp->fNeedsMin1 = TRUE;
                }
                if (lppl->rgwtMin[1] < 1000) {
                    lpciPlanTemp->fNeedsMin2 = TRUE;
                }
                if (lppl->rgwtMin[2] < 1000) {
                    lpciPlanTemp->fNeedsMin3 = TRUE;
                }
            }
        }
    }
    lpflAttack = NULL;
    lpflEnemy = NULL;
    for (ifl = 0; ifl < cFleet; ifl++) {
        lpfl = rglpfl[ifl];
        if (!rglpfl[ifl])
            break;
        if (!fMarkedPlanets) {
            IdNearestColonizablePlanet(lpfl, NULL);
        }
        if (lpfl->iPlayer != idPlayer) {
            lpfl->lpflNext = lpflEnemy;
            lpflEnemy = lpfl;
        } else if (lpfl->rgcsh[14] > 0 || lpfl->rgcsh[15] > 0) {
            cSBDefenderFleets++;
        } else {
            if (lpfl->rgcsh[0] > 0) {
                cFlMineLayers++;
                cMineLayers += lpfl->rgcsh[0];
            }
            for (i = 4; i <= 13 && lpfl->rgcsh[i] <= 0; i++) {
            }
            if (i <= 13) {
                lpfl->lpflNext = lpflAttack;
                lpflAttack = lpfl;
                if (j > 5 && ((lpfl->cord > 1 && lpfl->lpplord->rgord[1].grobj == grobjPlanet) || lpfl->idPlanet != idPlanetDeepSpace)) {
                    if (lpfl->cord > 1 && lpfl->lpplord->rgord[1].grobj == grobjPlanet) {
                        id = lpfl->lpplord->rgord[1].id;
                    } else {
                        id = lpfl->idPlanet;
                    }
                    lpb = vlpbAiPlanet + (10 + 16 * id);
                    if (*lpb != 0) {
                        *lpb |= 0x80;
                    }
                    cFlArmadas++;
                } else {
                    cFlDestroyers++;
                }
            } else if ((lpfl->rgcsh[2] > 0 || lpfl->rgcsh[3] > 0) && lpfl->cord > 1 && lpfl->rgwtMin[3] > 0 && lpfl->lpplord->rgord[1].grobj == grobjPlanet &&
                       lpciPlanTemp[lpfl->lpplord->rgord[1].id].cFreightersDst < 3) {
                lpciPlanTemp[lpfl->lpplord->rgord[1].id].cFreightersDst = lpciPlanTemp[lpfl->lpplord->rgord[1].id].cFreightersDst + 1;
            }
        }
    }
    UpdateProgressGauge(progressStep4);
    for (ifl = 0; ifl < cFleet; ifl++) {
        lpfl = rglpfl[ifl];
        if (!rglpfl[ifl])
            break;
        if (lpfl->iPlayer == idPlayer) {
            for (i = 0; i < 16 && (lpfl->rgcsh[i] <= 0 || rgRecycleShdef[i] != 0); i++) {
            }
            if (i == 16) {
                if (lpfl->idPlanet != idPlanetDeepSpace) {
                    lppl = LpplFromId(lpfl->idPlanet);
                    if (lppl && lppl->iPlayer == idPlayer && (lppl->fStarbase || Random(5) == 0)) {
                        ChangeMainObjSel(grobjFleet, lpfl->id);
                        sel.fl.lpplord->rgord[0].grTask = grTaskScrap;
                        FLookupFleet(idWriteBack, &sel.fl);
                        continue;
                    }
                }
                if ((lpfl->cord > 1 && lpfl->idPlanet == idPlanetDeepSpace) || FMoveToNearestStarbase(lpfl, FALSE))
                    continue;
            }
            if ((lpfl->rgcsh[14] > 0 || lpfl->rgcsh[15] > 0) && lpfl->idPlanet != idPlanetDeepSpace) {
                iStrDef = 0;
                if ((uint16_t)(game.turn - rgshdef[14].turn) < (uint16_t)(cRecyclePeriod - 10)) {
                    iStrDef += lpfl->rgcsh[14];
                }
                if ((uint16_t)(game.turn - rgshdef[15].turn) < (uint16_t)(cRecyclePeriod - 10)) {
                    iStrDef += lpfl->rgcsh[15];
                }
                if (iStrDef > 0) {
                    lpciPlanTemp[lpfl->idPlanet].fDefended = TRUE;
                    lpciPlanTemp[lpfl->idPlanet].fNeedDefenders = iStrDef < iAttackStr * 2;
                }
            } else {
                for (i = 4; i <= 13 && lpfl->rgcsh[i] <= 0; i++) {
                }
                if (i <= 13) {
                    if (lpfl->rgcsh[4] > 0 || lpfl->rgcsh[4] > 0) {
                        if (lpfl->rgcsh[4] + lpfl->rgcsh[4] >= iAttackStr || lpfl->cord > 1) {
                            IdTargetAttack(lpfl, lpflAttack, lpflEnemy, game.fAisBand);
                        }
                        if (lpfl->cord == 1 && Random(100) < 75) {
                            FFindBuddyAndJoinUp(lpfl, 4, 5, 100, 200);
                        }
                    } else {
                        TargetCyberArmada(lpfl);
                        if (lpfl->cord == 1 && Random(100) < 75) {
                            if (i <= 9) {
                                FFindBuddyAndJoinUp(lpfl, 6, 9, 100, 200);
                            } else {
                                FFindBuddyAndJoinUp(lpfl, 10, 13, 100, 200);
                            }
                        }
                    }
                } else if (lpfl->cord <= 1) {
                    if (game.turn <= 5 && lpfl->rgcsh[0] > 0 && lpfl->cord == 1) {
                        ChangeMainObjSel(grobjFleet, lpfl->id);
                        sel.fl.lpplord->rgord[0].grTask = grTaskScrap;
                        FLookupFleet(idWriteBack, &sel.fl);
                    } else {
                        if (lpfl->rgcsh[1] > 0) {
                            idPlanDst = IdNearestColonizablePlanet(lpfl, NULL);
                            if (idPlanDst == idplNone) {
                                if (lpfl->lpplord->rgord[0].grobj != grobjPlanet)
                                    continue;
                                lpciPlanTemp[lpfl->lpplord->rgord[0].id].fIdleColonizers = TRUE;
                                continue;
                            }
                            ChangeMainObjSel(grobjFleet, lpfl->id);
                            if (lpfl->idPlanet != idPlanetDeepSpace) {
                                FLookupPlanet(lpfl->idPlanet, &sel.pl);
                                if (sel.pl.iPlayer == idPlayer) {
                                    XferAiSupply(grobjPlanet, lpfl->idPlanet, grobjFleet, lpfl->id, Colonists, 250);
                                    FLookupFleet(lpfl->id, &sel.fl);
                                }
                            }
                            if (idPlanDst != idplNone) {
                                if (!FColonizeAiFleet(lpfl, idPlanDst) || lpfl->cord <= 1)
                                    continue;
                                vlpbAiPlanet[lpfl->lpplord->rgord[1].id * 16 + 15] = 4;
                                continue;
                            }
                        }
                        if (lpfl->rgcsh[2] > 0 || lpfl->rgcsh[3] > 0) {
                            if (lpfl->iplan != 4) {
                                ChangeMainObjSel(grobjFleet, lpfl->id);
                                sel.fl.iplan = 4;
                                FLookupFleet(idWriteBack, &sel.fl);
                            }
                            DoCyberFreighter(lpfl, lpciPlanTemp);
                        } else if (lpfl->rgcsh[0] > 0 && game.turn > 40 && lpfl->cord == 1) {
                            if (lpfl->cord > 1) {
                                if (lpfl->lpplord->rgord[0].grTask != grTaskNone) {
                                    ClearAiCurrentTask(lpfl, TRUE);
                                }
                            } else if ((cFlMineLayers <= 55 && (cFlMineLayers <= 40 || Random(3) == 0)) || !FFindBuddyAndJoinUp(lpfl, 0, 0, 72, 108)) {
                                if (lpfl->rgcsh[0] >= 7 && Random(5) == 0) {
                                    idPlanDst = IdRandomPlanetNearby(lpfl->pt, 105, TRUE);
                                    if (idPlanDst != idplNone && idPlanDst != lpfl->idPlanet) {
                                        ClearAiCurrentTask(lpfl, TRUE);
                                        ord.id = idPlanDst;
                                        ord.grobj = grobjPlanet;
                                        ord.pt = rgptPlan[idPlanDst];
                                        ord.grTask = grTaskLayMines;
                                        ord.tlm.cTime = 5;
                                        ord.tlm.cTimeOld = 5;
                                        ord.fValidTask = TRUE;
                                        ord.iWarp = 4;
                                        FMoveAiFleet(lpfl, &ord, FALSE);
                                        continue;
                                    }
                                }
                                if (lpfl->lpplord->rgord[0].grTask != grTaskLayMines) {
                                    ChangeMainObjSel(grobjFleet, lpfl->id);
                                    sel.fl.lpplord->rgord[0].grTask = grTaskLayMines;
                                    sel.fl.lpplord->rgord[0].tlm.cTime = 5;
                                    sel.fl.lpplord->rgord[0].tlm.cTimeOld = 5;
                                    FLookupFleet(idWriteBack, &sel.fl);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    UpdateProgressGauge(progressStep4);
    for (ipl = 0; ipl < vclpplAi; ipl++) {
        lppl = vrglpplAi[ipl];
        if (!vrglpplAi[ipl])
            break;
        lpciPlan = (CYBERINFO *)(vlpbAiData + (lppl->id * 2 + 2));
        lpciPlanTemp = (CYBERINFOTEMP *)(vlpbAiData + (dOffsetPlanTemp + lppl->id * 2));
        ChangeMainObjSel(grobjPlanet, lppl->id);
        InitProduction(rgprod);
        fWrite = FALSE;
        lNewPop = (uint32_t)(lppl->rgwtMin[3] * PctTrueMaxGrowth(idPlayer));
        GetResourcesAvailable(lppl, rgResAvail);
        GetProdQCost(lppl, rgResCost);
        for (i = 0; i < 4; i++) {
            if (rgResCost[i] > rgResAvail[i])
                goto LFinishProduction;
        }
        pctValue = PctPlanetDesirability(lppl, idPlayer);
        pctValueIdeal = PctPlanetOptValue(lppl, idPlayer);
        if (pctValue < 10) {
            i = LOWORD((int32_t)((rgResAvail[3] - rgResCost[3]) / 70)) + 1;
            AddItemToQueue(mdIdleTerraform, i, grobjPlanet, addItemEnd);
            fWrite = TRUE;
        } else {
            if (pctValue < pctValueIdeal && rgResAvail[3] - rgResCost[3] > 70) {
                AddItemToQueue(mdIdleTerraform, 1, grobjPlanet, addItemEnd);
                fWrite = TRUE;
            }
            if (lppl->fStarbase) {
                switch (lppl->isb) {
                default:
                    if ((!lpciPlan->fBltColony || lNewPop > 5500) && !lpciPlanTemp->fIdleColonizers && cExistColony < 40 && FShouldPlanetBuildColonizer(lppl)) {
                        AddItemToQueue(1, 1, grobjFleet, addItemEnd);
                        lpciPlan->fBltColony = TRUE;
                        fWrite = TRUE;
                        if (lNewPop > 15000 && game.turn < 100) {
                            AddItemToQueue(1, 1, grobjFleet, addItemEnd);
                        }
                    } else {
                        lpciPlan->fBltColony = FALSE;
                    }
                    if (lppl->rgwtMin[3] > 2000 && !rgshdef[2].fFree && cExistCargo < 50 && lpciPlanTemp->cIdleFreighters < 1 &&
                        LpplFindClosestEnum(lppl, FEnumDropOffStage2) != 0) {
                        AddItemToQueue(iLatestCargo, 1, grobjFleet, addItemEnd);
                        fWrite = TRUE;
                    }
                    if (rgshdef[0].hul.ihuldef == ihuldefFrigate && Random(4) == 0 && cMineLayers < 10000) {
                        id = lppl->id;
                        cFr = 0;
                        for (ifl = 0; ifl < cFleet; ifl++) {
                            lpfl = rglpfl[ifl];
                            if (!rglpfl[ifl])
                                break;
                            if (lpfl->idPlanet == id && lpfl->rgcsh[0] > 0 && lpfl->iPlayer == idPlayer) {
                                cFr = lpfl->rgcsh[0];
                                break;
                            }
                        }
                        if ((cFr < 10 || (cFr < 17 && Random(10) == 0)) && Random(cFr * 2 + 1) == 0) {
                            AddItemToQueue(0, 4, grobjFleet, addItemEnd);
                            fWrite = TRUE;
                        }
                    }
                    iSBDef = !lpciPlanTemp->fNeedDefenders ? ishdefNone : iLatestSBDefender;
                    if (iSBDef == ishdefNone && !lpciPlanTemp->fDefended && cSBDefenderFleets < 40) {
                        lpplEnemy = LpplFindClosestEnum(lppl, FEnumCalcEnemyPlanets);
                        if (!lpplEnemy || LDistance2(rgptPlan[lppl->id], rgptPlan[lpplEnemy->id]) > 90000) {
                            iSBDef = Random(100) >= 10 ? ishdefNone : iLatestSBDefender;
                        } else {
                            iSBDef = Random(100) >= 50 ? ishdefNone : iLatestSBDefender;
                        }
                    }
                    if (cFlDestroyers > 120) {
                        iLatestDestroyer = ishdefNone;
                    }
                    if (cFlArmadas > 250) {
                        iLatestBattle = ishdefNone;
                    }
                    iBuilt = iAddAttackFleet(lppl, iAttackStr, iLatestDestroyer, iLatestBattle, iSBDef);
                    fWrite |= iBuilt != 0;
                    if (iBuilt == 1) {
                        cFlArmadas++;
                    }
                    if (iBuilt == 2) {
                        cSBDefenderFleets++;
                    }
                    if (iBuilt == 3) {
                        cFlDestroyers++;
                    }
                case 1:
                case 3:
                case 6:
                case 8:
                    break;
                }
            }
        }
    LFinishProduction:
        FinishProduction(fWrite);
    }
    UpdateProgressGauge(progressStep4);
    HandleBasicAiTasks(iroCur, rgprod, IshdefAiSBLatest(), rgResAvail, rgResCost);
    UpdateProgressGauge(progressStep4);
    DoCyberPackets();
    UpdateProgressGauge(progressStep4);
    FillProductionQueue();
    return;
}

void DoCyberPackets() {
    int16_t        fTwoMA;
    int32_t        rgResCost[4];
    int32_t        rgResAvail[4];
    int16_t        iWarp;
    PLANET        *lpplDst;
    PLANET        *lppl;
    int16_t        i;
    int16_t        ipl;
    int16_t        iWarpDst;
    CYBERINFO     *lpciPlan;
    PROD           rgprod[64];
    int16_t        dOffsetPlanTemp;
    int16_t        idPlanDst;
    CYBERINFOTEMP *lpciPlanTemp;
    int16_t        fWrite;
    int16_t        iPacketMax;
    int16_t        iMinLimit;
    int16_t        iPacketAdd;
    CYBERINFO     *lpciPlanDst;
    CYBERINFOTEMP *lpciPlanT;
    int16_t        iWarpSrc;
    int32_t       *plMinMax;
    int32_t        lPackets;
    int32_t        lMineral;
    int16_t        cResLeft;
    double         dDistance;
    double         dMod;
    int16_t        cPacket[3];
    int32_t        lMinNeeded;
    double         dDistanceTgt;
    int16_t        iMin;

    dOffsetPlanTemp = game.cPlanMax * 2 + 2;
    for (ipl = 0; ipl < vclpplAi; ipl++) {
        lppl = vrglpplAi[ipl];
        if (!vrglpplAi[ipl])
            break;
        if (lppl->fStarbase) {
            lpciPlan = (CYBERINFO *)(vlpbAiData + (lppl->id * 2 + 2));
            lpciPlanTemp = (CYBERINFOTEMP *)(vlpbAiData + (dOffsetPlanTemp + lppl->id * 2));
            ChangeMainObjSel(grobjPlanet, lppl->id);
            InitProduction(rgprod);
            fWrite = FALSE;
            GetResourcesAvailable(lppl, rgResAvail);
            GetProdQCost(lppl, rgResCost);
            if (!lpciPlan->fNeedScanPkt && (lppl->isb == 1 || lppl->isb == 3 || lppl->isb == 6 || lppl->isb == 8) &&
                (lppl->rgwtMin[0] > 700 || lppl->rgwtMin[1] > 700 || lppl->rgwtMin[2] > 700)) {
                iPacketMax = LOWORD((int32_t)(rgResAvail[3] / 2));
                iPacketMax /= 5;
                if (iPacketMax < 7) {
                    lpplDst = NULL;
                } else {
                    lpplDst = LpplFindClosestEnum(lppl, FEnumNeedMinerals);
                }
                if (lpplDst) {
                    lpciPlanDst = (CYBERINFO *)(vlpbAiData + (lpplDst->id * 2 + 2));
                    lpciPlanT = (CYBERINFOTEMP *)(vlpbAiData + (dOffsetPlanTemp + lpplDst->id * 2));
                    iPacketAdd = 0;
                    if (lpplDst->isb == 1 || lpplDst->isb == 3 || lpplDst->isb == 6 || lpplDst->isb == 8) {
                        iMinLimit = 10;
                    } else {
                        iMinLimit = 1000;
                    }
                    if (lpplDst->rgwtMin[0] < iMinLimit && lppl->rgwtMin[0] > 700) {
                        iPacketAdd = iPacketMax >= 7 ? 7 : iPacketMax;
                        AddItemToQueue(iobjPacketIron, iPacketAdd, grobjPlanet, addItemFront);
                        iPacketMax -= iPacketAdd;
                        lpciPlanT->fNeedsMin1 = FALSE;
                        fWrite = TRUE;
                    }
                    if (lpplDst->rgwtMin[1] < iMinLimit && lppl->rgwtMin[1] > 700) {
                        iPacketAdd = iPacketMax >= 7 ? 7 : iPacketMax;
                        AddItemToQueue(iobjPacketBor, iPacketAdd, grobjPlanet, addItemFront);
                        iPacketMax -= iPacketAdd;
                        lpciPlanT->fNeedsMin2 = FALSE;
                        fWrite = TRUE;
                    }
                    if (lpplDst->rgwtMin[2] < iMinLimit && lppl->rgwtMin[2] > 700) {
                        iPacketAdd = iPacketMax >= 7 ? 7 : iPacketMax;
                        AddItemToQueue(iobjPacketGerm, iPacketAdd, grobjPlanet, addItemFront);
                        iPacketMax -= iPacketAdd;
                        lpciPlanT->fNeedsMin3 = FALSE;
                        fWrite = TRUE;
                    }
                    if (!fWrite)
                        goto LFinish;
                    iWarpSrc = IWarpMAFromLppl(lppl, &fTwoMA) - 4;
                    if (fTwoMA) {
                        iWarpSrc++;
                    }
                    iWarpDst = IWarpMAFromLppl(lpplDst, &fTwoMA) - 4;
                    if (fTwoMA) {
                        iWarpDst++;
                    }
                    sel.pl.iWarpFling = iWarpSrc >= iWarpDst ? iWarpDst : iWarpSrc;
                    sel.pl.idFling = lpplDst->id + 1;
                    FLookupPlanet(idWriteBack, &sel.pl);
                    goto LFinish;
                }
            }
            if (!lpciPlan->fNeedScanPkt && (rgplr[idPlayer].lvlAi > lvlAiStandard || (rgplr[idPlayer].lvlAi == lvlAiStandard && Random(3) == 0))) {
                plMinMax = (int32_t *)(vlpbAiData + (dOffsetPlanTemp + game.cPlanMax * 2));
                rgResAvail[0] -= rgResCost[0];
                rgResAvail[1] -= rgResCost[1];
                rgResAvail[2] -= rgResCost[2];
                lMineral = rgResAvail[0] - 70 + (rgResAvail[1] - 70) + (rgResAvail[2] - 70);
                cResLeft = LOWORD((int32_t)(rgResAvail[3] / 2));
                lPackets = (int16_t)((int16_t)(cResLeft - 5) / 5);
                *plMinMax = lMineral < (int32_t)(uint32_t)(lPackets * 70) ? lMineral : (uint32_t)(lPackets * 70);
                if (lMineral > 150) {
                    lpplDst = LpplFindClosestEnum(lppl, FEnumPktAttack);
                } else {
                    lpplDst = NULL;
                }
                if (lpplDst) {
                    cPacket[0] = 0;
                    cPacket[1] = 0;
                    cPacket[2] = 0;
                    lpciPlanDst = (CYBERINFO *)(vlpbAiData + (lpplDst->id * 2 + 2));
                    lpciPlanT = (CYBERINFOTEMP *)(vlpbAiData + (dOffsetPlanTemp + lpplDst->id * 2));
                    iPacketAdd = 0;
                    if (lpplDst->fStarbase) {
                        iWarpDst = IWarpMAFromLppl(lpplDst, &fTwoMA);
                        if (fTwoMA) {
                            iWarpDst++;
                        }
                    } else {
                        iWarpDst = 0;
                    }
                    iWarp = IWarpMAFromLppl(lppl, &fTwoMA) + 3;
                    dDistance = Sf64FromI32((int16_t)(iWarp * iWarp));
                    dDistanceTgt = DGetDistance(rgptPlan[lppl->id].x, rgptPlan[lppl->id].y, rgptPlan[lpplDst->id].x, rgptPlan[lpplDst->id].y);
                    dMod = Sf64From80(
                        (Sf80Div(Sf80FromU32((uint32_t)((iWarp * iWarp - iWarpDst * iWarpDst) * (100 - (lpplDst->uDefGuess + 5)))), Sf80From64(16000.0))));
                    lMinNeeded = Sf80ToI32((Sf80Div(
                        (1000 >= (uint16_t)((lpplDst->uPopGuess + 25) * 4) ? Sf80FromU32((uint32_t)((lpplDst->uPopGuess + 25) * 4)) : Sf80FromI32(1000)),
                        Sf80From64(dMod))));
                    lMineral = lMinNeeded < lMineral ? lMinNeeded : lMineral;
                    if (fTwoMA) {
                        lMineral = Sf80ToI32((Sf80Div(Sf80FromI32(lMineral),
                                                      Sf80From64(Sf64Pow(0.875, Sf64From80((Sf80Div(Sf80From64(dDistanceTgt), Sf80From64(dDistance)))))))));
                    } else {
                        lMineral = Sf80ToI32((
                            Sf80Div(Sf80FromI32(lMineral), Sf80From64(Sf64Pow(0.75, Sf64From80((Sf80Div(Sf80From64(dDistanceTgt), Sf80From64(dDistance)))))))));
                    }
                    for (; lMineral > 0; lMineral -= 70) {
                        iMin = 0;
                        if (rgResAvail[iMin] < rgResAvail[1]) {
                            iMin = 1;
                        }
                        if (rgResAvail[iMin] < rgResAvail[2]) {
                            iMin = 2;
                        }
                        cPacket[iMin]++;
                        rgResAvail[iMin] -= 70;
                    }
                    if (cPacket[0] > 0) {
                        AddItemToQueue(iobjPacketIron, cPacket[0], grobjPlanet, addItemFront);
                    }
                    if (cPacket[1] > 0) {
                        AddItemToQueue(iobjPacketBor, cPacket[1], grobjPlanet, addItemFront);
                    }
                    if (cPacket[2] > 0) {
                        AddItemToQueue(iobjPacketGerm, cPacket[2], grobjPlanet, addItemFront);
                    }
                    sel.pl.iWarpFling = iWarp - 4;
                    sel.pl.idFling = lpplDst->id + 1;
                    FLookupPlanet(idWriteBack, &sel.pl);
                    lpciPlanDst->iPktTarget = 3;
                    if (Sf80Lt(Sf80From64(dDistance), Sf80From64(dDistanceTgt))) {
                        lpciPlan->fNeedScanPkt = TRUE;
                    }
                    lpciPlan->fLaunchedPkt = FALSE;
                    fWrite = TRUE;
                    goto LFinish;
                }
            }
            if (!lpciPlan->fLaunchedPkt || lpciPlan->fNeedScanPkt) {
                if (!lpciPlan->fNeedScanPkt) {
                    i = Random(7);
                    if (i == lpciPlan->iLstPktDir) {
                        lpciPlan->iLstPktDir = i + 1;
                    } else {
                        lpciPlan->iLstPktDir = i;
                    }
                    idPlanDst = IdGetBestScannerDest(lppl, lpciPlan->iLstPktDir);
                }
                if (lpciPlan->fNeedScanPkt || idPlanDst != idplNone) {
                    lpciPlanDst = (CYBERINFO *)(vlpbAiData + (idPlanDst * 2 + 2));
                    if (lpciPlanDst->iPktTarget == 0 && FAddPacketToQueue(lppl)) {
                        lpciPlanDst->iPktTarget = 3;
                        lpciPlan->fLaunchedPkt = lpciPlan->fNeedScanPkt == 0;
                        fWrite = TRUE;
                    } else {
                        lpciPlan->fLaunchedPkt = FALSE;
                    }
                    iWarp = IWarpMAFromLppl(lppl, &fTwoMA) - 1;
                    sel.pl.iWarpFling = iWarp;
                    if (!lpciPlan->fNeedScanPkt) {
                        sel.pl.idFling = idPlanDst;
                    } else {
                        lpciPlan->fNeedScanPkt = FALSE;
                    }
                    FLookupPlanet(idWriteBack, &sel.pl);
                }
            } else {
                lpciPlan->fLaunchedPkt = FALSE;
            }
        LFinish:
            FinishProduction(fWrite);
        }
    }
    return;
}

int16_t IdGetBestScannerDest(PLANET *lppl, CompassDir iDir) {
    int16_t iDistance;
    int16_t iWarp;
    PLANET *lpplDst;
    int16_t dAdjust;
    int16_t iSize;
    POINT16 ptEdge;
    SCAN    scan;

    ptEdge = rgptPlan[lppl->id];
    iSize = 400 * game.mdSize + 400;
    iWarp = IWarpMAFromLppl(lppl, NULL) + 3;
    iDistance = iWarp * iWarp;
    ptEdge.x -= 1000;
    ptEdge.y -= 1000;
    if (iDir > dirSouthEast) {
        return idplNone;
    }
    switch (iDir) {
    case dirEast:
        ptEdge.x = iSize;
        break;
    case dirNorthEast:
        if (iSize - ptEdge.x > ptEdge.y) {
            ptEdge.x += ptEdge.y;
            ptEdge.y = 0;
            break;
        }
        ptEdge.y -= iSize - ptEdge.x;
        ptEdge.x = iSize;
        break;
    case dirNorth:
        ptEdge.y = 0;
        break;
    case dirNorthWest:
        if (ptEdge.x > ptEdge.y) {
            ptEdge.x -= ptEdge.y;
            ptEdge.y = 0;
            break;
        }
        ptEdge.y -= ptEdge.x;
        ptEdge.x = 0;
        break;
    case dirWest:
        ptEdge.x = 0;
        break;
    case dirSouthWest:
        if (iSize - ptEdge.x > ptEdge.y) {
            ptEdge.y += ptEdge.x;
            ptEdge.x = 0;
            break;
        }
        ptEdge.x -= iSize - ptEdge.y;
        ptEdge.y = iSize;
        break;
    case dirSouth:
        ptEdge.y = iSize;
        break;
    case dirSouthEast:
        if (ptEdge.x > ptEdge.y) {
            ptEdge.y += iSize - ptEdge.x;
            ptEdge.x = iSize;
        } else {
            ptEdge.x += iSize - ptEdge.y;
            ptEdge.y = iSize;
        }
    }
    dAdjust = Random(Sf80ToI32((Sf80Mul(Sf80FromI32(iSize), Sf80From64(0.3)))));
    dAdjust -= LOWORD(Sf80ToI32((Sf80Mul(Sf80FromI32(iSize), Sf80From64(0.15)))));
    if (ptEdge.x == 0 || ptEdge.x == iSize) {
        ptEdge.y += dAdjust;
        if (ptEdge.y > iSize) {
            dAdjust = ptEdge.y - iSize;
            ptEdge.y = iSize;
        } else if (ptEdge.y < 0) {
            dAdjust = 0 - ptEdge.y;
            ptEdge.y = 0;
        } else {
            dAdjust = 0;
        }
        if (ptEdge.x == 0) {
            ptEdge.x += dAdjust;
        } else {
            ptEdge.x -= dAdjust;
        }
    } else {
        ptEdge.x += dAdjust;
        if (ptEdge.x > iSize) {
            dAdjust = ptEdge.x - iSize;
            ptEdge.x = iSize;
        } else if (ptEdge.x < 0) {
            dAdjust = 0 - ptEdge.x;
            ptEdge.x = 0;
        } else {
            dAdjust = 0;
        }
        if (ptEdge.y == 0) {
            ptEdge.y += dAdjust;
        } else {
            ptEdge.y -= dAdjust;
        }
    }
    dAdjust = Random(iDistance);
    if (ptEdge.y == 0) {
        ptEdge.y += dAdjust;
    } else if (ptEdge.y == iSize) {
        ptEdge.y -= dAdjust;
    } else if (ptEdge.x == 0) {
        ptEdge.x += dAdjust;
    } else if (ptEdge.x == iSize) {
        ptEdge.x -= dAdjust;
    }
    ptEdge.y += 1000;
    ptEdge.x += 1000;
    if (FFindNearestObject(ptEdge, grobjPlanet | mdNoRecurse, &scan)) {
        lpplDst = LpplFromId(scan.idpl);
        if ((lpplDst && lpplDst->iPlayer == idPlayer) || (int32_t)LDistance2(rgptPlan[scan.idpl], rgptPlan[lppl->id]) < (int16_t)(iDistance * iDistance)) {
            return idplNone;
        }
        return scan.idpl + 1;
    }
    return idplNone;
}

int16_t FAddPacketToQueue(PLANET *lppl) {
    int32_t rgResCost[4];
    int16_t iMineral;
    int32_t rgResAvail[4];

    iMineral = 0;
    GetResourcesAvailable(lppl, rgResAvail);
    GetProdQCost(lppl, rgResCost);
    if (rgResAvail[iMineral] - rgResCost[iMineral] < rgResAvail[1] - rgResCost[1]) {
        iMineral = 1;
    }
    if (rgResAvail[iMineral] - rgResCost[iMineral] < rgResAvail[2] - rgResCost[2]) {
        iMineral = 2;
    }
    if (rgResAvail[iMineral] - rgResCost[iMineral] >= 170) {
        AddItemToQueue(iMineral + 14, 1, grobjPlanet, addItemFront);
        return TRUE;
    }
    return FALSE;
}

void FillProductionQueue() {
    PLANET *lppl;
    int16_t ipl;
    PROD    rgprod[64];

    for (ipl = 0; ipl < vclpplAi; ipl++) {
        lppl = vrglpplAi[ipl];
        if (!vrglpplAi[ipl])
            break;
        ChangeMainObjSel(grobjPlanet, lppl->id);
        InitProduction(rgprod);
        FinishProduction(FFillProdMinesAndFactories(lppl));
    }
    return;
}

int16_t FFillProdMinesAndFactories(PLANET *lppl) {
    int32_t rgResCost[4];
    int32_t rgResAvail[4];
    int16_t iAddFactories;
    int16_t iAddMines;
    PROD    prod;
    int32_t rgAlchCost[4];
    int32_t rgMineCost[4];
    int32_t rgFactCost[4];
    int32_t rgResLeft[4];
    int16_t iAddAlchemy;
    int16_t iMaxTerra;
    int16_t i;
    int16_t iMaxFactories;
    int16_t iMaxFactBuildable;
    int16_t iMaxMines;
    int16_t fInsert;
    PROD   *lpprod;
    int16_t cAdd;

    fInsert = FALSE;
    GetResourcesAvailable(lppl, rgResAvail);
    GetProdQCost(lppl, rgResCost);
    for (i = 0; i < 4; i++) {
        rgResLeft[i] = rgResAvail[i] - rgResCost[i];
    }
    if (rgResLeft[3] < 0) {
        return FALSE;
    }
    iMaxTerra = 0;
    iMaxMines = 0;
    iAddMines = 0;
    iMaxFactories = 0;
    iAddFactories = 0;
    iAddAlchemy = 0;
    rgMineCost[3] = 0;
    rgFactCost[3] = 0;
    rgAlchCost[3] = 0;
    i = 0;
    lpprod = lpplProdGlob->rgprod;
    while (i < lpplProdGlob->iprodMac) {
        if (lpprod->grobj == grobjPlanet && lpprod->iItem == mdIdleMine) {
            iAddMines += lpprod->cItem;
        }
        if (lpprod->grobj == grobjPlanet && lpprod->iItem == mdIdleFactory) {
            iAddFactories += lpprod->cItem;
        }
        i++;
        lpprod++;
    }
    for (i = cProdGlob - 1; i >= 0; i--) {
        if (pProdGlob[i].grobj == grobjPlanet) {
            if (pProdGlob[i].iItem == mdIdleMine) {
                iMaxMines = max(0, CMaxOperableMines(lppl, idPlayer, FALSE) - CMinesOperating(lppl) - iAddMines);
                GetProductionCosts(lppl, pProdGlob + i, rgMineCost, idPlayer, TRUE);
            }
            if (pProdGlob[i].iItem == mdIdleFactory) {
                iMaxFactories = max(0, CMaxOperableFactories(lppl, idPlayer, FALSE) - CFactoriesOperating(lppl) - iAddFactories);
                GetProductionCosts(lppl, pProdGlob + i, rgFactCost, idPlayer, TRUE);
            }
            if (pProdGlob[i].iItem == mdIdleAlchemy) {
                GetProductionCosts(lppl, pProdGlob + i, rgAlchCost, idPlayer, TRUE);
            }
            if (pProdGlob[i].iItem == mdIdleTerraform) {
                iMaxTerra = pProdGlob[i].cItem;
            }
        }
    }
    if (iMaxTerra > 0 && rgplr[idPlayer].idAi == idAiMacinti) {
        cAdd = LOWORD((int32_t)((rgResLeft[3] + 69) / 70));
        if (cAdd > iMaxTerra) {
            cAdd = iMaxTerra;
        }
        if (cAdd > 0) {
            AddItemToQueue(mdIdleTerraform, cAdd, grobjPlanet, addItemFront);
            return TRUE;
        }
    }
    if (rgResLeft[0] <= 0 || rgResLeft[1] <= 0 || rgResLeft[2] <= 0) {
        if (lppl->lpplprod) {
            prod = lppl->lpplprod->rgprod[0];
            if (prod.grobj == grobjPlanet && (prod.iItem == iobjAlchemy || prod.iItem == mdIdleAlchemy)) {
                return FALSE;
            }
        }
        if (iMaxMines > 0) {
            iAddMines = min(iMaxMines, (int16_t)(rgResLeft[3] / rgMineCost[3]));
        } else {
            iAddMines = 0;
        }
        rgResLeft[3] -= (uint32_t)(iAddMines * rgMineCost[3]);
        iAddFactories = 0;
        iAddAlchemy = 0;
        fInsert = TRUE;
        goto LAdd;
    }
    iMaxFactBuildable = iMaxFactories;
    if (iMaxFactories > 0) {
        if (!gd.fTutorial) {
            iMaxFactories = min(iMaxFactories, (int16_t)(rgResLeft[2] / rgFactCost[2]));
        } else {
            iMaxFactories = min(min(min(iMaxFactories, (int16_t)(rgResLeft[0] / rgFactCost[0])), (int16_t)(rgResLeft[1] / rgFactCost[1])),
                                (int16_t)(rgResLeft[2] / rgFactCost[2]));
        }
    }
    if (iMaxFactories > 0) {
        iAddFactories = min(iMaxFactories, (int16_t)(rgResLeft[3] / rgFactCost[3]));
    } else {
        iAddFactories = 0;
    }
    rgResLeft[3] -= (uint32_t)(iAddFactories * rgFactCost[3]);
    if (iMaxMines > 0) {
        iAddMines = min(iMaxMines, (int16_t)(rgResLeft[3] / rgMineCost[3]));
    } else {
        iAddMines = 0;
    }
    rgResLeft[3] -= (uint32_t)(iAddMines * rgMineCost[3]);
LAdd:
    if (rgAlchCost[3] > 0 && rgplr[idPlayer].rgTech[3] == 26 && rgplr[idPlayer].rgTech[5] == 26 && rgplr[idPlayer].rgTech[0] == 26 &&
        rgplr[idPlayer].rgTech[1] == 26 && rgplr[idPlayer].rgTech[4] == 26 && rgplr[idPlayer].rgTech[2] == 26) {
        iAddAlchemy = 0 <= LOWORD((int32_t)(rgResLeft[3] / rgAlchCost[3])) + 1 ? LOWORD((int32_t)(rgResLeft[3] / rgAlchCost[3])) + 1 : 0;
    } else {
        iAddAlchemy = 0;
    }
    rgResLeft[3] -= (uint32_t)(iAddAlchemy * rgAlchCost[3]);
    if (iAddFactories > 0) {
        AddItemToQueue(mdIdleFactory, iAddFactories, grobjPlanet, addItemEnd);
    }
    if (iAddMines > 0) {
        AddItemToQueue(mdIdleMine, iAddMines, grobjPlanet, addItemFront);
    }
    if (iAddAlchemy > 0 && game.turn > 100) {
        AddItemToQueue(mdIdleAlchemy, iAddAlchemy, grobjPlanet, !fInsert ? addItemEnd : addItemFront);
    }
    if (iAddMines + iAddFactories + iAddAlchemy > 0) {
        return TRUE;
    }
    return FALSE;
}

void DoCyberFreighter(FLEET *lpfl, CYBERINFOTEMP *lpciPlanTemp) {
    ORDER   ord;
    PLANET *lpplDst;
    PLANET *lpplCur;
    int16_t fDropOff;
    int16_t idPlanDst;
    SCAN    scan;

    fDropOff = FALSE;
    lpplCur = LpplFromId(lpfl->idPlanet);
    if (!lpplCur) {
        if (FFindNearestObject(lpfl->pt, grobjPlanet | mdNoRecurse, &scan)) {
            lpplDst = LpplFromId(scan.idpl);
        } else {
            lpplDst = NULL;
        }
        goto LTarget;
    }
    if (lpplCur->iPlayer == idPlayer) {
        if (lpplCur->rgwtMin[3] > 2000) {
            XferAiSupply(grobjPlanet, lpfl->idPlanet, grobjFleet, lpfl->id, Colonists, 1000);
            FLookupFleet(lpfl->id, &sel.fl);
            fDropOff = TRUE;
        } else {
            XferAiSupply(grobjPlanet, lpfl->idPlanet, grobjFleet, lpfl->id, Colonists, -1000);
            FLookupFleet(lpfl->id, &sel.fl);
        }
    } else if (lpplCur->iPlayer != iplrNone && GetRaceStat(&rgplr[lpplCur->iPlayer], rsMajorAdv) != raMacintosh && !lpplCur->fStarbase) {
        FLookupFleet(lpfl->id, &sel.fl);
        idPlanDst = lpplCur->id;
        memset(&ord, 0, sizeof(ORDER));
        ord.pt = rgptPlan[idPlanDst];
        ord.grobj = grobjPlanet;
        ord.id = idPlanDst;
        ord.grTask = grTaskXfer;
        ord.fValidTask = TRUE;
        ord.txp.rgia[3].iAction = iActionUnloadAll;
        ChangeMainObjSel(grobjFleet, lpfl->id);
        if (sel.fl.lpplord->rgord[0].id == idPlanDst && sel.fl.lpplord->rgord[0].grobj == grobjPlanet) {
            sel.fl.lpplord->rgord[0] = ord;
        } else {
            sel.fl.lpplord->rgord[1] = ord;
        }
        FLookupFleet(idWriteBack, &sel.fl);
        FMoveToNearestStarbase(lpfl, FALSE);
        FLookupFleet(lpfl->id, &sel.fl);
    } else {
        fDropOff = lpfl->rgwtMin[3] > 0;
    }
    if (fDropOff) {
        lpplDst = LpplFindClosestEnum(lpplCur, FEnumDropOffStage1);
        if (!lpplDst) {
            lpplDst = LpplFindClosestEnum(lpplCur, FEnumDropOffStage2);
        }
        if (!lpplDst) {
            if (lpplCur->iPlayer == idPlayer) {
                XferAiSupply(grobjPlanet, lpfl->idPlanet, grobjFleet, lpfl->id, Colonists, -1000);
                FLookupFleet(lpfl->id, &sel.fl);
                if (lpciPlanTemp[lpplCur->id].cIdleFreighters < 3) {
                    lpciPlanTemp[lpplCur->id].cIdleFreighters = lpciPlanTemp[lpplCur->id].cIdleFreighters + 1;
                }
            }
        } else {
            if (lpciPlanTemp[lpplDst->id].cFreightersDst < 3) {
                lpciPlanTemp[lpplDst->id].cFreightersDst = lpciPlanTemp[lpplDst->id].cFreightersDst + 1;
            }
        }
    } else {
        lpplDst = LpplFindClosestEnum(lpplCur, FEnumPickUp);
    }
LTarget:
    if (lpplDst) {
        memset(&ord, 0, sizeof(ORDER));
        ord.pt = rgptPlan[lpplDst->id];
        ord.grobj = grobjPlanet;
        ord.id = lpplDst->id;
        ord.grTask = grTaskNone;
        ord.fValidTask = TRUE;
        ord.iWarp = (uint16_t)IFindIdealWarp(lpfl, TRUE);
        if (FMoveAiFleet(lpfl, &ord, FALSE)) {
            lpfl->fMark = TRUE;
        }
    }
    return;
}

int16_t FEnumDropOffStage1(PLANET *lpplSrc, PLANET *lpplTest) {
    CYBERINFOTEMP *lpciPlanTemp;
    int16_t        dOffsetPlanTemp;

    dOffsetPlanTemp = game.cPlanMax * 2 + 2;
    lpciPlanTemp = (CYBERINFOTEMP *)(vlpbAiData + dOffsetPlanTemp);
    if (lpciPlanTemp[lpplTest->id].cFreightersDst == 3) {
        return FALSE;
    }
    if (lpplTest->iPlayer == idPlayer && lpplTest->rgwtMin[3] < 200 && lpciPlanTemp[lpplTest->id].cFreightersDst == 0) {
        if (LDistance2(rgptPlan[lpplSrc->id], rgptPlan[lpplTest->id]) <= 160000) {
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

int16_t FEnumDropOffStage2(PLANET *lpplSrc, PLANET *lpplTest) {
    CYBERINFOTEMP *lpciPlanTemp;
    int16_t        dOffsetPlanTemp;

    dOffsetPlanTemp = game.cPlanMax * 2 + 2;
    lpciPlanTemp = (CYBERINFOTEMP *)(vlpbAiData + dOffsetPlanTemp);
    if (lpciPlanTemp[lpplTest->id].cFreightersDst == 3) {
        return FALSE;
    }
    if (lpplTest->iPlayer != idPlayer || (uint32_t)(lpciPlanTemp[lpplTest->id].cFreightersDst * 210) + lpplTest->rgwtMin[3] >= 1000) {
        return FALSE;
    }
    if (LDistance2(rgptPlan[lpplSrc->id], rgptPlan[lpplTest->id]) <= 160000) {
        return TRUE;
    }
    return FALSE;
}

int16_t FEnumPickUp(PLANET *lpplSrc, PLANET *lpplTest) {
    if (lpplTest->iPlayer == idPlayer && lpplTest->rgwtMin[3] > 2200 && lpplTest->fStarbase) {
        return TRUE;
    }
    return FALSE;
}

int16_t FEnumNeedMinerals(PLANET *lpplSrc, PLANET *lpplTest) {
    double         dDistance;
    int16_t        iWarpSrc;
    int16_t        fTwoMA;
    int16_t        iMinLimit;
    CYBERINFO     *lpciPlan;
    int16_t        iWarpDst;
    CYBERINFOTEMP *lpciPlanTemp;
    int16_t        dOffsetPlanTemp;

    dOffsetPlanTemp = game.cPlanMax * 2 + 2;
    lpciPlan = (CYBERINFO *)(vlpbAiData + 2);
    lpciPlanTemp = (CYBERINFOTEMP *)(vlpbAiData + dOffsetPlanTemp);
    switch (lpplTest->isb) {
    case 1:
    case 3:
    case 6:
    case 8:
        iMinLimit = 10;
        break;
    default:
        iMinLimit = 1000;
    }
    if (lpplTest->iPlayer != idPlayer || !lpplTest->fStarbase || lpciPlan[lpplTest->id].iPktTarget > 0) {
        return FALSE;
    }
    if ((lpplSrc->rgwtMin[0] > 700 && lpciPlanTemp[lpplTest->id].fNeedsMin1) || (lpplSrc->rgwtMin[1] > 700 && lpciPlanTemp[lpplTest->id].fNeedsMin2) ||
        (lpplSrc->rgwtMin[2] > 700 && lpciPlanTemp[lpplTest->id].fNeedsMin3)) {
        iWarpSrc = IWarpMAFromLppl(lpplSrc, &fTwoMA);
        if (fTwoMA) {
            iWarpSrc++;
        }
        iWarpDst = IWarpMAFromLppl(lpplTest, &fTwoMA);
        if (fTwoMA) {
            iWarpDst++;
        }
        dDistance = Sf64From80((iWarpSrc >= iWarpDst ? Sf80FromI32(iWarpDst) : Sf80FromI32(iWarpSrc)));
        dDistance = Sf64From80((Sf80Mul(Sf80Mul(Sf80From64(dDistance), Sf80From64(dDistance)), Sf80From64(3.5))));
        if (Sf80Le(Sf80FromI32(LDistance2(rgptPlan[lpplSrc->id], rgptPlan[lpplTest->id])), Sf80Mul(Sf80From64(dDistance), Sf80From64(dDistance)))) {
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

int16_t FEnumPktAttack(PLANET *lpplSrc, PLANET *lpplTest) {
    double     dDistance;
    int16_t    fTwoMA;
    double     dMod;
    int16_t    iWarp;
    int32_t   *plMinMax;
    int32_t    lMineral;
    int32_t    lMinNeeded;
    CYBERINFO *lpciPlan;
    int16_t    iWarpDst;
    int16_t    dOffsetPlanTemp;
    double     dDistanceTgt;

    lMineral = 0;
    iWarpDst = 0;
    dOffsetPlanTemp = game.cPlanMax * 2 + 2;
    lpciPlan = (CYBERINFO *)(vlpbAiData + 2);
    plMinMax = (int32_t *)(vlpbAiData + (dOffsetPlanTemp + game.cPlanMax * 2));
    if (lpplTest->iPlayer == idPlayer || lpplTest->iPlayer == iplrNone || lpciPlan[lpplTest->id].iPktTarget > 0 ||
        GetRaceStat(&rgplr[lpplTest->iPlayer], rsMajorAdv) == raMacintosh) {
        return FALSE;
    }
    if (lpplTest->fStarbase) {
        if (rglpshdefSB[lpplTest->iPlayer][lpplTest->isb].det != detAll) {
            return FALSE;
        }
        iWarpDst = IWarpMAFromLppl(lpplTest, &fTwoMA);
        if (fTwoMA) {
            iWarpDst++;
        }
    }
    dDistance = Sf64FromI32(iWarp = IWarpMAFromLppl(lpplSrc, &fTwoMA) + 3);
    if (iWarp == iWarpDst) {
        return FALSE;
    }
    dDistance = Sf64From80((Sf80Mul(Sf80From64(dDistance), Sf80From64(dDistance))));
    dDistanceTgt = DGetDistance(rgptPlan[lpplSrc->id].x, rgptPlan[lpplSrc->id].y, rgptPlan[lpplTest->id].x, rgptPlan[lpplTest->id].y);
    if (Sf80Le(Sf80From64(dDistanceTgt), Sf80Mul(Sf80From64(dDistance), Sf80From64(2.5)))) {
        if (fTwoMA) {
            lMineral = Sf80ToI32(
                (Sf80Mul(Sf80FromI32(*plMinMax), Sf80From64(Sf64Pow(0.875, Sf64From80((Sf80Div(Sf80From64(dDistanceTgt), Sf80From64(dDistance)))))))));
        } else {
            lMineral =
                Sf80ToI32((Sf80Mul(Sf80FromI32(*plMinMax), Sf80From64(Sf64Pow(0.75, Sf64From80((Sf80Div(Sf80From64(dDistanceTgt), Sf80From64(dDistance)))))))));
        }
        if (lpplTest->uDefGuess > 99 || lpplTest->uPopGuess == 0) {
            return FALSE;
        }
        dMod = Sf64From80((Sf80Div(Sf80FromU32((uint32_t)((iWarp * iWarp - iWarpDst * iWarpDst) * (100 - (lpplTest->uDefGuess + 5)))), Sf80From64(16000.0))));
        lMinNeeded = Sf80ToI32(
            (Sf80Div((1000 >= (uint16_t)((lpplTest->uPopGuess + 25) * 4) ? Sf80FromU32((uint32_t)((lpplTest->uPopGuess + 25) * 4)) : Sf80FromI32(1000)),
                     Sf80From64(dMod))));
        if (lMinNeeded <= lMineral) {
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

int16_t FEnumCalcEnemyPlanets(PLANET *lpplSrc, PLANET *lpplTest) {
    if (lpplTest->iPlayer != idPlayer && lpplTest->iPlayer != iplrNone) {
        return TRUE;
    }
    return FALSE;
}

int16_t iBuildCyberStarbase(PLANET *lppl) {
    int16_t ishdefSB;
    PROD    rgprod[64];

    if (lppl->fStarbase || PctPlanetDesirability(lppl, idPlayer) < 15 || lppl->rgwtMin[3] < 500) {
        return -1;
    }
    ChangeMainObjSel(grobjPlanet, lppl->id);
    InitProduction(rgprod);
    if (lppl->rgMinConc[0] > 15 && lppl->rgMinConc[1] > 15 && lppl->rgMinConc[2] > 15) {
        ishdefSB = IshdefAiSBLatest();
    } else {
        ishdefSB = IshdefAiSBLatestOF();
    }
    return ishdefSB;
}

void EnsureCyberAiShdefs(int16_t iroCur) {
    int16_t low;
    int16_t ish;
    int16_t ishCur;
    int16_t i;
    int16_t high;
    SHDEF   shdef;

    if (!rgshdef[0].fFree && rgshdef[0].hul.ihuldef != ihuldefFrigate && rgplr[idPlayer].lvlAi > lvlAiStandard && rgshdef[0].cExist == 0 && game.turn > 5) {
        shdef = rgshdef[0];
        shdef.fFree = TRUE;
        FChangeAiShdef(&shdef, 0);
    }
    if (rgshdef[0].fFree) {
        FCreateAiShdef(0, ihuldefFrigate, (uint8_t *)&vrgCyberAip[cyberRecipeFrigateScoutMineLayer]);
    }
    if (rgshdef[4].fFree && game.turn > 30 &&
        (game.turn > 75 || !FCreateAiShdef(4, ihuldefDestroyer, (uint8_t *)&vrgCyberAip[cyberRecipeDestroyerBeamThrust]))) {
        for (i = 5; i > 0 && FCreateAiShdef(4, ihuldefDestroyer, (uint8_t *)&vrgCyberAip[vrgCyberIshAip[icyberRecipeDestroyerBeamThrust + Random(i)]]) == 0;
             i--) {
        }
    }
    if (rgshdef[5].fFree && !rgshdef[4].fFree && game.turn > (uint16_t)(rgshdef[4].turn + 20) &&
        (game.turn > 75 || !FCreateAiShdef(5, ihuldefDestroyer, (uint8_t *)&vrgCyberAip[cyberRecipeDestroyerTorpedoThrust]))) {
        for (i = 5; i > 0 && FCreateAiShdef(5, ihuldefDestroyer, (uint8_t *)&vrgCyberAip[vrgCyberIshAip[icyberRecipeDestroyerTorpedoThrust + Random(i)]]) == 0;
             i--) {
        }
    }
    if (rgshdef[2].fFree && game.turn > 20) {
        FCreateAiShdef(2, ihuldefPrivateer, (uint8_t *)&vrgCyberAip[cyberRecipePrivateerBeam]);
    }
    if (rgshdef[3].fFree && !rgshdef[2].fFree && game.turn > (uint16_t)(rgshdef[2].turn + 20)) {
        FCreateAiShdef(3, ihuldefPrivateer, (uint8_t *)&vrgCyberAip[cyberRecipePrivateerTorpedo]);
    }
    for (ish = 6; ish <= 10; ish += 4) {
        if (rgshdef[ish].fFree && ((ish == 6 && game.turn > 40) || (!rgshdef[6].fFree && game.turn > (uint16_t)(rgshdef[6].turn + 30)))) {
            ishCur = ish + 2;
            for (i = 3; i > 0; i--) {
                if (FCreateAiShdef(ishCur, ihuldefNubian, (uint8_t *)&vrgCyberAip[vrgCyberIshAip[icyberRecipeNubianMissileBeam + Random(i)]])) {
                    ishCur--;
                    break;
                }
            }
            for (i = 4; i > 0; i--) {
                if (FCreateAiShdef(ishCur, ihuldefBattleship,
                                   (uint8_t *)&vrgCyberAip[vrgCyberIshAip[icyberRecipeBattleshipMissileTorpedoThrust + Random(i)]])) {
                    ishCur--;
                    break;
                }
            }
            for (i = 3; i > 0; i--) {
                if (FCreateAiShdef(ishCur, ihuldefBattleship, (uint8_t *)&vrgCyberAip[vrgCyberIshAip[icyberRecipeBattleshipMixedBeamsThrust + Random(i)]])) {
                    ishCur--;
                    break;
                }
            }
            if (ishCur < ish)
                goto LBomber;
            high = 9 - (ishCur - ish) * 3;
            low = icyberRecipeBattleshipMixedBeamsThrust - high;
        LCruiser:
            for (i = high; i > 0; i--) {
                if (FCreateAiShdef(ishCur, ihuldefCruiser, (uint8_t *)&vrgCyberAip[vrgCyberIshAip[Random(i) + low]]))
                    break;
            }
            ishCur--;
            if (ishCur < ish)
                goto LBomber;
            high = 3;
            low = ishCur == ish ? icyberRecipeCruiserStreamingBeams : icyberRecipeCruiserTorpedo;
            goto LCruiser;
        LBomber:
            if (!FCreateAiShdef(ish + 3, ihuldefNubian, (uint8_t *)&vrgCyberAip[cyberRecipeNubianMcm]) &&
                !FCreateAiShdef(ish + 3, ihuldefBattleship, (uint8_t *)&vrgCyberAip[cyberRecipeBattleshipMcm]) &&
                !FCreateAiShdef(ish + 3, ihuldefB52Bomber, (uint8_t *)&vrgCyberAip[cyberRecipeB52RetroThenSmart])) {
                FCreateAiShdef(ish + 3, ihuldefB52Bomber, (uint8_t *)&vrgCyberAip[cyberRecipeB52SmartThenNormal]);
            }
        }
    }
    for (ish = 14; ish <= 15; ish++) {
        if (rgshdef[ish].fFree && ((ish == 14 && game.turn > 30) || (!rgshdef[14].fFree && game.turn > (uint16_t)(rgshdef[14].turn + 20)))) {
            for (i = 7; i > 0 && FCreateAiShdef(ish, ihuldefBattleship,
                                                (uint8_t *)&vrgCyberAip[vrgCyberIshAip[icyberRecipeBattleshipMixedBeamsThrust + Random(i)]]) == 0;
                 i--) {
            }
            if (i == 0) {
                for (i = 9;
                     i > 0 && FCreateAiShdef(ish, ihuldefCruiser, (uint8_t *)&vrgCyberAip[vrgCyberIshAip[icyberRecipeCruiserStreamingBeams + Random(i)]]) == 0;
                     i--) {
                }
            }
            if (i == 0) {
                for (i = 10;
                     i > 0 && FCreateAiShdef(ish, ihuldefDestroyer, (uint8_t *)&vrgCyberAip[vrgCyberIshAip[icyberRecipeDestroyerBeamThrust + Random(i)]]) == 0;
                     i--) {
                }
            }
        }
    }
    return;
}

int16_t iAddAttackFleet(PLANET *lppl, int16_t iAttackStr, int16_t iBestDestroyer, int16_t iBestBattle, int16_t iBestSBDefender) {
    int16_t fRet;
    int16_t iMaxFactories;
    int16_t iMaxMines;
    int16_t iRand;

    fRet = FALSE;
    iRand = Random(100);
    iMaxMines = max(0, CMaxOperableMines(lppl, idPlayer, FALSE) - CMinesOperating(lppl));
    iMaxFactories = max(0, CMaxOperableFactories(lppl, idPlayer, FALSE) - CFactoriesOperating(lppl));
    if (Random(100) < (iMaxMines < 100 || iMaxFactories < 100 ? 90 : 60)) {
        return 0;
    }
    if (iBestBattle != ishdefNone && iRand > 50) {
        if (!rgshdef[iBestBattle * 4 + 6].fFree) {
            AddItemToQueue(iBestBattle * 4 + 6, 2, grobjFleet, addItemEnd);
        }
        if (!rgshdef[iBestBattle * 4 + 7].fFree) {
            AddItemToQueue(iBestBattle * 4 + 7, 2, grobjFleet, addItemEnd);
        }
        if (!rgshdef[iBestBattle * 4 + 8].fFree && Random(100) < 75) {
            AddItemToQueue(iBestBattle * 4 + 8, 1, grobjFleet, addItemEnd);
        }
        if (!rgshdef[iBestBattle * 4 + 9].fFree && Random(100) < 50) {
            AddItemToQueue(iBestBattle * 4 + 9, 1, grobjFleet, addItemEnd);
        }
        return 1;
    }
    if (iBestSBDefender != ishdefNone && iRand > 25) {
        AddItemToQueue(iBestSBDefender, 1, grobjFleet, addItemEnd);
        return 2;
    }
    if (iBestDestroyer != ishdefNone) {
        AddItemToQueue(iBestDestroyer, 1, grobjFleet, addItemEnd);
        return 3;
    }
    return 0;
}

void TargetCyberArmada(FLEET *lpfl) {
    FLEET  *lpflTarget;
    ORDER   ord;
    PLANET *lppl;
    int16_t cshBomb;
    int16_t cshWar;
    PLANET *lpplTarget;

    if (lpfl->cord > 1) {
        ord = lpfl->lpplord->rgord[1];
        if (LDistance2(lpfl->pt, ord.pt) > 62500 && ord.grobj == grobjFleet)
            goto LTryNewTarget;
        if (ord.grobj == grobjFleet) {
            return;
        }
        if (ord.grobj == grobjPlanet) {
            lppl = LpplFromId(ord.id);
            if (!lppl || ((lppl->iPlayer != iplrNone && (lppl->iPlayer != idPlayer || lppl->fStarbase)) || lppl->turn != game.turn)) {
                return;
            }
        }
    }
LTryNewTarget:
    cshWar = 0;
    cshWar += lpfl->rgcsh[6];
    cshWar += lpfl->rgcsh[7];
    cshWar += lpfl->rgcsh[8] * 2;
    cshWar += lpfl->rgcsh[10];
    cshWar += lpfl->rgcsh[11];
    cshWar += lpfl->rgcsh[12] * 2;
    cshBomb = lpfl->rgcsh[9] + lpfl->rgcsh[13];
    lpfl->fMark = TRUE;
    ChangeMainObjSel(grobjFleet, lpfl->id);
    if (lpfl->idPlanet == idPlanetDeepSpace) {
        MoveToNearestPlanetOrEnemy(lpfl, 450);
        return;
    }
    lppl = LpplFromId(lpfl->idPlanet);
    if (lppl->iPlayer == idPlayer) {
        if (cshWar >= vrgAiArmadaPotency[0] && cshBomb >= vrgAiArmadaPotency[2]) {
        TargetPotentArmada:
            if (game.fAisBand) {
                lpplTarget = LpplFindBestEnum(lppl, FEnumCalcArmadaHumanDest);
            } else {
                lpplTarget = NULL;
            }
            if (!lpplTarget) {
                lpplTarget = LpplFindBestEnum(lppl, FEnumCalcArmadaDest);
            }
        TargetEveryArmada:
            if (lpplTarget) {
                vlpbAiPlanet[lpplTarget->id * 16 + 10] = vlpbAiPlanet[lpplTarget->id * 16 + 0xa] | 0x80;
                ord.id = lpplTarget->id;
                ord.grobj = grobjPlanet;
                ord.pt = rgptPlan[lpplTarget->id];
            FinishTargeting:
                ord.grTask = grTaskNone;
                ord.fValidTask = TRUE;
                ord.iWarp = 4;
                if (!FMoveAiFleet(lpfl, &ord, FALSE))
                    return;
            } else {
                lpflTarget = LpflFindClosestEnum(lpfl, FEnumCalcEnemyFleets);
                if (lpflTarget) {
                    ord.id = lpflTarget->id;
                    ord.grobj = grobjFleet;
                    ord.pt = lpflTarget->pt;
                    goto FinishTargeting;
                }
            }
        } else if (rgplr[idPlayer].lvlAi > lvlAiStandard && (cshWar > vrgAiArmadaPotency[0] * 2 || cshWar >= 60)) {
            if (Random(10) < 5 || (cshWar > vrgAiArmadaPotency[0] * 3 && Random(10) < 7) || (cshWar > 120 && Random(10) < 7))
                goto TargetPotentArmada;
        }
    } else if (cshWar < vrgAiArmadaPotency[1] || cshBomb < vrgAiArmadaPotency[3]) {
        ClearAiCurrentTask(lpfl, FALSE);
        if (rgplr[idPlayer].lvlAi > lvlAiStandard && ((cshWar > vrgAiArmadaPotency[0] * 2 && Random(10) < 5) ||
                                                      (cshWar > vrgAiArmadaPotency[0] * 4 && Random(10) < 7) || (cshWar > 120 && Random(10) < 7)))
            goto TargetPotentArmada;
        lpplTarget = LpplFindClosestEnum(lppl, FEnumOurStarbase);
        goto TargetEveryArmada;
    } else if (lppl->iPlayer == iplrNone) {
        goto TargetPotentArmada;
    }
}
