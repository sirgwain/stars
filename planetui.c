#include "win.h"

LRESULT CALLBACK PlanetWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    HDC                hdc;
    PAINTSTRUCT        ps;
    XFER               xf;
    int16_t            i;
    char              *psz;
    int32_t            lSel;
    RECT               rc;
    POINT16            pt;
    HCURSOR            hcs;
    DRAWITEMSTRUCT    *lpdis;
    MEASUREITEMSTRUCT *lpmis;
    PLANET            *lpplMac;
    PLANET            *lppl;
    FLEET             *lpfl;

    switch (message) {
    case WM_MDIACTIVATE:
        hwndActive = (lParam == (LPARAM)hwnd) == 0 ? NULL : hwnd;
        return 0;
    case WM_CREATE:
        SetPlanetTitleBar(hwnd);
        for (i = 0; i < 3; i++) {
            rghwndOrderDD[i] = CreateWindow(szCombobox, "OrdDD", (i == 1 ? 528 : 0) | (CBS_DROPDOWNLIST | WS_CHILD | WS_VSCROLL), 100, 100, 200,
                                            i == 2 ? 160 : 80, hwnd, NULL, hInst, NULL);
            SendMessage(rghwndOrderDD[i], WM_SETFONT, (WPARAM)rghfontArial8[1], 0);
        }
        for (i = idsTaskHere; i <= idsTransferFleet; i++) {
            psz = PszGetCompressedString(i);
            SendMessage(rghwndOrderDD[0], CB_ADDSTRING, 0, (LPARAM)psz);
        }
        hwndOrderED = CreateWindow(szEdit, NULL, ES_RIGHT | WS_CHILD | WS_BORDER, 100, 100, 200, 50, hwnd, NULL, hInst, NULL);
        SendMessage(hwndOrderED, EM_LIMITTEXT, 4, 0);
        SendMessage(hwndOrderED, WM_SETFONT, (WPARAM)rghfontArial8[1], 0);
        lpfnRealEditProc = (WNDPROC)GetWindowLongPtr(hwndOrderED, GWLP_WNDPROC);
        SetWindowLongPtr(hwndOrderED, GWLP_WNDPROC, (LONG_PTR)lpfnFakeEditProc);
        hwndBattleDD = CreateWindow(szCombobox, "BattleDD", CBS_DROPDOWNLIST | WS_CHILD | WS_VSCROLL, 100, 100, 200, 80, hwnd, NULL, hInst, NULL);
        SendMessage(hwndBattleDD, WM_SETFONT, (WPARAM)rghfontArial8[1], 0);
        hwndShipDD = CreateWindow(szCombobox, "ShipDD", CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_CHILD | WS_VSCROLL, 100, 100, 200, 80, hwnd,
                                  NULL, hInst, NULL);
        SendMessage(hwndShipDD, WM_SETFONT, (WPARAM)rghfontArial8[1], 0);
        GetClientRect(hwndShipDD, &rc);
        dyShipDD = rc.bottom;
        hwndShipLB = CreateWindow(szListbox, "ShipLB", LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | LBS_DISABLENOSCROLL | WS_CHILD | WS_BORDER | WS_VSCROLL, 100, 100,
                                  200, 80, hwnd, NULL, hInst, NULL);
        SendMessage(hwndShipLB, WM_SETFONT, (WPARAM)rghfontArial8[1], 0);
        hwndFleetCompLB =
            CreateWindow(szListbox, "FleetCompLB", LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_BORDER | WS_VSCROLL,
                         100, 100, 200, 80, hwnd, NULL, hInst, NULL);
        SendMessage(hwndFleetCompLB, WM_SETFONT, (WPARAM)rghfontArial8[0], 0);
        hwndPlanetProdLB =
            CreateWindow(szListbox, "PlanetProdLB", LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_BORDER | WS_VSCROLL,
                         100, 100, 200, 80, hwnd, NULL, hInst, NULL);
        SendMessage(hwndPlanetProdLB, WM_SETFONT, (WPARAM)rghfontArial8[0], 0);
        for (i = 0; i < 13; i++) {
            psz = PszGetCompressedString(idsCargo2 + i);
            rghwndBtn[i] = CreateWindow(szButton, psz, WS_CHILD, 100, 100, 100, dyArial8 * 2, hwnd, NULL, hInst, NULL);
            SendMessage(rghwndBtn[i], WM_SETFONT, (WPARAM)rghfontArial8[1], 0);
        }
        hwndRepCB =
            CreateWindow(szButton, PszGetCompressedString(idsRepeatOrders), BS_AUTOCHECKBOX | WS_CHILD, 100, 100, 150, dyArial8, hwnd, NULL, hInst, NULL);
        SendMessage(hwndRepCB, WM_SETFONT, (WPARAM)rghfontArial8[1], 0);
        break;
    case WM_ERASEBKGND:
        GetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        if ((HWND)lParam == hwndRepCB) {
            SetBkColor((HDC)wParam, crButtonFace);
            SetTextColor((HDC)wParam, crButtonText);
            return (LRESULT)hbrButtonFace;
        }
        goto Default;
    case WM_GETMINMAXINFO:
        ((MINMAXINFO *)lParam)->ptMinTrackSize.x = dxWinFrame * 2 + 198;
        ((MINMAXINFO *)lParam)->ptMinTrackSize.y = dyWinFrame * 2 + 198 + dyTitleBar;
        goto Default;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
        SetFocus(hwndFrame);
        PlanetClick(LOWORD(lParam), HIWORD(lParam), wParam, message == WM_RBUTTONDOWN);
        return 0;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        DrawPlanShip(hdc, tileAll);
        EndPaint(hwnd, &ps);
        break;
    case WM_SETCURSOR:
        hcs = 0;
        GetCursorPos16(&pt);
        ScreenToClient16(hwnd, &pt);
        GetClientRect(hwnd, &rc);
        if (PtInRect(&rc, PointFrom16(pt)) == 0)
            goto Default;
        hcs = ClickInShipOrders(pt, 0, TRUE, FALSE);
        if (!hcs) {
            hcs = ClickInPlanetOrders(pt, 0, TRUE, FALSE);
        }
        if (!hcs)
            goto Default;
        SetCursor(hcs);
        return 1;
    case WM_DRAWITEM:
        lpdis = (DRAWITEMSTRUCT *)lParam;
        if (lpdis->itemID == -1) {
            HandleFocusState(lpdis, -2);
        } else {
            switch (lpdis->itemAction) {
            case ODA_DRAWENTIRE:
                DrawCBEntireItem(lpdis, -4);
                break;
            case ODA_SELECT:
                DrawCBEntireItem(lpdis, -4);
                break;
            case ODA_FOCUS:
                DrawCBEntireItem(lpdis, -4);
            }
        }
        return 1;
    case WM_MEASUREITEM:
        lpmis = (MEASUREITEMSTRUCT *)lParam;
        lpmis->itemHeight = dyArial8 + 2;
        return 1;
    case WM_CHAR:
        if (wParam != 'f' && wParam != 'F') {
            return 0;
        }
        lppl = lpPlanets;
        lpplMac = lpPlanets + cPlanet;
        for (; lppl < lpplMac; lppl++) {
            if (lppl->iPlayer == idPlayer) {
                if (sel.grobj == grobjPlanet && sel.id == lppl->id)
                    break;
                SelectAdjPlanet(0, lppl->id);
                return 0;
            }
        }
        for (i = 0; i < cFleet; i++) {
            lpfl = rglpfl[i];
            if (!rglpfl[i])
                break;
            if (lpfl->iPlayer == idPlayer) {
                if (sel.grobj == grobjFleet && sel.id == lpfl->id)
                    break;
                SelectAdjFleet(0, lpfl->id);
                return 0;
            }
        }
        return 0;
    case WM_COMMAND:
        if (sel.grobj == grobjFleet) {
            ShipCommandProc(hwnd, wParam, lParam);
            return 0;
        }
        if ((HWND)lParam == hwndShipDD) {
            if (HIWORD(wParam) == 1) {
                DrawPlanShip(NULL, tileShipList | tileErase);
            }
        } else {
            if ((HWND)lParam == rghwndBtn[4] && HIWORD(wParam) == 0) {
                if (GetKeyState(VK_SHIFT) < 0) {
                    SelectAdjPlanet(0, IdFindAdjStarbase(sel.pl.id, FALSE));
                    goto LRefocus;
                }
                SelectAdjPlanet(-1, 0);
            LRefocus:
                SetFocus(hwndFrame);
                return 0;
            } else if ((HWND)lParam == rghwndBtn[5] && HIWORD(wParam) == 0) {
                if (GetKeyState(VK_SHIFT) < 0) {
                    SelectAdjPlanet(0, IdFindAdjStarbase(sel.pl.id, TRUE));
                    goto LRefocus;
                }
                SelectAdjPlanet(1, 0);
                goto LRefocus;
            } else if ((HWND)lParam == rghwndBtn[0] && HIWORD(wParam) == 0) {
                lSel = SendMessage(hwndShipDD, CB_GETCURSEL, 0, 0);
                if (lSel == -1 || !FLookupOrbitingXfer(sel.pl.id, LOWORD(lSel), &xf, idflNone)) {
                    return 0;
                }
                TransferStuff(sel.pl.id, grobjPlanet, xf.id, xf.grobj, mdXferCargo);
                goto LRefocus;
            } else if ((HWND)lParam == rghwndBtn[1] && HIWORD(wParam) == 0) {
                lSel = SendMessage(hwndShipDD, CB_GETCURSEL, 0, 0);
                if (lSel != -1 && FLookupOrbitingXfer(sel.pl.id, LOWORD(lSel), &xf, idflNone) && xf.grobj == grobjFleet) {
                    SelectAdjFleet(0, xf.id);
                }
                goto LRefocus;
            } else if ((HWND)lParam == rghwndBtn[2] && HIWORD(wParam) == 0) {
                pt.x = 610;
                pt.y = 470;
                ShipBuilder(pt);
                goto LRefocus;
            } else if ((HWND)lParam == rghwndBtn[11] && HIWORD(wParam) == 0) {
                ChangeProduction(FALSE);
                goto LRefocus;
            } else if ((HWND)lParam == rghwndBtn[12] && HIWORD(wParam) == 0) {
                if (AlertSz(PszFormatIds(idsSureWantDeleteEverythingPlanetsProductionQueue, NULL), MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL) != IDYES) {
                    return 0;
                }
                ChangeProduction(TRUE);
                goto LRefocus;
            } else {
                if ((HWND)lParam != hwndPlanetProdLB || HIWORD(wParam) != 1)
                    goto Default;
                DrawPlanShip(NULL, tileProductionOrOrbit);
                if (!gd.fTutorial)
                    goto Default;
                tutor.fProgress = TRUE;
                AdvanceTutor();
                goto Default;
            }
        }
    default:
    Default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

int16_t FDrawTileNC(HDC hdc, TILE *ptile, RECT *prc, char *pszTitle) {
    int16_t bt;
    RECT    rcT;

    bt = 112;
    prc->left = ptile->iCol * 198 + 4;
    prc->right = prc->left + 190;
    prc->top = ptile->yTop;
    prc->bottom = (!ptile->fPopped ? dyArial8 + 3 : ptile->dyFull) + prc->top;
    if (ptile->fMinDraw && !ptile->fMinTitle)
        goto FinishUp;

    if (!ptile->fMinDraw) {
        _Draw3dFrame(hdc, prc, 0);
    }
    rcT = *prc;
    ExpandRc(&rcT, -1, -1);
    rcT.bottom = rcT.top + dyArial8 + 2;
    SelectObject(hdc, rghfontArial8[1]);
    SetTextColor(hdc, crButtonText);
    SetBkColor(hdc, crButtonFace);
    if (!ptile->fMinDraw) {
        _Draw3dFrame(hdc, &rcT, 0);
    }
    RcCtrTextOut(hdc, &rcT, pszTitle, -1);
    SetRect(&rcT, prc->right - 17, prc->top + 1, prc->right, rcT.bottom + 1);
    if (ptile->fPopped) {
        bt = bt;
    } else {
        bt |= 1;
    }
    DrawBtn(hdc, &rcT, bt, FALSE, NULL);
    SelectObject(hdc, hbrButtonShadow);
    PatBlt(hdc, prc->right - 18, prc->top + 1, 1, rcT.bottom - prc->top - 1, PATCOPY);

FinishUp:
    prc->top += dyArial8 + 4;
    return ptile->fPopped;
}

void DrawPlanetMinSum(HDC hdc, TILE *ptile, OBJ obj) {
    int16_t dxRight;
    int16_t yTop;
    int16_t xRight;
    int16_t c;
    int16_t i;
    int16_t xLeft;
    HBRUSH  hbrSav;
    PLANET *ppl;
    RECT    rc;

    ppl = obj.ppl;
    if (ptile->fFixCtls) {
        rgrcRef[6].top = -5;
        rgrcRef[6].bottom = -6;
        rgrcRef[7].top = -5;
        rgrcRef[7].bottom = -6;
        rgrcRef[8].top = -5;
        rgrcRef[8].bottom = -6;
        rgrcRef[9].top = -5;
        rgrcRef[9].bottom = -6;
        rgrcRef[11].top = -5;
        rgrcRef[11].bottom = -6;
        rgrcRef[10].top = -5;
        rgrcRef[10].bottom = -6;
    }
    if (FDrawTileNC(hdc, ptile, &rc, PszGetCompressedString(idsMineralsHand))) {
        if (!ppl) {
            ppl = &sel.pl;
        }
        xLeft = rc.left + 4;
        xRight = rc.right - 4;
        yTop = rc.top;
        dxRight = dxMaxMineralQuan;
        SetRect(&rgrcRef[6], xLeft, yTop, xRight, 3 * dyArial8 + yTop);
        for (i = 0; i <= 2; i++) {
            if (!ptile->fMinDraw) {
                SelectObject(hdc, rghfontArial8[1]);
                SetTextColor(hdc, rgcrMinerals[i]);
                TextOut(hdc, xLeft, yTop, rgszMinerals[i], lstrlen(rgszMinerals[i]));
            }
            SelectObject(hdc, rghfontArial8[0]);
            SetTextColor(hdc, crButtonText);
            c = wsprintf(szWork, PszGetCompressedString(idsLdkt), ppl->rgwtMin[i]);
            RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
            yTop += dyArial8;
        }
        hbrSav = SelectObject(hdc, hbrButtonHilite);
        PatBlt(hdc, rc.left, yTop++, rc.right - rc.left, 1, PATCOPY);
        SelectObject(hdc, hbrSav);
        SetRect(&rgrcRef[7], xLeft, yTop, xRight, dyArial8 * 2 + yTop);
        if (!ptile->fMinDraw) {
            SelectObject(hdc, rghfontArial8[1]);
            c = CchGetString(idsMines4, szWork);
            TextOut(hdc, xLeft, yTop, szWork, c);
            SelectObject(hdc, rghfontArial8[0]);
        }
        if (GetRaceStat(&rgplr[idPlayer], rsMajorAdv) == raMacintosh) {
            c = wsprintf(szWork, PszGetCompressedString(idsD), CMinesOperating(ppl));
        } else {
            c = wsprintf(szWork, PszGetCompressedString(idsDD), ppl->cMines, CMaxOperableMines(ppl, idPlayer, FALSE));
        }
        RightTextOut(hdc, xRight, yTop, szWork, c, dxRight * 2);
        yTop += dyArial8;
        if (!ptile->fMinDraw) {
            SelectObject(hdc, rghfontArial8[1]);
            c = CchGetString(idsFactories4, szWork);
            TextOut(hdc, xLeft, yTop, szWork, c);
            SelectObject(hdc, rghfontArial8[0]);
        }
        if (GetRaceStat(&rgplr[idPlayer], rsMajorAdv) == raMacintosh) {
            c = CchGetString(idsN, szWork);
        } else {
            c = wsprintf(szWork, PszGetCompressedString(idsDD), ppl->cFactories, CMaxOperableFactories(ppl, idPlayer, FALSE));
        }
        RightTextOut(hdc, xRight, yTop, szWork, c, dxRight * 2);
        yTop += dyArial8;
    }
    return;
}

void DrawPlanetStats(HDC hdc, TILE *ptile, OBJ obj) {
    int16_t dxRight;
    int32_t l2;
    int16_t yTop;
    int16_t xRight;
    int16_t c;
    int16_t cRes;
    int16_t dRangeP;
    float   pct;
    int16_t cResAvail;
    int16_t dRange;
    char   *psz;
    int16_t xLeft;
    HBRUSH  hbrSav;
    int32_t l;
    RECT    rc;
    PART    part;

    if (FDrawTileNC(hdc, ptile, &rc, PszGetCompressedString(idsStatus))) {
        xLeft = rc.left + 4;
        xRight = rc.right - 4;
        yTop = rc.top;
        SelectObject(hdc, rghfontArial8[1]);
        c = CchGetString(idsPopulation4, szWork);
        l = GetTextExtent(hdc, szWork, c);
        c = CchGetString(idsScannerType, szWork);
        l2 = GetTextExtent(hdc, szWork, c);
        if (l2 > l) {
            l = l2;
        }
        c = CchGetString(idsScannerRange, szWork);
        l2 = GetTextExtent(hdc, szWork, c);
        if (l2 > l) {
            l = l2;
        }
        c = CchGetString(idsDefenses4, szWork);
        l2 = GetTextExtent(hdc, szWork, c);
        if (l2 > l) {
            l = l2;
        }
        c = CchGetString(idsDefenseType, szWork);
        l2 = GetTextExtent(hdc, szWork, c);
        if (l2 > l) {
            l = l2;
        }
        c = CchGetString(idsDefCoverage, szWork);
        l2 = GetTextExtent(hdc, szWork, c);
        if (l2 > l) {
            l = l2;
        }
        c = CchGetString(idsResourcesYear, szWork);
        l2 = GetTextExtent(hdc, szWork, c);
        if (l2 > l) {
            l = l2;
        }
        dxRight = xRight - xLeft - LOWORD(l);
        if (!ptile->fMinDraw) {
            c = CchGetString(idsPopulation4, szWork);
            TextOut(hdc, xLeft, yTop, szWork, c);
        }
        SelectObject(hdc, rghfontArial8[0]);
        SetRect(&rgrcRef[9], xLeft, yTop, xRight, yTop + dyArial8);
        c = CommaFormatLong(szWork, (uint32_t)(sel.pl.rgwtMin[3] * 100));
        RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
        yTop += dyArial8;
        if (!ptile->fMinDraw) {
            SelectObject(hdc, rghfontArial8[1]);
            c = CchGetString(idsResourcesYear, szWork);
            TextOut(hdc, xLeft, yTop, szWork, c);
            SelectObject(hdc, rghfontArial8[0]);
        }
        SetRect(&rgrcRef[8], xLeft, yTop, xRight, yTop + dyArial8);
        cResAvail = CResourcesAtPlanet(&sel.pl, idPlayer);
        cRes = cResAvail;
        if (!sel.pl.fNoResearch) {
            cResAvail -= MulDiv(cRes, rgplr[idPlayer].pctResearch, 100);
        }
        c = wsprintf(szWork, PszGetCompressedString(idsDD), cResAvail, cRes);
        RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
        yTop += dyArial8;
        hbrSav = SelectObject(hdc, hbrButtonHilite);
        PatBlt(hdc, rc.left, yTop++, rc.right - rc.left, 1, PATCOPY);
        SelectObject(hdc, hbrSav);
        if (!ptile->fMinDraw) {
            SelectObject(hdc, rghfontArial8[1]);
            c = CchGetString(idsScannerType, szWork);
            TextOut(hdc, xLeft, yTop, szWork, c);
            SelectObject(hdc, rghfontArial8[0]);
        }
        SetRect(&rgrcRef[11], xLeft, yTop, xRight, dyArial8 * 2 + yTop);
        if (GetRaceStat(&rgplr[idPlayer], rsMajorAdv) == raMacintosh) {
            CchGetString(idsOrganic, szWork);
            dRange = GetPlanetScannerRange(&sel.pl, &dRangeP);
        } else if (sel.pl.iScanner == 31) {
            CchGetString(idsNone4, szWork);
            dRange = 0;
        } else {
            LookupBestPlanetaryScanner(&part);
            strcpy(szWork, part.pcom->szName);
            dRange = GetPlanetScannerRange(&sel.pl, &dRangeP);
        }
        RightTextOut(hdc, xRight, yTop, szWork, 0, dxRight);
        yTop += dyArial8;
        if (!ptile->fMinDraw) {
            SelectObject(hdc, rghfontArial8[1]);
            c = CchGetString(idsScannerRange, szWork);
            TextOut(hdc, xLeft, yTop, szWork, c);
            SelectObject(hdc, rghfontArial8[0]);
        }
        if (dRange > 0) {
            if (dRangeP > 0) {
                c = wsprintf(szWork, PszGetCompressedString(idsDDLY), dRangeP, dRange);
            } else if (dRange < 100) {
                c = wsprintf(szWork, PszGetCompressedString(idsDLightYears), dRange);
            } else {
                c = wsprintf(szWork, PszGetCompressedString(idsDLY), dRange);
            }
        } else {
            CchGetString(idsNone4, szWork);
            c = 0;
        }
        RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
        yTop += dyArial8;
        hbrSav = SelectObject(hdc, hbrButtonHilite);
        PatBlt(hdc, rc.left, yTop++, rc.right - rc.left, 1, PATCOPY);
        SelectObject(hdc, hbrSav);
        if (!ptile->fMinDraw) {
            SelectObject(hdc, rghfontArial8[1]);
            c = CchGetString(idsDefenses4, szWork);
            TextOut(hdc, xLeft, yTop, szWork, c);
        }
        SelectObject(hdc, rghfontArial8[0]);
        SetRect(&rgrcRef[10], xLeft, yTop, xRight, 3 * dyArial8 + yTop);
        if (GetRaceStat(&rgplr[idPlayer], rsMajorAdv) == raMacintosh) {
            c = CchGetString(idsN, szWork);
        } else {
            c = wsprintf(szWork, PszGetCompressedString(idsDD), sel.pl.cDefenses, CMaxOperableDefenses(&sel.pl, idPlayer, FALSE));
        }
        RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
        yTop += dyArial8;
        if (!ptile->fMinDraw) {
            SelectObject(hdc, rghfontArial8[1]);
            c = CchGetString(idsDefenseType, szWork);
            TextOut(hdc, xLeft, yTop, szWork, c);
            SelectObject(hdc, rghfontArial8[0]);
        }
        if (sel.pl.cDefenses == 0) {
            CchGetString(GetRaceStat(&rgplr[idPlayer], rsMajorAdv) == raMacintosh ? idsN : idsNone4, szWork);
            dRange = 0;
        } else {
            FGetBestDefensePart(&part);
            strcpy(szWork, part.pcom->szName);
            dRange = 1;
        }
        RightTextOut(hdc, xRight, yTop, szWork, 0, dxRight);
        yTop += dyArial8;
        if (!ptile->fMinDraw) {
            SelectObject(hdc, rghfontArial8[1]);
            c = CchGetString(idsDefCoverage, szWork);
            TextOut(hdc, xLeft, yTop, szWork, c);
            SelectObject(hdc, rghfontArial8[0]);
        }
        if (dRange != 0) {
            CalcPctSurvive(&sel.pl, &pct, NULL);
            pct = Sf32From80((Sf80Sub(Sf80From64(1.0), Sf80From32(pct))));
            c = wsprintf(szWork, PCTDXPCTDPCTPCT, LOWORD(Sf80ToI32((Sf80Mul(Sf80From32(pct), Sf80FromI32(100))))),
                         LOWORD(Sf80ToI32(
                             (Sf80Mul((Sf80Sub(Sf80From32(pct), Sf80Div(Sf80FromI32((int16_t)LOWORD(Sf80ToI32((Sf80Mul(Sf80From32(pct), Sf80FromI32(100)))))),
                                                                        Sf80From64(100.0)))),
                                      Sf80FromI32(10000))))));
        } else {
            c = CchGetString(GetRaceStat(&rgplr[idPlayer], rsMajorAdv) == raMacintosh ? idsN : idsNone4, szWork);
            psz = szWork;
        }
        RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
    }
    return;
}

void DrawPlanetStarbase(HDC hdc, TILE *ptile, OBJ obj) {
    int16_t  fTwo;
    int16_t  dxRight;
    int16_t  iWarp;
    int16_t  bt;
    int16_t  yTop;
    int16_t  xRight;
    int16_t  c;
    SHDEF   *lpshdef;
    COLORREF crForeSav;
    uint16_t w;
    char    *psz;
    int16_t  xLeft;
    HBRUSH   hbrSav;
    int32_t  l;
    RECT     rc;

    if (ptile->fFixCtls) {
        rgrcRef[13].top = -5;
        rgrcRef[13].bottom = -6;
        rgrcRef[14].top = -5;
        rgrcRef[14].bottom = -6;
        rgrcRef[16].top = -5;
        rgrcRef[16].bottom = -6;
        rgrcRef[15].top = -5;
        rgrcRef[15].bottom = -6;
        ptile->fFixCtls = FALSE;
    }
    if (sel.pl.fStarbase) {
        lpshdef = rglpshdefSB[idPlayer] + sel.pl.isb;
        strcpy(szWork, lpshdef->hul.szClass);
        psz = szWork;
    } else {
        psz = PszGetCompressedString(idsStarbase2);
    }
    if (FDrawTileNC(hdc, ptile, &rc, psz)) {
        xLeft = rc.left + 4;
        xRight = rc.right - 4;
        yTop = rc.top;
        SetRect(&rc, xLeft - 2, yTop, xRight + 2, rc.bottom - 2);
        FillRect(hdc, &rc, hbrButtonFace);
        if (!sel.pl.fStarbase) {
            SetRect(&rgrcRef[14], -5, -5, -6, -6);
            rgrcRef[15] = rgrcRef[14];
            rgrcRef[16] = rgrcRef[14];
        } else {
            SetRect(&rgrcRef[14], xLeft, yTop, xRight, dyArial8 * 4 + yTop);
            SelectObject(hdc, rghfontArial8[1]);
            c = CchGetString(idsDockCapacity, szWork);
            l = GetTextExtent(hdc, szWork, c);
            dxRight = xRight - xLeft - LOWORD(l);
            TextOut(hdc, xLeft, yTop, szWork, c);
            SelectObject(hdc, rghfontArial8[0]);
            w = LphuldefFromId(lpshdef->hul.ihuldef)->hul.wtCargoMax;
            if (w == 0) {
                c = CchGetString(idsNone4, szWork);
            } else if ((uint32_t)w == 0xffff) {
                c = CchGetString(idsUnlimited, szWork);
            } else {
                c = wsprintf(szWork, PCTDKT, w);
            }
            RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
            yTop += dyArial8;
            SelectObject(hdc, rghfontArial8[1]);
            c = CchGetString(idsArmor2, szWork);
            TextOut(hdc, xLeft, yTop, szWork, c);
            w = lpshdef->hul.dp;
            if (w == 0) {
                c = CchGetString(idsNone4, szWork);
            } else {
                c = wsprintf(szWork, PszGetCompressedString(idsLddp), w, 0);
            }
            SelectObject(hdc, rghfontArial8[0]);
            RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
            yTop += dyArial8;
            SelectObject(hdc, rghfontArial8[1]);
            c = CchGetString(idsShields2, szWork);
            TextOut(hdc, xLeft, yTop, szWork, c);
            l = DpShieldOfShdef(lpshdef, idPlayer);
            if (l == 0) {
                c = CchGetString(idsNone4, szWork);
            } else {
                c = wsprintf(szWork, PszGetCompressedString(idsLddp), l);
            }
            SelectObject(hdc, rghfontArial8[0]);
            RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
            yTop += dyArial8;
            SelectObject(hdc, rghfontArial8[1]);
            c = CchGetString(idsDamage2, szWork);
            TextOut(hdc, xLeft, yTop, szWork, c);
            SelectObject(hdc, rghfontArial8[0]);
            w = sel.pl.pctDp;
            if (w != 0) {
                if (w < 5) {
                    w = 5;
                }
                c = wsprintf(szWork, PCTDPCTPCT, (uint32_t)w / 5);
                crForeSav = SetTextColor(hdc, 127);
            } else {
                c = CchGetString(idsNone4, szWork);
            }
            RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
            yTop += dyArial8;
            if (w != 0) {
                SetTextColor(hdc, crForeSav);
            }
            hbrSav = SelectObject(hdc, hbrButtonHilite);
            PatBlt(hdc, rc.left, yTop++, rc.right - rc.left, 1, PATCOPY);
            SelectObject(hdc, hbrSav);
            SelectObject(hdc, rghfontArial8[1]);
            SetRect(&rgrcRef[16], xLeft, yTop, xRight, yTop + dyArial8);
            c = CchGetString(idsMassDriver, szWork);
            TextOut(hdc, xLeft, yTop, szWork, c);
            iWarp = IWarpMAFromLppl(&sel.pl, &fTwo);
            if (iWarp > 0) {
                c = wsprintf(szWork, PszGetCompressedString(idsWarpD), iWarp);
                if (fTwo) {
                    szWork[c++] = '+';
                }
            } else {
                c = wsprintf(szWork, PszGetCompressedString(idsNone4));
            }
            SelectObject(hdc, rghfontArial8[0]);
            RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
            yTop += dyArial8;
            SelectObject(hdc, rghfontArial8[1]);
            c = CchGetString(idsDestination3, szWork);
            TextOut(hdc, xLeft, yTop, szWork, c);
            if (sel.pl.idFling == 0) {
                c = CchGetString(idsNone4, szWork);
            } else {
                psz = PszGetCompressedPlanet(rgidPlan[sel.pl.idFling - 1]);
                c = 0;
                strcpy(szWork, psz);
            }
            SelectObject(hdc, rghfontArial8[0]);
            RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
            yTop += dyArial8;
            c = (int16_t)(xRight - xLeft) / 3;
            SetRect(&rgrcRef[13], xLeft, yTop, xLeft + c, yTop + dyArial8 + 6);
            bt = 8;
            if (iWarp == 0) {
                bt |= 4;
            }
            DrawBtn(hdc, &rgrcRef[13], bt, gd.fSetMassMode, PszGetCompressedString(idsSetDest));
            if (iWarp > 0) {
                SetRect(&rgrcRef[15], xLeft + c + 4, yTop + 3, xRight, yTop + dyArial8 + 3);
                DrawMassWarpGauge(hdc, &rgrcRef[15], !fTwo ? iWarp : -iWarp, sel.pl.iWarpFling + 4);
            } else {
                SetRect(&rgrcRef[15], -5, -5, -6, -6);
                rgrcRef[16] = rgrcRef[15];
            }
        }
    }
    return;
}

void DrawMassWarpGauge(HDC hdc, RECT *prc, int16_t iBest, int16_t iCur) {
    int32_t lMax;
    int16_t c;
    int16_t fTwoMAs;
    int16_t iMode;
    HBRUSH  hbr;
    int32_t lCur;
    int32_t l;

    fTwoMAs = iBest < 0;
    SelectObject(hdc, rghfontArial8[1]);
    if (iCur < 5) {
        iCur = 5;
    }
    if (iBest < 0) {
        iBest = -iBest;
    }
    lMax = (int16_t)(iBest - 1);
    if (iCur <= iBest + fTwoMAs) {
        hbr = hbrPurple;
    } else if (iCur < iBest + fTwoMAs + 3) {
        hbr = hbrYellow;
    } else {
        hbr = hbrRed;
    }
    lCur = (int16_t)(iCur - 4);
    l = LDrawGauge(hdc, prc, 1, &lCur, &hbr, lMax);
    iMode = SetBkMode(hdc, TRANSPARENT);
    c = wsprintf(szWork, PszGetCompressedString(idsWarpLd), l + 4);
    l = GetTextExtent(hdc, szWork, c);
    RcCtrTextOut(hdc, prc, szWork, c);
    SetBkMode(hdc, iMode);
    return;
}

void DrawPlanetProduction(HDC hdc, TILE *ptile, OBJ obj) {
    int16_t swp;
    int16_t dxRight;
    int16_t yTop;
    int16_t xStart;
    int16_t xRight;
    char    szT[40];
    int16_t i;
    int16_t c;
    int16_t dyWrong;
    char   *psz;
    int16_t iSel;
    int16_t cch;
    int16_t xLeft;
    RECT    rcT;
    PLANET *ppl;
    RECT    rc;

    ppl = obj.ppl;
    if (ptile->fFixCtls) {
        ShowWindow(hwndPlanetProdLB, SW_HIDE);
        ShowWindow(rghwndBtn[11], SW_HIDE);
        ShowWindow(rghwndBtn[12], SW_HIDE);
        rgrcRef[17].top = -5;
        rgrcRef[17].bottom = -6;
        ptile->fFixCtls = FALSE;
    }
    if (!FDrawTileNC(hdc, ptile, &rc, PszGetCompressedString(idsProduction))) {
        ShowWindow(hwndPlanetProdLB, SW_HIDE);
        ShowWindow(rghwndBtn[11], SW_HIDE);
        ShowWindow(rghwndBtn[12], SW_HIDE);
    } else {
        xLeft = rc.left + 4;
        xRight = rc.right - 4;
        yTop = rc.top;
        yTop += 4;
        GetClientRect(hwndPlanetProdLB, &rcT);
        swp = SWP_NOZORDER | SWP_NOACTIVATE;
        dyPlanetProdLB = (dyArial8 + 2) * (!gd.fSmallTileMode ? 5 : 3);
        dyWrong = dyPlanetProdLB - (rcT.bottom - rcT.top);
        if (dxPlanetProdLB == xRight - xLeft && dyWrong >= 0 && dyWrong < dyArial8) {
            swp |= SWP_NOSIZE;
        } else {
            dxPlanetProdLB = xRight - xLeft;
        }
        SetWindowPos(hwndPlanetProdLB, NULL, xLeft, yTop, xRight - xLeft, dyPlanetProdLB, swp);
        ShowWindow(hwndPlanetProdLB, SW_SHOW);
        GetClientRect(hwndPlanetProdLB, &rcT);
        dyPlanetProdLB = rcT.bottom - rcT.top;
        yTop += dyPlanetProdLB + 4;
        iSel = LOWORD(SendMessage(hwndPlanetProdLB, LB_GETCURSEL, 0, 0));
        if (iSel < 0) {
            iSel = 0;
        }
        if (!ppl->lpplprod || ppl->lpplprod->iprodMac == 0) {
            c = 0;
            szWork[0] = 0;
        } else {
            psz = PszProductionETA(ppl, NULL, iSel, NULL, NULL);
            c = strlen(psz);
        }
        if (c != 0) {
            SelectObject(hdc, rghfontArial8[1]);
            cch = CchGetString(idsCompletion, szT);
            TextOut(hdc, xLeft, yTop, szT, cch);
            dxRight = xRight - xLeft - LOWORD(GetTextExtent(hdc, szT, cch));
            SelectObject(hdc, rghfontArial8[0]);
            RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
        } else {
            RightTextOut(hdc, xRight, yTop, szWork, 0, xRight - xLeft);
        }
        yTop += dyArial8;
        SelectObject(hdc, rghfontArial8[1]);
        cch = CchGetString(idsRoute3, szT);
        TextOut(hdc, xLeft, yTop, szT, cch);
        dxRight = xRight - xLeft - LOWORD(GetTextExtent(hdc, szT, cch));
        if (sel.pl.idRoute == 0) {
            c = CchGetString(idsNone4, szWork);
        } else {
            psz = PszGetCompressedPlanet(rgidPlan[sel.pl.idRoute - 1]);
            c = 0;
            strcpy(szWork, psz);
        }
        SelectObject(hdc, rghfontArial8[0]);
        RightTextOut(hdc, xRight, yTop, szWork, c, dxRight);
        yTop += dyArial8;
        c = (int16_t)(xRight - xLeft - 16) / 3;
        xStart = xLeft;
        i = 11;
        while (i <= 12) {
            SetWindowPos(rghwndBtn[i], NULL, xStart, yTop, c, (dyArial8 >> 1) + dyArial8, SWP_NOZORDER | SWP_NOACTIVATE);
            ShowWindow(rghwndBtn[i], SW_SHOW);
            i++;
            xStart += c + 8;
        }
        EnableWindow(rghwndBtn[12], sel.pl.lpplprod && sel.pl.lpplprod->iprodMac != 0);
        SetRect(&rgrcRef[17], xStart, yTop, xStart + c, yTop + dyArial8 + (dyArial8 >> 1));
        DrawBtn(hdc, &rgrcRef[17], 8, gd.fSetRouteMode, PszGetCompressedString(idsRoute2));
    }
    return;
}

void DrawPlanShipBitmap(HDC hdc, TILE *ptile, OBJ obj) {
    int16_t yTop;
    int16_t dy;
    int16_t xRight;
    int16_t i;
    char   *psz;
    int16_t dx;
    int16_t xLeft;
    HBRUSH  hbrSav;
    int16_t iOffset;
    RECT    rc;

    if (sel.grobj == grobjPlanet) {
        psz = PszGetPlanetName(obj.ppl->id);
        i = obj.ppl->id;
        i += 8;
        iOffset = i % 28;
    } else {
        psz = PszGetFleetName(obj.pfl->id);
    }
    if (ptile->fFixCtls) {
        for (i = 4; i <= 6; i++) {
            ShowWindow(rghwndBtn[i], SW_HIDE);
        }
        ptile->fFixCtls = FALSE;
    }
    if (!FDrawTileNC(hdc, ptile, &rc, psz)) {
        for (i = 4; i <= 6; i++) {
            ShowWindow(rghwndBtn[i], SW_HIDE);
        }
    } else {
        xLeft = rc.left + 12;
        xRight = rc.right - 12;
        yTop = (!gd.fSmallTileMode ? 6 : 2) + rc.top;
        if (sel.grobj == grobjFleet) {
            DrawFleetBitmap(&sel.fl, hdc, xLeft, yTop, TRUE, -1, 0, FALSE, -1, 0);
            goto DoBtns;
        }
        hbrSav = SelectObject(hdc, hbrButtonShadow);
        PatBlt(hdc, xLeft, yTop, 70, 2, PATCOPY);
        PatBlt(hdc, xLeft, yTop + 2, 2, 68, PATCOPY);
        SelectObject(hdc, hbrButtonHilite);
        PatBlt(hdc, xLeft + 2, yTop + 68, 68, 2, PATCOPY);
        PatBlt(hdc, xLeft + 68, yTop + 2, 2, 66, PATCOPY);
        PatBlt(hdc, xLeft + 1, yTop + 69, 1, 1, PATCOPY);
        PatBlt(hdc, xLeft + 69, yTop + 1, 1, 1, PATCOPY);
        PatBlt(hdc, xLeft + 2, yTop + 2, 66, 1, BLACKNESS);
        PatBlt(hdc, xLeft + 2, yTop + 3, 1, 65, BLACKNESS);
        PatBlt(hdc, xLeft + 3, yTop + 67, 65, 1, BLACKNESS);
        PatBlt(hdc, xLeft + 67, yTop + 3, 1, 64, BLACKNESS);
        SelectObject(hdc, hbrSav);
        SelectPalette(hdc, vhpal, FALSE);
        RealizePalette(hdc);
        DibBlt(hdc, xLeft + 3, yTop + 3, 64, 64, hdibPlanets, iOffset % 7 * 64, iOffset / 7 * 64, 64, 64, 13369376);
    DoBtns:
        dx = xRight - xLeft - 95;
        dy = 3 * dyArial8 >> 1;
        xLeft = xRight - dx;
        if (!ptile->fMinDraw) {
            if (gd.fSmallTileMode) {
                yTop -= 2;
                dy -= 2;
            } else {
                yTop -= 4;
            }
            iOffset = sel.grobj == grobjFleet ? 6 : 5;
            i = 4;
            while (i <= iOffset) {
                SetWindowPos(rghwndBtn[i], NULL, xLeft, yTop, dx, dy, SWP_NOZORDER | SWP_NOACTIVATE);
                ShowWindow(rghwndBtn[i], SW_SHOW);
                i++;
                yTop += (!gd.fSmallTileMode ? 3 : 2) + dy;
            }
        }
    }
    return;
}

void DrawPlanetShipList(HDC hdc, TILE *ptile, OBJ obj) {
    int16_t swp;
    int16_t fDoneDrawing;
    int32_t l2;
    int16_t yTop;
    int16_t fObjIsThing;
    int16_t fUnknown;
    int16_t idSkip;
    int16_t xStart;
    int16_t xRight;
    int16_t i;
    int16_t c;
    RECT    rcGauge;
    XFER    xf;
    FLEET  *pfl;
    int32_t lSel;
    int16_t xLeft;
    int32_t l;
    RECT    rc;

    pfl = obj.pfl;
    fDoneDrawing = FALSE;
    fObjIsThing = FALSE;
    if (ptile->fFixCtls) {
        for (i = 0; i <= 2; i++) {
            ShowWindow(rghwndBtn[i], SW_HIDE);
        }
        ShowWindow(hwndShipDD, SW_HIDE);
        ptile->fFixCtls = FALSE;
        rgrcRef[1].top = -5;
        rgrcRef[1].bottom = -6;
        rgrcRef[4].top = -5;
        rgrcRef[4].bottom = -6;
    }
    if (!FDrawTileNC(hdc, ptile, &rc, PszGetCompressedString(!pfl ? idsFleetsOrbit : idsOtherFleetsHere))) {
        for (i = 0; i <= 2; i++) {
            ShowWindow(rghwndBtn[i], SW_HIDE);
        }
        ShowWindow(hwndShipDD, SW_HIDE);
    } else {
        xLeft = rc.left + 4;
        xRight = rc.right - 4;
        yTop = rc.top + 2;
        swp = SWP_NOZORDER | SWP_NOACTIVATE;
        if (dxShipDD == xRight - xLeft) {
            swp |= SWP_NOSIZE;
        } else {
            dxShipDD = xRight - xLeft;
        }
        SetWindowPos(hwndShipDD, NULL, xLeft, yTop, xRight - xLeft, 5 * dyShipDD, swp);
        ShowWindow(hwndShipDD, SW_SHOW);
        yTop += dyShipDD + 3;
        lSel = SendMessage(hwndShipDD, CB_GETCURSEL, 0, 0);
        EnableWindow(rghwndBtn[0], lSel != -1);
        if (lSel == -1) {
            fDoneDrawing = TRUE;
            fUnknown = TRUE;
        }
        if (pfl) {
            idSkip = pfl->id;
        } else {
            idSkip = idflNone;
        }
        if (!fDoneDrawing && !FLookupOrbitingXfer(idSkip == idflNone ? sel.pl.id : pfl->idPlanet, LOWORD(lSel), &xf, idSkip)) {
            fDoneDrawing = TRUE;
        }
        if (fDoneDrawing) {
            EnableWindow(rghwndBtn[1], FALSE);
        } else {
            fObjIsThing = xf.grobj == grobjThing;
            EnableWindow(rghwndBtn[1], fObjIsThing == 0 && xf.fl.iPlayer == idPlayer);
        }
        rgrcRef[1].top = -5;
        rgrcRef[1].bottom = -6;
        rgrcRef[4].top = -5;
        rgrcRef[4].bottom = -6;
        fUnknown = fObjIsThing == 0 && xf.fl.det != detAll;
        if (!gd.fSmallTileMode) {
            if (!fDoneDrawing && !fUnknown) {
                SelectObject(hdc, rghfontArial8[1]);
                c = CchGetString(idsCargo3, szWork);
                l = GetTextExtent(hdc, szWork, c);
            }
            if (!fDoneDrawing && !fUnknown && !fObjIsThing) {
                c = CchGetString(idsFuel3, szWork);
                l2 = GetTextExtent(hdc, szWork, c);
                if (l2 > l) {
                    l = l2;
                }
                TextOut(hdc, xLeft, yTop, szWork, c);
                SetRect(&rcGauge, xLeft + LOWORD(l), yTop, xRight, yTop + dyArial8);
                if (idSkip != idflNone) {
                    rgrcRef[1] = rcGauge;
                }
                DrawFleetGauge(hdc, &rcGauge, &xf.fl, 4);
            } else if (ptile->fMinDraw || fUnknown != gd.fUnknownShip) {
                SetRect(&rcGauge, xLeft, yTop, xRight, dyArial8 * 2 + yTop + 8);
                FillRect(hdc, &rcGauge, hbrButtonFace);
            }
            gd.fUnknownShip = fUnknown;
            if (!fObjIsThing) {
                yTop += dyArial8 + 4;
            }
            if (!fDoneDrawing && !fUnknown) {
                c = CchGetString(idsCargo3, szWork);
                TextOut(hdc, xLeft, yTop, szWork, c);
                SetRect(&rcGauge, xLeft + LOWORD(l), yTop, xRight, yTop + dyArial8);
                rgrcRef[4] = rcGauge;
                if (fObjIsThing) {
                    DrawThingGauge(hdc, &rcGauge, &xf.th, 5);
                } else {
                    DrawFleetGauge(hdc, &rcGauge, &xf.fl, 5);
                }
                if (fObjIsThing) {
                    OffsetRect(&rcGauge, 0, dyArial8 + 4);
                    FillRect(hdc, &rcGauge, hbrButtonFace);
                    yTop += dyArial8 + 4;
                }
            }
            yTop += dyArial8 + 4;
        }
        c = (int16_t)(xRight - xLeft - 10) / 3;
        xStart = xLeft;
        i = 0;
        while (i <= 2) {
            SetWindowPos(rghwndBtn[(int16_t)(i + 1) % 3], NULL, xStart, yTop, c, (dyArial8 >> 1) + dyArial8, SWP_NOZORDER | SWP_NOACTIVATE);
            if (i != 2 || pfl) {
                ShowWindow(rghwndBtn[i], SW_SHOW);
            }
            i++;
            xStart += c + 6;
        }
        EnableWindow(rghwndBtn[2], (idSkip == idflNone || !fUnknown) && !fObjIsThing);
    }
    return;
}

void PlanetClick(int16_t x, int16_t y, int16_t sks, int16_t fRightBtn) {
    int16_t  bt;
    POINT16  pt;
    int16_t  ctile;
    int16_t  dy;
    RECT     rcTitle;
    int16_t  i;
    int16_t  xRel;
    uint16_t iCol;
    int16_t  iCur;
    TILE    *prgtile;
    RECT     rc;
    BTNT     btnt;
    HDC      hdc;
    TILE     tile;
    POINT16  ptNew;

    bt = 112;
    if (sel.grobj == grobjPlanet) {
        prgtile = rgtilePlanet;
        ctile = 6;
    } else {
        if (sel.grobj != grobjFleet) {
            return;
        }
        prgtile = rgtileShip;
        ctile = 7;
    }
    iCol = (uint32_t)x / 198;
    xRel = x - iCol * 198;
    if (xRel >= 4 && xRel < 194) {
        for (i = 0; i < ctile; i++) {
            if (prgtile[i].iCol >= iCol) {
                if (prgtile[i].iCol > iCol) {
                    return;
                }
                if (y >= prgtile[i].yTop && y < (!prgtile[i].fPopped ? dyArial8 + 3 : prgtile[i].dyFull) + prgtile[i].yTop)
                    break;
            }
        }
        if (i != ctile) {
            pt.x = x;
            pt.y = y;
            rcTitle.top = prgtile[i].yTop;
            rcTitle.bottom = dyArial8 + 3 + rcTitle.top + 1;
            rcTitle.left = iCol * 198 + 4;
            rcTitle.right = rcTitle.left + 191;
            if (PtInRect(&rcTitle, PointFrom16(pt)) != 0 && !fRightBtn) {
                rc = rcTitle;
                rc.top++;
                rc.left = rc.right - 17;
                if (PtInRect(&rc, PointFrom16(pt)) != 0) {
                    OffsetRc(&rc, -1, 0);
                    if (prgtile[i].fPopped) {
                        bt = bt;
                    } else {
                        bt |= 1;
                    }
                    InitBtnTrack(&btnt, hwndPlanet, NULL, &rc, bt, 0, FALSE, FALSE, NULL);
                    while (FTrackBtn(&btnt)) {
                    }
                    if (btnt.fDown) {
                        prgtile[i].fPopped = prgtile[i].fPopped == 0;
                        ReflowColumn(prgtile[i].iCol, i, TRUE);
                    }
                } else {
                    if (prgtile[i].fPopped) {
                        rcTitle.bottom = rcTitle.top + prgtile[i].dyFull + 1;
                    }
                    hdc = GetDC(hwndPlanet);
                    DrawFuzzyBorder(hdc, &rcTitle);
                    SetCapture(hwndPlanet);
                    ptNew = pt;
                    while (FGetMouseMove(&ptNew)) {
                        if (pt.x != ptNew.x || pt.y != ptNew.y) {
                            DrawFuzzyBorder(hdc, &rcTitle);
                            OffsetRc(&rcTitle, ptNew.x - pt.x, ptNew.y - pt.y);
                            pt = ptNew;
                            DrawFuzzyBorder(hdc, &rcTitle);
                        }
                    }
                    DrawFuzzyBorder(hdc, &rcTitle);
                    ReleaseCapture();
                    ReleaseDC(hwndPlanet, hdc);
                    pt.x = ((rcTitle.right - rcTitle.left) >> 1) + rcTitle.left;
                    pt.y = ((rcTitle.bottom - rcTitle.top) >> 1) + rcTitle.top;
                    iCol = 0 > (1 >= pt.x / 198 ? pt.x / 198 : 1) ? 0 : 1 < pt.x / 198 ? 1 : pt.x / 198;
                    iCur = i;
                    for (i = 0; i < ctile && prgtile[i].iCol < iCol; i++) {
                    }
                    for (; i < ctile && prgtile[i].iCol == iCol; i++) {
                        dy = !prgtile[i].fPopped ? dyArial8 + 3 : prgtile[i].dyFull;
                        if (pt.y < (dy >> 1) + prgtile[i].yTop)
                            break;
                    }
                    if (i == iCur || i == iCur + 1) {
                        i = prgtile[iCur].iCol;
                        if (iCol != i) {
                            prgtile[iCur].iCol = iCol;
                            ReflowColumn(iCol, iCur, TRUE);
                            if ((int16_t)iCol < i) {
                                ReflowColumn(i, iCur + 1, TRUE);
                            } else {
                                ReflowColumn(i, iCur, TRUE);
                            }
                        }
                    } else {
                        tile = prgtile[iCur];
                        if (i < iCur) {
                            memmove(prgtile + (i + 1), prgtile + i, (iCur - i) * sizeof(TILE));
                            iCur++;
                        } else {
                            memmove(prgtile + iCur, prgtile + (iCur + 1), (i - iCur - 1) * sizeof(TILE));
                            i--;
                        }
                        prgtile[i] = tile;
                        prgtile[i].iCol = iCol;
                        if (tile.iCol == iCol) {
                            ReflowColumn(iCol, i >= iCur ? iCur : i, TRUE);
                        } else {
                            ReflowColumn(iCol, i, TRUE);
                            ReflowColumn(tile.iCol, iCur, TRUE);
                        }
                    }
                }
            } else {
                if ((sel.grobj == grobjFleet && (prgtile[i].grbit == tileFleetOrders || prgtile[i].grbit == tileStarbaseOrWaypoint ||
                                                 prgtile[i].grbit == tileMineralsOrCargo || prgtile[i].grbit == 0x10)) ||
                    prgtile[i].grbit == tileShipList) {
                    ClickInShipOrders(pt, sks, FALSE, fRightBtn);
                } else if (sel.grobj == grobjPlanet && (prgtile[i].grbit == tileMineralsOrCargo || prgtile[i].grbit == tileStarbaseOrWaypoint ||
                                                        prgtile[i].grbit == tileProductionOrOrbit || prgtile[i].grbit == tilePlanetStats)) {
                    ClickInPlanetOrders(pt, sks, FALSE, fRightBtn);
                }
            }
        }
    }
    return;
}

HCURSOR ClickInPlanetOrders(POINT16 pt, int16_t sks, int16_t fCursor, int16_t fRightBtn) {
    int16_t i;
    int32_t rglQuan[3];
    int16_t iWarp;
    BTNT    btnt;

    if (sel.grobj != grobjPlanet) {
        return NULL;
    }
    if (fRightBtn) {
        return NULL;
    }
    if (PtInRect(&rgrcRef[6], PointFrom16(pt)) != 0) {
        if (fCursor) {
            return hcurArrowHelp;
        }
        i = (int16_t)(pt.y - rgrcRef[6].top) / dyArial8;
        GlobalPD.grPopup = grPopupMineral;
        GlobalPD.rgi[0] = i;
        GlobalPD.rgi[2] = sel.pl.rgwtMin[i];
        GlobalPD.rgi[3] = (uint32_t)sel.pl.rgMinConc[i];
        EstMineralsMined(&sel.pl, rglQuan, -1, FALSE);
        GlobalPD.rgi[4] = rglQuan[i];
        GlobalPD.rgi[1] = sel.pl.fHomeworld;
        Popup(hwndPlanet, pt.x, pt.y);
    } else if (PtInRect(&rgrcRef[7], PointFrom16(pt)) != 0) {
        if (fCursor) {
            return hcurArrowHelp;
        }
        GlobalPD.grPopup = grPopupPlanetIndustry;
        GlobalPD.idPlan = sel.pl.id;
        GlobalPD.fFactory = pt.y >= rgrcRef[7].top + dyArial8;
        if (GlobalPD.fFactory) {
            GlobalPD.cMax = CMaxFactories(&sel.pl, idPlayer);
            GlobalPD.cCur = sel.pl.cFactories;
            GlobalPD.cOperate = CMaxOperableFactories(&sel.pl, idPlayer, FALSE);
        } else {
            GlobalPD.cMax = CMaxMines(&sel.pl, idPlayer);
            GlobalPD.cCur = sel.pl.cMines;
            GlobalPD.cOperate = CMaxOperableMines(&sel.pl, idPlayer, FALSE);
        }
        Popup(hwndPlanet, pt.x, pt.y);
    } else if (PtInRect(&rgrcRef[8], PointFrom16(pt)) != 0) {
        if (fCursor) {
            return hcurArrowHelp;
        }
        GlobalPD.grPopup = grPopupResources;
        GlobalPD.idPlanet = sel.pl.id;
        GlobalPD.iPlanetVar = GlobalPD.iPlanVal = CResourcesAtPlanet(&sel.pl, idPlayer);
        if (!sel.pl.fNoResearch) {
            GlobalPD.iPlanVal -= MulDiv(GlobalPD.iPlanetVar, rgplr[idPlayer].pctResearch, 100);
        }
        Popup(hwndPlanet, pt.x, pt.y);
    } else if (PtInRect(&rgrcRef[9], PointFrom16(pt)) != 0) {
        if (fCursor) {
            return hcurArrowHelp;
        }
        GlobalPD.grPopup = grPopupPlanet;
        GlobalPD.idPlanet = sel.pl.id;
        Popup(hwndPlanet, pt.x, pt.y);
    } else if (PtInRect(&rgrcRef[10], PointFrom16(pt)) != 0) {
        if (sel.pl.cDefenses == 0) {
            return NULL;
        }
        if (fCursor) {
            return hcurArrowHelp;
        }
        FGetBestDefensePart(&GlobalPD.part);
        GlobalPD.grPopup = grPopupComponent;
        Popup(hwndPlanet, pt.x, pt.y);
    } else if (PtInRect(&rgrcRef[11], PointFrom16(pt)) != 0) {
        if (sel.pl.iScanner == 31 && GetRaceStat(&rgplr[idPlayer], rsMajorAdv) != raMacintosh) {
            return NULL;
        }
        if (fCursor) {
            return hcurArrowHelp;
        }
        if (GetRaceStat(&rgplr[idPlayer], rsMajorAdv) == raMacintosh) {
            GlobalPD.grPopup = grPopupString;
            GlobalPD.dxOut = 180;
            GlobalPD.psz = szPopupBuffer;
            CchGetString(idsRaceCannotBuildPlanetaryScannersStarbasesHave, szPopupBuffer);
        } else {
            LookupBestPlanetaryScanner(&GlobalPD.part);
            GlobalPD.grPopup = grPopupComponent;
        }
        Popup(hwndPlanet, pt.x, pt.y);
    } else if (PtInRect(&rgrcRef[13], PointFrom16(pt)) != 0) {
        iWarp = IWarpMAFromLppl(&sel.pl, NULL);
        if (iWarp == 0) {
            return NULL;
        }
        if (fCursor) {
            return hcurHand;
        }
        InitBtnTrack(&btnt, hwndPlanet, NULL, &rgrcRef[13], 8, 80, gd.fSetMassMode, TRUE, PszGetCompressedString(idsSetDest));
        while (FTrackBtn(&btnt)) {
        }
        gd.fSetMassMode ^= btnt.fDown;
    } else if (PtInRect(&rgrcRef[14], PointFrom16(pt)) != 0) {
        if (fCursor) {
            return hcurArrowHelp;
        }
        GlobalPD.grPopup = grPopupShdef;
        GlobalPD.lpshdef = rglpshdefSB[idPlayer] + sel.pl.isb;
        GlobalPD.fHideCounts = FALSE;
        GlobalPD.fShowDamage = TRUE;
        GlobalPD.fToken = FALSE;
        GlobalPD.fSummary = FALSE;
        Popup(hwndPlanet, pt.x, pt.y);
    } else if (PtInRect(&rgrcRef[16], PointFrom16(pt)) != 0) {
        iWarp = IWarpMAFromLppl(&sel.pl, NULL);
        if (iWarp == 0) {
            return NULL;
        }
        if (fCursor) {
            return hcurArrowHelp;
        }
        GlobalPD.part.hs.grhst = hstSpecialSB;
        GlobalPD.part.hs.iItem = iWarp + 2;
        FLookupPart(&GlobalPD.part);
        GlobalPD.grPopup = grPopupComponent;
        Popup(hwndPlanet, pt.x, pt.y);
    } else if (PtInRect(&rgrcRef[15], PointFrom16(pt)) != 0) {
        ClickInShipOrders(pt, sks, FALSE, fRightBtn);
    } else if (PtInRect(&rgrcRef[17], PointFrom16(pt)) != 0) {
        if (fCursor) {
            return hcurHand;
        }
        InitBtnTrack(&btnt, hwndPlanet, NULL, &rgrcRef[17], 8, 80, gd.fSetRouteMode, TRUE, PszGetCompressedString(idsRoute2));
        while (FTrackBtn(&btnt)) {
        }
        gd.fSetRouteMode ^= btnt.fDown;
    }
    return NULL;
}

void EnsureTileSize(int16_t fSmallTiles) {
    int16_t    iMul;
    int16_t    i;
    GrobjClass grobjSav;

    if (fSmallTiles != gd.fSmallTileMode) {
        gd.fSmallTileMode = fSmallTiles;
        iMul = !fSmallTiles ? 1 : -1;
        for (i = 0; i < 6; i++) {
            if (rgtilePlanet[i].grbit == tileProductionOrOrbit) {
                rgtilePlanet[i].dyFull += (dyArial8 + 2) * 2 * iMul;
            }
            if (rgtilePlanet[i].grbit == tileShipList) {
                rgtilePlanet[i].dyFull += (dyArial8 + 4) * 2 * iMul;
            }
            if (rgtilePlanet[i].grbit == tileBitmap) {
                rgtilePlanet[i].dyFull += 10 * iMul;
            }
        }
        for (i = 0; i < 7; i++) {
            if (rgtileShip[i].grbit == tileMineralsOrCargo) {
                rgtileShip[i].dyFull += (dyArial8 * 4 + 2) * iMul;
            }
            if (rgtileShip[i].grbit == tileFleetComp) {
                rgtileShip[i].dyFull += ((dyArial8 + 2) * 2 + 4 + dyArial8) * iMul;
            }
            if (rgtileShip[i].grbit == tileFleetOrders) {
                rgtileShip[i].dyFull += (dyArial8 + 9) * iMul;
            }
            if (rgtileShip[i].grbit == tileShipList) {
                rgtileShip[i].dyFull += (dyArial8 + 4) * 2 * iMul;
            }
            if (rgtileShip[i].grbit == tileBitmap) {
                rgtileShip[i].dyFull += 10 * iMul;
            }
            if (rgtileShip[i].grbit == tileStarbaseOrWaypoint) {
                rgtileShip[i].dyFull += iMul * 2;
            }
            if (rgtileShip[i].grbit == tileProductionOrOrbit) {
                rgtileShip[i].dyFull += 6 * iMul;
            }
        }
        grobjSav = sel.grobj;
        sel.grobj = grobjPlanet;
        for (i = 0; i < 4; i++) {
            ReflowColumn(i, -1, FALSE);
        }
        sel.grobj = grobjFleet;
        for (i = 0; i < 4; i++) {
            ReflowColumn(i, -1, FALSE);
        }
        sel.grobj = grobjSav;
    }
    return;
}

void ReflowColumn(int16_t iCol, int16_t iTile, int16_t fRedraw) {
    HDC     hdc;
    int16_t yTop;
    int16_t ctile;
    int16_t i;
    int16_t grbit;
    TILE   *ptile;
    RECT    rc;

    grbit = 0;
    if (sel.grobj == grobjFleet) {
        ptile = rgtileShip;
        ctile = 7;
    } else {
        ptile = rgtilePlanet;
        ctile = 6;
    }
    yTop = 4;
    if (iTile == -1) {
        for (i = 0; i < ctile && ptile[i].iCol != iCol; i++) {
        }
        if (i == ctile) {
            return;
        }
        iTile = i;
    } else {
        for (i = 0; i < iTile; i++) {
            if (ptile[i].iCol == iCol) {
                yTop += (!ptile[i].fPopped ? dyArial8 + 3 : ptile[i].dyFull) + 4;
            }
        }
    }
    if (fRedraw) {
        GetClientRect(hwndPlanet, &rc);
        rc.top = yTop;
        rc.left = 198 * iCol + 4;
        rc.right = rc.left + 191;
        hdc = GetDC(hwndPlanet);
        FillRect(hdc, &rc, hbrButtonFace);
    }
    for (; iTile < ctile && ptile[iTile].iCol == iCol; iTile++) {
        ptile[iTile].yTop = yTop;
        ptile[iTile].fFixCtls = TRUE;
        grbit |= ptile[iTile].grbit;
        yTop += (!ptile[iTile].fPopped ? dyArial8 + 3 : ptile[iTile].dyFull) + 4;
    }
    if (fRedraw) {
        DrawPlanShip(hdc, grbit);
        ReleaseDC(hwndPlanet, hdc);
    }
    return;
}

void HandleFocusState(DRAWITEMSTRUCT *lpdis, int16_t inflate) {
    if (lpdis->itemState & ODS_FOCUS) {
        FrameRect(lpdis->hDC, &lpdis->rcItem, hbr50Screen);
    }
    return;
}

void DrawCBEntireItem(DRAWITEMSTRUCT *lpdis, int16_t inflate) {
    int16_t fListbox;
    int16_t fSelected;
    RECT    rc;

    fSelected = lpdis->itemState & ODS_SELECTED;
    rc = lpdis->rcItem;
    fListbox = lpdis->hwndItem == hwndFleetCompLB || lpdis->hwndItem == hwndPlanetProdLB || inflate > 0;
    if (inflate > 0) {
        inflate = -inflate;
    }
    SendMessage(lpdis->hwndItem, !fListbox ? CB_GETLBTEXT : LB_GETTEXT, lpdis->itemID, (LPARAM)szWork);
    DrawProductionItem(lpdis->hDC, &rc, szWork, inflate, fSelected, fListbox);
    HandleFocusState(lpdis, inflate + 2);
    return;
}

void DrawProductionItem(HDC hdc, RECT *prc, char *psz, int16_t inflate, int16_t fSelected, int16_t fListbox) {
    HFONT    hfntSav;
    char    *pch;
    int16_t  ichT;
    COLORREF cr;
    int16_t  pctDmg;
    char     szT[20];
    RECT     rcIn;
    int16_t  ich;
    int16_t  fDoubleDraw;
    COLORREF crForeSav;
    int16_t  fFleet;
    RECT     rcDraw;
    int16_t  dx;
    HBRUSH   hbr;
    int16_t  fItalic;
    int16_t  cch;
    int16_t  bkSav;
    RECT     rc;

    fItalic = FALSE;
    fDoubleDraw = FALSE;
    fFleet = FALSE;
    rcIn = *prc;
    rc = rcIn;
    if (!fSelected) {
        hbr = hbrWindow;
        switch (*psz) {
        default:
            cr = 0xff;
            break;
        case 'I':
            fItalic = TRUE;
            /* fallthrough */
        case ' ':
        LDefCase:
            if (crWindow == 0) {
                cr = 0xffffff;
                break;
            }
            cr = 0;
            break;
        case 'P':
            fDoubleDraw = TRUE;
            pctDmg = psz[1];
            /* fallthrough */
        case 'Q':
            fFleet = TRUE;
            goto LDefCase;
        case '*':
            cr = 32512;
            break;
        case '#':
            cr = 8323072;
            break;
        case '&':
            cr = 8355711;
        }
    } else {
        cr = crWindow;
        switch (*psz) {
        default:
            hbr = hbrRed;
            break;
        case 'I':
            fItalic = TRUE;
            /* fallthrough */
        case ' ':
        LDefCaseSel:
            if (crWindow == 0) {
                hbr = GetStockObject(WHITE_BRUSH);
                break;
            }
            hbr = GetStockObject(BLACK_BRUSH);
            break;
        case 'P':
            fDoubleDraw = TRUE;
            pctDmg = psz[1];
            /* fallthrough */
        case 'Q':
            fFleet = TRUE;
            goto LDefCaseSel;
        case '*':
            hbr = hbrGreen;
            break;
        case '#':
            hbr = hbrBlue;
            break;
        case '&':
            hbr = hbrGray;
        }
    }
    if (fListbox == 2) {
        hbr = hbrButtonFace;
    }
    FillRect(hdc, &rcIn, hbr);
    if (fDoubleDraw) {
        rcDraw = rcIn;
        dx = rcIn.right - rcIn.left;
        dx = (int16_t)(dx * pctDmg) / 100;
        rcDraw.right = rcDraw.left + dx;
        FillRect(hdc, &rcDraw, hbrRed);
    }
    if (inflate != 0) {
        InflateRect(&rcIn, -2, -1);
    }
    if (!fListbox) {
        ich = 1;
    } else if (fFleet) {
        ich = 7;
    } else {
        ich = 6;
        if ((psz[1] - ' ') & 2) {
            fItalic = TRUE;
        }
    }
    crForeSav = SetTextColor(hdc, cr);
    bkSav = SetBkMode(hdc, TRANSPARENT);
    if (fItalic) {
        hfntSav = SelectObject(hdc, rghfontArial8[3]);
    }
    pch = psz + ich;
    cch = strlen(pch) + 1;
    do {
        cch--;
        dx = LOWORD(GetTextExtent(hdc, pch, cch));
    } while (dx > rcIn.right - rcIn.left);
    TextOut(hdc, rcIn.left, rcIn.top, pch, cch);
    if (fItalic) {
        SelectObject(hdc, hfntSav);
    }
    if (ich >= 6) {
        if ((psz[ich - 5] - ' ') & 2) {
            if (psz[ich - 1] == '*') {
                ich = CchGetString(idsNeeded, szT);
                goto LRightOut;
            }
            CchGetString(idsUpTo, szT);
        } else {
            szT[0] = 0;
        }
        ich = strlen(szT);
        for (ichT = 2 - fFleet; ichT < 6 && psz[ichT + fFleet] == ' '; ichT++) {
        }
        strncpy(&szT[ich], psz + (ichT + fFleet), 6 - ichT);
        ich += 6 - ichT;
        if (!fFleet && ((psz[fDoubleDraw + 1] - ' ') & 1)) {
            szT[ich++] = '%';
        }
    LRightOut:
        RightTextOut(hdc, rcIn.right, rcIn.top, szT, ich, 0);
    }
    SetTextColor(hdc, crForeSav);
    SetBkMode(hdc, bkSav);
    return;
}

void DrawPlanShip(HDC hdc, TileBits grbit) {
    HFONT    hfontSav;
    OBJ      objNull;
    int16_t  ctile;
    COLORREF crFore;
    OBJ      obj;
    int16_t  fMin;
    int16_t  i;
    COLORREF crBack;
    int16_t  fErase;
    TILE    *ptile;
    int16_t  fDC;
    RECT     rc;

    fDC = FALSE;
    objNull.pfl = 0;
    if (sel.id == -1) {
        for (i = 0; i < 13; i++) {
            ShowWindow(rghwndBtn[i], SW_HIDE);
        }
        for (i = 0; i < 3; i++) {
            ShowWindow(rghwndOrderDD[i], SW_HIDE);
        }
        ShowWindow(hwndOrderED, SW_HIDE);
        ShowWindow(hwndShipDD, SW_HIDE);
        ShowWindow(hwndBattleDD, SW_HIDE);
        ShowWindow(hwndShipLB, SW_HIDE);
        ShowWindow(hwndFleetCompLB, SW_HIDE);
        ShowWindow(hwndPlanetProdLB, SW_HIDE);
        ShowWindow(hwndRepCB, SW_HIDE);
        for (i = 0; i < 19; i++) {
            rgrcRef[i].bottom = -6;
            rgrcRef[i].top = -5;
        }
        if (rgplr[idPlayer].fDead && hdc) {
            GetClientRect(hwndPlanet, &rc);
            SetBkColor(hdc, crButtonFace);
            SetTextColor(hdc, crButtonText);
            i = CchGetString(idsDeceased, szWork);
            DiaganolTextOut(hdc, &rc, szWork, i);
        }
    } else {
        if (sel.grobj == grobjFleet) {
            ptile = rgtileShip;
            ctile = 7;
            obj.pfl = &sel.fl;
        } else {
            ptile = rgtilePlanet;
            ctile = 6;
            obj.ppl = &sel.pl;
        }
        if (!hdc) {
            fDC = TRUE;
            hdc = GetDC(hwndPlanet);
        }
        hfontSav = SelectObject(hdc, rghfontArial8[0]);
        crBack = SetBkColor(hdc, crButtonFace);
        crFore = SetTextColor(hdc, crButtonText);
        fErase = (grbit & tileErase) != 0;
        fMin = (grbit & tileMinimized) != 0;
        for (i = 0; i < ctile; i++) {
            if (grbit & ptile[i].grbit) {
                ptile[i].fErase = fErase;
                ptile[i].fMinDraw = fMin;
                ptile[i].pfn(hdc, ptile + i, !ptile[i].fNullPtr ? obj : objNull);
            }
        }
        SetTextColor(hdc, crFore);
        SetBkColor(hdc, crBack);
        SelectObject(hdc, hfontSav);
        if (fDC) {
            ReleaseDC(hwndPlanet, hdc);
        }
    }
    return;
}

void SetPlanetTitleBar(HWND hwnd) {
    char  szTitle[30];
    char *psz;

    if (sel.grobj == grobjPlanet) {
        psz = PszGetPlanetName(sel.pl.id);
        CchGetString(idsPlanet2, szTitle);
        lstrcat(szTitle, psz);
        psz = szTitle;
    } else if (sel.grobj == grobjFleet) {
        psz = PszGetFleetName(sel.fl.id);
    } else {
        psz = PszGetCompressedString(idsPlanetView);
    }
    SetWindowText(hwnd, psz);
    return;
}

void FillShipDD(int16_t idSkip) {
    THING  *lpthMac;
    int16_t i;
    THING  *lpth;
    FLEET  *lpfl;
    POINT16 ptSel;

    SendMessage(hwndShipDD, CB_RESETCONTENT, 0, 0);
    if (sel.grobj == grobjPlanet) {
        ptSel = rgptPlan[sel.id];
    } else {
        ptSel = sel.fl.pt;
    }
    for (i = 0; i < cFleet; i++) {
        lpfl = rglpfl[i];
        if (!rglpfl[i])
            break;
        if ((idSkip == idflNone && sel.id == lpfl->idPlanet) ||
            (idSkip != idflNone && lpfl->id != idSkip && lpfl->pt.x == sel.fl.pt.x && lpfl->pt.y == sel.fl.pt.y)) {
            PszGetFleetName(lpfl->id);
            memmove(&szWork[1], szWork, 50);
            szWork[0] = lpfl->iPlayer == idPlayer ? 32 : 120;
            SendMessage(hwndShipDD, CB_ADDSTRING, 0, (LPARAM)szWork);
        }
    }
    lpth = lpThings;
    lpthMac = lpThings + cThing;
    for (; lpth < lpthMac; lpth++) {
        if (lpth->ith == ithMineralPacket && lpth->pt.x == ptSel.x && lpth->pt.y == ptSel.y) {
            PszGetThingName(lpth->idFull);
            memmove(&szWork[1], szWork, 50);
            szWork[0] = lpth->iplr == idPlayer ? 32 : 120;
            SendMessage(hwndShipDD, CB_ADDSTRING, 0, (LPARAM)szWork);
        }
    }
    SendMessage(hwndShipDD, CB_SETCURSEL, 0, 0);
    return;
}

void FillPlanetProdLB(HWND hwnd, PLPROD *lpplprod, PLANET *lppl) {
    int32_t rgwtMin[4];
    int16_t i;
    char    szTemp[80];
    int32_t resCost;
    char   *psz;

    /* Given a planet, leave its queue's first item in szWork; see
       PszProdQueueTop. */
    if (lppl) {
        PszProdQueueTop(lppl, lpplprod);
        return;
    }
    lppl = &sel.pl;
    if (!hwnd) {
        hwnd = hwndPlanetProdLB;
    }
    SendMessage(hwnd, LB_RESETCONTENT, 0, 0);
    if (!lpplprod) {
        lpplprod = lppl->lpplprod;
    }
    if (!lpplprod || lpplprod->iprodMac == 0) {
        psz = PszGetCompressedString(idsQueueEmpty);
    } else {
        if (!hwndProdDlg)
            goto NoMsg;
        psz = PszGetCompressedString(idsTopQueue);
    }
    SendMessage(hwnd, LB_ADDSTRING, 0, (LPARAM)psz);
NoMsg:
    if (lpplprod) {
        resCost = 0;
        for (i = 0; i < 4; i++) {
            rgwtMin[i] = 0;
        }
        for (i = 0; i < lpplprod->iprodMac; i++) {
            FProdItemLine(lppl, lpplprod, i, TRUE, szTemp);
            SendMessage(hwnd, LB_ADDSTRING, 0, (LPARAM)szTemp);
        }
    }
    return;
}

// ShowPlanetSel refills the planet tile's production queue when
// ChangeMainObjSel selects a planet.
void ShowPlanetSel() {
    FillPlanetProdLB(NULL, NULL, NULL);
    SendMessage(hwndPlanetProdLB, LB_SETCURSEL, 0, 0);
    return;
}

// ShowFleetSel refills the fleet tiles' lists when ChangeMainObjSel selects
// a fleet.
void ShowFleetSel() {
    FillOrdersLB();
    FillFleetCompLB();
    FillBattleDD(sel.fl.iplan + 1);
    SendMessage(rghwndOrderDD[0], CB_SETCURSEL, sel.fl.lpplord->rgord[0].grTask, 0);
    return;
}

// ShowMainObjSel redraws the planet window for the object ChangeMainObjSel
// selected; fSameType is set when it is the same kind as before.
void ShowMainObjSel(int16_t fSameType, int16_t idSkip) {
    int16_t i;

    if (!fSameType) {
        for (i = 0; i < 13; i++) {
            ShowWindow(rghwndBtn[i], SW_HIDE);
        }
        for (i = 0; i < 3; i++) {
            ShowWindow(rghwndOrderDD[i], SW_HIDE);
        }
        ShowWindow(hwndOrderED, SW_HIDE);
        ShowWindow(hwndShipDD, SW_HIDE);
        ShowWindow(hwndBattleDD, SW_HIDE);
        ShowWindow(hwndShipLB, SW_HIDE);
        ShowWindow(hwndFleetCompLB, SW_HIDE);
        ShowWindow(hwndPlanetProdLB, SW_HIDE);
        ShowWindow(hwndRepCB, SW_HIDE);
        for (i = 0; i < 19; i++) {
            rgrcRef[i].bottom = -6;
            rgrcRef[i].top = -5;
        }
    }
    FillShipDD(idSkip);
    if (fSameType) {
        DrawPlanShip(NULL, 0x4fff);
    } else {
        InvalidateRect(hwndPlanet, NULL, TRUE);
        if ((grbitScan & grbitScanAddWaypoints) && sel.grobj == grobjPlanet) {
            grbitScan &= 0xffef;
            InvalidateRect(hwndTb, NULL, TRUE);
        }
    }
    SetPlanetTitleBar(hwndPlanet);
    if (gd.fTutorial) {
        AdvanceTutor();
    }
    return;
}

// FillSelProdLB refills the planet tile's production queue for the
// selected planet.
void FillSelProdLB() {
    FillPlanetProdLB(NULL, NULL, NULL);
    return;
}

// RedrawPlanShip redraws the planet window's tiles in grbit.
void RedrawPlanShip(TileBits grbit) {
    DrawPlanShip(NULL, grbit);
    return;
}
