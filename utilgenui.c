#include "win.h"

INT_PTR CALLBACK RandomSeedDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    char     szValue[33];
    char    *pch;
    uint32_t dw;
    RECT     rc;

    switch (message) {
    case WM_INITDIALOG:
        SetWindowPos(hwnd, NULL, 256, 256, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        SendDlgItemMessage(hwnd, IDC_EDIT1, EM_LIMITTEXT, 0x1f, 0);
        return 1;
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
        if (message == WM_CTLCOLORSTATIC) {
            SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            if (LOWORD(wParam) == IDOK) {
                GetDlgItemText(hwnd, IDC_EDIT1, szValue, 32);
                pch = szValue;
                dw = 0;
                for (; isdigit(*pch) != 0; pch++) {
                    dw = (uint32_t)(dw * 10) + (int16_t)(*pch - '0');
                }
                if (dw == 0) {
                    return 1;
                }
                Randomize(dw);
            }
            EndDialog(hwnd, LOWORD(wParam) == IDOK);
            return 1;
        }
        break;
    }
    return 0;
}

void CtrTextOut(HDC hdc, int16_t x, int16_t y, char *psz, int16_t cLen) {
    int16_t dx;

    if (cLen == 0) {
        cLen = lstrlen(psz);
    }
    dx = LOWORD(GetTextExtent(hdc, psz, cLen));
    TextOut(hdc, x - (dx >> 1), y, psz, cLen);
    return;
}

int16_t DxStreamTextOut(HDC hdc, int16_t *px, int16_t y, char *psz, int16_t cLen, int16_t fPrint) {
    int16_t dx;

    if (cLen == 0) {
        cLen = lstrlen(psz);
    }
    dx = LOWORD(GetTextExtent(hdc, psz, cLen));
    if (fPrint) {
        TextOut(hdc, *px, y, psz, cLen);
    }
    *px += dx;
    return dx;
}

void WrapTextOut(HDC hdc, int16_t *px, int16_t *py, char *psz, int16_t cLen, int16_t xLeft, int16_t dxWidth, int16_t *pxMax, int16_t fNewLine, int16_t fPrint) {
    int16_t dxRemain;
    char   *pchEnd;
    int16_t fItFit;
    int16_t xRight;
    int16_t dx;
    char   *pchStart;
    char   *pch;

    xRight = xLeft + dxWidth;
    dxRemain = dxWidth - (*px - xLeft);
    if (cLen == 0) {
        cLen = strlen(psz);
    }
    if (fNewLine) {
        *py += dyArial8;
        *px = xLeft;
    }
    pchStart = psz;
Top:
    pch = pchStart;
    pchEnd = pchStart + cLen;
    ChopTrailingSpaces(pch, &pchEnd);
    dx = LOWORD(GetTextExtent(hdc, pch, pchEnd - pch));
    fItFit = TRUE;
    for (; dx > dxRemain && pch < pchEnd && dx > 0; dx = LOWORD(GetTextExtent(hdc, pch, pchEnd - pch))) {
        fItFit = FALSE;
        ChopLastWord(pch, &pchEnd);
    }
    if (fItFit) {
        AddBackTrailingSpaces(&pchEnd, pchStart + cLen);
        dx = LOWORD(GetTextExtent(hdc, pchStart, pchEnd - pchStart));
    }
    if (pchStart == pchEnd) {
        if (*px != xLeft)
            goto WrapIt;
        pchEnd = pchStart + cLen;
        dx = LOWORD(GetTextExtent(hdc, pchStart, pchEnd - pchStart));
    }
    if (fPrint) {
        TextOut(hdc, *px, *py, pchStart, pchEnd - pchStart);
    }
    *px += dx;
    if (pxMax && *px > *pxMax) {
        *pxMax = *px;
    }
    if (pchEnd == pchStart + cLen)
        goto Done;
WrapIt:
    AddBackTrailingSpaces(&pchEnd, pchStart + cLen);
    cLen -= pchEnd - pchStart;
    pchStart = pchEnd;
    *py += dyArial8;
    *px = xLeft;
    dxRemain = dxWidth;

    goto Top;
Done:
    return;
}

void AddBackTrailingSpaces(char **ppch, char *pchEnd) {
    while (*ppch < pchEnd && **ppch == ' ') {
        (*ppch)++;
    }
    return;
}

void ChopLastWord(char *pBeg, char **ppEnd) {
    while (*ppEnd > pBeg && (*ppEnd)[-1] == ' ') {
        (*ppEnd)--;
    }
    while (*ppEnd > pBeg && (*ppEnd)[-1] != ' ') {
        (*ppEnd)--;
    }
    while (*ppEnd > pBeg && (*ppEnd)[-1] == ' ') {
        (*ppEnd)--;
    }
    return;
}

void ChopTrailingSpaces(char *pBeg, char **ppEnd) {
    while (*ppEnd > pBeg && (*ppEnd)[-1] == ' ') {
        (*ppEnd)--;
    }
    return;
}

void RcCtrTextOut(HDC hdc, RECT *prc, char *psz, int16_t cLen) {
    int16_t y;
    int16_t x;
    int32_t l;

    if (cLen == -1) {
        FillRect(hdc, prc, hbrButtonFace);
        cLen = 0;
    }
    if (cLen == 0) {
        cLen = lstrlen(psz);
    }
    l = GetTextExtent(hdc, psz, cLen);
    x = (int16_t)(prc->right - prc->left - LOWORD(l)) / 2 + prc->left;
    y = (int16_t)(prc->bottom - prc->top - HIWORD(l)) / 2 + prc->top;
    TextOut(hdc, x, y, psz, cLen);
    return;
}

void RightTextOut(HDC hdc, int16_t x, int16_t y, char *psz, int16_t cLen, int16_t dxErase) {
    int16_t dx;
    RECT    rc;

    if (cLen == -1) {
        cLen = CchGetString(idsNone2, psz);
    } else if (cLen == 0) {
        cLen = lstrlen(psz);
    }
    dx = LOWORD(GetTextExtent(hdc, psz, cLen));
    if (dxErase > dx) {
        SetRect(&rc, x - dxErase, y, x - dx, y + dyArial8);
        FillRect(hdc, &rc, hbrButtonFace);
    } else if (dxErase > 0 && dx > dxErase) {
        SetRect(&rc, x - dxErase, y, x, y + dyArial8);
        ExtTextOut(hdc, x - dx, y, ETO_OPAQUE | ETO_CLIPPED, &rc, psz, cLen, NULL);
        return;
    }
    TextOut(hdc, x - dx, y, psz, cLen);
    return;
}

void DiaganolTextOut(HDC hdc, RECT *prc, char *psz, int16_t cLen) {
    double   angle;
    HFONT    hfont;
    int16_t  dxFlat;
    int16_t  yStart;
    int16_t  dy;
    double   dcos;
    int16_t  dyText;
    int16_t  xStart;
    int16_t  dyEstFont;
    int16_t  dxText;
    LOGFONT *plf;
    double   dsin;
    HFONT    hfontSav;
    int16_t  dx;
    int16_t  dyFlat;
    double   rotate;
    int32_t  l;
    int16_t  dHtX;
    int16_t  dHtY;

    if (cLen == 0) {
        cLen = strlen(psz);
    }
    dx = prc->right - prc->left;
    dy = prc->bottom - prc->top;
    if (dx >= 10 && dy >= 10) {
        plf = LocalAlloc(64, sizeof(LOGFONT));
        plf->lfWeight = 900;
        strcpy(plf->lfFaceName, rgszArial[1]);
        plf->lfHeight = -(dx <= dy ? dy : dx);
    TryAgain:
        if (plf->lfHeight > -5)
            goto FreeLF;

        dyEstFont = MulDiv(111, -plf->lfHeight, 100);
        if (dy < dyEstFont) {
            plf->lfHeight += 2 <= (int16_t)(dyEstFont - dy) / 2 ? (int16_t)(dyEstFont - dy) / 2 : 2;
            goto TryAgain;
        }
        angle = (double)atan2((double)(int16_t)(dy - dyEstFont), (double)dx);
        rotate = (double)((long double)angle / 3.141592654 * 1800.0 + 0.5);
        plf->lfEscapement = LOWORD((int32_t)rotate);
        hfont = CreateFontIndirect(plf);
        hfontSav = SelectObject(hdc, hfont);
        l = GetTextExtent(hdc, psz, cLen);
        dxText = LOWORD(l);
        dyText = HIWORD(l);
        dsin = (double)sin(angle);
        dcos = (double)cos(angle);
        dxFlat = LOWORD((int32_t)((long double)dcos * dxText + (long double)dsin * dyText));
        dyFlat = LOWORD((int32_t)((long double)dsin * dxText + (long double)dcos * dyText));
        if (dxFlat + 8 > dx || dyFlat + 8 > dy) {
            SelectObject(hdc, hfontSav);
            DeleteObject(hfont);
            if (dxFlat + 8 > dx) {
                dHtX = MulDiv(plf->lfHeight, dx, dxFlat + 8);
                dHtX = MulDiv(dHtX, 100, 111);
            } else {
                dHtX = -1000;
            }
            if (dyFlat + 8 > dy) {
                dHtY = MulDiv(plf->lfHeight, dy, dyFlat + 8);
                dHtY = MulDiv(dHtY, 100, 111);
            } else {
                dHtY = -1000;
            }
            plf->lfHeight += max(2, max(dHtX, dHtY) - plf->lfHeight) + 1;
            goto TryAgain;
        }
        xStart = (int16_t)(dx - dxFlat) / 2 + prc->left;
        yStart = prc->bottom - (int16_t)(dy - dyFlat) / 2 - LOWORD((int32_t)((long double)dcos * dyText));
        TextOut(hdc, xStart, yStart, psz, cLen);
        SelectObject(hdc, hfontSav);
        DeleteObject(hfont);

    FreeLF:
        LocalFree(plf);
    }
    return;
}

void ExpandRc(RECT *prc, int16_t dx, int16_t dy) {
    prc->left -= dx;
    prc->right += dx;
    prc->top -= dy;
    prc->bottom += dy;
    return;
}

void OffsetRc(RECT *prc, int16_t dx, int16_t dy) {
    prc->left += dx;
    prc->right += dx;
    prc->top += dy;
    prc->bottom += dy;
    return;
}

void StickyDlgPos(HWND hwnd, POINT16 *ppt, int16_t fInit) {
    POINT16 ptScreenMax;
    RECT    rc;

    GetWindowRect(hwnd, &rc);
    if (!fInit) {
        ppt->x = rc.left;
        ppt->y = rc.top;
    } else {
        ptScreenMax.x = GetSystemMetrics(SM_CXSCREEN);
        ptScreenMax.y = GetSystemMetrics(SM_CYSCREEN);
        if (ppt->x == -1 && ppt->y == -1) {
            rc.left = (ptScreenMax.x - (rc.right - rc.left)) >> 1;
            rc.top = (ptScreenMax.y - (rc.bottom - rc.top)) >> 1;
        } else {
            OffsetRect(&rc, ppt->x - rc.left, ppt->y - rc.top);
            if (rc.right > ptScreenMax.x) {
                rc.left += ptScreenMax.x - rc.right;
            }
            if (rc.bottom > ptScreenMax.y) {
                rc.top += ptScreenMax.y - rc.bottom;
            }
            if (rc.left < 0) {
                rc.left = 0;
            }
            if (rc.top < 0) {
                rc.top = 0;
            }
        }
        SetWindowPos(hwnd, NULL, rc.left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
    return;
}

int32_t LDrawGauge(HDC hdc, RECT *prc, int16_t cSegs, int32_t *rgSize, HBRUSH *rghbr, int32_t cTot) {
    int16_t fHuge = FALSE; // Empty gauges skip the scale calculation.
    int16_t i;
    int32_t lSum;
    int32_t dx;
    RECT    rc;

    lSum = 0;
    rc = *prc;
    FrameRect(hdc, &rc, hbrWindowText);
    ExpandRc(&rc, -1, -1);
    if (cTot <= 0)
        goto FinRet;

    fHuge = cTot >= 10000000;
    if (fHuge) {
        cTot = (int32_t)(cTot / 1000);
    }
    dx = (int16_t)(rc.right - rc.left);
    for (i = 0; i < cSegs; i++) {
        if (!fHuge) {
            lSum += rgSize[i];
        } else {
            lSum += (int32_t)(rgSize[i] / 1000);
        }
        rc.right = prc->left + 1 + LOWORD((int32_t)((int32_t)(dx * lSum) / cTot));
        if (rc.right > rc.left) {
            FillRect(hdc, &rc, rghbr[i]);
        }
        rc.left = rc.right;
    }

FinRet:
    rc.right = prc->right - 1;
    if (rc.right > rc.left) {
        FillRect(hdc, &rc, hbrButtonFace);
    }
    if (fHuge) {
        lSum = (uint32_t)(lSum * 1000);
    }
    return lSum;
}

void _Draw3dFrame(HDC hdc, RECT *prc, int16_t fErase) {
    int16_t dy;
    int16_t dx;
    HBRUSH  hbrSav;
    RECT    rc;

    rc = *prc;
    dx = prc->right - prc->left;
    dy = prc->bottom - prc->top;
    hbrSav = SelectObject(hdc, hbrButtonHilite);
    if (fErase == -1 || fErase == -2) {
        PatBlt(hdc, rc.left, rc.bottom, dx + 1, 1, PATCOPY);
        PatBlt(hdc, rc.right, rc.top, 1, dy, PATCOPY);
        SelectObject(hdc, hbrButtonShadow);
        PatBlt(hdc, rc.left, rc.top, dx, 1, PATCOPY);
        PatBlt(hdc, rc.left, rc.top, 1, dy, PATCOPY);
        ExpandRc(&rc, -1, -1);
        SelectObject(hdc, hbrButtonHilite);
        dx -= 2;
        dy -= 2;
    }
    PatBlt(hdc, rc.left, rc.top, dx, 1, PATCOPY);
    PatBlt(hdc, rc.left, rc.top, 1, dy, PATCOPY);
    SelectObject(hdc, hbrButtonShadow);
    PatBlt(hdc, rc.left, rc.bottom, dx + 1, 1, PATCOPY);
    PatBlt(hdc, rc.right, rc.top, 1, dy, PATCOPY);
    if (fErase && fErase != -2) {
        SelectObject(hdc, hbrButtonFace);
        PatBlt(hdc, rc.left + 1, rc.top + 1, dx - 1, dy - 1, PATCOPY);
    }
    SelectObject(hdc, hbrSav);
    return;
}

void InitBtnTrack(BTNT *pbtnt, HWND hwnd, HDC hdc, RECT *prc, int16_t btf, int16_t dTimer, int16_t fInitDown, int16_t fNoEndRedraw, char *szText) {
    pbtnt->hwnd = hwnd;
    pbtnt->fCreatedDC = hdc == NULL;
    if (!hdc) {
        hdc = GetDC(hwnd);
    }
    pbtnt->hdc = hdc;
    pbtnt->rc = *prc;
    pbtnt->btf = btf;
    pbtnt->dTimer = dTimer;
    pbtnt->fFirst = TRUE;
    pbtnt->fInitDown = fInitDown;
    pbtnt->fNoEndRedraw = fNoEndRedraw;
    pbtnt->fDown = TRUE;
    pbtnt->szText = szText;
    return;
}

int16_t FTrackBtn(BTNT *pbtnt) {
    POINT16 pt;
    int16_t fInBtn;
    int32_t ticksNew;

    if (pbtnt->fFirst) {
        SetCapture(pbtnt->hwnd);
        DrawBtn(pbtnt->hdc, &pbtnt->rc, pbtnt->btf, pbtnt->fDown ^ pbtnt->fInitDown, pbtnt->szText);
        pbtnt->lTicks = GetCurrentTime() + (int16_t)(3 * pbtnt->dTimer);
        pbtnt->fFirst = FALSE;
        return TRUE;
    }
    pt.x = pbtnt->rc.left;
    pt.y = pbtnt->rc.top;
    while (FGetMouseMove(&pt)) {
        fInBtn = PtInRect(&pbtnt->rc, PointFrom16(pt));
        if (fInBtn != pbtnt->fDown) {
            pbtnt->fDown = fInBtn;
            DrawBtn(pbtnt->hdc, &pbtnt->rc, pbtnt->btf, pbtnt->fDown ^ pbtnt->fInitDown, pbtnt->szText);
        }
        ticksNew = GetCurrentTime();
        if (ticksNew >= pbtnt->lTicks && pbtnt->fDown) {
            pbtnt->lTicks = pbtnt->dTimer + ticksNew;
            return TRUE;
        }
    }
    if (pbtnt->fDown && !pbtnt->fNoEndRedraw) {
        pbtnt->fDown = FALSE;
        DrawBtn(pbtnt->hdc, &pbtnt->rc, pbtnt->btf, pbtnt->fDown ^ pbtnt->fInitDown, pbtnt->szText);
        pbtnt->fDown = TRUE;
    }
    ReleaseCapture();
    if (pbtnt->fCreatedDC) {
        ReleaseDC(pbtnt->hwnd, pbtnt->hdc);
    }
    return FALSE;
}

void DrawBtn(HDC hdc, RECT *prc, int16_t bt, int16_t fDown, char *szText) {
    int32_t  dxFace;
    int16_t  ipt;
    int16_t  dyOffset;
    int16_t  d;
    int16_t  fBar;
    int16_t  fDisabled;
    int16_t  dxOffset;
    int16_t  dy;
    int16_t  fNoShaft;
    int16_t  y;
    POINT16  rgptDraw[6];
    HBRUSH   hbrCur;
    COLORREF crSav;
    int16_t  dx;
    HBRUSH   hbrSav;
    int16_t  cpt;
    int16_t  x;
    RECT     rc;
    int16_t  dxyT;
    int16_t  bkMode;
    HFONT    hfontSav;

    fDisabled = bt & 4;
    rc = *prc;
    hbrSav = SelectObject(hdc, hbrWindowFrame);
    if (!(bt & 0x40)) {
        FrameRect(hdc, &rc, hbrWindowFrame);
        ExpandRc(&rc, -1, -1);
    }
    dx = rc.right - rc.left - 1;
    dy = rc.bottom - rc.top - 1;
    dxFace = (int16_t)(prc->right - prc->left) - 5;
    SelectObject(hdc, !fDown ? hbrButtonHilite : hbrButtonShadow);
    PatBlt(hdc, rc.left, rc.top, dx, 1, PATCOPY);
    PatBlt(hdc, rc.left, rc.top, 1, dy, PATCOPY);
    SelectObject(hdc, !fDown ? hbrButtonShadow : hbrButtonFace);
    PatBlt(hdc, rc.left, rc.bottom - 1, dx + 1, 1, PATCOPY);
    PatBlt(hdc, rc.right - 1, rc.top, 1, dy, PATCOPY);
    if (dx < 14 || dy < 14 || (bt & 0xc0)) {
        ExpandRc(&rc, -1, -1);
    } else {
        PatBlt(hdc, rc.left + 1, rc.bottom - 2, dx - 1, 1, PATCOPY);
        PatBlt(hdc, rc.right - 2, rc.top + 1, 1, dy - 2, PATCOPY);
        SetRect(&rc, rc.left + 1, rc.top + 1, rc.right - 2, rc.bottom - 2);
    }
    SelectObject(hdc, hbrButtonFace);
    FillRect(hdc, &rc, hbrButtonFace);
    if (!(bt & 8)) {
        fBar = bt & 0x10;
        fNoShaft = bt & 0x20;
        bt &= 3;
        if (fNoShaft) {
            cpt = 3;
            memcpy(rgptDraw, rgptTriangle, cpt * 4);
            dx = 8;
            dy = 4;
        } else {
            cpt = 5;
            memcpy(rgptDraw, rgptArrow, cpt * 4);
            dx = 6;
            dy = 6;
        }
        if (fBar) {
            for (ipt = 0; ipt < cpt; ipt++) {
                rgptDraw[ipt].y++;
            }
            dy += 2;
            rgptDraw[cpt].y = 0;
            rgptDraw[cpt].x = 0;
            cpt++;
        }
        for (ipt = 0; ipt < cpt; ipt++) {
            if (rgptDraw[ipt].x >= 0) {
                rgptDraw[ipt].x = LOWORD((int32_t)(rgptDraw[ipt].x * dxFace) / 11);
            } else {
                rgptDraw[ipt].x = rgptDraw[0].x * 2 - rgptDraw[-rgptDraw[ipt].x].x;
            }
            rgptDraw[ipt].y = LOWORD((int32_t)(rgptDraw[ipt].y * dxFace) / 11);
        }
        dx = LOWORD((int32_t)(dx * dxFace) / 11);
        dy = LOWORD((int32_t)(dy * dxFace) / 11);
        dxOffset = ((prc->right - prc->left - dx) >> 1) + fDown;
        dyOffset = ((prc->bottom - prc->top - dy) >> 1) + fDown;
        if (bt == 2 || bt == 3) {
            dxyT = dxOffset;
            dxOffset = dyOffset;
            dyOffset = dxyT;
        }
        for (ipt = 0; ipt < cpt; ipt++) {
            if (bt == 1) {
                rgptDraw[ipt].y = dy - rgptDraw[ipt].y;
            }
            if (bt == 2 || bt == 3) {
                d = dx - rgptDraw[ipt].x;
                rgptDraw[ipt].x = rgptDraw[ipt].y;
                rgptDraw[ipt].y = d;
                if (bt == 3) {
                    rgptDraw[ipt].x = dy - rgptDraw[ipt].x;
                }
            }
            rgptDraw[ipt].x += prc->left + dxOffset;
            rgptDraw[ipt].y += prc->top + dyOffset;
        }
        hbrCur = !fDisabled ? hbrButtonText : hbrButtonHilite;
        SelectObject(hdc, hbrCur);
    DrawAgain:
        if (bt == 0 || bt == 1) {
            dy = bt == 0 ? -1 : 1;
            x = rgptDraw[1].x;
            dx = rgptDraw[2].x - x + 1;
            y = rgptDraw[1].y;
            if (fBar) {
                PatBlt(hdc, x, rgptDraw[cpt - 1].y, dx, 1, PATCOPY);
            }
            while (dx > 0) {
                PatBlt(hdc, x, y, dx, 1, PATCOPY);
                x++;
                dx -= 2;
                y += dy;
            }
        } else {
            dx = bt == 2 ? -1 : 1;
            y = rgptDraw[2].y;
            dy = rgptDraw[1].y - y + 1;
            x = rgptDraw[1].x;
            if (fBar) {
                PatBlt(hdc, rgptDraw[cpt - 1].x, y, 1, dy, PATCOPY);
            }
            while (dy > 0) {
                PatBlt(hdc, x, y, 1, dy, PATCOPY);
                y++;
                dy -= 2;
                x += dx;
            }
        }
        if (!fNoShaft) {
            SetRect(&rc, rgptDraw[3].x >= rgptDraw[4].x ? rgptDraw[4].x : rgptDraw[3].x, rgptDraw[3].y >= rgptDraw[4].y ? rgptDraw[4].y : rgptDraw[3].y,
                    (rgptDraw[3].x <= rgptDraw[4].x ? rgptDraw[4].x : rgptDraw[3].x) + 1, (rgptDraw[3].y <= rgptDraw[4].y ? rgptDraw[4].y : rgptDraw[3].y) + 1);
            FillRect(hdc, &rc, hbrCur);
        }
        if (fDisabled) {
            for (ipt = 0; ipt < cpt; ipt++) {
                rgptDraw[ipt].x--;
                rgptDraw[ipt].y--;
            }
            fDisabled = FALSE;
            hbrCur = hbrButtonShadow;
            SelectObject(hdc, hbrButtonShadow);
            goto DrawAgain;
        }
    }
    SelectObject(hdc, hbrSav);
    if (szText) {
        hfontSav = SelectObject(hdc, rghfontArial8[1]);
        bkMode = SetBkMode(hdc, TRANSPARENT);
        crSav = SetTextColor(hdc, !fDisabled ? crButtonText : crButtonHilite);
        rc = *prc;
        if (fDown) {
            OffsetRc(&rc, 1, 1);
        }
        RcCtrTextOut(hdc, &rc, szText, 0);
        if (fDisabled) {
            OffsetRc(&rc, -1, -1);
            SetTextColor(hdc, crButtonShadow);
            RcCtrTextOut(hdc, &rc, szText, 0);
        }
        SetTextColor(hdc, crSav);
        SetBkMode(hdc, bkMode);
        SelectObject(hdc, hfontSav);
    }
    return;
}

int16_t FGetMouseMove(POINT16 *ppt) {
    MSG msg;

    while (PeekMessage(&msg, NULL, 0, 0, 1) != 0) {
        if (msg.message == WM_MOUSEMOVE || msg.message == WM_LBUTTONUP) {
            ppt->x = LOWORD(msg.lParam);
            ppt->y = HIWORD(msg.lParam);
            if (msg.message != WM_LBUTTONUP) {
                return TRUE;
            }
            return FALSE;
        }
    }
    return TRUE;
}

int16_t FGetRMouseMove(POINT16 *ppt) {
    MSG msg;

    while (PeekMessage(&msg, NULL, 0, 0, 1) != 0) {
        if (msg.message == WM_MOUSEMOVE || msg.message == WM_RBUTTONUP) {
            ppt->x = LOWORD(msg.lParam);
            ppt->y = HIWORD(msg.lParam);
            if (msg.message != WM_RBUTTONUP) {
                return TRUE;
            }
            return FALSE;
        }
    }
    return TRUE;
}

void DrawFuzzyBorder(HDC hdc, RECT *prc) {
    COLORREF crBack;
    int16_t  dy;
    int16_t  dx;
    HBRUSH   hbrSav;
    COLORREF crFore;

    crBack = SetBkColor(hdc, crButtonFace);
    crFore = SetTextColor(hdc, crWindowText);
    hbrSav = SelectObject(hdc, hbr50Screen);
    dx = prc->right - prc->left;
    dy = prc->bottom - prc->top;
    PatBlt(hdc, prc->left, prc->top, dx, 2, PATINVERT);
    PatBlt(hdc, prc->left, prc->bottom - 2, dx, 2, PATINVERT);
    PatBlt(hdc, prc->left, prc->top + 2, 2, dy - 4, PATINVERT);
    PatBlt(hdc, prc->right - 2, prc->top + 2, 2, dy - 4, PATINVERT);
    SelectObject(hdc, hbrSav);
    SetTextColor(hdc, crFore);
    SetBkColor(hdc, crBack);
    return;
}

int16_t FStringFitsScreen(char *lpsz, int16_t dxMax) {
    HDC     hdc;
    int16_t c;
    int16_t fFit;
    HFONT   hfontSav;

    fFit = TRUE;
    hdc = GetDC(hwndFrame);
    c = strlen(lpsz);
    hfontSav = SelectObject(hdc, rghfontArial8[0]);
    while (c > 0 && LOWORD(GetTextExtent(hdc, lpsz, c)) > (uint16_t)dxMax) {
        fFit = FALSE;
        c--;
        lpsz[c] = 0;
    }
    SelectObject(hdc, hfontSav);
    ReleaseDC(hwndFrame, hdc);
    return fFit;
}

HBRUSH HbrGet(COLORREF cr) {
    int16_t iFree;
    int16_t i;
    HBRUSH  hbr;

    iFree = -1;
    for (i = 0; i < chbrCache; i++) {
        if (rghbrCacheUse[i] == 0) {
            iFree = i;
        } else if (rgcrCache[i] == cr) {
            rghbrCacheUse[i]++;
            return rghbrCache[i];
        }
    }
    hbr = CreateSolidBrush(cr);
    if (!hbr) {
        return NULL;
    }
    if (iFree == -1) {
        if (chbrCache >= 32) {
            return hbr;
        }
        iFree = chbrCache++;
    }
    rghbrCacheUse[iFree] = 1;
    rghbrCache[iFree] = hbr;
    rgcrCache[iFree] = cr;
    return hbr;
}

void FreeHbr(HBRUSH hbr) {
    int16_t i;

    for (i = 0; i < chbrCache; i++) {
        if (rghbrCacheUse[i] > 0 && hbr == rghbrCache[i]) {
            rghbrCacheUse[i]--;
            if (rghbrCacheUse[i] != 0) {
                return;
            }
        DeleteBrush:
            DeleteObject(hbr);
            return;
        }
    }
    goto DeleteBrush;
}

uint16_t DibNumColors(void *pv) {
    int16_t           bits;
    BITMAPCOREHEADER *lpbc;
    BITMAPINFOHEADER *lpbi;

    lpbi = pv;
    lpbc = pv;
    if (lpbi->biSize != 12) {
        if (lpbi->biClrUsed != 0) {
            return LOWORD(lpbi->biClrUsed);
        }
        bits = lpbi->biBitCount;
    } else {
        bits = lpbc->bcBitCount;
    }
    switch (bits) {
    case 1:
        return 2;
    case 4:
        return 16;
    case 8:
        return 0x100;
    default:
        return 0;
    }
}

HPALETTE HpalFromDib(HGLOBAL hdib) {
    HPALETTE          hpal;
    int16_t           cColors;
    int16_t           i;
    uint8_t           bT;
    char             *lpb;
    BITMAPINFOHEADER *lpbi;
    LOGPALETTE       *ppal;

    lpb = LockResource(hdib);
    if (!lpb) {
        return NULL;
    }
    lpbi = (BITMAPINFOHEADER *)lpb;
    cColors = DibNumColors(lpbi);
    if (cColors > 256) {
        GlobalUnlock(hdib);
        return NULL;
    }
    ppal = LocalAlloc(64, cColors * 4 + 8);
    ppal->palNumEntries = cColors;
    ppal->palVersion = 768;
    memcpy(ppal->palPalEntry, lpb + 40, cColors * 4);
    for (i = 0; i < cColors; i++) {
        bT = ppal->palPalEntry[i].peRed;
        ppal->palPalEntry[i].peRed = ppal->palPalEntry[i].peBlue;
        ppal->palPalEntry[i].peBlue = bT;
    }
    hpal = CreatePalette(ppal);
    LocalFree(ppal);
    GlobalUnlock(hdib);
    return hpal;
}

HPALETTE HpalBlackReserved() {
    HPALETTE    hpal;
    int16_t     cColors;
    int16_t     i;
    LOGPALETTE *ppal;

    cColors = 256;
    ppal = LocalAlloc(64, cColors * 4 + 8);
    ppal->palNumEntries = cColors;
    ppal->palVersion = 768;
    for (i = 0; i < cColors; i++) {
        ppal->palPalEntry[i].peBlue = 0;
        ppal->palPalEntry[i].peGreen = 0;
        ppal->palPalEntry[i].peRed = 0;
        ppal->palPalEntry[i].peFlags = 1;
    }
    hpal = CreatePalette(ppal);
    LocalFree(ppal);
    return hpal;
}

uint16_t PaletteSize(void *pv) {
    uint16_t          NumColors;
    BITMAPINFOHEADER *lpbi;

    lpbi = pv;
    NumColors = DibNumColors(lpbi);
    if (lpbi->biSize == 12) {
        return 3 * NumColors;
    }
    return NumColors * 4;
}

int16_t DibBlt(HDC hdc, int16_t x0, int16_t y0, int16_t dx, int16_t dy, HGLOBAL hdib, int16_t x1, int16_t y1, int16_t dxSrc, int16_t dySrc, int32_t rop) {
    char             *pBuf;
    BITMAPINFOHEADER *lpbi;

    if (!hdib) {
        return PatBlt(hdc, x0, y0, dx, dy, rop);
    }
    lpbi = (BITMAPINFOHEADER *)GlobalLock(hdib);
    if (!lpbi) {
        return 0;
    }
    pBuf = (char *)lpbi + LOWORD(lpbi->biSize) + PaletteSize(lpbi);
    StretchDIBits(hdc, x0, y0, dx, dy, x1, y1, dxSrc, dySrc, pBuf, (LPBITMAPINFO)lpbi, 0, rop);
    GlobalUnlock(hdib);
    return 1;
}

HGLOBAL DibFromBitmap(HBITMAP hbm, uint32_t biStyle, uint16_t biBits, HPALETTE hpal) {
    HDC               hdc;
    HGLOBAL           h;
    uint32_t          dwLen;
    BITMAP            bm;
    BITMAPINFOHEADER  bi;
    HGLOBAL           hdib;
    BITMAPINFOHEADER *lpbi;

    if (!hbm) {
        return NULL;
    }
    if (!hpal) {
        hpal = GetStockObject(DEFAULT_PALETTE);
    }
    GetObject(hbm, 14, &bm);
    if (biBits == 0) {
        biBits = bm.bmPlanes * bm.bmBitsPixel;
    }
    bi.biSize = 40;
    bi.biWidth = bm.bmWidth;
    bi.biHeight = bm.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = biBits;
    bi.biCompression = biStyle;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;
    dwLen = (uint32_t)PaletteSize(&bi) + bi.biSize;
    hdc = GetDC(NULL);
    hpal = SelectPalette(hdc, hpal, FALSE);
    RealizePalette(hdc);
    hdib = GlobalAlloc(66, dwLen);
    if (!hdib) {
        SelectPalette(hdc, hpal, FALSE);
        ReleaseDC(NULL, hdc);
        return NULL;
    }
    lpbi = (BITMAPINFOHEADER *)GlobalLock(hdib);
    *lpbi = bi;
    GetDIBits(hdc, hbm, 0, LOWORD(bi.biHeight), NULL, (BITMAPINFO *)lpbi, 0);
    bi = *lpbi;
    GlobalUnlock(hdib);
    if (bi.biSizeImage == 0) {
        bi.biSizeImage = (uint32_t)((int32_t)((uint32_t)(((uint32_t)(bm.bmWidth * (uint32_t)biBits) + 31) / 32) * 4) * bm.bmHeight);
        if (biStyle != 0) {
            bi.biSizeImage = (uint32_t)((uint32_t)(bi.biSizeImage * 3) / 2);
        }
    }
    dwLen = (uint32_t)PaletteSize(&bi) + bi.biSize + bi.biSizeImage;
    h = GlobalReAlloc(hdib, dwLen, 0);
    if (h) {
        hdib = h;
        lpbi = (BITMAPINFOHEADER *)GlobalLock(hdib);
        if (GetDIBits(hdc, hbm, 0, LOWORD(bi.biHeight), (uint8_t *)lpbi + LOWORD(lpbi->biSize) + PaletteSize(lpbi), (BITMAPINFO *)lpbi, 0) == 0) {
            GlobalUnlock(hdib);
            hdib = 0;
            SelectPalette(hdc, hpal, FALSE);
            ReleaseDC(NULL, hdc);
            return NULL;
        }
        bi = *lpbi;
        GlobalUnlock(hdib);
        SelectPalette(hdc, hpal, FALSE);
        ReleaseDC(NULL, hdc);
        return hdib;
    }
    GlobalFree(hdib);
    hdib = 0;
    SelectPalette(hdc, hpal, FALSE);
    ReleaseDC(NULL, hdc);
    return hdib;
}

HGLOBAL HdibLoadBigResource(BitmapId idb) {
    HRSRC    hrsrc;
    HGLOBAL  hres;
    char    *lpstr;
    HGLOBAL  hdib;
    uint32_t cb;

    hrsrc = FindResource(hInst, MAKEINTRESOURCE(idb), MAKEINTRESOURCE(2));
    if (hrsrc == 0) {
        return NULL;
    }
    hres = LoadResource(hInst, hrsrc);
    if (!hres) {
        return NULL;
    }
    lpstr = LockResource(hres);
    if (!lpstr) {
        return NULL;
    }
    /* A writable copy: the original read the resource into its own block. */
    cb = SizeofResource(hInst, hrsrc);
    hdib = GlobalAlloc(GMEM_FIXED, cb);
    if (!hdib) {
        return NULL;
    }
    memcpy((char *)hdib, lpstr, cb);
    return hdib;
}

INT_PTR CALLBACK PasswordDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    char    szPass[60];
    RECT    rc;
    int32_t lSalt;

    switch (message) {
    case WM_INITDIALOG:
        SendDlgItemMessage(hwnd, IDC_EDIT1, EM_LIMITTEXT, 0xf, 0);
        SetWindowText(GetDlgItem(hwnd, IDC_PASSWORD_STATUS_TEXT), PszGetCompressedString(idsEnterPassword));
        return 1;
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
        if (message == WM_CTLCOLORSTATIC) {
            SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            if (LOWORD(wParam) == IDOK) {
                GetDlgItemText(hwnd, IDC_EDIT1, szPass, 60);
                lSalt = LSaltFromSz(szPass);
                if (lSalt == lSaltCur) {
                    if (!gd.fHotSeat) {
                        lSaltLast = lSaltCur;
                        strcpy(szPassLast, szPass);
                    }
                } else {
                    vcPasswordFailures++;
                    Delay(vcPasswordFailures < 10 ? 1000 : vcPasswordFailures < 100 ? 5000 : 10000);
                    AlertSz(PszFormatIds(idsPasswordHaveEnteredIncorrectPleaseTry, NULL), MB_ICONHAND);
                    SetFocus(GetDlgItem(hwnd, IDC_EDIT1));
                    SendDlgItemMessage(hwnd, IDC_EDIT1, EM_SETSEL, 0, -1);
                    break;
                }
            }
            EndDialog(hwnd, LOWORD(wParam) == IDOK);
            return 1;
        case IDC_HELP:
            WinHelp(hwnd, szHelpFile, HELP_CONTEXT, 1089);
            return 1;
        }
    }
    return 0;
}

INT_PTR CALLBACK NewPasswordDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    char    szPass[20];
    RECT    rc;
    int32_t lSalt2;
    int32_t lSalt;

    switch (message) {
    case WM_INITDIALOG:
        SendDlgItemMessage(hwnd, IDC_EDIT1, EM_LIMITTEXT, 0x10, 0);
        SendDlgItemMessage(hwnd, IDC_PASSWORD_CONFIRM, EM_LIMITTEXT, 0x10, 0);
        if (idPlayer == iplrNone) {
            SetWindowText(GetDlgItem(hwnd, IDC_PASSWORD_STATUS_TEXT), PszGetCompressedString(idsNotePasswordEffectiveImmediately));
            SetWindowText(hwnd, PszGetCompressedString(idsChangeHostPassword));
        } else {
            SetWindowText(GetDlgItem(hwnd, IDC_PASSWORD_STATUS_TEXT), PszGetCompressedString(idsNoteNewPasswordWillTakeEffectUntil));
        }
        return 1;
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
        if (message == WM_CTLCOLORSTATIC) {
            SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            if (LOWORD(wParam) == IDOK) {
                GetWindowText(GetDlgItem(hwnd, IDC_EDIT1), szPass, 18);
                lSalt = LSaltFromSz(szPass);
                GetWindowText(GetDlgItem(hwnd, IDC_PASSWORD_CONFIRM), szPass, 18);
                lSalt2 = LSaltFromSz(szPass);
                if (lSalt != lSalt2) {
                    AlertSz(PszFormatIds(idsPasswordsTypedTwoFieldsSamePleaseReenter, NULL), MB_ICONHAND);
                    SetFocus(GetDlgItem(hwnd, IDC_EDIT1));
                    SendDlgItemMessage(hwnd, IDC_EDIT1, EM_SETSEL, 0, -1);
                    break;
                }
                if (idPlayer != iplrNone) {
                    WriteMemRt(rtChgPassword, 4, &lSalt);
                } else {
                    lSaltCur = lSalt;
                    if (FWriteDataFile(szBase, idPlayer, FALSE)) {
                        lSaltLast = lSalt;
                    } else {
                        AlertSz(PszFormatIds(idsUnableCreateHostFile, NULL), MB_ICONHAND);
                        lSaltCur = lSaltLast;
                    }
                }
            }
            EndDialog(hwnd, LOWORD(wParam) == IDOK);
            return 1;
        case IDC_HELP:
            WinHelp(hwnd, szHelpFile, HELP_CONTEXT, idhChangePassword);
            return 1;
        }
    }
    return 0;
}

void ShowProgressGauge() {
    if (!hwndProgressGauge) {
        CreateDialog(hInst, MAKEINTRESOURCE(IDD_Gauge), hwndFrame, lpfnGaugeDlgProc);
    } else {
        UpdateProgressGauge(0);
    }
    return;
}

void HideProgressGauge() {
    if (hwndProgressGauge) {
        DestroyWindow(hwndProgressGauge);
        hwndProgressGauge = 0;
    }
    return;
}

INT_PTR CALLBACK ProgressGaugeDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    HDC         hdc;
    PAINTSTRUCT ps;
    RECT        rc;
    int16_t     dy;
    char       *psz;
    int16_t     dx;

    switch (message) {
    case WM_INITDIALOG:
        vpctProgressGauge = 0;
        hwndProgressGauge = hwnd;
        dx = GetSystemMetrics(SM_CXSCREEN);
        dy = GetSystemMetrics(SM_CYSCREEN);
        psz = PszGetCompressedString(idsGeneratingDataYearD);
        wsprintf(szWork, psz, game.turn + 2401);
        SetWindowText(GetDlgItem(hwnd, IDC_GAUGE_TEXT), szWork);
        GetWindowRect(hwnd, &rc);
        rc.left = (dx - (rc.right - rc.left)) >> 1;
        rc.top = (dy - (rc.bottom - rc.top)) >> 1;
        SetWindowPos(hwnd, NULL, rc.left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        return 1;
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
        SetBkColor((HDC)wParam, crButtonFace);
        return (INT_PTR)hbrButtonFace;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        DrawProgressGauge(hdc, TRUE, 0);
        EndPaint(hwnd, &ps);
        return 1;
    }
    return 0;
}

void DrawProgressGauge(HDC hdcOrig, int16_t fFull, int16_t iNumOnly) {
    HDC     hdc;
    int16_t dy;
    int16_t fNumOnly;
    int16_t dx2;
    int16_t dx;
    RECT    rc;
    int16_t c;
    char    szT[8];

    fNumOnly = iNumOnly > 0;
    if (hwndProgressGauge) {
        if (!hdcOrig) {
            hdc = GetDC(hwndProgressGauge);
        } else {
            hdc = hdcOrig;
        }
        GetClientRect(hwndProgressGauge, &rc);
        rc.top = rc.bottom >> 1;
        InflateRect(&rc, -8, -8);
        dx = rc.right - rc.left;
        dy = rc.bottom - rc.top;
        if (fFull) {
            SelectObject(hdc, hbrButtonShadow);
            PatBlt(hdc, rc.left - 1, rc.top - 1, dx + 2, 1, PATCOPY);
            PatBlt(hdc, rc.left - 1, rc.top - 1, 1, dy + 2, PATCOPY);
            SelectObject(hdc, hbrButtonHilite);
            PatBlt(hdc, rc.left, rc.bottom, dx + 1, 1, PATCOPY);
            PatBlt(hdc, rc.right, rc.top, 1, dy, PATCOPY);
        }
        InflateRect(&rc, -2, -2);
        dx -= 4;
        dy -= 4;
        if (gd.fProgressTxt) {
            SetBkColor(hdc, crButtonFace);
            SelectObject(hdc, rghfontArial7[0]);
            if (iNumOnly <= 0) {
                iNumOnly = vpctProgressGauge;
            }
            c = wsprintf(szT, PCTD, iNumOnly);
            RightTextOut(hdc, rc.right - 2, 1, szT, c, 80);
        }
        if (fNumOnly)
            goto LRelease;

        SelectObject(hdc, hbrBlue);
        for (dx = MulDiv(dx, vpctProgressGauge, 1000); dx > 0; dx -= dy) {
            if (dx >= dy) {
                dx2 = dy - 1;
            } else {
                dx2 = dx;
            }
            PatBlt(hdc, rc.left, rc.top, dx2, dy, PATCOPY);
            rc.left += dy;
        }

    LRelease:
        if (!hdcOrig) {
            ReleaseDC(hwndProgressGauge, hdc);
        }
    }
    return;
}

HFONT HfontPrinterCreate(HDC hdc, int16_t iSize, int16_t *pdyFont) {
    HFONT      hfontNew;
    LOGFONT   *plf;
    TEXTMETRIC tm;
    HFONT      hfontSav;

    plf = LocalAlloc(64, sizeof(LOGFONT));
    memset(plf, 0, sizeof(LOGFONT));
    plf->lfHeight = -MulDiv(iSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    strcpy(plf->lfFaceName, rgszArial[1]);
    hfontNew = CreateFontIndirect(plf);
    if (pdyFont && hfontNew) {
        hfontSav = SelectObject(hdc, hfontNew);
        GetTextMetrics(hdc, &tm);
        *pdyFont = tm.tmHeight + tm.tmExternalLeading;
        SelectObject(hdc, hfontSav);
    }
    LocalFree(plf);
    return hfontNew;
}

// IdAlertBox shows an AlertSz message in a message box and returns the
// button the player chose.
int16_t IdAlertBox(char *sz, int16_t mbType) { return MessageBox(GetFocus(), sz, "Stars!", mbType); }

// PromptPassword asks for the password of the file FCheckPassword is
// checking.
int16_t PromptPassword() {
    FARPROC lpProc;
    int16_t fRet;

    lpProc = MakeProcInstance(PasswordDlg, hInst);
    fRet = DialogBox(hInst, MAKEINTRESOURCE(IDD_PASSWORD), !hwndTitle ? hwndFrame : hwndTitle, lpProc);
    FreeProcInstance(lpProc);
    return fRet;
}

void UpdateProgressGauge(ProgressStep pctX10) {
    int16_t iNum;

    if (hwndProgressGauge) {
        iNum = 0;
        if (pctX10 == progressStep4) {
            pctX10 = vpctProgressGauge + 4;
        } else if (pctX10 == progressStep1) {
            pctX10 = vpctProgressGauge + 1;
        } else if (pctX10 < 0) {
            pctX10 = 0;
        } else if (pctX10 > 1000) {
            if (!gd.fProgressTxt) {
                return;
            }
            iNum = pctX10;
            pctX10 = vpctProgressGauge;
        }
        vpctProgressGauge = pctX10;
        DrawProgressGauge(NULL, FALSE, iNum);
    }
    return;
}
