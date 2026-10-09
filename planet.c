#include "common.h"

int16_t FGetBestDefensePart(PART *ppart) {
    int16_t fRet;
    int16_t i;
    PART    part;

    fRet = TRUE;
    part.hs.grhst = hstPlanetary;
    part.hs.iItem = iplanetarySDI;
    i = 0;
    while (i < 5 && FLookupPart(&part) == mdPartAvailAvailable) {
        i++;
        part.hs.iItem++;
    }
    if (i > 0) {
        i--;
    } else {
        fRet = FALSE;
    }
    part.hs.iItem = i + 9;
    FLookupPart(&part);
    *ppart = part;
    return fRet;
}

char *PszProductionETA(PLANET *lppl, PLPROD *lpplprod, ProdItemType iItem, int16_t *etaFirst, int16_t *etaLast) {
    int16_t  iTurnEnd;
    int16_t  iTurnBegin;
    int16_t  c;
    StringId ids;

    if (!lpplprod) {
        lpplprod = lppl->lpplprod;
    }
    EstimateItemProdSched(lppl, lpplprod, iItem, &iTurnBegin, &iTurnEnd);
    if (iTurnBegin == 100) {
        if (lpplprod && lpplprod->iprodMac > (int16_t)iItem && lpplprod->rgprod[iItem].grobj == grobjPlanet && lpplprod->rgprod[iItem].iItem < mdIdleFactory) {
            ids = idsUnknown2;
        } else {
            ids = idsNever;
        }
        c = CchGetString(ids, szWork);
    } else if (iTurnEnd == 100) {
        c = CchSprintf(szWork, PszGetCompressedString(idsDYears), iTurnBegin);
    } else if (iTurnBegin == iTurnEnd) {
        if (iTurnBegin == 0) {
            c = CchGetString(idsSkipped, szWork);
        } else if (iTurnBegin == -1) {
            c = CchGetString(idsNeeded, szWork);
        } else {
            c = CchSprintf(szWork, PszGetCompressedString(idsDYear), iTurnBegin);
            if (iTurnBegin != 1) {
                szWork[c] = 's';
                c++;
                szWork[c] = 0;
            }
        }
    } else {
        c = CchSprintf(szWork, PszGetCompressedString(idsDDYears), iTurnBegin, iTurnEnd);
    }
    if (etaFirst) {
        *etaFirst = iTurnBegin;
    }
    if (etaLast) {
        *etaLast = iTurnEnd;
    }
    return szWork;
}

void ChangeMainObjSel(GrobjClass grobjNew, int16_t iObjSel) {
    int16_t fSameType;
    int16_t idSkip;
    int16_t i;
    FLEET  *lpfl;

    idSkip = idflNone;
    fSameType = grobjNew == sel.grobj;
    if (!fAi || !fSameType || iObjSel != sel.id) {
        InvalidateReport(sel.grobj == grobjPlanet ? rptPlanets : rptFleets, 0);
        if (grobjNew == grobjPlanet) {
            InvalidateReport(rptPlanets, 0);
            if (!FLookupPlanet(iObjSel, &sel.pl)) {
                return;
            }
            sel.pt = rgptPlan[iObjSel];
            sel.scan.iwp = iwpNone;
            sel.iwpAct = iwpNone;
            for (i = 0; i < cFleet; i++) {
                lpfl = rglpfl[i];
                if (!rglpfl[i] || (lpfl->idPlanet == iObjSel && lpfl->iPlayer == idPlayer))
                    break;
            }
            if (i != cFleet) {
                FDupFleet(lpfl, &sel.fl);
                sel.grobjFull = grobjPlanet | grobjFleet;
            } else {
                sel.fl.id = idflNone;
                sel.grobjFull = grobjPlanet;
            }
            if (!fAi) {
                ShowPlanetSel();
            }
        } else {
            InvalidateReport(rptFleets, 0);
            if (!FLookupFleet(iObjSel, &sel.fl)) {
                return;
            }
            sel.pt = sel.fl.pt;
            if (sel.fl.idPlanet == idPlanetDeepSpace || (sel.fl.idPlanet != sel.pl.id && !FLookupPlanet(sel.fl.idPlanet, &sel.pl))) {
                sel.pl.id = idplNone;
            }
            sel.grobjFull = (sel.pl.id != idplNone) | 2;
            sel.iwpAct = 0;
            if (!fAi) {
                ShowFleetSel();
                idSkip = iObjSel;
            }
        }
        sel.grobj = grobjNew;
        sel.id = iObjSel;
        gd.fSetMassMode = FALSE;
        gd.fSetRouteMode = FALSE;
        if (!fAi) {
            ShowMainObjSel(fSameType, idSkip);
        }
    }
    return;
}

void SelectAdjPlanet(int16_t dInc, int16_t idPlanet) {
    PLANET *lpPlT;
    int16_t i;
    PLANET *lpPl;
    SCAN    scan;
    int16_t fWrap;

    fWrap = FALSE;
    if (cPlanet > 0 && idPlanet != idplNone) {
        if (dInc != 0) {
            idPlanet = sel.pl.id;
        }
        if (!vrptPlanet.fCached) {
            InvalidateReport(rptPlanets, 1);
        }
        lpPlT = lpPlanets;
        lpPl = lpPlanets;
        i = 0;
        for (; i < cPlanet && lpPl->id != idPlanet; lpPl++) {
            i++;
        }
        if (i == cPlanet || lpPl->det != detAll) {
            scan.pt = rgptPlan[idPlanet];
            scan.grobj = grobjPlanet | mdExact;
            ChangeScanSel(&scan, 0);
            goto FinishUp;
        }
        if (dInc != 0) {
            for (i = 0; i < rgplr[idPlayer].cPlanet && lpPlT[vlprgidPlanet[i]].id != idPlanet; i++) {
            }
            i += dInc;
            if (i >= rgplr[idPlayer].cPlanet) {
                i = 0;
            } else if (i < 0) {
                i = rgplr[idPlayer].cPlanet - 1;
            }
            i = vlprgidPlanet[i];
        }
        lpPlT += i;
        idPlanet = lpPlT->id;
        if (lpPlT->iPlayer != idPlayer) {
            return;
        }
        scan.pt = rgptPlan[idPlanet];
        scan.grobj = grobjPlanet | mdExact;
        ChangeScanSel(&scan, 0);
        ShowScanSel(0);
        ChangeMainObjSel(grobjPlanet, idPlanet);
        ShowScanSel(1);
    FinishUp:
        ShowSelAt(rgptPlan[idPlanet]);
    }
    return;
}

int16_t IdFindAdjStarbase(int16_t idPlanet, int16_t fNext) {
    PLANET *lpplMac;
    int16_t idLast;
    int16_t idFirst;
    PLANET *lppl;
    int16_t idAfter;
    int16_t idBefore;

    idLast = idplNone;
    idFirst = idplNone;
    idAfter = idplNone;
    idBefore = idplNone;
    lppl = lpPlanets;
    lpplMac = lpPlanets + cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->iPlayer == idPlayer && lppl->fStarbase && LphuldefFromId(rglpshdefSB[idPlayer][lppl->isb].hul.ihuldef)->hul.wtCargoMax != 0) {
            if (lppl->id > idPlanet) {
                if (idAfter == idplNone) {
                    idAfter = lppl->id;
                }
            } else if (lppl->id < idPlanet) {
                idBefore = lppl->id;
            }
            if (idFirst == idplNone) {
                idFirst = lppl->id;
            }
            idLast = lppl->id;
        }
    }
    if (fNext) {
        if (idAfter == idplNone) {
            return idFirst;
        }
        return idAfter;
    }
    if (idBefore == idplNone) {
        return idLast;
    }
    return idBefore;
}

int16_t IBestTerraform(PLANET *lppl, int16_t fHelp) {
    int16_t iSave;
    int16_t iBest;
    int16_t rgMax[3];
    int16_t pctT;
    int16_t i;
    int16_t iPlr;
    int16_t iEnv;
    int16_t pctCur;
    int16_t rgMin[3];
    int16_t rgpctBest[3];
    int16_t rgCost[3];
    int16_t iPlrSav;

    iPlrSav = idPlayer;
    iPlr = lppl->iPlayer;
    if (iPlr == iplrNone) {
        return 0;
    }
    idPlayer = iPlr;
    if (!FCanTerraformLppl(lppl, rgMin, rgMax, rgCost, fHelp)) {
        idPlayer = iPlrSav;
        return 0;
    }
    pctCur = PctPlanetDesirability(lppl, iPlr);
    for (i = 0; i < 3; i++) {
        if (rgMin[i] != envNone) {
            iEnv = rgMin[i];
        } else if (rgMax[i] != envNone) {
            iEnv = rgMax[i];
        } else {
            rgpctBest[i] = 0;
            continue;
        }
        iSave = lppl->rgEnvVar[i];
        lppl->rgEnvVar[i] = iEnv;
        pctT = PctPlanetDesirability(lppl, iPlr) - pctCur;
        if (pctT < 0) {
            pctT = -pctT;
        }
        rgpctBest[i] = (int16_t)(100 * pctT) / abs(iSave - iEnv) + 1;
        lppl->rgEnvVar[i] = iSave;
    }
    iSave = 0;
    for (i = 1; i < 3; i++) {
        if (rgpctBest[i] > rgpctBest[iSave]) {
            iSave = i;
        }
    }
    if (rgMin[iSave] != envNone) {
        iBest = -(iSave + 1);
    } else {
        iBest = iSave + 1;
    }
    idPlayer = iPlrSav;
    return iBest;
}

char *PszCalcEnvVar(EnvType iEnv, int16_t iVar) {
    switch (iEnv) {
    case Gravity:
    default:
        return PszCalcGravity(iVar);
    case Temperature:
        CchSprintf(szWork, "%d%cC", iVar * 4 - 200, 186);
        break;
    case Radiation:
        CchSprintf(szWork, "%dmR", iVar);
    }
    return szWork;
}

char *PszCalcGravity(int16_t iGravity) {
    int16_t d;
    int16_t iVal;

    d = abs(iGravity - 50);
    if (d <= 25) {
        iVal = d * 4 + 100;
    } else {
        iVal = (d - 25) * 24 + 200;
    }
    if (iGravity < 50) {
        iVal = 10000 / iVal;
    }
    CchSprintf(szWork, "%d.%02dg", iVal / 100, iVal % 100);
    return szWork;
}

int16_t PctPlanetCapacity(PLANET *lppl) {
    int32_t pctCap;
    int32_t lPopMax;

    lPopMax = CalcPlanetMaxPop(lppl->id, idPlayer);
    if (lPopMax <= 0) {
        return 0;
    }
    pctCap = (int32_t)((int32_t)((uint32_t)(lppl->rgwtMin[3] * 100) + (int32_t)(lPopMax / 2)) / lPopMax);
    if (pctCap >= 1000) {
        pctCap = 999;
    }
    return LOWORD(pctCap);
}

int16_t PctPlanetOptValue(PLANET *lppl, int16_t iPlr) {
    int16_t rgMax[3];
    int16_t i;
    int16_t rgMin[3];
    int16_t pctDesire;
    int16_t rgCost[3];
    int16_t rgiValSav[3];
    int16_t iNewVal;

    if (!FCanTerraformLppl(lppl, rgMin, rgMax, rgCost, TRUE)) {
        return PctPlanetDesirability(lppl, iPlr);
    }
    for (i = 0; i < 3; i++) {
        rgiValSav[i] = lppl->rgEnvVar[i];
        if (rgplr[iPlr].rgEnvVarMin[i] != envImmune && lppl->rgEnvVar[i] != rgplr[iPlr].rgEnvVar[i]) {
            iNewVal = envNone;
            if (lppl->rgEnvVar[i] < rgplr[iPlr].rgEnvVar[i]) {
                if (rgMax[i] > lppl->rgEnvVar[i]) {
                    iNewVal = rgplr[iPlr].rgEnvVar[i] >= rgMax[i] ? rgMax[i] : rgplr[iPlr].rgEnvVar[i];
                }
            } else if (rgMin[i] != envNone && rgMin[i] < lppl->rgEnvVar[i]) {
                iNewVal = rgplr[iPlr].rgEnvVar[i] <= rgMin[i] ? rgMin[i] : rgplr[iPlr].rgEnvVar[i];
            }
            if (iNewVal != envNone) {
                lppl->rgEnvVar[i] = iNewVal;
            }
        }
    }
    pctDesire = PctPlanetDesirability(lppl, idPlayer);
    for (i = 0; i < 3; i++) {
        lppl->rgEnvVar[i] = rgiValSav[i];
    }
    return pctDesire;
}

int16_t PctPlanetDesirability(PLANET *lppl, int16_t iPlr) {
    int16_t iMin;
    int16_t d;
    int16_t iMax;
    int32_t pctNeg;
    int16_t iPref;
    int16_t i;
    int16_t dPenalty;
    int32_t pctPos;
    int16_t pctVar;
    int16_t iPlanet;
    int32_t pctMod;

    pctPos = 0;
    pctNeg = 0;
    pctMod = 10000;
    for (i = 0; i < 3; i++) {
        iPlanet = lppl->rgEnvVar[i];
        iPref = rgplr[iPlr].rgEnvVar[i];
        iMin = rgplr[iPlr].rgEnvVarMin[i];
        iMax = rgplr[iPlr].rgEnvVarMax[i];
        if (iMax < 0) {
            pctPos += 10000;
        } else if (iPlanet >= iMin && iPlanet <= iMax) {
            pctVar = abs(iPlanet - iPref) * 100;
            if (iPlanet < iPref) {
                d = iPref - iMin;
                pctVar /= d;
                dPenalty = (iPref - iPlanet) * 2 - d;
            } else {
                d = iMax - iPref;
                pctVar /= d;
                dPenalty = (iPlanet - iPref) * 2 - d;
            }
            pctVar = 100 - pctVar;
            pctPos += (uint32_t)(pctVar * pctVar);
            if (dPenalty > 0) {
                pctMod = (uint32_t)(pctMod * (int16_t)(d * 2 - dPenalty));
                pctMod = (int32_t)(pctMod / (int16_t)(d * 2));
            }
        } else if (iPlanet < iMin) {
            pctNeg += 15 >= iMin - iPlanet ? (int16_t)(iMin - iPlanet) : 15;
        } else {
            pctNeg += 15 >= iPlanet - iMax ? (int16_t)(iPlanet - iMax) : 15;
        }
    }
    if (pctNeg != 0) {
        return -LOWORD(pctNeg);
    }
    pctPos = Sf80ToI32((Sf80Add(Sf80From64(Sf64Sqrt(Sf64From80((Sf80Div(Sf80FromI32(pctPos), Sf80From64(3.0)))))), Sf80From64(0.9))));
    pctPos = (int32_t)(pctPos * pctMod) / 10000;
    return LOWORD(pctPos);
}

int32_t CalcPlanetMaxPop(int16_t idpl, int16_t iplr) {
    PLANET  pl;
    int32_t lMaxPop;
    int32_t pctDesire;
    int16_t ihuldef;

    FLookupPlanet(idpl, &pl);
    if (GetRaceStat(&rgplr[iplr], rsMajorAdv) == raMacintosh) {
        if (pl.iPlayer != iplr || !pl.fStarbase) {
            return 0;
        }
        ihuldef = rglpshdefSB[iplr][pl.isb].hul.ihuldef - ihuldefOrbitalFort;
        lMaxPop = rglPopMac[ihuldef];
    } else {
        pctDesire = PctPlanetDesirability(&pl, iplr);
        if (pctDesire < 5) {
            lMaxPop = 500;
        } else {
            lMaxPop = (uint32_t)(pctDesire * 100);
        }
        if (GetRaceStat(&rgplr[iplr], rsMajorAdv) == raCheapCol) {
            lMaxPop -= (int32_t)(lMaxPop / 2);
        } else if (GetRaceStat(&rgplr[iplr], rsMajorAdv) == raNone) {
            lMaxPop += (int32_t)(lMaxPop / 5);
        }
    }
    if (GetRaceGrbit(&rgplr[iplr], ibitRaceOBRM) != 0) {
        lMaxPop += (int32_t)(lMaxPop / 10);
    }
    return lMaxPop;
}

int16_t CMaxMines(PLANET *lppl, int16_t iplr) {
    int32_t cMax;
    int32_t lPopMax;
    int16_t iEff;

    lPopMax = CalcPlanetMaxPop(lppl->id, iplr);
    iEff = GetRaceStat(&rgplr[iplr], rsMineOperate);
    cMax = (int32_t)(lPopMax * iEff) / 100;
    if (cMax < 10) {
        cMax = 10;
    }
    if (GetRaceStat(&rgplr[iplr], rsMajorAdv) == raMacintosh) {
        cMax = 0;
    }
    return LOWORD(cMax);
}

int16_t CMaxOperableMines(PLANET *lppl, int16_t iplr, int16_t fNextYear) {
    int16_t cMax;
    int32_t cCur;
    int32_t lPop;
    int16_t iEff;

    cMax = CMaxMines(lppl, iplr);
    iEff = GetRaceStat(&rgplr[iplr], rsMineOperate);
    lPop = lppl->rgwtMin[3];
    if (fNextYear) {
        lPop += ChgPopFromPlanet(lppl, FALSE);
    }
    cCur = (int32_t)(lPop * iEff) / 100;
    cMax = cMax < cCur ? cMax : LOWORD(cCur);
    if (cMax <= 0) {
        cMax = 1;
    }
    if (GetRaceStat(&rgplr[iplr], rsMajorAdv) == raMacintosh) {
        cMax = 0;
    }
    return cMax;
}

int16_t CMinesOperating(PLANET *lppl) {
    int16_t iplr;
    int16_t cMinesOp;
    int16_t cMines;

    iplr = lppl->iPlayer;
    if (iplr == iplrNone) {
        return 0;
    }
    if (GetRaceStat(&rgplr[iplr], rsMajorAdv) == raMacintosh) {
        return LOWORD(Sf64ToI32(Sf64Sqrt(Sf64FromI32(lppl->rgwtMin[3]))));
    }
    cMines = lppl->cMines;
    cMinesOp = CMaxOperableMines(lppl, lppl->iPlayer, FALSE);
    if (cMines > cMinesOp) {
        cMines = cMinesOp;
    }
    return cMines;
}

int16_t CFactoriesOperating(PLANET *lppl) {
    int16_t iplr;
    int16_t cFacts;
    int16_t cFactsOp;

    iplr = lppl->iPlayer;
    if (iplr == iplrNone) {
        return 0;
    }
    if (GetRaceStat(&rgplr[iplr], rsMajorAdv) == raMacintosh) {
        return 0;
    }
    cFacts = lppl->cFactories;
    cFactsOp = CMaxOperableFactories(lppl, lppl->iPlayer, FALSE);
    if (cFacts > cFactsOp) {
        cFacts = cFactsOp;
    }
    return cFacts;
}

int16_t CMaxFactories(PLANET *lppl, int16_t iplr) {
    int32_t cMax;
    int32_t lPopMax;
    int16_t iEff;

    lPopMax = CalcPlanetMaxPop(lppl->id, iplr);
    iEff = GetRaceStat(&rgplr[iplr], rsFactOperate);
    cMax = (int32_t)(lPopMax * iEff) / 100;
    if (cMax < 10) {
        cMax = 10;
    }
    if (GetRaceStat(&rgplr[iplr], rsMajorAdv) == raMacintosh) {
        cMax = 0;
    }
    return LOWORD(cMax);
}

int16_t CMaxOperableFactories(PLANET *lppl, int16_t iplr, int16_t fNextYear) {
    int16_t cMax;
    int32_t cCur;
    int32_t lPop;
    int16_t iEff;

    cMax = CMaxFactories(lppl, iplr);
    iEff = GetRaceStat(&rgplr[iplr], rsFactOperate);
    lPop = lppl->rgwtMin[3];
    if (fNextYear) {
        lPop += ChgPopFromPlanet(lppl, FALSE);
    }
    cCur = (int32_t)(lPop * iEff) / 100;
    cMax = cMax < cCur ? cMax : LOWORD(cCur);
    if (cMax <= 0) {
        cMax = 1;
    }
    if (GetRaceStat(&rgplr[iplr], rsMajorAdv) == raMacintosh) {
        cMax = 0;
    }
    return cMax;
}

int16_t CMaxDefenses(PLANET *lppl, int16_t iplr) {
    int16_t cMax;
    int16_t pctDesire;

    pctDesire = PctPlanetDesirability(lppl, iplr);
    cMax = 100 < (10 <= pctDesire * 4 ? pctDesire * 4 : 10) ? 100 : 10 > pctDesire * 4 ? 10 : pctDesire * 4;
    if (GetRaceStat(&rgplr[iplr], rsMajorAdv) == raMacintosh) {
        cMax = 0;
    }
    return cMax;
}

int16_t CMaxOperableDefenses(PLANET *lppl, int16_t iplr, int16_t fNextYear) {
    int16_t cMax;
    int32_t cCur;
    int32_t lPop;

    cMax = CMaxDefenses(lppl, iplr);
    lPop = lppl->rgwtMin[3];
    if (fNextYear) {
        lPop += ChgPopFromPlanet(lppl, FALSE);
    }
    cCur = (int32_t)((lPop + 24) / 25);
    if (cCur > 1000) {
        cCur = 1000;
    }
    cMax = cMax >= (int16_t)LOWORD(cCur) ? LOWORD(cCur) : cMax;
    if (GetRaceStat(&rgplr[iplr], rsMajorAdv) == raMacintosh) {
        cMax = 0;
    }
    return cMax;
}

int16_t CResourcesAtPlanet(PLANET *lppl, int16_t iplr) {
    int16_t cRes;
    int32_t lPop;
    int16_t cFact;
    int32_t lPopMax;
    int16_t iEff;
    int16_t pctVal;
    int16_t iEnergy;

    if (lppl->rgwtMin[3] == 0) {
        return 0;
    }
    iEff = GetRaceStat(&rgplr[iplr], rsResGen);
    lPop = lppl->rgwtMin[3];
    lPopMax = CalcPlanetMaxPop(lppl->id, iplr);
    if (lPop > lPopMax) {
        lPop = (int32_t)((lPop - lPopMax) / 2) + lPopMax;
        if (lPop > (int32_t)(lPopMax * 2)) {
            lPop = (int32_t)(lPopMax * 2);
        }
    }
    if (GetRaceStat(&rgplr[iplr], rsMajorAdv) == raMacintosh) {
        iEnergy = rgplr[iplr].rgTech[0];
        pctVal = PctPlanetDesirability(lppl, iplr);
        if (iEnergy < 1) {
            iEnergy = 1;
        }
        if (pctVal < 25) {
            pctVal = 25;
        }
        cRes = LOWORD(
            Sf80ToI32((Sf80Add(Sf80Div(Sf80Mul(Sf80From64(Sf64Sqrt(Sf64From80((Sf80Div(Sf80Mul(Sf80FromI32(lPop), Sf80FromI32(iEnergy)), Sf80FromI32(iEff)))))),
                                               Sf80FromI32(pctVal)),
                                       Sf80FromI32(10)),
                               Sf80From64(0.999)))));
        goto LFinishUp;
    }
    cRes = LOWORD((int32_t)(lPop / iEff));
    cFact = CMaxOperableFactories(lppl, iplr, FALSE);
    if ((int32_t)lppl->cFactories < cFact) {
        cFact = lppl->cFactories;
    }
    iEff = GetRaceStat(&rgplr[iplr], rsFactProd);
    cRes += LOWORD((int32_t)((uint32_t)(cFact * iEff) + 9) / 10);
LFinishUp:
    if (cRes == 0) {
        cRes = 1;
    }
    return cRes;
}

int16_t IWarpMAFromLppl(PLANET *lppl, int16_t *pfTwo) {
    int16_t fTwo;
    int16_t iWarp;
    int16_t i;
    HUL    *lphul;
    int16_t iNew;

    iWarp = 0;
    fTwo = FALSE;
    if (pfTwo) {
        *pfTwo = FALSE;
    }
    if (lppl->iPlayer == iplrNone || !lppl->fStarbase) {
        return 0;
    }
    if (lppl->iPlayer != idPlayer && idPlayer != iplrNone && rglpshdefSB[lppl->iPlayer][lppl->isb].det != detAll) {
        return 0;
    }
    lphul = &rglpshdefSB[lppl->iPlayer][lppl->isb].hul;
    for (i = 0; i < lphul->chs; i++) {
        if (lphul->rghs[i].grhst == hstSpecialSB && lphul->rghs[i].cItem > 0 && lphul->rghs[i].iItem >= ispecialSBMassDriver5 &&
            lphul->rghs[i].iItem <= ispecialSBUltraDriver13) {
            iNew = lphul->rghs[i].iItem - 2;
            if (iNew > iWarp) {
                fTwo = FALSE;
                iWarp = iNew;
            } else if (iNew == iWarp) {
                fTwo = TRUE;
            }
        }
    }
    if (pfTwo) {
        *pfTwo = fTwo;
    }
    return iWarp;
}

int16_t StargateRangeFromLppl(PLANET *lppl, int16_t iplr, int16_t ish) {
    int16_t i;
    HUL    *lphul;
    PART    part;

    if (lppl) {
        if (lppl->iPlayer == iplrNone || !lppl->fStarbase) {
            return 0;
        }
        lphul = &rglpshdefSB[lppl->iPlayer][lppl->isb].hul;
    } else {
        lphul = &rglpshdefSB[iplr][ish].hul;
    }
    for (i = 0; i < lphul->chs; i++) {
        if (lphul->rghs[i].grhst == hstSpecialSB && lphul->rghs[i].cItem > 0 && lphul->rghs[i].iItem >= ispecialSBStargate100250 &&
            lphul->rghs[i].iItem <= ispecialSBStargateAnyAny) {
            part.hs = lphul->rghs[i];
            FLookupPart(&part);
            if (part.pspecialsb->grAbility2 == -1) {
                return 10000;
            }
            return part.pspecialsb->grAbility2;
        }
    }
    return 0;
}

int16_t FProdIsTerra(PROD *lpprod) {
    if (lpprod->grobj == grobjPlanet) {
        switch (lpprod->iItem) {
        case mdIdleTerraform:
        case iobjMinTerraform:
        case iobjMaxTerraform:
            return TRUE;
        }
    }
    return FALSE;
}

int16_t IpctCanTerraformLppl(PLANET *lppl) {
    int16_t rgMax[3];
    int16_t i;
    int16_t rgMin[3];
    int16_t rgCost[3];
    int16_t ipct;

    if (!FCanTerraformLppl(lppl, rgMin, rgMax, rgCost, TRUE)) {
        return 0;
    }
    ipct = 0;
    for (i = 0; i < 3; i++) {
        if (rgMin[i] != envNone) {
            ipct += lppl->rgEnvVar[i] - rgMin[i];
        }
        if (rgMax[i] != envNone) {
            ipct += rgMax[i] - lppl->rgEnvVar[i];
        }
    }
    return ipct;
}

int16_t FCanTerraformLppl(PLANET *lppl, int16_t *rgEnvMin, int16_t *rgEnvMax, int16_t *rgEnvCost, int16_t fHelp) {
    int16_t fRet;
    int16_t i;
    int16_t rgMove[3];
    int16_t iPlrSav;
    PART    part;
    int16_t dMin;
    int16_t dMax;
    int16_t dCur;
    int16_t ienvIdeal;

    iPlrSav = idPlayer;
    if (idPlayer == iplrNone) {
        idPlayer = lppl->iPlayer;
    }
    part.hs.grhst = hstTerra;
    for (i = 7; i >= 0; i--) {
        part.hs.iItem = i;
        if (FLookupPart(&part) == mdPartAvailAvailable)
            break;
    }
    if (i >= 0) {
        fRet = TRUE;
        for (i = 0; i < 3; i++) {
            rgMove[i] = part.pterra->grAbility;
            rgEnvCost[i] = part.pterra->resCost;
        }
    } else {
        fRet = FALSE;
        for (i = 0; i < 3; i++) {
            rgMove[i] = 0;
        }
    }
    for (i = 3; i >= 0; i--) {
        part.hs.iItem = i + 8;
        if (FLookupPart(&part) == mdPartAvailAvailable)
            break;
    }
    if (i >= 0 && part.pterra->grAbility > rgMove[0]) {
        fRet = TRUE;
        rgMove[0] = part.pterra->grAbility;
        *rgEnvCost = part.pterra->resCost;
    }
    for (i = 3; i >= 0; i--) {
        part.hs.iItem = i + 12;
        if (FLookupPart(&part) == mdPartAvailAvailable)
            break;
    }
    if (i >= 0 && part.pterra->grAbility > rgMove[1]) {
        fRet = TRUE;
        rgMove[1] = part.pterra->grAbility;
        rgEnvCost[1] = part.pterra->resCost;
    }
    for (i = 3; i >= 0; i--) {
        part.hs.iItem = i + 16;
        if (FLookupPart(&part) == mdPartAvailAvailable)
            break;
    }
    if (i >= 0 && part.pterra->grAbility > rgMove[2]) {
        fRet = TRUE;
        rgMove[2] = part.pterra->grAbility;
        rgEnvCost[2] = part.pterra->resCost;
    }
    if (!fRet) {
        idPlayer = iPlrSav;
        return FALSE;
    }
    for (i = 0; i < 3; i++) {
        if (rgMove[i] == 0 || rgplr[idPlayer].rgEnvVarMin[i] == envImmune) {
            rgEnvMax[i] = envNone;
            rgEnvMin[i] = envNone;
        } else {
            rgEnvMin[i] = lppl->rgEnvVarOrig[i] - rgMove[i];
            rgEnvMax[i] = lppl->rgEnvVarOrig[i] + rgMove[i];
            if (rgEnvMin[i] >= lppl->rgEnvVar[i]) {
                rgEnvMin[i] = envNone;
            } else {
                rgEnvMin[i] = 1 <= rgEnvMin[i] ? rgEnvMin[i] : 1;
            }
            if (rgEnvMax[i] <= lppl->rgEnvVar[i]) {
                rgEnvMax[i] = envNone;
            } else {
                rgEnvMax[i] = 99 >= rgEnvMax[i] ? rgEnvMax[i] : 99;
            }
            if (fHelp) {
                if (lppl->rgEnvVar[i] == rgplr[idPlayer].rgEnvVar[i]) {
                    rgEnvMax[i] = envNone;
                    rgEnvMin[i] = envNone;
                } else if (lppl->rgEnvVar[i] > rgplr[idPlayer].rgEnvVar[i]) {
                    rgEnvMax[i] = envNone;
                    if (rgEnvMin[i] != envNone) {
                        rgEnvMin[i] = rgEnvMin[i] <= rgplr[idPlayer].rgEnvVar[i] ? rgplr[idPlayer].rgEnvVar[i] : rgEnvMin[i];
                    }
                } else {
                    rgEnvMin[i] = envNone;
                    if (rgEnvMax[i] != envNone) {
                        rgEnvMax[i] = rgEnvMax[i] >= rgplr[idPlayer].rgEnvVar[i] ? rgplr[idPlayer].rgEnvVar[i] : rgEnvMax[i];
                    }
                }
            } else {
                ienvIdeal = rgplr[idPlayer].rgEnvVar[i];
                dCur = abs(lppl->rgEnvVar[i] - ienvIdeal);
                if (rgEnvMin[i] != envNone) {
                    dMin = abs(rgEnvMin[i] - ienvIdeal);
                } else {
                    dMin = 0;
                }
                if (rgEnvMax[i] != envNone) {
                    dMax = abs(rgEnvMax[i] - ienvIdeal);
                } else {
                    dMax = 0;
                }
                if (dCur >= dMin && dCur >= dMax) {
                    rgEnvMax[i] = envNone;
                    rgEnvMin[i] = envNone;
                } else if (dMin >= dMax) {
                    rgEnvMax[i] = envNone;
                } else {
                    rgEnvMin[i] = envNone;
                }
            }
        }
    }
    for (i = 0; i < 3 && (rgEnvMax[i] == envNone && rgEnvMin[i] == envNone); i++) {
    }
    idPlayer = iPlrSav;
    if (i != 3) {
        return TRUE;
    }
    return FALSE;
}

void UninhabitPlanet(PLANET *lppl) {
    int16_t i;

    if (lppl->iPlayer >= 0 && GetRaceStat(&rgplr[lppl->iPlayer], rsMajorAdv) == raTerra) {
        for (i = 0; i < 3; i++) {
            lppl->rgEnvVar[i] = lppl->rgEnvVarOrig[i];
        }
    }
    lppl->iPlayer = iplrNone;
    lppl->rgwtMin[3] = 0;
    if (lppl->lpplprod) {
        FreePl((PL *)lpPlanets[lppl->id].lpplprod);
        lpPlanets[lppl->id].lpplprod = NULL;
        lppl->lpplprod = NULL;
    }
    lppl->fNoResearch = FALSE;
    lppl->fStarbase = FALSE;
    lppl->cDefenses = 0;
    lppl->iScanner = 31;
    lppl->lStarbase = 0;
    return;
}

int16_t PctCloakFromHuldef(HUL *lphul, int16_t iplr, int16_t *ppctSteal) {
    int16_t chs;
    HS     *lphs;
    int32_t cPts;
    int16_t cScore;
    int16_t j;

    cPts = 0;
    chs = lphul->chs;
    if (iplr != iplrNone && (int16_t)lphul->ihuldef >= ihuldefOrbitalFort && GetRaceGrbit(&rgplr[iplr], ibitRaceISB) != 0) {
        cPts = 40;
    } else {
        cPts = 0;
    }
    if (iplr != iplrNone && GetRaceStat(&rgplr[iplr], rsMajorAdv) == raStealth) {
        cPts += 300;
    }
    if (ppctSteal) {
        *ppctSteal = 0;
    }
    j = 0;
    lphs = lphul->rghs;
    while (j < chs) {
        cPts += CPtsCloakFromLphs(lphs);
        if (lphs->grhst == hstScanner && ppctSteal) {
            if (lphs->iItem == iscannerPickPocketScanner) {
                if (*ppctSteal < 70) {
                    *ppctSteal = 70;
                }
            } else if (lphs->iItem == iscannerRobberBaronScanner && *ppctSteal < 80) {
                *ppctSteal = 80;
            }
        }
        j++;
        lphs++;
    }
    if (cPts == 0) {
        return 0;
    }
    if (cPts < 0 || cPts > 25000) {
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

// FProdItemLine formats production queue item iprod as the planet window
// lists it: a schedule mark, the count and the name. An item that will
// never be built is marked '&' if fShowNever is set, or is left out
// (FALSE). PszNameProdItem names the item in szWork.
int16_t FProdItemLine(PLANET *lppl, PLPROD *lpplprod, int16_t iprod, int16_t fShowNever, char *szLine) {
    char    ch;
    int16_t cItem;
    char   *psz;
    PROD   *lpprod;
    int16_t etaLast;
    int16_t etaFirst;

    lpprod = &lpplprod->rgprod[iprod];
    psz = PszNameProdItem(lpprod);
    EstimateItemProdSched(lppl, lpplprod, iprod, &etaFirst, &etaLast);
    if ((etaFirst == 0 && etaLast == 0) || (etaFirst == -1 && etaLast == -1)) {
        if (!fShowNever)
            return FALSE;
        ch = '&';
    } else if ((etaFirst > 1 && etaFirst < 100) || (etaFirst == 100 && lpprod->grobj == grobjPlanet && lpprod->iItem < mdIdleFactory)) {
        ch = ' ';
    } else if (etaFirst == 1 && etaLast == 1) {
        ch = '*';
    } else if (etaFirst < 100) {
        ch = '#';
    } else {
        ch = '!';
    }
    cItem = lpprod->cItem;
    CchSprintf(szLine, "%c%5d%s", ch, cItem, psz);
    if (lpprod->grobj == grobjPlanet) {
        if (lpprod->iItem < mdIdleFactory) {
            szLine[1] += 2;
            if (lpprod->iItem == iobjAlchemy) {
                szLine[5] = '*';
            }
        }
        switch (lpprod->iItem) {
        case mdIdleTerraform:
        case iobjMinTerraform:
        case iobjMaxTerraform:
            szLine[1]++;
        }
    }
    return TRUE;
}

// PszProdQueueTop returns, in szWork, the first item of lppl's production
// queue (lpplprod, or the planet's own) that will be built, as
// FProdItemLine formats it, or "Queue is empty". The reports and the
// planet dump show it.
char *PszProdQueueTop(PLANET *lppl, PLPROD *lpplprod) {
    char    szLine[80];
    char   *psz;
    int16_t i;

    if (!lpplprod) {
        lpplprod = lppl->lpplprod;
    }
    if (!lpplprod || lpplprod->iprodMac == 0) {
        psz = PszGetCompressedString(idsQueueEmpty);
        if (psz != szWork) {
            strcpy(szWork, psz);
        }
    }
    if (lpplprod) {
        for (i = 0; i < lpplprod->iprodMac; i++) {
            if (FProdItemLine(lppl, lpplprod, i, FALSE, szLine)) {
                strcpy(szWork, szLine);
                return szWork;
            }
        }
        CchGetString(idsQueueEmpty, szWork);
    }
    return szWork;
}
