#include "common.h"

int16_t ChangeProduction(int16_t fClear) {
    jmp_buf  env;
    jmp_buf *penvMemSav;
    FARPROC  lpProcProd;
    PROD     rgprod[64];
    int16_t  fSuccess;

    penvMemSav = penvMem;
    penvMem = &env;
    if (setjmp(env) != 0) {
        if (lpplProdGlob) {
            FreePl((PL *)lpplProdGlob);
        }
        lpplProdGlob = NULL;
        if (hwndProdDlg) {
            EndDialog(hwndProdDlg, 0);
        }
        hwndProdDlg = 0;
        fDlgUp = FALSE;
        AlertSz(PszFormatIds(idsThereIsntEnoughFreeMemoryModifyProduction, NULL), MB_ICONHAND);
        penvMem = penvMemSav;
        return FALSE;
    }
    if (fClear) {
        fSuccess = 1;
        goto LWriteProdQ;
    }
    InitProduction(rgprod);
    fDlgUp = TRUE;
    lpProcProd = MakeProcInstance(ProductionDlg, hInst);
    fSuccess = DialogBox(hInst, MAKEINTRESOURCE(IDD_PRODUCTION), hwndFrame, lpProcProd);
    FreeProcInstance(lpProcProd);
    hwndProdDlg = 0;
    fDlgUp = FALSE;
LWriteProdQ:
    FinishProduction(fSuccess);
    if (fSuccess && sel.grobj == grobjPlanet) {
        DrawPlanShip(NULL, tilePlanetStats);
    }
    penvMem = penvMemSav;
    return TRUE;
}

void InitProduction(PROD *rgprod) {
    int16_t  iWarp;
    int16_t  iSrc;
    uint16_t u;
    int16_t  i;
    int16_t  ipl;
    PART     part;
    PROD    *lpprod;

    gd.fNoResearchSav = sel.pl.fNoResearch;
    if (!rgprod) {
        rgprod = pProdGlob;
    }
    if (sel.pl.lpplprod) {
        i = sel.pl.lpplprod->iprodMac;
    } else {
        i = 2;
    }
    lpplProdGlob = (PLPROD *)LpplAlloc(4, i, htOrd);
    if (sel.pl.lpplprod) {
        fmemcpy(lpplProdGlob->rgprod, sel.pl.lpplprod->rgprod, i * 4);
    } else {
        i = 0;
    }
    lpplProdGlob->iprodMac = i;
    cProdGlob = 0;
    pProdGlob = rgprod;
    memset(rgprod, 0, 64 * sizeof(PROD));
    if (sel.pl.fStarbase && LphuldefFromId(rglpshdefSB[idPlayer][sel.pl.isb].hul.ihuldef)->hul.wtCargoMax != 0) {
        for (i = 0; i < 16; i++) {
            if (!rgshdef[i].fFree && !rgshdef[i].fGift) {
                if (LphuldefFromId(rglpshdefSB[idPlayer][sel.pl.isb].hul.ihuldef)->hul.wtCargoMax >= rgshdef[i].hul.wtEmpty) {
                    rgprod[cProdGlob].cItem = 0x3ff;
                    rgprod[cProdGlob].iItem = i;
                    rgprod[cProdGlob].grobj = grobjFleet;
                    cProdGlob++;
                }
            }
        }
    }
    for (i = 0; i < 10; i++) {
        if (!rglpshdefSB[idPlayer][i].fFree && !rglpshdefSB[idPlayer][i].fGift && (sel.pl.isb != i || !sel.pl.fStarbase)) {
            rgprod[cProdGlob].cItem = 1;
            rgprod[cProdGlob].iItem = LOWORD((int16_t)(i + 16));
            rgprod[cProdGlob].grobj = grobjFleet;
            cProdGlob++;
        }
    }
    part.hs.grhst = hstPlanetary;
    part.hs.iItem = iplanetaryGenesisDevice;
    if (FLookupPart(&part) == mdPartAvailAvailable) {
        rgprod[cProdGlob].cItem = 1;
        rgprod[cProdGlob].iItem = iobjGenesis;
        rgprod[cProdGlob].grobj = grobjPlanet;
        cProdGlob++;
    }
    iWarp = IWarpMAFromLppl(&sel.pl, NULL);
    if (iWarp > 0) {
        for (i = 0; i < 4; i++) {
            rgprod[cProdGlob].cItem = 0x3ff;
            rgprod[cProdGlob].iItem = LOWORD((int16_t)(i + 14));
            rgprod[cProdGlob].grobj = grobjPlanet;
            cProdGlob++;
        }
    }
    u = CMaxFactories(&sel.pl, idPlayer) - sel.pl.cFactories;
    if ((int16_t)u > 0) {
        rgprod[cProdGlob].cItem = LOWORD(1020 >= u ? (uint32_t)u : 1020);
        rgprod[cProdGlob].iItem = mdIdleFactory;
        rgprod[cProdGlob].grobj = grobjPlanet;
        cProdGlob++;
    }
    u = CMaxMines(&sel.pl, idPlayer) - sel.pl.cMines;
    if ((int16_t)u > 0) {
        rgprod[cProdGlob].cItem = LOWORD(1020 >= u ? (uint32_t)u : 1020);
        rgprod[cProdGlob].iItem = mdIdleMine;
        rgprod[cProdGlob].grobj = grobjPlanet;
        cProdGlob++;
    }
    u = CMaxDefenses(&sel.pl, idPlayer) - sel.pl.cDefenses;
    if ((int16_t)u > 0) {
        rgprod[cProdGlob].cItem = u;
        rgprod[cProdGlob].iItem = mdIdleDefense;
        rgprod[cProdGlob].grobj = grobjPlanet;
        cProdGlob++;
    }
    rgprod[cProdGlob].cItem = 0x3ff;
    rgprod[cProdGlob].iItem = mdIdleAlchemy;
    rgprod[cProdGlob].grobj = grobjPlanet;
    cProdGlob++;
    if (sel.pl.iScanner == 31 && GetRaceStat(&rgplr[idPlayer], rsMajorAdv) != raMacintosh) {
        rgprod[cProdGlob].cItem = 1;
        rgprod[cProdGlob].iItem = iobjPlanetaryScanner;
        rgprod[cProdGlob].grobj = grobjPlanet;
        cProdGlob++;
    }
    i = IpctCanTerraformLppl(&sel.pl);
    if (i > 0) {
        rgprod[cProdGlob].cItem = (uint16_t)i;
        rgprod[cProdGlob].iItem = mdIdleTerraform;
        rgprod[cProdGlob].grobj = grobjPlanet;
        cProdGlob++;
    }
    for (i = 0; i < 7; i++) {
        if (GetRaceStat(&rgplr[idPlayer], rsMajorAdv) == raMacintosh && (i == 0 || i == 1 || i == 2))
            continue;
        if (GetRaceStat(&rgplr[idPlayer], rsMajorAdv) != raTerra || (i != 4 && i != 5)) {
            rgprod[cProdGlob].cItem = 0x3ff;
            rgprod[cProdGlob].iItem = LOWORD(i);
            rgprod[cProdGlob].grobj = grobjPlanet;
            cProdGlob++;
        }
    }
    ipl = 0;
    lpprod = lpplProdGlob->rgprod;
    while (ipl < lpplProdGlob->iprodMac) {
        for (iSrc = 0; iSrc < cProdGlob && ((uint32_t)pProdGlob[iSrc].grobj != lpprod->grobj || (uint32_t)pProdGlob[iSrc].iItem != lpprod->iItem); iSrc++) {
        }
        if (iSrc >= cProdGlob) {
            if (ipl + 1 < lpplProdGlob->iprodMac) {
                fmemcpy(lpprod, lpprod + 1, (lpplProdGlob->iprodMac - (ipl + 1)) * sizeof(PROD));
                ipl--;
            }
            lpplProdGlob->iprodMac--;
        } else {
            if (pProdGlob[iSrc].cItem < lpprod->cItem) {
                lpprod->cItem = pProdGlob[iSrc].cItem;
            }
            if (pProdGlob[iSrc].cItem != 0x3ff) {
                if (pProdGlob[iSrc].cItem < lpprod->cItem) {
                    pProdGlob[iSrc].cItem = 0;
                } else {
                    pProdGlob[iSrc].cItem -= lpprod->cItem;
                }
            }
        }
        ipl++;
        lpprod++;
    }
    if (gd.fTutorial && idPlayer == 0) {
        AdvanceTutor();
    }
    return;
}

void FinishProduction(int16_t fWrite) {
    if (fWrite) {
        FreePl((PL *)sel.pl.lpplprod);
        if (lpplProdGlob && lpplProdGlob->iprodMac == 0) {
            FreePl((PL *)lpplProdGlob);
            lpplProdGlob = NULL;
        }
        sel.pl.lpplprod = lpplProdGlob;
        lpplProdGlob = NULL;
        FLookupPlanet(idWriteBack, &sel.pl);
        FLookupPlanet(sel.pl.id, &sel.pl);
        if (!fAi) {
            FillPlanetProdLB(NULL, NULL, NULL);
            DrawPlanShip(NULL, tileProductionOrOrbit);
        }
    } else {
        sel.pl.fNoResearch = gd.fNoResearchSav;
        FreePl((PL *)lpplProdGlob);
    }
    lpplProdGlob = NULL;
    if (gd.fTutorial && idPlayer == 0) {
        tutor.fProgress = TRUE;
        AdvanceTutor();
    }
    return;
}

INT_PTR CALLBACK ProductionDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    HDC                hdc;
    PAINTSTRUCT        ps;
    RECT               rc;
    int16_t            dxPBtn;
    int16_t            dy;
    RECT               rcT;
    int16_t            i;
    int16_t            xCtr;
    int16_t            dx;
    int16_t            dyLB;
    int16_t            rgidProdBtns[10];
    DRAWITEMSTRUCT    *lpdis;
    MEASUREITEMSTRUCT *lpmis;
    POINT16            pt;
    int16_t            cMax;
    char               sz255[2];
    char              *rgszZip[6];
    ZIPPRODQ           rgzp[4];
    FARPROC            lpProc;
    int16_t            fRet;
    HCURSOR            hcs;

    switch (IS_WM_CTLCOLOR(message) ? WM_CTLCOLOR : message) { /* NATIVE: Win32 split WM_CTLCOLOR by control type. */
    case WM_INITDIALOG:
        rgidProdBtns[0] = 1070;
        rgidProdBtns[1] = 1071;
        rgidProdBtns[2] = 1;
        rgidProdBtns[3] = 2;
        rgidProdBtns[4] = 1048;
        rgidProdBtns[5] = 1049;
        rgidProdBtns[6] = 1081;
        rgidProdBtns[7] = 1082;
        rgidProdBtns[8] = 1069;
        rgidProdBtns[9] = 118;
        hwndProdDlg = hwnd;
        if (rgplr[idPlayer].cPlanet <= 1) {
            EnableWindow(GetDlgItem(hwnd, IDC_NEXT), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_BACK), FALSE);
        }
        if (gd.mdScreenSize >= 1) {
            dx = 760;
            dy = 580;
        } else {
            dx = 610;
            dy = 24 * dyArial8 + 24;
        }
        SetWindowPos(hwnd, NULL, 0, 0, dx, dy, SWP_NOMOVE | SWP_NOZORDER);
        GetClientRect(hwnd, &rc);
        xCtr = rc.right >> 1;
        dyLB = rc.bottom - (int16_t)(17 * dyArial8) / 2 - 24;
        rc.left = (int16_t)(11 * rc.right) / 20 + 16;
        dxPBtn = (int16_t)(rc.right - rc.left) / 4;
        rc.bottom -= (int16_t)(3 * dyArial8) / 2 + 6;
        SetWindowPos(GetDlgItem(hwnd, IDC_PRODUCTION_RESEARCH_LEFTOVERS_ONLY), NULL, 6, rc.bottom, rc.left - 12, (int16_t)(3 * dyArial8) / 2, SWP_NOZORDER);
        for (i = 0; i < 4; i++) {
            SetWindowPos(GetDlgItem(hwnd, rgidProdBtns[i]), NULL, rc.left, rc.bottom, dxPBtn - 6, (int16_t)(3 * dyArial8) / 2, SWP_NOZORDER);
            rc.left += dxPBtn;
        }
        dxPBtn += 24;
        rc.left = xCtr - ((dxPBtn - 6) >> 1);
        dy = (int16_t)(dyLB - 9 * dyArial8) / 5 + (int16_t)(3 * dyArial8) / 2 - 3;
        rc.top = 8;
        if (dy > 50) {
            rc.top += (int16_t)((dy - 50) * 5) / 2;
            dy = 50;
        }
        for (i = 4; i < 10; i++) {
            SetWindowPos(GetDlgItem(hwnd, rgidProdBtns[i]), NULL, rc.left, rc.top, dxPBtn - 6, (int16_t)(3 * dyArial8) / 2, SWP_NOZORDER);
            rc.top += dy;
        }
        SetWindowPos(GetDlgItem(hwnd, IDC_PRODUCTION_AVAILABLE_ITEMS), NULL, 6, 6, rc.left - 12, dyLB, SWP_NOZORDER);
        GetWindowRect(GetDlgItem(hwnd, IDC_PRODUCTION_AVAILABLE_ITEMS), &rcT);
        SetWindowPos(GetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), NULL, rc.left + dxPBtn, 6, rc.left - 12, rcT.bottom - rcT.top, SWP_NOZORDER);
        ScreenToClient(hwnd, (POINT *)&rcT.right);
        yTopFutureTech = rcT.bottom;
        InitializeProductionDlg(hwnd);
        if (gd.mdScreenSize == 1 && ptStickyProduceDlg.y == -1) {
            ptStickyProduceDlg.y = 0;
        }
        StickyDlgPos(hwnd, &ptStickyProduceDlg, TRUE);
        return 1;
    case WM_DRAWITEM:
        lpdis = (DRAWITEMSTRUCT *)lParam;
        if (lpdis->itemID == -1) {
            HandleFocusState(lpdis, -2);
        } else {
            switch (lpdis->itemAction) {
            case ODA_DRAWENTIRE:
            case ODA_SELECT:
            case ODA_FOCUS:
                DrawCBEntireItem(lpdis, 4);
            }
        }
        return 1;
    case WM_MEASUREITEM:
        lpmis = (MEASUREITEMSTRUCT *)lParam;
        lpmis->itemHeight = dyArial8 + 2;
        return 1;
    case WM_CTLCOLOR:
        if (GET_WM_CTLCOLOR_HWND(wParam, lParam) == GetDlgItem(hwnd, IDC_PRODUCTION_RESEARCH_LEFTOVERS_ONLY) ||
            message == WM_CTLCOLORSTATIC /* NATIVE: Win16 CTLCOLOR_STATIC was in HIWORD(lParam). */) {
            SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case WM_ERASEBKGND:
        GetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        if (PtInRect(&rcProdDiamond, PointFrom16(pt)) == 0)
            break;
        if (message == WM_LBUTTONDOWN) {
            GlobalPD.grPopup = grPopupString;
            GlobalPD.dxOut = 180;
            GlobalPD.psz = szPopupBuffer;
            CchGetString(idsRightClickBlueDiamondApplyProductionTemplate, szPopupBuffer);
            Popup(hwnd, pt.x, pt.y);
            break;
        }
        sz255[0] = -1;
        sz255[1] = 0;
        cMax = 0;
        for (i = 0; i < 4; i++) {
            if (vrgZipProd[i].fValid) {
                rgszZip[cMax++] = vrgZipProd[i].szName;
            }
        }
        rgszZip[cMax++] = sz255;
        rgszZip[cMax++] = PszGetCompressedString(idsCustomize);
        i = PopupMenu(hwnd, pt.x, pt.y, cMax, NULL, rgszZip, -1, TRUE);
        if (i == cMax - 1) {
            memcpy(rgzp, vrgZipProd, 160);
            lpProc = MakeProcInstance(ZipProdDlg, hInst);
            fRet = DialogBox(hInst, MAKEINTRESOURCE(IDD_ZIP_PROD), hwnd, lpProc);
            FreeProcInstance(lpProc);
            if (fRet)
                break;
            memcpy(vrgZipProd, rgzp, 160);
            break;
        }
        if (i < 0)
            break;
        for (cMax = 0; cMax < 4 && (!vrgZipProd[cMax].fValid || i-- != 0); cMax++) {
        }
        ProdCommandHandler(hwnd, 0x816, cMax);
        break;
    case WM_SETCURSOR:
        hcs = 0;
        GetCursorPos16(&pt);
        ScreenToClient16(hwnd, &pt);
        if (PtInRect(&rcProdDiamond, PointFrom16(pt)) == 0)
            break;
        SetCursor(hcurArrowHelp);
        return 1;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        GetClientRect(hwnd, &rc);
        DrawProductionDlg(hwnd, hdc, &rc, -1);
        EndPaint(hwnd, &ps);
        return 1;
    case WM_COMMAND:
        ProdCommandHandler(hwnd, wParam, lParam);
    }
    return 0;
}

void ProdCommandHandler(HWND hwnd, WPARAM wParam, LPARAM lParam) {
    int32_t lSel;
    int16_t iSrc;
    HWND    hwndLB;
    int16_t c;
    int16_t iDst;
    PROD    prodLast;
    int16_t ipl;
    int16_t fRefillSrc;
    int16_t iMac;
    RECT    rc;
    PROD   *lpprod;
    PROD    prod;
    int16_t cMax;
    PLPROD *lpplprodT;

    switch (GET_WM_COMMAND_ID(wParam, lParam)) {
    case IDC_PRODUCTION_ADD:
    AddItem:
        lSel = SendMessage(GetDlgItem(hwnd, IDC_PRODUCTION_AVAILABLE_ITEMS), LB_GETCURSEL, 0, 0);
        if (lSel < 0)
            break;
        for (iSrc = 0; iSrc < cProdGlob && (pProdGlob[iSrc].cItem == 0 || lSel-- != 0); iSrc++) {
        }
        prod = pProdGlob[iSrc];
        if (GetAsyncKeyState(VK_CONTROL) & 0xfffe) {
            if (GetAsyncKeyState(VK_SHIFT) & 0xfffe) {
                prod.cItem = LOWORD(prod.cItem < 1020 ? (uint32_t)prod.cItem : 1020);
            } else {
                prod.cItem = LOWORD(prod.cItem < 100 ? (uint32_t)prod.cItem : 100);
            }
        } else if (GetAsyncKeyState(VK_SHIFT) & 0xfffe) {
            prod.cItem = LOWORD(prod.cItem < 10 ? (uint32_t)prod.cItem : 10);
        } else {
            prod.cItem = 1;
        }
        if (pProdGlob[iSrc].cItem != 0x3ff) {
            pProdGlob[iSrc].cItem -= prod.cItem;
        }
        iMac = lpplProdGlob->iprodMac;
        lSel = SendMessage(GetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), LB_GETCURSEL, 0, 0);
        if (lSel < 0) {
            iDst = iMac - 1;
            if (iDst < 0) {
                iDst = 0;
            }
        } else {
            iDst = LOWORD(lSel) - 1;
        }
        if (iDst < iMac) {
            if (iDst >= 0) {
                prodLast = lpplProdGlob->rgprod[iDst];
                if ((uint32_t)prodLast.iItem == prod.iItem && (uint32_t)prodLast.grobj == prod.grobj) {
                RingItUp:
                    lpplProdGlob->rgprod[iDst].cItem = min(1020, lpplProdGlob->rgprod[iDst].cItem + prod.cItem);
                    if (lpplProdGlob->rgprod[iDst].cItem > 1 && lpplProdGlob->rgprod[iDst].iItem == iobjAlchemy &&
                        lpplProdGlob->rgprod[iDst].grobj == grobjPlanet) {
                        lpplProdGlob->rgprod[iDst].cItem = 1;
                    }
                    goto FixedUp;
                }
            }
            iDst++;
            if (iDst < iMac) {
                prodLast = lpplProdGlob->rgprod[iDst];
                if ((uint32_t)prodLast.iItem == prod.iItem && (uint32_t)prodLast.grobj == prod.grobj)
                    goto RingItUp;
            }
        }
        if (iMac >= 40) {
            MessageBeep(MB_OK);
            goto RedrawText;
        }
        if (iMac == lpplProdGlob->iprodMax) {
            lpplProdGlob = (PLPROD *)LpplReAlloc((PL *)lpplProdGlob, iMac + 4);
        }
        if (iDst != iMac) {
            fmemmove(lpplProdGlob + (1 + (iDst + 1)), &lpplProdGlob->rgprod[iDst], (iMac - iDst) * 4);
        }
        lpplProdGlob->rgprod[iDst] = prod;
        lpplProdGlob->iprodMac++;
    FixedUp:
        FillPlanetProdLB(GetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), lpplProdGlob, NULL);
        SendMessage(GetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), LB_SETCURSEL, iDst + 1, 0);
        if (pProdGlob[iSrc].cItem != 0)
            goto RedrawText;
        FillProdSrcLB(GetDlgItem(hwnd, IDC_PRODUCTION_AVAILABLE_ITEMS), -1);
        goto RedrawText;
    case IDC_PRODUCTION_REMOVE:
    RemoveItem:
        lSel = SendMessage(GetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), LB_GETCURSEL, 0, 0);
        if (lSel <= 0)
            break;
        iMac = lpplProdGlob->iprodMac;
        lSel--;
        prod = lpplProdGlob->rgprod[lSel];
        for (iSrc = 0; iSrc < cProdGlob && ((uint32_t)pProdGlob[iSrc].grobj != prod.grobj || (uint32_t)pProdGlob[iSrc].iItem != prod.iItem); iSrc++) {
        }
        fRefillSrc = pProdGlob[iSrc].cItem == 0;
        if (GetAsyncKeyState(VK_CONTROL) & 0xfffe) {
            if (GetAsyncKeyState(VK_SHIFT) & 0xfffe) {
                c = 1020;
            } else {
                c = 100;
            }
        } else if (GetAsyncKeyState(VK_SHIFT) & 0xfffe) {
            c = 10;
        } else {
            c = 1;
        }
        if (prod.grobj == grobjPlanet && prod.iItem == iobjAlchemy) {
            c = 1020;
        }
        c = c >= (int32_t)prod.cItem ? prod.cItem : c;
        if (pProdGlob[iSrc].cItem != 0x3ff) {
            pProdGlob[iSrc].cItem += c;
        }
        lpplProdGlob->rgprod[lSel].cItem -= c;
        if (lpplProdGlob->rgprod[lSel].cItem == 0) {
            if ((int32_t)(lSel + 1) < iMac) {
                fmemmove(&lpplProdGlob->rgprod[lSel], &lpplProdGlob->rgprod[lSel + 1], (iMac - LOWORD(lSel) - 1) * sizeof(PROD));
            } else {
                lSel--;
            }
            lpplProdGlob->iprodMac--;
        }
        FillPlanetProdLB(GetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), lpplProdGlob, NULL);
        if (lSel >= 0) {
            SendMessage(GetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), LB_SETCURSEL, LOWORD(lSel) + 1, 0);
        }
        if (!fRefillSrc)
            goto RedrawText;
        FillProdSrcLB(GetDlgItem(hwnd, IDC_PRODUCTION_AVAILABLE_ITEMS), -1);
        goto RedrawText;
    case IDC_PRODUCTION_CLEAR:
    case IDC_IMPORT:
        ipl = 0;
        lpprod = lpplProdGlob->rgprod;
        while (ipl < lpplProdGlob->iprodMac) {
            for (iSrc = 0; iSrc < cProdGlob && ((uint32_t)pProdGlob[iSrc].grobj != lpprod->grobj || (uint32_t)pProdGlob[iSrc].iItem != lpprod->iItem); iSrc++) {
            }
            if (pProdGlob[iSrc].cItem != 0x3ff) {
                pProdGlob[iSrc].cItem += lpprod->cItem;
            }
            ipl++;
            lpprod++;
        }
        if (GET_WM_COMMAND_ID(wParam, lParam) == IDC_IMPORT) {
            cMax = lpplProdGlob->iprodMac + vrgZipProd[lParam].cpq;
            if (cMax < 1) {
                cMax = 1;
            }
            lpplprodT = (PLPROD *)LpplAlloc(4, cMax, htOrd);
            fmemset(lpplprodT->rgprod, 0, cMax * 4);
            iDst = 0;
            for (iSrc = 0; iSrc < lpplProdGlob->iprodMac; iSrc++) {
                if (lpplProdGlob->rgprod[iSrc].grobj != grobjPlanet || lpplProdGlob->rgprod[iSrc].iItem >= mdIdleFactory) {
                    lpplprodT->rgprod[iDst] = lpplProdGlob->rgprod[iSrc];
                    iDst++;
                }
            }
            for (iSrc = 0; iSrc < vrgZipProd[lParam].cpq; iSrc++) {
                if ((GetRaceStat(&rgplr[idPlayer], rsMajorAdv) != raMacintosh || vrgZipProd[lParam].rgpq[iSrc].mdIdle > iobjDefense) &&
                    (GetRaceStat(&rgplr[idPlayer], rsMajorAdv) != raTerra ||
                     (vrgZipProd[lParam].rgpq[iSrc].mdIdle != iobjMinTerraform && vrgZipProd[lParam].rgpq[iSrc].mdIdle != iobjMaxTerraform))) {
                    lpplprodT->rgprod[iDst].grobj = grobjPlanet;
                    lpplprodT->rgprod[iDst].iItem = vrgZipProd[lParam].rgpq[iSrc].mdIdle;
                    lpplprodT->rgprod[iDst].cItem = vrgZipProd[lParam].rgpq[iSrc].cQuan;
                    iDst++;
                }
            }
            lpplprodT->iprodMac = iDst;
            FreePl((PL *)lpplProdGlob);
            lpplProdGlob = lpplprodT;
            sel.pl.fNoResearch = vrgZipProd[lParam].fNoResearch;
            SendMessage(GetDlgItem(hwnd, IDC_PRODUCTION_RESEARCH_LEFTOVERS_ONLY), BM_SETCHECK, sel.pl.fNoResearch, 0);
        } else {
            lpplProdGlob->iprodMac = 0;
        }
        FillPlanetProdLB(GetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), lpplProdGlob, NULL);
        SendMessage(GetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), LB_SETCURSEL, 0, 0);
        FillProdSrcLB(GetDlgItem(hwnd, IDC_PRODUCTION_AVAILABLE_ITEMS), -1);
        SendMessage(GetDlgItem(hwnd, IDC_PRODUCTION_AVAILABLE_ITEMS), LB_SETCURSEL, 0, 0);
        goto RedrawText;
    case IDC_PRODUCTION_AVAILABLE_ITEMS:
    case IDC_PRODUCTION_QUEUE:
        if (GET_WM_COMMAND_CMD(wParam, lParam) == 1) {
        RedrawText:
            GetClientRect(hwnd, &rc);
            rc.top = yTopFutureTech;
            rc.bottom = 7 * dyArial8 + rc.top;
            rc.left += 130;
            InvalidateRect(hwnd, &rc, TRUE);
            DrawProductionDlg(hwnd, NULL, &rc, -1);
        } else if (GET_WM_COMMAND_CMD(wParam, lParam) == 2) {
            if (GET_WM_COMMAND_ID(wParam, lParam) == IDC_PRODUCTION_AVAILABLE_ITEMS)
                goto AddItem;
            goto RemoveItem;
        }
        break;
    case IDC_PRODUCTION_RESEARCH_LEFTOVERS_ONLY:
        hwndLB = GetDlgItem(hwnd, IDC_PRODUCTION_QUEUE);
        sel.pl.fNoResearch = LOWORD(SendMessage(GetDlgItem(hwnd, IDC_PRODUCTION_RESEARCH_LEFTOVERS_ONLY), BM_GETCHECK, 0, 0));
        lSel = SendMessage(hwndLB, LB_GETCURSEL, 0, 0);
        FillPlanetProdLB(hwndLB, lpplProdGlob, NULL);
        SendMessage(hwndLB, LB_SETCURSEL, LOWORD(lSel), 0);
        goto RedrawText;
    case IDOK:
    case IDCANCEL:
        hwndProdDlg = 0;
        StickyDlgPos(hwnd, &ptStickyProduceDlg, FALSE);
        EndDialog(hwnd, GET_WM_COMMAND_ID(wParam, lParam) == IDOK);
        break;
    case IDC_BACK:
    case IDC_NEXT:
        c = GET_WM_COMMAND_ID(wParam, lParam) == IDC_NEXT ? 1 : -1;
        FinishProduction(TRUE);
        if (GetKeyState(VK_SHIFT) < 0) {
            SelectAdjPlanet(0, IdFindAdjStarbase(sel.pl.id, GET_WM_COMMAND_ID(wParam, lParam) == IDC_NEXT));
        } else {
            SelectAdjPlanet(c, 0);
        }
        InitProduction(NULL);
        InitializeProductionDlg(hwnd);
        GetClientRect(hwnd, &rc);
        rc.top = yTopFutureTech;
        rc.bottom = 9 * dyArial8 + rc.top;
        InvalidateRect(hwnd, &rc, TRUE);
        break;
    case IDC_PRODUCTION_ITEM_DOWN:
        lSel = SendMessage(GetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), LB_GETCURSEL, 0, 0);
        iMac = lpplProdGlob->iprodMac;
        if (lSel <= 0 || lSel >= iMac)
            break;
        lSel--;
        prod = lpplProdGlob->rgprod[lSel];
        lpplProdGlob->rgprod[lSel] = lpplProdGlob->rgprod[lSel + 1];
        lpplProdGlob->rgprod[lSel + 1] = prod;
        FillPlanetProdLB(GetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), lpplProdGlob, NULL);
        SendMessage(GetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), LB_SETCURSEL, LOWORD(lSel) + 2, 0);
        goto RedrawText;
    case IDC_PRODUCTION_ITEM_UP:
        lSel = SendMessage(GetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), LB_GETCURSEL, 0, 0);
        if (lSel <= 1)
            break;
        iMac = lpplProdGlob->iprodMac;
        lSel -= 2;
        prod = lpplProdGlob->rgprod[lSel];
        lpplProdGlob->rgprod[lSel] = lpplProdGlob->rgprod[lSel + 1];
        lpplProdGlob->rgprod[lSel + 1] = prod;
        FillPlanetProdLB(GetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), lpplProdGlob, NULL);
        SendMessage(GetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), LB_SETCURSEL, LOWORD(lSel) + 1, 0);
        goto RedrawText;
    case IDC_HELP:
        WinHelp(hwnd, szHelpFile, HELP_CONTEXT, idhProductionDialog);
    }
}

void InitializeProductionDlg(HWND hwnd) {
    char    rgch[86];
    int16_t i;
    int16_t iSel;
    PROD   *lpprod;

    iSel = -1;
    _wsprintf(rgch, PszGetCompressedString(idsProductionQueueS), PszGetPlanetName(sel.pl.id));
    SetWindowText(hwnd, rgch);
    FillProdSrcLB(GetDlgItem(hwnd, IDC_PRODUCTION_AVAILABLE_ITEMS), -1);
    SendMessage(GetDlgItem(hwnd, IDC_PRODUCTION_AVAILABLE_ITEMS), LB_SETCURSEL, 0, 0);
    i = 0;
    lpprod = lpplProdGlob->rgprod;
    while (i < lpplProdGlob->iprodMac) {
        if (lpprod->grobj != grobjPlanet || lpprod->iItem >= mdIdleFactory) {
            iSel = i;
        }
        i++;
        lpprod++;
    }
    FillPlanetProdLB(GetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), lpplProdGlob, NULL);
    SendMessage(GetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), LB_SETCURSEL, iSel + 1, 0);
    SendMessage(GetDlgItem(hwnd, IDC_PRODUCTION_RESEARCH_LEFTOVERS_ONLY), BM_SETCHECK, sel.pl.fNoResearch, 0);
    return;
}

void DrawProductionDlg(HWND hwnd, HDC hdc, RECT *prc, int16_t iDraw) {
    int32_t lSel;
    int16_t iSrc;
    int16_t idc;
    int16_t fCreatedDC;
    int16_t i;
    int16_t c;
    int32_t rgCost[4];
    int16_t dxkT;
    int16_t k;
    RECT    rc;
    PROD    prod;
    char    szT[100];

    fCreatedDC = FALSE;
    if (!hdc) {
        fCreatedDC = TRUE;
        hdc = GetDC(hwnd);
    }
    SelectObject(hdc, rghfontArial8[0]);
    dxkT = LOWORD(GetTextExtent(hdc, PszGetCompressedString(idsKt), 2));
    SelectObject(hdc, rghfontArial8[1]);
    SetBkColor(hdc, crButtonFace);
    for (i = 0; i < 2; i++) {
        idc = i == 0 ? 1046 : 1047;
        GetWindowRect(GetDlgItem(hwnd, idc), &rc);
        ScreenToClient(hwnd, (POINT *)&rc);
        ScreenToClient(hwnd, (POINT *)&rc.right);
        lSel = SendMessage(GetDlgItem(hwnd, idc), LB_GETCURSEL, 0, 0);
        if (lSel >= 0 && (lSel != 0 || i != 1)) {
            if (i == 0) {
                for (iSrc = 0; iSrc < cProdGlob && (pProdGlob[iSrc].cItem == 0 || lSel-- != 0); iSrc++) {
                }
                prod = pProdGlob[iSrc];
                prod.cItem = 1;
            } else {
                lSel--;
                prod = lpplProdGlob->rgprod[lSel];
            }
            GetProductionCosts(&sel.pl, &prod, rgCost, idPlayer, FALSE);
            rc.bottom = yTopFutureTech + 4;
            SelectObject(hdc, rghfontArial8[1]);
            c = CchGetString(idsRequiredMinerals, szWork);
            TextOut(hdc, rc.left, rc.bottom, szWork, c);
            rc.left += 20;
            rc.right -= 20;
            for (k = 0; k <= 3; k++) {
                c = k == 3 ? 5 : k;
                rc.bottom += dyArial8;
                SelectObject(hdc, rghfontArial8[1]);
                SetTextColor(hdc, rgcrMinerals[c]);
                TextOut(hdc, rc.left, rc.bottom, rgszMinerals[c], lstrlen(rgszMinerals[c]));
                SelectObject(hdc, rghfontArial8[0]);
                SetTextColor(hdc, crWindowText);
                c = _wsprintf(szWork, PCTLD, rgCost[k]);
                RightTextOut(hdc, rc.right - dxkT - 2, rc.bottom, szWork, c, dxMaxMineralQuan);
                if (k <= 2) {
                    TextOut(hdc, rc.right - dxkT, rc.bottom, PszGetCompressedString(idsKt), 2);
                }
            }
            if (i != 0) {
                rc.bottom += (int16_t)(3 * dyArial8) / 2;
                SelectObject(hdc, rghfontArial8[1]);
                c = _wsprintf(szT, PszGetCompressedString(idsDDoneCompletion), prod.pct);
                if (PszProductionETA(&sel.pl, lpplProdGlob, LOWORD(lSel), NULL, NULL) != szWork) {
                }
                strcpy(&szT[c], szWork);
                TextOut(hdc, rc.left - 20, rc.bottom, szT, strlen(szT));
                SelectObject(hdc, rghfontArial8[0]);
            }
        }
    }
    GetClientRect(hwnd, &rc);
    rc.top = rc.bottom - ((int16_t)(5 * dyArial8) / 2 + 12);
    rc.bottom = (dyArial8 | 1) + rc.top;
    rc.left = 6;
    rc.right = rc.left + dyArial8;
    DrawDiamond(hdc, &rc, hbrBBlue);
    rcProdDiamond = rc;
    SelectObject(hdc, rghfontArial8[1]);
    c = CchGetString(idsApplyDefineProductionTemplate, szWork);
    TextOut(hdc, rc.right + 4, rc.top, szWork, c);
    if (fCreatedDC) {
        ReleaseDC(hwnd, hdc);
    }
    return;
}

void FillProdSrcLB(HWND hwndLB, int16_t mdFill) {
    char    szT[80];
    int16_t i;
    char   *psz;

    for (i = 0; i < 6; i++) {
        szT[i] = ' ';
    }
    SendMessage(hwndLB, LB_RESETCONTENT, 0, 0);
    for (i = 0; i < cProdGlob; i++) {
        if (pProdGlob[i].cItem > 0) {
            psz = PszNameProdItem(pProdGlob + i);
            strcpy(&szT[6], psz);
            if (pProdGlob[i].grobj == grobjFleet) {
                szT[0] = pProdGlob[i].iItem < iobjPacketGerm ? 42 : 35;
            } else if (pProdGlob[i].iItem < mdIdleFactory) {
                szT[0] = 'I';
                strcat(&szT[6], " (Auto Build)");
            } else {
                szT[0] = ' ';
            }
            SendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM)szT);
        }
    }
    return;
}

char *PszNameProdItem(PROD *lpprod) {
    uint32_t iItem;
    int16_t  iDelta;

    iItem = lpprod->iItem;
    if (lpprod->grobj == grobjFleet) {
        if (iItem >= 16) {
            iItem -= 16;
            if (rglpshdefSB[idPlayer][iItem].fFree) {
            LBogus:
                szWork[0] = 0;
                return szWork;
            }
            fstrcpy(szWork, rglpshdefSB[idPlayer][iItem].hul.szClass);
            if (!sel.pl.fStarbase) {
                return szWork;
            }
            iDelta = rglpshdefSB[idPlayer][sel.pl.isb].hul.ihuldef - rglpshdefSB[idPlayer][iItem].hul.ihuldef;
            if (iDelta > 0) {
                strcat(szWork, " (downgrade)");
                return szWork;
            }
            if (iDelta >= 0) {
                return szWork;
            }
            strcat(szWork, " (upgrade)");
            return szWork;
        } else {
            if (rgshdef[iItem].fFree)
                goto LBogus;
            strcpy(szWork, rgshdef[iItem].hul.szClass);
            return szWork;
        }
    }
    if (iItem >= iobjPlanetaryScannerFirst && iItem <= iobjPlanetaryScannerLast) {
        fstrcpy(szWork, LpplanetaryFromId(LOWORD(iItem) - iobjPlanetaryScannerFirst)->szName);
    } else if (iItem == iobjPlanetaryScanner) {
        CchGetString(idsPlanetaryScanner, szWork);
    } else {
        CchGetString(LOWORD(iItem) + 126, szWork);
    }
    return szWork;
}

void GetProductionCosts(PLANET *lppl, PROD *lpprod, uint32_t *rgCost, int16_t iplr, int16_t fOnlyOne) {
    uint16_t      rgCostsCur[4];
    uint32_t      iItem;
    uint16_t      rgCosts[4];
    int16_t       i;
    int16_t       j;
    SHDEF        *lpshdef;
    RaceAttribute raMajor;
    uint32_t      cItem;
    int16_t       fStarbase;
    PART          part;
    int16_t       cost;
    int16_t       chs;
    HUL          *lphulNew;
    HUL          *lphulCur;
    int16_t       costUpg;
    int16_t       costHalf;
    HUL          *lphulT;
    int16_t       rgCostsPartCur[4];
    int16_t       rgCostsPartNew[4];

    raMajor = GetRaceStat(&rgplr[lppl->iPlayer], rsMajorAdv);
    fStarbase = FALSE;
    iItem = lpprod->iItem;
    cItem = lpprod->cItem;
    if (lpprod->grobj == grobjFleet) {
        if (iItem >= iobjPacketGerm) {
            lpshdef = rglpshdefSB[iplr];
            iItem -= 16;
            fStarbase = TRUE;
        } else {
            lpshdef = rglpshdef[iplr];
        }
        if (lpshdef[iItem].fFree) {
            for (i = 0; i < 4; i++) {
                rgCost[i] = 0;
            }
            return;
        }
        GetTrueHullCost(iplr, &lpshdef[iItem].hul, rgCosts);
        if (fStarbase && lppl->fStarbase) {
            lphulCur = &rglpshdefSB[iplr][lppl->isb].hul;
            lphulNew = &lpshdef[iItem].hul;
            GetTrueHullCost(iplr, lphulCur, rgCostsCur);
            if (lphulCur->ihuldef != lphulNew->ihuldef) {
                for (i = 0; i < 4; i++) {
                    costHalf = (uint32_t)rgCosts[i] / 2;
                    costUpg = rgCosts[i] - (int16_t)rgCostsCur[i] / 2;
                    if (costHalf > costUpg) {
                        rgCosts[i] = costHalf;
                    } else {
                        rgCosts[i] = costUpg;
                    }
                }
            } else {
                lphulT = &LphuldefFromId(lphulCur->ihuldef)->hul;
                part.hs.grhst = hstNone;
                part.phul = lphulT;
                GetTruePartCost(iplr, &part, rgCostsPartCur);
                for (i = 0; i < 4; i++) {
                    rgCosts[i] -= rgCostsPartCur[i];
                }
                chs = lphulCur->chs;
                for (i = 0; i < chs; i++) {
                    if (lphulCur->rghs[i].cItem != 0 && lphulNew->rghs[i].cItem != 0) {
                        part.hs = lphulCur->rghs[i];
                        FLookupPart(&part);
                        GetTruePartCost(iplr, &part, rgCostsPartCur);
                        part.hs = lphulNew->rghs[i];
                        FLookupPart(&part);
                        GetTruePartCost(iplr, &part, rgCostsPartNew);
                        if (lphulCur->rghs[i].grhst != lphulNew->rghs[i].grhst) {
                            for (j = 0; j < 4; j++) {
                                rgCostsPartCur[j] *= lphulCur->rghs[i].cItem;
                                rgCostsPartNew[j] *= lphulNew->rghs[i].cItem;
                                cost = (int16_t)(3 * rgCostsPartNew[j]) / 10 <= rgCostsPartNew[j] - (int16_t)(7 * rgCostsPartCur[j]) / 10
                                           ? rgCostsPartNew[j] - (int16_t)(7 * rgCostsPartCur[j]) / 10
                                           : (int16_t)(3 * rgCostsPartNew[j]) / 10;
                                rgCosts[j] -= rgCosts[j] >= (uint16_t)(rgCostsPartNew[j] - cost) ? rgCostsPartNew[j] - cost : rgCosts[j];
                            }
                        } else if (lphulCur->rghs[i].iItem != lphulNew->rghs[i].iItem) {
                            for (j = 0; j < 4; j++) {
                                rgCostsPartCur[j] *= lphulCur->rghs[i].cItem;
                                rgCostsPartNew[j] *= lphulNew->rghs[i].cItem;
                                cost = (int16_t)(rgCostsPartNew[j] * 2) / 10 <= rgCostsPartNew[j] - (int16_t)(rgCostsPartCur[j] * 8) / 10
                                           ? rgCostsPartNew[j] - (int16_t)(rgCostsPartCur[j] * 8) / 10
                                           : (int16_t)(rgCostsPartNew[j] * 2) / 10;
                                rgCosts[j] -= rgCosts[j] >= (uint16_t)(rgCostsPartNew[j] - cost) ? rgCostsPartNew[j] - cost : rgCosts[j];
                            }
                        } else {
                            for (j = 0; j < 4; j++) {
                                rgCostsPartCur[j] *= lphulCur->rghs[i].cItem;
                                rgCostsPartNew[j] *= lphulNew->rghs[i].cItem;
                                cost = 0 <= rgCostsPartNew[j] - rgCostsPartCur[j] ? rgCostsPartNew[j] - rgCostsPartCur[j] : 0;
                                rgCosts[j] -= rgCosts[j] >= (uint16_t)(rgCostsPartNew[j] - cost) ? rgCostsPartNew[j] - cost : rgCosts[j];
                            }
                        }
                    }
                }
            }
        }
        if (fStarbase && (GetRaceGrbit(&rgplr[iplr], ibitRaceISB) != 0 || GetRaceStat(&rgplr[iplr], rsMajorAdv) == raMacintosh)) {
            for (i = 0; i < 4; i++) {
                rgCosts[i] -= (uint32_t)rgCosts[i] / 5;
            }
        }
        if (fStarbase) {
            for (i = 0; i < 4; i++) {
                rgCost[i] = (uint32_t)((uint32_t)(rgCosts[i] + 1) / 2);
            }
        } else {
            for (i = 0; i < 4; i++) {
                rgCost[i] = (uint32_t)rgCosts[i];
            }
        }
    } else {
        switch (iItem) {
        case iobjFactory:
        case mdIdleFactory:
            cost = GetRaceGrbit(&rgplr[iplr], ibitRaceCheapFact);
            if (gd.fTutorial) {
                for (i = 0; i < 3; i++) {
                    rgCost[i] = (int16_t)(2 - cost);
                }
            } else {
                rgCost[1] = 0;
                *rgCost = 0;
                rgCost[2] = (int16_t)(4 - cost);
            }
            rgCost[3] = GetRaceStat(&rgplr[iplr], rsFactBuild);
            break;
        case iobjMine:
        case mdIdleMine:
            for (i = 0; i < 3; i++) {
                rgCost[i] = 0;
            }
            rgCost[3] = GetRaceStat(&rgplr[iplr], rsMineBuild);
            break;
        case iobjDefense:
        case mdIdleDefense:
            part.hs.grhst = hstPlanetary;
            part.hs.iItem = iplanetarySDI;
            FLookupPart(&part);
            for (i = 0; i < 3; i++) {
                rgCost[i] = part.pplanetary->rgwtOreCost[i];
            }
            rgCost[3] = (uint32_t)part.pplanetary->resCost;
            if (GetRaceStat(&rgplr[iplr], rsMajorAdv) != raDefend)
                break;
            for (i = 0; i < 4; i++) {
                rgCost[i] = (uint32_t)((uint32_t)(rgCost[i] * 3) / 5);
            }
            break;
        case iobjAlchemy:
        case mdIdleAlchemy:
            for (i = 0; i < 3; i++) {
                rgCost[i] = 0;
            }
            rgCost[3] = (uint32_t)(GetRaceGrbit(&rgplr[iplr], ibitRaceMineralAlchemy) == 0 ? 100 : 25);
            break;
        case iobjGenesis:
            part.hs.grhst = hstPlanetary;
            part.hs.iItem = iplanetaryGenesisDevice;
            FLookupPart(&part);
            GetTruePartCost(iplr, &part, rgCosts);
            for (i = 0; i < 4; i++) {
                rgCost[i] = (uint32_t)rgCosts[i];
            }
            break;
        case iobjPacketIron:
        case iobjPacketBor:
        case iobjPacketGerm:
            j = raMajor == raMassAccel ? 70 : raMajor == raStargate ? 120 : 110;
            for (i = 0; i < 3; i++) {
                rgCost[i] = iItem - 14 == (uint32_t)i ? j : 0;
            }
            rgCost[i] = (uint32_t)(raMajor == raMassAccel ? 5 : 10);
            break;
        case iobjPacket:
        case iobjPacketMixed:
            j = raMajor == raMassAccel ? 25 : raMajor == raStargate ? 48 : 44;
            for (i = 0; i < 3; i++) {
                rgCost[i] = j;
            }
            rgCost[i] = (uint32_t)(raMajor == raMassAccel ? 5 : 10);
            break;
        case iobjMinTerraform:
        case iobjMaxTerraform:
        case mdIdleTerraform:
            rgCost[2] = 0;
            rgCost[1] = 0;
            *rgCost = 0;
            if (GetRaceGrbit(&rgplr[iplr], ibitRaceTT) != 0) {
                rgCost[3] = 70;
            } else {
                rgCost[3] = 100;
            }
            if (raMajor != raTerra)
                break;
            rgCost[3] = (uint32_t)(rgCost[3] / 2);
            break;
        case iobjPlanetaryScanner:
            iItem = iobjPlanetaryScannerFirst;
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
            part.hs.grhst = hstPlanetary;
            part.hs.iItem = LOWORD(iItem) - 18;
            FLookupPart(&part);
            GetTruePartCost(iplr, &part, rgCosts);
            for (i = 0; i < 4; i++) {
                rgCost[i] = (uint32_t)rgCosts[i];
            }
        }
    }
    if (!fOnlyOne) {
        for (i = 0; i < 4; i++) {
            rgCost[i] = (uint32_t)(rgCost[i] * cItem);
        }
    }
    return;
}

void EstimateItemProdSched(PLANET *lppl, PLPROD *lpplprod, ProdItemType iItem, int16_t *piFirst, int16_t *piLast) {
    int32_t    cResearch;
    PLANET     pl;
    int32_t    rglQuan[3];
    int16_t    cBuilt;
    PROD       prodPartial;
    mdProdStat mdStatus;
    int16_t    i;
    int16_t    j;
    int16_t    iPass;
    int16_t    fAlchemy;
    int16_t    iMac;
    int32_t    rgRes[4];
    PROD      *lpprod;

    if (!lpplprod) {
        lpplprod = lppl->lpplprod;
    }
    pl = *lppl;
    pl.lpplprod = (PLPROD *)LpplAlloc(4, lpplprod->iprodMax, htOrd);
    fmemcpy(pl.lpplprod->rgprod, lpplprod->rgprod, lpplprod->iprodMac * 4);
    pl.lpplprod->iprodMac = lpplprod->iprodMac;
    iMac = lpplprod->iprodMac;
    prodPartial.cItem = 0;
    *piLast = 0;
    *piFirst = 0;
    for (iPass = 1; iPass < 100; iPass++) {
        EstMineralsMined(&pl, rglQuan, -1, TRUE);
        for (j = 0; j < 3; j++) {
            rgRes[j] = pl.rgwtMin[j];
        }
        rgRes[3] = CResourcesAtPlanet(&pl, lppl->iPlayer);
        if (!pl.fNoResearch) {
            cResearch = (int32_t)(rgRes[3] * (int16_t)rgplr[lppl->iPlayer].pctResearch) / 100;
            rgRes[3] -= cResearch;
        } else {
            cResearch = 0;
        }
        fAlchemy = FALSE;
        for (i = -1; i < iMac; i++) {
            if (i == -1) {
                lpprod = &prodPartial;
            } else {
                lpprod = &pl.lpplprod->rgprod[i];
            }
            if (lpprod->cItem != 0) {
                if (lpprod->iItem == iobjAlchemy && lpprod->grobj == grobjPlanet) {
                    if (i < iMac - 1) {
                        if (i == iItem) {
                            *piLast = -1;
                            *piFirst = -1;
                            goto LCleanUp;
                        }
                        fAlchemy = TRUE;
                        continue;
                    }
                    lpprod->cItem = 1020;
                }
                cBuilt = CBuildProdItem(&pl, lpprod, i == -1 ? NULL : &prodPartial, rgRes, fAlchemy, (int16_t *)&mdStatus, FALSE);
                if (iItem == i) {
                    if (cBuilt > 0 && *piFirst == 0) {
                        *piFirst = iPass;
                    }
                    if (mdStatus == mdProdStatSkippedAuto) {
                        if (*piFirst != 0) {
                            *piLast = iPass - 1;
                        }
                        goto LCleanUp;
                    }
                    if (mdStatus == mdProdStatComplete || mdStatus == mdProdStatCompleteAuto) {
                        *piLast = iPass;
                        goto LCleanUp;
                    }
                }
                fAlchemy = FALSE;
                if (lpprod->grobj == grobjPlanet) {
                    switch (lpprod->iItem) {
                    case iobjMine:
                    case mdIdleMine:
                        pl.cMines += cBuilt;
                        break;
                    case iobjFactory:
                    case mdIdleFactory:
                        pl.cFactories += cBuilt;
                    }
                }
                if ((int16_t)mdStatus >= mdProdStatSome)
                    break;
            }
        }
        if ((int16_t)iItem < iobjMine) {
            *piFirst = LOWORD(rgRes[3]);
            if (iItem == iprodEstimateResearchResources) {
                *piFirst += LOWORD(cResearch);
            }
            goto LCleanUp;
        }
        for (j = 0; j < 3; j++) {
            pl.rgwtMin[j] = rgRes[j];
        }
        ChgPopFromPlanet(&pl, TRUE);
    }
    if (*piFirst == 0) {
        *piFirst = 100;
    }
    *piLast = 100;
LCleanUp:
    if (pl.lpplprod) {
        FreePl((PL *)pl.lpplprod);
    }
    return;
}

INT_PTR CALLBACK ZipProdDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    HDC         hdc;
    PAINTSTRUCT ps;
    int16_t     i;
    int16_t     iBase;
    RECT        rc;
    int16_t     dy;
    RECT        rc2;
    HWND        hwndRad;
    char       *psz;
    char       *pszT;
    RECT        rcGBox;
    int16_t     cch;
    FARPROC     lpProc;
    int16_t     cpq;

    switch (IS_WM_CTLCOLOR(message) ? WM_CTLCOLOR : message) { /* NATIVE: Win32 split WM_CTLCOLOR by control type. */
    case WM_INITDIALOG:
        SetWindowText(hwnd, PszGetCompressedString(idsCustomizeProductionTemplates));
        GetWindowRect(hwnd, &rc);
        GetClientRect(hwnd, &rc2);
        dy = rc.bottom - rc.top - rc2.bottom;
        GetWindowRect(GetDlgItem(hwnd, IDC_ZIP_PROD_QUEUE), &rc2);
        MapWindowPoints(NULL, hwnd, (POINT *)&rc2, 2);
        vyZPDStatic = rc2.bottom + 2;
        dy += rc2.bottom + dyArial8 + 6;
        SetWindowPos(hwnd, NULL, 0, 0, rc.right - rc.left, dy, SWP_NOMOVE | SWP_NOZORDER);
        CheckRadioButton(hwnd, IDC_ZIP_PROD_PRESET_1, IDC_ZIP_PROD_PRESET_4, IDC_ZIP_PROD_PRESET_1);
        EnableZipProdBtns(hwnd, 0);
        iResTechNow = Energy;
        FillZipProdLB(hwnd, &vrgZipProd[iResTechNow]);
        for (i = 1073; i <= 1076; i++) {
            iBase = i - 1073;
            if (vrgZipProd[iBase].fValid) {
                pszT = szWork;
                psz = vrgZipProd[iBase].szName;
                while (*psz != 0) {
                    if ((*pszT++ = *psz++) == '&') {
                        *pszT++ = '&';
                    }
                }
                *pszT = 0;
                psz = szWork;
            } else {
                psz = PszGetCompressedString(idsUnusedD);
                _wsprintf(szWork, psz, iBase + 1);
                psz = szWork;
            }
            hwndRad = GetDlgItem(hwnd, i);
            SetWindowText(hwndRad, psz);
        }
        StickyDlgPos(hwnd, &ptStickyZipProdDlg, TRUE);
        if (gd.fTutorial) {
            AdvanceTutor();
        }
        return 1;
    case WM_ERASEBKGND:
        GetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case WM_CTLCOLOR:
        for (i = 1073; i <= 1076 && GET_WM_CTLCOLOR_HWND(wParam, lParam) != GetDlgItem(hwnd, i); i++) {
        }
        if (i <= 1076) {
            SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        GetClientRect(hwnd, &rc);
        GetWindowRect(GetDlgItem(hwnd, IDC_ZIP_PROD_PRESET_1), &rcGBox);
        ScreenToClient(hwnd, (POINT *)&rcGBox);
        GetWindowRect(GetDlgItem(hwnd, IDC_ZIP_PROD_PRESET_4), &rc);
        ScreenToClient(hwnd, (POINT *)&rc.right);
        rcGBox.right = rc.right;
        rcGBox.bottom = rc.bottom;
        ExpandRc(&rcGBox, dyArial8, dyArial8 >> 1);
        _Draw3dFrame(hdc, &rcGBox, -1);
        SelectObject(hdc, rghfontArial8[1]);
        SetBkColor(hdc, crButtonFace);
        cch = CchGetString(idsCustomOrders, szWork);
        TextOut(hdc, rcGBox.left + 8, rcGBox.top - (dyArial8 >> 1), szWork, cch);
        rcGBox.top = rcGBox.bottom + 8;
        if (vrgZipProd[iResTechNow].fValid) {
            cch = CchGetString(vrgZipProd[iResTechNow].fNoResearch + 1222, szWork);
            TextOut(hdc, rcGBox.left, vyZPDStatic, szWork, cch);
        }
        EndPaint(hwnd, &ps);
        return 1;
    case WM_COMMAND:
        if (GET_WM_COMMAND_CMD(wParam, lParam) == 0 && GET_WM_COMMAND_ID(wParam, lParam) >= IDC_ZIP_PROD_PRESET_1 &&
            GET_WM_COMMAND_ID(wParam, lParam) <= IDC_ZIP_PROD_PRESET_4) {
            iResTechNow = GET_WM_COMMAND_ID(wParam, lParam) - 1073;
            EnableZipProdBtns(hwnd, iResTechNow);
            FillZipProdLB(hwnd, &vrgZipProd[iResTechNow]);
        } else {
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
            case IDOK:
            case IDCANCEL:
                StickyDlgPos(hwnd, &ptStickyZipProdDlg, FALSE);
                EndDialog(hwnd, GET_WM_COMMAND_ID(wParam, lParam) == IDOK);
                vyZPDStatic = -1;
                if (gd.fTutorial) {
                    AdvanceTutor();
                }
                return 1;
            case IDC_IMPORT:
            case IDC_RENAME:
                if (vrgZipProd[iResTechNow].fValid) {
                    strcpy(szWork, vrgZipProd[iResTechNow].szName);
                } else {
                    _wsprintf(szWork, PszGetCompressedString(idsCustomD), iResTechNow);
                }
                lpProc = MakeProcInstance(RenameZipDlg, hInst);
                if (iResTechNow == Energy)
                    goto LDontRename;
                if (DialogBox(hInst, MAKEINTRESOURCE(IDD_RENAME), hwndFrame, lpProc) != 0) {
                    if (szWork[0] == 0) {
                        _wsprintf(szWork, PszGetCompressedString(idsCustomD), iResTechNow);
                    }
                    strcpy(vrgZipProd[iResTechNow].szName, szWork);
                    pszT = &szWork[64];
                    psz = szWork;
                    while (*psz != 0) {
                        if ((*pszT++ = *psz++) == '&') {
                            *pszT++ = '&';
                        }
                    }
                    *pszT = 0;
                    SetWindowText(GetDlgItem(hwnd, iResTechNow + 1073), &szWork[64]);
                LDontRename:
                    if (GET_WM_COMMAND_ID(wParam, lParam) == IDC_IMPORT) {
                        vrgZipProd[iResTechNow].fValid = TRUE;
                        cpq = 0;
                        for (i = 0; i < lpplProdGlob->iprodMac; i++) {
                            if (lpplProdGlob->rgprod[i].grobj == grobjPlanet && lpplProdGlob->rgprod[i].iItem < mdIdleFactory) {
                                vrgZipProd[iResTechNow].rgpq[cpq].mdIdle = lpplProdGlob->rgprod[i].iItem;
                                vrgZipProd[iResTechNow].rgpq[cpq].cQuan = lpplProdGlob->rgprod[i].cItem;
                                cpq++;
                                if (cpq >= 12)
                                    break;
                            }
                        }
                        vrgZipProd[iResTechNow].cpq = cpq;
                        vrgZipProd[iResTechNow].fNoResearch = sel.pl.fNoResearch;
                        FillZipProdLB(hwnd, &vrgZipProd[iResTechNow]);
                    }
                    EnableZipProdBtns(hwnd, iResTechNow);
                }
                FreeProcInstance(lpProc);
                SetFocus(hwnd);
                gd.fChgZipProd = TRUE;
                break;
            case IDC_DELETE:
                vrgZipProd[iResTechNow].fValid = FALSE;
                _wsprintf(szWork, PszGetCompressedString(idsUnusedD), iResTechNow + 1);
                SetWindowText(GetDlgItem(hwnd, iResTechNow + 1073), szWork);
                FillZipProdLB(hwnd, &vrgZipProd[iResTechNow]);
                gd.fChgZipProd = TRUE;
                break;
            case IDC_HELP:
                WinHelp(hwnd, szHelpFile, HELP_CONTEXT, idhCustomizeProductionTemplatesDialog);
                return 1;
            }
        }
    }
    return 0;
}

void EnableZipProdBtns(HWND hwnd, int16_t iSel) {
    int16_t fEnabled;

    fEnabled = vrgZipProd[iSel].fValid != 0 && iSel > 0;
    EnableWindow(GetDlgItem(hwnd, IDC_DELETE), fEnabled);
    EnableWindow(GetDlgItem(hwnd, IDC_RENAME), fEnabled);
    return;
}

void FillZipProdLB(HWND hwndDlg, ZIPPRODQ *pzpq) {
    int16_t i;
    HWND    hwndLB;
    char    szAuto[40];
    char    szFormat[15];
    RECT    rc;

    hwndLB = GetDlgItem(hwndDlg, IDC_ZIP_PROD_QUEUE);
    GetClientRect(hwndDlg, &rc);
    rc.top = vyZPDStatic;
    rc.bottom = vyZPDStatic + dyArial8;
    InvalidateRect(hwndDlg, &rc, TRUE);
    SendMessage(hwndLB, LB_RESETCONTENT, 0, 0);
    if (!pzpq->fValid || pzpq->cpq == 0) {
        SendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM)PszGetCompressedString(idsAutoBuildOrders));
    } else {
        CchGetString(idsSD2, szFormat);
        for (i = 0; i < pzpq->cpq; i++) {
            CchGetString(pzpq->rgpq[i].mdIdle + 126, szAuto);
            if (pzpq->rgpq[i].cQuan == 1 || pzpq->rgpq[i].mdIdle == iobjAlchemy) {
                strcpy(szWork, szAuto);
            } else {
                _wsprintf(szWork, szFormat, szAuto, pzpq->rgpq[i].cQuan);
            }
            SendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM)szWork);
        }
    }
    return;
}
