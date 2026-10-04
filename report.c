#include "common.h"

uint16_t mpicolgrbitBU[12] = {255, 255, 255, 255, 255, 255, 255, 8, 16, 32, 64, 128};

LRESULT CALLBACK ReportDlg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HDC         hdc;
    HMENU       hmenu;
    RECT        rc;
    int16_t     i;
    int16_t     dx;
    uint16_t    swp;
    int16_t     cRow;
    POINT16     pt;
    int16_t     ibit;
    int16_t     iCol;
    int16_t     iRow;
    int16_t     xCur;
    int16_t     iCur;
    int16_t     iNew;
    PAINTSTRUCT ps;
    MessageId   idm;

    switch (msg) {
    case WM_CREATE:
        hwndReportDlg = hwnd;
        hdc = GetDC(hwnd);
        SelectObject(hdc, rghfontArial8[1]);
        for (i = 0; i < vprptCur->cFields; i++) {
            dx = DxReportColHdr(vprptCur->irpt, i, szWork, hdc);
            vprptCur->rgbdx[i] = dx / 2;
        }
        ReleaseDC(hwnd, hdc);
        SortReportCache(vprptCur->irpt, vprptCur->icolSort);
        SetWindowPos(hwnd, NULL, 0, 0, vprptCur->ptSize.x, vprptCur->ptSize.y, SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW);
        StickyDlgPos(hwnd, &vprptCur->ptDlg, TRUE);
        vprptCur->hwndVScroll = CreateWindow("SCROLLBAR", NULL, SBS_VERT | WS_CHILD, 0, 0, 50, 50, hwnd, NULL, hInst, NULL);
        vprptCur->hwndHScroll = CreateWindow("SCROLLBAR", NULL, WS_CHILD, 0, 0, 50, 50, hwnd, NULL, hInst, NULL);
        if (gd.fTutorial) {
            AdvanceTutor();
        }
        /* fallthrough */
    case WM_SIZE:
        GetClientRect(hwnd, &rc);
        cRow = (int16_t)(rc.bottom - 36) / (dyArial8 + 4);
        vprptCur->cRowsVis = cRow >= vprptCur->cRows ? vprptCur->cRows : cRow;
        if (vprptCur->cRowsVis >= vprptCur->cRows) {
            swp = SWP_NOZORDER | SWP_HIDEWINDOW;
            vprptCur->irowFirst = 0;
            SetScrollPos(vprptCur->hwndVScroll, SB_CTL, 0, FALSE);
        } else {
            swp = SWP_NOZORDER | SWP_SHOWWINDOW;
            if (vprptCur->irowFirst + vprptCur->cRowsVis > vprptCur->cRows && vprptCur->irowFirst > 0) {
                vprptCur->irowFirst = vprptCur->cRows - vprptCur->cRowsVis;
                if (vprptCur->irowFirst < 0) {
                    vprptCur->irowFirst = 0;
                }
            }
            SetScrollPos(vprptCur->hwndVScroll, SB_CTL, vprptCur->irowFirst, FALSE);
            SetScrollRange(vprptCur->hwndVScroll, SB_CTL, 0, vprptCur->cRows - vprptCur->cRowsVis, TRUE);
        }
        dx = GetSystemMetrics(SM_CXVSCROLL);
        SetWindowPos(vprptCur->hwndVScroll, NULL, rc.right - dx, dyArial8 + 6, dx, (dyArial8 + 4) * vprptCur->cRowsVis + 1, swp);
        SetHScrollBar();
        if (msg == WM_CREATE) {
            return 1;
        }
        return 0;
    case WM_GETMINMAXINFO:
        ((MINMAXINFO *)lParam)->ptMinTrackSize.x = 300;
        ((MINMAXINFO *)lParam)->ptMinTrackSize.y = 220;
        return 0;
    case WM_ERASEBKGND:
        GetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
        xCur = 2;
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        if (pt.y < 2 || pt.x < 2)
            break;
        if (pt.y < dyArial8 + 6) {
            iRow = -1;
        } else {
            iRow = (int16_t)(pt.y - 2 - (dyArial8 + 4)) / (dyArial8 + 4);
            if (iRow >= vprptCur->cRowsVis)
                break;
            iRow += vprptCur->irowFirst;
        }
        iCol = -1;
        i = 0;
        ibit = 1;
        while (i < vprptCur->cFields) {
            if ((ibit & vprptCur->grbitVisible) && (i == 0 || i >= vprptCur->cFieldFirst)) {
                xCur += vprptCur->rgbdx[i] * 2;
                if (xCur > pt.x) {
                    iCol = i;
                    break;
                }
            }
            i++;
            ibit *= 2;
        }
        if (iCol == -1)
            break;
        if (iRow == -1) {
            ReportColumnPopup(pt, iCol, msg == WM_RBUTTONDOWN);
        } else {
            ExecuteReportClick(pt, vprptCur->irpt, iCol, iRow);
        }
        if (gd.fTutorial) {
            AdvanceTutor();
        }
        break;
    case WM_VSCROLL:
        iCur = GetScrollPos(GET_WM_VSCROLL_HWND(wParam, lParam), SB_CTL);
        iNew = iCur;
        if (GET_WM_VSCROLL_CODE(wParam, lParam) <= SB_BOTTOM) {
            switch (GET_WM_VSCROLL_CODE(wParam, lParam)) {
            case SB_BOTTOM:
                iNew = 2000;
                break;
            case SB_LINEDOWN:
                iNew++;
                break;
            case SB_LINEUP:
                iNew--;
                break;
            case SB_PAGEDOWN:
                iNew += vprptCur->cRowsVis - 1;
                break;
            case SB_PAGEUP:
                iNew -= vprptCur->cRowsVis - 1;
                break;
            case SB_THUMBPOSITION:
            case SB_THUMBTRACK:
                iNew = GET_WM_VSCROLL_POS(wParam, lParam);
                break;
            case SB_TOP:
                iNew = 0;
            }
        }
        if (iNew > vprptCur->cRows - vprptCur->cRowsVis) {
            iNew = vprptCur->cRows - vprptCur->cRowsVis;
        }
        if (iNew < 0) {
            iNew = 0;
        }
        if (iNew != iCur) {
            vprptCur->irowFirst = iNew;
            GetClientRect(hwnd, &rc);
            rc.left = 2;
            rc.right -= GetSystemMetrics(SM_CXVSCROLL);
            rc.top = dyArial8 + 6;
            rc.bottom = (dyArial8 + 4) * vprptCur->cRowsVis + rc.top;
            ScrollWindow(hwnd, 0, (dyArial8 + 4) * (iCur - iNew), &rc, &rc);
            SetScrollPos(GET_WM_VSCROLL_HWND(wParam, lParam), SB_CTL, iNew, TRUE);
            UpdateWindow(hwnd);
        }
        return 0;
    case WM_HSCROLL:
        iCur = GetScrollPos(GET_WM_HSCROLL_HWND(wParam, lParam), SB_CTL);
        iNew = iCur;
        if (GET_WM_HSCROLL_CODE(wParam, lParam) <= SB_BOTTOM) {
            switch (GET_WM_HSCROLL_CODE(wParam, lParam)) {
            case SB_BOTTOM:
                iNew = 2000;
                break;
            case SB_LINEDOWN:
                iNew++;
                break;
            case SB_LINEUP:
                iNew--;
                break;
            case SB_PAGEDOWN:
                iNew += 3;
                break;
            case SB_PAGEUP:
                iNew -= 3;
                break;
            case SB_THUMBPOSITION:
            case SB_THUMBTRACK:
                iNew = GET_WM_HSCROLL_POS(wParam, lParam);
                break;
            case SB_TOP:
                iNew = 0;
            }
        }
        if (iNew > vprptCur->cColScroll) {
            iNew = vprptCur->cColScroll;
        }
        if (iNew < 0) {
            iNew = 0;
        }
        if (iNew != iCur) {
            SetScrollPos(GET_WM_HSCROLL_HWND(wParam, lParam), SB_CTL, iNew, TRUE);
            iNew = GetScrollPos(GET_WM_HSCROLL_HWND(wParam, lParam), SB_CTL);
            if (iNew != iCur) {
                i = 1;
                for (ibit = 2; i < vprptCur->cFields && (!(ibit & vprptCur->grbitVisible) || iNew-- > 0); ibit *= 2) {
                    i++;
                }
                vprptCur->cFieldFirst = i;
                InvalidateRect(hwnd, NULL, TRUE);
                UpdateWindow(hwnd);
            }
        }
        return 0;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        DrawReport(hwnd, hdc, &ps.rcPaint);
        EndPaint(hwnd, &ps);
        gd.fRptSafeDraw = FALSE;
        return 1;
    case WM_DESTROY:
        StickyDlgPos(hwnd, &vprptCur->ptDlg, FALSE);
        GetWindowRect(hwnd, &rc);
        vprptCur->ptSize.x = rc.right - rc.left;
        vprptCur->ptSize.y = rc.bottom - rc.top;
        hwndReportDlg = 0;
        fBrowserValid = FALSE;
        hmenu = GetASubMenu(hwndFrame, menuReport);
        switch (vprptCur->irpt) {
        case rptFleets:
            idm = 2303;
            break;
        case rptEnemyFleets:
            idm = 2304;
            break;
        case rptPlanets:
            idm = 2301;
            break;
        case rptBattles:
            idm = 2305;
        }
        CheckMenuItem(hmenu, idm, MF_UNCHECKED);
        vprptCur = 0;
        if (gd.fTutorial) {
            AdvanceTutor();
        }
        break;
    case WM_COMMAND:
        if (GET_WM_COMMAND_ID(wParam, lParam) == IDCANCEL) {
            DestroyWindow(hwnd);
            return 1;
        }
        break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void SetHScrollBar() {
    uint16_t swp;
    int16_t  dy;
    int16_t  ccolSkipped;
    int16_t  ccolHidden;
    int16_t  i;
    int16_t  ibit;
    int16_t  xRight;
    int16_t  dx;
    int16_t  xTitle;
    RECT     rc;

    GetClientRect(hwndReportDlg, &rc);
    dx = GetSystemMetrics(SM_CXVSCROLL);
    xTitle = vprptCur->rgbdx[0] * 2 + 2;
    xRight = rc.right - xTitle - 2 - dx;
    ccolSkipped = 0;
    ccolHidden = 0;
    i = vprptCur->cFields - 1;
    ibit = 1 << vprptCur->cFields;
    while (i > 0) {
        if (ibit & vprptCur->grbitVisible) {
            if (i < vprptCur->cFieldFirst) {
                ccolSkipped++;
            }
            xRight -= vprptCur->rgbdx[i] * 2;
            if (xRight < 0) {
                ccolHidden++;
            }
        }
        i--;
        ibit >>= 1;
    }
    vprptCur->cColScroll = 0;
    if (ccolHidden == 0) {
        swp = SWP_NOZORDER | SWP_HIDEWINDOW;
        vprptCur->cFieldFirst = 1;
        SetScrollPos(vprptCur->hwndHScroll, SB_CTL, 0, FALSE);
    } else {
        swp = SWP_NOZORDER | SWP_SHOWWINDOW;
        if (ccolSkipped > ccolHidden) {
            ccolSkipped = 0;
            vprptCur->cFieldFirst = 1;
        }
        SetScrollPos(vprptCur->hwndHScroll, SB_CTL, ccolSkipped, FALSE);
        SetScrollRange(vprptCur->hwndHScroll, SB_CTL, 0, ccolHidden, TRUE);
        vprptCur->cColScroll = ccolHidden;
    }
    dy = GetSystemMetrics(SM_CYHSCROLL);
    SetWindowPos(vprptCur->hwndHScroll, NULL, xTitle, dyArial8 + 6 + (dyArial8 + 4) * vprptCur->cRowsVis + 1, rc.right - dx - xTitle, dy, swp);
    return;
}

void DrawReport(HWND hwnd, HDC hdc, RECT *prc) {
    char    szTit[40];
    int16_t irowLast;
    int16_t j;
    int16_t i;
    int16_t yRow;
    int16_t ibit;
    int16_t dx;
    int16_t xCol;
    RECT    rc;

    ibit = 1;
    xCol = 2;
    yRow = 2;
    SelectObject(hdc, rghfontArial8[1]);
    SetBkMode(hdc, TRANSPARENT);
    if (prc->top > dyArial8 + 4 + yRow)
        goto NoHdrDraw;

    i = 0;
    while (i < vprptCur->cFields) {
        if ((ibit & vprptCur->grbitVisible) && (i == 0 || i >= vprptCur->cFieldFirst)) {
            dx = DxReportColHdr(vprptCur->irpt, i, szTit, hdc);
            vprptCur->rgbdx[i] = dx / 2;
            SetRect(&rc, xCol, yRow, xCol + dx - 1, dyArial8 + 4 + yRow);
            if (gd.fRptSafeDraw) {
                FillRect(hdc, &rc, hbrButtonFace);
            }
            if (i == 0) {
                TextOut(hdc, rc.left + 3, rc.top + 2, szTit, strlen(szTit));
            } else {
                CtrTextOut(hdc, (int16_t)(rc.right - rc.left) / 2 + rc.left, rc.top + 2, szTit, 0);
            }
            _Draw3dFrame(hdc, &rc, 0);
            xCol += dx;
        }
        i++;
        ibit *= 2;
    }

NoHdrDraw:
    irowLast = vprptCur->irowFirst + vprptCur->cRowsVis;
    if (irowLast > vprptCur->cRows) {
        irowLast = vprptCur->cRows;
    }
    for (i = vprptCur->irowFirst; i < irowLast; i++) {
        yRow += dyArial8 + 4;
        xCol = 2;
        if (yRow >= prc->top && yRow <= prc->bottom) {
            SelectObject(hdc, hbrButtonShadow);
            PatBlt(hdc, xCol, yRow, 1, dyArial8 + 4, PATCOPY);
        }
        j = 0;
        ibit = 1;
        while (j < vprptCur->cFields) {
            if ((ibit & vprptCur->grbitVisible) && (j == 0 || j >= vprptCur->cFieldFirst)) {
                dx = vprptCur->rgbdx[j] * 2;
                if (yRow >= prc->top - (dyArial8 + 4) && yRow <= prc->bottom) {
                    SelectObject(hdc, hbrButtonShadow);
                    PatBlt(hdc, xCol + dx - 1, yRow, 1, dyArial8 + 4, PATCOPY);
                    PatBlt(hdc, xCol, dyArial8 + 4 + yRow, dx, 1, PATCOPY);
                    SetRect(&rc, xCol + 2, yRow + 2, xCol + dx - 3, dyArial8 + 4 + yRow - 1);
                    if (gd.fRptSafeDraw) {
                        FillRect(hdc, &rc, hbrButtonFace);
                    }
                    DrawReportItem(hdc, &rc, vprptCur->irpt, i, j);
                }
                xCol += dx;
            }
            j++;
            ibit *= 2;
        }
    }
    return;
}

INT_PTR CALLBACK ScoreXDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    int16_t     i;
    RECT        rc;
    HDC         hdc;
    PAINTSTRUCT ps;
    POINT16     pt;
    char        szT[40];
    int16_t     cchHistory;
    char       *rgszScan[20];
    int16_t     c;
    int32_t     rgid[12];
    char       *psz;
    int16_t     iSel;
    int16_t     cch;

    switch (message) {
    case WM_INITDIALOG:
        InitScoreDlg(hwnd, gd.fScoreVictory);
        fInScoreDialog = TRUE;
        StickyDlgPos(hwnd, &ptStickyScoreXDlg, TRUE);
        hwndScoreXDlg = hwnd;
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
        if (gd.fScoreVictory == 2) {
            DrawHistoryReport(hdc);
        } else if (gd.fScoreVictory) {
            DrawVCReport(hdc);
        } else {
            DrawScoreReport(hdc);
        }
        EndPaint(hwnd, &ps);
        return 1;
    case WM_SETCURSOR:
        if (gd.fScoreVictory != 2) {
            return 0;
        }
        GetCursorPos16(&pt);
        ScreenToClient16(hwnd, &pt);
        if (pt.y >= dyArial10 + dyArial8 - 2 || pt.y <= 2) {
            return 0;
        }
        SetCursor(hcurHand);
        return 1;
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
        c = 0;
        if (gd.fScoreVictory != 2 || HIWORD(lParam) <= 2 || HIWORD(lParam) >= (uint16_t)(dyArial10 + dyArial8 - 2)) {
            return 0;
        }
        cchHistory = CchGetString(idsHistory, szT);
        for (i = 0; i < 8; i++) {
            strcpy(&szWork[i * 40], szT);
            psz = &szWork[i * 40 + cchHistory];
            cch = CchGetString(i + 435, psz);
            psz[cch - 1] = 0;
            rgid[c] = (uint32_t)(gd.iCurGraph == i);
            rgszScan[c++] = &szWork[i * 40];
        }
        iSel = PopupMenu(hwnd, LOWORD(lParam), HIWORD(lParam), c, rgid, rgszScan, -2, FALSE);
        if (iSel == -1) {
            return 0;
        }
        gd.iCurGraph = iSel;
        gd.fChgReports = TRUE;
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDCANCEL:
            StickyDlgPos(hwnd, &ptStickyScoreXDlg, FALSE);
            EndDialog(hwnd, i);
            fInScoreDialog = FALSE;
            hwndScoreXDlg = 0;
            if (gd.fTutorial) {
                AdvanceTutor();
            }
            return 1;
        case IDC_HELP:
            WinHelp(hwnd, szHelpFile, HELP_CONTEXT, idhScoreSheet);
            return 1;
        case IDC_SCORE_SWITCH:
            gd.fScoreVictory = (uint32_t)(gd.fScoreVictory + 1) % 3;
            InvalidateRect(hwnd, NULL, TRUE);
            InitScoreDlg(hwnd, gd.fScoreVictory);
            if (gd.fTutorial) {
                AdvanceTutor();
            }
        }
        /* fallthrough */
    default:
        return 0;
    }
}

void InitScoreDlg(HWND hwnd, int16_t fVictory) {
    HDC     hdc;
    int16_t dxDig;
    int16_t dy;
    int16_t dyFrame;
    int16_t dxFrame;
    RECT    rcWindow;
    char   *psz;
    int16_t dx;
    RECT    rc;

    hdc = GetDC(hwnd);
    SelectObject(hdc, rghfontArial8[1]);
    dxDig = LOWORD(GetTextExtent(hdc, "9", 1));
    if (fVictory == 2) {
        dx = 600;
        dy = 400;
    } else if (fVictory) {
        psz = PszGetCompressedString(idsExceedsSecondPlaceScore);
        vdxScoreX = (uint32_t)(LOWORD(GetTextExtent(hdc, psz, strlen(psz))) * 3) / 2 + 6 * dxDig;
        dx = (int16_t)((4 <= game.cPlayer ? game.cPlayer : 4) * dyArial8 * 3) / 2 + vdxScoreX + 8;
        dy = (int16_t)(11 * dyArial8 * 3) / 2 + 88;
    } else {
        psz = PszGetCompressedString(idsUnarmedShips2);
        vdxScoreX = LOWORD(GetTextExtent(hdc, psz, strlen(psz))) + 8;
        dx = (4 <= game.cPlayer ? game.cPlayer : 4) * dxDig * 5 + vdxScoreX + 8;
        dy = (int16_t)(11 * dyArial8 * 3) / 2 + 88;
    }
    ReleaseDC(hwnd, hdc);
    GetWindowRect(hwnd, &rcWindow);
    GetClientRect(hwnd, &rc);
    dxFrame = rcWindow.right - rcWindow.left - rc.right;
    dyFrame = rcWindow.bottom - rcWindow.top - rc.bottom;
    SetWindowPos(hwnd, NULL, 0, 0, dxFrame + dx, dyFrame + dy, SWP_NOMOVE | SWP_NOZORDER);
    GetWindowRect(GetDlgItem(hwnd, IDCANCEL), &rc);
    MapWindowPoints(NULL, hwnd, (POINT *)&rc, 2);
    OffsetRect(&rc, 0, dy - 4 - rc.bottom);
    dx = (int16_t)(dx - (rc.right - rc.left) * 3) / 4;
    SetWindowPos(GetDlgItem(hwnd, IDC_SCORE_SWITCH), NULL, dx, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    SetWindowPos(GetDlgItem(hwnd, IDCANCEL), NULL, dx * 2 + (rc.right - rc.left), rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    SetWindowPos(GetDlgItem(hwnd, IDC_HELP), NULL, 3 * dx + (rc.right - rc.left) * 2, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    SetWindowText(hwnd, PszGetCompressedString(fVictory + 1210));
    return;
}

void DrawVCReport(HDC hdc) {
    int16_t  grbitVC;
    int16_t  xStart;
    int16_t  dxDig;
    int16_t  yTop;
    POINT16  pt;
    int16_t  cCurSav;
    StringId ids;
    COLORREF cr;
    int16_t  cCur;
    HDC      hdcMem;
    int16_t  j;
    int16_t  i;
    int16_t  iPass;
    char    *psz;
    HBITMAP  hbmpSav;
    int16_t  cch;
    int16_t  xLeft;
    int32_t  l;
    StringId idsT;
    int16_t  vcVal;

    yTop = 88;
    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, rghfontArial8[4]);
    xLeft = vdxScoreX;
    for (i = 0; i < game.cPlayer; i++) {
        psz = PszPlayerName(i, TRUE, TRUE, TRUE, 0, NULL);
        cch = strlen(psz);
        l = GetTextExtent(hdc, psz, cch);
        dxDig = LOWORD((int32_t)((long double)(uint32_t)LOWORD(l) / 1.4142));
        if (rgplr[i].fInclude && rgplr[i].fDead) {
            cr = 8355711;
        } else if (vlprgScoreX[i].fWinner) {
            cr = 16711680;
        } else {
            cr = 0;
        }
        SetTextColor(hdc, cr);
        TextOut(hdc, xLeft - dxDig + (int16_t)(3 * dyArial8) / 2, yTop - dxDig - dyArial8 / 2 - 4, psz, cch);
        xLeft += (int16_t)(3 * dyArial8) / 2;
    }
    SelectObject(hdc, rghfontArial8[1]);
    SelectObject(hdc, hbrButtonShadow);
    dxDig = LOWORD(GetTextExtent(hdc, "9", 1));
    ids = idsOwns;
    cCur = 0;
    hdcMem = CreateCompatibleDC(hdc);
    hbmpSav = SelectObject(hdcMem, hbmpMono);
    SetTextColor(hdc, 0);
    SetBkColor(hdc, 0xffffff);
    xLeft = vdxScoreX + 4;
    pt.x = (int16_t)((int16_t)(3 * dyArial8) / 2 - 12) / 2;
    pt.y = (int16_t)((int16_t)(3 * dyArial8) / 2 - 11) / 2 - dyArial8 / 4;
    for (i = 0; i < game.cPlayer; i++) {
        grbitVC = vlprgScoreX[i].grbitVC;
        for (j = 0; j < 7; j++, grbitVC >>= 1) {
            if (grbitVC & 1) {
                BitBlt(hdc, xLeft + pt.x, yTop + pt.y + (int16_t)(j * dyArial8 * 3) / 2, 14, 12, hdcMem, 0, 0, SRCAND);
                if ((rgplr[i].fInclude && rgplr[i].fDead) || GetVCCheck(&game, (j > 1) + j) == 0) {
                    SetTextColor(hdc, crButtonShadow);
                LOrIt:
                    SetBkColor(hdc, 0);
                    BitBlt(hdc, xLeft + pt.x, yTop + pt.y + (int16_t)(j * dyArial8 * 3) / 2, 14, 12, hdcMem, 0, 0, SRCPAINT);
                    SetTextColor(hdc, 0);
                    SetBkColor(hdc, 0xffffff);
                } else if (vlprgScoreX[i].fWinner) {
                    SetTextColor(hdc, 16711680);
                    goto LOrIt;
                }
            }
        }
        PatBlt(hdc, xLeft, yTop - dyArial8 / 4, 1, (int16_t)(7 * dyArial8 * 3) / 2 + 1, PATCOPY);
        xLeft += (int16_t)(3 * dyArial8) / 2;
    }
    PatBlt(hdc, xLeft, yTop - dyArial8 / 4, 1, (int16_t)(7 * dyArial8 * 3) / 2 + 1, PATCOPY);
    SelectObject(hdcMem, hbmpSav);
    DeleteDC(hdcMem);
    for (i = 0; i < 9; i++) {
        for (iPass = 0; iPass < 2; iPass++) {
            if (iPass == 0) {
                cCurSav = cCur;
                xStart = 0;
            } else {
                ids -= 3;
                cCur = cCurSav;
                if (i < 7) {
                    xStart = vdxScoreX - xLeft;
                } else {
                    xStart = 8;
                }
            }
            xLeft = xStart;
            if (i <= 7 && iPass == 1) {
                PatBlt(hdc, vdxScoreX + 4, yTop - dyArial8 / 4, (int16_t)(dyArial8 * game.cPlayer * 3) / 2, 1, PATCOPY);
                if (i == 7) {
                    yTop += dyArial8 / 2;
                }
            }
            cch = CchGetString(ids++, szWork);
            if (iPass == 1) {
                SetTextColor(hdc, i >= 7 || GetVCCheck(&game, cCur) != 0 ? 0 : 8355711);
                TextOut(hdc, xLeft, yTop, szWork, cch);
            }
            xLeft += LOWORD(GetTextExtent(hdc, szWork, cch));
            for (j = 0; j < 2; j++) {
                if (j == 1 && i != 1) {
                    ids++;
                    break;
                }
                vcVal = GetVCVal(&game, cCur, FALSE);
                if (i == 0) {
                    idsT = idsPlanets3;
                    vcVal = LOWORD((int32_t)(vcVal * game.cPlanMax) / 100);
                } else {
                    idsT = ids;
                }
                cch = _wsprintf(szWork, PCTD, vcVal);
                if (i == 3) {
                    strcat(szWork, "%");
                    cch++;
                }
                if (iPass == 1) {
                    TextOut(hdc, xLeft, yTop, szWork, cch);
                }
                xLeft += LOWORD(GetTextExtent(hdc, szWork, cch));
                if (i != 2 && i != 3) {
                    xLeft += 4;
                }
                cch = CchGetString(idsT, szWork);
                if (iPass == 1) {
                    TextOut(hdc, xLeft, yTop, szWork, cch);
                }
                xLeft += LOWORD(GetTextExtent(hdc, szWork, cch));
                cCur++;
                ids++;
            }
            if (iPass == 1) {
                yTop += (int16_t)(3 * dyArial8) / 2;
            }
        }
    }
    return;
}

void DrawScoreReport(HDC hdc) {
    int16_t  dxDig;
    int16_t  yTop;
    POINT16  pt;
    int16_t  dx45;
    StringId ids;
    COLORREF cr;
    int32_t  lMax;
    int16_t  j;
    int16_t  i;
    int16_t  iPass;
    char    *psz;
    int32_t  lVal;
    int16_t  cch;
    int16_t  xLeft;
    int32_t  l;

    yTop = 88;
    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, rghfontArial8[1]);
    dxDig = LOWORD(GetTextExtent(hdc, "9", 1));
    SelectObject(hdc, rghfontArial8[4]);
    xLeft = (int16_t)(5 * dxDig - (int16_t)(3 * dyArial8) / 2) / 2 + vdxScoreX;
    for (i = 0; i < game.cPlayer; i++) {
        psz = PszPlayerName(i, TRUE, TRUE, TRUE, 0, NULL);
        cch = strlen(psz);
        l = GetTextExtent(hdc, psz, cch);
        dx45 = LOWORD((int32_t)((long double)(uint32_t)LOWORD(l) / 1.4142));
        if (rgplr[i].fInclude && rgplr[i].fDead) {
            cr = 8355711;
        } else if (vlprgScoreX[i].fWinner) {
            cr = 16711680;
        } else {
            cr = 0;
        }
        SetTextColor(hdc, cr);
        TextOut(hdc, xLeft - dx45 + (int16_t)(3 * dyArial8) / 2, yTop - dx45 - dyArial8 / 2 - 4, psz, cch);
        xLeft += 5 * dxDig;
    }
    SelectObject(hdc, rghfontArial8[1]);
    SelectObject(hdc, hbrButtonShadow);
    ids = idsPlanets;
    xLeft = vdxScoreX + 4;
    pt.x = (int16_t)((int16_t)(3 * dyArial8) / 2 - 12) / 2;
    pt.y = (int16_t)((int16_t)(3 * dyArial8) / 2 - 11) / 2 - dyArial8 / 4;
    for (i = 0; i <= game.cPlayer; i++) {
        PatBlt(hdc, xLeft, yTop - dyArial8 / 4, 1, (int16_t)(9 * dyArial8 * 3) / 2 + 1, PATCOPY);
        xLeft += 5 * dxDig;
    }
    SetTextColor(hdc, 0);
    for (i = 0; i < 9; i++) {
        PatBlt(hdc, vdxScoreX + 4, yTop - dyArial8 / 4, game.cPlayer * dxDig * 5, 1, PATCOPY);
        cch = CchGetString(ids++, szWork);
        SetTextColor(hdc, 0);
        RightTextOut(hdc, vdxScoreX, yTop, szWork, cch, 0);
        xLeft = 5 * dxDig + vdxScoreX + 2;
        lMax = 0;
        for (iPass = 0; iPass < 2; iPass++) {
            for (j = 0; j < game.cPlayer; j++) {
                if (!vlprgScoreX[j].fValid) {
                    lVal = -1;
                } else {
                    switch (i) {
                    case 0:
                        lVal = vlprgScoreX[j].score.cPlanet;
                        break;
                    case 1:
                        lVal = vlprgScoreX[j].score.cStarbase;
                        break;
                    case 2:
                    case 3:
                    case 4:
                        lVal = (int32_t)((uint32_t)(vlprgScoreX[j].score.rgcsh[i - 2] & 0x1fff) << (vlprgScoreX[j].score.rgcsh[i - 2] >> 0xd << 1));
                        break;
                    case 5:
                        lVal = vlprgScoreX[j].score.cTechLevels;
                        break;
                    case 6:
                        lVal = vlprgScoreX[j].score.cResources;
                        break;
                    case 7:
                        lVal = vlprgScoreX[j].score.lScore;
                        break;
                    case 8:
                        lVal = (int16_t)vlprgScoreX[j].turn;
                    }
                }
                if (iPass == 0) {
                    if (lVal > lMax) {
                        lMax = lVal;
                    }
                } else if (lVal >= 0 && (!rgplr[j].fInclude || !rgplr[j].fDead)) {
                    if (i == 8) {
                        SetTextColor(hdc, lVal == 1 ? 16711680 : 0);
                    } else {
                        SetTextColor(hdc, lVal == lMax ? 16711680 : 0);
                    }
                    psz = PszFromLongK(lVal, &cch);
                    RightTextOut(hdc, xLeft, yTop, psz, cch, 0);
                }
                if (iPass == 1) {
                    xLeft += 5 * dxDig;
                }
            }
        }
        yTop += (int16_t)(3 * dyArial8) / 2;
    }
    PatBlt(hdc, vdxScoreX + 4, yTop - dyArial8 / 4, game.cPlayer * dxDig * 5, 1, PATCOPY);
    return;
}

void DrawHistoryReport(HDC hdc) {
    char     szT[100];
    RECT     rcChart;
    uint16_t dYear;
    POINT16  pt;
    int16_t  dy;
    int32_t  cYears;
    int32_t  cCur;
    uint16_t iYearBase;
    int16_t  j;
    int16_t  i;
    int16_t  yCur;
    int16_t  cDrawn;
    char    *psz;
    int16_t  dx;
    int32_t  cScaleMax;
    int16_t  xCur;
    int32_t  cInc;
    int16_t  cch;
    RECT     rcDiamond;
    RECT     rc;
    HPEN     hpenSav;
    HPEN     hpen;
    SCOREX  *lpsx;

    SetBkMode(hdc, TRANSPARENT);
    GetClientRect(hwndScoreXDlg, &rcChart);
    ExpandRc(&rcChart, -48, 0);
    rcChart.top = dyArial10 + dyArial8;
    rcChart.bottom -= dyArial8 * 4;
    rc = rcChart;
    rc.top = dyArial8 / 2;
    rc.bottom = rc.top + dyArial10;
    cch = CchGetString(idsHistory, szT);
    psz = &szT[cch];
    cch = CchGetString(gd.iCurGraph + 435, psz);
    psz[cch - 1] = 0;
    SelectObject(hdc, rghfontArial10[1]);
    SetTextColor(hdc, crButtonText);
    RcCtrTextOut(hdc, &rc, szT, 0);
    SetRect(&rcDiamond, 15, rc.top, dyArial10 + 15, rc.top + dyArial10);
    DrawDiamond(hdc, &rcDiamond, hbrBBlue);
    dx = rcChart.right - rcChart.left;
    dy = rcChart.bottom - rcChart.top;
    PatBlt(hdc, rcChart.left, rcChart.top, dx, dy, BLACKNESS);
    SelectObject(hdc, hbrButtonHilite);
    PatBlt(hdc, rcChart.left - 1, rcChart.bottom, dx + 3, 1, PATCOPY);
    PatBlt(hdc, rcChart.left - 2, rcChart.bottom + 1, dx + 4, 1, PATCOPY);
    PatBlt(hdc, rcChart.right, rcChart.top - 1, 1, dy + 3, PATCOPY);
    PatBlt(hdc, rcChart.right + 1, rcChart.top - 2, 1, dy + 4, PATCOPY);
    SelectObject(hdc, hbrButtonShadow);
    PatBlt(hdc, rcChart.left - 2, rcChart.top - 2, dx + 3, 1, PATCOPY);
    PatBlt(hdc, rcChart.left - 2, rcChart.top - 1, dx + 2, 1, PATCOPY);
    PatBlt(hdc, rcChart.left - 2, rcChart.top - 2, 1, dy + 3, PATCOPY);
    PatBlt(hdc, rcChart.left - 1, rcChart.top - 2, 1, dy + 2, PATCOPY);
    dx -= 4;
    dy -= 4;
    ExpandRc(&rcChart, -2, -2);
    if (game.turn <= 100) {
        iYearBase = 0;
    } else {
        iYearBase = game.turn - 100;
    }
    cYears = (uint32_t)((int32_t)(game.turn + 4) / 5 * 5);
    if (cYears > 100) {
        cYears = 100;
    } else if (cYears > 50) {
        cYears = (uint32_t)((int32_t)((cYears + 5) / 10) * 10);
    }
    xCur = rcChart.left;
    SelectObject(hdc, rghfontArial8[1]);
    if (cYears <= 50) {
        j = 5;
    } else {
        j = 10;
    }
    cDrawn = (int16_t)LOWORD(cYears) / j;
    if (cDrawn > 0) {
        for (i = 0; i <= cDrawn; i++) {
            xCur = (int16_t)(dx * i) / cDrawn + rcChart.left;
            cch = _wsprintf(szT, PCTD, iYearBase + 2400 + i * j);
            CtrTextOut(hdc, xCur, rcChart.bottom + 6, szT, cch);
            if (i > 0 && i < cDrawn) {
                PatBlt(hdc, xCur, rcChart.top - 2, 1, dy + 4, PATCOPY);
            }
        }
    }
    cScaleMax = -1;
    for (i = 0; i < game.cPlayer; i++) {
        if (rgsxPlr[i]) {
            for (j = 0; j < rgcsxPlr[i]; j++) {
                cCur = LFetchScoreXVal(rgsxPlr[i] + j, gd.iCurGraph);
                if (cCur > cScaleMax) {
                    cScaleMax = cCur;
                }
            }
        }
    }
    if (cScaleMax >= 0) {
        if (cScaleMax < 5) {
            cScaleMax = 5;
        }
        if (cScaleMax <= 12) {
            cInc = 1;
        } else if (cScaleMax <= 25) {
            cInc = 2;
        } else if (cScaleMax <= 60) {
            cInc = 5;
        } else if (cScaleMax <= 120) {
            cInc = 10;
        } else if (cScaleMax <= 300) {
            cInc = 25;
        } else if (cScaleMax <= 600) {
            cInc = 50;
        } else if (cScaleMax <= 1200) {
            cInc = 100;
        } else if (cScaleMax <= 6000) {
            cInc = 500;
        } else if (cScaleMax <= 12000) {
            cInc = 1000;
        } else {
            cInc = (uint32_t)((int32_t)(cScaleMax / 12) / 500 * 500);
        }
        xCur = rcChart.left - 6;
        for (cCur = cInc; cCur < cScaleMax; cCur += cInc) {
            yCur = rcChart.bottom - LOWORD((int32_t)((int32_t)(dy * cCur) / cScaleMax));
            if (yCur < dyArial8 / 2 + rcChart.top)
                break;
            cch = _wsprintf(szWork, PCTLD, cCur);
            RightTextOut(hdc, xCur, yCur - dyArial8 / 2, szWork, cch, 0);
            PatBlt(hdc, rcChart.left - 2, yCur, dx + 4, 1, PATCOPY);
        }
        xCur = rcChart.left + 6;
        yCur = rcChart.top + 6;
        for (i = 0; i < game.cPlayer; i++) {
            if (rgsxPlr[i]) {
                psz = PszPlayerName(i, TRUE, TRUE, FALSE, 0, NULL);
                SetTextColor(hdc, i == idPlayer ? 0xffffff : rgcrPlrHistory[i]);
                TextOut(hdc, xCur, yCur, psz, strlen(psz));
                yCur += dyArial8;
            }
        }
        for (i = 0; i < game.cPlayer; i++) {
            if (rgsxPlr[i]) {
                cDrawn = 0;
                hpen = CreatePen(0, 1, i == idPlayer ? 0xffffff : rgcrPlrHistory[i]);
                hpenSav = SelectObject(hdc, hpen);
                for (j = 0; j < rgcsxPlr[i]; j++) {
                    lpsx = rgsxPlr[i] + j;
                    if (lpsx->turn >= iYearBase && lpsx->turn <= (uint16_t)(LOWORD(cYears) + iYearBase)) {
                        dYear = lpsx->turn - iYearBase;
                        cCur = LFetchScoreXVal(lpsx, gd.iCurGraph);
                        pt.x = LOWORD((int32_t)((int32_t)((uint32_t)dYear * dx) / cYears)) + rcChart.left;
                        pt.y = rcChart.bottom - LOWORD((int32_t)((int32_t)(cCur * dy) / cScaleMax));
                        if (cDrawn == 0) {
                            MoveTo(hdc, pt.x, pt.y);
                        } else {
                            LineTo(hdc, pt.x, pt.y);
                        }
                        cDrawn++;
                    }
                }
                if (cDrawn == 1) {
                    SetPixel(hdc, pt.x, pt.y, i == idPlayer ? 0xffffff : rgcrPlrHistory[i]);
                }
                SelectObject(hdc, hpenSav);
                DeleteObject(hpen);
            }
        }
    }
    return;
}

int32_t LFetchScoreXVal(SCOREX *lpsx, int16_t iVal) {
    switch (iVal) {
    case 0:
        return lpsx->score.cPlanet;
    case 1:
        return lpsx->score.cStarbase;
    case 2:
    case 3:
    case 4:
        return (int32_t)((uint32_t)(lpsx->score.rgcsh[iVal - 2] & 0x1fff) << (lpsx->score.rgcsh[iVal - 2] >> 0xd << 1));
    case 5:
        return lpsx->score.cTechLevels;
    case 6:
        return lpsx->score.cResources;
    case 7:
        return lpsx->score.lScore;
    default:
        return 0;
    }
}

int16_t DxReportColHdr(ReportType irpt, int16_t iCol, char *psz, HDC hdc) {
    char     szT[40];
    StringId ids;
    int16_t  dxDigit;
    int16_t  dx;
    int16_t  cch;
    int16_t  dx2;

    szT[0] = '8';
    dxDigit = LOWORD(GetTextExtent(hdc, szT, 1));
    switch (irpt) {
    default:
        *psz = 0;
        return 0;
    case rptPlanets:
        cch = CchGetString(iCol + 1113, psz);
        dx = LOWORD(GetTextExtent(hdc, psz, cch));
        if ((uint16_t)iCol > colPlanetRoutingDest)
            break;
        switch (iCol) {
        case colPlanetStarbase:
        case colPlanetDriverDest:
        case colPlanetRoutingDest:
            dx2 = 15 * dxDigit;
            goto DxChk;
        case colPlanetName:
            dx *= 2;
            break;
        case colPlanetProduction:
            dx = 3 * dx + 20;
            break;
        case colPlanetValue:
            ids = idsN100100;
        ChkAltString:
            cch = CchGetString(ids, szT);
            dx2 = LOWORD(GetTextExtent(hdc, szT, cch));
        DxChk:
            if (dx2 <= dx)
                break;
            dx = dx2;
            break;

        case colPlanetResources:
            ids = idsN10001000;
            goto ChkAltString;
        case colPlanetMines:
        case colPlanetFactories:
            dx2 = 5 * dxDigit;
            goto DxChk;
        case colPlanetMiningRate:
        case colPlanetMinConc:
            dx2 = 11 * dxDigit;
            goto DxChk;
        case colPlanetMinerals:
            dx2 = 14 * dxDigit;
            goto DxChk;
        case colPlanetCapacity:
            dx2 = dxDigit * 4 + 2;
            goto DxChk;
        }
        break;
    case rptFleets:
        cch = CchGetString(iCol + 1138, psz);
        dx = LOWORD(GetTextExtent(hdc, psz, cch));
        if ((uint16_t)iCol > colFleetMass)
            break;
        switch (iCol) {
        case colFleetLocation:
            dx = dx * 2 + dx / 2;
            break;
        case colFleetName:
        case colFleetId:
        case colFleetDestination:
        case colFleetComposition:
        case colFleetBattlePlan:
        case colFleetMass:
            dx *= 2;
            break;
        case colFleetEta:
            dx = dx * 2 - dxDigit;
            break;
        case colFleetTask:
            dx = dx * 4 + dx / 2;
            break;
        case colFleetFuel:
            dx2 = 5 * dxDigit;
            if (dx2 <= dx)
                break;
            dx = dx2;
            break;
        case colFleetCargo:
            dx2 = 19 * dxDigit;
            if (dx2 > dx) {
                dx = dx2;
            }
        }
        break;
    case rptEnemyFleets:
        cch = CchGetString(iCol + 1150, psz);
        dx = LOWORD(GetTextExtent(hdc, psz, cch));
        switch (iCol) {
        case colEnemyFleetLocation:
            dx = dx * 2 + dx / 2;
            break;
        case colEnemyFleetName:
            dx = 3 * dx;
            break;
        case colEnemyFleetId:
        case colEnemyFleetComposition:
            dx *= 2;
        }
        break;
    case rptBattles:
        cch = CchGetString(iCol + 1162, psz);
        dx = LOWORD(GetTextExtent(hdc, psz, cch));
        if (iCol == colBattleLocation) {
            dx = dx * 2 + dx / 2;
        }
    }
    dx += 5;
    return (int16_t)(dx + 1) / 2 * 2;
}

void DrawReportItem(HDC hdc, RECT *prc, ReportType irpt, int16_t irow, int16_t icol) {
    BTLDATA *lpbd;
    char     szT[100];
    char     chT;
    char    *lpsz;
    PLANET  *lppl;
    int16_t  j;
    int16_t  i;
    FLEET   *lpfl;
    int16_t  dx;
    char    *psz;
    int16_t  xCur;
    int16_t  cch;
    int32_t  l;
    HBRUSH   hbr;
    RECT     rc;
    float    pct;
    int32_t  rgl[4];
    int16_t  iItem;
    PLANET   pl;
    int16_t  fEnough;

    szT[0] = '8';
    dx = LOWORD(GetTextExtent(hdc, szT, 1));
    switch (irpt) {
    case rptPlanets:
        lppl = lpPlanets + vlprgidPlanet[irow];
        SelectObject(hdc, rghfontArial8[1]);
        SetTextColor(hdc, 0);
        SetBkMode(hdc, TRANSPARENT);
        if ((uint16_t)icol > colPlanetRoutingDest)
            break;
        switch (icol) {
        case colPlanetStarbase:
            if (!lppl->fStarbase) {
                lpsz = szDblDash;
            } else {
                lpsz = rglpshdefSB[idPlayer][lppl->isb].hul.szClass;
            }
            ExtTextOut(hdc, prc->left, prc->top, ETO_CLIPPED, prc, lpsz, fstrlen(lpsz), NULL);
            break;
        case colPlanetRoutingDest:
            if (lppl->idRoute == 0) {
                psz = szDblDash;
            } else {
                psz = PszGetPlanetName(lppl->idRoute - 1);
            }
            ExtTextOut(hdc, prc->left, prc->top, ETO_CLIPPED, prc, psz, strlen(psz), NULL);
            break;
        case colPlanetDriverDest:
            if (lppl->idFling == 0) {
                psz = szDblDash;
            } else {
                psz = PszGetPlanetName(lppl->idFling - 1);
            }
            ExtTextOut(hdc, prc->left, prc->top, ETO_CLIPPED, prc, psz, strlen(psz), NULL);
            break;
        case colPlanetName:
            if (sel.grobj == grobjPlanet && sel.pl.id == lppl->id) {
                SetTextColor(hdc, 127);
            }
            psz = PszGetPlanetName(lppl->id);
            ExtTextOut(hdc, prc->left, prc->top, ETO_CLIPPED, prc, psz, strlen(psz), NULL);
            if (!lppl->fStarbase)
                break;
            rc = *prc;
            dx = (int16_t)(rc.bottom - rc.top) / 3 - 1;
            rc.left = rc.right - dx;
            if (LphuldefFromId(rglpshdefSB[idPlayer][lppl->isb].hul.ihuldef)->hul.wtCargoMax != 0) {
                hbr = hbrYellow;
            } else {
                hbr = hbrBlue;
            }
            rc.top = prc->top;
            rc.bottom = rc.top + dx;
            FillRect(hdc, &rc, hbr);
            if (IWarpMAFromLppl(lppl, NULL) > 0) {
                rc.top = prc->top + dx + 1;
                rc.bottom = rc.top + dx;
                FillRect(hdc, &rc, hbrPurple);
            }
            if (IStargateFromLppl(lppl) == -1)
                break;
            rc.top = (dx + 1) * 2 + prc->top;
            rc.bottom = rc.top + dx;
            FillRect(hdc, &rc, hbrGreen);
            break;
        case colPlanetPopulation:
            if (CalcPlanetMaxPop(lppl->id, lppl->iPlayer) < lppl->rgwtMin[3]) {
                SetTextColor(hdc, 0xff);
            }
            cch = CommaFormatLong(szT, (uint32_t)(lppl->rgwtMin[3] * 100));
            RightTextOut(hdc, prc->right, prc->top, szT, cch, 0);
            break;
        case colPlanetCapacity:
            cch = _wsprintf(szT, PCTDPCTPCT, PctPlanetCapacity(lppl));
            RightTextOut(hdc, prc->right, prc->top, szT, cch, 0);
            break;
        case colPlanetDefense:
            i = lppl->cDefenses;
            j = CMaxOperableDefenses(lppl, idPlayer, FALSE);
            if (i > 0) {
                CalcPctSurvive(lppl, &pct, NULL);
                pct = (float)((long double)1.0 - pct);
                cch = _wsprintf(szT, PCTDXPCTDPCTPCT, LOWORD((int32_t)((long double)pct * 100)),
                                LOWORD((int32_t)((pct - (long double)(int16_t)LOWORD((int32_t)((long double)pct * 100)) / 100.0) * 10000)));
                goto DrawPlusDef;
            }
            szT[2] = '-';
            szT[1] = '-';
            szT[0] = '-';
            cch = 3;
            goto DrawPlusDef;
        case colPlanetFactories:
            i = lppl->cFactories;
            j = CMaxOperableFactories(lppl, idPlayer, FALSE);
            goto DrawMineFact;
        case colPlanetMines:
            i = lppl->cMines;
            j = CMaxOperableMines(lppl, idPlayer, FALSE);
        DrawMineFact:
            cch = CommaFormatLong(szT, i);
        DrawPlusDef:
            if (i >= j) {
                SetTextColor(hdc, i == j ? 32512 : 0xff);
            }
            RightTextOut(hdc, prc->right, prc->top, szT, cch, 0);
            break;
        case colPlanetValue:
            cch = CchGetString(idsN100, szT);
            dx = LOWORD(GetTextExtent(hdc, szT, cch));
            i = PctPlanetDesirability(lppl, idPlayer);
            cch = _wsprintf(szT, PCTDPCTPCT, i);
            if (i <= 10) {
                SetTextColor(hdc, i >= 0 ? 32639 : 0xff);
            }
            RightTextOut(hdc, prc->left + dx, prc->top, szT, cch, 0);
            j = PctPlanetOptValue(lppl, idPlayer);
            if (j <= i)
                break;
            if (j > 0) {
                SetTextColor(hdc, j > 10 ? 0 : 32639);
            }
            cch = _wsprintf(szT, "(%d%%)", j);
            RightTextOut(hdc, prc->right, prc->top, szT, cch, 0);
            break;
        case colPlanetMinerals:
        case colPlanetMiningRate:
            xCur = dx * 4 + prc->left;
            if (icol == colPlanetMiningRate) {
                xCur -= dx;
                EstMineralsMined(lppl, rgl, -1, FALSE);
            }
            for (i = 0; i < 3; i++) {
                if (icol == colPlanetMinerals) {
                    l = lppl->rgwtMin[i];
                } else {
                    l = rgl[i];
                }
                DrawMineralItem(hdc, xCur, prc->top, i, l);
                if (icol == colPlanetMiningRate) {
                    xCur += dx * 4;
                } else {
                    xCur += 5 * dx;
                }
            }
            break;
        case colPlanetMinConc:
            xCur = 3 * dx + prc->left;
            for (i = 0; i < 3; i++) {
                iItem = lppl->rgMinConc[i];
                DrawMineralItem(hdc, xCur, prc->top, i, iItem);
                xCur += dx * 4;
            }
            break;
        case colPlanetProduction:
            pl = sel.pl;
            sel.pl = *lppl;
            FillPlanetProdLB(NULL, NULL, lppl);
            sel.pl = pl;
            DrawProductionItem(hdc, prc, szWork, 0, FALSE, 2);
            break;
        case colPlanetResources:
            cch = CchGetString(idsN1000, szT);
            dx = LOWORD(GetTextExtent(hdc, szT, cch));
            i = j = CResourcesAtPlanet(lppl, idPlayer);
            if (!lppl->fNoResearch) {
                i -= MulDiv(i, rgplr[idPlayer].pctResearch, 100);
            }
            CchGetString(idsD4, szT);
            cch = _wsprintf(szWork, szT, i);
            RightTextOut(hdc, prc->left + dx, prc->top, szWork, cch, 0);
            cch = _wsprintf(szT, PCTD, j);
            RightTextOut(hdc, prc->right, prc->top, szT, cch, 0);
        }
        break;

    case rptFleets:
        lpfl = rglpfl[vlprgidFleet[irow]];
        SelectObject(hdc, rghfontArial8[1]);
        SetTextColor(hdc, 0);
        SetBkMode(hdc, TRANSPARENT);
        if ((uint16_t)icol > colFleetMass)
            break;
        switch (icol) {
        case colFleetBattlePlan:
            i = lpfl->iplan;
            fstrcpy(szT, rglpbtlplan[lpfl->iplr][lpfl->iplan].szName);
            psz = szT;
            ExtTextOut(hdc, prc->left, prc->top, ETO_CLIPPED, prc, psz, strlen(psz), NULL);
            break;
        case colFleetName:
            if (sel.grobj == grobjFleet && sel.fl.id == lpfl->id) {
                SetTextColor(hdc, 127);
            }
            psz = PszGetFleetName(lpfl->id);
            ExtTextOut(hdc, prc->left, prc->top, ETO_CLIPPED, prc, psz, strlen(psz), NULL);
            break;
        case colFleetLocation:
            if (lpfl->idPlanet != idPlanetDeepSpace) {
                psz = PszGetPlanetName(lpfl->idPlanet);
            } else {
                psz = szT;
                _wsprintf(psz, PszGetCompressedString(idsSpaceDD), lpfl->pt.x, lpfl->pt.y);
            }
            ExtTextOut(hdc, prc->left, prc->top, ETO_CLIPPED, prc, psz, strlen(psz), NULL);
            break;
        case colFleetDestination:
            psz = PszGetDestName(lpfl, hdc);
            ExtTextOut(hdc, prc->left, prc->top, ETO_CLIPPED, prc, psz, strlen(psz), NULL);
            break;
        case colFleetTask:
            psz = PszGetTaskName(lpfl, &i);
            if (i != -1) {
                SetTextColor(hdc, rgcrMinerals[i]);
            }
            ExtTextOut(hdc, prc->left, prc->top, ETO_CLIPPED, prc, psz, strlen(psz), NULL);
            break;
        case colFleetEta:
            psz = PszGetETA(hdc, lpfl, NULL);
            RightTextOut(hdc, prc->right, prc->top, psz, strlen(psz), 0);
            break;
        case colFleetCargo:
            xCur = dx * 4 + prc->left;
            for (i = 0; i <= 3; i++) {
                DrawMineralItem(hdc, xCur, prc->top, i, lpfl->rgwtMin[i]);
                xCur += 5 * dx;
            }
            break;
        case colFleetFuel:
            fEnough = lpfl->cord <= 1 || lpfl->rgwtMin[4] >= EstFuelUse(lpfl, 0, lpfl->lpplord->rgord[1].iWarp, -1, FALSE);
            DrawMineralItem(hdc, dx * 4 + prc->left, prc->top, -fEnough, lpfl->rgwtMin[4]);
            break;
        case colFleetComposition:
            i = IshdefPrimaryFromLpfl(lpfl, &j);
            if (lpfl->rgdv[i].dp != 0) {
                SetTextColor(hdc, 0xff);
            }
            prc->right -= 6 * dx;
            psz = rgshdef[i].hul.szClass;
            ExtTextOut(hdc, prc->left, prc->top, ETO_CLIPPED, prc, psz, strlen(psz), NULL);
            psz = szT;
            cch = _wsprintf(psz, PCTD, lpfl->rgcsh[i]);
            chT = '+';
            RightTextOut(hdc, 6 * dx + prc->right - LOWORD(GetTextExtent(hdc, &chT, 1)), prc->top, psz, cch, 0);
            if (j <= 1)
                break;
            TextOut(hdc, 6 * dx + prc->right - LOWORD(GetTextExtent(hdc, &chT, 1)), prc->top, &chT, 1);
            break;
        case colFleetCloak:
            i = PctCloakFromLpfl(lpfl);
            if (i == 0) {
                cch = strlen(szDblDash);
                psz = szDblDash;
            } else {
                psz = szT;
                cch = _wsprintf(psz, PCTDPCTPCT, i);
            }
            RightTextOut(hdc, prc->right - 5, prc->top, psz, cch, 0);
            break;
        case colFleetMass:
            DrawMineralItem(hdc, prc->right - 2, prc->top, -1, WtFromLpfl(lpfl));
            break;
        case colFleetId:
            psz = szT;
            cch = _wsprintf(psz, "%d", lpfl->ifl + 1);
            RightTextOut(hdc, prc->right - 2, prc->top, psz, cch, 0);
        }
        break;
    case rptBattles:
        lpbd = BtlDataGet(vlprgidMisc[irow]);
        if (!lpbd)
            break;
        SelectObject(hdc, rghfontArial8[1]);
        SetTextColor(hdc, 0);
        SetBkMode(hdc, TRANSPARENT);
        if ((uint16_t)icol > colBattleTheirsLeft)
            break;
        switch (icol) {
        case colBattleLocation:
            if (lpbd->idPlanet != idPlanetNone) {
                psz = PszGetPlanetName(lpbd->idPlanet);
            } else {
                psz = szT;
                _wsprintf(psz, PszGetCompressedString(idsSpaceDD), lpbd->pt.x, lpbd->pt.y);
            }
            if (lpbd->pt.x == sel.scan.pt.x && lpbd->pt.y == sel.scan.pt.y) {
                SetTextColor(hdc, 127);
            }
            ExtTextOut(hdc, prc->left, prc->top, ETO_CLIPPED, prc, psz, strlen(psz), NULL);
            break;
        case colBattleSides:
            DrawMineralItem(hdc, prc->right - 2, prc->top, -1, (uint32_t)lpbd->cplr);
            break;
        case colBattleStarbase:
            if (CBattleUnits(lpbd, grBuOurUnits | grBuIncludeSb) != 0) {
                chT = 'O';
            } else if (CBattleUnits(lpbd, grBuTheirUnits | grBuIncludeSb) != 0) {
                chT = 'T';
            } else {
                chT = ' ';
            }
            CtrTextOut(hdc, (int16_t)(prc->right - prc->left) / 2 + prc->left, prc->top, &chT, 1);
            break;
        case colBattleUnarmed:
            i = 11;
            goto BtlUnitsCom;
        case colBattleScout:
            i = 19;
            goto BtlUnitsCom;
        case colBattleWarship:
            i = 35;
            goto BtlUnitsCom;
        case colBattleBomber:
            i = 67;
            goto BtlUnitsCom;
        case colBattleUtility:
            i = 131;
            goto BtlUnitsCom;
        case colBattleUnits:
            i = 255;
            goto BtlUnitsCom;
        case colBattleOurs:
            i = 253;
            goto BtlUnitsCom;
        case colBattleTheirs:
            i = 254;
        BtlUnitsCom:
            DrawMineralItem(hdc, prc->right - 2, prc->top, -1, CBattleUnits(lpbd, i));
            break;
        case colBattleOurDead:
        case colBattleTheirDead:
            l = CBattleKills(lpbd, icol == colBattleOurDead);
            DrawMineralItem(hdc, prc->right - 2, prc->top, -1, l);
            break;
        case colBattleOursLeft:
            i = 253;
        LUnitsLeft:
            l = CBattleUnits(lpbd, i);
            l -= CBattleKills(lpbd, icol == colBattleOursLeft);
            DrawMineralItem(hdc, prc->right - 2, prc->top, -1, l);
            break;
        case colBattleTheirsLeft:
            i = 254;
            goto LUnitsLeft;
        }
        break;

    case rptEnemyFleets:
        lpfl = rglpfl[vlprgidMisc[irow]];
        SelectObject(hdc, rghfontArial8[1]);
        SetTextColor(hdc, 0);
        SetBkMode(hdc, TRANSPARENT);
        if ((uint16_t)icol <= colEnemyFleetUtility) {
            switch (icol) {
            case colEnemyFleetName:
                if (sel.scan.grobj == grobjFleet && rglpfl[sel.scan.ifl]->id == lpfl->id) {
                    SetTextColor(hdc, 127);
                }
                psz = PszGetFleetName(lpfl->id);
                ExtTextOut(hdc, prc->left, prc->top, ETO_CLIPPED, prc, psz, strlen(psz), NULL);
                break;
            case colEnemyFleetLocation:
                if (lpfl->idPlanet != idPlanetDeepSpace) {
                    psz = PszGetPlanetName(lpfl->idPlanet);
                } else {
                    psz = szT;
                    _wsprintf(psz, PszGetCompressedString(idsSpaceDD), lpfl->pt.x, lpfl->pt.y);
                }
                ExtTextOut(hdc, prc->left, prc->top, ETO_CLIPPED, prc, psz, strlen(psz), NULL);
                break;
            case colEnemyFleetComposition:
                i = IshdefPrimaryFromLpfl(lpfl, &j);
                prc->right -= 6 * dx;
                lpsz = rglpshdef[lpfl->iPlayer][i].hul.szClass;
                ExtTextOut(hdc, prc->left, prc->top, ETO_CLIPPED, prc, lpsz, fstrlen(lpsz), NULL);
                psz = szT;
                cch = _wsprintf(psz, PCTD, lpfl->rgcsh[i]);
                chT = '+';
                RightTextOut(hdc, 6 * dx + prc->right - LOWORD(GetTextExtent(hdc, &chT, 1)), prc->top, psz, cch, 0);
                if (j <= 1)
                    break;
                TextOut(hdc, 6 * dx + prc->right - LOWORD(GetTextExtent(hdc, &chT, 1)), prc->top, &chT, 1);
                break;
            case colEnemyFleetShips:
                l = 0;
                for (i = 0; i < 16; i++) {
                    l += lpfl->rgcsh[i];
                }
                DrawMineralItem(hdc, prc->right - 2, prc->top, -1, l);
                break;
            case colEnemyFleetUnarmed:
                l = 0;
                for (i = 0; i < 16; i++) {
                    if (lpfl->rgcsh[i] != 0) {
                        j = LphuldefFromId(rglpshdef[lpfl->iPlayer][i].hul.ihuldef)->imdCategory;
                        if (j <= 1 || j >= 6) {
                            l += lpfl->rgcsh[i];
                        }
                    }
                }
                DrawMineralItem(hdc, prc->right - 2, prc->top, -1, l);
                break;
            case colEnemyFleetScout:
                j = 2;
            LEFleetCount:
                l = 0;
                for (i = 0; i < 16; i++) {
                    if (lpfl->rgcsh[i] != 0 && LphuldefFromId(rglpshdef[lpfl->iPlayer][i].hul.ihuldef)->imdCategory == j) {
                        l += lpfl->rgcsh[i];
                    }
                }
                DrawMineralItem(hdc, prc->right - 2, prc->top, -1, l);
                break;
            case colEnemyFleetWarship:
                j = 3;
                goto LEFleetCount;
            case colEnemyFleetBomber:
                j = 5;
                goto LEFleetCount;
            case colEnemyFleetUtility:
                j = 4;
                goto LEFleetCount;
            case colEnemyFleetMass:
                DrawMineralItem(hdc, prc->right - 2, prc->top, -1, lpfl->wtFleet);
                break;
            case colEnemyFleetId:
                psz = szT;
                cch = _wsprintf(psz, "%d", lpfl->ifl + 1);
                RightTextOut(hdc, prc->right - 2, prc->top, psz, cch, 0);
                break;
            case colEnemyFleetWarp:
                if (!lpfl->fdirValid || lpfl->iwarpFlt <= 0) {
                    psz = "--";
                    cch = 2;
                } else {
                    psz = szT;
                    cch = _wsprintf(psz, "%d", lpfl->iwarpFlt);
                }
                RightTextOut(hdc, prc->right - 2, prc->top, psz, cch, 0);
            }
            break;
        }
    }
    return;
}

void DrawMineralItem(HDC hdc, int16_t x, int16_t y, int16_t iMineral, int32_t l) {
    char   *psz;
    int16_t cch;

    if (iMineral < 0) {
        SetTextColor(hdc, 0);
    } else {
        SetTextColor(hdc, rgcrMinerals[iMineral]);
    }
    if (l >= 0) {
        psz = PszFromLongK(l, &cch);
    } else {
        cch = strlen(szDblDash);
        psz = szDblDash;
    }
    RightTextOut(hdc, x, y, psz, cch, 0);
    return;
}

char *PszGetDestName(FLEET *lpfl, HDC hdc) {
    int16_t i;
    ORDER   ord;

    ord = lpfl->lpplord->rgord[0];
    if (lpfl->cord <= 1)
        goto LNoDest;

    if (ord.fValidTask) {
        switch (ord.grTask) {
        case grTaskXfer:
            for (i = 0; i < 5; i++) {
                if (ord.txp.rgia[i].iAction == iActionWaitPercent)
                    goto LDelayed;
            }
            break;
        case grTaskColonize:
            if (ord.grobj != grobjPlanet)
                break;
            goto LNoDest;
        case grTaskMerge:
        case grTaskScrap:
            goto LNoDest;
        case grTaskLayMines:
        LDelayed:
            if (hdc) {
                SetTextColor(hdc, 127);
            }
            return PszGetCompressedString(idsDelayed);
        default:
            break;
        }
    }
    ord = lpfl->lpplord->rgord[1];
    return PszGetLocName(ord.grobj, ord.id, ord.pt.x, ord.pt.y);

LNoDest:
    return szDblDash;
}

int16_t FDestIsWP0(FLEET *lpfl) {
    int16_t i;
    ORDER   ord;

    ord = lpfl->lpplord->rgord[0];
    if (lpfl->cord > 1) {
        if (ord.fValidTask) {
            switch (ord.grTask) {
            case grTaskXfer:
                for (i = 0; i < 5; i++) {
                    if (ord.txp.rgia[i].iAction == iActionWaitPercent) {
                        return TRUE;
                    }
                }
                return FALSE;
            case grTaskColonize:
                if (ord.grobj != grobjPlanet)
                    break;
                /* fallthrough */
            case grTaskMerge:
            case grTaskScrap:
            case grTaskLayMines:
                return TRUE;
            }
        }
        return FALSE;
    }
    return TRUE;
}

char *PszGetETA(HDC hdc, FLEET *lpfl, int16_t *pcYears) {
    POINT16 pt;
    int16_t c;
    int16_t i;
    ORDER   ord;
    char   *psz;

    ord = lpfl->lpplord->rgord[0];
    pt = ord.pt;
    if (lpfl->cord > 1) {
        if (ord.fValidTask) {
            switch (ord.grTask) {
            case grTaskXfer:
                for (i = 0; i < 5; i++) {
                    if (ord.txp.rgia[i].iAction == iActionWaitPercent)
                        goto LNoETA;
                }
                break;
            case grTaskColonize:
                if (ord.grobj == grobjPlanet)
                    goto LNoETA;
            default:
                break;
            case grTaskMerge:
            case grTaskScrap:
            case grTaskLayMines:
                goto LNoETA;
            }
        }
        ord = lpfl->lpplord->rgord[1];
        CchGetETA(hdc, lpfl, szWork, 1, TRUE);
        if (hdc && EstFuelUse(lpfl, 0, ord.iWarp, -1, FALSE) > lpfl->rgwtMin[4]) {
            SetTextColor(hdc, 0xff);
        }
        if (pcYears) {
            psz = szWork;
            c = 0;
            for (; *psz >= '0' && *psz <= '9'; psz++) {
                c = 10 * c + (*psz - '0');
            }
            if (c == 0) {
                c = 32000;
            }
            *pcYears = c;
        }
        return szWork;
    }
LNoETA:
    if (pcYears) {
        *pcYears = 0;
    }
    return szDblDash;
}

char *PszGetTaskName(FLEET *lpfl, int16_t *picr) {
    int16_t        icr;
    StringId       ids;
    XferActionType opOrd;
    int16_t        iZip;
    int16_t        i;
    ORDER          ord;
    int16_t        fPercent;
    char          *psz;

    icr = -1;
    ord = lpfl->lpplord->rgord[0];
    *picr = -1;
    if (ord.fValidTask) {
        switch (ord.grTask) {
        case grTaskXfer:
            for (i = 0; i < 4; i++) {
                if (ord.txp.rgia[i].iAction == iActionWaitPercent)
                    goto LShowTask;
            }
            break;
        case grTaskColonize:
        case grTaskMerge:
        case grTaskScrap:
        case grTaskLayMines:
        case grTaskAutoRoute:
            goto LShowTask;
        }
    }
    if (lpfl->cord > 1) {
        ord = lpfl->lpplord->rgord[1];
    }
LShowTask:
    if (ord.fValidTask) {
        ids = ord.grTask + 99;
        switch (ord.grTask) {
        case grTaskXfer:
            for (i = 0; i < 4; i++) {
                if (vrgZip[i].fValid && memcmp(&vrgZip[i], &ord.txp, 10) == 0)
                    return vrgZip[i].szName;
            }
            ids = idsAction;
            iZip = -1;
            if (ord.txp.rgia[4].iAction == iActionLoadDunnage &&
                (ord.txp.rgia[3].iAction == iActionNone || (ord.txp.rgia[3].iAction == iActionUnloadAll && ord.txp.rgia[0].iAction == iActionUnloadAll))) {
                opOrd = ord.txp.rgia[0].iAction;
                switch (opOrd) {
                case iActionLoadAll:
                case iActionUnloadAll:
                case iActionWaitPercent:
                    for (i = 1; i < 3 && ord.txp.rgia[i].iAction == opOrd; i++) {
                    }
                    if (i == 3) {
                        switch (opOrd) {
                        case iActionLoadAll:
                            iZip = 0;
                            break;
                        case iActionUnloadAll:
                            iZip = 1;
                            break;
                        case iActionWaitPercent:
                            iZip = 2;
                        }
                        return rgszZipOrder[iZip];
                    }
                }
            }
            for (i = 4; i >= 0; i--) {
                opOrd = ord.txp.rgia[i].iAction;
                if (opOrd + 109 > (int16_t)ids) {
                    ids = opOrd + 109;
                    icr = i;
                }
            }
            if (ids == idsAction) {
                return PszGetCompressedString(idsTransport);
            }
            opOrd = ord.txp.rgia[icr].iAction;
            *picr = icr;
            fPercent = FALSE;
            switch (opOrd) {
            case iActionLoadDunnage:
                if (icr == 4) {
                    ids = idsLoadOptimal;
                }
                /* fallthrough */
            case iActionLoadAll:
            case iActionUnloadAll:
                return PszGetCompressedString(ids);
            case iActionFillPercent:
            case iActionWaitPercent:
                fPercent = TRUE;
                /* fallthrough */
            case iActionUnloadExact:
            case iActionSetAmount:
            case iActionSetWaypoint:
                psz = PszGetCompressedString(ids);
                psz[strlen(psz) - 3] = 0;
                if (fPercent) {
                    _wsprintf(szWork, "%s %d%%", psz, ord.txp.rgia[icr].cQuan);
                } else {
                    _wsprintf(szWork, icr == 4 ? "%s %dmg" : "%s %dkT", psz, ord.txp.rgia[icr].cQuan);
                }
                return szWork;
            default:
                return szDblDash;
            }
        case grTaskLayMines:
            if (ord.tlm.cTime < 5) {
                _wsprintf(szWork, "%s  %dy", PszGetCompressedString(ids), ord.tlm.cTime + 1);
            } else {
                CchGetString(ids, szWork);
            }
            return szWork;
        case grTaskPatrol:
            if (ord.tptl.iDist < 11) {
                _wsprintf(szWork, "%s  %dly", PszGetCompressedString(ids), (ord.tptl.iDist + 1) * 50);
            } else {
                CchGetString(ids, szWork);
            }
            return szWork;
        case grTaskAutoRoute:
        default:
            return PszGetCompressedString(ids);
        }
    }
    return szDblDash;
}

void SortReportCache(ReportType irpt, int16_t icol) {
    uint16_t rgidRep[1024];
    PLANET  *lpplMac;
    int16_t  cRows;
    uint16_t iItem;
    PLANET  *lppl;
    FLEET   *lpfl;
    int16_t  i;

    cRows = 0;
    iItem = 0;
    if (vprptCur->icolSort != icol) {
        vicolSortPrev = vprptCur->icolSort;
        viSubsortPrev = vprptCur->iSubsort;
        vfAscendingPrev = vprptCur->fAscending;
        vprptCur->icolSort = icol;
        gd.fChgReports = TRUE;
    }
    if (hwndReportDlg || !vprptCur->fCached) {
        switch (irpt) {
        case rptFleets:
            vlprgidRep = vlprgidFleet;
            for (iItem = 0; (int16_t)iItem < cFleet; iItem++) {
                lpfl = rglpfl[iItem];
                if (!rglpfl[iItem])
                    break;
                if (lpfl->iplr == idPlayer) {
                    rgidRep[cRows++] = iItem;
                }
            }
            break;
        case rptEnemyFleets:
            vlprgidRep = vlprgidMisc;
            vrptBattle.fCached = FALSE;
            for (iItem = 0; (int16_t)iItem < cFleet; iItem++) {
                lpfl = rglpfl[iItem];
                if (!rglpfl[iItem])
                    break;
                if (lpfl->iplr != idPlayer) {
                    rgidRep[cRows++] = iItem;
                }
                if (cRows >= 1020)
                    break;
            }
            break;
        case rptPlanets:
            vlprgidRep = vlprgidPlanet;
            lppl = lpPlanets;
            lpplMac = lpPlanets + cPlanet;
            for (; lppl < lpplMac; lppl++) {
                if (lppl->iPlayer == idPlayer && lppl->det == detAll) {
                    rgidRep[cRows++] = iItem;
                }
                iItem++;
            }
            break;
        case rptBattles:
            vlprgidRep = vlprgidMisc;
            vrptEFleet.fCached = FALSE;
            cRows = CBattles();
            for (i = 0; i < cRows; i++) {
                rgidRep[i] = i;
            }
            break;
        default:
            return;
        }
        vprptCur->cRows = cRows;
        qsort(rgidRep, cRows, sizeof(uint16_t), (QSORTCOMPARE)ICompReport);
        fmemcpy(vlprgidRep, rgidRep, cRows * 2);
        vprptCur->fCached = TRUE;
    }
    return;
}

int ICompReport(uint16_t *pid1, uint16_t *pid2) {
    char         szT[80];
    int32_t      l2;
    int16_t      fAscending;
    int16_t      icolSort;
    int16_t      i1;
    HullCategory j;
    int16_t      i;
    int32_t      l1;
    int16_t      iSubsort;
    char        *psz;
    int16_t      iRet;
    int16_t      i2;
    int16_t      fTier2;
    ReportType   irpt;
    PLANET      *lppl2;
    PLANET      *lppl1;
    float        pct2;
    float        pct1;
    int16_t      iFirst;
    int32_t      rgl[4];
    int16_t      iLast;
    FLEET       *lpfl2;
    FLEET       *lpfl1;
    BTLDATA     *lpbd1;
    int16_t      ibtl2;
    int16_t      ibtl1;
    BTLDATA     *lpbd2;

    iRet = 0;
    iSubsort = vprptCur->iSubsort;
    irpt = vprptCur->irpt;
    icolSort = vprptCur->icolSort;
    fAscending = vprptCur->fAscending;
    fTier2 = FALSE;
TryTier2:
    switch (irpt) {
    case rptPlanets:
        lppl1 = lpPlanets + *pid1;
        lppl2 = lpPlanets + *pid2;
        if ((uint16_t)icolSort > colPlanetRoutingDest)
            break;
        switch (icolSort) {
        case colPlanetStarbase:
            if (!lppl1->fStarbase) {
                if (!lppl2->fStarbase) {
                    iRet = 0;
                    break;
                }
                iRet = 1;
                break;
            }
            if (!lppl2->fStarbase) {
                iRet = -1;
                break;
            }
            iRet = fstrcmp(rglpshdefSB[idPlayer][lppl1->isb].hul.szClass, rglpshdefSB[idPlayer][lppl2->isb].hul.szClass);
            break;
        case colPlanetRoutingDest:
            if (lppl1->idRoute == 0) {
                if (lppl2->idRoute == 0) {
                    iRet = 0;
                    break;
                }
                iRet = 1;
                break;
            }
            if (lppl2->idRoute == 0) {
                iRet = -1;
                break;
            }
            psz = PszGetPlanetName(lppl1->idRoute - 1);
            strcpy(szT, psz);
            psz = PszGetPlanetName(lppl2->idRoute - 1);
            iRet = strcmp(szT, psz);
            break;
        case colPlanetDriverDest:
            if (lppl1->idFling == 0) {
                if (lppl2->idFling == 0) {
                    iRet = 0;
                    break;
                }
                iRet = 1;
                break;
            }
            if (lppl2->idFling == 0) {
                iRet = -1;
                break;
            }
            psz = PszGetPlanetName(lppl1->idFling - 1);
            strcpy(szT, psz);
            psz = PszGetPlanetName(lppl2->idFling - 1);
            iRet = strcmp(szT, psz);
            break;
        case colPlanetName:
            psz = PszGetPlanetName(lppl1->id);
            strcpy(szT, psz);
            psz = PszGetPlanetName(lppl2->id);
            iRet = strcmp(szT, psz);
            break;
        case colPlanetProduction:
            FillPlanetProdLB(NULL, NULL, lppl1);
            strcpy(szT, szWork);
            FillPlanetProdLB(NULL, NULL, lppl2);
            iRet = strcmp(&szT[6], &szWork[6]);
            break;
        case colPlanetCapacity:
            iRet = PctPlanetCapacity(lppl1) - PctPlanetCapacity(lppl2);
            break;
        case colPlanetPopulation:
            iRet = LOWORD(lppl1->rgwtMin[3]) - LOWORD(lppl2->rgwtMin[3]);
            break;
        case colPlanetValue:
            iRet = PctPlanetDesirability(lppl1, idPlayer) - PctPlanetDesirability(lppl2, idPlayer);
            break;
        case colPlanetMinConc:
            if (iSubsort == 3) {
                i2 = 0;
                i1 = 0;
                for (i = 0; i < 3; i++) {
                    i1 += lppl1->rgMinConc[i];
                    i2 += lppl2->rgMinConc[i];
                }
            } else {
                i1 = lppl1->rgMinConc[iSubsort];
                i2 = lppl2->rgMinConc[iSubsort];
            }
            iRet = i1 - i2;
            break;
        case colPlanetDefense:
            if (lppl1->cDefenses == 0) {
                pct1 = (float)0;
            } else {
                CalcPctSurvive(lppl1, &pct1, NULL);
                pct1 = (float)((long double)1.0 - pct1);
            }
            if (lppl2->cDefenses == 0) {
                pct2 = (float)0;
            } else {
                CalcPctSurvive(lppl2, &pct2, NULL);
                pct2 = (float)((long double)1.0 - pct2);
            }
            if ((long double)pct1 < (long double)pct2) {
                iRet = -1;
                break;
            }
            if ((long double)pct1 > (long double)pct2) {
                iRet = 1;
                break;
            }
            iRet = 0;
            break;
        case colPlanetFactories:
            iRet = lppl1->cFactories - lppl2->cFactories;
            break;
        case colPlanetMines:
            iRet = lppl1->cMines - lppl2->cMines;
            break;
        case colPlanetMiningRate:
            if (iSubsort == 3) {
                iFirst = 0;
                iLast = 2;
            } else {
                iLast = iSubsort;
                iFirst = iSubsort;
            }
            EstMineralsMined(lppl1, rgl, -1, FALSE);
            l1 = 0;
            for (i = iFirst; i <= iLast; i++) {
                l1 += rgl[i];
            }
            EstMineralsMined(lppl2, rgl, -1, FALSE);
            l2 = 0;
            for (i = iFirst; i <= iLast; i++) {
                l2 += rgl[i];
            }
            iRet = LOWORD(l1) - LOWORD(l2);
            break;
        case colPlanetMinerals:
            if (iSubsort == 3) {
                iFirst = 0;
                iLast = 2;
            } else {
                iLast = iSubsort;
                iFirst = iSubsort;
            }
            l2 = 0;
            l1 = 0;
            for (i = iFirst; i <= iLast; i++) {
                l1 += lppl1->rgwtMin[i];
                l2 += lppl2->rgwtMin[i];
            }
            l1 -= l2;
            if (l1 < 0) {
                iRet = -1;
                break;
            }
            if (l1 > 0) {
                iRet = 1;
                break;
            }
            iRet = 0;
            break;
        case colPlanetResources:
            iRet = CResourcesAtPlanet(lppl1, idPlayer) - CResourcesAtPlanet(lppl2, idPlayer);
        }
        break;
    case rptFleets:
        lpfl1 = rglpfl[*pid1];
        lpfl2 = rglpfl[*pid2];
        if ((uint16_t)icolSort > colFleetMass)
            break;
        switch (icolSort) {
        case colFleetName:
            psz = PszGetFleetName(lpfl1->id);
            strcpy(szT, psz);
            psz = PszGetFleetName(lpfl2->id);
            iRet = strcmp(szT, psz);
            break;
        case colFleetLocation:
            if (lpfl1->idPlanet != idPlanetDeepSpace) {
                psz = PszGetPlanetName(lpfl1->idPlanet);
                strcpy(szT, psz);
            } else {
                _wsprintf(szT, PszGetCompressedString(idsSpaceDD), lpfl1->pt.x, lpfl1->pt.y);
            }
            if (lpfl2->idPlanet != idPlanetDeepSpace) {
                psz = PszGetPlanetName(lpfl2->idPlanet);
            } else {
                _wsprintf(szWork, PszGetCompressedString(idsSpaceDD), lpfl2->pt.x, lpfl2->pt.y);
                psz = szWork;
            }
            iRet = strcmp(szT, psz);
            break;
        case colFleetBattlePlan:
            iRet = lpfl1->iplan - lpfl2->iplan;
            break;
        case colFleetDestination:
            psz = PszGetDestName(lpfl1, NULL);
            strcpy(szT, psz);
            psz = PszGetDestName(lpfl2, NULL);
            iRet = strcmp(szT, psz);
            if (iRet < 0) {
                iRet = -1;
                break;
            }
            if (iRet <= 0)
                break;
            iRet = 1;
            break;
        case colFleetEta:
            PszGetETA(NULL, lpfl1, &i1);
            PszGetETA(NULL, lpfl2, &i2);
            iRet = i1 - i2;
            break;
        case colFleetCloak:
            l1 = PctCloakFromLpfl(lpfl1);
            l2 = PctCloakFromLpfl(lpfl2);
            goto LRetDiff;
        case colFleetComposition:
            l1 = IshdefPrimaryFromLpfl(lpfl1, &i1);
            l2 = IshdefPrimaryFromLpfl(lpfl2, &i2);
            if (l1 == l2) {
                iRet = lpfl1->rgcsh[l1] - lpfl2->rgcsh[l2];
            } else {
                iRet = strcmp(rgshdef[l1].hul.szClass, rgshdef[l2].hul.szClass);
                if (iRet == 0) {
                    iRet = lpfl1->rgcsh[l1] - lpfl2->rgcsh[l2];
                }
                if (iRet == 0) {
                    iRet = LOWORD(l1) - LOWORD(l2);
                }
            }
            if (iRet != 0)
                break;
            iRet = i1 - i2;
            break;
        case colFleetTask:
            psz = PszGetTaskName(lpfl1, &i1);
            strcpy(szT, psz);
            psz = PszGetTaskName(lpfl2, &i2);
            iRet = strcmp(szT, psz);
            if (iRet < 0) {
                iRet = -1;
                break;
            }
            if (iRet > 0) {
                iRet = 1;
                break;
            }
            if (i1 > i2) {
                iRet = 1;
                break;
            }
            if (i1 >= i2)
                break;
            iRet = -1;
            break;
        case colFleetFuel:
            iSubsort = -1;
            /* fallthrough */
        case colFleetCargo:
            if (iSubsort == 4) {
                l2 = 0;
                l1 = 0;
                for (i = 0; i <= 3; i++) {
                    l1 += lpfl1->rgwtMin[i];
                    l2 += lpfl2->rgwtMin[i];
                }
                goto LRetDiff;
            }
            if (iSubsort == -1) {
                iSubsort = 4;
            }
            l1 = lpfl1->rgwtMin[iSubsort];
            l2 = lpfl2->rgwtMin[iSubsort];
        LRetDiff:
            l1 -= l2;
            if (l1 < 0) {
                iRet = -1;
            } else if (l1 > 0) {
                iRet = 1;
            } else {
                iRet = 0;
            }
            break;
        case colFleetMass:
            l1 = WtFromLpfl(lpfl1);
            l2 = WtFromLpfl(lpfl2);
            goto LRetDiff;
        case colFleetId:
            l1 = lpfl1->id;
            l2 = lpfl2->id;
            goto LRetDiff;
        }
        break;
    case rptBattles:
        ibtl1 = *pid1;
        ibtl2 = *pid2;
        lpbd1 = BtlDataGet(ibtl1);
        lpbd2 = BtlDataGet(ibtl2);
        if (!lpbd1 || !lpbd2) {
            iRet = 0;
            break;
        }
        if ((uint16_t)icolSort > colBattleTheirsLeft)
            break;
        switch (icolSort) {
        case colBattleLocation:
            if (lpbd1->idPlanet != idPlanetNone) {
                psz = PszGetPlanetName(lpbd1->idPlanet);
                strcpy(szT, psz);
            } else {
                _wsprintf(szT, PszGetCompressedString(idsSpaceDD), lpbd1->pt.x, lpbd1->pt.y);
            }
            if (lpbd2->idPlanet != idPlanetNone) {
                psz = PszGetPlanetName(lpbd2->idPlanet);
            } else {
                _wsprintf(szWork, PszGetCompressedString(idsSpaceDD), lpbd2->pt.x, lpbd2->pt.y);
                psz = szWork;
            }
            iRet = strcmp(szT, psz);
            break;
        case colBattleStarbase:
            l1 = CBattleUnits(lpbd1, grBuOurUnits | grBuIncludeSb);
            if (l1 == 0) {
                l1 = (int32_t)(CBattleUnits(lpbd1, grBuTheirUnits | grBuIncludeSb) * 2);
            }
            l2 = CBattleUnits(lpbd2, grBuOurUnits | grBuIncludeSb);
            if (l2 != 0)
                goto LRetDiff;
            l2 = (int32_t)(CBattleUnits(lpbd2, grBuTheirUnits | grBuIncludeSb) * 2);
            goto LRetDiff;
        case colBattleSides:
            l1 = (uint32_t)lpbd1->cplr;
            l2 = (uint32_t)lpbd2->cplr;
            goto LRetDiff;
        case colBattleUnarmed:
            i = 11;
            goto BtlUnitsCom;
        case colBattleScout:
            i = 19;
            goto BtlUnitsCom;
        case colBattleWarship:
            i = 35;
            goto BtlUnitsCom;
        case colBattleBomber:
            i = 67;
            goto BtlUnitsCom;
        case colBattleUtility:
            i = 131;
            goto BtlUnitsCom;
        case colBattleUnits:
            i = 255;
            goto BtlUnitsCom;
        case colBattleOurs:
            i = 253;
            goto BtlUnitsCom;
        case colBattleTheirs:
            i = 254;
        BtlUnitsCom:
            l1 = CBattleUnits(lpbd1, i);
            l2 = CBattleUnits(lpbd2, i);
            goto LRetDiff;
        case colBattleOurDead:
        case colBattleTheirDead:
            l1 = CBattleKills(lpbd1, icolSort == colBattleOurDead);
            l2 = CBattleKills(lpbd2, icolSort == colBattleOurDead);
            goto LRetDiff;
        case colBattleOursLeft:
            i = 253;
        LUnitsLeft:
            l1 = CBattleUnits(lpbd1, i);
            l2 = CBattleUnits(lpbd2, i);
            l1 -= CBattleKills(lpbd1, icolSort == colBattleOursLeft);
            l2 -= CBattleKills(lpbd2, icolSort == colBattleOursLeft);
            goto LRetDiff;
        case colBattleTheirsLeft:
            i = 254;
            goto LUnitsLeft;
        }
        break;

    case rptEnemyFleets:
        lpfl1 = rglpfl[*pid1];
        lpfl2 = rglpfl[*pid2];
        if ((uint16_t)icolSort <= colEnemyFleetUtility) {
            switch (icolSort) {
            case colEnemyFleetName:
                psz = PszGetFleetName(lpfl1->id);
                strcpy(szT, psz);
                psz = PszGetFleetName(lpfl2->id);
                iRet = strcmp(szT, psz);
                break;
            case colEnemyFleetLocation:
                if (lpfl1->idPlanet != idPlanetDeepSpace) {
                    psz = PszGetPlanetName(lpfl1->idPlanet);
                    strcpy(szT, psz);
                } else {
                    _wsprintf(szT, PszGetCompressedString(idsSpaceDD), lpfl1->pt.x, lpfl1->pt.y);
                }
                if (lpfl2->idPlanet != idPlanetDeepSpace) {
                    psz = PszGetPlanetName(lpfl2->idPlanet);
                } else {
                    _wsprintf(szWork, PszGetCompressedString(idsSpaceDD), lpfl2->pt.x, lpfl2->pt.y);
                    psz = szWork;
                }
                iRet = strcmp(szT, psz);
                break;
            case colEnemyFleetComposition:
                l1 = IshdefPrimaryFromLpfl(lpfl1, &i1);
                l2 = IshdefPrimaryFromLpfl(lpfl2, &i2);
                if (l1 == l2) {
                    iRet = lpfl1->rgcsh[l1] - lpfl2->rgcsh[l2];
                } else {
                    iRet = fstrcmp(rglpshdef[lpfl1->iPlayer][l1].hul.szClass, rglpshdef[lpfl2->iPlayer][l2].hul.szClass);
                    if (iRet == 0) {
                        iRet = lpfl1->rgcsh[l1] - lpfl2->rgcsh[l2];
                    }
                    if (iRet == 0) {
                        iRet = LOWORD(l1) - LOWORD(l2);
                    }
                }
                if (iRet != 0)
                    break;
                iRet = i1 - i2;
                break;
            case colEnemyFleetMass:
                l1 = lpfl1->wtFleet;
                l2 = lpfl2->wtFleet;
                goto LRetDiff;
            case colEnemyFleetId:
                l1 = lpfl1->ifl;
                l2 = lpfl2->ifl;
                goto LRetDiff;
            case colEnemyFleetShips:
                l2 = 0;
                l1 = 0;
                for (i = 0; i < 16; i++) {
                    l1 += lpfl1->rgcsh[i];
                    l2 += lpfl2->rgcsh[i];
                }
                goto LRetDiff;
            case colEnemyFleetUnarmed:
                l2 = 0;
                l1 = 0;
                for (i = 0; i < 16; i++) {
                    if (lpfl1->rgcsh[i] != 0) {
                        j = LphuldefFromId(rglpshdef[lpfl1->iPlayer][i].hul.ihuldef)->imdCategory;
                        if ((int16_t)j <= hullCatFreighter || (int16_t)j >= hullCatMiner) {
                            l1 += lpfl1->rgcsh[i];
                        }
                    }
                    if (lpfl2->rgcsh[i] != 0) {
                        j = LphuldefFromId(rglpshdef[lpfl2->iPlayer][i].hul.ihuldef)->imdCategory;
                        if ((int16_t)j <= hullCatFreighter || (int16_t)j >= hullCatMiner) {
                            l2 += lpfl2->rgcsh[i];
                        }
                    }
                }
                goto LRetDiff;
            case colEnemyFleetScout:
                j = hullCatScout;
            LEFleetCount:
                l2 = 0;
                l1 = 0;
                for (i = 0; i < 16; i++) {
                    if (lpfl1->rgcsh[i] != 0 && LphuldefFromId(rglpshdef[lpfl1->iPlayer][i].hul.ihuldef)->imdCategory == j) {
                        l1 += lpfl1->rgcsh[i];
                    }
                    if (lpfl2->rgcsh[i] != 0 && LphuldefFromId(rglpshdef[lpfl2->iPlayer][i].hul.ihuldef)->imdCategory == j) {
                        l2 += lpfl2->rgcsh[i];
                    }
                }
                goto LRetDiff;
            case colEnemyFleetWarship:
                j = hullCatWarship;
                goto LEFleetCount;
            case colEnemyFleetBomber:
                j = hullCatBomber;
                goto LEFleetCount;
            case colEnemyFleetUtility:
                j = hullCatUtility;
                goto LEFleetCount;
            case colEnemyFleetWarp:
                if (lpfl1->fdirValid) {
                    l1 = lpfl1->iwarpFlt;
                } else {
                    l1 = -1;
                }
                if (lpfl2->fdirValid) {
                    l2 = lpfl2->iwarpFlt;
                    goto LRetDiff;
                }
                l2 = -1;
                goto LRetDiff;
            }
            break;
        }
    }
    if (!fAscending) {
        iRet = -iRet;
    }
    if (iRet == 0 && !fTier2 && (icolSort != vicolSortPrev || iSubsort != viSubsortPrev) && vicolSortPrev >= 0) {
        icolSort = vicolSortPrev;
        iSubsort = viSubsortPrev;
        fAscending = vfAscendingPrev;
        fTier2 = TRUE;
        goto TryTier2;
    }
    return iRet;
}

void ReportColumnPopup(POINT16 pt, int16_t icol, int16_t fRightBtn) {
    HDC     hdc;
    char    szT[50];
    char    rgsz[32][50];
    int16_t iBase;
    int16_t cSubsort;
    int16_t j;
    int16_t i;
    int16_t ibit;
    int16_t fccolChange;
    int16_t rgcol[32];
    char    szColTitle[50];
    int16_t cItems;
    char   *psz[32];
    int16_t cch;
    int16_t iRet;
    int16_t iHide;
    int16_t iSortLast;

    cSubsort = 0;
    fccolChange = 0;
    hdc = GetDC(hwndReportDlg);
    DxReportColHdr(vprptCur->irpt, icol, szColTitle, hdc);
    cItems = 0;
    for (i = 0; i < 2; i++) {
        cch = CchGetString(i == 0 ? idsSort : idsReverseSort, rgsz[cItems]);
        strcpy(&rgsz[cItems][cch], szColTitle);
        cItems++;
        if ((vprptCur->irpt == rptPlanets && (icol == colPlanetMinConc || icol == colPlanetMinerals || icol == colPlanetMiningRate)) ||
            (vprptCur->irpt == rptFleets && icol == colFleetCargo)) {
            strcpy(rgsz[cItems], rgsz[cItems - 1]);
            rgsz[cItems - 1][0] = 0;
            cItems++;
            for (j = 0; j < (vprptCur->irpt == rptFleets) + 3; j++) {
                strcpy(rgsz[cItems], rgszMinerals[j]);
                cItems++;
            }
            rgsz[cItems][0] = -1;
            rgsz[cItems][1] = 0;
            cItems++;
            strcpy(rgsz[cItems], PszGetCompressedString(idsWeightedAverage));
            cItems++;
            rgsz[cItems++][0] = 0;
            cSubsort = 5;
        }
    }
    rgsz[cItems][0] = -1;
    rgsz[cItems][1] = 0;
    cItems++;
    cch = CchGetString(idsHide, rgsz[cItems]);
    strcpy(&rgsz[cItems][cch], szColTitle);
    cch = CchGetString(idsColumn, szT);
    strcat(rgsz[cItems], szT);
    iHide = cItems;
    iSortLast = cItems;
    cItems++;
    rgsz[cItems][0] = -1;
    rgsz[cItems][1] = 0;
    cItems++;
    if (icol == 0) {
        cItems -= 2;
        iHide = -1;
    }
    iBase = cItems;
    i = 0;
    ibit = 1;
    while (i < vprptCur->cFields) {
        if (!(ibit & vprptCur->grbitVisible)) {
            DxReportColHdr(vprptCur->irpt, i, szColTitle, hdc);
            cch = CchGetString(idsShow, rgsz[cItems]);
            strcpy(&rgsz[cItems][cch], szColTitle);
            cch = CchGetString(idsColumn, szT);
            strcat(rgsz[cItems], szT);
            rgcol[cItems] = i;
            cItems++;
        }
        i++;
        ibit *= 2;
    }
    if (cItems == iBase) {
        cItems--;
    }
    ReleaseDC(hwndReportDlg, hdc);
    for (i = 0; i < cItems; i++) {
        if (rgsz[i][0] != 0) {
            psz[i] = rgsz[i];
        } else {
            psz[i] = 0;
        }
    }
    iRet = PopupMenu(hwndReportDlg, pt.x, pt.y, cItems, NULL, psz, -1, fRightBtn);
    if (iRet >= 0) {
        gd.fChgReports = TRUE;
        if (iRet < iSortLast) {
            vicolSortPrev = vprptCur->icolSort;
            viSubsortPrev = vprptCur->iSubsort;
            vfAscendingPrev = vprptCur->fAscending;
            vprptCur->icolSort = icol;
            if (cSubsort == 0) {
                vprptCur->fAscending = iRet == 0;
            } else {
                vprptCur->iSubsort = (int16_t)(iRet - 2) % (cSubsort + 3 + (vprptCur->irpt == rptFleets));
                if (vprptCur->iSubsort > (vprptCur->irpt == rptFleets) + 3) {
                    vprptCur->iSubsort = (vprptCur->irpt == rptFleets) + 3;
                }
                vprptCur->fAscending = iRet < cSubsort + 2;
            }
            SortReportCache(vprptCur->irpt, icol);
        } else if (iRet == iHide) {
            fccolChange = 1;
            vprptCur->grbitVisible &= (int16_t)~(1 << icol);
        } else if (iRet >= iBase) {
            fccolChange = 1;
            vprptCur->grbitVisible |= (int16_t)(1 << rgcol[iRet]);
        }
        if (fccolChange) {
            SetHScrollBar();
        }
        InvalidateRect(hwndReportDlg, NULL, TRUE);
    }
    return;
}

void InvalidateReport(ReportType irpt, int16_t fReload) {
    int16_t   fResetRpt;
    int16_t   fClearRpt;
    RPT      *prptSav;
    uint16_t *lprgidSav;
    RECT      rc;

    fClearRpt = FALSE;
    fResetRpt = FALSE;
    if (!gd.fGeneratingTurn && !fAi) {
        if (hwndReportDlg && irpt == vprptCur->irpt) {
            GetClientRect(hwndReportDlg, &rc);
            if (fReload != 2) {
                rc.top = dyArial8 + 6;
                rc.bottom = (dyArial8 + 4) * vprptCur->cRowsVis + rc.top;
            }
            InvalidateRect(hwndReportDlg, &rc, fReload == 2);
            gd.fRptSafeDraw = TRUE;
        } else if (!vprptCur) {
            fClearRpt = fReload;
            if (irpt == rptPlanets) {
                vprptCur = &vrptPlanet;
            } else {
                vprptCur = &vrptFleet;
            }
        } else if (fReload && vprptCur->irpt != irpt) {
            lprgidSav = vlprgidRep;
            prptSav = vprptCur;
            fResetRpt = TRUE;
            if (irpt == rptPlanets) {
                vprptCur = &vrptPlanet;
            } else {
                vprptCur = &vrptFleet;
            }
        }
        vprptCur->fCached = FALSE;
        if (fReload) {
            SortReportCache(vprptCur->irpt, vprptCur->icolSort);
        }
        if (fClearRpt) {
            vprptCur = 0;
        } else if (fResetRpt) {
            vprptCur = prptSav;
            vlprgidRep = lprgidSav;
        } else if (fReload && hwndReportDlg && irpt == vprptCur->irpt) {
            SetScrollRange(vprptCur->hwndVScroll, SB_CTL, 0, vprptCur->cRows - vprptCur->cRowsVis, FALSE);
            InvalidateRect(hwndReportDlg, NULL, TRUE);
        }
    }
    return;
}

void ExecuteReportClick(POINT16 pt, ReportType irpt, int16_t icol, int16_t irow) {
    HDC      hdc;
    BTLDATA *lpbd;
    PLANET  *lppl;
    int16_t  i;
    FLEET   *lpfl;
    int16_t  ibit;
    int32_t  rglQuan[4];
    int16_t  xCur;
    int16_t  dxOffset;
    SCAN     scan;

    hdc = GetDC(hwndReportDlg);
    switch (irpt) {
    case rptPlanets:
        lppl = lpPlanets + vlprgidPlanet[irow];
        if (hwndProdDlg) {
            MessageBeep(MB_OK);
            break;
        }
        SelectAdjPlanet(0, lppl->id);
        InvalidateReport(rptPlanets, 0);
        switch (icol) {
        case colPlanetName:
            if (lppl->fStarbase && pt.x > vprptCur->rgbdx[0] * 2 - 8) {
            LShowStarbase:
                GlobalPD.grPopup = grPopupShdef;
                GlobalPD.lpshdef = rglpshdefSB[idPlayer] + sel.pl.isb;
                GlobalPD.fHideCounts = FALSE;
                GlobalPD.fShowDamage = TRUE;
                GlobalPD.fToken = FALSE;
                GlobalPD.fSummary = FALSE;
                Popup(hwndReportDlg, pt.x, pt.y);
            }
            break;
        case colPlanetStarbase:
            if (!lppl->fStarbase)
                break;
            goto LShowStarbase;
        case colPlanetPopulation:
        case colPlanetValue:
            GlobalPD.grPopup = grPopupPlanet;
            GlobalPD.idPlanet = sel.pl.id;
            Popup(hwndReportDlg, pt.x, pt.y);
            break;
        case colPlanetDefense:
            if (sel.pl.cDefenses == 0)
                break;
            FGetBestDefensePart(&GlobalPD.part);
            GlobalPD.grPopup = grPopupComponent;
            Popup(hwndReportDlg, pt.x, pt.y);
            break;
        case colPlanetResources:
            GlobalPD.grPopup = grPopupResources;
            GlobalPD.idPlanet = sel.pl.id;
            GlobalPD.iPlanetVar = GlobalPD.iPlanVal = CResourcesAtPlanet(&sel.pl, idPlayer);
            if (!sel.pl.fNoResearch) {
                GlobalPD.iPlanVal -= MulDiv(GlobalPD.iPlanetVar, rgplr[idPlayer].pctResearch, 100);
            }
            Popup(hwndReportDlg, pt.x, pt.y);
            break;
        case colPlanetMinerals:
        case colPlanetMiningRate:
        case colPlanetMinConc:
            xCur = 2;
            i = 0;
            ibit = 1;
            while (i < icol) {
                if ((ibit & vprptCur->grbitVisible) && (i == 0 || i >= vprptCur->cFieldFirst)) {
                    xCur += vprptCur->rgbdx[i] * 2;
                }
                i++;
                ibit *= 2;
            }
            dxOffset = pt.x - xCur;
            for (i = 1; i <= 3 && (int16_t)(i * 2 * vprptCur->rgbdx[icol]) / 3 <= dxOffset; i++) {
            }
            i--;
            GlobalPD.grPopup = grPopupMineral;
            GlobalPD.rgi[0] = i;
            GlobalPD.rgi[2] = sel.pl.rgwtMin[i];
            GlobalPD.rgi[3] = (uint32_t)sel.pl.rgMinConc[i];
            EstMineralsMined(&sel.pl, rglQuan, -1, FALSE);
            GlobalPD.rgi[4] = rglQuan[i];
            GlobalPD.rgi[1] = sel.pl.fHomeworld;
            Popup(hwndReportDlg, pt.x, pt.y);
            break;
        case colPlanetProduction:
            if (hwndProdDlg)
                break;
            ChangeProduction(FALSE);
            break;
        case colPlanetMines:
        case colPlanetFactories:
            GlobalPD.grPopup = grPopupPlanetIndustry;
            GlobalPD.idPlan = sel.pl.id;
            GlobalPD.fFactory = icol == colPlanetFactories;
            if (GlobalPD.fFactory) {
                GlobalPD.cMax = CMaxFactories(&sel.pl, idPlayer);
                GlobalPD.cCur = sel.pl.cFactories;
                GlobalPD.cOperate = CMaxOperableFactories(&sel.pl, idPlayer, FALSE);
            } else {
                GlobalPD.cMax = CMaxMines(&sel.pl, idPlayer);
                GlobalPD.cCur = sel.pl.cMines;
                GlobalPD.cOperate = CMaxOperableMines(&sel.pl, idPlayer, FALSE);
            }
            Popup(hwndReportDlg, pt.x, pt.y);
        }
        break;
    case rptFleets:
        lpfl = rglpfl[vlprgidFleet[irow]];
        if (mdXferDlg != mdXferNone) {
            MessageBeep(MB_OK);
            break;
        }
        SelectAdjFleet(0, lpfl->id);
        InvalidateReport(rptFleets, 0);
        switch (icol) {
        case colFleetFuel:
        case colFleetCargo:
            if (mdXferDlg != mdXferNone)
                break;
            if (sel.fl.idPlanet != idPlanetDeepSpace) {
                TransferStuff(sel.fl.id, grobjFleet, sel.fl.idPlanet, grobjPlanet, mdXferCargo);
                break;
            }
            TransferStuff(sel.fl.id, grobjFleet, -1, grobjOther, mdXferCargo);
            break;
        case colFleetDestination:
        case colFleetEta:
        case colFleetTask:
            if (FDestIsWP0(lpfl))
                break;
            SendMessage(hwndShipLB, LB_SETCURSEL, 1, 0);
            SetScanWp(1);
            break;
        case colFleetComposition:
            GlobalPD.grPopup = grPopupFleet;
            GlobalPD.fRedDamage = TRUE;
            GlobalPD.grbit = 0xff;
            GlobalPD.lpfl = lpfl;
            Popup(hwndReportDlg, pt.x, pt.y);
        }
        break;
    case rptBattles:
        lpbd = BtlDataGet(vlprgidMisc[irow]);
        if (!lpbd)
            break;
        if (lpbd->pt.x == sel.scan.pt.x && lpbd->pt.y == sel.scan.pt.y)
            goto LDisplayVCRAnyway;

        scan.pt = lpbd->pt;
        scan.grobj = grobjPlanet | grobjFleet | grobjOther | grobjThing | mdExact;
        ChangeScanSel(&scan, 0);
        CtrPointScan(scan.pt, TRUE);
        InvalidateReport(rptPlanets, 0);
        if (lpbd->pt.x == sel.scan.pt.x && lpbd->pt.y == sel.scan.pt.y)
            break;

    LDisplayVCRAnyway:
        if (hwndVCRDlg)
            break;
        BattleVCR(lpbd->id);
        break;
    case rptEnemyFleets:
        if (vprptCur == &vrptEFleet && irow >= -2) {
            if (irow < 0) {
                if (irow == -1) {
                    irowEFleetCur++;
                } else {
                    irowEFleetCur--;
                }
                if (irowEFleetCur >= vprptCur->cRows) {
                    irowEFleetCur = 0;
                } else if (irowEFleetCur < 0) {
                    irowEFleetCur = vprptCur->cRows - 1;
                }
                irow = irowEFleetCur;
            }
            lpfl = rglpfl[vlprgidMisc[irow]];
            if (mdXferDlg != mdXferNone) {
                MessageBeep(MB_OK);
            } else {
                FFindNearestObject(lpfl->pt, grobjFleet, &scan);
                scan.ifl = vlprgidMisc[irow];
                ChangeScanSel(&scan, 2);
                FEnsurePointOnScreen(lpfl->pt, TRUE);
                irowEFleetCur = irow;
                InvalidateReport(rptEnemyFleets, 0);
            }
        }
    }
    ReleaseDC(hwndReportDlg, hdc);
    return;
}

void DumpUniverse() {
    StringId ids;
    int16_t  i;
    jmp_buf  env;
    int16_t  fOpen;
    int16_t  fSuccess;
    int16_t  fSilentSav;
    jmp_buf *penvMemSav;
    int16_t  cch;

    fSilentSav = fFileErrSilent;
    fSuccess = TRUE;
    fOpen = FALSE;
    if (game.lid == 0 || idPlayer == iplrNone) {
        fSuccess = FALSE;
        goto DisplayStatus;
    } else {
        penvMemSav = penvMem;
        penvMem = &env;
        if (setjmp(env) != 0) {
            if (fOpen) {
                StreamClose();
            }
            fFileErrSilent = fSilentSav;
            fSuccess = FALSE;
            penvMem = penvMemSav;
            goto DisplayStatus;
        } else {
            fFileErrSilent = TRUE;
            _wsprintf(szWork, "%s.map", szBase);
            StreamOpen(szWork, mdCreate);
            fOpen = TRUE;
            RgToStream("#\tX\tY\tName\r\n", 12);
            for (i = 0; i < game.cPlanMax; i++) {
                cch = _wsprintf(szWork, "%d\t%d\t%d\t%s\r\n", i + 1, rgptPlan[i].x, rgptPlan[i].y, PszGetCompressedPlanet(rgidPlan[i]));
                RgToStream(szWork, cch);
            }
            StreamClose();
        }
    }
DisplayStatus:
    ids = !fSuccess ? idsUnableWriteUniverseDefinitionSMapOperation : idsUniverseDefinitionHasSuccessfullyWrittenSMap;
    _wsprintf(szWork, PszGetCompressedString(ids), szBase);
    if (fSuccess) {
        AlertSz(szWork, MB_ICONASTERISK);
    } else {
        AlertSz(szWork, MB_ICONHAND);
    }
    fFileErrSilent = fSilentSav;
    penvMem = penvMemSav;
    return;
}

void DumpPlanets() {
    PLANET  *lpplMac;
    StringId ids;
    PLANET  *lppl;
    char     szFile[256];
    char     szForm[256];
    int16_t  j;
    int16_t  i;
    jmp_buf  env;
    int16_t  fOpen;
    int16_t  fSuccess;
    int16_t  fSilentSav;
    jmp_buf *penvMemSav;
    char    *psz;
    int16_t  cch;
    int32_t  l;
    float    pct;
    int32_t  rgl[4];
    PART     part;

    fSilentSav = fFileErrSilent;
    fSuccess = TRUE;
    fOpen = FALSE;
    if (game.lid == 0 || idPlayer == iplrNone) {
        fSuccess = FALSE;
        goto DisplayStatus;
    } else {
        penvMemSav = penvMem;
        penvMem = &env;
        if (setjmp(env) != 0) {
            if (fOpen) {
                StreamClose();
            }
            fFileErrSilent = fSilentSav;
            fSuccess = FALSE;
            penvMem = penvMemSav;
            goto DisplayStatus;
        } else {
            fFileErrSilent = TRUE;
            if (gd.fPerPlayerDumps) {
                _wsprintf(szFile, "%s.p%d", szBase, idPlayer + 1);
            } else {
                _wsprintf(szFile, "%s.pla", szBase);
            }
            StreamOpen(szFile, mdCreate);
            fOpen = TRUE;
            j = gd.fPerPlayerDumps + 2;
            for (i = 0; i < j; i++) {
                cch = CchGetString(i + 1244, szForm);
                for (psz = szForm; *psz != 0; psz++) {
                    if (*psz == '*') {
                        *psz = '\t';
                    }
                }
                RgToStream(szForm, cch);
                if (i == j - 1) {
                    RgToStream(szCRLF, 2);
                }
            }
            lppl = lpPlanets;
            lpplMac = lpPlanets + cPlanet;
            for (; lppl < lpplMac; lppl++) {
                strcpy(szForm, PszGetCompressedPlanet(rgidPlan[lppl->id]));
                RgToStream(szForm, strlen(szForm));
                szForm[0] = '\t';
                if (lppl->iPlayer == iplrNone) {
                    RgToStream(szForm, 1);
                } else {
                    strcpy(&szForm[1], PszPlayerName(lppl->iPlayer, TRUE, FALSE, FALSE, 0, NULL));
                    RgToStream(szForm, strlen(szForm));
                }
                if (lppl->iPlayer == iplrNone || !lppl->fStarbase) {
                    cch = 1;
                } else {
                    fstrcpy(&szForm[1], rglpshdefSB[lppl->iPlayer][lppl->isb].hul.szClass);
                    cch = strlen(szForm);
                }
                RgToStream(szForm, cch);
                itoa(game.turn - lppl->turn, &szForm[1], 10);
                RgToStream(szForm, strlen(szForm));
                szForm[1] = 0;
                if (lppl->det == detAll) {
                    strcpy(&szForm[1], PszFromLong((uint32_t)(lppl->rgwtMin[3] * 100), NULL));
                } else if (lppl->iPlayer != iplrNone && lppl->det >= detSome) {
                    l = (uint32_t)(lppl->uPopGuess * 400);
                    strcpy(&szForm[1], PszFromLong(l, NULL));
                }
                RgToStream(szForm, strlen(szForm));
                if (lppl->det < detSome) {
                    szForm[1] = 0;
                } else {
                    i = PctPlanetDesirability(lppl, idPlayer);
                    _wsprintf(&szForm[1], PCTDPCTPCT, i);
                }
                RgToStream(szForm, strlen(szForm));
                szForm[1] = 0;
                if (lppl->det == detAll) {
                    FillPlanetProdLB(NULL, NULL, lppl);
                    strcpy(&szForm[1], szWork);
                }
                RgToStream(szForm, strlen(szForm));
                if (lppl->det == detAll) {
                    CalcPctSurvive(lppl, &pct, NULL);
                    pct = (float)((long double)1.0 - pct);
                    _wsprintf(&szForm[1], "%ld\t%ld\t%d.%d%%", (int32_t)lppl->cMines, (int32_t)lppl->cFactories, LOWORD((int32_t)((long double)pct * 100)),
                              LOWORD((int32_t)((pct - (long double)(int16_t)LOWORD((int32_t)((long double)pct * 100)) / 100.0) * 10000)));
                } else {
                    szForm[2] = '\t';
                    szForm[1] = '\t';
                    szForm[3] = 0;
                    if (gd.fPerPlayerDumps && lppl->uDefGuess != 0) {
                        cch = _wsprintf(&szForm[3], "%d%%", lppl->uDefGuess * 6 + 3);
                        szForm[cch + 3] = 0;
                    }
                }
                RgToStream(szForm, strlen(szForm));
                szForm[1] = 0;
                for (i = 0; i < 3; i++) {
                    if (lppl->det >= detSome) {
                        strcpy(&szForm[1], PszFromLong(lppl->rgwtMin[i], NULL));
                    }
                    RgToStream(szForm, strlen(szForm));
                }
                szForm[1] = 0;
                for (i = 0; i < 3; i++) {
                    if (lppl->det >= detMore) {
                        EstMineralsMined(lppl, rgl, -1, FALSE);
                        strcpy(&szForm[1], PszFromLong(rgl[i], NULL));
                    }
                    RgToStream(szForm, strlen(szForm));
                }
                szForm[1] = 0;
                for (i = 0; i < 3; i++) {
                    if (lppl->det >= detSome) {
                        strcpy(&szForm[1], PszFromInt(lppl->rgMinConc[i], NULL));
                    }
                    RgToStream(szForm, strlen(szForm));
                }
                if (lppl->det == detAll) {
                    strcpy(&szForm[1], PszFromInt(CResourcesAtPlanet(lppl, idPlayer), NULL));
                } else {
                    szForm[1] = 0;
                }
                RgToStream(szForm, strlen(szForm));
                if (gd.fPerPlayerDumps) {
                    if (lppl->det >= detSome) {
                        for (i = 0; i < 3; i++) {
                            strcpy(&szForm[1], PszCalcEnvVar(i, lppl->rgEnvVar[i]));
                            RgToStream(szForm, strlen(szForm));
                        }
                        for (i = 0; i < 3; i++) {
                            strcpy(&szForm[1], PszCalcEnvVar(i, lppl->rgEnvVarOrig[i]));
                            RgToStream(szForm, strlen(szForm));
                        }
                        strcpy(&szForm[1], PszFromInt(PctPlanetOptValue(lppl, idPlayer), NULL));
                        strcat(&szForm[1], "%");
                        RgToStream(szForm, strlen(szForm));
                    } else {
                        szForm[1] = 0;
                        for (i = 0; i < 7; i++) {
                            RgToStream(szForm, 1);
                        }
                    }
                    if (lppl->det == detAll) {
                        strcpy(&szForm[1], PszFromInt(PctPlanetCapacity(lppl), NULL));
                        RgToStream(szForm, strlen(szForm));
                        strcpy(&szForm[1], PszFromInt(GetPlanetScannerRange(lppl, &i), NULL));
                        RgToStream(szForm, strlen(szForm));
                        strcpy(&szForm[1], PszFromInt(i, NULL));
                        RgToStream(szForm, strlen(szForm));
                        if (lppl->idFling == 0) {
                            i = 0;
                            szForm[1] = 0;
                        } else {
                            i = lppl->iWarpFling + 4;
                            strcpy(&szForm[1], PszGetPlanetName(lppl->idFling - 1));
                        }
                        RgToStream(szForm, strlen(szForm));
                        strcpy(&szForm[1], PszFromInt(i, NULL));
                        RgToStream(szForm, strlen(szForm));
                        if (lppl->idRoute == 0) {
                            szForm[1] = 0;
                        } else {
                            strcpy(&szForm[1], PszGetPlanetName(lppl->idRoute - 1));
                        }
                        RgToStream(szForm, strlen(szForm));
                        if (lppl->fStarbase) {
                            i = IStargateFromLppl(lppl);
                            if (i != -1) {
                                part.hs.grhst = hstSpecialSB;
                                part.hs.iItem = i;
                                FLookupPart(&part);
                                strcpy(&szForm[1], PszFromInt(part.pspecialsb->grAbility2, NULL));
                                RgToStream(szForm, strlen(szForm));
                                strcpy(&szForm[1], PszFromInt(part.pspecialsb->grAbility, NULL));
                                RgToStream(szForm, strlen(szForm));
                            }
                        } else {
                            i = -1;
                        }
                        if (i == -1) {
                            RgToStream("\t0\t0", 4);
                        }
                        if (lppl->fStarbase) {
                            i = lppl->pctDp;
                        } else {
                            i = 0;
                        }
                        strcpy(&szForm[1], PszFromInt(i, NULL));
                        RgToStream(szForm, strlen(szForm));
                    }
                }
                RgToStream(szCRLF, 2);
            }
            StreamClose();
        }
    }
DisplayStatus:
    ids = !fSuccess ? idsUnableWritePlanetInformationSOperationTerminated : idsKnownPlanetInformationHasSuccessfullyWrittenS;
    _wsprintf(szWork, PszGetCompressedString(ids), szFile);
    if (fSuccess) {
        AlertSz(szWork, MB_ICONASTERISK);
    } else {
        AlertSz(szWork, MB_ICONHAND);
    }
    fFileErrSilent = fSilentSav;
    penvMem = penvMemSav;
    return;
}

void DumpFleets() {
    int16_t  iplr;
    StringId ids;
    char     szFile[256];
    char     szForm[256];
    int16_t  ifl;
    FLEET   *lpfl;
    int16_t  j;
    int16_t  i;
    jmp_buf  env;
    int16_t  fOpen;
    int16_t  fSuccess;
    int16_t  fSilentSav;
    jmp_buf *penvMemSav;
    char    *psz;
    int16_t  cch;
    int32_t  l;

    fSilentSav = fFileErrSilent;
    fSuccess = TRUE;
    fOpen = FALSE;
    iplr = idPlayer;
    if (game.lid == 0 || idPlayer == iplrNone) {
        fSuccess = FALSE;
        goto DisplayStatus;
    } else {
        penvMemSav = penvMem;
        penvMem = &env;
        if (setjmp(env) != 0) {
            if (fOpen) {
                StreamClose();
            }
            fFileErrSilent = fSilentSav;
            fSuccess = FALSE;
            penvMem = penvMemSav;
            goto DisplayStatus;
        } else {
            fFileErrSilent = TRUE;
            if (gd.fPerPlayerDumps) {
                _wsprintf(szFile, "%s.f%d", szBase, idPlayer + 1);
            } else {
                _wsprintf(szFile, "%s.fle", szBase);
            }
            StreamOpen(szFile, mdCreate);
            fOpen = TRUE;
            j = gd.fPerPlayerDumps + 2;
            for (i = 0; i < j; i++) {
                cch = CchGetString(i + 1247, szForm);
                for (psz = szForm; *psz != 0; psz++) {
                    if (*psz == '*') {
                        *psz = '\t';
                    }
                }
                RgToStream(szForm, cch);
                if (i == j - 1) {
                    RgToStream(szCRLF, 2);
                }
            }
            iplr = idPlayer;
            for (ifl = 0; ifl < cFleet; ifl++) {
                lpfl = rglpfl[ifl];
                if (!rglpfl[ifl])
                    break;
                idPlayer = iplrNone;
                psz = PszGetFleetName(lpfl->id);
                idPlayer = iplr;
                RgToStream(psz, strlen(psz));
                szForm[0] = '\t';
                strcpy(&szForm[1], PszFromInt(lpfl->pt.x, NULL));
                RgToStream(szForm, strlen(szForm));
                strcpy(&szForm[1], PszFromInt(lpfl->pt.y, NULL));
                RgToStream(szForm, strlen(szForm));
                if (lpfl->idPlanet == idPlanetDeepSpace) {
                    cch = 1;
                } else {
                    psz = PszGetPlanetName(lpfl->idPlanet);
                    strcpy(&szForm[1], psz);
                    cch = strlen(psz) + 1;
                }
                RgToStream(szForm, cch);
                if (lpfl->det == detAll) {
                    strcpy(&szForm[1], PszGetDestName(lpfl, NULL));
                } else if (gd.fPerPlayerDumps && lpfl->det < detAll && lpfl->fdirValid) {
                    strcpy(&szForm[1], PszFromInt(lpfl->dirFltX, NULL));
                    strcat(szForm, ".");
                    strcat(szForm, PszFromInt(lpfl->dirFltY, NULL));
                } else {
                    szForm[1] = 0;
                }
                RgToStream(szForm, strlen(szForm));
                if (rglpbtlplan[lpfl->iplr] != 0) {
                    fstrcpy(&szForm[1], rglpbtlplan[lpfl->iplr][lpfl->iplan].szName);
                } else {
                    szForm[1] = 0;
                }
                RgToStream(szForm, strlen(szForm));
                l = 0;
                for (i = 0; i < 16; i++) {
                    l += lpfl->rgcsh[i];
                }
                strcpy(&szForm[1], PszFromLong(l, NULL));
                RgToStream(szForm, strlen(szForm));
                for (i = 0; i < 5; i++) {
                    strcpy(&szForm[1], PszFromLong(lpfl->rgwtMin[i], NULL));
                    RgToStream(szForm, strlen(szForm));
                }
                if (gd.fPerPlayerDumps) {
                    strcpy(&szForm[1], PszFromInt(lpfl->iPlayer + 1, NULL));
                    RgToStream(szForm, strlen(szForm));
                    if (lpfl->cord > 1) {
                        strcpy(&szForm[1], PszGetETA(NULL, lpfl, NULL));
                    } else {
                        szForm[1] = '0';
                        szForm[2] = 0;
                    }
                    RgToStream(szForm, strlen(szForm));
                    if (lpfl->cord >= 2) {
                        i = lpfl->lpplord->rgord[1].iWarp;
                    } else if (lpfl->det < detAll && lpfl->fdirValid) {
                        i = lpfl->iwarpFlt;
                    } else {
                        i = 0;
                    }
                    strcpy(&szForm[1], PszFromInt(i, NULL));
                    RgToStream(szForm, strlen(szForm));
                    strcpy(&szForm[1], PszFromLong(WtFromLpfl(lpfl), NULL));
                    RgToStream(szForm, strlen(szForm));
                    strcpy(&szForm[1], PszFromInt(PctCloakFromLpfl(lpfl), NULL));
                    RgToStream(szForm, strlen(szForm));
                    j = GetFleetScannerRange(lpfl, &i, NULL, NULL);
                    if (j == -1) {
                        j = 0;
                    }
                    strcpy(&szForm[1], PszFromInt(j, NULL));
                    RgToStream(szForm, strlen(szForm));
                    strcpy(&szForm[1], PszFromInt(i, NULL));
                    RgToStream(szForm, strlen(szForm));
                    if (lpfl->det == detAll) {
                        strcpy(&szForm[1], PszGetTaskName(lpfl, &i));
                    } else {
                        szForm[1] = 0;
                    }
                    RgToStream(szForm, strlen(szForm));
                    strcpy(&szForm[1], PszFromLong(CMineFromLpfl(lpfl), NULL));
                    RgToStream(szForm, strlen(szForm));
                    strcpy(&szForm[1], PszFromLong(CMineSweepFromLpfl(lpfl), NULL));
                    RgToStream(szForm, strlen(szForm));
                    strcpy(&szForm[1], PszFromLong(CLayMinesFromLpfl(lpfl, mineAll, ishdefAll), NULL));
                    RgToStream(szForm, strlen(szForm));
                    strcpy(&szForm[1], PszFromLong(PctTerraFromLpfl(lpfl), NULL));
                    RgToStream(szForm, strlen(szForm));
                    l = 0;
                    for (i = 0; i < 16; i++) {
                        if (lpfl->rgcsh[i] != 0) {
                            j = LphuldefFromId(rglpshdef[lpfl->iPlayer][i].hul.ihuldef)->imdCategory;
                            if (j <= 1 || j >= 6) {
                                l += lpfl->rgcsh[i];
                            }
                        }
                    }
                    strcpy(&szForm[1], PszFromLong(l, NULL));
                    RgToStream(szForm, strlen(szForm));
                    for (j = 2; j < 6; j++) {
                        l = 0;
                        for (i = 0; i < 16; i++) {
                            if (lpfl->rgcsh[i] != 0 && LphuldefFromId(rglpshdef[lpfl->iPlayer][i].hul.ihuldef)->imdCategory == j) {
                                l += lpfl->rgcsh[i];
                            }
                        }
                        strcpy(&szForm[1], PszFromLong(l, NULL));
                        RgToStream(szForm, strlen(szForm));
                    }
                }
                RgToStream(szCRLF, 2);
            }
            StreamClose();
        }
    }
DisplayStatus:
    ids = !fSuccess ? idsUnableWriteFleetInformationSOperationTerminated : idsKnownFleetInformationHasSuccessfullyWrittenS;
    _wsprintf(szWork, PszGetCompressedString(ids), szFile);
    if (fSuccess) {
        AlertSz(szWork, MB_ICONASTERISK);
    } else {
        AlertSz(szWork, MB_ICONHAND);
    }
    fFileErrSilent = fSilentSav;
    penvMem = penvMemSav;
    return;
}

INT_PTR CALLBACK PrintMapDlg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    int16_t i;
    RECT    rc;
    HWND    hwndEdit;

    switch (IS_WM_CTLCOLOR(msg) ? WM_CTLCOLOR : msg) { /* NATIVE: Win32 split WM_CTLCOLOR by control type. */
    case WM_INITDIALOG:
        for (i = 0; i < 2; i++) {
            hwndEdit = GetDlgItem(hwnd, i + 268);
            SendMessage(hwndEdit, EM_LIMITTEXT, 1, 0);
            SendMessage(hwndEdit, WM_SETFONT, (WPARAM)rghfontArial8[1], 0);
            szWork[0] = vrgcPrintMapPage[i] + '0';
            szWork[1] = 0;
            SetWindowText(hwndEdit, szWork);
        }
        StickyDlgPos(hwnd, &ptStickyPrintMapDlg, TRUE);
        return 1;
    case WM_ERASEBKGND:
        GetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case WM_CTLCOLOR:
        if (msg == WM_CTLCOLORSTATIC /* NATIVE: Win16 CTLCOLOR_STATIC was in HIWORD(lParam). */) {
            SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDOK:
        case IDCANCEL:
            if (GET_WM_COMMAND_ID(wParam, lParam) == IDOK) {
                for (i = 0; i < 2; i++) {
                    hwndEdit = GetDlgItem(hwnd, i + 268);
                    GetWindowText(hwndEdit, szWork, 10);
                    if (szWork[0] == 0 || szWork[1] != 0 || szWork[0] <= '0' || szWork[0] > '9') {
                        AlertSz(PszFormatIds(idsMustSpecifyNumberBetween19, NULL), MB_ICONHAND);
                        SetFocus(hwndEdit);
                        break;
                    }
                    vrgcPrintMapPage[i] = szWork[0] - '0';
                }
            }
            StickyDlgPos(hwnd, &ptStickyPrintMapDlg, FALSE);
            EndDialog(hwnd, GET_WM_COMMAND_ID(wParam, lParam) == IDOK);
            return 1;
        case IDC_HELP:
            WinHelp(hwnd, szHelpFile, HELP_CONTEXT, idhPrintingAMapOfTheUniverse);
            return 1;
        case IDC_PRINT_MAP_PAGES_X:
        case IDC_PRINT_MAP_PAGES_Y:
            if (GET_WM_COMMAND_CMD(wParam, lParam) == 0x400) {
                GetWindowText(GET_WM_COMMAND_HWND(wParam, lParam), szWork, 10);
                if (szWork[0] != 0 && (szWork[0] <= '0' || szWork[0] > '9')) {
                    MessageBeep(MB_OK);
                    SetWindowText(GET_WM_COMMAND_HWND(wParam, lParam), &szWork[1]);
                }
            }
        }
    }
    return 0;
}
