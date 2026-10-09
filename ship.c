#include "common.h"

int16_t FCanSplit(int32_t cBoat) {
    if (rgplr[idPlayer].cFleet == 0x200) {
        return FALSE;
    }
    if (cBoat > 1) {
        return TRUE;
    }
    return FALSE;
}

int16_t FCanSplitAll(int32_t cBoat) {
    if (cBoat - 1 + rgplr[idPlayer].cFleet > 0x200) {
        return FALSE;
    }
    if (cBoat > 1) {
        return TRUE;
    }
    return FALSE;
}

int16_t FCanMerge(FLEET *pfl) {
    int16_t i;
    FLEET  *lpfl;
    int32_t csh;
    int16_t cfl;
    int16_t ishdef;

    cfl = 0;
    csh = 0;
    for (i = 0; i < cFleet; i++) {
        lpfl = rglpfl[i];
        if (!rglpfl[i])
            break;
        if (lpfl->iPlayer == pfl->iPlayer && lpfl->pt.x == sel.fl.pt.x && lpfl->pt.y == sel.fl.pt.y) {
            cfl++;
            for (ishdef = 0; ishdef < 16; ishdef++) {
                csh += lpfl->rgcsh[ishdef];
            }
        }
    }
    if (cfl == 1 || csh > (int32_t)(uint32_t)(32766 - (rgplr[pfl->iPlayer].cFleet - 1))) {
        return FALSE;
    }
    return TRUE;
}

void SelectAdjFleet(int16_t dInc, int16_t idFleet) {
    POINT16 pt;
    int16_t idOld;
    int16_t i;
    FLEET  *lpfl;
    int16_t idNew;
    FLEET  *lpflT;
    SCAN    scan;

    idOld = idflNone;
    if (cFleet > 0) {
        if (dInc != 0) {
            idFleet = sel.fl.id;
        }
        if (!vrptFleet.fCached) {
            InvalidateReport(rptFleets, 1);
        }
        for (i = 0; i < cFleet; i++) {
            lpfl = rglpfl[i];
            if (!rglpfl[i] || lpfl->id == idFleet)
                break;
        }
        if (i == cFleet || lpfl->iPlayer != idPlayer) {
            if (i == cFleet) {
                return;
            }
            pt = lpfl->pt;
            scan.pt = lpfl->pt;
            scan.grobj = grobjPlanet | grobjFleet | mdExact;
            ChangeScanSel(&scan, 0);
            goto FinishUp;
        } else {
            if (dInc != 0) {
                for (i = 0; i < (int16_t)rgplr[idPlayer].cFleet && rglpfl[vlprgidFleet[i]]->id != idFleet; i++) {
                }
                i += dInc;
                if (i >= (int16_t)rgplr[idPlayer].cFleet) {
                    i = 0;
                } else if (i < 0) {
                    i = rgplr[idPlayer].cFleet - 1;
                }
                i = vlprgidFleet[i];
            } else if (sel.grobj == grobjFleet && sel.fl.pt.x == lpfl->pt.x && sel.fl.pt.y == lpfl->pt.y) {
                idOld = sel.fl.id;
            }
            lpflT = rglpfl[i];
            idNew = lpflT->id;
            pt = lpflT->pt;
            scan.pt = lpflT->pt;
            scan.grobj = grobjFleet | mdExact;
            ChangeScanSel(&scan, 0);
            ShowScanSel(0);
            ChangeMainObjSel(grobjFleet, idNew);
            ShowScanSel(1);
        }
    FinishUp:
        ShowSelAt(pt);
        if (idOld != idflNone) {
            SetFleetDropDownSel(idOld);
        }
    }
    return;
}

int32_t LGetFleetStat(FLEET *lpfl, int16_t grStat) {
    int16_t i;
    int32_t l;

    l = 0;
    if (lpfl->det != detAll) {
        return 32000;
    }
    for (i = 0; i < 16; i++) {
        if (lpfl->rgcsh[i] != 0) {
            l += (uint32_t)(lpfl->rgcsh[i] * WtMaxShdefStat(rglpshdef[lpfl->iPlayer] + i, grStat));
        }
    }
    return l;
}

int16_t WtMaxShdefStat(SHDEF *lpshdef, int16_t grStat) {
    int16_t wt;
    int16_t j;
    HUL    *lphul;

    lphul = &lpshdef->hul;
    if (grStat != 1) {
        if (grStat != 2) {
            return 0;
        }
        wt = LphuldefFromId(lphul->ihuldef)->hul.wtCargoMax;
        for (j = 0; j < lphul->chs; j++) {
            if (lphul->rghs[j].grhst == hstSpecialM) {
                switch (lphul->rghs[j].iItem) {
                default:
                    break;
                case ispecialMCargoPod:
                    wt += lphul->rghs[j].cItem * 50;
                    break;
                case ispecialMSuperCargoPod:
                    wt += lphul->rghs[j].cItem * 100;
                    break;
                case ispecialMMultiCargoPod:
                    wt += lphul->rghs[j].cItem * 250;
                }
            }
        }
    } else {
        wt = LphuldefFromId(lphul->ihuldef)->hul.wtFuelMax;
        for (j = 0; j < lphul->chs; j++) {
            if (lphul->rghs[j].grhst != hstSpecialM) {
                if (lphul->rghs[j].grhst == hstSpecialE && lphul->rghs[j].iItem == ispecialEAntiMatterGenerator) {
                    wt += lphul->rghs[j].cItem * 200;
                }
            } else if (lphul->rghs[j].iItem == ispecialMFuelTank) {
                wt += lphul->rghs[j].cItem * 250;
            } else if (lphul->rghs[j].iItem == ispecialMSuperFuelTank) {
                wt += lphul->rghs[j].cItem * 500;
            }
        }
    }
    return wt;
}

int16_t FEnumCalcJettison(void *lprt, RecordType rt, int16_t cb, PLANET *lppl, int16_t iFleet) {
    POINT16  pt;
    int16_t  i;
    int16_t  grbit;
    FLEET    fl;
    int16_t  j;
    RTXFERX *prtxferx;
    RTXFER  *prtxfer;

    if (rt == rtLogCargoXfer8 || rt == rtLogCargoXfer16) {
        prtxfer = lprt;
        if (prtxfer->grobj1 != grobjFleet || prtxfer->grobj2 != grobjOther) {
            return TRUE;
        }
        if (!FLookupFleet(iFleet, &fl)) {
            return TRUE;
        }
        pt = fl.pt;
        if (!FLookupFleet(prtxfer->id1, &fl)) {
            return TRUE;
        }
        if (fl.pt.x != pt.x || fl.pt.y != pt.y) {
            return TRUE;
        }
        grbit = prtxfer->grbitItems;
        if (rt == rtLogCargoXfer16) {
            prtxferx = lprt;
        }
        j = 0;
        i = 0;
        while (i < 5) {
            if (grbit & 1) {
                if (rt == rtLogCargoXfer8) {
                    lppl->rgwtMin[i] -= (int16_t)prtxfer->rgcQuan[j];
                } else {
                    lppl->rgwtMin[i] -= prtxferx->rgcQuan[j];
                }
                j++;
            }
            i++;
            grbit >>= 1;
        }
    }
    return TRUE;
}

int32_t GetCargoFree(FLEET *lpfl) {
    int32_t cHave;
    int16_t i;

    cHave = 0;
    for (i = 0; i <= 3; i++) {
        cHave += lpfl->rgwtMin[i];
    }
    return LGetFleetStat(lpfl, 2) - cHave;
}

int32_t GetFuelFree(FLEET *lpfl) { return LGetFleetStat(lpfl, 1) - lpfl->rgwtMin[4]; }

int32_t ChgCargo(GrobjClass grobj, int16_t id, MineralType iSupply, int32_t dChg, void *pobj) {
    THING  *pth;
    XFER    xfer;
    int16_t i;
    FLEET  *pfl;
    PLANET *ppl;
    int32_t wtFree;

    switch (grobj) {
    case grobjPlanet:
    case grobjOther:
        if (pobj) {
            ppl = pobj;
        } else if (grobj == grobjPlanet) {
            FLookupPlanet(id, &xfer.pl);
            ppl = &xfer.pl;
        } else {
            memset(&xfer.pl, 0, sizeof(PLANET));
            ppl = &xfer.pl;
        }
        if (iSupply <= Fuel) {
            if (iSupply == Fuel) {
                return 0;
            }
            if (dChg == 0) {
                return ppl->rgwtMin[iSupply];
            }
            if (ppl->rgwtMin[iSupply] + dChg < 0) {
                dChg = -ppl->rgwtMin[iSupply];
            }
            ppl->rgwtMin[iSupply] += dChg;
        }
        if (dChg == 0 || pobj || grobj == grobjOther)
            break;
        FLookupPlanet(idWriteBack, &xfer.pl);
        break;
    case grobjThing:
        if (pobj) {
            pth = pobj;
        } else {
            FLookupThing(id, &xfer.th);
            pth = &xfer.th;
        }
        if (iSupply >= Colonists) {
            return 0;
        }
        if (iSupply <= Fuel) {
            if (dChg == 0) {
                return pth->thp.rgwtMin[iSupply];
            }
            if (pth->thp.rgwtMin[iSupply] + dChg < 0) {
                dChg = (int16_t)-pth->thp.rgwtMin[iSupply];
            }
            wtFree = (uint32_t)(pth->thp.wtMax * 10);
            for (i = 0; i < 3; i++) {
                wtFree -= pth->thp.rgwtMin[i];
            }
            if (dChg > wtFree) {
                dChg = wtFree;
            }
            pth->thp.rgwtMin[iSupply] += LOWORD(dChg);
        }
        if (dChg == 0 || pobj)
            break;
        FLookupThing(idWriteBack, pth);
        break;
    default:
        if (pobj) {
            pfl = pobj;
        } else {
            FLookupFleet(id, &xfer.fl);
            pfl = &xfer.fl;
        }
        if (iSupply <= Fuel) {
            if (dChg == 0) {
                return pfl->rgwtMin[iSupply];
            }
            if (pfl->rgwtMin[iSupply] + dChg < 0) {
                dChg = -pfl->rgwtMin[iSupply];
            }
            if (iSupply == Colonists && pfl->det != detAll) {
                dChg = 0;
            }
            if (dChg >= (iSupply == Fuel ? GetFuelFree(pfl) : GetCargoFree(pfl))) {
                if (iSupply == Fuel) {
                    dChg = GetFuelFree(pfl);
                } else {
                    dChg = GetCargoFree(pfl);
                }
            }
            pfl->rgwtMin[iSupply] += dChg;
        }
        if (dChg != 0 && !pobj) {
            FLookupFleet(idWriteBack, pfl);
        }
    }
    return dChg;
}

int32_t XferSupply(MineralType iSupply, int32_t cQuan) {
    int16_t iSrc;
    int32_t dChg;
    int32_t cAvailable;

    if (cQuan == 0) {
        return 0;
    }
    iSrc = cQuan > 0;
    if (iSrc == 0) {
        cQuan = -cQuan;
    }
    cAvailable = ChgCargo(pxfer[iSrc].grobj, pxfer[iSrc].id, iSupply, 0, (uint8_t *)(pxfer + iSrc) + 4);
    if (cQuan > cAvailable) {
        cQuan = cAvailable;
    }
    if (cQuan == 0) {
        return 0;
    }
    dChg = ChgCargo(pxfer[iSrc == 0].grobj, pxfer[iSrc == 0].id, iSupply, cQuan, (uint8_t *)(pxfer + (iSrc == 0)) + 4);
    if (dChg != 0) {
        ChgCargo(pxfer[iSrc].grobj, pxfer[iSrc].id, iSupply, -dChg, (uint8_t *)(pxfer + iSrc) + 4);
    }
    return dChg;
}

void DeleteWpFar(FLEET *lpfl, int16_t iDel, int16_t fRecycle) {
    ORDER ord;

    if (fRecycle) {
        if (iDel == 86 || lpfl->cord == 2 ||
            (lpfl->lpplord->rgord[lpfl->cord - 1].pt.x == lpfl->lpplord->rgord[iDel].pt.x &&
             lpfl->lpplord->rgord[lpfl->cord - 1].pt.y == lpfl->lpplord->rgord[iDel].pt.y)) {
            fRecycle = FALSE;
        } else {
            ord = lpfl->lpplord->rgord[iDel];
        }
    }
    memmove(&lpfl->lpplord->rgord[iDel], &lpfl->lpplord->rgord[iDel + 1], (lpfl->cord - iDel - 1) * sizeof(ORDER));
    if (fRecycle) {
        lpfl->lpplord->rgord[lpfl->cord - 1] = ord;
    } else {
        lpfl->cord--;
        lpfl->lpplord->iordMac--;
    }
    return;
}

int32_t EstFuelUse(FLEET *lpfl, int16_t iOrd, int16_t iWarp, int32_t dTravel, int16_t fRangeOnly) {
    int32_t iEffNext;
    int32_t lT;
    int16_t fEfficient;
    double  d;
    int32_t iEffCur;
    int32_t wtCargoT;
    int32_t lFuel;
    ORDER  *lpord;
    int16_t i;
    SHDEF  *lpshdef;
    int32_t wtCargo;
    int16_t j;
    int32_t wtMass;
    int32_t rgieff[16];

    iEffCur = 0;
    gd.fRadiatingEngine = FALSE;
    if (iWarp == -1) {
        iWarp = lpfl->lpplord->rgord[iOrd + 1].iWarp;
    }
    fEfficient = GetRaceGrbit(&rgplr[lpfl->iPlayer], ibitRaceIFE);
    for (i = 0, lpshdef = rglpshdef[lpfl->iPlayer]; i < 16; i++, lpshdef++) {
        if (lpfl->rgcsh[i] != 0) {
            for (j = 0; j < lpshdef->hul.chs && lpshdef->hul.rghs[j].grhst != hstEngine; j++) {
            }
            if (j == lpshdef->hul.chs || lpshdef->hul.rghs[j].cItem < LphuldefFromId(lpshdef->hul.ihuldef)->hul.rghs[j].cItem) {
                rgieff[i] = 99999;
                continue;
            }
            rgieff[i] = LpengineFromId(lpshdef->hul.rghs[j].iItem)->rgcFuelUsed[iWarp];
            if (fEfficient) {
                rgieff[i] -= (int32_t)(rgieff[i] * 15) / 100;
            }
            if (lpshdef->hul.rghs[j].iItem == iengineRadiatingHydroRamScoop) {
                gd.fRadiatingEngine = TRUE;
            }
        }
    }
    wtCargo = 0;
    for (i = 0; i <= 3; i++) {
        wtCargo += lpfl->rgwtMin[i];
    }
    if (dTravel == -1) {
        if (fRangeOnly) {
            dTravel = 1000;
        } else {
            lpord = &lpfl->lpplord->rgord[iOrd];
            d = DGetDistance(lpord->pt.x, lpord->pt.y, lpord[1].pt.x, lpord[1].pt.y);
            dTravel = (int16_t)LOWORD(Sf80ToI32((Sf80Add(Sf80From64(d), Sf80From64(0.9999)))));
        }
    }
    lFuel = 0;
    while (1) {
        iEffNext = 999999;
        for (i = 0; i < 16; i++) {
            if (lpfl->rgcsh[i] > 0) {
                if (rgieff[i] == iEffCur) {
                    if (wtCargo < (int32_t)(uint32_t)(lpfl->rgcsh[i] * WtMaxShdefStat(rglpshdef[lpfl->iPlayer] + i, 2))) {
                        wtCargoT = wtCargo;
                    } else {
                        wtCargoT = (uint32_t)(lpfl->rgcsh[i] * (int16_t)WtMaxShdefStat(rglpshdef[lpfl->iPlayer] + i, 2));
                    }
                    wtCargo -= wtCargoT;
                    if (rgieff[i] > 0) {
                        wtMass = (uint32_t)(lpfl->rgcsh[i] * (uint32_t)rglpshdef[lpfl->iPlayer][i].hul.wtEmpty) + wtCargoT;
                        lT = (uint32_t)(iEffCur * dTravel);
                        if (wtMass < 200 || (lT < 500000 && wtMass < 4000) || (lT < 100000 && wtMass < 20000)) {
                            lFuel += (int32_t)(wtMass * lT) / 2000;
                        } else {
                            lFuel = Sf80ToI32((Sf80Div(Sf80Mul(Sf80FromI32(lT), Sf80FromI32(wtMass)), Sf80From64(2000.0)))) + lFuel;
                        }
                    }
                } else if (rgieff[i] > iEffCur && rgieff[i] < iEffNext) {
                    iEffNext = rgieff[i];
                }
            }
        }
        if (iEffNext == 999999)
            break;
        iEffCur = iEffNext;
    }
    if (!fRangeOnly) {
        lFuel += 9;
    }
    lFuel = (int32_t)(lFuel / 10);
    if (fRangeOnly) {
        if (lFuel == 0) {
            lFuel = 1000000000;
        } else if (lFuel > 100000) {
            lFuel = (int32_t)(lpfl->rgwtMin[4] / (int32_t)(lFuel / 1000));
        } else {
            lFuel = (int32_t)((int32_t)(lpfl->rgwtMin[4] * 1000) / lFuel);
        }
    }
    return lFuel;
}

int16_t IFindIdealWarp(FLEET *lpfl, int16_t fIgnoreScoops) {
    int16_t i;
    int16_t j;
    int16_t iWorst;
    ENGINE *lpengine;

    iWorst = 10;
    if (!lpfl) {
        lpfl = &sel.fl;
    }
    for (i = 0; i < 16; i++) {
        if (lpfl->rgcsh[i] > 0) {
            for (j = 0; j < rglpshdef[lpfl->iPlayer][i].hul.chs && rglpshdef[lpfl->iPlayer][i].hul.rghs[j].grhst != hstEngine; j++) {
            }
            if (j == rglpshdef[lpfl->iPlayer][i].hul.chs) {
                iWorst = 0;
                break;
            }
            j = rglpshdef[lpfl->iPlayer][i].hul.rghs[j].iItem;
            lpengine = LpengineFromId(j);
            for (; iWorst > 0; iWorst--) {
                if (lpengine->rgcFuelUsed[iWorst] <= 120) {
                    if (lpengine->rgcFuelUsed[iWorst] > 0 && !fIgnoreScoops && j != iengineTransGalacticMizerScoop && j != iengineGalaxyScoop) {
                        if (iWorst >= 5 && lpengine->rgcFuelUsed[iWorst - 1] == 0) {
                            iWorst--;
                        } else if (iWorst >= 6 && lpengine->rgcFuelUsed[iWorst - 2] == 0) {
                            iWorst -= 2;
                        } else if (iWorst >= 7 && lpengine->rgcFuelUsed[iWorst - 3] == 0) {
                            iWorst -= 3;
                        }
                    }
                    if (iWorst == 10) {
                        switch (j) {
                        default:
                            iWorst = 9;
                        case iengineInterspace10:
                        case iengineEnigmaPulsar:
                        case iengineTransStar10:
                        case iengineTransGalacticMizerScoop:
                        case iengineGalaxyScoop:
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }
    return iWorst;
}

// IWarpForWaypoint is IWarpBestForWaypoint's body. fFastest asks for the
// speed colonizers get, as IWarpFastestForWaypoint does.
static int16_t IWarpForWaypoint(FLEET *lpfl, ORDER *lpord, int16_t fFastest) {
    int32_t lFuel;
    int16_t iWarp;
    int16_t cTravel;
    int16_t iwp;
    int16_t lDist;
    int16_t cSpeed;
    int16_t fGoFlatOutAi;
    int16_t fGoFlatOut;
    int16_t iWarpAi;
    int16_t iWarpSav;
    int16_t j;
    int16_t i;
    PLANET *lppl;
    int16_t iWarpOld;
    SCAN    scan;

    iWarpSav = lpord->iWarp;
    iWarp = IFindIdealWarp(NULL, FALSE);
    if (fAi) {
        iWarpAi = IFindIdealWarp(NULL, TRUE);
    }
    for (iwp = lpfl->cord - 1; iwp >= 0 && lpord != &lpfl->lpplord->rgord[iwp]; iwp--) {
    }
    if (iwp <= 0) {
        return iWarp;
    }
    if (fFastest || lpord->grTask == grTaskColonize || lpord->grTask == grTaskScrap) {
        fGoFlatOut = TRUE;
    } else {
        fGoFlatOut = FALSE;
        for (i = 0; i < 16; i++) {
            if (lpfl->rgcsh[i] > 0) {
                for (j = 0; j < rglpshdef[lpfl->iPlayer][i].hul.chs; j++) {
                    if (rglpshdef[lpfl->iPlayer][i].hul.rghs[j].grhst == hstSpecialM &&
                        (rglpshdef[lpfl->iPlayer][i].hul.rghs[j].iItem == ispecialMColonizationModule ||
                         rglpshdef[lpfl->iPlayer][i].hul.rghs[j].iItem == ispecialMOrbitalConstructionModule)) {
                        fGoFlatOut = TRUE;
                        break;
                    }
                }
            }
        }
    }
    if (!fGoFlatOut && fAi) {
        fGoFlatOutAi = TRUE;
        fGoFlatOut = TRUE;
    } else {
        fGoFlatOutAi = FALSE;
    }
    if (iWarp < 9) {
        iWarpOld = iWarp;
        if (FFindNearestObject(lpord->pt, grobjPlanet | mdExact, &scan)) {
            lppl = LpplFromId(scan.idpl);
        } else {
            lppl = NULL;
        }
        if (!fGoFlatOut && (!lppl || (lppl->iPlayer != iplrNone && lppl->iPlayer != idPlayer))) {
            if (iwp > 1 && lpord[-1].iWarp > (uint16_t)iWarp && lpord[-1].iWarp <= 10) {
                iWarp = lpord[-1].iWarp;
            }
            if (LFuelUseToWaypoint(lpfl, iwp, TRUE) >= (int32_t)(LGetFleetStat(lpfl, 1) / 10) || lpfl->rgwtMin[4] < (int32_t)(LGetFleetStat(lpfl, 1) * 7) / 10)
                goto LOptimizeSpeed;
            iWarp++;
            goto LTryLimitedSpeed;
        } else {
            if (fGoFlatOutAi && lppl && lppl->iPlayer == idPlayer && rglpshdefSB[idPlayer][lppl->isb].hul.ihuldef != ihuldefOrbitalFort) {
                fGoFlatOutAi = FALSE;
            }
            iWarp = 9;
        }
    LTryLimitedSpeed:
        while (iWarp > iWarpOld) {
            lpord->iWarp = iWarp;
            lFuel = LFuelUseToWaypoint(lpfl, iwp, TRUE);
            if (lFuel > lpfl->rgwtMin[4])
                goto LDecWarp;
            if (((lppl && lppl->fStarbase && lppl->iPlayer == idPlayer && LphuldefFromId(rglpshdefSB[idPlayer][lppl->isb].hul.ihuldef)->hul.wtCargoMax != 0) ||
                 lFuel <= (int32_t)(lpfl->rgwtMin[4] / 2) || fGoFlatOut))
                break;
        LDecWarp:
            iWarp--;
        }
        lpord->iWarp = iWarpSav;
    }
    if (fGoFlatOutAi && iWarp > iWarpAi) {
        iWarp = iWarpAi;
    }
LOptimizeSpeed:
    if (iWarp > 1 && lpord->grobj != grobjFleet) {
        lDist = LOWORD(Sf64ToI32(DGetDistance(lpord->pt.x, lpord->pt.y, lpord[-1].pt.x, lpord[-1].pt.y)));
        cSpeed = iWarp * iWarp;
        cTravel = (int16_t)(iWarp * iWarp + lDist - 1) / cSpeed;
        do {
            iWarp--;
            if (iWarp <= 1)
                break;
            cSpeed = iWarp * iWarp;
        } while (cTravel == (int16_t)(lDist + cSpeed - 1) / cSpeed);
        iWarp++;
    } else {
        cTravel = 2;
    }
    if (FCanFleetUseStargates(lpfl, lpord[-1].pt, lpord->pt) == 1) {
        iWarp = 11;
    }
    if (iWarp > 11) {
        iWarp = 9;
    }
    return iWarp;
}

// IWarpBestForWaypoint picks the warp for the leg to lpord, a waypoint of
// lpfl's orders.
int16_t IWarpBestForWaypoint(FLEET *lpfl, ORDER *lpord) { return IWarpForWaypoint(lpfl, lpord, FALSE); }

// IWarpFastestForWaypoint picks the warp a colonizing fleet gets for the
// leg to lpord: the fastest up to warp 9 whose fuel to lpord fits the
// tank, then the slowest that arrives as soon. Players set colonize tasks
// on scouts to get it; the scanner gives it to Alt+clicked waypoints.
int16_t IWarpFastestForWaypoint(FLEET *lpfl, ORDER *lpord) { return IWarpForWaypoint(lpfl, lpord, TRUE); }

int32_t LFuelUseToWaypoint(FLEET *lpfl, int16_t iwp, int16_t fMaxCargo) {
    int32_t lCur;
    int16_t iWarp;
    int16_t dist;
    PLANET *lppl;
    int16_t i;
    int32_t lTot;
    ORDER  *lpord;
    int16_t cYears;
    SHDEF  *lpshdef;
    int16_t j;
    double  dbl;
    int32_t l;
    int32_t lOneYearUse;
    int32_t lFuelGain;

    lTot = 0;
    lCur = 0;
    lpord = lpfl->lpplord->rgord;
    for (i = 0; i < iwp; i++) {
        iWarp = lpord[i + 1].iWarp;
        if (iWarp > 0 && iWarp < 11) {
            dbl = Sf64From80((Sf80Add(Sf80From64(DGetDistance(lpord[i].pt.x, lpord[i].pt.y, lpord[i + 1].pt.x, lpord[i + 1].pt.y)), Sf80From64(0.99999))));
            dist = LOWORD(Sf64ToI32(dbl));
            dbl = Sf64From80((Sf80Div(Sf80Div(Sf80From64(dbl), Sf80FromI32(iWarp)), Sf80FromI32(iWarp))));
            cYears = LOWORD(Sf80ToI32((Sf80Add(Sf80From64(dbl), Sf80From64(0.9999)))));
            l = EstFuelUse(lpfl, i, iWarp, -1, FALSE);
        } else {
            cYears = 1;
            l = 0;
        }
        if (cYears > 1) {
            lOneYearUse = EstFuelUse(lpfl, i, iWarp, (int16_t)(iWarp * iWarp), FALSE);
            lFuelGain = (uint32_t)(lOneYearUse * (int16_t)(cYears - 1));
            lFuelGain += EstFuelUse(lpfl, i, iWarp, (int16_t)(dist - iWarp * iWarp * (cYears - 1)), FALSE);
            if (lFuelGain > l) {
                l = lFuelGain;
            }
            lFuelGain = LCalcFuelGainFromRamScoops(lpfl, iWarp, (int16_t)(iWarp * iWarp));
            for (j = 0; j < 16; j++) {
                if (lpfl->rgcsh[j] != 0) {
                    lpshdef = rglpshdef[idPlayer] + j;
                    if (lpshdef->hul.ihuldef == ihuldefFuelTransport || lpshdef->hul.ihuldef == ihuldefSuperFuelXport) {
                        lFuelGain += (uint32_t)(lpfl->rgcsh[j] * 200);
                    }
                }
            }
            if (lFuelGain > 0) {
                if (lOneYearUse <= lFuelGain) {
                    l = lOneYearUse;
                } else {
                    lOneYearUse = (uint32_t)((lOneYearUse - lFuelGain) * (int16_t)(cYears - 1)) + lOneYearUse;
                    if (lOneYearUse < l) {
                        l = lOneYearUse;
                    }
                }
            }
        }
        lCur += l;
        if (lCur > lTot) {
            lTot = lCur;
        }
        if (lpord[i + 1].grobj == grobjPlanet) {
            lppl = LpplFromId(lpord[i + 1].id);
            if (lppl && lppl->iPlayer == idPlayer && lppl->fStarbase && LphuldefFromId(rglpshdefSB[idPlayer][lppl->isb].hul.ihuldef)->hul.wtCargoMax != 0) {
                lCur = 0;
            }
        }
    }
    return lTot;
}

void FleetTransferCargoBalance(FLEET *pflNew1, FLEET *pflNew2) {
    int16_t iplr;
    int32_t rgCargoCapLoss[2];
    int32_t wtCargoXfer;
    int16_t fDeadFleet;
    int32_t wtCargoTot;
    int16_t rgrgcshLoss[2][16];
    int32_t rgrgCargoDelta[2][5];
    int32_t rgFuelCapacity[2];
    FLEET  *rgpflNew[2];
    int16_t wtCargoMax;
    int16_t wtFuelMax;
    int16_t i;
    int32_t lChg;
    int32_t rgFuelCapLoss[2];
    FLEET   rgflCur[2];
    int16_t j;
    SHDEF  *lpshdef;
    int32_t rgCargoCapacity[2];
    int16_t ishdef;
    int32_t l;
    int32_t cshDmgDst;
    int32_t cshDmgSrc;
    int16_t iSrc;
    int32_t pctNew;
    int32_t cshDmgMoved;

    fDeadFleet = FALSE;
    rgpflNew[0] = pflNew1;
    rgpflNew[1] = pflNew2;
    iplr = pflNew1->iPlayer;
    for (i = 0; i < 2; i++) {
        if (rgpflNew[i]->fDead) {
            fDeadFleet = TRUE;
            memset(&rgflCur[i], 0, sizeof(FLEET));
            rgflCur[i].iPlayer = iplr;
        } else {
            FLookupFleet(rgpflNew[i]->id, &rgflCur[i]);
        }
        rgCargoCapLoss[i] = 0;
        rgCargoCapacity[i] = 0;
        rgFuelCapLoss[i] = 0;
        rgFuelCapacity[i] = 0;
        for (j = 0; j < 5; j++) {
            rgrgCargoDelta[i][j] = 0;
        }
    }
    for (ishdef = 0; ishdef < 16; ishdef++) {
        if (rgflCur[0].rgcsh[ishdef] != 0 || rgflCur[1].rgcsh[ishdef] != 0) {
            lpshdef = rglpshdef[iplr] + ishdef;
            wtFuelMax = WtMaxShdefStat(lpshdef, 1);
            wtCargoMax = WtMaxShdefStat(lpshdef, 2);
            for (i = 0; i < 2; i++) {
                rgrgcshLoss[i][ishdef] = rgflCur[i].rgcsh[ishdef] - rgpflNew[i]->rgcsh[ishdef];
                if (rgflCur[i].rgcsh[ishdef] != 0) {
                    rgFuelCapacity[i] += (uint32_t)(rgflCur[i].rgcsh[ishdef] * wtFuelMax);
                    rgCargoCapacity[i] += (uint32_t)(rgflCur[i].rgcsh[ishdef] * wtCargoMax);
                    if (rgrgcshLoss[i][ishdef] > 0) {
                        rgFuelCapLoss[i] += (uint32_t)(rgrgcshLoss[i][ishdef] * wtFuelMax);
                        rgCargoCapLoss[i] += (uint32_t)(rgrgcshLoss[i][ishdef] * wtCargoMax);
                    }
                }
            }
            if (!fDeadFleet && rgrgcshLoss[0][ishdef] == -rgrgcshLoss[1][ishdef] && rgrgcshLoss[0][ishdef] != 0) {
                iSrc = rgrgcshLoss[0][ishdef] < 0;
                if (rgflCur[iSrc].rgcsh[ishdef] > 0) {
                    cshDmgSrc = (int32_t)(rgflCur[iSrc].rgdv[ishdef].pctSh * rgflCur[iSrc].rgcsh[ishdef]) / 100;
                } else {
                    cshDmgSrc = 0;
                }
                if (rgflCur[iSrc == 0].rgcsh[ishdef] > 0) {
                    cshDmgDst = (int32_t)(rgflCur[iSrc == 0].rgdv[ishdef].pctSh * rgflCur[iSrc == 0].rgcsh[ishdef]) / 100;
                } else {
                    cshDmgDst = 0;
                }
                if (cshDmgSrc != 0 && cshDmgDst != 0) {
                    if (cshDmgSrc > rgrgcshLoss[iSrc][ishdef]) {
                        cshDmgMoved = rgrgcshLoss[iSrc][ishdef];
                    } else {
                        cshDmgMoved = cshDmgSrc;
                    }
                    pctNew = (int32_t)((int32_t)((uint32_t)(cshDmgDst * rgflCur[iSrc == 0].rgdv[ishdef].pctDp) +
                                                 (uint32_t)(cshDmgMoved * rgflCur[iSrc].rgdv[ishdef].pctDp) + rgpflNew[iSrc == 0]->rgcsh[ishdef] - 1) /
                                       rgpflNew[iSrc == 0]->rgcsh[ishdef]);
                    rgpflNew[iSrc == 0]->rgdv[ishdef].dp = rgpflNew[iSrc == 0]->rgdv[ishdef].pctSh | (LOWORD(pctNew) & 0x1ff) * 0x80;
                    pctNew = (int32_t)((int32_t)((uint32_t)((cshDmgDst + cshDmgMoved) * 100) + rgpflNew[iSrc == 0]->rgcsh[ishdef] - 1) /
                                       rgpflNew[iSrc == 0]->rgcsh[ishdef]);
                    rgpflNew[iSrc == 0]->rgdv[ishdef].dp = (rgpflNew[iSrc == 0]->rgdv[ishdef].dp & 0xff80) | (LOWORD(pctNew) & 0x7f);
                    if (cshDmgMoved == cshDmgSrc) {
                        rgpflNew[iSrc]->rgdv[ishdef].dp = 0;
                    } else {
                        pctNew = (int32_t)((int32_t)((uint32_t)((cshDmgSrc - cshDmgMoved) * 100) + rgpflNew[iSrc]->rgcsh[ishdef] - 1) /
                                           rgpflNew[iSrc]->rgcsh[ishdef]);
                        rgpflNew[iSrc]->rgdv[ishdef].pctSh = LOWORD(pctNew);
                    }
                } else if (cshDmgSrc != 0) {
                    if (cshDmgSrc > rgrgcshLoss[iSrc][ishdef]) {
                        cshDmgMoved = rgrgcshLoss[iSrc][ishdef];
                    } else {
                        cshDmgMoved = cshDmgSrc;
                    }
                    rgpflNew[iSrc == 0]->rgdv[ishdef].dp = rgpflNew[iSrc == 0]->rgdv[ishdef].pctSh | (rgpflNew[iSrc]->rgdv[ishdef].pctDp & 0x1ff) * 0x80;
                    pctNew = (int32_t)((int32_t)((uint32_t)(cshDmgMoved * 100) + rgpflNew[iSrc == 0]->rgcsh[ishdef] - 1) / rgpflNew[iSrc == 0]->rgcsh[ishdef]);
                    rgpflNew[iSrc == 0]->rgdv[ishdef].dp = (rgpflNew[iSrc == 0]->rgdv[ishdef].dp & 0xff80) | (LOWORD(pctNew) & 0x7f);
                    if (cshDmgMoved == cshDmgSrc) {
                        rgpflNew[iSrc]->rgdv[ishdef].dp = 0;
                    } else {
                        pctNew = (int32_t)((int32_t)((uint32_t)((cshDmgSrc - cshDmgMoved) * 100) + rgpflNew[iSrc]->rgcsh[ishdef] - 1) /
                                           rgpflNew[iSrc]->rgcsh[ishdef]);
                        rgpflNew[iSrc]->rgdv[ishdef].pctSh = LOWORD(pctNew);
                    }
                } else if (cshDmgDst != 0) {
                    pctNew = (int32_t)((int32_t)((uint32_t)(cshDmgDst * 100) + rgpflNew[iSrc == 0]->rgcsh[ishdef] - 1) / rgpflNew[iSrc == 0]->rgcsh[ishdef]);
                    rgpflNew[iSrc == 0]->rgdv[ishdef].dp = (rgpflNew[iSrc == 0]->rgdv[ishdef].dp & 0xff80) | (LOWORD(pctNew) & 0x7f);
                } else {
                    rgpflNew[iSrc == 0]->rgdv[ishdef].dp = rgpflNew[iSrc == 0]->rgdv[ishdef].dp & 0xff80;
                }
            }
        }
    }
    for (i = 0; i < 2; i++) {
        if (rgFuelCapacity[i] != 0) {
            if (rgpflNew[i]->rgwtMin[4] > 45000 || rgFuelCapLoss[i] > 45000) {
                lChg = Sf80ToI32((Sf80Div(Sf80Mul(Sf80FromI32(rgpflNew[i]->rgwtMin[4]), Sf80FromI32(rgFuelCapLoss[i])), Sf80FromI32(rgFuelCapacity[i]))));
            } else {
                lChg = (int32_t)((int32_t)(rgpflNew[i]->rgwtMin[4] * rgFuelCapLoss[i]) / rgFuelCapacity[i]);
            }
            rgrgCargoDelta[i][4] -= lChg;
        }
        if (rgCargoCapacity[i] != 0) {
            wtCargoTot = 0;
            for (j = 0; j <= 3; j++) {
                wtCargoTot += rgpflNew[i]->rgwtMin[j];
            }
            if (wtCargoTot > 45000 || rgCargoCapLoss[i] > 45000) {
                wtCargoXfer = Sf80ToI32((Sf80Div(Sf80Mul(Sf80FromI32(wtCargoTot), Sf80FromI32(rgCargoCapLoss[i])), Sf80FromI32(rgCargoCapacity[i]))));
            } else {
                wtCargoXfer = (int32_t)((int32_t)(wtCargoTot * rgCargoCapLoss[i]) / rgCargoCapacity[i]);
            }
            lChg = wtCargoXfer;
            if (wtCargoXfer != 0 && wtCargoTot != 0) {
                for (j = 0; j <= 3; j++) {
                    if (rgpflNew[i]->rgwtMin[j] > 45000 || wtCargoXfer > 45000) {
                        l = Sf80ToI32((Sf80Div(Sf80Mul(Sf80FromI32(rgpflNew[i]->rgwtMin[j]), Sf80FromI32(wtCargoXfer)), Sf80FromI32(wtCargoTot))));
                    } else {
                        l = (int32_t)((int32_t)(rgpflNew[i]->rgwtMin[j] * wtCargoXfer) / wtCargoTot);
                    }
                    l = l < lChg ? l : lChg;
                    rgrgCargoDelta[i][j] -= l;
                    lChg -= l;
                }
                if (lChg > 0) {
                    for (j = 0; j <= 3 && lChg > 0; j++) {
                        if (rgpflNew[i]->rgwtMin[j] + rgrgCargoDelta[i][j] > 0) {
                            rgrgCargoDelta[i][j]--;
                            lChg--;
                        }
                    }
                }
            }
        }
    }
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 5; j++) {
            rgpflNew[i]->rgwtMin[j] += rgrgCargoDelta[i][j];
            rgpflNew[i == 0]->rgwtMin[j] -= rgrgCargoDelta[i][j];
        }
    }
    return;
}

void DestroyAllIshdefSB(int16_t ishdefSB, int16_t iplr) {
    PLANET *lppl;
    PLANET *lpplMac;

    lppl = lpPlanets;
    lpplMac = lpPlanets + cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->iPlayer == iplr && lppl->fStarbase && lppl->isb == ishdefSB) {
            lppl->fStarbase = FALSE;
            KillQueuedShips(lppl);
            KillQueuedMassPackets(lppl);
        }
    }
    return;
}

void DestroyAllIshdef(int16_t ishdef, int16_t iplr) {
    FLEET   flDead;
    int16_t cKill;
    FLEET  *lpfl;
    int16_t i;
    int16_t grbit;
    int16_t j;
    int16_t cDel;
    FLEET   flNew;

    cDel = 0;
    if (ishdef >= 16) {
        DestroyAllIshdefSB(ishdef - 16, iplr);
        InvalidateReport(rptPlanets, 1);
    } else {
        for (lpfl = *rglpfl, i = 0; i < cFleet; lpfl = rglpfl[i]) {
            if (lpfl->iPlayer != iplr || lpfl->rgcsh[ishdef] <= 0)
                goto IncrementI;
            memset(&flDead, 0, sizeof(FLEET));
            cKill = lpfl->rgcsh[ishdef];
            cDel += cKill;
            for (j = 0; j < 16 && (j == ishdef || lpfl->rgcsh[j] == 0); j++) {
            }
            if (j == 16) {
                lpfl->rgcsh[ishdef] = 0;
                FDeleteFleet(lpfl->id, grobjNone, -1);
                continue;
            }
            flDead.iplr = iplr;
            flDead.fDead = TRUE;
            flDead.rgcsh[ishdef] = cKill;
            flNew = *lpfl;
            flNew.rgcsh[ishdef] = 0;
            FleetTransferCargoBalance(&flNew, &flDead);
            *lpfl = flNew;
            if (sel.grobj == grobjFleet && sel.fl.id == flNew.id) {
                FLookupFleet(flNew.id, &sel.fl);
                ShowScanSel(0);
                FillShipDD(sel.fl.id);
                grbit = -31819;
                FLookupFleet(sel.fl.id, &sel.fl);
                FillFleetCompLB();
                RedrawPlanShip(grbit);
                InvalidateMine();
            }
        IncrementI:
            i++;
        }
        InvalidateReport(rptFleets, 1);
    }
    RemoveIshdefFromAllQueues(ishdef, FALSE);
    return;
}

void RemoveIshdefFromAllQueues(int16_t ishdef, int16_t fSpaceDocks) {
    int16_t iprod;
    PLANET *lppl;
    int16_t iDst;
    PLANET *lpplMac;
    PROD   *lpprod;

    lppl = lpPlanets;
    lpplMac = lpPlanets + cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->lpplprod && lppl->lpplprod->iprodMac != 0 && lppl->iPlayer == idPlayer && lppl->fStarbase &&
            (!fSpaceDocks || rglpshdefSB[idPlayer][lppl->isb].hul.ihuldef == ihuldefSpaceDock)) {
            iDst = 0;
            iprod = 0;
            lpprod = lppl->lpplprod->rgprod;
            while (iprod < lppl->lpplprod->iprodMac) {
                if (lpprod->grobj != grobjFleet || lpprod->iItem != (uint32_t)ishdef) {
                    if (iDst != iprod) {
                        lppl->lpplprod->rgprod[iDst] = *lpprod;
                    }
                    iDst++;
                }
                iprod++;
                lpprod++;
            }
            if (iDst == 0) {
                FreePl((PL *)lppl->lpplprod);
                lppl->lpplprod = NULL;
            } else if (iDst != iprod) {
                lppl->lpplprod->iprodMac = iDst;
            }
        }
    }
    if (sel.grobj == grobjPlanet && sel.pl.lpplprod) {
        FLookupPlanet(sel.pl.id, &sel.pl);
        FillSelProdLB();
    }
    return;
}

int16_t CshQueued(int16_t ishdef, int16_t *pfProgress, int16_t fSpaceDocks) {
    int16_t iprod;
    PLANET *lppl;
    int16_t csh;
    PLANET *lpplMac;
    PROD   *lpprod;

    csh = 0;
    *pfProgress = FALSE;
    lppl = lpPlanets;
    lpplMac = lpPlanets + cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->lpplprod && lppl->lpplprod->iprodMac != 0 && lppl->iPlayer == idPlayer && lppl->fStarbase &&
            (!fSpaceDocks || rglpshdefSB[idPlayer][lppl->isb].hul.ihuldef == ihuldefSpaceDock)) {
            iprod = 0;
            lpprod = lppl->lpplprod->rgprod;
            while (iprod < lppl->lpplprod->iprodMac) {
                if (lpprod->grobj == grobjFleet && lpprod->iItem == (uint32_t)ishdef) {
                    csh += lpprod->cItem;
                    if (lpprod->pct != 0) {
                        *pfProgress = TRUE;
                    }
                }
                iprod++;
                lpprod++;
            }
        }
    }
    return csh;
}

void Merge2Fleets(FLEET *lpflDst, FLEET *lpflDel, int16_t fNoDelete) {
    FLEET   rgfl[2];
    int16_t i;
    int16_t csh;
    int16_t fLeft;

    rgfl[0] = *lpflDst;
    rgfl[1] = *lpflDel;
    fLeft = FALSE;
    for (i = 0; i < 16; i++) {
        /* Ship counts are int16_t. The original added without a limit, so a
           merge past 32767 ships of a design went negative; the ships that
           don't fit stay in the merged fleet. */
        csh = 32767 - rgfl[0].rgcsh[i];
        if (csh > rgfl[1].rgcsh[i]) {
            csh = rgfl[1].rgcsh[i];
        }
        rgfl[0].rgcsh[i] += csh;
        rgfl[1].rgcsh[i] -= csh;
        if (rgfl[1].rgcsh[i] != 0) {
            fLeft = TRUE;
        }
    }
    /* Ships that gated, fought or hit mines this turn can't heal, so the
       fleet they join can't either. The original dropped the merged fleet's
       fNoHeal before HealShips. */
    if (rgfl[1].fNoHeal) {
        rgfl[0].fNoHeal = TRUE;
    }
    FleetTransferCargoBalance(rgfl, &rgfl[1]);
    for (i = 0; i < 2; i++) {
        FLookupFleet(idWriteBack, &rgfl[i]);
    }
    if (fLeft) {
        InvalidateReport(rptFleets, 2);
    } else if (fNoDelete) {
        lpflDel->fDead = TRUE;
    } else {
        FDeleteFleet(rgfl[1].id, grobjFleet, rgfl[0].id);
        InvalidateReport(rptFleets, 2);
    }
    return;
}

void FleetOrdersChangeTarget(FLEET *lpflOld) {
    int16_t    id;
    POINT16    pt;
    int16_t    fChg;
    FLEET     *lpfl;
    int16_t    iord;
    int16_t    iflMac;
    SCAN       scan;
    GrobjClass grobj;

    fChg = FALSE;
    for (iflMac = 0; iflMac < cFleet; iflMac++) {
        lpfl = rglpfl[iflMac];
        if (!rglpfl[iflMac])
            break;
        if (lpfl->lpplord) {
            for (iord = lpfl->cord - 1; iord >= 0; iord--) {
                if (lpfl->lpplord->rgord[iord].grobj == grobjFleet && lpfl->lpplord->rgord[iord].id == lpflOld->id) {
                    if (!fChg) {
                        pt = lpflOld->pt;
                        lpflOld->pt.x++;
                        if (FFindNearestObject(pt, grobjPlanet | grobjFleet | mdExact, &scan)) {
                            if (scan.grobjFull & grobjFleet) {
                                grobj = grobjFleet;
                                id = rglpfl[scan.ifl]->id;
                            } else {
                                grobj = grobjPlanet;
                                id = scan.idpl;
                            }
                        } else {
                            grobj = grobjOther;
                            id = iord;
                        }
                        lpflOld->pt.x--;
                    }
                    lpfl->lpplord->rgord[iord].id = id;
                    lpfl->lpplord->rgord[iord].grobj = grobj;
                }
            }
        }
    }
    return;
}

void GetTruePartCost(int16_t iPlayer, PART *ppart, uint16_t *rgCost) {
    int16_t  cExcess;
    int16_t  cCur;
    int16_t  i;
    COMPART *lpcom;

    lpcom = ppart->pcom;
    for (i = 0; i < 3; i++) {
        rgCost[i] = lpcom->rgwtOreCost[i];
    }
    rgCost[3] = lpcom->resCost;
    if (iPlayer != iplrNone) {
        if ((ppart->hs.grhst & hstTerra) ||
            ((ppart->hs.grhst & hstPlanetary) && ppart->hs.iItem >= iplanetarySDI && ppart->hs.iItem <= iplanetaryNeutronShield) ||
            ((ppart->hs.grhst & hstPlanetary) && ppart->hs.iItem >= iplanetaryViewer50 && ppart->hs.iItem <= iplanetarySnooper620X))
            goto LOtherDiddles;
        cExcess = 100;
        for (i = 0; i < 6; i++) {
            cCur = rgplr[iPlayer].rgTech[i] - lpcom->rgTech[i];
            if (lpcom->rgTech[i] > 0 && cCur < cExcess) {
                cExcess = cCur;
            }
        }
        if (cExcess == 100) {
            for (i = 0; i < 6; i++) {
                if (rgplr[iPlayer].rgTech[i] < cExcess) {
                    cExcess = rgplr[iPlayer].rgTech[i];
                }
            }
        }
        if (cExcess >= 1) {
            if (cExcess > 19) {
                cExcess = 19;
            }
            if (GetRaceGrbit(&rgplr[iPlayer], ibitRaceBleedingEdgeTech) != 0) {
                cExcess = 5 * cExcess;
                if (cExcess > 80) {
                    cExcess = 80;
                }
            } else {
                cExcess *= 4;
                if (cExcess > 75) {
                    cExcess = 75;
                }
            }
            for (i = 0; i < 4; i++) {
                if (rgCost[i] > 0) {
                    rgCost[i] -= LMulDiv(rgCost[i], cExcess, 100);
                    if (rgCost[i] == 0) {
                        rgCost[i] = 1;
                    }
                }
            }
        }
    LOtherDiddles:
        if (ppart->hs.grhst == hstSpecialSB && GetRaceStat(&rgplr[iPlayer], rsMajorAdv) == raStargate &&
            GetRaceStat(&rgplr[iPlayer], rsMajorAdv) == raStargate && ppart->hs.iItem >= ispecialSBStargate100250 &&
            ppart->hs.iItem <= ispecialSBStargateAnyAny) {
            for (i = 0; i < 4; i++) {
                rgCost[i] -= rgCost[i] >> 2;
            }
        } else {
            switch (ppart->hs.grhst) {
            case hstBeam:
            case hstTorp:
            case hstBomb:
                if (GetRaceStat(&rgplr[iPlayer], rsMajorAdv) == raAttack) {
                    for (i = 0; i < 4; i++) {
                        rgCost[i] -= rgCost[i] >> 2;
                    }
                    break;
                }
                /* fallthrough */
            default:
                switch (ppart->hs.grhst) {
                case hstBeam:
                case hstTorp:
                case hstBomb:
                    if (GetRaceStat(&rgplr[iPlayer], rsMajorAdv) == raDefend) {
                        for (i = 0; i < 4; i++) {
                            rgCost[i] += rgCost[i] >> 2;
                        }
                        break;
                    }
                    /* fallthrough */
                default:
                    if (ppart->hs.grhst == hstTerra && GetRaceStat(&rgplr[iPlayer], rsMajorAdv) == raTerra) {
                        rgCost[3] = (uint32_t)rgCost[3] / 2;
                    } else if (ppart->hs.grhst == hstEngine && GetRaceGrbit(&rgplr[iPlayer], ibitRaceCheapEngines) != 0) {
                        for (i = 0; i < 4; i++) {
                            rgCost[i] -= rgCost[i] >> 1;
                        }
                    }
                }
            }
        }
        if (cExcess < 1 && GetRaceGrbit(&rgplr[iPlayer], ibitRaceBleedingEdgeTech) != 0 && !gd.fDontCalcBleed) {
            for (i = 0; i < 6 && lpcom->rgTech[i] <= 0; i++) {
            }
            if (i < 6) {
                gd.fBleedingEdge = TRUE;
                for (i = 0; i < 4; i++) {
                    rgCost[i] *= 2;
                }
            } else {
                gd.fBleedingEdge = FALSE;
            }
        } else {
            gd.fBleedingEdge = FALSE;
        }
    }
    return;
}
