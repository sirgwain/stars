#include "common.h"

void Produce() {
    int32_t    lResCur;
    int16_t    cMax;
    int32_t    rgResAvail[4];
    int16_t    iprodCur;
    mdProdStat mdStatus;
    int16_t    cBuilt;
    int16_t    fNoResearch;
    PLANET    *lppl;
    int16_t    i;
    MessageId  idm;
    PROD       prodPartial;
    int16_t    fPrevProdIsAlch;
    int16_t    fAutoBuildDone;
    int32_t    lResearchTake;
    PROD      *lpprod;
    PLANET    *lpplMac;
    int16_t    cMax2;

    MineMinerals();
    for (i = 0; i < game.cPlayer; i++) {
        rgplr[i].lResLastYear = 0;
    }
    lppl = lpPlanets;
    lpplMac = lpPlanets + cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (!lppl->lpplprod) {
            if (lppl->iPlayer != iplrNone) {
                FSendPlrMsg2(lppl->iPlayer, idmProductionQueueEmpty, lppl->id, lppl->id, 0);
                lResCur = CResourcesAtPlanet(lppl, lppl->iPlayer);
                if (lResCur != 0 && vrgPlanResExtra[lppl->id] != 0) {
                    lResCur += (int32_t)(lResCur * (uint32_t)vrgPlanResExtra[lppl->id]) / (int32_t)((uint32_t)vrgPlanResExtra[lppl->id] + lResCur);
                }
                rgplr[lppl->iPlayer].lResLastYear = rgplr[lppl->iPlayer].lResLastYear + lResCur;
            }
        } else if (lppl->iPlayer != iplrNone && lppl->lpplprod->iprodMac != 0) {
            fNoResearch = lppl->fNoResearch;
            for (i = 0; i < 3; i++) {
                rgResAvail[i] = lppl->rgwtMin[i];
            }
            lResCur = CResourcesAtPlanet(lppl, lppl->iPlayer);
            if (lResCur != 0 && vrgPlanResExtra[lppl->id] != 0) {
                lResCur += (int32_t)(lResCur * (uint32_t)vrgPlanResExtra[lppl->id]) / (int32_t)((uint32_t)vrgPlanResExtra[lppl->id] + lResCur);
            }
            rgResAvail[3] = lResCur;
            if (rgResAvail[3] != 0) {
                if (fNoResearch) {
                    lResearchTake = 0;
                } else {
                    lResearchTake = (int32_t)(rgResAvail[3] * (int16_t)rgplr[lppl->iPlayer].pctResearch) / 100;
                    rgResAvail[3] -= lResearchTake;
                    rgplr[lppl->iPlayer].lResLastYear = rgplr[lppl->iPlayer].lResLastYear + lResearchTake;
                }
                fAutoBuildDone = TRUE;
            TopOfQueue:
                fPrevProdIsAlch = FALSE;
                iprodCur = 0;
                while (lppl->lpplprod && iprodCur < lppl->lpplprod->iprodMac) {
                    lpprod = &lppl->lpplprod->rgprod[iprodCur];
                    if (lpprod->cItem <= 0)
                        goto RemoveFromQueue;
                    if (lpprod->grobj == grobjPlanet) {
                        if ((lpprod->iItem >= iobjPlanetaryScannerFirst && lpprod->iItem <= iobjPlanetaryScannerSnooper620X) ||
                            lpprod->iItem == iobjPlanetaryScanner) {
                            if (lppl->iScanner != 31) {
                                FSendPlrMsg2(lppl->iPlayer, idmOrderBuildScannerCanceledAlreadyHaveScanner, lppl->id, lppl->id, 0);
                                goto RemoveFromQueue;
                            }
                        } else if (lpprod->iItem >= iobjPacketIron && lpprod->iItem <= iobjPacketMixed) {
                            if (IWarpMAFromLppl(lppl, NULL) == 0 || lppl->idFling == 0) {
                                FSendPlrMsg2(lppl->iPlayer, idmHasOrdersBuildMineralPacketEitherDoesnt, lppl->id, lppl->id, 0);
                                goto RemoveFromQueue;
                            }
                        } else {
                            switch (lpprod->iItem) {
                            case mdIdleFactory:
                                cMax = CMaxFactories(lppl, lppl->iPlayer);
                                cMax2 = CMaxOperableFactories(lppl, lppl->iPlayer, TRUE);
                                if (cMax2 > cMax) {
                                    cMax = cMax2;
                                }
                                cMax -= lppl->cFactories;
                            LCantBuildP:
                                idm = idmHasOrdersBuildPlanetaryInstallationsBeyondMaximu;
                            LCantBuildP2:
                                if (cMax < (int32_t)lpprod->cItem) {
                                    FSendPlrMsg2(lppl->iPlayer, idm, lppl->id, lppl->id, 0);
                                    if (cMax <= 0)
                                        goto RemoveFromQueue;
                                    lpprod->cItem = cMax;
                                }
                                break;
                            case mdIdleMine:
                                cMax = CMaxMines(lppl, lppl->iPlayer);
                                cMax2 = CMaxOperableMines(lppl, lppl->iPlayer, TRUE);
                                if (cMax2 > cMax) {
                                    cMax = cMax2;
                                }
                                cMax -= lppl->cMines;
                                goto LCantBuildP;
                            case mdIdleDefense:
                                cMax = CMaxDefenses(lppl, lppl->iPlayer);
                                cMax2 = CMaxOperableDefenses(lppl, lppl->iPlayer, TRUE);
                                if (cMax2 > cMax) {
                                    cMax = cMax2;
                                }
                                cMax -= lppl->cDefenses;
                                goto LCantBuildP;
                            case mdIdleTerraform:
                                cMax = IpctCanTerraformLppl(lppl);
                                idm = idmHasOrdersTerraformBeyondMaximumAllowedOrders;
                                goto LCantBuildP2;
                            }
                        }
                    }
                    if (lpprod->iItem == iobjAlchemy && lpprod->grobj == grobjPlanet && iprodCur < lppl->lpplprod->iprodMac - 1) {
                        fPrevProdIsAlch = TRUE;
                        iprodCur++;
                        continue;
                    }
                    prodPartial.cItem = 0;
                    cBuilt = CBuildProdItem(lppl, lpprod, &prodPartial, rgResAvail, fPrevProdIsAlch, (int16_t *)&mdStatus, FALSE);
                    if (fAutoBuildDone && (mdStatus == mdProdStatSomeAuto || mdStatus == mdProdStatNoneAuto)) {
                        fAutoBuildDone = FALSE;
                    }
                    if (cBuilt > 0 && !FBuildObject(lppl, lpprod->grobj, lpprod->iItem, cBuilt, rgResAvail) &&
                        (lpprod->grobj != grobjPlanet || lpprod->iItem >= mdIdleFactory)) {
                        lpprod->cItem = 0;
                    }
                    if (lppl->iPlayer == iplrNone && !lppl->lpplprod)
                        goto TopOfQueue;
                    if (mdStatus == mdProdStatComplete) {
                    RemoveFromQueue:
                        if (lppl->lpplprod->iprodMac == fPrevProdIsAlch + 1) {
                            FreePl((PL *)lppl->lpplprod);
                            lppl->lpplprod = NULL;
                            break;
                        }
                        if (iprodCur < lppl->lpplprod->iprodMac - 1) {
                            memmove(lppl->lpplprod + (1 + (iprodCur - fPrevProdIsAlch)), lppl->lpplprod + (1 + (iprodCur + 1)),
                                    (lppl->lpplprod->iprodMac - iprodCur - 1) * 4);
                        }
                        lppl->lpplprod->iprodMac -= fPrevProdIsAlch + 1;
                        iprodCur -= fPrevProdIsAlch + 1;
                    } else if ((int16_t)mdStatus >= mdProdStatSome) {
                        if (prodPartial.cItem > 0) {
                            if (lppl->lpplprod->iprodMac == lppl->lpplprod->iprodMax) {
                                lppl->lpplprod = (PLPROD *)LpplReAlloc((PL *)lppl->lpplprod, lppl->lpplprod->iprodMac + 1);
                            }
                            memmove(&lppl->lpplprod->rgprod[1], lppl->lpplprod->rgprod, lppl->lpplprod->iprodMac * sizeof(PROD));
                            lppl->lpplprod->rgprod[0] = prodPartial;
                            lppl->lpplprod->iprodMac++;
                        }
                        break;
                    }
                    iprodCur++;
                    fPrevProdIsAlch = FALSE;
                }
                if (!lppl->lpplprod || (iprodCur >= lppl->lpplprod->iprodMac && fAutoBuildDone)) {
                    FSendPlrMsg2(lppl->iPlayer, idmHasCompletedOrdersProductionQueueEmpty, lppl->id, lppl->id, 0);
                }
                for (i = 0; i < 3; i++) {
                    lppl->rgwtMin[i] = rgResAvail[i];
                }
                rgplr[lppl->iPlayer].lResLastYear = rgplr[lppl->iPlayer].lResLastYear + rgResAvail[3];
            }
        }
    }
    UpdatePopulations();
    UpdateResearchStatus(TRUE);
    if (!game.fNoRandom) {
        RandomEvents();
    }
    /* The score counts each planet as it produced and grew. The original
       counted it at the end of the turn, so loading colonists at waypoint 1
       hid a player's resources from the published scores. vrgwtPopScore
       and vrgiplrPopScore hold each planet's population and owner. */
    if (vrgwtPopScore) {
        lppl = lpPlanets;
        lpplMac = lpPlanets + cPlanet;
        for (; lppl < lpplMac; lppl++) {
            vrgwtPopScore[lppl->id] = lppl->rgwtMin[3];
            vrgiplrPopScore[lppl->id] = lppl->iPlayer;
        }
    }
    return;
}

int16_t CBuildProdItem(PLANET *lppl, PROD *lpprod, PROD *pprodPartial, int32_t *rgRes, int16_t fAlchemy, int16_t *pmdStatus, int16_t fCalcOnly) {
    int32_t  pctT;
    int16_t  cMax;
    uint32_t iobjOther;
    int32_t  cCanBuild;
    int32_t  lMinNeeded;
    int32_t  lAlchCost;
    PROD     prod;
    int16_t  fAutoBuild;
    int16_t  cBuilt;
    int16_t  cAlchemy;
    int32_t  rgCostPaid[4];
    int16_t  i;
    int16_t  fResourceBlocked;
    int32_t  pctInitial;
    int32_t  pctTooBig;
    int32_t  pct;
    int32_t  rgCost[4];
    int16_t  fMineralBlocked;
    int32_t  AddCost;

    cAlchemy = 0;
    pctInitial = lpprod->pct;
    prod = *lpprod;
    GetProductionCosts(lppl, lpprod, rgCost, lppl->iPlayer, TRUE);
    cBuilt = 0;
    fAutoBuild = prod.grobj == grobjPlanet && prod.iItem < mdIdleFactory;
    if (fAutoBuild) {
        cMax = 1000;
        switch (prod.iItem) {
        case iobjMine:
            iobjOther = mdIdleMine;
            cMax = CMaxOperableMines(lppl, lppl->iPlayer, TRUE) - lppl->cMines;
            break;
        case iobjFactory:
            iobjOther = mdIdleFactory;
            cMax = CMaxOperableFactories(lppl, lppl->iPlayer, TRUE) - lppl->cFactories;
            break;
        case iobjDefense:
            iobjOther = mdIdleDefense;
            cMax = CMaxOperableDefenses(lppl, lppl->iPlayer, TRUE) - lppl->cDefenses;
            break;
        case iobjAlchemy:
            iobjOther = mdIdleAlchemy;
            break;
        case iobjMinTerraform:
        case iobjMaxTerraform:
            iobjOther = mdIdleTerraform;
            cMax = IpctCanTerraformLppl(lppl);
            if (cMax <= 0 || prod.iItem != iobjMinTerraform || ChgPopFromPlanet(lppl, FALSE) < 0 || PctPlanetDesirability(lppl, lppl->iPlayer) <= 0)
                break;
            cMax = 0;
            break;
        case iobjPacket:
            iobjOther = iobjPacketMixed;
            if (IWarpMAFromLppl(lppl, NULL) == 0 || lppl->idFling == 0) {
                cMax = 0;
            }
        }
        if (cMax < 0) {
            cMax = 0;
        }
        if ((uint32_t)prod.cItem > (uint32_t)cMax || prod.iItem == iobjAlchemy) {
            prod.cItem = cMax;
        }
    }
    for (i = 0; i < 4; i++) {
        rgCostPaid[i] = (uint32_t)((uint32_t)(rgCost[i] * prod.pct) / 100);
    }
    while (prod.cItem > 0) {
        for (i = 0; i < 4 && rgCost[i] - rgCostPaid[i] <= rgRes[i]; i++) {
        }
        if (i < 4) {
            fMineralBlocked = FALSE;
            fResourceBlocked = FALSE;
            pct = 100;
            for (i = 0; i < 4; i++) {
                if (rgCost[i] > 0) {
                    if (rgRes[i] >= rgCost[i]) {
                        pctT = 100;
                    } else {
                        pctT = (int32_t)((int32_t)((rgRes[i] + rgCostPaid[i]) * 100) / rgCost[i]);
                        pctTooBig = (int32_t)((int32_t)((rgRes[i] + rgCostPaid[i] + 1) * 100) / rgCost[i]);
                        pctT = pctT <= pctTooBig - 1 ? pctTooBig - 1 : pctT;
                    }
                    if (pctT < pct) {
                        lMinNeeded = rgCost[i] - rgCostPaid[i] - rgRes[i];
                        pct = pctT;
                        if (i == 3) {
                            fResourceBlocked = TRUE;
                        } else {
                            fMineralBlocked = TRUE;
                        }
                    }
                }
            }
            if (fMineralBlocked && fAutoBuild) {
                if (fAlchemy)
                    goto LAlchemize;
                fAutoBuild = 2;
                break;
            }
            for (i = 0; i < 4; i++) {
                AddCost = (int32_t)(rgCost[i] * pct) / 100 - rgCostPaid[i];
                rgRes[i] -= AddCost;
                rgCostPaid[i] += AddCost;
            }
            prod.pct = LOWORD(pct);
            if (!fAlchemy || fResourceBlocked)
                break;
        LAlchemize:
            lAlchCost = (uint32_t)(GetRaceGrbit(&rgplr[lppl->iPlayer], ibitRaceMineralAlchemy) == 0 ? 100 : 25);
            cCanBuild = (int32_t)(rgRes[3] / lAlchCost);
            if (cCanBuild > lMinNeeded) {
                cCanBuild = lMinNeeded;
            }
            if (cCanBuild > 0) {
                for (i = 0; i < 3; i++) {
                    rgRes[i] += cCanBuild;
                }
                rgRes[i] -= (uint32_t)(lAlchCost * cCanBuild);
                cAlchemy += LOWORD(cCanBuild);
            }
            if (cCanBuild == lMinNeeded)
                continue;
            if (rgRes[3] > 0 && pprodPartial) {
                memset(pprodPartial, 0, sizeof(PROD));
                pprodPartial->grobj = grobjPlanet;
                pprodPartial->iItem = mdIdleAlchemy;
                pprodPartial->cItem = 1;
                pctT = (int32_t)((int32_t)(rgRes[3] * 100) / lAlchCost);
                pctTooBig = (int32_t)((int32_t)((rgRes[3] + 1) * 100) / lAlchCost);
                pctT = max(pctT, pctTooBig - 1);
                pprodPartial->pct = LOWORD(pctT);
                rgRes[3] -= (int32_t)(pctT * lAlchCost) / 100;
            }
            break;
        } else {
            cBuilt++;
            prod.cItem--;
            prod.pct = 0;
            for (i = 0; i < 4; i++) {
                rgRes[i] -= rgCost[i] - rgCostPaid[i];
                rgCostPaid[i] = 0;
            }
        }
    }
    if (cBuilt > 0 && prod.grobj == grobjPlanet && (prod.iItem == mdIdleAlchemy || prod.iItem == iobjAlchemy)) {
        cAlchemy += cBuilt;
        for (i = 0; i < 3; i++) {
            rgRes[i] += cBuilt;
        }
    }
    if (cAlchemy != 0 && !fCalcOnly && gd.fGeneratingTurn) {
        FSendPlrMsg2(lppl->iPlayer, idmScientistsHaveTransmutedCommonMaterialsKtEach, lppl->id, lppl->id, cAlchemy);
    }
    if (pmdStatus) {
        if (fAutoBuild == 2) {
            *pmdStatus = cBuilt <= 0 ? mdProdStatNoneAuto : mdProdStatSomeAuto;
        } else if (fAutoBuild && prod.cItem == 0) {
            *pmdStatus = cBuilt <= 0 ? mdProdStatSkippedAuto : mdProdStatCompleteAuto;
        } else if (cBuilt == 0) {
            *pmdStatus = pctInitial == prod.pct ? mdProdStatBlockedSame : mdProdStatBlockedDiff;
        } else if (prod.cItem == 0) {
            *pmdStatus = mdProdStatComplete;
        } else {
            *pmdStatus = mdProdStatSome;
        }
    }
    if (!fCalcOnly && !fAutoBuild) {
        *lpprod = prod;
    }
    if (fAutoBuild && pprodPartial && pprodPartial->cItem == 0 && prod.pct > 0) {
        *pprodPartial = prod;
        pprodPartial->cItem = 1;
        pprodPartial->iItem = LOWORD(iobjOther);
    }
    return cBuilt;
}

int16_t FBuildObject(PLANET *lppl, GrobjClass grobj, int16_t iItem, int16_t cBuilt, int32_t *rgMinerals) {
    int16_t       iWarp;
    int16_t       i;
    FLEET        *lpfl;
    MessageId     idm;
    int16_t       fTwoMAs;
    SHDEF        *lpshdef;
    int16_t       cAllowed;
    int32_t       dpOrig;
    int16_t       cshDamaged;
    int16_t       cshOrig;
    uint16_t      dpShdef;
    THING        *lpthMac;
    PacketDecay   iDecayRate;
    THING        *lpth;
    RaceAttribute raMajor;
    int16_t       iWarpAsked;
    int16_t       cSize;
    int16_t       rgwt[3];
    int32_t       l;
    EnvType       iEnv;
    PART          part;

    if (grobj == grobjFleet) {
        if (iItem >= 16) {
            iItem -= 16;
            lpshdef = rglpshdefSB[lppl->iPlayer] + iItem;
            if (lpshdef->fFree || !FCanBuildShdef(lpshdef, lppl->iPlayer)) {
                return FALSE;
            }
            idm = idmHasBuiltNew;
            if (lpshdef->hul.wtCargoMax != 0) {
                idm++;
                if ((uint32_t)lpshdef->hul.wtCargoMax == 0xffff) {
                    idm++;
                }
            }
            FSendPlrMsg(lppl->iPlayer, idm, lppl->id, lppl->id, lppl->iPlayer << 5 | (iItem + 0x10), LphuldefFromId(lpshdef->hul.ihuldef)->hul.wtCargoMax, 0, 0,
                        0, 0);
            if (lppl->fStarbase && (int16_t)rglpshdefSB[lppl->iPlayer][lppl->isb].hul.ihuldef > (int16_t)rglpshdefSB[lppl->iPlayer][iItem].hul.ihuldef) {
                KillQueuedShips(lppl);
            }
            iWarp = IWarpMAFromLppl(lppl, &fTwoMAs);
            if (lppl->fStarbase) {
                rglpshdefSB[lppl->iPlayer][lppl->isb].cExist = rglpshdefSB[lppl->iPlayer][lppl->isb].cExist - 1;
            } else {
                lppl->fStarbase = TRUE;
            }
            lppl->isb = iItem;
            if (iWarp <= 0) {
                iWarp = IWarpMAFromLppl(lppl, &fTwoMAs);
                if (iWarp > 0) {
                    lppl->iWarpFling = iWarp + fTwoMAs - 4;
                } else {
                    lppl->iWarpFling = 0;
                    lppl->idFling = 0;
                    KillQueuedMassPackets(lppl);
                }
            }
            lpshdef->cBuilt++;
            lpshdef->cExist++;
            return TRUE;
        }
        if (!lppl->fStarbase || iItem >= 16) {
            return FALSE;
        }
        lpshdef = rglpshdef[lppl->iPlayer] + iItem;
        if (lpshdef->fFree || !FCanBuildShdef(lpshdef, lppl->iPlayer)) {
            FSendPlrMsg2(lppl->iPlayer, idmStarbaseFailedBuildNewShipTypeBecause, lppl->id, iItem + 1, 0);
            return FALSE;
        }
        if (rgplr[lppl->iPlayer].cFleet == 0x200) {
            for (i = 0; i < cFleet; i++) {
                lpfl = rglpfl[i];
                if (!rglpfl[i] || lpfl->iPlayer > lppl->iPlayer)
                    break;
                if (lpfl->iPlayer >= lppl->iPlayer && lpfl->lpplord->rgord[0].pt.x == rgptPlan[lppl->id].x &&
                    lpfl->lpplord->rgord[0].pt.y == rgptPlan[lppl->id].y && 32766 - cBuilt > lpfl->rgcsh[iItem]) {
                    if (lpfl->rgcsh[iItem] != 0 && lpfl->rgdv[iItem].pctDp != 0) {
                        dpShdef = rglpshdef[lpfl->iPlayer][iItem].hul.dp;
                        cshOrig = lpfl->rgcsh[iItem];
                        cshDamaged = LOWORD((int32_t)(lpfl->rgdv[iItem].pctSh * cshOrig) / 100);
                        if (cshDamaged == 0) {
                            cshDamaged = 1;
                        }
                        dpOrig = (int32_t)((int32_t)((uint32_t)dpShdef * lpfl->rgdv[iItem].pctDp) / 10 * cshDamaged) / 50;
                        lpfl->rgdv[iItem].pctSh = LOWORD((int32_t)(cshDamaged * 100) / (int16_t)(cshOrig + cBuilt));
                        if (lpfl->rgdv[iItem].pctSh == 0) {
                            lpfl->rgdv[iItem].pctSh = 1;
                        }
                        cshDamaged = LOWORD((int32_t)(lpfl->rgdv[iItem].pctSh * (int16_t)(cshOrig + cBuilt)) / 100);
                        if (cshDamaged == 0) {
                            cshDamaged = 1;
                        }
                        lpfl->rgdv[iItem].pctDp = LOWORD((int32_t)((int32_t)(dpOrig * 5) / cshDamaged) * 100 / (int32_t)dpShdef);
                    } else {
                        lpfl->rgdv[iItem].dp = 0;
                    }
                    CreateShip(lppl->iPlayer, lpfl, iItem, cBuilt);
                    FSendPlrMsg(lppl->iPlayer, idmStarbaseBuiltNewSDueLack27b, lpfl->id | 0x8000, lppl->id, cBuilt, lppl->iPlayer << 5 | iItem, lpfl->id, 0, 0,
                                0);
                    return TRUE;
                }
            }
            FSendPlrMsg(lppl->iPlayer, idmStarbaseBuiltNewShipSTypeLost, lppl->id, lppl->id, cBuilt, lppl->iPlayer << 5 | iItem, 0, 0, 0, 0);
            return FALSE;
        }
        lpfl = LpflNew(lppl->iPlayer, lppl->id);
        CreateShip(lppl->iPlayer, lpfl, iItem, cBuilt);
        lpfl->rgwtMin[4] = LGetFleetStat(lpfl, 1);
        if (lppl->idRoute != 0) {
            AutoRouteFleet(lpfl, lppl);
            if (cBuilt == 1) {
                idm = lpfl->lpplord->rgord[1].iWarp == 0 ? idmStarbaseHasBuiltNewWhichWillRouted : idmStarbaseHasBuiltNewWhichRouted;
                FSendPlrMsg(lppl->iPlayer, idm, lpfl->id | 0x8000, lppl->id, lppl->iPlayer << 5 | iItem, lppl->idRoute - 1, 0, 0, 0, 0);
            } else {
                idm = lpfl->lpplord->rgord[1].iWarp == 0 ? idmStarbaseHasBuiltNewShipsWhichWill : idmStarbaseHasBuiltNewShipsWhichRouted;
                FSendPlrMsg(lppl->iPlayer, idm, lpfl->id | 0x8000, lppl->id, cBuilt, lppl->iPlayer << 5 | iItem, lppl->idRoute - 1, 0, 0, 0);
            }
        } else {
            AutoFleetOrder(lpfl, lppl);
            if (cBuilt == 1) {
                FSendPlrMsg2(lppl->iPlayer, idmStarbaseHasBuiltNew, lpfl->id | 0x8000, lppl->id, lppl->iPlayer << 5 | iItem);
            } else {
                FSendPlrMsg(lppl->iPlayer, idmStarbaseHasBuiltNewShips, lpfl->id | 0x8000, lppl->id, cBuilt, lppl->iPlayer << 5 | iItem, 0, 0, 0, 0);
            }
        }
    } else {
        if (grobj != grobjPlanet) {
            return FALSE;
        }
        if ((uint16_t)iItem > iobjPlanetaryScanner) {
            return FALSE;
        }
        switch (iItem) {
        case iobjFactory:
        case mdIdleFactory:
            cAllowed = CMaxFactories(lppl, lppl->iPlayer) - lppl->cFactories;
            cBuilt = cBuilt >= cAllowed ? cAllowed : cBuilt;
            if (cBuilt <= 0)
                return FALSE;
            lppl->cFactories += cBuilt;
            idm = idmHaveBuiltFactory;
        SendMsgFactMine:
            cBuilt += FRemovePlayerMessage(lppl->iPlayer, idm, lppl->id);
            if (cBuilt > 1) {
                FSendPlrMsg2(lppl->iPlayer, idm + 1, lppl->id, cBuilt, lppl->id);
            } else {
                FSendPlrMsg2(lppl->iPlayer, idm, lppl->id, lppl->id, 0);
            }
            break;
        case iobjMine:
        case mdIdleMine:
            cAllowed = CMaxMines(lppl, lppl->iPlayer) - lppl->cMines;
            cBuilt = cBuilt >= cAllowed ? cAllowed : cBuilt;
            if (cBuilt > 0) {
                lppl->cMines += cBuilt;
                idm = idmHaveBuiltMine;
                goto SendMsgFactMine;
            }
            return FALSE;
        case iobjDefense:
        case mdIdleDefense:
            cAllowed = CMaxDefenses(lppl, lppl->iPlayer) - lppl->cDefenses;
            cBuilt = cBuilt >= cAllowed ? cAllowed : cBuilt;
            if (cBuilt > 0) {
                lppl->cDefenses += cBuilt;
                idm = idmHaveBuiltDefenseOutpost;
                goto SendMsgFactMine;
            }
            return FALSE;
        case 10:
            break;
        case iobjAlchemy:
        case mdIdleAlchemy:
            break;
        case iobjPacket:
        case iobjPacketIron:
        case iobjPacketBor:
        case iobjPacketGerm:
        case iobjPacketMixed:
            raMajor = GetRaceStat(&rgplr[lppl->iPlayer], rsMajorAdv);
            iWarp = IWarpMAFromLppl(lppl, &fTwoMAs);
            if (iWarp == 0) {
                FSendPlrMsg2(lppl->iPlayer, idmMineralPacketFormedHasDisintegratedBecausePlanet, lppl->id, lppl->id, 0);
                return FALSE;
            }
            if (lppl->idFling == 0) {
                FSendPlrMsg2(lppl->iPlayer, idmMineralPacketFormedHasDisintegratedBecauseDidnt, lppl->id, lppl->id, 0);
                return FALSE;
            }
            if (iItem == iobjPacket) {
                iItem = iobjPacketMixed;
            }
            if (iItem == iobjPacketMixed) {
                cSize = raMajor == raMassAccel ? 25 : 40;
            } else {
                cSize = raMajor == raMassAccel ? 70 : 100;
            }
            for (i = 0; i < 3; i++) {
                if (i == iItem - 14 || iItem == iobjPacketMixed) {
                    l = (uint32_t)(cSize * cBuilt);
                    if (l > 32760) {
                        l = 32760;
                    }
                    rgwt[i] = LOWORD(l);
                } else {
                    rgwt[i] = 0;
                }
            }
            iWarpAsked = lppl->iWarpFling + 4;
            if (iWarpAsked < 5 || iWarpAsked > iWarp + 3) {
                iWarpAsked = iWarp + fTwoMAs;
            }
            if (iWarpAsked <= iWarp + fTwoMAs) {
                iDecayRate = decayNone;
            } else {
                iDecayRate = iWarpAsked - iWarp - fTwoMAs;
            }
            if (raMajor == raStargate && (int16_t)iDecayRate < decay50Pct) {
                iDecayRate++;
            }
            iWarp = iWarpAsked - 4;
            lpth = lpThings;
            lpthMac = lpThings + cThing;
            for (; lpth < lpthMac &&
                   (lpth->iplr != lppl->iPlayer || lpth->ith != ithMineralPacket || lpth->pt.x != rgptPlan[lppl->id].x || lpth->pt.y != rgptPlan[lppl->id].y ||
                    lpth->thp.iWarp != iWarp || lpth->thp.idPlanet != lppl->idFling - 1 || lpth->thp.iDecayRate != iDecayRate || lpth->thp.wtMax >= 1630);
                 lpth++) {
            }
            if (lpth != lpthMac) {
                lpth->thp.wtMax = 0;
                for (i = 0; i < 3; i++) {
                    lpth->thp.rgwtMin[i] += rgwt[i];
                    if (lpth->thp.rgwtMin[i] < 0) {
                        lpth->thp.rgwtMin[i] = 32760;
                    }
                    lpth->thp.wtMax += (int16_t)(lpth->thp.rgwtMin[i] + 9) / 10;
                }
                FSendPlrMsg2(lppl->iPlayer, idmHasProducedMineralPacketWhichHasCombined, lppl->id, lppl->id, lppl->idFling - 1);
                return TRUE;
            }
            lpth = LpthNew(lppl->iPlayer, ithMineralPacket);
            if (!lpth) {
                FSendPlrMsg2(lppl->iPlayer, idmHasOrdersBuildMineralPacketEitherDoesnt, lppl->id, lppl->id, 0);
                return TRUE;
            }
            for (i = 0; i < 3; i++) {
                lpth->thp.rgwtMin[i] = rgwt[i];
                lpth->thp.wtMax += (int16_t)(rgwt[i] + 9) / 10;
            }
            lpth->thp.iWarp = iWarp;
            lpth->thp.iDecayRate = iDecayRate;
            lpth->thp.idPlanet = lppl->idFling - 1;
            lpth->pt = rgptPlan[lppl->id];
            FSendPlrMsg2(lppl->iPlayer, idmHasProducedMineralPacketWhichHasDestination, lppl->id, lppl->id, lppl->idFling - 1);
            return TRUE;
        case iobjGenesis:
            for (i = 0; i < game.cPlayer; i++) {
                FSendPlrMsg2(i, idmStrongFundamentalForcesHaveRebirthed, lppl->id, lppl->id, 0);
            }
            if (GetRaceStat(&rgplr[lppl->iPlayer], rsMajorAdv) != raMacintosh) {
                lppl->cFactories = 0;
                lppl->cMines = 0;
                lppl->cDefenses = 0;
                lppl->iScanner = 31;
            }
            for (i = 0; i < 3; i++) {
                lppl->rgwtMin[i] = 0;
                lppl->rgEnvVar[i] = lppl->rgEnvVarOrig[i] = Random(50) + Random(50) + 1;
                lppl->rgMinConc[i] = Random(40) + Random(40) + 25;
            }
            return TRUE;
        case iobjMinTerraform:
        case iobjMaxTerraform:
        case mdIdleTerraform:
            while (cBuilt-- != 0) {
                i = IBestTerraform(lppl, TRUE);
                if (i != 0) {
                    iEnv = abs(i) - 1;
                    cAllowed = lppl->rgEnvVar[iEnv] + (i <= 0 ? -1 : 1);
                    if (1 > (99 >= cAllowed ? cAllowed : 99)) {
                        cAllowed = 1;
                    } else if (99 < cAllowed) {
                        cAllowed = 99;
                    }
                    lppl->rgEnvVar[iEnv] = cAllowed;
                    FSendPlrMsg(lppl->iPlayer, idmTerraformingEffortsHave, lppl->id, lppl->id, i > 0, iEnv, iEnv * 256 + cAllowed, 0, 0, 0);
                }
            }
            return TRUE;
        case iobjPlanetaryScanner:
            idPlayer = lppl->iPlayer;
            LookupBestPlanetaryScanner(&part);
            idPlayer = iplrNone;
            iItem = part.hs.iItem + 18;
            /* fallthrough */
        case iobjPlanetaryScannerFirst:
        case iobjPlanetaryScannerViewer90:
        case iobjPlanetaryScannerScoper150:
        case iobjPlanetaryScannerScoper220:
        case iobjPlanetaryScannerScoper280:
        case iobjPlanetaryScannerSnooper320X:
        case iobjPlanetaryScannerSnooper400X:
        case iobjPlanetaryScannerSnooper500X:
        case iobjPlanetaryScannerSnooper620X:
            FSendPlrMsg(lppl->iPlayer, idmHasBuiltNewPlanetaryScanner, lppl->id, lppl->id, -32768, iItem - 18, 0, 0, 0, 0);
            lppl->iScanner = iItem - 18;
            break;
        }
    }
    return TRUE;
}

void CreateShip(int16_t iPlr, FLEET *lpfl, int16_t ishdef, int16_t cShip) {
    lpfl->rgcsh[ishdef] += cShip;
    rglpshdef[iPlr][ishdef].cExist += cShip;
    rglpshdef[iPlr][ishdef].cBuilt += cShip;
    return;
}

void RandomEvents() {
    MeteorStrike();
    PlanetaryClimateChange();
    DiscoverNewMinerals();
    MysteryTrader();
    return;
}

void TransferToOthers() {
    int32_t   l2;
    int16_t   idDst;
    XFER      rgxf[2];
    int16_t   idSrc;
    int16_t   i;
    MessageId idm;
    XFERFULL *lpxfMax;
    XFERFULL *lpxfCur;
    int32_t   l;

    if (cXferFull != 0) {
        lpxfCur = lpxf;
        lpxfMax = lpxf + cXferFull;
        for (; lpxfCur < lpxfMax; lpxfCur++) {
            if (!FLookupObject(lpxfCur->grobj2, lpxfCur->id2, &rgxf[1].fl))
                goto DoNext;
            if (lpxfCur->grobj1 == 1) {
                rgxf[0].fl.iPlayer = LpplFromId(lpxfCur->id1)->iPlayer;
            } else {
                rgxf[0].fl.iPlayer = lpxfCur->id1 >> 9 & 0xf;
            }
            for (i = 0; i < 5; i++) {
                l2 = lpxfCur->rgcQuan[i];
                if (l2 != 0) {
                    l = ChgCargo(lpxfCur->grobj2, lpxfCur->id2, i, l2, NULL);
                    if (l != l2) {
                        idm = i == 4 ? idmAttemptedTransferColonistsSuccessfullyReceivedRe : idmAttemptedTransferSuccessfullyReceived;
                        idSrc = lpxfCur->id1 | (lpxfCur->grobj1 == 2 ? 0x8000 : 0);
                        idDst = lpxfCur->id2 | (lpxfCur->grobj2 == 2 ? 0x8000 : 0);
                        if (l == 0) {
                            idm += 4;
                        }
                        FSendPlrMsg(rgxf[0].fl.iPlayer, idm, idSrc, idSrc, LOWORD(l2), HIWORD(l2), i, idDst, LOWORD(l), HIWORD(l));
                        idm = i == 4 ? idmReceivedHoweverColonistsSentRemainsOtherColonist : idmReceivedHoweverSentRemainderLostSpace;
                        if (l == 0) {
                            FSendPlrMsg(rgxf[1].fl.iPlayer, idm + 4, idDst, idDst, LOWORD(l2), HIWORD(l2), i, idSrc, 0, 0);
                        } else {
                            FSendPlrMsg(rgxf[1].fl.iPlayer, idm, idDst, idDst, LOWORD(l), HIWORD(l), i, idSrc, LOWORD(l2), HIWORD(l2));
                        }
                    } else {
                        idm = i == 4 ? idmSuccessfullyTransferred2 : idmSuccessfullyTransferred;
                        idSrc = lpxfCur->id1 | (lpxfCur->grobj1 == 2 ? 0x8000 : 0);
                        idDst = lpxfCur->id2 | (lpxfCur->grobj2 == 2 ? 0x8000 : 0);
                        FSendPlrMsg(rgxf[0].fl.iPlayer, idm, idSrc, idSrc, LOWORD(l), HIWORD(l), i, idDst, 0, 0);
                        idm = i == 4 ? idmSuccessfullyReceived2 : idmSuccessfullyReceived;
                        FSendPlrMsg(rgxf[1].fl.iPlayer, idm, idDst, idDst, LOWORD(l), HIWORD(l), i, idSrc, 0, 0);
                    }
                }
            }
        DoNext:;
        }
    }
    return;
}

void DropColonists() {
    COLDROP *lpcdLook;
    int16_t  fTie;
    int32_t  cMax;
    PLANET   pl;
    int32_t  lDefensePower;
    int32_t  cPowerTot;
    COLDROP *lpcdCur;
    int16_t  iMax;
    int16_t  idPlanet;
    int16_t  iplrOldOwner;
    int32_t  cColTot;
    int32_t  lOldPop;
    int32_t  c2nd;
    int16_t  i;
    int32_t  rgcPower[16];
    int16_t  cSides;
    float    pctSurvive;
    int32_t  rgcCol[16];
    int32_t  lPower;
    COLDROP *lpcdMax;
    int16_t  cpq;
    int16_t  iDst;
    PROD     prod;
    int16_t  ipq;
    int16_t  iTech;
    int16_t  iBonus;

    if (cColDrop != 0) {
        lpcdCur = lpcd;
        lpcdMax = lpcd + cColDrop;
        for (; lpcdCur < lpcdMax; lpcdCur++) {
            if (lpcdCur->idPlanetDst != idplNone && lpcdCur->cColonist != 0) {
                memset(rgcCol, 0, 64);
                memset(rgcPower, 0, 64);
                cPowerTot = 0;
                cColTot = 0;
                idPlanet = lpcdCur->idPlanetDst;
                FLookupPlanet(idPlanet, &pl);
                iplrOldOwner = pl.iPlayer;
                CalcPctSurvive(&pl, &pctSurvive, NULL);
                pctSurvive = Sf32From80((Sf80Add(Sf80From32(pctSurvive), Sf80Div((Sf80Sub(Sf80From64(1.0), Sf80From32(pctSurvive))), Sf80From64(4.0)))));
                for (lpcdLook = lpcdCur; lpcdLook < lpcdMax; lpcdLook++) {
                    if (idPlanet == lpcdLook->idPlanetDst) {
                        if (GetRaceStat(&rgplr[lpcdLook->idPlr], rsMajorAdv) == raMacintosh && (!lpcdLook->fCanColonize || pl.iPlayer != iplrNone)) {
                            FSendPlrMsg2(lpcdLook->idPlr, idmColonistsAttemptingSetShopReducedProtoplasmicBlo, pl.id, pl.id, 0);
                        } else if (pl.iPlayer == iplrNone && !lpcdLook->fCanColonize) {
                            FSendPlrMsg(lpcdLook->idPlr, idmColonistsForcedTransportDiedBecauseDidColonize, pl.id, LOWORD(lpcdLook->cColonist),
                                        HIWORD(lpcdLook->cColonist), pl.id, 0, 0, 0, 0);
                        } else if (pl.fStarbase && pl.iPlayer != iplrNone) {
                            FSendPlrMsg2(lpcdLook->idPlr, idmColonistsAssaultingHaveKilledForcesOrbitingStarb, pl.id, pl.id, 0);
                        } else {
                            rgcCol[lpcdLook->idPlr] = rgcCol[lpcdLook->idPlr] + lpcdLook->cColonist;
                            cColTot += lpcdLook->cColonist;
                            if (GetRaceStat(&rgplr[lpcdLook->idPlr], rsMajorAdv) == raAttack) {
                                lPower = 165;
                            } else if (GetRaceStat(&rgplr[lpcdLook->idPlr], rsMajorAdv) == raMacintosh) {
                                lPower = 0;
                            } else {
                                lPower = 110;
                            }
                            lPower = Sf80ToI32((Sf80Mul(Sf80FromI32(((int32_t)(lpcdLook->cColonist * lPower) / 100)), Sf80From32(pctSurvive))));
                            cPowerTot += lPower;
                            rgcPower[lpcdLook->idPlr] = rgcPower[lpcdLook->idPlr] + lPower;
                        }
                        lpcdLook->idPlanetDst = idplNone;
                    }
                }
                if (pl.iPlayer != iplrNone) {
                    if (GetRaceStat(&rgplr[pl.iPlayer], rsMajorAdv) == raDefend) {
                        lPower = 200;
                    } else {
                        lPower = 100;
                    }
                    lDefensePower = (int32_t)(pl.rgwtMin[3] * lPower) / 100;
                    if (lDefensePower > cPowerTot) {
                        for (i = 0; i < 16; i++) {
                            if (rgcCol[i] != 0) {
                                if (Sf80Eq(Sf80From32(pctSurvive), Sf80From64(1.0))) {
                                    FSendPlrMsg(i, idmColonistsDroppedMassacredGroundTroops, pl.id, LOWORD(rgcCol[i]), HIWORD(rgcCol[i]), pl.id,
                                                pl.iPlayer | 0x30, 0, 0, 0);
                                    FSendPlrMsg(pl.iPlayer, idmGroundTroopsValiantlyDestroyedAttackingBarbarian, pl.id, pl.id, LOWORD(rgcCol[i]),
                                                HIWORD(rgcCol[i]), i | 0x30, 0, 0, 0);
                                } else {
                                    FSendPlrMsg(i, idmColonistsDroppedDestroyedPlanetaryDefensesRestMa, pl.id, LOWORD(rgcCol[i]), HIWORD(rgcCol[i]), pl.id,
                                                Sf80ToI32((Sf80Mul((Sf80Sub(Sf80From64(1.0), Sf80From32(pctSurvive))), Sf80FromI32(10000)))), pl.iPlayer | 0x30,
                                                0, 0);
                                    FSendPlrMsg(pl.iPlayer, idmPlanetaryDefensesGroundTroopsDestroyedInvadingTr, pl.id, pl.id, LOWORD(rgcCol[i]),
                                                HIWORD(rgcCol[i]), i | 0x30, 0, 0, 0);
                                }
                            }
                        }
                        pl.rgwtMin[3] -= (int32_t)((int32_t)(pl.rgwtMin[3] * cPowerTot) / lDefensePower);
                        goto WritePlanet;
                    }
                    lOldPop = pl.rgwtMin[3];
                    UninhabitPlanet(&pl);
                } else {
                    lDefensePower = 0;
                    lOldPop = 0;
                }
                cMax = -1;
                c2nd = 0;
                cSides = 0;
                fTie = FALSE;
                iMax = 0;
                for (i = 0; i < game.cPlayer; i++) {
                    if (rgcCol[i] != 0) {
                        cSides++;
                        if (rgcPower[i] >= cMax) {
                            if (rgcPower[i] == cMax) {
                                fTie = TRUE;
                            } else {
                                fTie = FALSE;
                                c2nd = cMax;
                                cMax = rgcPower[i];
                                iMax = i;
                            }
                        }
                    }
                }
                if (cMax < 0)
                    goto IncCur;
                if (fTie) {
                    for (i = 0; i < game.cPlayer; i++) {
                        if (rgcCol[i] != 0) {
                            FSendPlrMsg2(i, idmInvolvedWayAssaultNobodysTroopsSurvivedBrutal, pl.id, cSides, pl.id);
                        }
                    }
                    if (iplrOldOwner != iplrNone) {
                        FSendPlrMsg2(iplrOldOwner, idmMultitudeEnemiesHaveMountedProngAttackResulting, pl.id, cSides, pl.id);
                        pl.iPlayer = iplrNone;
                    }
                } else {
                    if (iplrOldOwner != iplrNone) {
                        for (i = 0; i < 16; i++) {
                            if (rgcCol[i] != 0) {
                                if (i == iMax) {
                                    FSendPlrMsg2(i, idmTroopsCrushSColonistsControlPlanet, pl.id, iplrOldOwner | 0x20, pl.id);
                                } else {
                                    FSendPlrMsg2(i, idmColonistsDroppedDestroyedSpiritedFighting, pl.id, pl.id, 0);
                                }
                            }
                        }
                        FSendPlrMsg(iplrOldOwner, idmHaveAttackedFirstRateStormTroopersThough, pl.id, iMax | 0x30, pl.id, LOWORD(rgcCol[iMax]),
                                    HIWORD(rgcCol[iMax]), 0, 0, 0);
                        memset(rgTechBattle, 0, 6);
                        memset(rgTechTrader, 0, 13);
                        for (i = 0; i < 6; i++) {
                            rgTechBattle[i] = rgplr[iplrOldOwner].rgTech[i];
                        }
                        i = ITechLearnATech(iMax, -1, pl.id, idmWreckageDiscoveredBattleHasBoostedResearchResour, NULL);
                        pl.iPlayer = iplrNone;
                    } else if (cSides > 1) {
                        for (i = 0; i < 16; i++) {
                            if (rgcCol[i] != 0) {
                                if (i == iMax) {
                                    FSendPlrMsg2(i, idmInvolvedWayRaceUninhabitedPlanetForcesCrush, pl.id, cSides, pl.id);
                                } else {
                                    FSendPlrMsg(i, idmColonistsDestroyedWayRaceUninhabitedPlanetContro, pl.id, cSides, pl.id, iMax | 0xb0, 0, 0, 0, 0);
                                }
                            }
                        }
                    } else {
                        FSendPlrMsg2(iMax, idmColonistsControl + (GetRaceStat(&rgplr[iMax], rsMajorAdv) == raMacintosh), pl.id, pl.id, 0);
                    }
                    if (iMax != -1) {
                        cpq = rgplr[iMax].zpq1.cpq;
                        pl.fNoResearch = rgplr[iMax].zpq1.fNoResearch;
                        if (cpq > 0) {
                            pl.lpplprod = (PLPROD *)LpplAlloc(4, rgplr[iMax].zpq1.cpq, htOrd);
                            memset(&prod, 0, sizeof(PROD));
                            prod.grobj = grobjPlanet;
                            iDst = 0;
                            for (ipq = 0; ipq < cpq; ipq++) {
                                if ((GetRaceStat(&rgplr[iMax], rsMajorAdv) != raMacintosh || rgplr[iMax].zpq1.rgpq[ipq].mdIdle > iobjDefense) &&
                                    (GetRaceStat(&rgplr[iMax], rsMajorAdv) != raTerra ||
                                     (rgplr[iMax].zpq1.rgpq[ipq].mdIdle != iobjMinTerraform && rgplr[iMax].zpq1.rgpq[ipq].mdIdle != iobjMaxTerraform))) {
                                    prod.iItem = rgplr[iMax].zpq1.rgpq[ipq].mdIdle;
                                    prod.cItem = rgplr[iMax].zpq1.rgpq[ipq].cQuan;
                                    pl.lpplprod->rgprod[iDst] = prod;
                                    iDst++;
                                }
                            }
                            if (iDst > 0) {
                                pl.lpplprod->iprodMac = iDst;
                                LpplFromId(pl.id)->lpplprod = pl.lpplprod;
                            } else {
                                FreePl((PL *)pl.lpplprod);
                                pl.lpplprod = NULL;
                            }
                        }
                    }
                    pl.iPlayer = iMax;
                    if (GetRaceStat(&rgplr[iMax], rsMajorAdv) == raMacintosh) {
                        pl.fStarbase = TRUE;
                        pl.isb = 0;
                        rglpshdefSB[iMax]->cExist++;
                        rglpshdefSB[iMax]->cBuilt++;
                    }
                    if (cPowerTot == 0 || cMax == 0) {
                        pl.rgwtMin[3] = rgcCol[iMax];
                    } else {
                        lPower = (int32_t)((int32_t)(cMax * (cPowerTot - lDefensePower)) / cPowerTot);
                        pl.rgwtMin[3] = (int32_t)((int32_t)(rgcCol[iMax] * lPower) / cMax);
                    }
                    if (c2nd > 0) {
                        pl.rgwtMin[3] = (int32_t)((int32_t)(pl.rgwtMin[3] * (cMax - c2nd)) / cMax);
                    }
                    if (pl.rgwtMin[3] < 1) {
                        pl.rgwtMin[3] = 1;
                    }
                }
            WritePlanet:
                if (pl.iPlayer != iplrNone && pl.fArtifact) {
                    pl.fArtifact = FALSE;
                    if (!game.fNoRandom) {
                        iTech = Random(6);
                        iBonus = Random(301) + 100;
                        if (pl.rgwtMin[3] < 10) {
                            iBonus = (int16_t)(LOWORD(pl.rgwtMin[3]) * iBonus) / 10;
                        }
                        FSendPlrMsg(pl.iPlayer, idmColonistsSettlingHaveFoundStrangeArtifactBoostin, gotoResearch, pl.id, iTech, iBonus, 0, 0, 0, 0);
                        rgplr[pl.iPlayer].rgResSpent[iTech] = rgplr[pl.iPlayer].rgResSpent[iTech] + iBonus;
                        if (game.fSlowTech) {
                            iBonus >>= 1;
                        }
                    }
                }
                FLookupPlanet(idWriteBack, &pl);
            }
        IncCur:;
        }
        cColDrop = 0;
    }
    return;
}

void HealShips() {
    int16_t pctShipHeal;
    int16_t dpHeal;
    PLANET *lppl;
    int16_t i;
    FLEET  *lpfl;
    SHDEF  *lpshdef;
    int16_t pct;
    int16_t ishdef;
    PLANET *lpplMac;

    pctShipHeal = 0;
    for (i = 0; i < cFleet; i++) {
        lpfl = rglpfl[i];
        if (!rglpfl[i])
            break;
        if (!lpfl->fDead && !lpfl->fNoHeal) {
            dpHeal = 0;
            pctShipHeal = 0;
            for (ishdef = 0; ishdef < 16; ishdef++) {
                if (lpfl->rgdv[ishdef].dp != 0) {
                    dpHeal = 1;
                }
                if (lpfl->rgcsh[ishdef] != 0) {
                    if (rglpshdef[lpfl->iPlayer][ishdef].hul.ihuldef == ihuldefSuperFuelXport) {
                        pctShipHeal = 50;
                    } else if (pctShipHeal < 5 && rglpshdef[lpfl->iPlayer][ishdef].hul.ihuldef == ihuldefFuelTransport) {
                        pctShipHeal = 25;
                    }
                }
            }
            if (dpHeal != 0) {
                if (!lpfl->fHereAllTurn) {
                    pct = 5;
                } else if (lpfl->idPlanet == idPlanetDeepSpace) {
                    pct = 10;
                } else {
                    lppl = LpplFromId(lpfl->idPlanet);
                    if (lppl->iPlayer != lpfl->iPlayer) {
                        if (lppl->iPlayer == iplrNone) {
                            pct = 15;
                        } else {
                            pct = 15;
                        }
                    } else if (lppl->fStarbase && !lppl->fNoHeal) {
                        lpshdef = rglpshdefSB[lppl->iPlayer] + lppl->isb;
                        if (LphuldefFromId(lpshdef->hul.ihuldef)->hul.wtCargoMax != 0) {
                            pct = 100;
                        } else {
                            pct = 40;
                        }
                    } else {
                        pct = 25;
                    }
                }
                if (GetRaceStat(&rgplr[lpfl->iPlayer], rsMajorAdv) == raDefend) {
                    pct *= 2;
                }
                pct += pctShipHeal;
                for (ishdef = 0; ishdef < 16; ishdef++) {
                    if (lpfl->rgdv[ishdef].dp != 0) {
                        if (lpfl->rgdv[ishdef].pctDp > (uint16_t)pct) {
                            lpfl->rgdv[ishdef].pctDp -= pct;
                        } else {
                            lpfl->rgdv[ishdef].dp = 0;
                        }
                    }
                }
            }
        }
    }
    lppl = lpPlanets;
    lpplMac = lpPlanets + cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->fStarbase && !lppl->fNoHeal) {
            if (GetRaceStat(&rgplr[lppl->iPlayer], rsMajorAdv) == raDefend) {
                pct = 75;
            } else {
                pct = 50;
            }
            if (lppl->pctDp != 0) {
                lpshdef = rglpshdefSB[lppl->iPlayer] + lppl->isb;
                if ((uint16_t)pct > lppl->pctDp) {
                    lppl->pctDp = 0;
                } else {
                    lppl->pctDp -= pct;
                }
            }
        }
    }
    return;
}

void AutoTerraform() {
    int16_t rgMax[3];
    int16_t rgp[16];
    PLANET *lppl;
    int16_t i;
    int16_t rgMin[3];
    int16_t rgCost[3];
    int16_t fTerra;
    PLANET *lpplMac;

    fTerra = FALSE;
    for (i = 0; i < game.cPlayer; i++) {
        rgp[i] = GetRaceStat(&rgplr[i], rsMajorAdv) == raTerra;
        if (rgp[i] != 0) {
            fTerra = TRUE;
        }
    }
    if (fTerra) {
        lppl = lpPlanets;
        lpplMac = lpPlanets + cPlanet;
        for (; lppl < lpplMac; lppl++) {
            if (lppl->iPlayer != iplrNone && rgp[lppl->iPlayer] != 0) {
                if (lppl->fStarbase && lppl->iPlayer == iplrNone) {
                    lppl->fStarbase = FALSE;
                }
                i = Random(3);
                if (rgplr[lppl->iPlayer].rgEnvVar[i] != envImmune && rgplr[lppl->iPlayer].rgEnvVar[i] != lppl->rgEnvVarOrig[i] && Random(10) == 0) {
                    if (lppl->rgwtMin[3] >= 1000 || Random(1000) < (int16_t)LOWORD(lppl->rgwtMin[3])) {
                        if (rgplr[lppl->iPlayer].rgEnvVar[i] < lppl->rgEnvVarOrig[i]) {
                            lppl->rgEnvVarOrig[i]--;
                        } else {
                            lppl->rgEnvVarOrig[i]++;
                        }
                        FSendPlrMsg2(lppl->iPlayer, idmEngineersHaveManagedImproveUnderlying1, lppl->id, lppl->id, i);
                    }
                }
                if (FCanTerraformLppl(lppl, rgMin, rgMax, rgCost, TRUE)) {
                    for (i = 0; i < 3; i++) {
                        if (rgMin[i] != envNone) {
                            lppl->rgEnvVar[i] = rgMin[i];
                        } else if (rgMax[i] != envNone) {
                            lppl->rgEnvVar[i] = rgMax[i];
                        }
                    }
                    i = PctPlanetDesirability(lppl, lppl->iPlayer);
                    FSendPlrMsg2(lppl->iPlayer, idmHasAutoTerraformedValue, lppl->id, lppl->id, i);
                }
            }
        }
    }
    return;
}

void RemoteTerraforming() {
    int16_t fHelp;
    int16_t iBest;
    int16_t pctCur;
    PLANET *lppl;
    int16_t ifl;
    FLEET  *lpfl;
    int16_t cDone;
    EnvType iEnv;
    int16_t cAllowed;
    int32_t ipct;
    int16_t pctNew;

    for (ifl = 0; ifl < cFleet; ifl++) {
        lpfl = rglpfl[ifl];
        if (!rglpfl[ifl])
            break;
        if (!lpfl->fDead && lpfl->idPlanet != idPlanetDeepSpace && lpPlanets[lpfl->idPlanet].iPlayer != iplrNone) {
            ipct = PctTerraFromLpfl(lpfl);
            if (ipct > 0) {
                lppl = lpPlanets + lpfl->idPlanet;
                fHelp = lpfl->iPlayer == lppl->iPlayer || rgplr[lpfl->iPlayer].rgmdRelation[lppl->iPlayer] == 1 || lpfl->iPlayer == lppl->iPlayer;
                if (fHelp || !lppl->fStarbase) {
                    pctCur = PctPlanetDesirability(lppl, lppl->iPlayer);
                    cDone = 0;
                    while (ipct-- > 0) {
                        iBest = IBestRemoteTerra(lppl, lpfl->iPlayer, fHelp);
                        if (iBest == 0)
                            break;
                        iEnv = abs(iBest) - 1;
                        cAllowed = lppl->rgEnvVar[iEnv] + (iBest <= 0 ? -1 : 1);
                        if (1 > (99 >= cAllowed ? cAllowed : 99)) {
                            cAllowed = 1;
                        } else if (99 < cAllowed) {
                            cAllowed = 99;
                        }
                        lppl->rgEnvVar[iEnv] = cAllowed;
                        cDone++;
                    }
                    pctNew = PctPlanetDesirability(lppl, lppl->iPlayer);
                    FSendPlrMsg(lpfl->iPlayer, (!fHelp ? idmHasDegradedValue : idmHasImprovedValue) + (pctCur == pctNew), lpfl->id | 0x8000, lpfl->id, lppl->id,
                                pctCur, pctNew, 0, 0, 0);
                    if (lpfl->iPlayer != lppl->iPlayer && pctNew != pctCur) {
                        FSendPlrMsg(lppl->iPlayer, !fHelp ? idmHasDegradedValue : idmHasImprovedValue, lppl->id, lpfl->id, lppl->id, pctCur, pctNew, 0, 0, 0);
                    }
                }
            }
        }
    }
    return;
}

int16_t FQueueColonistDrop(FLEET *lpfl, PLANET *lppl, int32_t cColonists) {
    int16_t  iColDrop;
    COLDROP *lpcdT;

    if (cColonists <= 0) {
        return TRUE;
    }
    iColDrop = 0;
    lpcdT = lpcd;
    for (; iColDrop < cColDrop && (lpcdT->idFleetSrc != lpfl->id || lpcdT->idPlanetDst != lppl->id); iColDrop++) {
        lpcdT++;
    }
    if (iColDrop == cColDrop) {
        if (cColDrop >= 1000) {
            return FALSE;
        }
        lpcdT->idFleetSrc = lpfl->id;
        lpcdT->idPlr = lpfl->iPlayer;
        lpcdT->idPlanetDst = lppl->id;
        lpcdT->cColonist = 0;
        lpcdT->fCanColonize = TRUE;
        cColDrop++;
    }
    lpcdT->cColonist += cColonists;
    return LOWORD(cColonists);
}

void UpdatePopulations() {
    int32_t lPopChg;
    PLANET *lppl;
    PLANET *lpplMac;
    int32_t lPopOld;
    int16_t fMac;

    lppl = lpPlanets;
    lpplMac = lpPlanets + cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->iPlayer == iplrNone || lppl->rgwtMin[3] == 0)
            goto NextPlanet;
        lPopChg = ChgPopFromPlanet(lppl, TRUE);
        if (lPopChg != 0 && lPopChg < 0 && lppl->rgwtMin[3] > 0) {
            lPopOld = lppl->rgwtMin[3] - lPopChg;
            if (PctPlanetDesirability(lppl, lppl->iPlayer) < 0) {
                FSendPlrMsg(lppl->iPlayer, idmPopulationHasDecreased, lppl->id, lppl->id, LOWORD(lPopOld), HIWORD(lPopOld), LOWORD(lppl->rgwtMin[3]),
                            HIWORD(lppl->rgwtMin[3]), 0, 0);
            } else {
                FSendPlrMsg(lppl->iPlayer, idmPopulationHasDecreasedColonistsDueOvercrowding, lppl->id, lppl->id, -LOWORD(lPopChg),
                            LOWORD((uint32_t)((uint32_t)-lPopChg >> 0x10)), 0, 0, 0, 0);
            }
        }
    NextPlanet:
        if (lppl->iPlayer != iplrNone && lppl->rgwtMin[3] == 0) {
            fMac = GetRaceStat(&rgplr[lppl->iPlayer], rsMajorAdv) == raMacintosh;
            FSendPlrMsg2(lppl->iPlayer, (lPopChg < 0 ? idmColonistsHaveDiedOffLongerControlPlanet : idmColonistsHaveJumpedShipLongerControlPlanet) + fMac,
                         lppl->id, lppl->id, 0);
            UninhabitPlanet(lppl);
        }
        if (lppl->iPlayer == iplrNone) {
            UninhabitPlanet(lppl);
        }
    }
    return;
}

void UpdateGuesses() {
    PLANET *lppl;
    float   pct;
    PLANET *lpplMac;
    int32_t l;

    lppl = lpPlanets;
    lpplMac = lpPlanets + cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->rgwtMin[3] != 0) {
            l = lppl->rgwtMin[3];
            if (GetRaceStat(&rgplr[lppl->iPlayer], rsMajorAdv) == raMacintosh) {
                l = 0;
            } else {
                l += -(int32_t)(l >> 3) + Random((int32_t)(l >> 2));
                l = (int32_t)(l >> 2);
                if (l > 4090) {
                    l = 4090;
                } else if (l < 1) {
                    l = 1;
                }
            }
            lppl->uPopGuess = LOWORD(l);
            if (lppl->cDefenses == 0) {
                lppl->uDefGuess = 0;
            } else {
                CalcPctSurvive(lppl, &pct, NULL);
                l = 100 - Sf80ToI32((Sf80Add(Sf80Mul(Sf80From32(pct), Sf80From64(100.0)), Sf80From64(0.5)))) + 4;
                l = (int32_t)(l / 6);
                if (l < 1) {
                    l = 1;
                } else if (l > 15) {
                    l = 15;
                }
                lppl->uDefGuess = LOWORD(l);
            }
        } else {
            lppl->uGuesses = 0;
        }
    }
    return;
}

void MineMinerals() {
    int32_t rglQuan[3];
    PLANET *lppl;
    PLANET *lpplMac;

    lppl = lpPlanets;
    lpplMac = lpPlanets + cPlanet;
    for (; lppl < lpplMac; lppl++) {
        EstMineralsMined(lppl, rglQuan, -1, TRUE);
    }
    return;
}

void MeteorStrike() {
    int16_t rgEnv[3];
    int16_t iT;
    int32_t rgQuan[4];
    int16_t iSize;
    PLANET *lppl;
    int16_t rgAffect[3];
    int16_t i;
    int16_t iConc;
    int16_t j;

    if (Random(20) == 0) {
        lppl = lpPlanets + Random(cPlanet);
        if ((lppl->iPlayer == iplrNone || lppl->rgwtMin[3] <= 50 || game.turn >= 20) && game.turn >= 10) {
            iSize = Random(4);
            for (i = 0; i < 3; i++) {
                rgEnv[i] = i;
            }
            for (i = 0; i < 3; i++) {
                j = Random(3);
                iT = rgEnv[i];
                rgEnv[i] = rgEnv[j];
                rgEnv[j] = iT;
            }
            for (i = 0; i < 3; i++) {
                rgAffect[i] = i;
                rgQuan[i] = (int16_t)(Random(250) + 50);
            }
            for (i = 0; i < 2; i++) {
                j = Random(3 - i) + i;
                iT = rgAffect[i];
                rgAffect[i] = rgAffect[j];
                rgAffect[j] = iT;
            }
            for (i = 0; i < game.cPlayer; i++) {
                FSendPlrMsg(i,
                            (i == lppl->iPlayer && GetRaceStat(&rgplr[i], rsMajorAdv) != raMacintosh ? idmSmallCometHasCrashedPlanetKilling25
                                                                                                     : idmSmallCometHasCrashedBringingNewMinerals) +
                                iSize,
                            lppl->id, lppl->id, rgEnv[0], rgEnv[1], rgEnv[2], 0, 0, 0);
            }
            if (lppl->iPlayer != iplrNone && GetRaceStat(&rgplr[lppl->iPlayer], rsMajorAdv) != raMacintosh) {
                lppl->rgwtMin[3] -= (int32_t)(lppl->rgwtMin[3] * (int16_t)(20 * iSize + 25)) / 100;
            }
            for (i = 0; i <= iSize && i < 3; i++) {
                rgQuan[rgAffect[i]] = rgQuan[rgAffect[i]] + (int16_t)(Random(17000) + 3000);
                iConc = lppl->rgMinConc[rgAffect[i]];
                iConc += Random(50) + 50;
                if (iSize == 3) {
                    iConc += Random(15) + 15;
                }
                if (iConc > 200) {
                    iConc = 200;
                }
                lppl->rgMinConc[rgAffect[i]] = iConc;
            }
            for (i = 0; i < 3; i++) {
                lppl->rgwtMin[i] += (int32_t)(rgQuan[i] >> 4);
            }
            for (i = 0; i < 3 && i <= iSize; i++) {
                iT = Random(3) + 3;
                if (iSize == 3) {
                    iT += Random(3) + 3;
                }
                if (Random(2) != 0) {
                    iT = -iT;
                }
                j = lppl->rgEnvVar[i] + iT;
                if (j < 1) {
                    j = 1;
                } else if (j > 99) {
                    j = 99;
                }
                lppl->rgEnvVar[i] = j;
                j = lppl->rgEnvVarOrig[i] + iT;
                if (j < 1) {
                    j = 1;
                } else if (j > 99) {
                    j = 99;
                }
                lppl->rgEnvVarOrig[i] = j;
            }
            TossNonAutoBuildItems(lppl);
        }
    }
    return;
}

void TossNonAutoBuildItems(PLANET *lppl) {
    int16_t iDst;
    int16_t iSrc;

    if (lppl->lpplprod) {
        iDst = 0;
        for (iSrc = 0; iSrc < lppl->lpplprod->iprodMac; iSrc++) {
            if (lppl->lpplprod->rgprod[iSrc].grobj == grobjPlanet && lppl->lpplprod->rgprod[iSrc].iItem < mdIdleFactory) {
                if (iSrc > iDst) {
                    lppl->lpplprod->rgprod[iDst] = lppl->lpplprod->rgprod[iSrc];
                }
                iDst++;
            }
        }
        if (iDst > 0) {
            lppl->lpplprod->iprodMac = iDst;
        } else {
            FreePl((PL *)lppl->lpplprod);
            lppl->lpplprod = NULL;
        }
    }
    return;
}

void PlanetaryClimateChange() {
    int16_t iT;
    PLANET *lppl;
    int16_t i;
    int16_t j;

    if (Random(20) == 0) {
        lppl = lpPlanets + Random(cPlanet);
        if (lppl->iPlayer == iplrNone || lppl->rgwtMin[3] <= 50 || game.turn >= 20) {
            i = Random(3);
            if (lppl->iPlayer != iplrNone) {
                FSendPlrMsg2(lppl->iPlayer, idmFundamentalChangesEnvironmentHavePermanentlyAlte, lppl->id, lppl->id, i);
            }
            iT = Random(3) + 3;
            if (iT == 3) {
                iT += Random(3) + 3;
            }
            if (Random(2) != 0) {
                iT = -iT;
            }
            j = lppl->rgEnvVar[i] + iT;
            if (j < 1) {
                j = 1;
            } else if (j > 99) {
                j = 99;
            }
            lppl->rgEnvVar[i] = j;
            j = lppl->rgEnvVarOrig[i] + iT;
            if (j < 1) {
                j = 1;
            } else if (j > 99) {
                j = 99;
            }
            lppl->rgEnvVarOrig[i] = j;
            TossNonAutoBuildItems(lppl);
        }
    }
    return;
}

void DiscoverNewMinerals() {
    PLANET *lppl;
    int16_t i;

    if (Random(15 - game.mdSize) == 0) {
        lppl = lpPlanets + Random(cPlanet);
        if (game.turn >= 10) {
            i = Random(3);
            if (lppl->iPlayer != iplrNone) {
                FSendPlrMsg(lppl->iPlayer, idmSurveyorsHaveDiscoveredPreviouslyUnknownDepositS, lppl->id, lppl->id, i, 0, 0, 0, 0, 0);
            }
            if (lppl->rgMinConc[i] < 180) {
                lppl->rgMinConc[i] += Random(15) + 5;
            }
        }
    }
    return;
}

void MysteryTrader() {
    int16_t     iSrc;
    int16_t     cRand;
    int16_t     i;
    THING      *lpth;
    GrbitTrader grbitTrader;
    int16_t     rgC[4];

    if (game.turn >= 40) {
        if ((uint32_t)game.turn % 100 == 71) {
            cRand = 2;
        } else if ((uint32_t)game.turn % 100 == 33) {
            cRand = 3;
        } else if ((game.turn & 0x7f) == 0x31) {
            cRand = 4;
        } else {
            if (game.turn & 1) {
                return;
            }
            cRand = 7;
        }
        if (Random(cRand) == 0) {
            lpth = LpthNew(0, ithMysteryTrader);
            if (lpth) {
                lpth->tht.iWarp = Random(5) + 8;
                for (i = 0; i < 4; i += 2) {
                    rgC[i] = Random(400 * game.mdSize + 361) + 1020;
                }
                if (Random(2) == 0) {
                    rgC[1] = 1020;
                    rgC[3] = 400 * game.mdSize + 1380;
                } else {
                    rgC[1] = 400 * game.mdSize + 1380;
                    rgC[3] = 1020;
                }
                iSrc = Random(2);
                lpth->pt.x = rgC[iSrc];
                lpth->pt.y = rgC[iSrc == 0];
                lpth->tht.ptDest.x = rgC[iSrc + 2];
                lpth->tht.ptDest.y = rgC[(iSrc == 0) + 2];
                if (game.turn < 100) {
                    cRand = 5;
                } else if (game.turn < 250) {
                    cRand = 3;
                } else {
                    cRand = 2;
                }
                if (lpth->tht.iWarp <= 9) {
                    cRand++;
                } else if (lpth->tht.iWarp >= 11) {
                    cRand--;
                }
                if (Random(10) < cRand) {
                    if (Random(6) == 0) {
                        lpth->tht.grbitTrader = grbitTraderLifeboat;
                    } else {
                        lpth->tht.grbitTrader = grbitTraderNone;
                    }
                } else {
                    grbitTrader = 1 << Random(13);
                    switch (grbitTrader) {
                    case grbitTraderTorp:
                    case grbitTraderBeam:
                    case grbitTraderGenesis:
                    case grbitTraderJumpgate:
                        grbitTrader = 1 << Random(13);
                        if (((game.turn < 120 && grbitTrader == grbitTraderBeam) || (game.turn < 150 && grbitTrader == grbitTraderGenesis) ||
                             (game.turn < 180 && grbitTrader == grbitTraderJumpgate)) &&
                            Random(2) != 0) {
                            grbitTrader = grbitTraderNone;
                        }
                    }
                    lpth->tht.grbitTrader = grbitTrader;
                }
                for (i = 0; i < game.cPlayer; i++) {
                    FSendPlrMsg2(i, idmMysteriousTradingVesselBroadcastingProposalHasDe, gotoThing, lpth->idFull, 0);
                }
            }
        }
    }
    return;
}

void UpdatePlayerScores() {
    int32_t  lScoreTot;
    int16_t  cFirst;
    SCORE    score;
    int16_t  cDead;
    int16_t  c;
    int16_t  i;
    uint8_t  rgcCond[16];
    uint16_t wWinners2;
    int32_t  rglScore[16];
    int16_t  iScoreMax;
    int16_t  j;
    uint16_t wWinners;
    int16_t  imsg;
    int32_t  lScore2nd;
    int32_t  lScoreMax;

    cDead = 0;
    cFirst = 0;
    iScoreMax = 0;
    lScore2nd = 0;
    lScoreTot = 0;
    gd.fGameOverMan = FALSE;
    memset(rgcCond, 0, 16);
    for (i = 0; i < game.cPlayer; i++) {
        rglScore[i] = CalcPlayerScore(i, &score);
        vlprgScoreX[i].score = score;
        vlprgScoreX[i].iPlayer = i;
        vlprgScoreX[i].fValid = TRUE;
        vlprgScoreX[i].grbitVC = 0;
        lScoreTot += rglScore[i];
        if (score.cPlanet == 0 && score.rgcsh[0] == 0 && score.rgcsh[1] == 0 && score.rgcsh[2] == 0 && !rgplr[i].fDead) {
            rgplr[i].fDead = TRUE;
            for (j = 0; j < game.cPlayer; j++) {
                if (j != i) {
                    FSendPrependedPlrMsg(j, idmTracesHaveEliminatedGalaxyMayRestPeace, gotoScore, i | 0x30, 0, 0, 0, 0, 0, 0);
                }
            }
        }
        if (score.cPlanet >= LMulDiv(cPlanet, GetVCVal(&game, vcOwnsPercentPlanets, FALSE), 100)) {
            vlprgScoreX[i].grbitVC |= 1;
            if (GetVCCheck(&game, vcOwnsPercentPlanets) != 0) {
                rgcCond[i]++;
            }
        }
        if ((int32_t)((uint32_t)(score.rgcsh[2] & 0x1fff) << (score.rgcsh[2] >> 0xd << 1)) >= GetVCVal(&game, vcOwnsCapitalShips, FALSE)) {
            vlprgScoreX[i].grbitVC |= 0x20;
            if (GetVCCheck(&game, vcOwnsCapitalShips) != 0) {
                rgcCond[i]++;
            }
        }
        if (rglScore[i] >= GetVCVal(&game, vcExceedsScore, FALSE)) {
            vlprgScoreX[i].grbitVC |= 4;
            if (GetVCCheck(&game, vcExceedsScore) != 0) {
                rgcCond[i]++;
            }
        }
        c = 0;
        for (j = 0; j < 6; j++) {
            if (rgplr[i].rgTech[j] >= GetVCVal(&game, vcAttainsTechLevel, FALSE)) {
                c++;
            }
        }
        if (c >= GetVCVal(&game, vcAttainsTechFields, FALSE)) {
            vlprgScoreX[i].grbitVC |= 2;
            if (GetVCCheck(&game, vcAttainsTechLevel) != 0) {
                rgcCond[i]++;
            }
        }
        if ((int32_t)(score.cResources / 1000) >= GetVCVal(&game, vcProductionCapacity, FALSE)) {
            vlprgScoreX[i].grbitVC |= 0x10;
            if (GetVCCheck(&game, vcProductionCapacity) != 0) {
                rgcCond[i]++;
            }
        }
    }
    if (game.cPlayer == 1) {
        vlprgScoreX->iRank = 1;
    } else {
        for (i = 0; i < game.cPlayer; i++) {
            if (rgplr[i].fDead) {
                cDead++;
            }
            rgplr[i].wScore = 1;
            for (j = 0; j < game.cPlayer; j++) {
                if (rglScore[j] > rglScore[i]) {
                    rgplr[i].wScore++;
                }
            }
            if (rgplr[i].wScore == 1) {
                iScoreMax = i;
                lScoreMax = rglScore[i];
                cFirst++;
            } else if (rgplr[i].wScore == 2) {
                lScore2nd = rglScore[i];
            }
        }
        if (cFirst > 1) {
            lScore2nd = lScoreMax;
        }
        for (i = 0; i < game.cPlayer; i++) {
            vlprgScoreX[i].turn = rgplr[i].wScore;
        }
        if ((int16_t)game.turn >= GetVCVal(&game, vcHighestScoreAfterYears, FALSE) && cFirst == 1) {
            vlprgScoreX[iScoreMax].grbitVC |= 0x40;
            if (GetVCCheck(&game, vcHighestScoreAfterYears) != 0) {
                rgcCond[iScoreMax]++;
            }
        }
        if (cDead + 1 >= game.cPlayer) {
            gd.fGameOverMan = TRUE;
            if (!rgplr[iScoreMax].fDead) {
                FSendPrependedPlrMsg(iScoreMax, idmTracesEveryOtherRivalHaveEliminatedGalaxy, gotoScore, 0, 0, 0, 0, 0, 0, 0);
            }
            for (i = 0; i < game.cPlayer; i++) {
                if (i != iScoreMax) {
                    FSendPrependedPlrMsg(i, idmDeadPlanetsHaveOverrunSpaceshipsDefeated, gotoScore, 0, 0, 0, 0, 0, 0, 0);
                }
            }
        } else {
            if (lScoreMax >= (int32_t)(lScore2nd * (int16_t)(GetVCVal(&game, vcExceedsSecondPlaceBy, FALSE) + 100)) / 100) {
                vlprgScoreX[iScoreMax].grbitVC |= 8;
                if (GetVCCheck(&game, vcExceedsSecondPlaceBy) != 0) {
                    rgcCond[iScoreMax]++;
                }
            }
            if (game.turn >= (uint16_t)GetVCVal(&game, vcMinYearsBeforeWin, FALSE)) {
                wWinners = 0;
                j = GetVCVal(&game, vcMeetsNumCriteria, FALSE);
                if (j >= 1) {
                    for (i = game.cPlayer - 1; i >= 0; i--) {
                        wWinners *= 2;
                        if (rgcCond[i] >= j) {
                            vlprgScoreX[i].fWinner = TRUE;
                            wWinners |= 1;
                        }
                    }
                    if (wWinners != 0) {
                        gd.fGameOverMan = TRUE;
                    }
                }
                if (gd.fGameOverMan) {
                    i = 0;
                    j = 1;
                    while (i < game.cPlayer) {
                        wWinners2 = wWinners;
                        if (rgplr[i].fDead) {
                            imsg = idmDeadPlanetsHaveOverrunSpaceshipsDefeated;
                        } else if (!(j & wWinners)) {
                            imsg = idmForcesHaveDeclaredWinnerGameAdvisedAccept;
                        } else if ((j ^ wWinners) != 0) {
                            imsg = idmAlongHaveDeclaredWinnersGameMayContinue;
                            wWinners2 &= ~j;
                        } else {
                            imsg = idmHaveDeclaredWinnerGameMayContinuePlay;
                        }
                        FSendPrependedPlrMsg(i, imsg, gotoScore, wWinners2, 0, 0, 0, 0, 0, 0);
                        i++;
                        j *= 2;
                    }
                }
            }
        }
    }
    return;
}

void CreateBackupDir() {
    char *pchT;

    strcpy(szBackup, szBase);
    pchT = strrchr(szBackup, chDirSep);
    if (!pchT) {
        pchT = szBackup;
    } else {
        pchT++;
    }
    *pchT = 0;
    if (vcBackupDirs <= 1) {
        strcpy(pchT, "backup");
    } else if (vcBackupDirs <= 99) {
        CchSprintf(pchT, "backup%d", (uint32_t)game.turn % vcBackupDirs);
    } else {
        CchSprintf(pchT, "backup.%03d", (uint32_t)game.turn % vcBackupDirs);
    }
    MakeDir(szBackup);
    strcat(szBackup, szDirSep);
    return;
}

int16_t FPacketDecay(THING *lpth, int16_t pctRate) {
    uint16_t iRateMin;
    int16_t  iRate;
    int16_t  i;
    uint16_t wDecay;
    int32_t  lDecay;

    if (lpth->thp.iDecayRate <= decayNone) {
        return FALSE;
    }
    switch (lpth->thp.iDecayRate) {
    case decay10Pct:
        iRate = 10;
        break;
    case decay25Pct:
        iRate = 25;
        break;
    case decay50Pct:
        iRate = 50;
    }
    if (GetRaceStat(&rgplr[lpth->iplr], rsMajorAdv) == raMassAccel) {
        iRate /= 2;
        iRateMin = 5;
    } else {
        iRateMin = 10;
    }
    lDecay = 0;
    for (i = 0; i < 3; i++) {
        if (lpth->thp.rgwtMin[i] != 0) {
            wDecay = LOWORD((int32_t)((uint32_t)(lpth->thp.rgwtMin[i] * iRate) * pctRate) / 10000);
            wDecay = iRateMin <= wDecay ? wDecay : iRateMin;
            if (lpth->thp.rgwtMin[i] <= (int16_t)wDecay) {
                wDecay = lpth->thp.rgwtMin[i];
            }
            lpth->thp.rgwtMin[i] -= wDecay;
            lDecay += lpth->thp.rgwtMin[i];
        }
    }
    if (lDecay == 0) {
        FreeLpth(lpth);
        return TRUE;
    }
    lpth->thp.wtMax = LOWORD((int32_t)((lDecay + 9) / 10));
    return FALSE;
}

void ThingDecay() {
    THING   *lpthMac;
    int32_t  pctDecay;
    int16_t  i;
    int16_t  ifl;
    FLEET   *lpfl;
    THING   *lpth;
    uint16_t wDecay;
    int32_t  lDecay;
    int16_t  fMineExpert;
    int32_t  dy;
    int32_t  dx;

    for (ifl = 0; ifl < cFleet; ifl++) {
        lpfl = rglpfl[ifl];
        if (!rglpfl[ifl])
            break;
        lpfl->fBombed = FALSE;
    }
    lpth = lpThings;
    lpthMac = lpThings + cThing;
    for (; lpth < lpthMac; lpth++) {
        if (lpth->ith == ithMineralPacket) {
            if (lpth->thp.iWarp == 0) {
                if (lpth->thp.fMoved) {
                    lpth->thp.fMoved = FALSE;
                    continue;
                }
                lDecay = 0;
                for (i = 0; i < 3; i++) {
                    if (lpth->thp.rgwtMin[i] != 0) {
                        wDecay = 10 <= lpth->thp.rgwtMin[i] / 10 ? lpth->thp.rgwtMin[i] / 10 : 10;
                        lpth->thp.rgwtMin[i] -= wDecay;
                        if (lpth->thp.rgwtMin[i] < 0) {
                            lpth->thp.rgwtMin[i] = 0;
                        }
                        lDecay += lpth->thp.rgwtMin[i];
                    }
                }
                if (lDecay == 0) {
                    FreeLpth(lpth);
                LFixUpLpth:
                    lpth--;
                    lpthMac--;
                    continue;
                } else {
                    lpth->thp.wtMax = LOWORD((int32_t)((lDecay + 9) / 10));
                    continue;
                }
            } else if (FPacketDecay(lpth, 100)) {
                goto LFixUpLpth;
            }
            continue;
        } else if (lpth->ith == ithMinefield) {
            fMineExpert = GetRaceStat(&rgplr[lpth->iplr], rsMajorAdv) == raMines;
            if (lpth->thm.fDetonate) {
                lDecay = lpth->thm.cMines;
                for (ifl = 0; ifl < cFleet; ifl++) {
                    lpfl = rglpfl[ifl];
                    if (!rglpfl[ifl])
                        break;
                    if (!lpfl->fDead) {
                        dx = (int16_t)(lpfl->pt.x - lpth->pt.x);
                        dy = (int16_t)(lpfl->pt.y - lpth->pt.y);
                        /* The original set fBombed even when the fleet was
                           immune to this field, sparing it from every other
                           detonating field. */
                        if (!lpfl->fBombed && (uint32_t)(dx * dx) + (uint32_t)(dy * dy) <= lDecay && !FTravelThroughMineFields(lpfl, NULL, lpth)) {
                            lpfl->fBombed = TRUE;
                        }
                    }
                }
            }
            pctDecay = (int16_t)(((fMineExpert == 0) * 3 + 1) * CPlanetsInCircle(lpth->pt, lpth->thm.cMines) + 2);
            if (pctDecay > 50) {
                pctDecay = 50;
            }
            if (lpth->thm.fDetonate) {
                pctDecay += 25;
            }
            lDecay = (int32_t)(lpth->thm.cMines * pctDecay) / 100;
            if (lDecay < pctDecay) {
                lDecay = pctDecay;
            }
            if (lpth->thm.iType != mineSpeedBump) {
                lDecay = 10 <= lDecay ? lDecay : 10;
            }
            if (lDecay >= lpth->thm.cMines) {
                FreeLpth(lpth);
                lpth--;
                lpthMac--;
            } else {
                lpth->thm.cMines -= lDecay;
            }
        }
    }
    return;
}

void UnmarkMineFields() {
    THING *lpthMac;
    THING *lpth;

    lpth = lpThings;
    lpthMac = lpThings + cThing;
    for (; lpth < lpthMac; lpth++) {
        if (lpth->ith == ithMinefield) {
            lpth->thm.grbitPlrNow = 0;
        }
    }
    return;
}

void SweepForMines() {
    int16_t  iplr;
    THING   *lpthMac;
    POINT16  pt;
    int32_t  dy;
    int32_t  lCur;
    PLANET  *lppl;
    int16_t  ifl;
    FLEET   *lpfl;
    THING   *lpth;
    int32_t  cMineCur;
    int32_t  dx;
    int32_t  cMine;
    uint16_t grbitPlr;
    PLANET  *lpplMac;

    for (ifl = 0; ifl < cFleet; ifl++) {
        lpfl = rglpfl[ifl];
        if (!rglpfl[ifl])
            break;
        cMine = CMineSweepFromLpfl(lpfl);
        if (cMine > 0 && !lpfl->fDead) {
            iplr = lpfl->iplr;
            grbitPlr = 1 << lpfl->iplr;
            pt = lpfl->pt;
            lpth = lpThings;
            lpthMac = lpThings + cThing;
            for (; lpth < lpthMac; lpth++) {
                if (lpth->ith == ithMinefield && lpth->iplr != iplr && FAttackPlayer(lpfl, lpth->iplr)) {
                    dx = (int16_t)(pt.x - lpth->pt.x);
                    dy = (int16_t)(pt.y - lpth->pt.y);
                    lCur = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                    if (lpth->thm.cMines >= (uint32_t)(dx * dx) + (uint32_t)(dy * dy)) {
                        if (lpth->thm.iType == mineSpeedBump) {
                            cMineCur = (int32_t)(cMine / 3);
                        } else {
                            cMineCur = cMine;
                        }
                        if (cMineCur < 2) {
                            cMineCur = 2;
                        }
                        if (lpth->thm.cMines - cMineCur < lCur - 1) {
                            cMineCur = lpth->thm.cMines - lCur + 1;
                        }
                        if (cMineCur > lpth->thm.cMines) {
                            cMineCur = lpth->thm.cMines;
                        }
                        FSendPlrMsg(lpfl->iPlayer, idmHasSweptMinesMineField, 0x8000 | lpfl->id, lpfl->id, LOWORD(cMineCur), HIWORD(cMineCur), lpth->iplr,
                                    lpth->thm.iType, lpth->pt.x, lpth->pt.y);
                        FSendPlrMsg(lpth->iplr, idmSomeoneHasSweptMinesMineField, gotoThing, lpth->idFull, LOWORD(cMineCur), HIWORD(cMineCur), lpth->thm.iType,
                                    lpth->pt.x, lpth->pt.y, 0);
                        lpth->thm.cMines -= cMineCur;
                        if (lpth->thm.cMines <= 0) {
                            FreeLpth(lpth);
                            lpth--;
                            lpthMac--;
                        } else {
                            lpth->thm.grbitPlr |= 1 << lpfl->iPlayer;
                        }
                    }
                }
            }
        }
    }
    lppl = lpPlanets;
    lpplMac = lpPlanets + cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->fStarbase && lppl->iPlayer != iplrNone) {
            cMine = CMineSweepFromLphul(&rglpshdefSB[lppl->iPlayer][lppl->isb].hul);
            if (cMine > 0) {
                iplr = lppl->iPlayer;
                grbitPlr = 1 << lppl->iPlayer;
                pt = rgptPlan[lppl->id];
                lpth = lpThings;
                lpthMac = lpThings + cThing;
                for (; lpth < lpthMac; lpth++) {
                    if (lpth->ith == ithMinefield && lpth->iplr != iplr && iplr != lpth->iplr && rgplr[iplr].rgmdRelation[lpth->iplr] != 1) {
                        dx = (int16_t)(pt.x - lpth->pt.x);
                        dy = (int16_t)(pt.y - lpth->pt.y);
                        lCur = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                        if (lpth->thm.cMines >= (uint32_t)(dx * dx) + (uint32_t)(dy * dy)) {
                            if (lpth->thm.iType == mineSpeedBump) {
                                cMineCur = (int32_t)(cMine / 3);
                            } else {
                                cMineCur = cMine;
                            }
                            if (cMineCur < 2) {
                                cMineCur = 2;
                            }
                            if (lpth->thm.cMines - cMineCur < lCur - 1) {
                                cMineCur = lpth->thm.cMines - lCur + 1;
                            }
                            if (cMineCur > lpth->thm.cMines) {
                                cMineCur = lpth->thm.cMines;
                            }
                            FSendPlrMsg(iplr, idmStarbaseHasSweptMinesMineField, lppl->id, lppl->id, LOWORD(cMineCur), HIWORD(cMineCur), lpth->iplr,
                                        lpth->thm.iType, lpth->pt.x, lpth->pt.y);
                            FSendPlrMsg(lpth->iplr, idmSomeoneHasSweptMinesMineField, gotoThing, lpth->idFull, LOWORD(cMineCur), HIWORD(cMineCur),
                                        lpth->thm.iType, lpth->pt.x, lpth->pt.y, 0);
                            lpth->thm.cMines -= cMineCur;
                            if (lpth->thm.cMines <= 0) {
                                FreeLpth(lpth);
                                lpth--;
                                lpthMac--;
                            } else {
                                lpth->thm.grbitPlr |= 1 << lppl->iPlayer;
                            }
                        }
                    }
                }
            }
        }
    }
    return;
}

void BreedColonistsInTransit() {
    int16_t fNoBreeders;
    char    grfBreeder[16];
    int32_t lColGain;
    PLANET *lppl;
    int16_t ifl;
    FLEET  *lpfl;
    int16_t i;
    int32_t lColGainAct;

    fNoBreeders = TRUE;
    for (i = 0; i < game.cPlayer; i++) {
        if ((grfBreeder[i] = GetRaceStat(&rgplr[i], rsMajorAdv) == raDefend) == 1) {
            fNoBreeders = FALSE;
        }
    }
    if (!fNoBreeders) {
        for (ifl = 0; ifl < cFleet; ifl++) {
            lpfl = rglpfl[ifl];
            if (!rglpfl[ifl])
                break;
            if (!lpfl->fDead && grfBreeder[lpfl->iPlayer] != 0 && lpfl->rgwtMin[3] != 0) {
                lColGain = (int32_t)(lpfl->rgwtMin[3] * (int16_t)rgplr[lpfl->iPlayer].pctIdealGrowth) / 200;
                if (lColGain <= 0) {
                    if (Random(3) != 0)
                        continue;
                    lColGain = 1;
                }
                lColGainAct = ChgCargo(grobjFleet, lpfl->id, Colonists, lColGain, NULL);
                if (lColGainAct > 0) {
                    FSendPlrMsg2(lpfl->iPlayer, idmColonistsHaveMadeGoodUseTimeIncreasing, lpfl->id | 0x8000, lpfl->id, LOWORD(lColGainAct));
                }
                if (lColGainAct < lColGain && lpfl->idPlanet != idPlanetDeepSpace) {
                    lppl = LpplFromId(lpfl->idPlanet);
                    if (lppl && lppl->iPlayer == lpfl->iPlayer) {
                        lColGain -= lColGainAct;
                        lppl->rgwtMin[3] += lColGain;
                        FSendPlrMsg(lpfl->iPlayer, idmBreedingActivitiesHaveOverflowedLivingSpaceColon, lpfl->id | 0x8000, lpfl->id, LOWORD(lColGain),
                                    HIWORD(lColGain), lpfl->idPlanet, 0, 0, 0);
                    }
                }
            }
        }
    }
    return;
}

void UpdateResearchStatus(int16_t fUsePool) {
    mdPartAvail  mdAvail;
    int16_t      fRedoItAll;
    int16_t      iTechCur;
    int16_t      fUsePoolOrig;
    int16_t      iTechNext;
    int16_t      iT;
    int16_t      iItem;
    int16_t      fGeneral;
    int16_t      fChgNow;
    int16_t      i;
    int16_t      ibitCur;
    int32_t      rglFieldSpent[6];
    HullSlotType grbitCur;
    int16_t      cPlrAlive;
    int32_t      lSpent;
    PART         part;
    int32_t      l;
    int16_t      iTT;
    int32_t      l15pct;
    int16_t      iTechNext2;
    char         TechLevel;
    int16_t      jj;
    int16_t      iGoto;
    MessageId    idm;

    cPlrAlive = 0;
    fUsePoolOrig = fUsePool;
    if (fUsePool) {
        for (i = 0; i < 6; i++) {
            rglFieldSpent[i] = 0;
        }
    }
    for (i = 0; i < game.cPlayer; i++) {
        fUsePool = fUsePoolOrig;
        fGeneral = GetRaceGrbit(&rgplr[i], ibitRaceGeneralizedResearch);
        iTechCur = rgplr[i].iTechNow;
        iTechNext = rgplr[i].iTechNext;
        idPlayer = i;
        if (!rgplr[i].fDead) {
            cPlrAlive++;
        }
    RedoItAll:
        fRedoItAll = FALSE;
        for (iT = 0; iT < 6; iT++) {
            lSpent = rgplr[i].rgResSpent[iT];
            fChgNow = FALSE;
            if (game.fSlowTech) {
                lSpent = (int32_t)(lSpent * 2);
            }
            if (iT == iTechCur && fUsePool && fGeneral < 2) {
                if (fGeneral) {
                    fRedoItAll = TRUE;
                    fGeneral = 2;
                    lSpent += (int32_t)((rgplr[i].lResLastYear + 1) / 2);
                    rglFieldSpent[iT] += (int32_t)((rgplr[i].lResLastYear + 1) / 2);
                    for (iTT = 0; iTT < 6; iTT++) {
                        if (iTT != iT) {
                            l15pct = (int32_t)((uint32_t)(rgplr[i].lResLastYear * 3) + 19) / 20;
                            if (game.fSlowTech) {
                                rgplr[i].rgResSpent[iTT] += (int32_t)(l15pct / 2);
                            } else {
                                rgplr[i].rgResSpent[iTT] += l15pct;
                            }
                            rglFieldSpent[iTT] += l15pct;
                        }
                    }
                } else {
                    lSpent += rgplr[i].lResLastYear;
                    rglFieldSpent[iT] += rgplr[i].lResLastYear;
                }
            }
        CheckForBreakthrough:
            if (rgplr[i].rgTech[iT] < 26 && (!rgplr[i].fCrippled || rgplr[i].rgTech[iT] < 10)) {
                l = GetTechLevelCost(iT, rgplr[i].rgTech[iT] + 1, i);
                if (l <= lSpent && !(rgplr[i].rgTech[iT] >= 26 && (rgplr[i].fCrippled || rgplr[i].rgTech[iT] >= 26))) {
                    iTechNext2 = iTechCur;
                    lSpent -= l;
                    rgplr[i].rgTech[iT]++;
                    TechLevel = rgplr[i].rgTech[iT];
                    if (TechLevel == 26 && iTechNext == 6) {
                        iTechNext = 7;
                    }
                    if (iTechCur == iT && iTechNext != 6) {
                        if (iTechNext != 7) {
                            iTechNext2 = iTechNext;
                        } else {
                            iTechNext2 = 0;
                            for (jj = 1; jj < 6; jj++) {
                                if (rgplr[i].rgTech[jj] < LOBYTE((int16_t)(((uint16_t)iTechNext2 & 0xff00) | ((uint16_t)rgplr[i].rgTech[iTechNext2] & 0xff)))) {
                                    iTechNext2 = jj;
                                }
                            }
                        }
                        fChgNow = TRUE;
                    }
                    FSendPlrMsg(i, !fGeneral ? idmScientistsHaveCompletedResearchTechLevelWill : idmScientistsHaveCompletedResearchTechLevelPrimary,
                                gotoResearch, TechLevel, iT, iTechNext2, 0, 0, 0, 0);
                    grbitCur = hstEngine;
                    ibitCur = 0;
                    while (grbitCur != hstNone) {
                        if (grbitCur & (hstEngine | hstScanner | hstShield | hstArmor | hstBeam | hstTorp | hstBomb | hstMining | hstMines | hstSpecialSB |
                                        hstSBHull | hstSpecialE | hstSpecialM | hstTerra | hstHull | hstPlanetary)) {
                            iItem = 0;
                            part.hs.grhst = grbitCur;
                            while (1) {
                                part.hs.iItem = iItem;
                                mdAvail = FLookupPart(&part);
                                if (mdAvail == mdPartAvailInvalid)
                                    break;
                                if (mdAvail == mdPartAvailAvailable &&
                                    rgplr[i].rgTech[iT] == LOBYTE((int16_t)(((uint16_t)iT & 0xff00) | ((uint16_t)part.pcom->rgTech[iT] & 0xff)))) {
                                    if (grbitCur == hstSBHull) {
                                        idm = idmRecentBreakthroughHasAlsoGivenHullDesign;
                                        iGoto = -3;
                                    } else if (grbitCur == hstHull) {
                                        idm = idmRecentBreakthroughHasAlsoGivenHullType;
                                        iGoto = -3;
                                    } else if (grbitCur == hstTerra && GetRaceGrbit(&rgplr[i], ibitRaceTT) != 0 &&
                                               (iItem == iterraGravityTerraform3 || iItem == iterraTempTerraform3 || iItem == iterraRadiationTerraform3)) {
                                        iItem++;
                                        continue;
                                    } else {
                                        if (grbitCur == hstPlanetary && iItem >= iplanetarySDI && iItem <= iplanetaryNeutronShield) {
                                            idm = idmRecentBreakthroughHasAlsoTaughtHowBuild;
                                        } else if (grbitCur == hstPlanetary && iItem >= iplanetaryViewer50 && iItem <= iplanetarySnooper620X) {
                                            idm = idmRecentBreakthroughHasAlsoTaughtHowBuild2;
                                        } else {
                                            idm = idmRecentBreakthroughHasAlsoGivenBenefit;
                                        }
                                        iGoto = ibitCur << 8 | 0xc000 | iItem;
                                    }
                                    FSendPlrMsg(i, idm, iGoto, iT, grbitCur, iItem, 0, 0, 0, 0);
                                }
                                iItem++;
                            }
                        }
                        grbitCur *= 2;
                        ibitCur++;
                    }
                    if ((fUsePool || fChgNow || iTechNext == 7) && iTechNext != 6 && iT == iTechCur) {
                        if (iTechNext == 7) {
                            iTechNext = 0;
                            for (jj = 1; jj < 6; jj++) {
                                if (rgplr[i].rgTech[jj] < LOBYTE((int16_t)(((uint16_t)iTechNext & 0xff00) | ((uint16_t)rgplr[i].rgTech[iTechNext] & 0xff)))) {
                                    iTechNext = jj;
                                }
                            }
                            rgplr[idPlayer].iTechNow = iTechNext;
                            rgplr[i].rgResSpent[iT] = 0;
                            iTechCur = iTechNext;
                            iTechNext = 7;
                        } else {
                            rgplr[idPlayer].iTechNext = 6;
                            rgplr[idPlayer].iTechNow = iTechNext;
                            rgplr[i].rgResSpent[iT] = 0;
                            iTechCur = iTechNext;
                            iTechNext = 6;
                        }
                        if (game.fSlowTech) {
                            lSpent = (int32_t)((lSpent + 1) >> 1);
                        }
                        rgplr[i].rgResSpent[iTechCur] += lSpent;
                        fUsePool = FALSE;
                        fRedoItAll = TRUE;
                    } else {
                        goto CheckForBreakthrough;
                    }
                } else {
                    if (game.fSlowTech) {
                        lSpent = (int32_t)((lSpent + 1) >> 1);
                    }
                    rgplr[i].rgResSpent[iT] = lSpent;
                }
            }
        }
        if (fRedoItAll)
            goto RedoItAll;
    }
    idPlayer = iplrNone;
    fRedoItAll = FALSE;
    if (fUsePoolOrig && cPlrAlive > 1) {
        for (i = 0; i < game.cPlayer; i++) {
            if (GetRaceStat(&rgplr[i], rsMajorAdv) == raStealth) {
                for (iT = 0; iT < 6; iT++) {
                    if (rglFieldSpent[iT] > 0) {
                        lSpent = (int32_t)(rglFieldSpent[iT] / cPlrAlive) / 2;
                        if (lSpent > 1) {
                            fRedoItAll = TRUE;
                            FSendPlrMsg2(i, idmIntelligenceGatheringActivitiesCombinedSynergist, gotoResearch, iT, LOWORD(lSpent));
                            if (game.fSlowTech) {
                                lSpent = (int32_t)((lSpent + 1) >> 1);
                            }
                            rgplr[i].rgResSpent[iT] += lSpent;
                        }
                    }
                }
            }
        }
        if (fRedoItAll) {
            UpdateResearchStatus(FALSE);
        }
    }
    return;
}

int16_t IBestRemoteTerra(PLANET *lppl, int16_t iplr, int16_t fHelp) {
    int16_t iBest;
    int16_t i;
    PLAYER  plrSav;

    plrSav = rgplr[lppl->iPlayer];
    rgplr[lppl->iPlayer] = rgplr[iplr];
    for (i = 0; i < 3; i++) {
        rgplr[lppl->iPlayer].rgEnvVar[i] = plrSav.rgEnvVar[i];
        rgplr[lppl->iPlayer].rgEnvVarMin[i] = plrSav.rgEnvVarMin[i];
        rgplr[lppl->iPlayer].rgEnvVarMax[i] = plrSav.rgEnvVarMax[i];
    }
    iBest = IBestTerraform(lppl, fHelp);
    rgplr[lppl->iPlayer] = plrSav;
    return iBest;
}
