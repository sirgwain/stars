#include "win.h"

void DrawShipOrders(HDC hdc, TILE *ptile, OBJ obj) {
    int16_t swp;
    int16_t dxRight;
    int16_t iWarp;
    int16_t yTop;
    POINT16 pt;
    RECT    rcT;
    int16_t dWrong;
    int32_t lTot;
    int16_t c;
    FLEET  *pfl;
    int16_t xRight;
    int16_t iScanActual;
    RECT    rcGauge;
    char   *psz;
    int16_t xLeft;
    ORDER   ord;
    int32_t l;
    RECT    rc;

    pfl = obj.pfl;
    if (ptile->fFixCtls) {
        rgrcRef[0].top = -5;
        rgrcRef[0].bottom = -6;
        rgrcRef[12].top = -5;
        rgrcRef[12].bottom = -6;
        ShowWindow(hwndShipLB, SW_HIDE);
        ShowWindow(hwndRepCB, SW_HIDE);
        ptile->fFixCtls = FALSE;
    }
    if (!FDrawTileNC(hdc, ptile, &rc, PszGetCompressedString(idsFleetWaypoints))) {
        ShowWindow(hwndShipLB, SW_HIDE);
        ShowWindow(hwndRepCB, SW_HIDE);
    } else {
        xLeft = rc.left + 4;
        xRight = rc.right - 4;
        yTop = (!gd.fSmallTileMode ? 4 : 2) + rc.top;
        rgrcRef[12].top = -5;
        rgrcRef[12].bottom = -6;
        GetClientRect(hwndShipLB, &rcT);
        swp = SWP_NOZORDER | SWP_NOACTIVATE;
        dyShipLB = (dyArial8 + 2) * (!gd.fSmallTileMode ? 4 : 3);
        dWrong = dyShipLB - (rcT.bottom - rcT.top);
        if (dxShipLB == xRight - xLeft && dWrong >= 0 && dWrong < dyArial8) {
            swp |= SWP_NOSIZE;
        } else {
            dxShipLB = xRight - xLeft;
        }
        SetWindowPos(hwndShipLB, NULL, xLeft, yTop, xRight - xLeft, dyShipLB, swp);
        ShowWindow(hwndShipLB, SW_SHOW);
        GetClientRect(hwndShipLB, &rcT);
        dyShipLB = rcT.bottom - rcT.top;
        yTop += (!gd.fSmallTileMode ? 4 : 2) + dyShipLB;
        SelectObject(hdc, rghfontArial8[1]);
        if (!ptile->fMinDraw) {
            rcT.top = yTop;
            rcT.bottom = yTop + dyArial8;
            rcT.left = xLeft;
            rcT.right = xRight;
            FillRect(hdc, &rcT, hbrButtonFace);
        }
        c = CchGetString(idsComing, szWork);
        l = GetTextExtent(hdc, szWork, c);
        c = CchGetString(idsWayPt, szWork);
        lTot = GetTextExtent(hdc, szWork, c);
        if (lTot > l) {
            l = lTot;
        }
        dxRight = xRight - xLeft - LOWORD(l);
        if (!ptile->fMinDraw) {
            c = CchGetString(sel.iwpAct <= 0 ? idsWayPt : idsComing, szWork);
            TextOut(hdc, xLeft, yTop, szWork, c);
        }
        SelectObject(hdc, rghfontArial8[0]);
        if (sel.iwpAct > 0) {
            ord = sel.fl.lpplord->rgord[sel.iwpAct - 1];
            psz = PszGetLocName(ord.grobj, ord.id, ord.pt.x, ord.pt.y);
            iScanActual = sel.iwpAct;
        } else if (sel.fl.cord <= 1) {
            psz = "";
        } else {
            ord = sel.fl.lpplord->rgord[1];
            psz = PszGetLocName(ord.grobj, ord.id, ord.pt.x, ord.pt.y);
            ord = sel.fl.lpplord->rgord[0];
            iScanActual = 1;
        }
        RightTextOut(hdc, xRight, yTop, psz, 0, dxRight);
        yTop += dyArial8;
        if (sel.fl.cord > 1 && sel.iwpAct != 0)
            goto DoDistance;
        rgrcRef[0].top = -5;
        rgrcRef[0].bottom = -6;
        if (sel.fl.cord <= 1) {
            SetRect(&rc, xLeft - 1, yTop, xRight - 1, dyArial8 * 4 + yTop);
            FillRect(hdc, &rc, hbrButtonFace);
            yTop += (dyArial8 - gd.fSmallTileMode) * 4 + gd.fSmallTileMode * 2;
            goto DoCheckBox;
        }
    DoDistance:
        if (!ptile->fMinDraw) {
            SelectObject(hdc, rghfontArial8[1]);
            c = CchGetString(idsDistance, szWork);
            TextOut(hdc, xLeft, yTop, szWork, c);
        }
        SelectObject(hdc, rghfontArial8[0]);
        pt = sel.fl.lpplord->rgord[iScanActual].pt;
        RightTextOut(hdc, xRight, yTop, PszGetDistance(ord.pt.x, ord.pt.y, pt.x, pt.y), 0, dxRight);
        yTop += dyArial8 - gd.fSmallTileMode;
        SelectObject(hdc, rghfontArial8[1]);
        if (!ptile->fMinDraw) {
            c = CchGetString(idsWarpFactor, szWork);
            TextOut(hdc, xLeft, yTop, szWork, c);
        }
        iWarp = sel.fl.lpplord->rgord[iScanActual].iWarp;
        if (sel.iwpAct != 0) {
            SetRect(&rcGauge, xLeft + LOWORD(l), yTop, xRight - 1, yTop + dyArial8);
            rgrcRef[0] = rcGauge;
            DrawFleetGauge(hdc, &rcGauge, NULL, 6);
        } else {
            SelectObject(hdc, rghfontArial8[0]);
            if (iWarp < 11) {
                c = wsprintf(szWork, PszGetCompressedString(idsWarpD2), iWarp);
            } else {
                c = CchGetString(idsUseStargate, szWork);
            }
            RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
        }
        yTop += dyArial8;
        if (!ptile->fMinDraw) {
            SelectObject(hdc, rghfontArial8[1]);
            c = CchGetString(idsTravelTime, szWork);
            TextOut(hdc, xLeft, yTop, szWork, c);
        }
        SelectObject(hdc, rghfontArial8[0]);
        c = CchGetETA(hdc, pfl, szWork, iScanActual, FALSE);
        RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
        SetTextColor(hdc, 0);
        yTop += dyArial8 - gd.fSmallTileMode;
        if (!ptile->fMinDraw) {
            SelectObject(hdc, rghfontArial8[1]);
            c = CchGetString(idsEstFuelUsage, szWork);
            TextOut(hdc, xLeft, yTop, szWork, c);
        }
        SelectObject(hdc, rghfontArial8[0]);
        lTot = LFuelUseToWaypoint(&sel.fl, iScanActual, FALSE);
        c = wsprintf(szWork, PszGetCompressedString(idsLdmg), lTot);
        if (lTot > sel.fl.rgwtMin[4]) {
            SetTextColor(hdc, 0xff);
        }
        RightTextOut(hdc, xRight, yTop, szWork, c, dxRight - 20);
        if (lTot > sel.fl.rgwtMin[4]) {
            SetTextColor(hdc, 0);
        }
        yTop += dyArial8;
    DoCheckBox:
        SendMessage(hwndRepCB, BM_SETCHECK, sel.fl.fRepOrders, 0);
        SetWindowPos(hwndRepCB, NULL, xLeft, yTop, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        ShowWindow(hwndRepCB, SW_SHOW);
        SetRect(&rgrcRef[12], xRight - (dyArial8 | 1), yTop, xRight, (dyArial8 | 1) + yTop);
        DrawDiamond(hdc, &rgrcRef[12], hbrBBlue);
    }
    return;
}

void DrawShipWayPtOrders(HDC hdc, TILE *ptile, OBJ obj) {
    int16_t  dxKt;
    int16_t  dxT;
    int16_t  swp;
    int16_t  dxRight;
    int16_t  yTop;
    int16_t  yTopMsg;
    StringId ids;
    int16_t  edWid;
    PLANET  *lppl;
    ORDER   *lpord;
    FLEET   *pfl;
    int16_t  i;
    int16_t  fActive;
    int16_t  xRight;
    TaskType grtask;
    char     szT[8];
    int16_t  yBot;
    int16_t  dxRight2;
    char    *psz;
    int16_t  cch;
    int16_t  xLeft;
    int32_t  l;
    RECT     rc;
    RECT     rcT;
    char    *pszT;
    int16_t  j;
    int32_t  cMine;
    int16_t  dyCur;
    int16_t  c;
    int32_t  rgl[4];

    if (ptile->fFixCtls) {
        for (i = 0; i < 3; i++) {
            ShowWindow(rghwndOrderDD[i], SW_HIDE);
        }
        ShowWindow(hwndOrderED, SW_HIDE);
        ptile->fFixCtls = FALSE;
        rgrcRef[5].top = -5;
        rgrcRef[5].bottom = -6;
        rgrcRef[18].top = -5;
        rgrcRef[18].bottom = -6;
    }
    if (!FDrawTileNC(hdc, ptile, &rc, PszGetCompressedString(idsWaypointTask))) {
        for (i = 0; i < 3; i++) {
            ShowWindow(rghwndOrderDD[i], SW_HIDE);
        }
        ShowWindow(hwndOrderED, SW_HIDE);
    } else {
        pfl = obj.pfl;
        xLeft = rc.left + 4;
        xRight = rc.right - 4;
        yTop = (!gd.fSmallTileMode ? 4 : 2) + rc.top;
        yBot = rc.bottom - 4;
        dxRight = xRight - xLeft;
        rgrcRef[5].top = -5;
        rgrcRef[5].bottom = -6;
        swp = SWP_NOZORDER | SWP_NOACTIVATE;
        if (rgdxOrderDD[0] == dxRight) {
            swp |= SWP_NOSIZE;
        } else {
            rgdxOrderDD[0] = dxRight;
        }
        SetWindowPos(rghwndOrderDD[0], NULL, xLeft, yTop, dxRight, 10 * dyArial8 + dyShipDD, swp);
        ShowWindow(rghwndOrderDD[0], SW_SHOW);
        yTop += dyShipDD + 3;
        yTopMsg = yTop;
        l = SendMessage(rghwndOrderDD[0], CB_GETCURSEL, 0, 0);
        grtask = LOWORD(l);
        if (IsWindowVisible(rghwndOrderDD[1]) == 0) {
            SetRect(&rc, xLeft - 1, yTop, xRight + 1, yBot + 2);
            FillRect(hdc, &rc, hbrButtonFace);
        } else if (IsWindowVisible(rghwndOrderDD[2]) == 0) {
            SetRect(&rc, xLeft - 1, yTop + dyShipDD + 3, xRight + 1, yBot + 2);
            FillRect(hdc, &rc, hbrButtonFace);
        }
        switch (grtask) {
        case grTaskXfer:
        case grTaskLayMines:
        case grTaskPatrol:
        case grTaskGive:
            swp = SWP_NOZORDER | SWP_NOACTIVATE;
            switch (grtask) {
            case grTaskXfer:
                dxRight2 = dxRight - dyShipDD + 2;
                break;
            case grTaskPatrol:
                SelectObject(hdc, rghfontArial8[1]);
                psz = PszGetCompressedString(idsWarpFactor);
                cch = strlen(psz);
                dxT = LOWORD(GetTextExtent(hdc, psz, cch)) + 2;
                psz = PszGetCompressedString(idsIntercept);
                cch = strlen(psz);
                SetRect(&rc, xLeft, yTop + 4, xLeft + dxT, yBot);
                FillRect(hdc, &rc, hbrButtonFace);
                DrawText(hdc, psz, cch, &rc, DT_NOPREFIX);
                dxRight2 = dxRight - dxT - 2;
                xLeft += dxT + 2;
                break;
            case grTaskGive:
                SelectObject(hdc, rghfontArial8[1]);
                cch = CchGetString(idsTo3, szT);
                szT[cch] = ' ';
                cch++;
                szT[cch] = 0;
                /* NATIVE: original measured psz before it was set; see WIN16-PARITY.md */
                dxT = LOWORD(GetTextExtent(hdc, szT, cch)) + 2;
                SetRect(&rc, xLeft, yTop + 4, xLeft + dxT, yBot);
                FillRect(hdc, &rc, hbrButtonFace);
                DrawText(hdc, szT, cch, &rc, DT_NOPREFIX);
                dxRight2 = dxRight - dxT - 2;
                xLeft += dxT + 2;
                break;
            default:
                dxRight2 = dxRight;
            }
            if (rgdxOrderDD[1] == dxRight2) {
                swp |= SWP_NOSIZE;
            } else {
                rgdxOrderDD[1] = dxRight2;
            }
            SetWindowPos(rghwndOrderDD[1], NULL, xLeft, yTop, dxRight2, 6 * dyShipDD, swp);
            ShowWindow(rghwndOrderDD[1], SW_SHOW);
            if (grtask == grTaskXfer) {
                SetRect(&rcT, xLeft + dxRight - (dyShipDD | 1) + 8, yTop + 3, xLeft + dxRight, (dyShipDD | 1) + yTop - 5);
                rgrcRef[5] = rcT;
                DrawDiamond(hdc, &rcT, hbrBBlue);
                break;
            }
            if (grtask != grTaskPatrol)
                break;
            yTop += dyShipDD + 4;
            SetRect(&rcT, xLeft - dxT - 2, yTop, xRight - 1, yTop + dyArial8);
            SelectObject(hdc, rghfontArial8[1]);
            psz = PszGetCompressedString(idsWarpFactor);
            cch = strlen(psz);
            DrawText(hdc, psz, cch, &rcT, DT_NOPREFIX);
            rcT.left += dxT + 2;
            rgrcRef[18] = rcT;
            DrawFleetGauge(hdc, &rcT, NULL, 7);
            break;
        default:
            if (IsWindowVisible(rghwndOrderDD[1]) != 0) {
                ShowWindow(rghwndOrderDD[1], SW_HIDE);
                SetRect(&rc, xLeft - 1, yTop, xRight + 1, yBot + 2);
                FillRect(hdc, &rc, hbrButtonFace);
            }
        }
        yTop += dyShipDD + 3;
        lpord = &sel.fl.lpplord->rgord[sel.iwpAct];
        if (lpord->grobj == grobjPlanet) {
            lppl = LpplFromId(lpord->id);
        } else {
            lppl = NULL;
        }
        if (grtask == grTaskXfer) {
            dxKt = 0;
            for (i = 0; i < 5; i++) {
                if ((int16_t)LOWORD(GetTextExtent(hdc, vrgszUnits[i], 2)) > dxKt) {
                    dxKt = LOWORD(GetTextExtent(hdc, vrgszUnits[i], 2));
                }
            }
            i = LOWORD(SendMessage(rghwndOrderDD[1], CB_GETCURSEL, 0, 0));
            if (i == 0) {
                i = 4;
            } else {
                i--;
            }
            SelectObject(hdc, rghfontArial8[1]);
            edWid = LOWORD(GetTextExtent(hdc, PszGetCompressedString(idsN99999Kt), 9));
            dxRight -= edWid + 8;
            swp = SWP_NOZORDER | SWP_NOACTIVATE;
            if (rgdxOrderDD[2] == dxRight) {
                swp |= SWP_NOSIZE;
            } else {
                rgdxOrderDD[2] = dxRight;
            }
            SetWindowPos(rghwndOrderDD[2], NULL, xLeft, yTop, dxRight, 9 * dyShipDD, swp);
            ShowWindow(rghwndOrderDD[2], SW_SHOW);
            l = SendMessage(rghwndOrderDD[2], CB_GETCURSEL, 0, 0);
            fActive = TRUE;
            switch (l) {
            case 0:
            case 1:
            case 2:
            case 7:
                fActive = FALSE;
                break;
            case 5:
            case 6:
                i = 5;
            }
            psz = vrgszUnits[i];
            swp = SWP_NOZORDER | SWP_NOACTIVATE;
            if (dxOrderED == edWid - dxKt) {
                swp |= SWP_NOSIZE;
            } else {
                dxOrderED = edWid - dxKt;
            }
            SetWindowPos(hwndOrderED, NULL, xRight - edWid - 4, yTop, edWid - dxKt, dyShipDD, swp);
            EnableWindow(hwndOrderED, fActive);
            ShowWindow(hwndOrderED, SW_SHOW);
            SetRect(&rc, xRight - dxKt - 1, yTop + 2, xRight - 1, yBot);
            FillRect(hdc, &rc, hbrButtonFace);
            if (fActive) {
                DrawText(hdc, psz, 2, &rc, DT_NOPREFIX);
            }
        } else {
            ShowWindow(rghwndOrderDD[2], SW_HIDE);
            ShowWindow(hwndOrderED, SW_HIDE);
        }
        switch (grtask) {
        case grTaskScrap:
            ids = idsNoteShipsFleetWillDismantledMineralsCan;
            goto LDisplayMsg;
        case grTaskMerge:
            if (sel.fl.lpplord->rgord[sel.iwpAct].grobj == grobjFleet)
                break;
            ids = idsWarningDestinationWaypointFleetMergeWillSucessfu;
            psz = PszGetCompressedString(ids);
            SetTextColor(hdc, 127);
            SetRect(&rc, xLeft, yTopMsg, xRight, yBot + 2);
            SelectObject(hdc, rghfontArial7[0]);
            DrawText(hdc, psz, strlen(psz), &rc, DT_WORDBREAK | DT_NOPREFIX);
            SetTextColor(hdc, crButtonText);
            break;
        case grTaskLayMines:
            yTopMsg = yTop;
            l = CLayMinesFromLpfl(&sel.fl, mineAll, ishdefAll);
            if (l <= 0) {
                ids = idsWarningFleetHasMineLayingPods;
                goto LDisplayMsg;
            }
            ids = idsFleetCanLayLdMinesPerYear;
            pszT = PszGetCompressedString(ids);
            wsprintf(szWork, pszT, l);
            psz = szWork;
            goto LDisplayMsg2;
        case grTaskColonize:
            fActive = FALSE;
            for (i = 0; i < 16; i++) {
                if (sel.fl.rgcsh[i] > 0) {
                    for (j = 0; j < rglpshdef[sel.fl.iPlayer][i].hul.chs; j++) {
                        if (rglpshdef[sel.fl.iPlayer][i].hul.rghs[j].grhst == hstSpecialM &&
                            (rglpshdef[sel.fl.iPlayer][i].hul.rghs[j].iItem == ispecialMColonizationModule ||
                             rglpshdef[sel.fl.iPlayer][i].hul.rghs[j].iItem == ispecialMOrbitalConstructionModule)) {
                            fActive = TRUE;
                            goto FoundColony;
                        }
                    }
                }
            }
        FoundColony:
            if (fActive) {
                if (sel.fl.rgwtMin[3] == 0) {
                    ids = idsRememberLoadColonistsBeforeEmbarkingMission;
                    goto LDisplayMsg;
                }
                ids = idsNoteShipsFleetWillDismantledProvideSupplies;
                goto LDisplayMsg;
            }
            ids = idsWarningColonizeMissionCannotCarriedBecauseNone;
        LDisplayMsg:
            psz = PszGetCompressedString(ids);
        LDisplayMsg2:
            SetTextColor(hdc, ids == idsNoteShipsFleetWillDismantledProvideSupplies ? crButtonText : 127);
            SetRect(&rc, xLeft, yTopMsg, xRight, yBot + 2);
            SelectObject(hdc, rghfontArial7[0]);
            DrawText(hdc, psz, strlen(psz), &rc, DT_WORDBREAK | DT_NOPREFIX);
            SetTextColor(hdc, crButtonText);
            break;
        case grTaskMine:
            cMine = CMineFromLpfl(&sel.fl);
            if (cMine > 0) {
                if (lppl && (lppl->iPlayer == iplrNone || (lppl->iPlayer == sel.fl.iPlayer && GetRaceStat(&rgplr[idPlayer], rsMajorAdv) == raMacintosh))) {
                    if (dyArial8 > 14) {
                        SelectObject(hdc, rghfontArial7[0]);
                        dyCur = dyArial7;
                    } else {
                        SelectObject(hdc, rghfontArial8[0]);
                        dyCur = dyArial8;
                    }
                    if (lppl->det <= detMinimal) {
                        ids = idsPlanetaryDataAvailableEstimateMineralMiningRates;
                        goto ShowString;
                    }
                    EstMineralsMined(lppl, rgl, cMine, FALSE);
                    c = CchGetString(idsMiningRatePerYear, szWork);
                    TextOut(hdc, xLeft, yTopMsg, szWork, c);
                    yTopMsg += dyCur;
                    dxRight = xLeft;
                    for (i = 0; i < 3; i++) {
                        SetTextColor(hdc, rgcrMinerals[i]);
                        c = wsprintf(szWork, PCTLD, rgl[i]);
                        DxStreamTextOut(hdc, &dxRight, yTopMsg, szWork, c, TRUE);
                        SetTextColor(hdc, crButtonText);
                        DxStreamTextOut(hdc, &dxRight, yTopMsg, "kT  ", 4, TRUE);
                    }
                    goto DoneMine;
                }
                ids = idsNoteCanMineUninhabitedPlanets;
            } else {
                ids = idsWarningFleetContainsShipsRemoteMiningModules;
            }
            if (dyArial8 > 14) {
                SelectObject(hdc, rghfontArial6[0]);
            } else {
                SelectObject(hdc, rghfontArial7[0]);
            }
        ShowString:
            SetTextColor(hdc, ids == 229 ? crButtonText : 127);
            psz = PszGetCompressedString(ids);
            SetRect(&rc, xLeft, yTopMsg, xRight, yBot + 2);
            DrawText(hdc, psz, strlen(psz), &rc, DT_WORDBREAK | DT_NOPREFIX);
        DoneMine:
            SetTextColor(hdc, crButtonText);
        }
        return;
    }
    return;
}

void DrawShipPlanet(HDC hdc, TILE *ptile, OBJ obj) {
    int16_t yTop;
    int16_t dy;
    int16_t i;
    int16_t xRight;
    char   *psz;
    int16_t dx;
    int16_t xLeft;
    RECT    rc;
    THING  *lpth;
    THING  *lpthMac;

    if (obj.pfl->idPlanet != idPlanetDeepSpace) {
        psz = PszGetPlanetName(obj.pfl->idPlanet | 0x8000);
    } else {
        psz = PszGetCompressedString(idsDeepSpace2);
    }
    if (ptile->fFixCtls) {
        ShowWindow(rghwndBtn[3], SW_HIDE);
        ShowWindow(rghwndBtn[7], SW_HIDE);
        ptile->fFixCtls = FALSE;
    }
    if (!FDrawTileNC(hdc, ptile, &rc, psz)) {
        ShowWindow(rghwndBtn[3], SW_HIDE);
        ShowWindow(rghwndBtn[7], SW_HIDE);
    } else {
        xLeft = rc.left + 4;
        xRight = rc.right - 4;
        yTop = (!gd.fSmallTileMode ? 4 : 1) + rc.top;
        dx = (int16_t)(xRight - xLeft - 16) / 3;
        dy = 3 * dyArial8 >> 1;
        EnableWindow(rghwndBtn[3], obj.pfl->idPlanet != idPlanetDeepSpace && sel.pl.iPlayer == idPlayer);
        SetWindowText(rghwndBtn[7], PszGetCompressedString(obj.pfl->idPlanet == idPlanetDeepSpace ? idsJettison2 : idsXFer));
        if (obj.pfl->idPlanet == idPlanetDeepSpace) {
            lpth = lpThings;
            lpthMac = lpThings + cThing;
            for (; lpth < lpthMac && (lpth->ith != ithMineralPacket || obj.pfl->pt.x != lpth->pt.x || obj.pfl->pt.y != lpth->pt.y); lpth++) {
            }
            EnableWindow(rghwndBtn[7], lpth == lpthMac);
        } else {
            EnableWindow(rghwndBtn[7], TRUE);
        }
        if (!ptile->fMinDraw) {
            i = 3;
            while (i <= 7) {
                SetWindowPos(rghwndBtn[i], NULL, xLeft, yTop, dx, dy, SWP_NOZORDER | SWP_NOACTIVATE);
                ShowWindow(rghwndBtn[i], SW_SHOW);
                i += 4;
                xLeft += dx * 2 + 16;
            }
        }
    }
    return;
}

void DrawShipCargo(HDC hdc, TILE *ptile, OBJ obj) {
    int16_t dxRight;
    int32_t l2;
    int16_t yTop;
    int16_t i;
    int16_t c;
    FLEET  *pfl;
    int16_t xRight;
    RECT    rcGauge;
    int16_t xLeft;
    int32_t l;
    RECT    rc;

    pfl = obj.pfl;
    if (ptile->fFixCtls) {
        rgrcRef[2].top = -5;
        rgrcRef[2].bottom = -6;
        rgrcRef[3].top = -5;
        rgrcRef[3].bottom = -6;
    }
    if (FDrawTileNC(hdc, ptile, &rc, PszGetCompressedString(idsFuelCargo))) {
        xLeft = rc.left + 4;
        xRight = rc.right - 4;
        yTop = rc.top + 1;
        dxRight = dxMaxMineralQuan;
        SelectObject(hdc, rghfontArial8[1]);
        c = CchGetString(idsCargo3, szWork);
        l = GetTextExtent(hdc, szWork, c);
        c = CchGetString(idsFuel3, szWork);
        l2 = GetTextExtent(hdc, szWork, c);
        if (l2 > l) {
            l = l2;
        }
        if (!ptile->fMinDraw) {
            TextOut(hdc, xLeft, yTop, szWork, c);
        }
        SetRect(&rcGauge, xLeft + LOWORD(l), yTop, xRight, yTop + dyArial8);
        rgrcRef[2] = rcGauge;
        DrawFleetGauge(hdc, &rcGauge, pfl, 4);
        yTop += (!gd.fSmallTileMode ? 4 : 2) + dyArial8;
        if (!ptile->fMinDraw) {
            c = CchGetString(idsCargo3, szWork);
            TextOut(hdc, xLeft, yTop, szWork, c);
        }
        SetRect(&rcGauge, xLeft + LOWORD(l), yTop, xRight, yTop + dyArial8);
        rgrcRef[3] = rcGauge;
        DrawFleetGauge(hdc, &rcGauge, pfl, 5);
        yTop += dyArial8 + 4;
        if (!gd.fSmallTileMode) {
            for (i = 0; i <= 2; i++) {
                if (!ptile->fMinDraw) {
                    SelectObject(hdc, rghfontArial8[1]);
                    SetTextColor(hdc, rgcrMinerals[i]);
                    TextOut(hdc, xLeft, yTop, rgszMinerals[i], lstrlen(rgszMinerals[i]));
                }
                SelectObject(hdc, rghfontArial8[0]);
                SetTextColor(hdc, crButtonText);
                c = wsprintf(szWork, PszGetCompressedString(idsLdkt), pfl->rgwtMin[i]);
                RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
                yTop += dyArial8;
            }
            if (!ptile->fMinDraw) {
                SelectObject(hdc, rghfontArial8[1]);
                SetTextColor(hdc, 0xffffff);
                c = CchGetString(idsColonists2, szWork);
                TextOut(hdc, xLeft, yTop, szWork, c);
                SelectObject(hdc, rghfontArial8[0]);
                SetTextColor(hdc, crButtonText);
            }
            c = wsprintf(szWork, PszGetCompressedString(idsLdkt), pfl->rgwtMin[3]);
            RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
            yTop += dyArial8;
        }
    }
    return;
}

void DrawFleetComp(HDC hdc, TILE *ptile, OBJ obj) {
    int32_t cBoat;
    int16_t swp;
    int16_t dxRight;
    int16_t yTop;
    RECT    rcT;
    int16_t dyWrong;
    int16_t c;
    int16_t i;
    FLEET  *pfl;
    int16_t xStart;
    int16_t xRight;
    int16_t dxLabel;
    int16_t xLeft;
    int32_t l;
    RECT    rc;

    pfl = obj.pfl;
    if (ptile->fFixCtls) {
        ShowWindow(hwndFleetCompLB, SW_HIDE);
        ShowWindow(rghwndBtn[8], SW_HIDE);
        ShowWindow(rghwndBtn[9], SW_HIDE);
        ShowWindow(rghwndBtn[10], SW_HIDE);
        ShowWindow(hwndBattleDD, SW_HIDE);
        ptile->fFixCtls = FALSE;
    }
    if (!FDrawTileNC(hdc, ptile, &rc, PszGetCompressedString(idsFleetComposition))) {
        ShowWindow(hwndFleetCompLB, SW_HIDE);
        ShowWindow(rghwndBtn[8], SW_HIDE);
        ShowWindow(rghwndBtn[9], SW_HIDE);
        ShowWindow(rghwndBtn[10], SW_HIDE);
        ShowWindow(hwndBattleDD, SW_HIDE);
    } else {
        xLeft = rc.left + 4;
        xRight = rc.right - 4;
        yTop = rc.top + 3;
        GetClientRect(hwndFleetCompLB, &rcT);
        swp = SWP_NOZORDER | SWP_NOACTIVATE;
        dyFleetCompLB = (dyArial8 + 2) * (!gd.fSmallTileMode ? 5 : 3);
        dyWrong = dyFleetCompLB - (rcT.bottom - rcT.top);
        if (dxFleetCompLB == xRight - xLeft && dyWrong >= 0 && dyWrong < dyArial8) {
            swp |= SWP_NOSIZE;
        } else {
            dxFleetCompLB = xRight - xLeft;
        }
        SetWindowPos(hwndFleetCompLB, NULL, xLeft, yTop, xRight - xLeft, dyFleetCompLB, swp);
        ShowWindow(hwndFleetCompLB, SW_SHOW);
        GetClientRect(hwndFleetCompLB, &rcT);
        dyFleetCompLB = rcT.bottom - rcT.top;
        yTop += (!gd.fSmallTileMode ? 4 : 2) + dyFleetCompLB;
        SelectObject(hdc, rghfontArial8[1]);
        c = CchGetString(idsBattlePlan2, szWork);
        dxLabel = LOWORD(GetTextExtent(hdc, szWork, c));
        if (!ptile->fMinDraw) {
            TextOut(hdc, xLeft, yTop + 4, szWork, c);
        }
        swp = SWP_NOZORDER | SWP_NOACTIVATE;
        if (dxBattleDD == xRight - xLeft - dxLabel) {
            swp |= SWP_NOSIZE;
        } else {
            dxBattleDD = xRight - xLeft - dxLabel;
        }
        SetWindowPos(hwndBattleDD, NULL, xLeft + dxLabel, yTop, dxBattleDD, 5 * dyShipDD, swp);
        ShowWindow(hwndBattleDD, SW_SHOW);
        yTop += dyShipDD + 3;
        c = CchGetString(idsEstRange, szWork);
        l = GetTextExtent(hdc, szWork, c);
        if (!ptile->fMinDraw) {
            TextOut(hdc, xLeft, yTop, szWork, c);
        }
        SelectObject(hdc, rghfontArial8[0]);
        c = CchGetString(idsN9999LY, szWork);
        dxRight = LOWORD(GetTextExtent(hdc, szWork, c)) + 6;
        i = IFindIdealWarp(NULL, FALSE);
        l = EstFuelUse(&sel.fl, 0, i, -1, TRUE);
        if (l >= 1000000000) {
            c = CchGetString(idsInfinite, szWork);
        } else {
            c = wsprintf(szWork, PszGetCompressedString(idsLdLY), l);
        }
        RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
        if (!gd.fSmallTileMode) {
            yTop += dyArial8;
            if (!ptile->fMinDraw) {
                SelectObject(hdc, rghfontArial8[1]);
                c = CchGetString(idsPercentCloaked, szWork);
                TextOut(hdc, xLeft, yTop, szWork, c);
            }
            SelectObject(hdc, rghfontArial8[0]);
            i = PctCloakFromLpfl(&sel.fl);
            if (i == 0) {
                c = CchGetString(idsNone2, szWork);
            } else {
                c = wsprintf(szWork, PCTDPCTPCT, i);
            }
            RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
        }
        yTop += dyArial8 + 2 - gd.fSmallTileMode;
        xStart = xLeft;
        c = (int16_t)(xRight - xLeft - 10) / 3;
        i = 8;
        while (i <= 10) {
            SetWindowPos(rghwndBtn[i], NULL, xStart, yTop, c, (dyArial8 >> 1) + dyArial8, SWP_NOZORDER | SWP_NOACTIVATE);
            ShowWindow(rghwndBtn[i], SW_SHOW);
            i++;
            xStart += c + 6;
        }
        cBoat = 0;
        for (i = 0; i < 16; i++) {
            cBoat += sel.fl.rgcsh[i];
        }
        EnableWindow(rghwndBtn[8], FCanSplit(cBoat));
        EnableWindow(rghwndBtn[9], FCanSplitAll(cBoat));
        EnableWindow(rghwndBtn[10], FCanMerge(pfl));
    }
    return;
}

void ShipCommandProc(HWND hwnd, WPARAM wParam, LPARAM lParam) {
    int16_t fPercent;
    FARPROC lpProc;
    int32_t lSel;
    XFER    xf;
    char    szT[34];
    int32_t lMin;
    int16_t ishdef;
    int16_t grbit;
    int16_t ifl;
    FLEET  *lpfl;
    int16_t rgifl[512];
    int16_t ish;
    int16_t ishPrimary;
    FLEET  *lpflBest;
    char    rgb[8];
    int16_t i;
    int16_t iInit;

    fPercent = FALSE;
    if (HIWORD(wParam) == 0) {
        SetFocus(hwndFrame);
    }
    if ((HWND)lParam == rghwndBtn[4] && HIWORD(wParam) == 0) {
        SelectAdjFleet(-1, 0);
    } else if ((HWND)lParam == rghwndBtn[5] && HIWORD(wParam) == 0) {
        SelectAdjFleet(1, 0);
    } else if ((HWND)lParam == rghwndBtn[6] && HIWORD(wParam) == 0) {
        strcpy(szWork, PszGetFleetName(sel.fl.id));
        StickyDlgPos(hwnd, &ptStickyRenameDlg, FALSE);
        lpProc = MakeProcInstance(RenameDlg, hInst);
        if (DialogBox(hInst, MAKEINTRESOURCE(IDD_RENAME), hwndFrame, lpProc) != 0) {
            FreeProcInstance(lpProc);
            strcpy(szT, szWork);
            if (strcmp(szT, PszGetFleetName(sel.fl.id)) != 0) {
                LogChangeName(grobjFleet, sel.fl.id, szT);
                InvalidateReport(rptFleets, 1);
                FillOrdersLB();
                DrawPlanShip(NULL, tileFleetOrders | tileBitmap | tileErase);
                InvalidateRect(hwndMessage, NULL, TRUE);
                InvalidateRect(hwndScanner, NULL, TRUE);
                SetMineralTitleBar(hwndMine);
            }
        } else {
            FreeProcInstance(lpProc);
        }
    } else if ((HWND)lParam == rghwndBtn[3] && HIWORD(wParam) == 0) {
        SelectAdjPlanet(0, sel.fl.idPlanet);
        SetFleetDropDownSel(sel.fl.id);
    } else if ((HWND)lParam == rghwndBtn[7] && HIWORD(wParam) == 0) {
        if (sel.fl.idPlanet != idPlanetDeepSpace) {
            TransferStuff(sel.fl.id, grobjFleet, sel.fl.idPlanet, grobjPlanet, mdXferCargo);
        } else {
            TransferStuff(sel.fl.id, grobjFleet, -1, grobjOther, mdXferCargo);
        }
    } else if ((HWND)lParam == hwndShipDD) {
        if (HIWORD(wParam) == 1) {
            DrawPlanShip(NULL, tileShipList | tileErase);
        }
    } else if ((HWND)lParam == hwndShipLB) {
        if (HIWORD(wParam) == 1) {
            lSel = SendMessage(hwndShipLB, LB_GETCURSEL, 0, 0);
            SetScanWp(LOWORD(lSel));
        }
    } else if ((HWND)lParam == hwndFleetCompLB) {
        if (HIWORD(wParam) == 1) {
            lSel = SendMessage(hwndFleetCompLB, LB_GETCURSEL, 0, 0);
            if (lSel >= 0) {
                for (ishdef = 0; ishdef < 16 && (sel.fl.rgcsh[ishdef] <= 0 || lSel-- > 0); ishdef++) {
                }
                GlobalPD.grPopup = grPopupShdef;
                GlobalPD.lpshdef = &rgshdef[ishdef];
                GlobalPD.fHideCounts = FALSE;
                GlobalPD.fShowDamage = TRUE;
                GlobalPD.fToken = FALSE;
                GlobalPD.fSummary = FALSE;
                Popup(hwndFleetCompLB, 10, 10);
            }
        }
    } else if ((HWND)lParam == hwndBattleDD) {
        if (HIWORD(wParam) == 1) {
            lSel = SendMessage(hwndBattleDD, CB_GETCURSEL, 0, 0);
            if (lSel != -1) {
                if (lSel == 0) {
                    lpProc = MakeProcInstance(BattlePlansDlg, hInst);
                    lSel = DialogBox(hInst, MAKEINTRESOURCE(IDD_BATTLE_PLANS), hwndFrame, lpProc);
                    FreeProcInstance(lpProc);
                    sel.fl.iplan = lSel;
                } else {
                    sel.fl.iplan = lSel - 1;
                }
                FLookupFleet(idWriteBack, &sel.fl);
            }
        }
    } else if ((HWND)lParam == rghwndBtn[0] && HIWORD(wParam) == 0) {
        lSel = SendMessage(hwndShipDD, CB_GETCURSEL, 0, 0);
        if (lSel != -1 && FLookupOrbitingXfer(sel.pl.id, LOWORD(lSel), &xf, sel.fl.id)) {
            TransferStuff(sel.fl.id, grobjFleet, xf.id, xf.grobj, mdXferCargo);
        }
    } else if ((HWND)lParam == rghwndBtn[1] && HIWORD(wParam) == 0) {
        lSel = SendMessage(hwndShipDD, CB_GETCURSEL, 0, 0);
        if (lSel != -1 && FLookupOrbitingXfer(sel.pl.id, LOWORD(lSel), &xf, sel.fl.id) && xf.grobj == grobjFleet) {
            SelectAdjFleet(0, xf.id);
        }
    } else if ((HWND)lParam == rghwndBtn[2] && HIWORD(wParam) == 0) {
        lSel = SendMessage(hwndShipDD, CB_GETCURSEL, 0, 0);
        if (lSel != -1 && FLookupOrbitingXfer(sel.pl.id, LOWORD(lSel), &xf, sel.fl.id) && xf.grobj == grobjFleet) {
            TransferStuff(sel.fl.id, grobjFleet, xf.id, grobjFleet, mdXferShips);
            if (grbitScan & grbitScanFleetPaths) {
                InvalidateRect(hwndScanner, NULL, TRUE);
            }
            InvalidateReport(rptFleets, 1);
        }
    } else if ((HWND)lParam == rghwndBtn[8] && HIWORD(wParam) == 0) {
        TransferStuff(sel.fl.id, grobjFleet, -1, grobjOther, mdXferShips);
        InvalidateReport(rptFleets, 1);
        if (gd.fTutorial) {
            AdvanceTutor();
        }
    } else if ((HWND)lParam == rghwndBtn[9] && HIWORD(wParam) == 0) {
        FFleetSplitAll(&sel.fl);
        FillShipDD(sel.fl.id);
        grbit = -31819;
        FLookupFleet(sel.fl.id, &sel.fl);
        FillFleetCompLB();
        DrawPlanShip(NULL, grbit);
        InvalidateRect(hwndMine, NULL, TRUE);
    } else if ((HWND)lParam == rghwndBtn[10] && HIWORD(wParam) == 0) {
        vrgiflMerge = rgifl;
        vcflMerge = 0;
        for (ifl = 0; ifl < cFleet; ifl++) {
            lpfl = rglpfl[ifl];
            if (!rglpfl[ifl])
                break;
            if (lpfl->iPlayer == idPlayer && !lpfl->fDead && lpfl->pt.x == sel.fl.pt.x && lpfl->pt.y == sel.fl.pt.y) {
                rgifl[vcflMerge++] = ifl;
            }
        }
        lpfl = NULL;
        lpProc = MakeProcInstance(MergeFleetsDlg, hInst);
        if (DialogBox(hInst, MAKEINTRESOURCE(IDD_MERGE_FLEETS), hwndFrame, lpProc) != 0) {
            for (ifl = 0; ifl < vcflMerge; ifl++) {
                if (vrgiflMerge[ifl] != iflNone) {
                    if (rglpfl[vrgiflMerge[ifl]]->id == sel.fl.id) {
                        lpfl = rglpfl[vrgiflMerge[ifl]];
                    }
                    vrgiflMerge[ifl] = rglpfl[vrgiflMerge[ifl]]->id;
                }
            }
            for (ifl = 0; ifl < vcflMerge; ifl++) {
                if (vrgiflMerge[ifl] != iflNone) {
                    if (!lpfl) {
                        lpfl = LpflFromId(vrgiflMerge[ifl]);
                    } else if (vrgiflMerge[ifl] != lpfl->id) {
                        break;
                    }
                }
            }
            if (ifl == vcflMerge) {
                lpfl = NULL;
            } else if (lpfl->id != sel.fl.id) {
                SelectAdjFleet(0, lpfl->id);
            }
        }
        FreeProcInstance(lpProc);
        if (lpfl) {
            FFleetMergeAll(&sel.fl);
            FillShipDD(sel.fl.id);
            grbit = -31819;
            FLookupFleet(sel.fl.id, &sel.fl);
            FillFleetCompLB();
            DrawPlanShip(NULL, grbit);
            InvalidateRect(hwndMine, NULL, TRUE);
            if (grbitScan & grbitScanFleetPaths) {
                InvalidateRect(hwndScanner, NULL, TRUE);
            }
            vrgiflMerge = 0;
            vcflMerge = 0;
            if (gd.fTutorial) {
                AdvanceTutor();
            }
        }
    } else if ((HWND)lParam == hwndRepCB && HIWORD(wParam) == 0) {
        sel.fl.fRepOrders = LOWORD(SendMessage(hwndRepCB, BM_GETCHECK, 0, 0));
        FLookupFleet(idWriteBack, &sel.fl);
    } else if ((HWND)lParam == rghwndOrderDD[0]) {
        if (HIWORD(wParam) == 1) {
            lSel = SendMessage(rghwndOrderDD[0], CB_GETCURSEL, 0, 0);
            if (LOWORD(lSel) != sel.fl.lpplord->rgord[sel.iwpAct].grTask) {
                if (lSel == 3 || sel.fl.lpplord->rgord[sel.iwpAct].grTask == grTaskMine) {
                    InvalidateRect(hwndMine, NULL, TRUE);
                }
                sel.fl.lpplord->rgord[sel.iwpAct].grTask = LOWORD(lSel);
                memset((uint8_t *)&sel.fl.lpplord->rgord[sel.iwpAct] + 8, 0, 10);
                switch (LOWORD(lSel)) {
                case 7:
                    sel.fl.lpplord->rgord[sel.iwpAct].tptl.iDist = 0;
                    sel.fl.lpplord->rgord[sel.iwpAct].tptl.iWarp = 0;
                    break;
                case 9:
                    sel.fl.lpplord->rgord[sel.iwpAct].tsell.iPlrX = 0;
                    break;
                case 6:
                    sel.fl.lpplord->rgord[sel.iwpAct].tsell.iPlrX = 5;
                    break;
                case 4:
                    if (sel.fl.lpplord->rgord[sel.iwpAct].grobj != grobjFleet) {
                        lpflBest = NULL;
                        ishPrimary = 0;
                        for (ish = 1; ish < 16; ish++) {
                            if (sel.fl.rgcsh[ish] > sel.fl.rgcsh[ishPrimary]) {
                                ishPrimary = ish;
                            }
                        }
                        for (ifl = 0; ifl < cFleet; ifl++) {
                            lpfl = rglpfl[ifl];
                            if (!rglpfl[ifl])
                                break;
                            if (lpfl->pt.x == sel.fl.lpplord->rgord[sel.iwpAct].pt.x && lpfl->pt.y == sel.fl.lpplord->rgord[sel.iwpAct].pt.y &&
                                lpfl->iPlayer == idPlayer && !lpfl->fDead && lpfl->id != sel.fl.id) {
                                if (lpfl->rgcsh[ishPrimary] > 0) {
                                    lpflBest = lpfl;
                                    if (lpfl->cord == 1)
                                        break;
                                } else if (!lpflBest || (lpflBest->cord > 1 && lpfl->cord == 1)) {
                                    lpflBest = lpfl;
                                }
                            }
                        }
                        if (lpflBest) {
                            sel.fl.lpplord->rgord[sel.iwpAct].grobj = grobjFleet;
                            sel.fl.lpplord->rgord[sel.iwpAct].id = lpflBest->id;
                            FLookupFleet(idWriteBack, &sel.fl);
                            FillOrdersLB();
                        }
                    }
                }
                FLookupFleet(idWriteBack, &sel.fl);
                UpdateOrdersDDs(1);
                DrawPlanShip(NULL, tileStarbaseOrWaypoint | tileErase);
            }
        }
    } else if ((HWND)lParam == rghwndOrderDD[1]) {
        if (HIWORD(wParam) == 1) {
            lSel = SendMessage(rghwndOrderDD[1], CB_GETCURSEL, 0, 0);
            switch (sel.fl.lpplord->rgord[sel.iwpAct].grTask) {
            case grTaskPatrol:
                sel.fl.lpplord->rgord[sel.iwpAct].tptl.iDist = LOWORD(lSel);
                FLookupFleet(idWriteBack, &sel.fl);
                UpdateOrdersDDs(1);
                break;
            case grTaskGive:
                sel.fl.lpplord->rgord[sel.iwpAct].tsell.iPlrX = LOWORD(lSel);
                FLookupFleet(idWriteBack, &sel.fl);
                UpdateOrdersDDs(1);
                break;
            case grTaskXfer:
                UpdateOrdersDDs(2);
                DrawPlanShip(NULL, tileStarbaseOrWaypoint);
                break;
            default:
                sel.fl.lpplord->rgord[sel.iwpAct].tlm.cTime = LOWORD(lSel);
                sel.fl.lpplord->rgord[sel.iwpAct].tlm.cTimeOld = LOWORD(lSel);
                FLookupFleet(idWriteBack, &sel.fl);
            }
        }
    } else if ((HWND)lParam == rghwndOrderDD[2]) {
        if (HIWORD(wParam) == 1) {
            lSel = SendMessage(rghwndOrderDD[2], CB_GETCURSEL, 0, 0);
            lMin = SendMessage(rghwndOrderDD[1], CB_GETCURSEL, 0, 0);
            if (lMin == 0) {
                lMin = 4;
            } else {
                lMin--;
            }
            sel.fl.lpplord->rgord[sel.iwpAct].txp.rgia[lMin].iAction = LOWORD(lSel);
            FLookupFleet(idWriteBack, &sel.fl);
            UpdateOrdersDDs(3);
            DrawPlanShip(NULL, tileStarbaseOrWaypoint);
        }
    } else if ((HWND)lParam == hwndOrderED && HIWORD(wParam) == 768) {
        lSel = SendMessage(rghwndOrderDD[2], CB_GETCURSEL, 0, 0);
        if (lSel == 5 || lSel == 6) {
            fPercent = TRUE;
        }
        GetWindowText(hwndOrderED, rgb, 8);
        iInit = atoi(rgb);
        i = 0 <= iInit ? iInit : 0;
        if (fPercent) {
            i = 100 >= i ? i : 100;
        } else {
            i = 4000 >= i ? i : 4000;
        }
        if (iInit != i) {
            AlertSz(PszFormatIds(idsAmountCargoMaySpecifyHereMustBetween, NULL), MB_ICONHAND);
            wsprintf(szWork, PCTD, i);
            SetWindowText(hwndOrderED, szWork);
        }
        lMin = SendMessage(rghwndOrderDD[1], CB_GETCURSEL, 0, 0);
        if (lMin == 0) {
            lMin = 4;
        } else {
            lMin--;
        }
        sel.fl.lpplord->rgord[sel.iwpAct].txp.rgia[lMin].cQuan = i;
        FLookupFleet(idWriteBack, &sel.fl);
    }
    return;
}

void DrawFleetGauge(HDC hdc, RECT *prc, FLEET *lpfl, int16_t grbit) {
    HBRUSH  rghbr[5];
    int32_t lMax;
    int16_t c;
    int16_t i;
    int32_t rgSize[5];
    int16_t iMode;
    int16_t cSections;
    int32_t l;

    if (!lpfl) {
        lpfl = &sel.fl;
    }
    SelectObject(hdc, rghfontArial8[1]);
    cSections = 1;
    lMax = LGetFleetStat(lpfl, 2);
    if (grbit >= 0 && grbit <= 4) {
        rghbr[0] = rghbrMineral[grbit];
        rgSize[0] = lpfl->rgwtMin[grbit];
        if (grbit == 4) {
            lMax = LGetFleetStat(lpfl, 1);
        }
    } else {
        switch (grbit) {
        case 5:
            for (i = 0; i <= 3; i++) {
                rghbr[i] = rghbrMineral[i];
                rgSize[i] = lpfl->rgwtMin[i];
            }
            cSections = 4;
            break;
        case 6:
            lMax = 11;
            rgSize[0] = sel.fl.lpplord->rgord[sel.iwpAct].iWarp;
            if (rgSize[0] > 10 || (rgSize[0] == 10 && IFindIdealWarp(&sel.fl, FALSE) < 10)) {
                rghbr[0] = rghbrMineral[2];
                break;
            }
            rghbr[0] = rghbrMineral[4];
            break;
        case 7:
            lMax = 10;
            rgSize[0] = (uint32_t)sel.fl.lpplord->rgord[sel.iwpAct].tsell.iPlrX;
            if (rgSize[0] > 10 || (rgSize[0] == 10 && IFindIdealWarp(&sel.fl, FALSE) < 10)) {
                rghbr[0] = rghbrMineral[2];
            } else {
                rghbr[0] = rghbrMineral[4];
            }
        }
    }
    l = LDrawGauge(hdc, prc, cSections, rgSize, rghbr, lMax);
    iMode = SetBkMode(hdc, TRANSPARENT);
    if (grbit == 6) {
        if (l != 0) {
            if (l < 11) {
                c = wsprintf(szWork, PszGetCompressedString(idsWarpLd), l);
            } else {
                c = CchGetString(idsUseStargate, szWork);
            }
        } else {
            c = CchGetString(idsStopped, szWork);
        }
    } else if (grbit == 7) {
        if (l != 0) {
            c = wsprintf(szWork, PszGetCompressedString(idsWarpLd), l);
        } else {
            c = CchGetString(idsAutomatic, szWork);
        }
    } else if (cSections == 1 && grbit != 4) {
        c = wsprintf(szWork, PszGetCompressedString(idsLdkt), l);
    } else if (grbit == 4) {
        c = wsprintf(szWork, PszGetCompressedString(idsLdLdmg), l, lMax);
    } else {
        c = wsprintf(szWork, PszGetCompressedString(idsLdLdkt), l, lMax);
    }
    l = GetTextExtent(hdc, szWork, c);
    if ((int16_t)LOWORD(l) < prc->right - prc->left - 3) {
        RcCtrTextOut(hdc, prc, szWork, c);
    }
    SetBkMode(hdc, iMode);
    return;
}

void DrawFleetBitmap(FLEET *lpfl, HDC hdc, int16_t x, int16_t y, int16_t fFrame, int16_t ibmp, int16_t cDiff, int16_t fShrink, int16_t ibmpRace, int16_t csh) {
    int16_t dxyPlus;
    int16_t yCur;
    int16_t c;
    int16_t i;
    int16_t dxy;
    int16_t dx;
    int16_t xCur;
    int16_t dxyPlusWidth;
    HBRUSH  hbrSav;

    if (fFrame) {
        hbrSav = SelectObject(hdc, hbrButtonShadow);
        PatBlt(hdc, x, y, 68, 2, PATCOPY);
        PatBlt(hdc, x, y, 2, 68, PATCOPY);
        SelectObject(hdc, hbrButtonHilite);
        PatBlt(hdc, x + 2, y + 66, 66, 1, PATCOPY);
        PatBlt(hdc, x + 3, y + 66, 65, 1, PATCOPY);
        PatBlt(hdc, x + 66, y + 2, 1, 64, PATCOPY);
        PatBlt(hdc, x + 67, y + 1, 1, 65, PATCOPY);
        SelectObject(hdc, hbrSav);
        x += 2;
        y += 2;
    }
    if (ibmp < 0) {
        i = IshdefPrimaryFromLpfl(lpfl, &cDiff);
        ibmp = rglpshdef[lpfl->iPlayer][i].hul.ibmp;
    }
    ibmp %= 148;
    SelectPalette(hdc, vhpal, FALSE);
    RealizePalette(hdc);
    if (fShrink) {
        DibBlt(hdc, x, y, 32, 32, rghdibShipsT[ibmp >> 5], ((ibmp & 0x1f) >> 2) * 0x20, (3 - (ibmp & 3)) * 0x20, 32, 32, 13369376);
        dxy = 32;
        dxyPlus = 5;
        dxyPlusWidth = 1;
        if (ibmpRace >= 0) {
            DibBlt(hdc, x, y + 24, 8, 8, hdibRacesX, (ibmpRace & 7) * 8, (3 - ((ibmpRace & 0x1f) >> 3)) * 8, 8, 8, 13369376);
        }
    } else {
        DibBlt(hdc, x, y, 64, 64, rghdibShips[ibmp >> 5], ((ibmp & 0x1f) >> 2) * 0x40, (3 - (ibmp & 3)) * 0x40, 64, 64, 13369376);
        dxy = 64;
        dxyPlus = 8;
        dxyPlusWidth = 2;
        if (ibmpRace >= 0) {
            DibBlt(hdc, x, y + 48, 16, 16, hdibRacesT, (ibmpRace & 7) * 0x10, (3 - ((ibmpRace & 0x1f) >> 3)) * 0x10, 16, 16, 13369376);
        }
    }
    if (csh == 0 || fShrink) {
        if (cDiff > 4) {
            cDiff = 4;
        }
        cDiff--;
        for (i = 0; i < cDiff; i++) {
            xCur = ((i & 1) ^ ((i & 2) == 2)) == 0 ? x + 2 : x + dxy - 2 - dxyPlus;
            yCur = !(i & 2) ? y + 2 : y + dxy - 2 - dxyPlus;
            PatBlt(hdc, xCur, (int16_t)(dxyPlus - 1) / 2 + yCur, dxyPlus, dxyPlusWidth, WHITENESS);
            PatBlt(hdc, (int16_t)(dxyPlus - 1) / 2 + xCur, yCur, dxyPlusWidth, dxyPlus, WHITENESS);
        }
    } else {
        SelectObject(hdc, rghfontArial7[0]);
        SetTextColor(hdc, 0xffffff);
        if (cDiff > 1) {
            c = wsprintf(szWork, PCTD, cDiff);
            TextOut(hdc, x + 1, y + 1, szWork, c);
        }
        if (csh > 1) {
            c = wsprintf(szWork, PCTD, csh);
            dx = LOWORD(GetTextExtent(hdc, szWork, c));
            TextOut(hdc, x + dxy - 1 - dx, y + 1, szWork, c);
        }
    }
    return;
}

int16_t TransferStuff(int16_t id1, GrobjClass grobj1, int16_t id2, GrobjClass grobj2, MdXfer mdXfer) {
    XFER    xfer[2];
    FARPROC lpProcXfer;
    int16_t rgValidHull[16];
    int32_t lPopPrev;
    int16_t iDelFleet;
    int16_t i;
    FLEET  *lpfl;
    int16_t fSuccess;
    int16_t grbit;
    int16_t j;
    BTN     rgbtn[32];
    POINT16 pt;
    RECT    rc;

    lPopPrev = -1;
    xfer[0].id = id1;
    xfer[0].grobj = grobj1;
    xfer[1].id = id2;
    xfer[1].grobj = grobj2;
    pxfer = xfer;
    mdXferDlg = mdXfer;
    for (i = 0; i < 2; i++) {
        if (xfer[i].grobj != grobjOther) {
            if (!FLookupObject(xfer[i].grobj, xfer[i].id, &xfer[i].fl)) {
                return 0;
            }
            if (xfer[i].grobj == grobjPlanet) {
                lPopPrev = xfer[i].pl.rgwtMin[3];
            }
        } else if (mdXfer == mdXferShips) {
            lpfl = LpflNewSplit(&xfer[0].fl);
            xfer[1].fl = *lpfl;
            xfer[1].id = lpfl->id;
            xfer[1].grobj = grobjFleet;
        } else {
            xfer[i].fl.id = idflNone;
            for (j = 0; j < 4; j++) {
                xfer[i].pl.rgwtMin[j] = 0;
            }
            EnumLogRts((int16_t (*)(void *, int16_t, int16_t, void *, int16_t))FEnumCalcJettison, &xfer[i].pl, id1);
        }
    }
    if (mdXfer == mdXferShips) {
        cXferValidHulls = 0;
        for (i = 0; i < 16; i++) {
            if (xfer[0].fl.rgcsh[i] != 0 || (xfer[1].grobj == grobjFleet && xfer[1].fl.rgcsh[i] != 0)) {
                rgValidHull[cXferValidHulls++] = i;
            }
        }
        rgXferValidHulls = rgValidHull;
    }
    rgbtnXfer = rgbtn;
    crgbtnXfer = 32;
    if (gd.fTutorial) {
        AdvanceTutor();
    }
    lpProcXfer = MakeProcInstance(TransferDlg, hInst);
    fSuccess = DialogBox(hInst, MAKEINTRESOURCE(IDD_TRANSFER), hwndFrame, lpProcXfer);
    FreeProcInstance(lpProcXfer);
    if (fSuccess) {
        iDelFleet = -1;
        if (mdXfer == mdXferShips) {
            for (i = 0; i < 16 && xfer[1].fl.rgcsh[i] <= 0; i++) {
            }
            if (i == 16 && grobj2 == grobjOther)
                goto CancelSplit;
            FleetTransferCargoBalance(&xfer[0].fl, &xfer[1].fl);
        }
        for (i = 0; i < 2; i++) {
            switch (xfer[i].grobj) {
            case grobjPlanet:
            case grobjOther:
                FLookupPlanet(idWriteBack, &xfer[i].pl);
                if (xfer[i].grobj != grobjPlanet || lPopPrev != xfer[i].pl.rgwtMin[3])
                    break;
                lPopPrev = -1;
                break;
            case grobjFleet:
                FLookupFleet(idWriteBack, &xfer[i].fl);
                if (mdXfer != mdXferShips)
                    break;
                for (j = 0; j < 16 && xfer[i].fl.rgcsh[j] == 0; j++) {
                }
                if (j != 16)
                    break;
                iDelFleet = i;
                break;
            case grobjThing:
                FLookupThing(idWriteBack, &xfer[i].th);
            }
        }
        if (iDelFleet != -1) {
            FDeleteFleet(xfer[iDelFleet].fl.id, grobjFleet, xfer[iDelFleet == 0].fl.id);
        }
        if (mdXfer == mdXferShips) {
            FillShipDD(sel.fl.id);
            if (grbitScan & grbitScanFleetPaths) {
                InvalidateRect(hwndScanner, NULL, TRUE);
            }
        }
        if (sel.grobj == grobjPlanet) {
            grbit = -32691;
            FLookupPlanet(sel.pl.id, &sel.pl);
            FillPlanetProdLB(NULL, NULL, NULL);
        } else {
            grbit = -31819;
            FLookupFleet(sel.fl.id, &sel.fl);
            FillFleetCompLB();
        }
        DrawPlanShip(NULL, grbit);
        if (sel.scan.grobj == grobjFleet) {
            InvalidateRect(hwndMine, NULL, TRUE);
        } else {
            InvalidateMineralBars();
        }
        if (lPopPrev != -1 && GetRaceStat(&rgplr[idPlayer], rsMajorAdv) == raMacintosh && (grbitScan & grbitScanCoverage)) {
            InvalidateRect(hwndScanner, NULL, FALSE);
        } else if (lPopPrev != -1 && (grbitScan & grbitScanViewMask) == 4) {
        LInvalScanPlan:
            pt = sel.pt;
            LogicalToScan(&pt);
            rc.right = pt.x;
            rc.bottom = pt.y;
            rc.left = pt.x;
            rc.top = pt.y;
            InflateRect(&rc, 20, 20);
            rc.top -= 20;
            InvalidateRect(hwndScanner, &rc, FALSE);
        } else if ((grbitScan & grbitScanViewMask) == 1) {
            goto LInvalScanPlan;
        }
    } else if (mdXfer == mdXferShips && grobj2 == grobjOther) {
    CancelSplit:
        FDeleteFleet(xfer[1].fl.id, grobjNone, 0);
        CancelMemRt(rtLogFleetSplit);
    }
    mdXferDlg = mdXferNone;
    return 0;
}

INT_PTR CALLBACK TransferDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    HDC         hdc;
    int16_t     dyMore;
    PAINTSTRUCT ps;
    POINT16     pt;
    HWND        hwndBtn;
    RECT        rcBtn;
    int16_t     dx;
    RECT        rc;

    switch (message) {
    case WM_INITDIALOG:
        StickyDlgPos(hwnd, &ptStickyTransferDlg, TRUE);
        GetClientRect(hwnd, &rc);
        if (mdXferDlg == mdXferShips) {
            SetWindowText(hwnd, PszGetCompressedString(idsShipTransfer));
            if (cXferValidHulls > 10) {
                dyMore = rc.bottom / 2;
                rc.bottom += dyMore;
                SetWindowPos(hwnd, NULL, 0, 0, rc.right, rc.bottom, SWP_NOMOVE | SWP_NOZORDER);
                GetClientRect(hwnd, &rc);
                dyMore -= GetSystemMetrics(SM_CYCAPTION) + 2;
                dx = GetSystemMetrics(SM_CXDLGFRAME) + 4;
                hwndBtn = GetDlgItem(hwnd, IDOK);
                GetWindowRect(hwndBtn, &rcBtn);
                pt.x = rcBtn.left;
                pt.y = rcBtn.top;
                ScreenToClient16(hwnd, &pt);
                pt.y += dyMore;
                pt.x -= dx;
                SetWindowPos(hwndBtn, NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE);
                hwndBtn = GetDlgItem(hwnd, IDC_HELP);
                GetWindowRect(hwndBtn, &rcBtn);
                pt.x = rcBtn.left;
                pt.y = rcBtn.top;
                ScreenToClient16(hwnd, &pt);
                pt.y += dyMore;
                pt.x -= dx;
                SetWindowPos(hwndBtn, NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE);
                hwndBtn = GetDlgItem(hwnd, IDCANCEL);
                GetWindowRect(hwndBtn, &rcBtn);
                pt.x = rcBtn.left;
                pt.y = rcBtn.top;
                ScreenToClient16(hwnd, &pt);
                pt.y += dyMore;
                pt.x -= dx;
                SetWindowPos(hwndBtn, NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE);
            }
        }
        FSetupXferBtns(&rc);
        if (gd.fTutorial) {
            AdvanceTutor();
        }
        return 1;
    case WM_ERASEBKGND:
        GetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        GetClientRect(hwnd, &rc);
        DrawXferDlg(hwnd, hdc, &rc, SupplyAll);
        EndPaint(hwnd, &ps);
        return 1;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
        return FTrackXfer(hwnd, LOWORD(lParam), HIWORD(lParam), wParam);
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            StickyDlgPos(hwnd, &ptStickyTransferDlg, FALSE);
            EndDialog(hwnd, LOWORD(wParam) == IDOK);
            if (gd.fTutorial) {
                AdvanceTutor();
            }
            return 1;
        case IDC_HELP:
            WinHelp(hwnd, szHelpFile, HELP_CONTEXT, (uint32_t)(mdXferDlg == mdXferShips ? 1080 : 1075));
            return 1;
        }
        /* fallthrough */
    default:
        return 0;
    }
}

int16_t FTrackXfer(HWND hwnd, int16_t x, int16_t y, int16_t fkb) {
    POINT16 ptOld;
    POINT16 pt;
    int32_t dChg;
    BTNT    btnt;
    int32_t cCur;
    int16_t i;
    int16_t iBtn;
    int16_t iVal;
    BTN     btn;
    int32_t cNew;
    RECT    rc;

    GetClientRect(hwnd, &rc);
    pt.x = x;
    pt.y = y;
    for (i = 0; i < crgbtnXfer && ((rgbtnXfer[i].bt & 4) || PtInRect(&rgbtnXfer[i].rc, PointFrom16(pt)) == 0); i++) {
    }
    if (i == crgbtnXfer) {
        return FALSE;
    }
    iBtn = i >> 1;
    btn = rgbtnXfer[i];
    iVal = btn.iVal & 0x7f;
    if (!btn.fVisible) {
        if (iVal > 4)
            goto FinishUp;
        if (pxfer[1].grobj == grobjThing) {
            if (iVal == 4 || iVal == 3)
                goto FinishUp;
        } else if (pxfer[btn.iSide].fl.iPlayer != idPlayer) {
            goto FinishUp;
        }
        SetCapture(hwnd);
        ptOld.y = -1;
        ptOld.x = -1;
        while (FGetMouseMove(&pt)) {
            if (pt.x != ptOld.x || pt.y != ptOld.y) {
                ptOld = pt;
                if (btn.iSide == 1 && pxfer[1].grobj == grobjThing) {
                    cNew = (uint32_t)(pxfer[1].th.thp.wtMax * 10);
                } else if (iVal == 4) {
                    cNew = LGetFleetStat(&pxfer[btn.iSide].fl, 1);
                } else {
                    cNew = LGetFleetStat(&pxfer[btn.iSide].fl, 2);
                }
                cNew = (int32_t)((int16_t)(pt.x - btn.rc.left) * cNew) / (int16_t)(btn.rc.right - btn.rc.left - 2);
                cCur = ChgCargo(pxfer[btn.iSide].grobj, pxfer[btn.iSide].id, iVal, 0, (uint8_t *)(pxfer + btn.iSide) + 4);
                dChg = cNew - cCur;
                if (XferSupply(iVal, btn.iSide == 0 ? dChg : -dChg) != 0) {
                    DrawXferDlg(hwnd, NULL, &rc, iVal);
                }
            }
        }
        ReleaseCapture();
    } else {
        InitBtnTrack(&btnt, hwnd, NULL, &btn.rc, btn.bt, 80, FALSE, FALSE, NULL);
        if (fkb & 8) {
            dChg = (uint32_t)(!(fkb & 4) ? 100 : 1000);
        } else if (fkb & 4) {
            dChg = 10;
        } else {
            dChg = 1;
        }
        while (FTrackBtn(&btnt)) {
            if (mdXferDlg == mdXferShips) {
                i = (int16_t)LOWORD(dChg) < pxfer[btn.iSide == 0].fl.rgcsh[iVal] ? LOWORD(dChg) : pxfer[btn.iSide == 0].fl.rgcsh[iVal];
                if (i != 0) {
                    if (pxfer[btn.iSide].fl.rgcsh[iVal] >= 32766 - i) {
                        i = 1;
                    }
                    pxfer[btn.iSide].fl.rgcsh[iVal] = pxfer[btn.iSide].fl.rgcsh[iVal] + i;
                    pxfer[btn.iSide == 0].fl.rgcsh[iVal] -= i;
                    DrawXferDlg(hwnd, btnt.hdc, &rc, iBtn);
                }
            } else if (iVal >= 0 && iVal <= 4 && XferSupply(iVal, btn.iSide == 0 ? dChg : -dChg) != 0) {
                DrawXferDlg(hwnd, btnt.hdc, &rc, iVal);
            }
        }
    }
FinishUp:
    UpdateXferBtns();
    DrawXferDlg(hwnd, NULL, &rc, SupplyButtonsOnly);
    return TRUE;
}

void UpdateXferBtns() {
    int16_t iSide;
    int16_t i;
    int16_t iLastButton;
    int16_t iVal;
    int32_t lLeft;

    iLastButton = mdXferDlg == mdXferShips ? cXferValidHulls * 2 : 4;
    for (i = 0; i < crgbtnXfer; i++) {
        iVal = rgbtnXfer[i].iVal;
        if (rgbtnXfer[i].fVisible && (iVal <= iLastButton || mdXferDlg == mdXferShips)) {
            iSide = rgbtnXfer[i].iSide;
            if (mdXferDlg == mdXferShips) {
                if (pxfer[iSide].fl.rgcsh[iVal] == 32766) {
                    lLeft = 0;
                } else {
                    lLeft = pxfer[iSide == 0].fl.rgcsh[iVal];
                }
            } else {
                lLeft = ChgCargo(pxfer[iSide == 0].grobj, pxfer[iSide == 0].id, iVal, 0, (uint8_t *)(pxfer + (iSide == 0)) + 4);
                if (lLeft == 0 || pxfer[iSide].grobj != grobjFleet) {
                    if (pxfer[iSide].grobj == grobjPlanet && iVal == 4) {
                        lLeft = 0;
                    }
                } else if (iVal == 4) {
                    lLeft = GetFuelFree(&pxfer[iSide].fl);
                } else {
                    lLeft = GetCargoFree(&pxfer[iSide].fl);
                }
            }
            if (lLeft == 0) {
                rgbtnXfer[i].bt |= 4;
            } else {
                rgbtnXfer[i].bt &= 0xfffb;
            }
        }
    }
    return;
}

void DrawXferDlg(HWND hwnd, HDC hdc, RECT *prc, MineralType iSupply) {
    RECT    rgrc[2];
    int16_t fCreatedDC;
    int16_t i;
    int16_t dxCtr;

    fCreatedDC = FALSE;
    if (!hdc) {
        fCreatedDC = TRUE;
        hdc = GetDC(hwnd);
    }
    dxCtr = prc->right >> 1;
    if (iSupply < Ironium) {
        PatBlt(hdc, dxCtr, 0, 1, prc->bottom, BLACKNESS);
        for (i = 0; i < crgbtnXfer; i++) {
            if (rgbtnXfer[i].fVisible) {
                DrawBtn(hdc, &rgbtnXfer[i].rc, rgbtnXfer[i].bt, FALSE, NULL);
            }
        }
        if (iSupply == SupplyButtonsOnly)
            goto RelDC;
    }
    GetXferLeftRightRcs(prc, rgrc, &rgrc[1]);
    for (i = 0; i < 2; i++) {
        if (mdXferDlg == mdXferShips) {
            DrawFleetShipsXferSide(hdc, &rgrc[i], &pxfer[i].fl, iSupply);
        } else {
            switch (pxfer[i].grobj) {
            case grobjFleet:
                DrawFleetCargoXferSide(hdc, &rgrc[i], &pxfer[i].fl, iSupply);
                break;
            case grobjPlanet:
            case grobjOther:
                DrawPlanetXferSide(hdc, &rgrc[i], &pxfer[i].pl, iSupply);
                break;
            case grobjThing:
                DrawThingXferSide(hdc, &rgrc[i], &pxfer[i].th, iSupply);
            }
        }
    }
RelDC:
    if (fCreatedDC) {
        ReleaseDC(hwnd, hdc);
    }
    return;
}

void GetXferLeftRightRcs(RECT *prcWhole, RECT *prcLeft, RECT *prcRight) {
    SetRect(prcLeft, 0, 0, prcWhole->right >> 1, prcWhole->bottom);
    ExpandRc(prcLeft, -(dyArial8 + 3) - 4, -4);
    prcLeft->left -= dyArial8 + 1;
    SetRect(prcRight, prcWhole->right >> 1, 0, prcWhole->right, prcWhole->bottom);
    ExpandRc(prcRight, -(dyArial8 + 3) - 4, -4);
    prcRight->right += dyArial8 + 1;
    return;
}

int16_t FSetupXferBtns(RECT *prc) {
    int16_t cBtn;
    int16_t iMax;
    int16_t dy;
    int16_t iMin;
    int16_t i;
    int16_t fThingXfer;
    int16_t j;
    int16_t dxCtr;
    RECT    rcRight;
    int16_t dxLabels;
    RECT    rcBtn;
    RECT    rcLeft;
    RECT    rc;

    cBtn = 0;
    dxLabels = mdXferDlg == mdXferShips ? 140 : 75;
    fThingXfer = pxfer[1].grobj == grobjThing;
    dxCtr = prc->right >> 1;
    dy = dyArial8 + 10;
    iMax = mdXferDlg == mdXferShips ? cXferValidHulls : 5;
    if (mdXferDlg != mdXferShips) {
        dy += (dyArial8 + 6) * 2;
    }
    i = 0;
    while (i < iMax) {
        if (i == 4 && mdXferDlg != mdXferShips) {
            dy -= (dyArial8 + 6) * 6;
        }
        SetRect(&rcBtn, dxCtr - (dyArial8 + 3) + 1, dy, dxCtr + 1, dyArial8 + 3 + dy);
        for (j = 0; j < 2; j++) {
            rgbtnXfer[cBtn].rc = rcBtn;
            rgbtnXfer[cBtn].bt = j == 0 ? 2 : 3;
            if (fThingXfer && i >= 4) {
                rgbtnXfer[cBtn].fVisible = FALSE;
                rgbtnXfer[cBtn].rc.bottom = -100;
            } else {
                rgbtnXfer[cBtn].fVisible = TRUE;
            }
            rgbtnXfer[cBtn].iSide = j;
            rgbtnXfer[cBtn].iVal = mdXferDlg == mdXferShips ? rgXferValidHulls[i] : i;
            cBtn++;
            OffsetRc(&rcBtn, dyArial8 + 2, 0);
        }
        i++;
        dy += dyArial8 + 6;
    }
    GetXferLeftRightRcs(prc, &rcLeft, &rcRight);
    if (mdXferDlg != mdXferShips) {
        if (pxfer->grobj != grobjPlanet) {
            rc = rcLeft;
            i = 0;
        } else {
            if (pxfer[1].grobj == grobjPlanet)
                goto NoGauges;
            rc = rcRight;
            i = 1;
        }
        for (; i < 2; i++) {
            SetRect(&rcBtn, rc.left + dxLabels + 10, rc.top + dyArial8 + 6, rc.right - 4, dyArial8 * 2 + rc.top + 6);
            if (mdXferDlg == mdXferShips) {
                iMin = 0;
                iMax = cXferValidHulls;
            } else {
                iMin = 0;
                iMax = 5;
            }
            for (j = iMin; j < iMax; j++) {
                if (mdXferDlg != mdXferShips) {
                    if (j == 0) {
                        OffsetRc(&rcBtn, 0, (dyArial8 + 6) * 2);
                    } else if (j == 4) {
                        OffsetRc(&rcBtn, 0, -(dyArial8 + 6) * 6);
                    }
                }
                rgbtnXfer[cBtn].rc = rcBtn;
                rgbtnXfer[cBtn].bt = 0;
                rgbtnXfer[cBtn].fVisible = FALSE;
                rgbtnXfer[cBtn].iSide = i;
                rgbtnXfer[cBtn].iVal = j + 128;
                cBtn++;
                OffsetRc(&rcBtn, 0, dyArial8 + 6);
            }
            if (pxfer[1].grobj == grobjPlanet)
                break;
            rc = rcRight;
        }
    }
NoGauges:
    crgbtnXfer = cBtn;
    UpdateXferBtns();
    return TRUE;
}

void DrawThingXferSide(HDC hdc, RECT *prc, THING *pth, MineralType iSupply) {
    int16_t yTop;
    int16_t i;
    int16_t xRight;
    int16_t dxLabels;
    RECT    rcGauge;
    int16_t xLeft;
    RECT    rc;

    dxLabels = 75;
    rc = *prc;
    rc.bottom = rc.top + rc.right - rc.left;
    SetTextColor(hdc, crButtonText);
    SetBkColor(hdc, crButtonFace);
    SelectObject(hdc, rghfontArial8[1]);
    if (iSupply == SupplyAll) {
        _Draw3dFrame(hdc, &rc, 0);
    }
    ExpandRc(&rc, -1, -1);
    rc.bottom = rc.top + dyArial8 + 2;
    if (iSupply == SupplyAll) {
        _Draw3dFrame(hdc, &rc, 0);
    }
    if (iSupply == SupplyAll) {
        RcCtrTextOut(hdc, &rc, PszGetThingName(pth->idFull), 0);
    }
    xLeft = prc->left + 4;
    xRight = prc->right - 4;
    yTop = rc.bottom + 3 + (dyArial8 + 6);
    if (iSupply == SupplyAll) {
        SelectObject(hdc, rghfontArial8[1]);
        for (i = -1; i < 3; i++) {
            if (i != 4 && i != 4) {
                RightTextOut(hdc, xLeft + dxLabels, yTop, PszGetCompressedString(i == -1 ? idsPacketShell : idsIronium + i), 0, 0);
            }
            yTop += dyArial8 + 6;
        }
    }
    if (iSupply != Fuel && iSupply != Colonists) {
        yTop = rc.bottom + 3 + (dyArial8 + 6);
        xLeft += dxLabels + 6;
        SetRect(&rcGauge, xLeft, yTop, xRight, yTop + dyArial8);
        DrawThingGauge(hdc, &rcGauge, pth, 5);
        for (i = 0; i < 3; i++) {
            OffsetRc(&rcGauge, 0, dyArial8 + 6);
            if (iSupply == SupplyAll || iSupply == i) {
                DrawThingGauge(hdc, &rcGauge, pth, i);
                if (iSupply == i)
                    break;
            }
        }
    }
    return;
}

void DrawFleetCargoXferSide(HDC hdc, RECT *prc, FLEET *pfl, MineralType iSupply) {
    int16_t yTop;
    int16_t fOtherPlr;
    int16_t c;
    int16_t i;
    int16_t xRight;
    FLEET   fl;
    int16_t dxLabels;
    RECT    rcGauge;
    int16_t xLeft;
    RECT    rc;
    int16_t iMap;

    fOtherPlr = pfl->iPlayer != idPlayer;
    dxLabels = 75;
    fl = *pfl;
    rc = *prc;
    rc.bottom = rc.top + rc.right - rc.left;
    SetTextColor(hdc, crButtonText);
    SetBkColor(hdc, crButtonFace);
    SelectObject(hdc, rghfontArial8[1]);
    if (iSupply == SupplyAll) {
        _Draw3dFrame(hdc, &rc, 0);
    }
    ExpandRc(&rc, -1, -1);
    rc.bottom = rc.top + dyArial8 + 2;
    if (iSupply == SupplyAll) {
        _Draw3dFrame(hdc, &rc, 0);
    }
    if (iSupply == SupplyAll) {
        RcCtrTextOut(hdc, &rc, PszGetFleetName(fl.id), 0);
    }
    xLeft = prc->left + 4;
    xRight = prc->right - 4;
    yTop = rc.bottom + 3;
    if (iSupply == SupplyAll) {
        SelectObject(hdc, rghfontArial8[1]);
        for (i = 0; i < 6; i++) {
            if (i != 6 || !fOtherPlr) {
                RightTextOut(hdc, xLeft + dxLabels, yTop, PszGetCompressedString(idsFuel + i), 0, 0);
            }
            yTop += dyArial8 + 6;
        }
    }
    yTop = rc.bottom + 3;
    xLeft += dxLabels + 6;
    if (fOtherPlr) {
        xRight = xLeft + dxMaxMineralQuan;
        SetRect(&rc, xLeft - 2, yTop - 1, xLeft + dxMaxMineralQuan + 2, yTop + dyArial8 + 1);
        i = 0;
        while (i < 6) {
            if (i == 1) {
                OffsetRc(&rc, 0, dyArial8 + 6);
            } else {
                if (i == 0) {
                    iMap = 4;
                } else {
                    iMap = i - 2;
                }
                if (iSupply == SupplyAll || iSupply == iMap) {
                    _Draw3dFrame(hdc, &rc, iSupply == iMap);
                    c = wsprintf(szWork, PszGetCompressedString(idsLdkt + (iMap == 4)), fl.rgwtMin[iMap]);
                    RightTextOut(hdc, xRight, yTop, szWork, c, 0);
                    if (iSupply == i)
                        break;
                }
                OffsetRc(&rc, 0, dyArial8 + 6);
            }
            i++;
            yTop += dyArial8 + 6;
        }
    } else {
        SetRect(&rcGauge, xLeft, yTop, xRight, yTop + dyArial8);
        if (iSupply == SupplyAll || iSupply == Fuel) {
            DrawFleetGauge(hdc, &rcGauge, &fl, 4);
        }
        if (iSupply != Fuel) {
            yTop += dyArial8 + 6;
            OffsetRc(&rcGauge, 0, dyArial8 + 6);
            DrawFleetGauge(hdc, &rcGauge, &fl, 5);
            yTop += dyArial8 + 6;
            for (i = 0; i <= 3; i++) {
                OffsetRc(&rcGauge, 0, dyArial8 + 6);
                if (iSupply == SupplyAll || iSupply == i) {
                    DrawFleetGauge(hdc, &rcGauge, &fl, i);
                    if (iSupply == i)
                        break;
                }
                yTop += dyArial8 + 6;
            }
        }
    }
    return;
}

void DrawFleetShipsXferSide(HDC hdc, RECT *prc, FLEET *pfl, MineralType iSupply) {
    int16_t yTop;
    int16_t fOtherPlr;
    int16_t c;
    int16_t i;
    int16_t xRight;
    FLEET   fl;
    int16_t xLeft;
    RECT    rc;

    fOtherPlr = pfl->iPlayer != idPlayer;
    fl = *pfl;
    rc = *prc;
    SetTextColor(hdc, crButtonText);
    SetBkColor(hdc, crButtonFace);
    SelectObject(hdc, rghfontArial8[1]);
    if (iSupply == SupplyAll) {
        _Draw3dFrame(hdc, &rc, 0);
    }
    ExpandRc(&rc, -1, -1);
    rc.bottom = rc.top + dyArial8 + 2;
    if (iSupply == SupplyAll) {
        _Draw3dFrame(hdc, &rc, 0);
    }
    if (iSupply == SupplyAll) {
        RcCtrTextOut(hdc, &rc, PszGetFleetName(fl.id), 0);
    }
    xLeft = prc->right - 4 - dxMaxMineralQuan - 2;
    xRight = xLeft + dxMaxMineralQuan;
    yTop = rc.bottom + 3;
    if (iSupply == SupplyAll) {
        SelectObject(hdc, rghfontArial8[1]);
        for (i = 0; i < cXferValidHulls; i++) {
            RightTextOut(hdc, xLeft - 8, yTop, rgshdef[rgXferValidHulls[i]].hul.szClass, 0, 0);
            yTop += dyArial8 + 6;
        }
    }
    yTop = rc.bottom + 3;
    SetRect(&rc, xLeft - 2, yTop - 1, xLeft + dxMaxMineralQuan + 2, yTop + dyArial8 + 1);
    for (i = 0; i < cXferValidHulls; i++) {
        if (iSupply == SupplyAll || iSupply == i) {
            _Draw3dFrame(hdc, &rc, iSupply == i);
            c = wsprintf(szWork, PCTD, pfl->rgcsh[rgXferValidHulls[i]]);
            RightTextOut(hdc, xRight, yTop, szWork, c, 0);
            if (iSupply == i)
                break;
        }
        OffsetRc(&rc, 0, dyArial8 + 6);
        yTop += dyArial8 + 6;
    }
    return;
}

void DrawPlanetXferSide(HDC hdc, RECT *prc, PLANET *ppl, MineralType iSupply) {
    PLANET  pl;
    int16_t yTop;
    int16_t c;
    int16_t i;
    int16_t xRight;
    char   *psz;
    int16_t xLeft;
    RECT    rc;

    pl = *ppl;
    rc = *prc;
    rc.bottom = rc.top + rc.right - rc.left;
    SetTextColor(hdc, crButtonText);
    SetBkColor(hdc, crButtonFace);
    SelectObject(hdc, rghfontArial8[1]);
    if (iSupply == SupplyAll) {
        _Draw3dFrame(hdc, &rc, 0);
    }
    ExpandRc(&rc, -1, -1);
    rc.bottom = rc.top + dyArial8 + 2;
    if (iSupply == SupplyAll) {
        _Draw3dFrame(hdc, &rc, 0);
        if (pl.id != idplNone) {
            psz = PszGetPlanetName(pl.id);
        } else {
            psz = PszGetCompressedString(idsDeepSpace);
        }
        RcCtrTextOut(hdc, &rc, psz, 0);
    }
    xLeft = prc->left + 4;
    xRight = prc->right - 4;
    yTop = rc.bottom + 3;
    if (iSupply == SupplyAll) {
        SelectObject(hdc, rghfontArial8[1]);
        for (i = 0; i < 6; i++) {
            if (i > 1) {
                RightTextOut(hdc, xLeft + 75, yTop, PszGetCompressedString(idsFuel + i), 0, 0);
            }
            yTop += dyArial8 + 6;
        }
    }
    yTop = rc.bottom + 3;
    xLeft += 81;
    xRight = xLeft + dxMaxMineralQuan + 12;
    SetRect(&rc, xLeft - 2, yTop - 1, xLeft + dxMaxMineralQuan + 14, yTop + dyArial8 + 1);
    i = 0;
    while (i <= 4) {
        if (i == 0) {
            yTop += (dyArial8 + 6) * 2;
            OffsetRc(&rc, 0, (dyArial8 + 6) * 2);
        } else if (i == 4) {
            yTop -= (dyArial8 + 6) * 6;
            OffsetRc(&rc, 0, (dyArial8 + 6) * 6);
        }
        if ((iSupply == SupplyAll || iSupply == i) && i != 4) {
            _Draw3dFrame(hdc, &rc, iSupply == i);
            c = wsprintf(szWork, PszGetCompressedString(idsLdkt), pl.rgwtMin[i]);
            RightTextOut(hdc, xRight, yTop, szWork, c, 0);
            if (iSupply == i)
                break;
        }
        OffsetRc(&rc, 0, dyArial8 + 6);
        i++;
        yTop += dyArial8 + 6;
    }
    return;
}

HCURSOR ClickInShipOrders(POINT16 pt, int16_t sks, int16_t fCursor, int16_t fRightBtn) {
    int32_t    lCur;
    HDC        hdc;
    PLANET     pl;
    int16_t    iWarp;
    POINT16    ptOld;
    int16_t    idPlan;
    int32_t    lMax;
    int32_t    lSel;
    int16_t    iSkip;
    int32_t    xRnd;
    int16_t    grbit;
    XFER       xf;
    int32_t    lNew;
    int16_t    irc;
    int32_t    dx;
    int32_t    lTempMin;
    int16_t    fFirst;
    int16_t    fTwoMAs;
    int32_t    lTempMax;
    int16_t    cMax;
    char       sz255[2];
    int16_t    i;
    char      *rgszZip[11];
    ZIPORDER   rgzo[4];
    FARPROC    lpProc;
    int16_t    fRet;
    TASKXPORT *lptxp;
    int16_t    fSep;
    int16_t    c;
    ORDER     *lpord;
    THING     *lpth;
    FLEET     *lpfl;
    int32_t   *rgid;
    int32_t    idFirst;
    int32_t    idSel;
    int16_t    iChecked;
    THING     *lpthMac;
    SCAN       scan;

    lTempMin = 0;
    irc = -1;
    if (sel.grobj == grobjNone) {
        return NULL;
    }
    if (PtInRect(&rgrcRef[5], PointFrom16(pt)) != 0) {
        if (fCursor) {
            return hcurArrowHelp;
        }
        if (fRightBtn) {
            sz255[0] = -1;
            sz255[1] = 0;
            for (i = 0; i < 4; i++) {
                rgszZip[i] = rgszZipOrder[i];
            }
            rgszZip[4] = sz255;
            cMax = 5;
            for (i = 0; i < 4; i++) {
                if (vrgZip[i].fValid) {
                    rgszZip[cMax++] = vrgZip[i].szName;
                }
            }
            if (cMax > 5) {
                rgszZip[cMax++] = sz255;
            }
            rgszZip[cMax++] = PszGetCompressedString(idsCustomize);
            i = PopupMenu(hwndPlanet, pt.x, pt.y, cMax, NULL, rgszZip, -1, TRUE);
            if (i == cMax - 1) {
                memcpy(rgzo, vrgZip, 96);
                lpProc = MakeProcInstance(ZipOrderDlg, hInst);
                fRet = DialogBox(hInst, MAKEINTRESOURCE(IDD_ZIP_PROD), hwndFrame, lpProc);
                FreeProcInstance(lpProc);
                if (!fRet) {
                    memcpy(vrgZip, rgzo, 96);
                }
            } else if (i > 4) {
                i -= 4;
                iSkip = 0;
                while (i != 0) {
                    if (vrgZip[iSkip].fValid) {
                        i--;
                        if (i == 0)
                            break;
                    }
                    iSkip++;
                }
                sel.fl.lpplord->rgord[sel.iwpAct].txp = vrgZip[iSkip].txp;
                goto LWriteZip;
            } else if (i != -1) {
                lptxp = (TASKXPORT *)&sel.fl.lpplord->rgord[sel.iwpAct].txp;
                switch (i) {
                case 0:
                    lptxp->rgia[4].iAction = iActionLoadDunnage;
                    for (i = 0; i < 3; i++) {
                        lptxp->rgia[i].iAction = iActionLoadAll;
                    }
                    lptxp->rgia[3].iAction = iActionNone;
                    break;
                case 1:
                    lptxp->rgia[4].iAction = iActionLoadDunnage;
                    for (i = 0; i <= 3; i++) {
                        lptxp->rgia[i].iAction = iActionUnloadAll;
                    }
                    break;
                case 2:
                    lptxp->rgia[4].iAction = iActionLoadDunnage;
                    for (i = 0; i < 3; i++) {
                        lptxp->rgia[i].iAction = iActionWaitPercent;
                        lptxp->rgia[i].cQuan = 100;
                    }
                    lptxp->rgia[3].iAction = iActionNone;
                    break;
                case 3:
                    for (i = 0; i < 5; i++) {
                        lptxp->rgia[i].iAction = iActionNone;
                        lptxp->rgia[i].cQuan = 0;
                    }
                }
            LWriteZip:
                FLookupFleet(idWriteBack, &sel.fl);
                UpdateOrdersDDs(1);
                DrawPlanShip(NULL, tileStarbaseOrWaypoint);
            }
        } else {
            GlobalPD.grPopup = grPopupShipOrders;
            Popup(hwndPlanet, pt.x, pt.y);
        }
    } else if (PtInRect(&rgrcRef[12], PointFrom16(pt)) != 0) {
        if (fCursor) {
            return hcurArrowHelp;
        }
        if (fRightBtn) {
            iChecked = -1;
            lpord = &sel.fl.lpplord->rgord[sel.iwpAct];
            FFindNearestObject(lpord->pt, grobjPlanet | grobjFleet | grobjOther | grobjThing | mdExact, &scan);
            /* Room for every fleet and thing here, the planet or deep space
               and a separator. The original stopped at 100 entries, so
               objects past them couldn't be targeted (target list
               overload). */
            rgid = LpAlloc((cFleet + cThing + 3) * sizeof(int32_t), htMisc);
            if (scan.idpl != idplNone) {
                rgid[0] = scan.idpl;
            } else {
                rgid[0] = 268435456;
            }
            rgid[1] = -1;
            c = 2;
            if (lpord->grobj == grobjPlanet || lpord->grobj == grobjOther) {
                iChecked = 0;
            }
            for (i = 0; i < cFleet; i++) {
                lpfl = rglpfl[i];
                if (!rglpfl[i])
                    break;
                if (scan.pt.x == lpfl->pt.x && scan.pt.y == lpfl->pt.y && lpfl->id != sel.fl.id) {
                    if (lpord->grobj == grobjFleet && lpord->id == lpfl->id) {
                        iChecked = c;
                    }
                    rgid[c++] = lpfl->id | 0x80000000;
                }
            }
            if (c == 2) {
                c = 1;
            }
            fSep = c == 0;
            lpth = lpThings;
            lpthMac = lpThings + cThing;
            for (; lpth < lpthMac; lpth++) {
                if (scan.pt.x == lpth->pt.x && scan.pt.y == lpth->pt.y) {
                    if (!fSep) {
                        rgid[c++] = -1;
                        fSep = TRUE;
                    }
                    rgid[c++] = (uint32_t)(uint16_t)lpth->idFull | 0x20000000;
                }
            }
            i = PopupMenu(hwndPlanet, pt.x, pt.y, c, rgid, NULL, iChecked, TRUE);
            idFirst = rgid[0];
            idSel = i >= 0 ? rgid[i] : 0;
            FreeLp(rgid, htMisc);
            if (i >= 0) {
                if (i == 0 && idFirst == 268435456) {
                    lpord->grobj = grobjOther;
                    lpord->id = -1;
                } else if (idSel & 0x20000000) {
                    lpord->grobj = grobjThing;
                    lpord->id = LOWORD(idSel);
                } else if (idSel & 0x80000000) {
                    lpord->grobj = grobjFleet;
                    lpord->id = LOWORD(idSel);
                } else {
                    lpord->grobj = grobjPlanet;
                    lpord->id = LOWORD(idFirst);
                }
                FLookupFleet(idWriteBack, &sel.fl);
                FillOrdersLB();
                SetOrdersLbSel(sel.iwpAct);
            }
        } else {
            GlobalPD.grPopup = grPopupString;
            GlobalPD.dxOut = 180;
            GlobalPD.psz = szPopupBuffer;
            CchGetString(idsRightClickBlueDiamondBringPopupMenu, szPopupBuffer);
            Popup(hwndPlanet, pt.x, pt.y);
        }
    } else {
        if (fRightBtn) {
            return NULL;
        }
        if (PtInRect(rgrcRef, PointFrom16(pt)) != 0) {
            if (sel.grobj != grobjFleet && fCursor) {
                return NULL;
            }
            irc = 0;
            lTempMax = 11;
            lMax = 11;
            lCur = sel.fl.lpplord->rgord[sel.iwpAct].iWarp;
            grbit = 6;
        } else if (PtInRect(&rgrcRef[15], PointFrom16(pt)) != 0) {
            irc = 15;
            iWarp = IWarpMAFromLppl(&sel.pl, &fTwoMAs);
            lTempMax = (int16_t)(iWarp - 1);
            lMax = (int16_t)(iWarp - 1);
            lTempMin = 1;
            lCur = sel.pl.iWarpFling;
        } else if (PtInRect(&rgrcRef[1], PointFrom16(pt)) != 0) {
            if (sel.grobj == grobjFleet) {
                idPlan = sel.fl.idPlanet;
                iSkip = sel.fl.id;
            } else {
                iSkip = -1;
                idPlan = sel.pl.id;
            }
            lSel = SendMessage(hwndShipDD, CB_GETCURSEL, 0, 0);
            FLookupOrbitingXfer(idPlan, LOWORD(lSel), &xf, iSkip);
            if (xf.grobj != grobjFleet || xf.fl.iPlayer != idPlayer) {
                return NULL;
            }
            irc = 1;
            lMax = LGetFleetStat(&xf.fl, 1);
            lCur = xf.fl.rgwtMin[4];
            grbit = 4;
            if (sel.grobj == grobjFleet) {
                lTempMax = lCur + sel.fl.rgwtMin[4];
                lTempMin = lCur - (LGetFleetStat(&sel.fl, 1) - sel.fl.rgwtMin[4]);
            } else {
                lTempMax = lCur;
            }
        } else {
            if (PtInRect(&rgrcRef[3], PointFrom16(pt)) != 0) {
                if (fCursor) {
                    return hcurHand;
                }
                if (sel.fl.idPlanet != idPlanetDeepSpace) {
                    TransferStuff(sel.fl.id, grobjFleet, sel.fl.idPlanet, grobjPlanet, mdXferCargo);
                } else {
                    lpth = lpThings;
                    lpthMac = lpThings + cThing;
                    for (; lpth < lpthMac && (lpth->ith != ithMineralPacket || sel.fl.pt.x != lpth->pt.x || sel.fl.pt.y != lpth->pt.y); lpth++) {
                    }
                    if (lpth == lpthMac) {
                        TransferStuff(sel.fl.id, grobjFleet, -1, grobjOther, mdXferCargo);
                    } else {
                        MessageBeep(MB_OK);
                    }
                }
                return NULL;
            }
            if (PtInRect(&rgrcRef[4], PointFrom16(pt)) != 0) {
                if (fCursor) {
                    return hcurHand;
                }
                lSel = SendMessage(hwndShipDD, CB_GETCURSEL, 0, 0);
                if (lSel == -1) {
                    return NULL;
                }
                if (FLookupOrbitingXfer(sel.pl.id, LOWORD(lSel), &xf, sel.grobj == grobjFleet ? sel.fl.id : idflNone)) {
                    TransferStuff(sel.id, sel.grobj, xf.id, xf.grobj, mdXferCargo);
                }
                return NULL;
            }
            if (PtInRect(&rgrcRef[18], PointFrom16(pt)) != 0) {
                if ((sel.grobj != grobjFleet && fCursor) || sel.fl.lpplord->rgord[sel.iwpAct].grTask != grTaskPatrol) {
                    return NULL;
                }
                irc = 18;
                lTempMax = 10;
                lMax = 10;
                lCur = (uint32_t)sel.fl.lpplord->rgord[sel.iwpAct].tsell.iPlrX;
                grbit = 7;
            }
        }
    }
    if (irc == -1) {
        return NULL;
    }
    if (fCursor) {
        return hcurHand;
    }
    dx = (int16_t)(rgrcRef[irc].right - rgrcRef[irc].left - 2);
    xRnd = (int32_t)(dx / (lMax + 1)) >> 1;
    hdc = GetDC(hwndPlanet);
    SetCapture(hwndPlanet);
    ptOld.y = -1;
    ptOld.x = -1;
    lTempMax = lMax < lTempMax ? lMax : lTempMax;
    lTempMin = 0 <= lTempMin ? lTempMin : 0;
    fFirst = TRUE;
    while (fFirst || FGetMouseMove(&pt)) {
        fFirst = FALSE;
        if (pt.x != ptOld.x || pt.y != ptOld.y) {
            ptOld = pt;
            lNew = (int32_t)((int32_t)(((int16_t)(pt.x - rgrcRef[irc].left) + xRnd) * lMax) / dx);
            if (lTempMin > (lNew < lTempMax ? lNew : lTempMax)) {
                lNew = lTempMin;
            } else if (lNew >= lTempMax) {
                lNew = lTempMax;
            }
            if (lNew != lCur) {
                switch (irc) {
                case 0:
                    SetScanPathWarp(sel.iwpAct, LOWORD(lNew));
                    DrawPlanShip(NULL, tileFleetOrders | tileMinimized);
                    break;
                case 18:
                    sel.fl.lpplord->rgord[sel.iwpAct].tsell.iPlrX = LOWORD(lNew);
                    DrawPlanShip(NULL, tileStarbaseOrWaypoint | tileMinimized);
                    break;
                case 15:
                    DrawMassWarpGauge(hdc, &rgrcRef[15], !fTwoMAs ? iWarp : -iWarp, LOWORD(lNew) + 4);
                    break;
                case 2:
                    sel.fl.rgwtMin[4] = lNew;
                    DrawFleetGauge(hdc, &rgrcRef[irc], NULL, grbit);
                    break;
                case 1:
                    if (sel.grobj == grobjFleet) {
                        sel.fl.rgwtMin[4] -= lNew - lCur;
                        DrawFleetGauge(hdc, &rgrcRef[2], &sel.fl, grbit);
                    } else {
                        DrawPlanShip(NULL, tileMineralsOrCargo | tileMinimized);
                    }
                    xf.fl.rgwtMin[4] = lNew;
                    DrawFleetGauge(hdc, &rgrcRef[irc], &xf.fl, grbit);
                }
                lCur = lNew;
            }
        }
    }
    grbit = sel.scan.grobj == grobjOther ? sel.scan.grobjFull : sel.scan.grobj;
    if (irc == 2) {
        FLookupFleet(idWriteBack, &sel.fl);
        FLookupPlanet(idWriteBack, &pl);
        DrawPlanShip(NULL, tileFleetOrders | tileFleetComp | tileMinimized);
        if ((grbit & 1) && sel.fl.idPlanet == sel.scan.idpl) {
        FixMinWin:
            InvalidateMineralBars();
        } else if ((grbit & 2) && sel.fl.id == rglpfl[sel.scan.ifl]->id) {
            InvalidateRect(hwndMine, NULL, TRUE);
        }
    } else if (irc == 1) {
        FLookupFleet(idWriteBack, &xf.fl);
        if (sel.grobj == grobjFleet) {
            FLookupFleet(idWriteBack, &sel.fl);
            DrawPlanShip(NULL, tileMineralsOrCargo | tileFleetOrders | tileFleetComp | tileMinimized);
        } else {
            FLookupPlanet(idWriteBack, &sel.pl);
            if ((grbit & 1) && sel.pl.id == sel.scan.idpl)
                goto FixMinWin;
        }
    } else if (irc == 0 || irc == 18) {
        FLookupFleet(idWriteBack, &sel.fl);
    } else if (irc == 15 && LOWORD(lCur) != sel.pl.iWarpFling) {
        sel.pl.iWarpFling = LOWORD(lCur);
        FLookupPlanet(idWriteBack, &sel.pl);
    }
    ReleaseCapture();
    return (HCURSOR)(uintptr_t)ReleaseDC(hwndPlanet, hdc);
}

void DeleteCurWayPoint(int16_t fBackup) {
    POINT16 pt;
    POINT16 rgpt[3];
    int16_t cpt;
    SCAN    scan;
    int16_t ipt;
    RECT    rc;

    if (sel.fl.cord < 2 || sel.iwpAct == 0) {
        MessageBeep(MB_ICONASTERISK);
    } else {
        if (grbitScan & grbitScanFleetPaths) {
            rgpt[0] = sel.fl.lpplord->rgord[sel.iwpAct].pt;
            rgpt[1] = sel.fl.lpplord->rgord[sel.iwpAct - 1].pt;
            if (sel.iwpAct < sel.fl.cord - 1) {
                cpt = 3;
                rgpt[2] = sel.fl.lpplord->rgord[sel.iwpAct + 1].pt;
            } else {
                cpt = 2;
            }
        }
        RedrawScanSel(NULL, 0);
        memmove(&sel.fl.lpplord->rgord[sel.iwpAct], &sel.fl.lpplord->rgord[sel.iwpAct + 1], (sel.fl.cord - sel.iwpAct - 1) * sizeof(ORDER));
        sel.fl.cord--;
        sel.fl.lpplord->iordMac--;
        sel.iwpAct--;
        if (sel.iwpAct < sel.fl.cord - 1) {
            pt = sel.fl.lpplord->rgord[sel.iwpAct].pt;
            if (pt.x == sel.fl.lpplord->rgord[sel.iwpAct + 1].pt.x && pt.y == sel.fl.lpplord->rgord[sel.iwpAct + 1].pt.y) {
                memmove(&sel.fl.lpplord->rgord[sel.iwpAct + 1], &sel.fl.lpplord->rgord[sel.iwpAct + 2], (sel.fl.cord - sel.iwpAct - 2) * sizeof(ORDER));
                sel.fl.cord--;
                sel.fl.lpplord->iordMac--;
            }
        }
        if (!fBackup && sel.iwpAct < sel.fl.cord - 1) {
            sel.iwpAct++;
        }
        RedrawScanSel(NULL, 0);
        FLookupFleet(idWriteBack, &sel.fl);
        FFindNearestObject(sel.fl.lpplord->rgord[sel.iwpAct].pt, grobjPlanet | grobjFleet | grobjOther | grobjThing | mdExact, &scan);
        sel.iwpAct = -2;
        ChangeScanSel(&scan, 1);
        if (grbitScan & grbitScanFleetPaths) {
            for (ipt = 0; ipt < cpt; ipt++) {
                LogicalToScan(&rgpt[ipt]);
            }
            BoundPoints(&rc, rgpt, cpt);
            InvalidateRect(hwndScanner, &rc, TRUE);
        }
    }
    return;
}

LRESULT CALLBACK FakeEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg != WM_CHAR || ((wParam >= '0' && wParam <= '9') || wParam == 8)) {
        return CallWindowProc(lpfnRealEditProc, hwnd, msg, wParam, lParam);
    }
    return 0;
}

void SetFleetDropDownSel(int16_t id) {
    int16_t idSkip;
    int16_t i;
    FLEET  *lpfl;
    int16_t iOffset;

    iOffset = 0;
    idSkip = sel.grobj == grobjFleet ? sel.fl.id : idflNone;
    for (i = 0; i < cFleet && rglpfl[i]->id != id; i++) {
        lpfl = rglpfl[i];
        if (sel.pt.x == lpfl->pt.x && sel.pt.y == lpfl->pt.y && lpfl->id != idSkip) {
            iOffset++;
        }
    }
    SendMessage(hwndShipDD, CB_SETCURSEL, iOffset, 0);
    DrawPlanShip(NULL, tileShipList | tileMinimized);
    return;
}

void FillFleetCompLB() {
    int16_t i;
    int32_t pctDmg;

    SendMessage(hwndFleetCompLB, LB_RESETCONTENT, 0, 0);
    for (i = 0; i < 16; i++) {
        if (sel.fl.rgcsh[i] > 0) {
            pctDmg = (int32_t)((uint32_t)(sel.fl.rgdv[i].pctSh * sel.fl.rgdv[i].pctDp) + 250) / 500;
            wsprintf(szWork, "%c%c%5d%s", pctDmg == 0 ? 81 : 80, pctDmg == 0 ? 32 : (int16_t)(int8_t)LOBYTE(LOWORD(pctDmg)), sel.fl.rgcsh[i],
                     rgshdef[i].hul.szClass);
            SendMessage(hwndFleetCompLB, LB_ADDSTRING, 0, (LPARAM)szWork);
        }
    }
    return;
}

void FillOrdersLB() {
    int16_t i;
    char   *psz;
    ORDER   ord;

    SendMessage(hwndShipLB, LB_RESETCONTENT, 0, 0);
    for (i = 0; i < sel.fl.cord; i++) {
        ord = sel.fl.lpplord->rgord[i];
        psz = PszGetLocName(ord.grobj, ord.id, ord.pt.x, ord.pt.y);
        SendMessage(hwndShipLB, LB_ADDSTRING, 0, (LPARAM)psz);
    }
    SetOrdersLbSel(sel.iwpAct);
    if (sel.grobj == grobjFleet) {
        DrawPlanShip(NULL, tileFleetOrders | tileStarbaseOrWaypoint);
    }
    return;
}

void SetOrdersLbSel(int16_t iSel) {
    SendMessage(hwndShipLB, LB_SETCURSEL, iSel, 0);
    if (iSel > (!gd.fSmallTileMode ? 2 : 1)) {
        SendMessage(hwndShipLB, LB_SETTOPINDEX, iSel - (!gd.fSmallTileMode ? 2 : 1), 0);
    }
    UpdateWindow(hwndShipLB);
    UpdateOrdersDDs(0);
    return;
}

void UpdateOrdersDDs(int16_t iLevel) {
    int32_t rglSel[3];
    int16_t iMin;
    int16_t i;
    char   *psz;
    int16_t iSel;
    int16_t iMax;
    char    szT[80];

    iSel = -1;
    if (iLevel == 0) {
        rglSel[0] = SendMessage(rghwndOrderDD[0], CB_SETCURSEL, sel.fl.lpplord->rgord[sel.iwpAct].grTask, 0);
    } else {
        rglSel[0] = SendMessage(rghwndOrderDD[0], CB_GETCURSEL, 0, 0);
    }
    if (iLevel <= 1) {
    DoMinerals:
        SendMessage(rghwndOrderDD[1], CB_RESETCONTENT, 0, 0);
        switch (rglSel[0]) {
        case 1:
            iMax = LGetFleetStat(&sel.fl, 2) == 0 ? 1 : 5;
            for (i = 0; i < iMax; i++) {
                if (i == 0) {
                    iMin = 4;
                } else {
                    iMin = i - 1;
                }
                strcpy(&szWork[1], rgszMinerals[iMin]);
                if (sel.fl.lpplord->rgord[sel.iwpAct].txp.rgia[iMin].iAction != iActionNone) {
                    szWork[0] = '*';
                    if (iSel == -1) {
                        iSel = iMin;
                    }
                } else {
                    szWork[0] = ' ';
                }
                SendMessage(rghwndOrderDD[1], CB_ADDSTRING, 0, (LPARAM)szWork);
            }
            if (iSel == -1 || iSel == 4) {
                iSel = 0;
            } else {
                iSel++;
            }
            rglSel[1] = SendMessage(rghwndOrderDD[1], CB_SETCURSEL, iSel, 0);
            break;
        case 7:
            psz = PszGetCompressedString(idsWithinDLY);
            for (i = 0; i < 11; i++) {
                wsprintf(szWork, psz, 50 * i + 50);
                SendMessage(rghwndOrderDD[1], CB_ADDSTRING, 0, (LPARAM)szWork);
            }
            psz = PszGetCompressedString(idsAnyEnemy);
            SendMessage(rghwndOrderDD[1], CB_ADDSTRING, 0, (LPARAM)psz);
            iSel = sel.fl.lpplord->rgord[sel.iwpAct].tptl.iDist;
            rglSel[1] = SendMessage(rghwndOrderDD[1], CB_SETCURSEL, iSel, 0);
            break;
        case 9:
            szT[0] = ' ';
            for (i = 0; i < game.cPlayer; i++) {
                if (i != idPlayer) {
                    psz = PszPlayerName(i, TRUE, TRUE, TRUE, 0, NULL);
                    strcpy(&szT[1], psz);
                    SendMessage(rghwndOrderDD[1], CB_ADDSTRING, 0, (LPARAM)szT);
                }
            }
            iSel = sel.fl.lpplord->rgord[sel.iwpAct].tsell.iPlrX;
            rglSel[1] = SendMessage(rghwndOrderDD[1], CB_SETCURSEL, iSel, 0);
            break;
        case 6:
            for (i = 0; i < 5; i++) {
                wsprintf(szWork, PszGetCompressedString(idsDYearC), i + 1, i == 0 ? 32 : 115);
                SendMessage(rghwndOrderDD[1], CB_ADDSTRING, 0, (LPARAM)szWork);
            }
            SendMessage(rghwndOrderDD[1], CB_ADDSTRING, 0, (LPARAM)PszGetCompressedString(idsIindefinitely));
            rglSel[1] = SendMessage(rghwndOrderDD[1], CB_SETCURSEL, sel.fl.lpplord->rgord[sel.iwpAct].tsell.iPlrX, 0);
        }
    } else {
        rglSel[1] = SendMessage(rghwndOrderDD[1], CB_GETCURSEL, 0, 0);
        if (rglSel[0] == 1 && iLevel <= 3) {
            iSel = LOWORD(rglSel[1]);
            if (iSel == 0) {
                iSel = 4;
            } else {
                iSel--;
            }
            goto DoMinerals;
        }
    }
    if (iLevel <= 2) {
        SendMessage(rghwndOrderDD[2], CB_RESETCONTENT, 0, 0);
        if (rglSel[0] == 1) {
            for (i = idsAction; i <= idsSetWaypoint; i++) {
                if (i == idsLoadDunnage && rglSel[1] == 0) {
                    psz = PszGetCompressedString(idsLoadOptimal);
                } else {
                    psz = PszGetCompressedString(i);
                }
                SendMessage(rghwndOrderDD[2], CB_ADDSTRING, 0, (LPARAM)psz);
            }
            iSel = LOWORD(rglSel[1]);
            if (iSel == 0) {
                iSel = 4;
            } else {
                iSel--;
            }
            rglSel[2] = SendMessage(rghwndOrderDD[2], CB_SETCURSEL, sel.fl.lpplord->rgord[sel.iwpAct].txp.rgia[iSel].iAction, 0);
        }
    } else {
        rglSel[2] = SendMessage(rghwndOrderDD[2], CB_GETCURSEL, 0, 0);
    }
    if (iLevel <= 3 && rglSel[0] == 1) {
        iSel = LOWORD(rglSel[1]);
        if (iSel == 0) {
            iSel = 4;
        } else {
            iSel--;
        }
        wsprintf(szWork, "%u", sel.fl.lpplord->rgord[sel.iwpAct].txp.rgia[iSel].cQuan);
        SetWindowText(hwndOrderED, szWork);
    }
    return;
}

void FillBattleDD(int16_t iSel) {
    int16_t i;

    SendMessage(hwndBattleDD, CB_RESETCONTENT, 0, 0);
    CchGetString(idsBattlePlans, szWork);
    SendMessage(hwndBattleDD, CB_ADDSTRING, 0, (LPARAM)szWork);
    for (i = 0; i < rgcbtlplan[idPlayer]; i++) {
        strcpy(szWork, rglpbtlplan[idPlayer][i].szName);
        SendMessage(hwndBattleDD, CB_ADDSTRING, 0, (LPARAM)szWork);
    }
    SendMessage(hwndBattleDD, CB_SETCURSEL, iSel, 0);
    return;
}
