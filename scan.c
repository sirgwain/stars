#include "win.h"

uint32_t rgcrScanMine[3] = {16711680, 65535, 255};
int16_t  vrgPopRad[19] = {25, 50, 100, 200, 400, 800, 1000, 1500, 2250, 3000, 4000, 5000, 6000, 7500, 9000, 11000, 14000, 18000, 25000};

// Wheel deltas short of a notch, for zooming and sideways scrolling.
static int16_t dWheelZoom;
static int16_t dWheelScroll;

// Dragging the scanner with the left button pans it. fPanDown is set while
// a plain left press may still become a pan, fPanning once it has. The
// drag is measured from ptPan, where the scanner's top was xPanTop, yPanTop.
static int16_t fPanDown;
static int16_t fPanning;
static POINT16 ptPan;
static int16_t xPanTop;
static int16_t yPanTop;

// FAltDown tells whether Alt is down; Wine's Mac driver sends Alt for Cmd.
// Alt+click (Cmd+click) adds a waypoint at the fastest useful speed.
static int16_t FAltDown(void) { return GetKeyState(VK_MENU) < 0; }

LRESULT CALLBACK ScannerWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HDC         hdc;
    POINT16     pt;
    PAINTSTRUCT ps;
    RECT        rc;
    ScanZoom    iScanNew;
    HPEN        hpenSav;
    int16_t     iRopSav;
    int16_t     i;
    uint32_t    tick;
    PLANET      plT;
    int16_t     fChgScan;
    SCAN        scan;
    int16_t     c;
    THING      *lpth;
    FLEET      *lpfl;
    int16_t     fSep;
    int32_t    *rgid;
    int32_t     idSel;
    int16_t     iChecked;
    int16_t     iSel;
    THING      *lpthMac;
    int16_t     id;
    int16_t     d;
    int16_t     dy;
    int16_t     dx;

    switch (msg) {
    case WM_MDIACTIVATE:
        hwndActive = (lParam == (LPARAM)hwnd) == 0 ? NULL : hwnd;
        break;
    case WM_CREATE:
        yScanTop = 1000;
        xScanTop = 1000;
        break;
    case WM_CHAR:
        if (wParam == 'v' || wParam == 'V') {
            hdc = GetDC(hwndScanner);
            pt = sel.scan.pt;
            LogicalToScan(&pt);
            iRopSav = SetROP2(hdc, R2_XORPEN);
            hpenSav = SelectObject(hdc, hpenYellow);
            for (i = 0; i < 2; i++) {
                MoveToEx(hdc, pt.x - 300, pt.y - 300, NULL);
                LineTo(hdc, pt.x + 300, pt.y + 300);
                MoveToEx(hdc, pt.x - 300, pt.y + 300, NULL);
                LineTo(hdc, pt.x + 300, pt.y - 300);
                if (i == 0) {
                    tick = GetTickCount();
                    while (GetTickCount() < tick + 150) {
                        Yield();
                    }
                }
            }
            SelectObject(hdc, hpenSav);
            SetROP2(hdc, iRopSav);
            ReleaseDC(hwndScanner, hdc);
            break;
        }
        if (wParam == '-') {
            iScanNew = iScanZoom - 1;
            if (iScanNew < zoom25) {
                iScanNew = zoom25;
            }
        } else {
            iScanNew = iScanZoom + 1;
            if (iScanNew > zoom400) {
                iScanNew = zoom400;
            }
        }
        if (iScanNew != iScanZoom) {
            SendMessage(hwndFrame, WM_COMMAND, iScanNew + 3905, 0);
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        if (rglpfl && !gd.fNoScannerDraw) {
            GetClientRect(hwnd, &rc);
            DrawScannerSBar(hdc, &ps.rcPaint, NULL, TRUE);
            rc.bottom -= dySBar;
            DrawScanner(hdc, &ps.rcPaint);
        }
        EndPaint(hwnd, &ps);
        break;
    case WM_SETCURSOR:
        GetCursorPos16(&pt);
        ScreenToClient16(hwndScanner, &pt);
        GetClientRect(hwnd, &rc);
        if (PtInRect(&rc, PointFrom16(pt)) == 0)
            goto Default;
        rc.bottom -= dySBar;
        if (PtInRect(&rc, PointFrom16(pt)) == 0) {
            SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(32512)));
        } else if (sel.grobj == grobjFleet && ((GetAsyncKeyState(VK_SHIFT) & 0xfffe) || (grbitScan & grbitScanAddWaypoints) || FAltDown())) {
            SetCursor(hcurScanAdd);
        } else if (FNearAWayPoint(pt, FALSE)) {
            SetCursor(hcurOpenGrab);
        } else if (gd.fSetMassMode || gd.fSetRouteMode ||
                   (sel.grobj == grobjPlanet &&
                    (((GetAsyncKeyState(VK_SHIFT) & 0xfffe) && IWarpMAFromLppl(&sel.pl, NULL) > 0) || (GetAsyncKeyState(VK_CONTROL) & 0xfffe)))) {
            SetCursor(hcurScanAdd);
        } else {
            SetCursor(hcurScanner);
        }
        return 1;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
        SetFocus(hwndFrame);
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        GetClientRect(hwnd, &rc);
        if (pt.y >= rc.bottom - dySBar) {
            if (msg != WM_LBUTTONDOWN || pt.y >= rc.bottom - (dySBar >> 1) || !(sel.scan.grobjFull & (grobjPlanet | grobjFleet)))
                break;
            if (sel.scan.grobj == grobjFleet) {
                GlobalPD.grPopup = grPopupFleet;
                GlobalPD.lpfl = rglpfl[sel.scan.ifl];
            } else {
                GlobalPD.grPopup = grPopupUnknownObj;
            }
            Popup(hwnd, pt.x, pt.y);
            break;
        }
        // A plain left press acts as a click and, if the mouse then moves,
        // pans. Shift, Ctrl and the waypoint, mass driver and route modes
        // keep the press for themselves; a waypoint drag ends with the
        // button up, which WM_MOUSEMOVE notices.
        fPanDown = msg == WM_LBUTTONDOWN && !(wParam & (MK_SHIFT | MK_CONTROL)) && !gd.fSetMassMode && !gd.fSetRouteMode &&
                   !(sel.grobj == grobjFleet && ((grbitScan & grbitScanAddWaypoints) || FAltDown()));
        ptPan = pt;
        ScanToLogical(&pt);
        FFindNearestObject(pt, gd.fSetMassMode != 0 || gd.fSetRouteMode ? grobjPlanet : grobjPlanet | grobjFleet | grobjOther | grobjThing, &scan);
        if ((gd.fSetMassMode || (sel.grobj == grobjPlanet && (wParam & 4) && IWarpMAFromLppl(&sel.pl, NULL) > 0)) && msg == WM_LBUTTONDOWN) {
            DrawShipScanPath(NULL, FALSE);
            if (scan.idpl == sel.pl.id) {
                sel.pl.idFling = 0;
            } else {
                sel.pl.idFling = scan.idpl + 1;
            }
            FLookupPlanet(idWriteBack, &sel.pl);
            gd.fSetMassMode = FALSE;
            DrawShipScanPath(NULL, TRUE);
            DrawPlanShip(NULL, tileStarbaseOrWaypoint | tileMinimized);
            break;
        }
        if ((gd.fSetRouteMode || (sel.grobj == grobjPlanet && (wParam & 8))) && msg == WM_LBUTTONDOWN) {
            DrawShipScanPath(NULL, FALSE);
            if (scan.idpl == sel.pl.id) {
                sel.pl.idRoute = 0;
            } else {
                sel.pl.idRoute = scan.idpl + 1;
            }
            FLookupPlanet(idWriteBack, &sel.pl);
            gd.fSetRouteMode = FALSE;
            DrawShipScanPath(NULL, TRUE);
            DrawPlanShip(NULL, tileProductionOrOrbit | tileMinimized);
            break;
        }
        if (msg == WM_MBUTTONDOWN || (msg == WM_RBUTTONDOWN && (wParam & 4))) {
            FHandleMeasuringTape(&scan, pt);
            break;
        }
        if (msg == WM_RBUTTONDOWN) {
            iChecked = -1;
            pt = scan.pt;
            /* Room for every fleet and thing here, a planet and a separator.
               The original stopped at 100 entries, so objects past them
               couldn't be picked (target list overload). */
            rgid = LpAlloc((cFleet + cThing + 3) * sizeof(int32_t), htMisc);
            if (scan.grobjFull & grobjPlanet) {
                rgid[0] = scan.idpl;
                rgid[1] = -1;
                c = 2;
                if (sel.grobj == grobjPlanet && sel.id == scan.idpl) {
                    iChecked = 0;
                }
            } else {
                c = 0;
            }
            for (i = 0; i < cFleet; i++) {
                lpfl = rglpfl[i];
                if (!rglpfl[i])
                    break;
                if (pt.x == lpfl->pt.x && pt.y == lpfl->pt.y) {
                    if (sel.grobj == grobjFleet && lpfl->id == sel.id) {
                        iChecked = c;
                    }
                    rgid[c++] = lpfl->id | 0x80000000;
                }
            }
            if (c == 2 && (scan.grobjFull & grobjPlanet)) {
                c = 1;
            }
            fSep = c == 0;
            lpth = lpThings;
            lpthMac = lpThings + cThing;
            for (; lpth < lpthMac; lpth++) {
                if (pt.x == lpth->pt.x && pt.y == lpth->pt.y) {
                    if (!fSep) {
                        rgid[c++] = -1;
                        fSep = TRUE;
                    }
                    rgid[c++] = (uint32_t)(uint16_t)lpth->idFull | 0x20000000;
                }
            }
            LogicalToScan(&pt);
            iSel = PopupMenu(hwnd, pt.x, pt.y, c, rgid, NULL, iChecked, TRUE);
            idSel = iSel >= 0 ? rgid[iSel] : 0;
            FreeLp(rgid, htMisc);
            if (iSel < 0)
                break;
            if (idSel & 0x80000000) {
                scan.grobj = grobjFleet;
                id = LOWORD(idSel);
                for (i = 0; i < cFleet; i++) {
                    lpfl = rglpfl[i];
                    if (!rglpfl[i] || lpfl->id == id)
                        break;
                }
                scan.ifl = i;
            } else if (idSel & 0x20000000) {
                scan.grobj = grobjThing;
                lpth = lpThings;
                lpthMac = lpThings + cThing;
                for (; lpth < lpthMac && lpth->idFull != LOWORD(idSel); lpth++) {
                }
                scan.ith = (int16_t)((uint8_t *)lpth - (uint8_t *)lpThings) / 18;
            } else {
                scan.grobj = grobjPlanet;
            }
            ChangeScanSel(&scan, 2);
            if (scan.grobj == grobjPlanet && (!FLookupPlanet(scan.idpl, &plT) || plT.iPlayer != idPlayer))
                break;
            goto DblClick;
        } else {
            if (sel.grobj == grobjFleet && ((wParam & 4) || (grbitScan & grbitScanAddWaypoints) || FAltDown())) {
                FAddWayPoint(pt, &scan);
                break;
            }
            fChgScan = scan.pt.x == sel.scan.pt.x && scan.pt.y == sel.scan.pt.y;
            ChangeScanSel(&scan, 1);
            if ((FNearAWayPoint(pt, TRUE) && FHandleWayPointDrag(pt)) || !fChgScan || (scan.grobj != grobjFleet && scan.grobj != grobjPlanet))
                break;
            if (scan.pt.x == sel.pt.x && scan.pt.y == sel.pt.y) {
                if (!FGetNextObjHere(&scan, TRUE))
                    break;
            } else if (scan.grobj == grobjPlanet) {
                if (!FLookupPlanet(scan.idpl, &plT))
                    break;
                if (plT.iPlayer != idPlayer || ((scan.grobjFull & grobjFleet) && sel.grobj == grobjPlanet && scan.idpl == sel.id)) {
                    if (!(scan.grobjFull & grobjFleet))
                        break;
                    scan.grobj = grobjFleet;
                }
            }
        }
    DblClick:
        if ((scan.grobj == grobjFleet && rglpfl[scan.ifl]->iPlayer != idPlayer) || scan.grobj == grobjThing)
            break;
        if (scan.grobj == grobjFleet) {
            scan.iwp = iwpNone;
        }
        ChangeScanSel(&scan, 1);
        RedrawScanSel(NULL, 0);
        ChangeMainObjSel(scan.grobj, scan.grobj == grobjPlanet ? scan.idpl : rglpfl[scan.ifl]->id);
        RedrawScanSel(NULL, 1);
        if (scan.grobj != grobjFleet || !(scan.grobjFull & grobjPlanet))
            break;
        scan.grobj = grobjPlanet;
        ChangeScanSel(&scan, 1);
        break;
    case WM_MOUSEMOVE:
        if (!fPanDown)
            goto Default;
        if (!(wParam & MK_LBUTTON)) {
            fPanDown = FALSE;
            goto Default;
        }
        pt.x = (short)LOWORD(lParam);
        pt.y = (short)HIWORD(lParam);
        if (!fPanning) {
            if (abs(pt.x - ptPan.x) < GetSystemMetrics(SM_CXDRAG) && abs(pt.y - ptPan.y) < GetSystemMetrics(SM_CYDRAG))
                break;
            fPanning = TRUE;
            xPanTop = xScanTop;
            yPanTop = yScanTop;
            SetCapture(hwnd);
            SetCursor(hcurCloseGrab);
        }
        // The galaxy follows the mouse. Scanner tops are multiples of 4, so
        // the new tops are rounded to them from where the drag began.
        d = ((xPanTop - ScanToPt(pt.x - ptPan.x)) + 2) & 0xfffc;
        if (d != xScanTop) {
            SendMessage(hwnd, WM_HSCROLL, MAKEWPARAM(SB_THUMBPOSITION, (WORD)d), 0);
        }
        d = ((yPanTop - ScanToPt(pt.y - ptPan.y)) + 2) & 0xfffc;
        if (d != yScanTop) {
            SendMessage(hwnd, WM_VSCROLL, MAKEWPARAM(SB_THUMBPOSITION, (WORD)d), 0);
        }
        break;
    case WM_LBUTTONUP:
        fPanDown = FALSE;
        if (fPanning) {
            ReleaseCapture();
        }
        goto Default;
    case WM_CAPTURECHANGED:
        if (fPanning) {
            fPanning = FALSE;
            fPanDown = FALSE;
        }
        goto Default;
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
        // The wheel zooms at the cursor, as map viewers do; Ctrl+wheel, a
        // touchpad pinch, zooms too. The tilt wheel and Shift+wheel scroll
        // sideways, wheel down to the right.
        d = GET_WHEEL_DELTA_WPARAM(wParam);
        if (msg == WM_MOUSEWHEEL && !(GET_KEYSTATE_WPARAM(wParam) & MK_SHIFT)) {
            c = CWheelNotches(&dWheelZoom, d);
            if (c == 0)
                break;
            iScanNew = iScanZoom + c;
            if (iScanNew < zoom25) {
                iScanNew = zoom25;
            } else if (iScanNew > zoom400) {
                iScanNew = zoom400;
            }
            if (iScanNew != iScanZoom) {
                // A pan in progress is measured at the old zoom.
                if (fPanning) {
                    ReleaseCapture();
                }
                fPanDown = FALSE;
                pt.x = (short)LOWORD(lParam);
                pt.y = (short)HIWORD(lParam);
                ScreenToClient16(hwnd, &pt);
                ZoomScanAt(iScanNew, pt);
            }
            break;
        }
        c = CWheelNotches(&dWheelScroll, msg == WM_MOUSEWHEEL ? -d : d);
        if (c == 0)
            break;
        d = c * CWheelLines(dScanPage / dScanInc) * dScanInc;
        SendMessage(hwnd, WM_HSCROLL, MAKEWPARAM(SB_THUMBPOSITION, (WORD)(xScanTop + d)), 0);
        break;
    case WM_SIZE:
        SetScanScrollBars(hwnd);
        PostMessage(hwndFrame, WM_COMMAND, iScanZoom + 3905, 0);
        goto Default;
    case WM_HSCROLL:
    case WM_VSCROLL:
        if (LOWORD(wParam) <= SB_THUMBTRACK) {
            switch (LOWORD(wParam)) {
            case SB_LINEUP:
                d = -dScanInc;
                break;
            case SB_LINEDOWN:
                d = dScanInc;
                break;
            case SB_PAGEUP:
                d = -dScanPage;
                break;
            case SB_PAGEDOWN:
                d = dScanPage;
                break;
            case SB_THUMBPOSITION:
            case SB_THUMBTRACK:
                d = (short)HIWORD(wParam) - (msg == WM_VSCROLL ? yScanTop : xScanTop);
                d &= 0xfffc;
            }
        } else {
            d = 0;
        }
        if (d == 0)
            break;
        if (msg == WM_VSCROLL) {
            dy = yScanTop;
            SetScrollPos(hwnd, SB_VERT, yScanTop + d, TRUE);
            yScanTop = GetScrollPos(hwnd, SB_VERT);
            ScrollScanner(0, PtToScan(dy - yScanTop));
            break;
        }
        dx = xScanTop;
        SetScrollPos(hwnd, SB_HORZ, xScanTop + d, TRUE);
        xScanTop = GetScrollPos(hwnd, SB_HORZ);
        ScrollScanner(PtToScan(dx - xScanTop), 0);
        break;
    default:
    Default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int16_t DrawScanner(HDC hdc, RECT *prc) {
    int16_t  xOff;
    int16_t  dExpand;
    HPEN     hpenSav;
    FLEET   *lpflT;
    int16_t  j;
    int16_t  yTop;
    int16_t  xMax;
    POINT16  pt;
    int16_t  id;
    COLORREF crFore;
    int16_t  iBkPrev;
    POINT16  ptD;
    PLANET  *lpplMac;
    int16_t  yBmp;
    int16_t  dy;
    HBITMAP  hbmpXSav;
    HBITMAP  hbmpScreen;
    int16_t  id2;
    HDC      hdcScreen;
    PLANET  *lppl;
    int16_t  yMax;
    char     rgWhatsHere[999];
    HDC      hdcMem;
    THING   *lpth;
    FLEET   *lpfl;
    int16_t  i;
    int16_t  xMin;
    HBITMAP  hbmpSav;
    int16_t  iord;
    RECT     rcClip;
    int16_t  yOff;
    RECT     rcDraw;
    int16_t  dRange;
    int16_t  idP;
    POINT16  ptO;
    int16_t  fSelected;
    int16_t  fMA;
    int16_t  fStarbase;
    POINT16  ptSelMain;
    THING   *lpthMac;
    int16_t  fStargate;
    ScanView mdScanBase;
    int16_t  yMin;
    int16_t  dx;
    HBRUSH   hbrSav;
    int16_t  xLeft;
    int16_t  fDoDraw;
    POINT16  ptOrigin;
    RECT     rc;
    int32_t  l;
    COLORREF crBack;
    int16_t  rgy[250];
    int16_t  fPlanetScanner;
    int16_t  rgx[250];
    int16_t  rgrad[250];
    DRAWCIR  dc;
    int16_t  dPlanRange;
    int16_t  dThingRange;
    int16_t  ropSav;
    int16_t  fDetonating;
    POINT16  pt2;
    THING   *lpthDest;
    HBITMAP  hbmpTrSav;
    int16_t  fTerra;
    int16_t  dRad;
    int16_t  pctDesire;
    HBRUSH   hbr;
    int16_t  iOff;
    int16_t  xOut;
    int16_t  fConc;
    int16_t  yOut;
    int16_t  iRel;
    int32_t  lPop;
    COLORREF cr;

    mdScanBase = grbitScan & grbitScanViewMask;
    hdcScreen = 0;
    hbmpScreen = 0;
    if (!rglpfl || gd.fNoScannerDraw || gd.fGeneratingTurn) {
        return 0;
    }
    xLeft = xScanTop;
    yTop = yScanTop;
    ptOrigin.x = PtToScan(0xfa0 - xScanTop) & 7;
    ptOrigin.y = PtToScan(0xfa0 - yScanTop) & 7;
    GetClientRect(hwndScanner, &rc);
    ExcludeClipRect(hdc, 0, rc.bottom - dySBar, rc.right, rc.bottom);
    l = (int16_t)(prc->right - prc->left);
    l = (uint32_t)(l * (int16_t)(prc->bottom - prc->top));
    if (l < 48000) {
        hdcMem = CreateCompatibleDC(hdc);
        if (hdcMem) {
            hbmpScreen = CreateCompatibleBitmap(hdc, prc->right - (prc->left & 0xfff8), prc->bottom - (prc->top & 0xfff8));
            if (!hbmpScreen) {
                DeleteDC(hdcMem);
            }
        }
        if (hbmpScreen) {
            hdcScreen = hdc;
            hdc = hdcMem;
            hbmpXSav = SelectObject(hdc, hbmpScreen);
            SetWindowOrgEx(hdc, prc->left & 0xfff8, prc->top & 0xfff8, NULL);
            pt.y = 0;
            pt.x = 0;
            ClientToScreen16(hwndScanner, &pt);
            ptOrigin.x = (ptOrigin.x + 8 - (pt.x & 7)) & 7;
            ptOrigin.y = (ptOrigin.y + 8 - (pt.y & 7)) & 7;
            rcDraw = *prc;
        }
    }
    FillRect(hdc, prc, GetStockObject(BLACK_BRUSH));
    rcClip = *prc;
    dExpand = (iScanZoom >= zoom200 && mdScanBase == scanViewPopulation) ||
                      (iScanZoom >= zoom100 && (mdScanBase == scanViewSurfaceMinerals || mdScanBase == scanViewMineralConc))
                  ? 20
                  : 9;
    ExpandRc(prc, dExpand, dExpand);
    prc->bottom += 14;
    dx = ScanToPt(prc->right - prc->left);
    dy = ScanToPt(prc->bottom - prc->top);
    xMin = ScanToPt(prc->left) + xLeft;
    xMax = xMin + dx;
    yMax = dGalInv - yTop - ScanToPt(prc->top);
    yMin = yMax - dy;
    xOff = -xLeft;
    yOff = dGalInv - yTop;
    i = 0;
    id = 1;
    while (i < 16) {
        if (rgshdef[i].fFree) {
            grbitScanShip &= ~(id & grbitScanShip);
        }
        i++;
        id *= 2;
    }
    hdcMem = CreateCompatibleDC(hdc);
    hbmpSav = SelectObject(hdcMem, hbmpScanner);
    if (sel.grobj != grobjNone) {
        ptSelMain = sel.pt;
    } else {
        ptSelMain.y = -2;
        ptSelMain.x = -2;
    }
    if (!gd.fFleetLinkValid) {
        LinkFleets(FALSE);
    } else {
        for (i = 0; i < cFleet; i++) {
            lpfl = rglpfl[i];
            if (!rglpfl[i])
                break;
            lpfl->fDone = FALSE;
        }
    }
    if (grbitScan & grbitScanCoverage) {
        fPlanetScanner = 0;
        dc.rgx = rgx;
        dc.rgy = rgy;
        dc.rgrad = rgrad;
        dc.cCur = 0;
        dc.cMax = 250;
        dc.hdc = hdc;
        dc.rcClip = *prc;
        dc.fCovered = FALSE;
        dc.fHollowOut = FALSE;
        IntersectClipRect(hdc, rcClip.left, rcClip.top, rcClip.right, rcClip.bottom);
        hbrSav = SelectObject(hdc, hbrRadar);
        hpenSav = SelectObject(hdc, hpenRadar);
        lppl = lpPlanets;
        lpplMac = lpPlanets + cPlanet;
        for (; lppl < lpplMac; lppl++) {
            if (lppl->iPlayer == idPlayer) {
                dRange = GetPlanetScannerRange(lppl, &dPlanRange);
                if (dPlanRange > 0) {
                    fPlanetScanner = 1;
                }
                if (vpctRadarView < 100) {
                    dRange = MulDiv(dRange, vpctRadarView, 100);
                }
                id = lppl->id;
                rc.left = PtToScan(xOff + rgptPlan[id].x - dRange);
                rc.top = PtToScan(yOff - rgptPlan[id].y - dRange);
                rc.right = PtToScan(xOff + rgptPlan[id].x + dRange);
                rc.bottom = PtToScan(yOff - rgptPlan[id].y + dRange);
                DrawRadarCircle(&dc, &rc);
            }
        }
        for (i = 0; i < cFleet; i++) {
            lpfl = rglpfl[i];
            if (!rglpfl[i])
                break;
            if (lpfl->iPlayer == idPlayer) {
                dRange = GetFleetScannerRange(lpfl, &dPlanRange, NULL, NULL);
                if (dPlanRange > 0) {
                    fPlanetScanner |= 2;
                }
                if (dRange > 0) {
                    if (vpctRadarView < 100) {
                        dRange = MulDiv(dRange, vpctRadarView, 100);
                    }
                    rc.left = PtToScan(xOff + lpfl->pt.x - dRange);
                    rc.top = PtToScan(yOff - lpfl->pt.y - dRange);
                    rc.right = PtToScan(xOff + lpfl->pt.x + dRange);
                    rc.bottom = PtToScan(yOff - lpfl->pt.y + dRange);
                    DrawRadarCircle(&dc, &rc);
                }
            }
        }
        DrawRadarCircle(&dc, NULL);
        if (!hbrRadarNear) {
            hbrRadarNear = HbrGet(vcScreenColors > 8 ? 24672 : 32639);
        }
        if (!hpenRadarNear) {
            hpenRadarNear = CreatePen(0, 1, vcScreenColors > 8 ? 24672 : 32639);
        }
        SelectObject(hdc, hbrRadarNear);
        SelectObject(hdc, hpenRadarNear);
        if (fPlanetScanner & 1) {
            lppl = lpPlanets;
            lpplMac = lpPlanets + cPlanet;
            for (; lppl < lpplMac; lppl++) {
                if (lppl->iPlayer == idPlayer) {
                    dRange = GetPlanetScannerRange(lppl, &dPlanRange);
                    if (dPlanRange > 0) {
                        dRange >>= 1;
                        if (vpctRadarView < 100) {
                            dRange = MulDiv(dRange, vpctRadarView, 100);
                        }
                        id = lppl->id;
                        rc.left = PtToScan(xOff + rgptPlan[id].x - dRange);
                        rc.top = PtToScan(yOff - rgptPlan[id].y - dRange);
                        rc.right = PtToScan(xOff + rgptPlan[id].x + dRange);
                        rc.bottom = PtToScan(yOff - rgptPlan[id].y + dRange);
                        DrawRadarCircle(&dc, &rc);
                    }
                }
            }
        }
        if (fPlanetScanner & 2) {
            for (i = 0; i < cFleet; i++) {
                lpfl = rglpfl[i];
                if (!rglpfl[i])
                    break;
                if (lpfl->iPlayer == idPlayer) {
                    dRange = GetFleetScannerRange(lpfl, &dPlanRange, NULL, NULL);
                    if (dPlanRange > 0) {
                        if (vpctRadarView < 100) {
                            dPlanRange = MulDiv(dPlanRange, vpctRadarView, 100);
                        }
                        rc.left = PtToScan(xOff + lpfl->pt.x - dPlanRange);
                        rc.top = PtToScan(yOff - lpfl->pt.y - dPlanRange);
                        rc.right = PtToScan(xOff + lpfl->pt.x + dPlanRange);
                        rc.bottom = PtToScan(yOff - lpfl->pt.y + dPlanRange);
                        DrawRadarCircle(&dc, &rc);
                    }
                }
            }
        }
        if (GetRaceStat(&rgplr[idPlayer], rsMajorAdv) == raMassAccel) {
            lpth = lpThings;
            lpthMac = lpThings + cThing;
            for (; lpth < lpthMac; lpth++) {
                if (lpth->ith == ithMineralPacket && lpth->iplr == idPlayer && lpth->thp.iWarp != 0) {
                    dThingRange = lpth->thp.iWarp + 4;
                    dThingRange *= dThingRange;
                    if (vpctRadarView < 100) {
                        dThingRange = MulDiv(dThingRange, vpctRadarView, 100);
                    }
                    rc.left = PtToScan(xOff + lpth->pt.x - dThingRange);
                    rc.top = PtToScan(yOff - lpth->pt.y - dThingRange);
                    rc.right = PtToScan(xOff + lpth->pt.x + dThingRange);
                    rc.bottom = PtToScan(yOff - lpth->pt.y + dThingRange);
                    DrawRadarCircle(&dc, &rc);
                }
            }
        }
        DrawRadarCircle(&dc, NULL);
        SelectObject(hdc, hbrRadar);
        SelectObject(hdc, hpenRadar);
        if (mdScanBase != scanViewNoPlayerInfo) {
            id = sel.grobj == grobjFleet ? sel.fl.id : idflNone;
            id2 = sel.scan.grobj == grobjFleet ? rglpfl[sel.scan.ifl]->id : idflNone;
            SelectObject(hdcMem, hbmpScanShip);
            SetTextColor(hdc, 0);
            SetBkColor(hdc, 0xffffff);
            for (i = 0; i < cFleet; i++) {
                lpfl = rglpfl[i];
                if (!rglpfl[i])
                    break;
                if (lpfl->idPlanet == idPlanetDeepSpace && (CShipsScanVis(lpfl) > 0 || lpfl->id == id || lpfl->id == id2)) {
                    pt = lpfl->pt;
                    if (pt.x >= xMin && pt.x < xMax && pt.y >= yMin && pt.y < yMax) {
                        pt.x = PtToScan(xOff + pt.x);
                        pt.y = PtToScan(yOff - pt.y);
                        fSelected = lpfl->pt.x == ptSelMain.x && lpfl->pt.y == ptSelMain.y;
                        if (fSelected) {
                            SelectObject(hdcMem, hbmpScanner);
                            BitBlt(hdc, pt.x - 5, pt.y - 5, 11, 11, hdcMem, 11, 80, SRCAND);
                            SelectObject(hdcMem, hbmpScanShip);
                        } else {
                            GetScanFleetOrientation(lpfl, &ptO, &ptD);
                            BitBlt(hdc, pt.x - ptD.x / 2, pt.y - ptD.y / 2, ptD.x, ptD.y, hdcMem, ptO.x, ptO.y, SRCAND);
                        }
                    }
                }
            }
            SelectObject(hdcMem, hbmpScanner);
        }
        SelectObject(hdc, hpenSav);
        SelectObject(hdc, hbrSav);
    }
    if (grbitScan & grbitScanMineFields) {
        dc.rgx = rgx;
        dc.rgy = rgy;
        dc.rgrad = rgrad;
        dc.cCur = 0;
        dc.cMax = 250;
        dc.hdc = hdc;
        dc.rcClip = *prc;
        dc.fCovered = FALSE;
        dc.fHollowOut = TRUE;
        for (i = 0; i < 3; i++) {
            UnrealizeObject(rghbrPat[i]);
        }
        SetBrushOrgEx(hdc, ptOrigin.x, ptOrigin.y, NULL);
        IntersectClipRect(hdc, rcClip.left, rcClip.top, rcClip.right, rcClip.bottom);
        hbrSav = SelectObject(hdc, rghbrPat[0]);
        hpenSav = SelectObject(hdc, GetStockObject(NULL_PEN));
        ropSav = GetROP2(hdc);
        SetBkColor(hdc, 0);
        for (j = 0; j < 3; j++) {
            if ((j != 0 || (grbitScanMines & 1)) && (j != 1 || (grbitScanMines & 2)) && (j != 2 || (grbitScanMines & 0xc))) {
                for (i = 0; i < 3; i++) {
                    for (fDetonating = FALSE; fDetonating <= (i == 0); fDetonating++) {
                        SetBrushOrgEx(hdc, ptOrigin.x, ptOrigin.y, NULL);
                        SelectObject(hdc, rghbrPat[i]);
                        SetTextColor(hdc, !fDetonating ? rgcrScanMine[j] : 16711935);
                        lpth = lpThings;
                        lpthMac = lpThings + cThing;
                        for (; lpth < lpthMac; lpth++) {
                            if (lpth->ith == ithMinefield && lpth->thm.iType == i && (j != 0 || lpth->iplr == idPlayer) &&
                                (j < 1 || (lpth->iplr != idPlayer && (rgplr[idPlayer].rgmdRelation[lpth->iplr] == 1) != (j == 2)))) {
                                if (j == 2 && (grbitScanMines & 0xc) != 0xc) {
                                    if (rgplr[idPlayer].rgmdRelation[lpth->iplr] == 0) {
                                        if (!(grbitScanMines & 4))
                                            continue;
                                    } else if (!(grbitScanMines & 8)) {
                                        continue;
                                    }
                                }
                                if (lpth->thm.fDetonate == fDetonating) {
                                    pt = lpth->pt;
                                    dRange = LOWORD(Sf64ToI32(Sf64Sqrt(Sf64FromI32(lpth->thm.cMines))));
                                    rc.left = PtToScan(xOff + pt.x - dRange);
                                    rc.top = PtToScan(yOff - pt.y - dRange);
                                    rc.right = PtToScan(xOff + pt.x + dRange);
                                    rc.bottom = PtToScan(yOff - pt.y + dRange);
                                    DrawRadarCircle(&dc, &rc);
                                    for (id = 0; id < game.cPlanMax && (rgptPlan[id].x != pt.x || rgptPlan[id].y != pt.y); id++) {
                                    }
                                    if (id == game.cPlanMax) {
                                        pt.x = PtToScan(xOff + pt.x);
                                        pt.y = PtToScan(yOff - pt.y);
                                        dRange = PtToScan(1);
                                        if (dRange < 1) {
                                            dRange = 1;
                                        } else if (dRange > 3) {
                                            dRange = 3;
                                        }
                                        SetRect(&rc, pt.x - dRange, pt.y - dRange, pt.x + dRange + 1, pt.y + dRange + 1);
                                        SetBkColor(hdc, rgcrScanMine[j]);
                                        ExtTextOut(hdc, rc.left, rc.top, ETO_OPAQUE, &rc, NULL, 0, NULL);
                                        SetBkColor(hdc, 0);
                                    }
                                }
                            }
                        }
                        DrawRadarCircle(&dc, NULL);
                    }
                }
            }
        }
        if (sel.scan.grobj == grobjThing) {
            lpth = lpThings + sel.scan.ith;
            if (lpth->ith == ithMinefield) {
                SetBrushOrgEx(hdc, ptOrigin.x, ptOrigin.y, NULL);
                SelectObject(hdc, rghbrPat[lpth->thm.iType]);
                SetTextColor(hdc, 16776960);
                pt = lpth->pt;
                dRange = LOWORD(Sf64ToI32(Sf64Sqrt(Sf64FromI32(lpth->thm.cMines))));
                rc.left = PtToScan(xOff + pt.x - dRange);
                rc.top = PtToScan(yOff - pt.y - dRange);
                rc.right = PtToScan(xOff + pt.x + dRange);
                rc.bottom = PtToScan(yOff - pt.y + dRange);
                DrawRadarCircle(&dc, &rc);
                DrawRadarCircle(&dc, NULL);
            }
        }
        SetROP2(hdc, ropSav);
        SelectObject(hdc, hpenSav);
        SelectObject(hdc, hbrSav);
    }
    if (cThing != 0 && mdScanBase != scanViewNoPlayerInfo) {
        IntersectClipRect(hdc, rcClip.left, rcClip.top, rcClip.right, rcClip.bottom);
        hbrSav = SelectObject(hdc, hbrShip);
        hpenSav = SelectObject(hdc, hpenDkPurple);
        lpth = lpThings;
        lpthMac = lpThings + cThing;
        for (; lpth < lpthMac; lpth++) {
            switch (lpth->ith) {
            case ithMineralPacket:
            case ithWormhole:
            case ithMysteryTrader:
                pt = lpth->pt;
                LogicalToScan(&pt);
                if (lpth->ith == ithWormhole && lpth->idFull < lpth->thw.idPartner && (1 << idPlayer & lpth->thw.grbitPlrTrav)) {
                    lpthDest = LpthFromId(lpth->thw.idPartner);
                    if (lpthDest) {
                        MoveToEx(hdc, pt.x, pt.y, NULL);
                        pt2 = lpthDest->pt;
                        LogicalToScan(&pt2);
                        LineTo(hdc, pt2.x, pt2.y);
                        LineTo(hdc, pt2.x, pt2.y + 1);
                    }
                }
                if (lpth->ith == ithWormhole) {
                    BitBlt(hdc, pt.x - 4, pt.y - 4, 9, 9, hdcMem, 9, 92, SRCAND);
                    BitBlt(hdc, pt.x - 4, pt.y - 4, 9, 9, hdcMem, 0, 92, SRCPAINT);
                } else if (lpth->ith == ithMysteryTrader) {
                    hbmpTrSav = SelectObject(hdcMem, hbmpScanShip);
                    crFore = SetTextColor(hdc, 16776960);
                    crBack = SetBkColor(hdc, 0);
                    GetDxDyOrientation(lpth->tht.ptDest.x - lpth->pt.x, lpth->tht.ptDest.y - lpth->pt.y, &ptO, &ptD);
                    BitBlt(hdc, pt.x - ptD.x / 2, pt.y - ptD.y / 2, ptD.x, ptD.y, hdcMem, ptO.x, ptO.y, SRCPAINT);
                    SetTextColor(hdc, crFore);
                    SetBkColor(hdc, crBack);
                    SelectObject(hdcMem, hbmpTrSav);
                } else {
                    dRange = iScanZoom <= zoom100 ? 2 : iScanZoom > zoom150 ? 5 : 3;
                    dx = dRange * 2 + 1;
                    if (lpth->thp.iWarp == 0) {
                        SelectObject(hdc, hpenYellow);
                        MoveToEx(hdc, pt.x, pt.y - dRange - 1, NULL);
                        LineTo(hdc, pt.x - dRange - 1, pt.y);
                        LineTo(hdc, pt.x, pt.y + dRange + 1);
                        LineTo(hdc, pt.x + dRange + 1, pt.y);
                        LineTo(hdc, pt.x, pt.y - dRange - 1);
                        SelectObject(hdc, hpenDkPurple);
                    } else {
                        if (lpth->iplr != idPlayer) {
                            SelectObject(hdc, hbrRed);
                        }
                        PatBlt(hdc, pt.x - dRange, pt.y - dRange, dx, 1, PATCOPY);
                        PatBlt(hdc, pt.x - dRange, pt.y - dRange, 1, dx, PATCOPY);
                        PatBlt(hdc, pt.x - dRange, pt.y + dRange, dx, 1, PATCOPY);
                        PatBlt(hdc, pt.x + dRange, pt.y - dRange, 1, dx, PATCOPY);
                        if (lpth->iplr != idPlayer) {
                            SelectObject(hdc, hbrShip);
                        }
                    }
                }
            }
        }
        SelectObject(hdc, hbrSav);
        SelectObject(hdc, hpenSav);
    }
    if ((grbitScan & grbitScanFleetPaths) && mdScanBase != scanViewNoPlayerInfo) {
        id = sel.grobj == grobjFleet ? sel.fl.id : idflNone;
        id2 = sel.scan.grobj == grobjFleet ? rglpfl[sel.scan.ifl]->id : idflNone;
        hpenSav = SelectObject(hdc, hpenStarbase);
        for (i = 0; i < cFleet; i++) {
            lpfl = rglpfl[i];
            if (!rglpfl[i])
                break;
            if (!lpfl->fDead && lpfl->det >= detAll && lpfl->cord > 1 && (CShipsScanVis(lpfl) > 0 || lpfl->id == id || lpfl->id == id2)) {
                for (iord = 0; iord < lpfl->cord; iord++) {
                    pt = lpfl->lpplord->rgord[iord].pt;
                    pt.x = PtToScan(xOff + pt.x);
                    pt.y = PtToScan(yOff - pt.y);
                    if (iord == 0) {
                        MoveToEx(hdc, pt.x, pt.y, NULL);
                    } else {
                        LineTo(hdc, pt.x, pt.y);
                    }
                }
            }
        }
        SelectObject(hdc, hpenSav);
    }
    SelectClipRgn(hdc, hrgnHuge);
    GetClientRect(hwndScanner, &rc);
    ExcludeClipRect(hdc, 0, rc.bottom - dySBar, rc.right, rc.bottom);
    memset(rgWhatsHere, 0, 999);
    switch (iScanZoom) {
    case zoom400:
        SelectObject(hdc, rghfontArial10[1]);
        break;
    case zoom200:
        SelectObject(hdc, rghfontArial8[1]);
        break;
    case zoom100:
    case zoom125:
    case zoom150:
        SelectObject(hdc, rghfontArial8[0]);
        break;
    case zoom75:
        SelectObject(hdc, rghfontArial6[0]);
    }
    crFore = SetTextColor(hdc, 0xffffff);
    crBack = SetBkColor(hdc, 0);
    iBkPrev = SetBkMode(hdc, TRANSPARENT);
    j = iScanZoom >= zoom200 && mdScanBase == scanViewPopulation ? 11 : 0;
    for (i = 0; i < game.cPlanMax; i++) {
        if (rgptPlan[i].x >= xMin && rgptPlan[i].x < xMax && rgptPlan[i].y >= yMin && rgptPlan[i].y < yMax) {
            fDoDraw = TRUE;

        LBailIn:
            pt.x = PtToScan(xOff + rgptPlan[i].x);
            pt.y = PtToScan(yOff - rgptPlan[i].y);
            if (fDoDraw) {
                if (ptSelMain.x == rgptPlan[i].x && ptSelMain.y == rgptPlan[i].y && mdScanBase <= scanViewMineralConc) {
                    BitBlt(hdc, pt.x - 5, pt.y - 5, 11, 11, hdcMem, 0, 69, SRCAND);
                    BitBlt(hdc, pt.x - 5, pt.y - 5, 11, 11, hdcMem, 0, 33, SRCPAINT);
                } else {
                    BitBlt(hdc, pt.x - 1, pt.y - 1, 3, 3, hdcMem, 11, 15, SRCCOPY);
                }
            }
            if ((grbitScan & grbitScanPlanetNames) && iScanZoom >= zoom75 && rgptPlan[i].x >= xMin - 50 && rgptPlan[i].x < xMax + 50 &&
                rgptPlan[i].y >= yMin - 20 && rgptPlan[i].y < yMax + 20) {
                PszGetPlanetName(i);
                if (grbitScan & grbitScanPlayerColors) {
                    lppl = LpplFromId(i);
                    if (lppl && lppl->iPlayer != iplrNone) {
                        SetTextColor(hdc, lppl->iPlayer == idPlayer ? 0xffffff : rgcrPlrHistory[lppl->iPlayer]);
                    }
                }
                CtrTextOut(hdc, pt.x, pt.y + 5 + j, szWork, 0);
                if (grbitScan & grbitScanPlayerColors) {
                    SetTextColor(hdc, 0xffffff);
                }
            }
        } else if (grbitScan & grbitScanPlanetNames) {
            fDoDraw = FALSE;
            goto LBailIn;
        }
    }
    SetBkMode(hdc, iBkPrev);
    SetBkColor(hdc, crBack);
    SetTextColor(hdc, crFore);
    j = iScanZoom >= zoom200 && mdScanBase == scanViewPopulation ? 11 : 0;
    if (sel.grobj != grobjNone && sel.pt.x == sel.scan.pt.x && sel.pt.y == sel.scan.pt.y) {
        pt = sel.pt;
        LogicalToScan(&pt);
        BitBlt(hdc, pt.x - 5, pt.y + 11 + j, 11, 12, hdcMem, 0, 80, SRCAND);
        BitBlt(hdc, pt.x - 5, pt.y + 11 + j, 11, 12, hdcMem, 0, 57, SRCPAINT);
    } else if (sel.scan.grobj != grobjNone) {
        pt = sel.scan.pt;
        LogicalToScan(&pt);
        BitBlt(hdc, pt.x - 3, pt.y + 7 + j, 7, 8, hdcMem, 22, 80, SRCAND);
        BitBlt(hdc, pt.x - 3, pt.y + 7 + j, 7, 8, hdcMem, 22, 49, SRCPAINT);
    }
    if (cPlanet != 0 && mdScanBase != scanViewNoPlayerInfo) {
        lppl = lpPlanets;
        i = 0;
        while (i < cPlanet) {
            id = lppl->id;
            fStarbase = lppl->fStarbase != 0 && lppl->iPlayer != iplrNone;
            if (fStarbase) {
                if (rglpshdefSB[lppl->iPlayer][lppl->isb].hul.ihuldef == ihuldefOrbitalFort) {
                    fStarbase = 2;
                }
                fMA = IWarpMAFromLppl(lppl, NULL) > 0;
                fStargate = IStargateFromLppl(lppl) != -1;
            } else {
                fMA = FALSE;
                fStargate = FALSE;
            }
            if (rgptPlan[id].x >= xMin && rgptPlan[id].x < xMax && rgptPlan[id].y >= yMin && rgptPlan[id].y < yMax) {
                pt.x = PtToScan(xOff + rgptPlan[id].x);
                pt.y = PtToScan(yOff - rgptPlan[id].y);
                switch (mdScanBase) {
                case scanViewPlanetValue:
                    fTerra = FALSE;
                    if (lppl->det < detSome)
                        break;
                    pctDesire = PctPlanetDesirability(lppl, idPlayer);
                    if (pctDesire < 0 || GetRaceStat(&rgplr[idPlayer], rsMajorAdv) == raTerra) {
                        pctDesire = PctPlanetOptValue(lppl, idPlayer);
                        if (pctDesire >= 0 && GetRaceStat(&rgplr[idPlayer], rsMajorAdv) != raTerra) {
                            fTerra = TRUE;
                        }
                    }
                    hbrSav = SelectObject(hdc, pctDesire < 0 ? hbrRadar : fTerra ? hbrDkYellow : hbrGreen);
                    hpenSav = SelectObject(hdc, pctDesire < 0 ? hpenRadar : fTerra ? hpenDkYellow : hpenDkGreen);
                    if (pctDesire >= 0) {
                        dRad = pctDesire / 11 + 2;
                    } else {
                        dRad = (int16_t)-pctDesire / 5 + 2;
                    }
                    if (dRad > 10) {
                        dRad = 10;
                    }
                    Ellipse(hdc, pt.x - dRad, pt.y - dRad, pt.x + dRad + 1, pt.y + dRad + 1);
                    dRad -= 2;
                    if (dRad < 3) {
                        dRad++;
                    }
                    if (dRad < 1) {
                        dRad = 1;
                    }
                    SelectObject(hdc, pctDesire < 0 ? hbrEnemy : fTerra ? hbrYellow : hbrShip);
                    SelectObject(hdc, pctDesire < 0 ? hpenEnemy : fTerra ? hpenYellow : hpenShip);
                    Ellipse(hdc, pt.x - dRad, pt.y - dRad, pt.x + dRad + 1, pt.y + dRad + 1);
                    if (lppl->iPlayer != iplrNone) {
                        rc.left = pt.x - 1;
                        rc.right = rc.left + 9;
                        rc.top = pt.y - 20;
                        rc.bottom = rc.top + 8;
                        FillRect(hdc, &rc, GetStockObject(BLACK_BRUSH));
                        if (lppl->iPlayer == idPlayer) {
                            hbr = hbrBBlue;
                        } else if (rgplr[idPlayer].rgmdRelation[lppl->iPlayer] == 1) {
                            hbr = hbrStarbase;
                        } else if (rgplr[idPlayer].rgmdRelation[lppl->iPlayer] == 0) {
                            hbr = hbrRadar;
                        } else {
                            hbr = hbrEnemy;
                        }
                        SelectObject(hdc, hbr);
                        PatBlt(hdc, pt.x, pt.y - 19, 1, 21, PATCOPY);
                        PatBlt(hdc, pt.x, pt.y - 19, 7, 6, PATCOPY);
                    }
                    SelectObject(hdc, hpenSav);
                    SelectObject(hdc, hbrSav);
                    break;
                case scanViewSurfaceMinerals:
                case scanViewMineralConc:
                    fConc = mdScanBase == scanViewMineralConc;
                    iOff = iScanZoom < zoom100;
                    if (lppl->det < detMore && (!fConc || lppl->det < detSome))
                        goto LNormalScannerMode;
                    xOut = pt.x - vrgScanPO[iOff][0];
                    yOut = pt.y - vrgScanPO[iOff][1];
                    hbrSav = SelectObject(hdc, hbrButtonFace);
                    PatBlt(hdc, xOut - 2, yOut, vrgScanPO[iOff][2], 1, PATCOPY);
                    PatBlt(hdc, xOut - 2, yOut - vrgScanPO[iOff][2] + 1, 1, vrgScanPO[iOff][2], PATCOPY);
                    for (j = 0; j < 3; j++) {
                        if (fConc) {
                            l = (int16_t)((int16_t)lppl->rgMinConc[j] / 5);
                            if (l > 20) {
                                l = 20;
                            }
                        } else {
                            l = lppl->rgwtMin[j];
                            l = (int32_t)(((int16_t)(cMinGrafMax / 40) + l) / (int16_t)(cMinGrafMax / 20));
                            if (l > 20) {
                                l = 20;
                            }
                        }
                        if (iOff != 0) {
                            l = (int32_t)(l / 2);
                        }
                        if (l > 0) {
                            SelectObject(hdc, rghbrMineral[j]);
                            PatBlt(hdc, xOut, yOut - LOWORD(l), vrgScanPO[iOff][3], LOWORD(l), PATCOPY);
                        }
                        xOut += vrgScanPO[iOff][4];
                    }
                    SelectObject(hdc, hbrSav);
                    goto LNormalScannerMode;
                case scanViewPopulation:
                    fTerra = FALSE;
                    if (lppl->det >= detSome && lppl->iPlayer != iplrNone) {
                        if (lppl->iPlayer == idPlayer) {
                            lPop = lppl->rgwtMin[3];
                        } else {
                            lPop = (int32_t)(lppl->uPopGuess * 4);
                        }
                        for (dRad = 0; dRad < 19 && vrgPopRad[dRad] <= lPop; dRad++) {
                        }
                        dRad += 2;
                        if (lppl->iPlayer == idPlayer) {
                            iRel = 0;
                        } else if (rgplr[idPlayer].rgmdRelation[lppl->iPlayer] == 1) {
                            iRel = 1;
                        } else {
                            iRel = 3;
                        }
                        hbrSav = SelectObject(hdc, iRel == 0 ? hbrShip : iRel == 3 ? hbrEnemy : hbrYellow);
                        hpenSav = SelectObject(hdc, iRel == 0 ? hpenDkGreen : iRel == 3 ? hpenEnemy : hpenDkYellow);
                        if (iScanZoom < zoom200) {
                            dRad = (dRad + 1) >> 1;
                        }
                        Ellipse(hdc, pt.x - dRad, pt.y - dRad, pt.x + dRad + 1, pt.y + dRad + 1);
                        SelectObject(hdc, hpenSav);
                        SelectObject(hdc, hbrSav);
                        break;
                    }
                    if (lppl->iPlayer != iplrNone)
                        break;
                    /* fallthrough */
                default:
                LNormalScannerMode:
                    if (lppl->iPlayer == iplrNone) {
                        if (rgptPlan[id].x != ptSelMain.x || rgptPlan[id].y != ptSelMain.y) {
                            BitBlt(hdc, pt.x - 1, pt.y - 1, 3, 3, hdcMem, 11, 18, SRCCOPY);
                        }
                    } else if (rgptPlan[id].x == ptSelMain.x && rgptPlan[id].y == ptSelMain.y) {
                        if (lppl->iPlayer == idPlayer) {
                            yBmp = 0;
                        } else if (rgplr[idPlayer].rgmdRelation[lppl->iPlayer] == 1) {
                            yBmp = 22;
                        } else {
                            yBmp = 11;
                        }
                        BitBlt(hdc, pt.x - 5, pt.y - 5, 11, 11, hdcMem, 0, 69, SRCAND);
                        BitBlt(hdc, pt.x - 5, pt.y - 5, 11, 11, hdcMem, 0, yBmp, SRCPAINT);
                        if (fStarbase) {
                            hbrSav = SelectObject(hdc, fStarbase == 2 ? hbrBlue : hbrYellow);
                            PatBlt(hdc, pt.x + 4, pt.y - 6, 5, 5, BLACKNESS);
                            PatBlt(hdc, pt.x + 5, pt.y - 6, 3, 5, PATCOPY);
                            PatBlt(hdc, pt.x + 4, pt.y - 5, 5, 3, PATCOPY);
                            SelectObject(hdc, hbrSav);
                        }
                        if (fStargate) {
                            hbrSav = SelectObject(hdc, hbrGreen);
                            PatBlt(hdc, pt.x - 7, pt.y - 6, 5, 5, BLACKNESS);
                            PatBlt(hdc, pt.x - 6, pt.y - 6, 3, 5, PATCOPY);
                            PatBlt(hdc, pt.x - 7, pt.y - 5, 5, 3, PATCOPY);
                            SelectObject(hdc, hbrSav);
                        }
                        if (fMA) {
                            hbrSav = SelectObject(hdc, hbrPurple);
                            PatBlt(hdc, pt.x - 2, pt.y - 9, 5, 5, BLACKNESS);
                            PatBlt(hdc, pt.x - 1, pt.y - 9, 3, 5, PATCOPY);
                            PatBlt(hdc, pt.x - 2, pt.y - 8, 5, 3, PATCOPY);
                            SelectObject(hdc, hbrSav);
                        }
                    } else {
                        if (lppl->iPlayer == idPlayer) {
                            yBmp = 0;
                        } else if (rgplr[idPlayer].rgmdRelation[lppl->iPlayer] == 1) {
                            yBmp = 10;
                        } else {
                            yBmp = 5;
                        }
                        BitBlt(hdc, pt.x - 2, pt.y - 2, 5, 5, hdcMem, 11, yBmp, SRCCOPY);
                        if (fStarbase) {
                            hbrSav = SelectObject(hdc, fStarbase == 2 ? hbrBlue : hbrYellow);
                            PatBlt(hdc, pt.x + 3, pt.y - 4, 3, 3, PATCOPY);
                            SelectObject(hdc, hbrSav);
                        }
                        if (fStargate) {
                            hbrSav = SelectObject(hdc, hbrGreen);
                            PatBlt(hdc, pt.x - 5, pt.y - 4, 3, 3, BLACKNESS);
                            PatBlt(hdc, pt.x - 4, pt.y - 4, 1, 3, PATCOPY);
                            PatBlt(hdc, pt.x - 5, pt.y - 3, 3, 1, PATCOPY);
                            SelectObject(hdc, hbrSav);
                        }
                        if (fMA) {
                            hbrSav = SelectObject(hdc, hbrPurple);
                            PatBlt(hdc, pt.x - 1, pt.y - 6, 3, 3, BLACKNESS);
                            PatBlt(hdc, pt.x, pt.y - 6, 1, 3, PATCOPY);
                            PatBlt(hdc, pt.x - 1, pt.y - 5, 3, 1, PATCOPY);
                            SelectObject(hdc, hbrSav);
                        }
                    }
                }
            }
            i++;
            lppl++;
        }
    }
    if (cFleet != 0 && mdScanBase != scanViewNoPlayerInfo) {
        lpflT = *rglpfl;
        hbrSav = SelectObject(hdc, hbrShip);
        id = sel.grobj == grobjFleet ? sel.fl.id : idflNone;
        id2 = sel.scan.grobj == grobjFleet ? rglpfl[sel.scan.ifl]->id : idflNone;
        for (i = 0; i < cFleet; i++) {
            lpflT = rglpfl[i];
            if (!rglpfl[i])
                break;
            if (CShipsScanVis(lpflT) > 0 || lpflT->id == id || lpflT->id == id2) {
                idP = lpflT->idPlanet;
                if (idP != idPlanetDeepSpace) {
                    pt = rgptPlan[idP];
                } else {
                    pt = lpflT->pt;
                }
                if (pt.x >= xMin && pt.x < xMax && pt.y >= yMin && pt.y < yMax) {
                    pt.x = PtToScan(xOff + pt.x);
                    pt.y = PtToScan(yOff - pt.y);
                    yBmp = lpflT->iPlayer != idPlayer;
                    fSelected = lpflT->pt.x == ptSelMain.x && lpflT->pt.y == ptSelMain.y;
                    if (idP != idPlanetDeepSpace) {
                        if (rgWhatsHere[idP] != 3 && rgWhatsHere[idP] != yBmp + 1 && mdScanBase <= scanViewMineralConc) {
                            rgWhatsHere[idP] += yBmp + 1;
                            yBmp = rgWhatsHere[idP] - 1;
                            if (fSelected) {
                                BitBlt(hdc, pt.x - 9, pt.y - 9, 19, 19, hdcMem, 29, 69, SRCAND);
                                BitBlt(hdc, pt.x - 9, pt.y - 9, 19, 19, hdcMem, 29, 19 * yBmp, SRCPAINT);
                            } else {
                                BitBlt(hdc, pt.x - 5, pt.y - 5, 11, 11, hdcMem, 16, 69, SRCAND);
                                BitBlt(hdc, pt.x - 5, pt.y - 5, 11, 11, hdcMem, 16, 11 * yBmp, SRCPAINT);
                            }
                            if ((grbitScan & grbitScanShipCounts) && !lpflT->fDone) {
                                DrawScanFleetCount(lpflT, pt.x, pt.y - (!fSelected ? 5 : 9) - 2, hdc, hdcMem);
                            }
                        }
                    } else if (fSelected) {
                        BitBlt(hdc, pt.x - 5, pt.y - 5, 11, 11, hdcMem, 11, 11 * yBmp + 36, SRCPAINT);
                        if ((grbitScan & grbitScanShipCounts) && !lpflT->fDone) {
                            DrawScanFleetCount(lpflT, pt.x, pt.y - 7, hdc, hdcMem);
                        }
                    } else {
                        SelectObject(hdcMem, hbmpScanShip);
                        if (yBmp == 0) {
                            cr = 16711680;
                        } else if (rgplr[idPlayer].rgmdRelation[lpflT->iPlayer] == 1) {
                            cr = 0xffff;
                        } else {
                            cr = 0xff;
                        }
                        SetTextColor(hdc, cr);
                        SetBkColor(hdc, 0);
                        GetScanFleetOrientation(lpflT, &ptO, &ptD);
                        BitBlt(hdc, pt.x - ptD.x / 2, pt.y - ptD.y / 2, ptD.x, ptD.y, hdcMem, ptO.x, ptO.y, SRCPAINT);
                        if ((grbitScan & grbitScanShipCounts) && !lpflT->fDone) {
                            DrawScanFleetCount(lpflT, pt.x, pt.y - ptD.y / 2 - 2, hdc, hdcMem);
                        }
                        SelectObject(hdcMem, hbmpScanner);
                    }
                }
            }
        }
        SelectObject(hdc, hbrSav);
    }
    ExpandRc(prc, -dExpand, -dExpand);
    prc->bottom -= 14;
    crFore = SetTextColor(hdc, 0);
    crBack = GetBkColor(hdc);
    if (sel.grobj == grobjFleet || sel.scan.grobj == grobjFleet || sel.scan.grobj == grobjThing || (sel.grobj == grobjPlanet && sel.pl.fStarbase)) {
        IntersectClipRect(hdc, prc->left, prc->top, prc->right, prc->bottom);
        fOrdersVis = FALSE;
        DrawShipScanPath(hdc, TRUE);
        SelectClipRgn(hdc, hrgnHuge);
    }
    SetBkColor(hdc, crBack);
    SetTextColor(hdc, crFore);
    SelectObject(hdcMem, hbmpSav);
    DeleteDC(hdcMem);
    if (hdcScreen) {
        SetWindowOrgEx(hdc, 0, 0, NULL);
        BitBlt(hdcScreen, prc->left, prc->top, prc->right - prc->left, prc->bottom - prc->top, hdc, prc->left & 7, prc->top & 7, SRCCOPY);
        SelectObject(hdc, hbmpXSav);
        DeleteObject(hbmpScreen);
        DeleteDC(hdc);
    }
    return 0;
}

void DrawScanFleetCount(FLEET *lpfl, int16_t x, int16_t y, HDC hdc, HDC hdcMem) {
    int32_t  l2;
    int16_t  f999;
    COLORREF cr;
    FLEET   *lpflWalk;
    int16_t  iPlr;
    HBITMAP  hbmpSav;
    int32_t  l;

    lpflWalk = lpfl;
    iPlr = !(grbitScan & grbitScanPlayerColors) ? -2 : -1;
    l = 0;
    f999 = FALSE;
    do {
        l2 = CShipsScanVis(lpflWalk);
        if (l2 > 0) {
            l += l2;
            if (lpflWalk->iPlayer != iPlr && iPlr != -2) {
                if (iPlr == iplrNone) {
                    iPlr = lpflWalk->iPlayer;
                } else {
                    iPlr = -2;
                }
            }
        }
        lpflWalk->fDone = TRUE;
        lpflWalk = lpflWalk->lpflNext;
    } while (lpflWalk != lpfl);
    if (l > 999) {
        l = 999;
    } else if (l <= 0) {
        return;
    }
    hbmpSav = SelectObject(hdcMem, hbmpNumbers);
    SetBkColor(hdc, 0);
    SetTextColor(hdc, 0xffffff);
    if (iPlr >= 0 && iPlr != idPlayer) {
        cr = rgcrPlrHistory[iPlr];
    } else {
        cr = 0xffffff;
    }
    x--;
    y -= 7;
    if (l > 99) {
        f999 = TRUE;
        x -= 5;
        if (cr != 0xffffff) {
            BitBlt(hdc, x, y, 4, 7, hdcMem, (int16_t)LOWORD(l) / 100 * 4, 0, 2229030);
            SetTextColor(hdc, cr);
        }
        BitBlt(hdc, x, y, 4, 7, hdcMem, (int16_t)LOWORD(l) / 100 * 4, 0, SRCPAINT);
        if (cr != 0xffffff) {
            SetTextColor(hdc, 0xffffff);
        }
        x += 8;
        l = (int32_t)(l % 100);
    }
    if (l > 9 || f999) {
        x -= 3;
        if (cr != 0xffffff) {
            BitBlt(hdc, x, y, 4, 7, hdcMem, (int16_t)LOWORD(l) / 10 * 4, 0, 2229030);
            SetTextColor(hdc, cr);
        }
        BitBlt(hdc, x, y, 4, 7, hdcMem, (int16_t)LOWORD(l) / 10 * 4, 0, SRCPAINT);
        if (cr != 0xffffff) {
            SetTextColor(hdc, 0xffffff);
        }
        x += 5;
        l = (int32_t)(l % 10);
    }
    if (cr != 0xffffff) {
        BitBlt(hdc, x, y, 4, 7, hdcMem, LOWORD(l) * 4, 0, 2229030);
        SetTextColor(hdc, cr);
    }
    BitBlt(hdc, x, y, 4, 7, hdcMem, LOWORD(l) * 4, 0, SRCPAINT);
    if (cr != 0xffffff) {
        SetTextColor(hdc, 0xffffff);
    }
    SelectObject(hdcMem, hbmpSav);
    return;
}

int32_t CShipsScanVis(FLEET *lpfl) {
    int16_t  j;
    int32_t  csh;
    int16_t  k;
    uint16_t grbitSh;

    csh = 0;
    if (grbitScan & grbitScanIdleFleets) {
        if (lpfl->iPlayer == idPlayer &&
            (lpfl->cord != 1 || lpfl->lpplord->rgord[0].grTask == grTaskLayMines || lpfl->lpplord->rgord[0].grTask == grTaskScrap ||
             lpfl->lpplord->rgord[0].grTask == grTaskMine || lpfl->lpplord->rgord[0].grTask == grTaskPatrol)) {
            return 0;
        }
        if (lpfl->iPlayer != idPlayer && lpfl->iwarpFlt == 0) {
            return 0;
        }
    }
    if ((grbitScan & grbitScanDesignFilter) && lpfl->iPlayer == idPlayer) {
        grbitSh = grbitScanShip;
        j = 0;
        for (; grbitSh != 0; grbitSh >>= 1) {
            if ((grbitSh & 1) && lpfl->rgcsh[j] > 0) {
                csh += lpfl->rgcsh[j];
            }
            j++;
        }
    } else if ((grbitScan & grbitScanEnemyFilter) && lpfl->iPlayer != idPlayer) {
        grbitSh = grbitScanEShip;
        j = 0;
        for (; grbitSh != 0; grbitSh >>= 1) {
            if (grbitSh & 1) {
                for (k = 0; k < 16; k++) {
                    if (lpfl->rgcsh[k] > 0 && LphuldefFromId(rglpshdef[lpfl->iPlayer][k].hul.ihuldef)->imdCategory == j) {
                        csh += lpfl->rgcsh[k];
                    }
                }
            }
            j++;
        }
    } else {
        for (j = 0; j < 16; j++) {
            csh += lpfl->rgcsh[j];
        }
    }
    return csh;
}

void DrawRadarCircle(DRAWCIR *pdc, RECT *prc) {
    int16_t  y2;
    int32_t  r2;
    COLORREF crSav;
    int16_t  dy;
    int16_t  y;
    int16_t  iFree;
    int16_t  i;
    int16_t  dx;
    int16_t  x2;
    int16_t  rad;
    int32_t  l;
    int16_t  x;
    RECT     rc;

    if (!prc || (!pdc->fCovered && IntersectRect(&rc, prc, &pdc->rcClip) != 0)) {
        if (prc) {
            rad = (prc->right - prc->left) >> 1;
            x = prc->left + rad;
            y = prc->top + rad;
            r2 = (uint32_t)(rad * rad);
            for (i = 0; i < 4; i++) {
                x2 = i >= 2 ? pdc->rcClip.right : pdc->rcClip.left;
                y2 = !(i & 1) ? pdc->rcClip.bottom : pdc->rcClip.top;
                dx = x2 - x;
                dy = y2 - y;
                if (dx > r2 || dy > r2 || (uint32_t)(dx * dx) + (uint32_t)(dy * dy) > r2)
                    break;
            }
            if (i == 4) {
                pdc->fCovered = TRUE;
                pdc->cCur = 0;
            DrawEllipse:
                if (pdc->fHollowOut) {
                    SetROP2(pdc->hdc, R2_MASKPEN);
                    crSav = SetBkColor(pdc->hdc, 0xffffff);
                    Ellipse(pdc->hdc, prc->left, prc->top, prc->right, prc->bottom);
                    SetBkColor(pdc->hdc, crSav);
                    SetROP2(pdc->hdc, R2_MERGEPEN);
                }
                Ellipse(pdc->hdc, prc->left, prc->top, prc->right, prc->bottom);
                return;
            } else if (pdc->cCur == pdc->cMax) {
                goto DrawEllipse;
            } else {
                iFree = pdc->cCur;
                for (i = 0; i < pdc->cCur; i++) {
                    if (pdc->rgrad[i] <= 0) {
                        iFree = i;
                    } else {
                        x2 = pdc->rgx[i];
                        y2 = pdc->rgy[i];
                        dx = x - x2;
                        dy = y - y2;
                        l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                        if (pdc->rgrad[i] < rad) {
                            if (Sf80Le(Sf80Add(Sf80From64(Sf64Sqrt(Sf64FromI32(l))), Sf80FromI32(pdc->rgrad[i])), Sf80FromI32(rad))) {
                                pdc->rgrad[i] = 0;
                                iFree = i;
                            }
                        } else if (pdc->rgrad[i] > rad) {
                            if (Sf80Le(Sf80Add(Sf80From64(Sf64Sqrt(Sf64FromI32(l))), Sf80FromI32(rad)), Sf80FromI32(pdc->rgrad[i]))) {
                                return;
                            }
                        } else if (x2 == x && y2 == y) {
                            return;
                        }
                    }
                }
                pdc->rgx[iFree] = x;
                pdc->rgy[iFree] = y;
                pdc->rgrad[iFree] = rad;
                if (iFree != pdc->cCur) {
                    return;
                }
                pdc->cCur++;
                return;
            }
        } else {
            if (pdc->fHollowOut) {
                SetROP2(pdc->hdc, R2_MASKPEN);
                crSav = SetBkColor(pdc->hdc, 0xffffff);
                l = 1;
            }
        L2ndEl:
            for (i = 0; i < pdc->cCur; i++) {
                if (pdc->rgrad[i] > 0) {
                    x = pdc->rgx[i];
                    y = pdc->rgy[i];
                    rad = pdc->rgrad[i];
                    Ellipse(pdc->hdc, x - rad, y - rad, x + rad + 1, y + rad + 1);
                }
            }
            if (pdc->fHollowOut && l == 1) {
                l = 0;
                SetBkColor(pdc->hdc, crSav);
                SetROP2(pdc->hdc, R2_MERGEPEN);
                goto L2ndEl;
            }

            pdc->fCovered = FALSE;
            pdc->cCur = 0;
        }
    }
    return;
}

// DrawPathYearTicks draws a tick across the selected fleet's leg from
// ptFrom to ptTo, logical points, where each year at iWarp ends: a fleet
// moves iWarp squared light years a year and stops at each waypoint. Ticks
// closer than 6 pixels are left out. It leaves the pen at ptTo. The path is
// drawn with R2_XORPEN, so drawing it again erases the ticks too; each tick
// is two halves that skip the path's own pixel, which XOR would clear.
static void DrawPathYearTicks(HDC hdc, POINT16 ptFrom, POINT16 ptTo, int16_t iWarp) {
    POINT16 ptA;
    POINT16 ptB;
    POINT16 pt;
    double  dLeg;
    double  dScan;
    double  dxTick;
    double  dyTick;
    int16_t dYear;
    int16_t i;
    int16_t iSide;

    ptA = ptFrom;
    LogicalToScan(&ptA);
    ptB = ptTo;
    LogicalToScan(&ptB);
    if (iWarp >= 1 && iWarp <= 10) {
        dYear = iWarp * iWarp;
        dLeg = Sf64Hypot(Sf64FromI32(ptTo.x - ptFrom.x), Sf64FromI32(ptTo.y - ptFrom.y));
        dScan = Sf64Hypot(Sf64FromI32(ptB.x - ptA.x), Sf64FromI32(ptB.y - ptA.y));
        if (Sf64Lt(Sf64FromI32(dYear), dLeg) && Sf64Le(Sf64FromI32(6), Sf64Div(Sf64Mul(dScan, Sf64FromI32(dYear)), dLeg))) {
            // A unit step across the leg on screen.
            dxTick = Sf64Div(Sf64FromI32(-(ptB.y - ptA.y)), dScan);
            dyTick = Sf64Div(Sf64FromI32((ptB.x - ptA.x)), dScan);
            for (i = 1; Sf64Lt(Sf64FromI32(i * dYear), dLeg); i++) {
                pt.x = (int16_t)Sf64ToI32(Sf64Floor(Sf64Add(Sf64Add(Sf64FromI32(ptA.x), Sf64Div(Sf64FromI32((ptB.x - ptA.x) * i * dYear), dLeg)), 0.5)));
                pt.y = (int16_t)Sf64ToI32(Sf64Floor(Sf64Add(Sf64Add(Sf64FromI32(ptA.y), Sf64Div(Sf64FromI32((ptB.y - ptA.y) * i * dYear), dLeg)), 0.5)));
                for (iSide = -1; iSide <= 1; iSide += 2) {
                    MoveToEx(hdc, pt.x + (int16_t)Sf64ToI32(Sf64Floor(Sf64Add(Sf64Mul(Sf64FromI32(iSide), dxTick), 0.5))),
                             pt.y + (int16_t)Sf64ToI32(Sf64Floor(Sf64Add(Sf64Mul(Sf64FromI32(iSide), dyTick), 0.5))), NULL);
                    LineTo(hdc, pt.x + (int16_t)Sf64ToI32(Sf64Floor(Sf64Add(Sf64Mul(Sf64FromI32(iSide * 4), dxTick), 0.5))),
                           pt.y + (int16_t)Sf64ToI32(Sf64Floor(Sf64Add(Sf64Mul(Sf64FromI32(iSide * 4), dyTick), 0.5))));
                }
            }
        }
    }
    MoveToEx(hdc, ptB.x, ptB.y, NULL);
}

// SetScanPathWarp sets the warp of the selected fleet's waypoint iwp. Its
// path's year ticks depend on the warp, so a path on screen is erased
// first and drawn again after.
void SetScanPathWarp(int16_t iwp, int16_t iWarp) {
    int16_t fVis;

    fVis = fOrdersVis;
    if (fVis) {
        DrawShipScanPath(NULL, FALSE);
    }
    sel.fl.lpplord->rgord[iwp].iWarp = iWarp;
    if (fVis) {
        DrawShipScanPath(NULL, TRUE);
    }
    return;
}

void DrawShipScanPath(HDC hdc, int16_t fShow) {
    ORDER  *lpord2;
    int16_t rgDup[87];
    int16_t j;
    HPEN    hpenSav;
    POINT16 pt2;
    POINT16 pt;
    int16_t iRopSav;
    int16_t dy;
    ORDER  *lpord1;
    POINT16 ptCur;
    FLEET  *lpfl;
    int16_t i;
    int16_t fHdc;
    int32_t lWarp2;
    int16_t dRad;
    int16_t dx;
    RECT    rc;
    THING  *lpth;
    double  dAngle;
    POINT16 rgptArrow[2];
    int16_t dx5;
    POINT16 ptTick;
    int16_t dy5;
    double  m;
    int16_t id;
    int16_t fDoneRoute;

    fHdc = FALSE;
    if ((sel.grobj == grobjFleet || sel.scan.grobj == grobjFleet || sel.scan.grobj == grobjThing ||
         (sel.grobj == grobjPlanet && ((sel.pl.fStarbase && sel.pl.idFling != 0) || sel.pl.idRoute != 0))) &&
        (fShow != fOrdersVis && !gd.fNoScannerDraw)) {
        fOrdersVis = fShow;
        if (!hdc) {
            hdc = GetDC(hwndScanner);
            fHdc = TRUE;
        }
        if (sel.scan.grobj == grobjThing && sel.scan.ith != ithNone) {
            lpth = lpThings + sel.scan.ith;
            ptCur = lpth->pt;
            if (lpth->ith == ithMysteryTrader) {
                dx = lpth->tht.ptDest.x - ptCur.x;
                dy = lpth->tht.ptDest.y - ptCur.y;
                lWarp2 = lpth->tht.iWarp;
            } else {
                if (lpth->ith != ithMineralPacket || lpth->thp.iWarp == 0)
                    goto LNextCheck;
                dx = rgptPlan[lpth->thp.idPlanet].x - ptCur.x;
                dy = rgptPlan[lpth->thp.idPlanet].y - ptCur.y;
                lWarp2 = (uint32_t)(lpth->thp.iWarp + 4);
            }
            if (dx != 0 || dy != 0) {
                lWarp2 = (uint32_t)(lWarp2 * (uint32_t)(lWarp2 * 5));
                lpfl = NULL;
                goto LCommonLineCode;
            }
        }
    LNextCheck:
        if (sel.scan.grobj != grobjFleet)
            goto LNoObjPath;
        lpfl = rglpfl[sel.scan.ifl];
        ptCur = lpfl->pt;
        if (!lpfl->fdirValid || lpfl->iwarpFlt <= 0)
            goto LNoObjPath;
        dx = lpfl->dirFltX - 127;
        dy = lpfl->dirFltY - 127;
        if (dx == 0 && dy == 0)
            goto LNoObjPath;
        lWarp2 = (uint32_t)(lpfl->iwarpFlt * lpfl->iwarpFlt * 5);
    LCommonLineCode:
        GetClientRect(hwndScanner, &rc);
        ExcludeClipRect(hdc, rc.left, rc.bottom - dySBar, rc.right, rc.bottom);
        hpenSav = SelectObject(hdc, hpenStarbase);
        iRopSav = SetROP2(hdc, R2_XORPEN);
        if (dx == 0) {
            dx5 = 0;
            dy5 = dy >= 0 ? LOWORD(lWarp2) : -LOWORD(lWarp2);
        } else {
            lWarp2 = (uint32_t)(lWarp2 * lWarp2);
            m = Sf64From80((Sf80Div(Sf80FromI32(dy), Sf80FromI32(dx))));
            dx5 = LOWORD(Sf64ToI32(Sf64Sqrt(Sf64From80((Sf80Div(Sf80FromI32(lWarp2), (Sf80Add(Sf80Mul(Sf80From64(m), Sf80From64(m)), Sf80FromI32(1)))))))));
            if (dx < 0) {
                dx5 = -dx5;
            }
            dy5 = LOWORD((int32_t)((int32_t)(dx5 * dy) / dx));
        }
        LogicalToScan(&ptCur);
        dx5 = PtToScan(dx5);
        dy5 = -PtToScan(dy5);
        MoveToEx(hdc, ptCur.x - dx5, ptCur.y - dy5, NULL);
        LineTo(hdc, ptCur.x + dx5, ptCur.y + dy5);
        if (dy == 0) {
            ptTick.x = 0;
            ptTick.y = dx >= 0 ? 4 : -4;
        } else {
            m = Sf64From80((Sf80Div(Sf80FromI32((int16_t)-dx), Sf80FromI32(dy))));
            ptTick.x = LOWORD(Sf64ToI32(Sf64Sqrt(Sf64From80((Sf80Div(Sf80From64(24.0), (Sf80Add(Sf80Mul(Sf80From64(m), Sf80From64(m)), Sf80FromI32(1)))))))));
            if (ptTick.x == 0) {
                ptTick.y = dy >= 0 ? 4 : -4;
            } else {
                if (dx > 0) {
                    ptTick.x = -ptTick.x;
                }
                ptTick.y = LOWORD((int32_t)((int32_t)(ptTick.x * dx) / dy));
            }
        }
        dAngle = Sf64From80((Sf80Sub(Sf80From64(Sf64Atan2(Sf64FromI32((int16_t)-dy), Sf64FromI32((int16_t)-dx))), Sf80From64(0.7853982))));
        for (i = 0; i < 2; i++) {
            rgptArrow[i].x = LOWORD(Sf80ToI32((Sf80Add(Sf80Mul(Sf80FromI32(5), Sf80From64(Sf64Cos(dAngle))), Sf80From64(0.5)))));
            rgptArrow[i].y = LOWORD(Sf80ToI32((Sf80Add(Sf80Mul(Sf80FromI32(5), Sf80From64(Sf64Sin(dAngle))), Sf80From64(0.5)))));
            dAngle = Sf64From80((Sf80Add(Sf80From64(dAngle), Sf80From64(1.5707964))));
        }
        j = -5;
        for (i = -5; i <= 5; i++) {
            if (i == 0) {
                j = 5;
            } else {
                pt.x = LOWORD((int32_t)(((int32_t)((uint32_t)(dx5 * i) * 2) + j) / 10)) + ptCur.x;
                pt.y = LOWORD((int32_t)(((int32_t)((uint32_t)(dy5 * i) * 2) + j) / 10)) + ptCur.y;
                if (i > 0) {
                    for (j = 0; j < 2; j++) {
                        MoveToEx(hdc, pt.x + rgptArrow[j].x, pt.y - rgptArrow[j].y, NULL);
                        LineTo(hdc, pt.x, pt.y);
                    }
                } else {
                    MoveToEx(hdc, pt.x + ptTick.x, pt.y + ptTick.y, NULL);
                    LineTo(hdc, pt.x - ptTick.x, pt.y - ptTick.y);
                    LineTo(hdc, pt.x - ptTick.x, pt.y - ptTick.y - 1);
                }
            }
        }
        SetROP2(hdc, iRopSav);
        SelectObject(hdc, hpenSav);
    LNoObjPath:
        if (sel.grobj == grobjPlanet) {
            fDoneRoute = FALSE;
            if (sel.pl.fStarbase && sel.pl.idFling != 0) {
                hpenSav = SelectObject(hdc, hpenDkPurple);
                id = sel.pl.idFling - 1;
            LDrawPath:
                iRopSav = SetROP2(hdc, R2_XORPEN);
                GetClientRect(hwndScanner, &rc);
                ExcludeClipRect(hdc, rc.left, rc.bottom - dySBar, rc.right, rc.bottom);
                pt = rgptPlan[sel.pl.id];
                LogicalToScan(&pt);
                MoveToEx(hdc, pt.x, pt.y, NULL);
                pt = rgptPlan[id];
                LogicalToScan(&pt);
                LineTo(hdc, pt.x, pt.y);
                SetROP2(hdc, iRopSav);
                SelectObject(hdc, hpenSav);
            }
            if (sel.pl.idRoute == 0 || fDoneRoute)
                goto LFinishUp;
            fDoneRoute = TRUE;
            id = sel.pl.idRoute - 1;
            hpenSav = SelectObject(hdc, hpenDkGreen);
            goto LDrawPath;
        }
        if (sel.grobj == grobjFleet && sel.fl.cord > 1) {
            memset(rgDup, 0, sel.fl.cord * 2);
            lpord1 = sel.fl.lpplord->rgord;
            pt = lpord1->pt;
            i = 1;
            while (i < sel.fl.cord) {
                lpord2 = lpord1 + 1;
                pt2 = lpord2->pt;
                if (rgDup[i] != 0)
                    goto DoNext;

                j = i + 1;
                while (j < sel.fl.cord) {
                    if ((pt.x == lpord2->pt.x && pt.y == lpord2->pt.y && pt2.x == lpord2[1].pt.x && pt2.y == lpord2[1].pt.y) ||
                        (pt2.x == lpord2->pt.x && pt2.y == lpord2->pt.y && pt.x == lpord2[1].pt.x && pt.y == lpord2[1].pt.y)) {
                        rgDup[i] = 1;
                        rgDup[j] = 2;
                    }
                    j++;
                    lpord2++;
                }

            DoNext:
                pt = pt2;
                i++;
                lpord1++;
            }
            GetClientRect(hwndScanner, &rc);
            ExcludeClipRect(hdc, rc.left, rc.bottom - dySBar, rc.right, rc.bottom);
            if (fShow && (grbitScan & grbitScanFleetPaths)) {
                hpenSav = SelectObject(hdc, hpenStarbase);
                pt = sel.fl.lpplord->rgord[0].pt;
                LogicalToScan(&pt);
                MoveToEx(hdc, pt.x, pt.y, NULL);
                for (i = 1; i < sel.fl.cord; i++) {
                    pt2 = sel.fl.lpplord->rgord[i].pt;
                    LogicalToScan(&pt2);
                    LineTo(hdc, pt2.x, pt2.y);
                    pt = pt2;
                }
                SelectObject(hdc, hpenSav);
            }
            hpenSav = SelectObject(hdc, hpenShip);
            iRopSav = SetROP2(hdc, R2_XORPEN);
            pt = sel.fl.lpplord->rgord[0].pt;
            dRad = pt.x == sel.pt.x && pt.y == sel.pt.y ? 5 : 5;
            LogicalToScan(&pt);
            ExcludeClipRect(hdc, pt.x - dRad, pt.y - dRad, pt.x + dRad + 1, pt.y + dRad + 1);
            MoveToEx(hdc, pt.x, pt.y, NULL);
            for (i = 1; i < sel.fl.cord; i++) {
                pt2 = sel.fl.lpplord->rgord[i].pt;
                dRad = pt2.x == sel.pt.x && pt2.y == sel.pt.y ? 5 : 5;
                LogicalToScan(&pt2);
                ExcludeClipRect(hdc, pt2.x - dRad, pt2.y - dRad, pt2.x + dRad + 1, pt2.y + dRad + 1);
                if (rgDup[i] == 2) {
                    MoveToEx(hdc, pt2.x, pt2.y, NULL);
                } else {
                    if (rgDup[i] == 1) {
                        SelectObject(hdc, (grbitScan & grbitScanFleetPaths) ? hpenYellow : GetStockObject(WHITE_PEN));
                    }
                    LineTo(hdc, pt2.x, pt2.y);
                    if (rgDup[i] == 1) {
                        SelectObject(hdc, hpenShip);
                    }
                    DrawPathYearTicks(hdc, sel.fl.lpplord->rgord[i - 1].pt, sel.fl.lpplord->rgord[i].pt, sel.fl.lpplord->rgord[i].iWarp);
                }
                pt = pt2;
            }
            SetROP2(hdc, iRopSav);
            SelectObject(hdc, hpenSav);
            SelectClipRgn(hdc, hrgnHuge);
        }
    LFinishUp:
        if (fHdc) {
            ReleaseDC(hwndScanner, hdc);
        }
    }
    return;
}

void DrawScannerSBar(HDC hdc, RECT *prc, SBAR *psbar, int16_t fFullRedraw) {
    int16_t    fhdc;
    COLORREF   crText;
    POINT16    pt2;
    int16_t    id;
    POINT16    pt;
    int16_t    grReal;
    int16_t    iBkPrev;
    int16_t    c;
    COLORREF   crBk;
    int16_t    dxHole;
    HFONT      hfontSav;
    RECT       rcClip;
    char      *psz;
    HBRUSH     hbrSav;
    int16_t    fDoName;
    int32_t    l;
    GrobjClass grobj;
    RECT       rcT;
    RECT       rc;
    char       szBuf[100];

    fhdc = 0;
    fDoName = TRUE;
    GetClientRect(hwndScanner, &rc);
    rc.top = rc.bottom - dySBar;
    if (!prc || IntersectRect(&rcT, prc, &rc) != 0) {
        if (!hdc) {
            fhdc = 1;
            hdc = GetDC(hwndScanner);
        }
        hfontSav = SelectObject(hdc, rghfontArial8[1]);
        if (psbar) {
            grobj = psbar->grbit;
        } else {
            grobj = sel.scan.grobj;
        }
        iBkPrev = SetBkMode(hdc, TRANSPARENT);
        crBk = SetBkColor(hdc, crButtonFace);
        crText = SetTextColor(hdc, crButtonText);
        hbrSav = SelectObject(hdc, hbrButtonFace);
        if (fFullRedraw) {
            PatBlt(hdc, rc.left, rc.top, rc.right - rc.left, dySBar, PATCOPY);
            SelectObject(hdc, hbrButtonHilite);
            PatBlt(hdc, rc.left, rc.top, 1, dySBar, PATCOPY);
            PatBlt(hdc, rc.left, rc.top, rc.right - rc.left, 1, PATCOPY);
        }
        rc.bottom -= dySBar >> 1;
        rcT = rc;
        rcT.top += 4;
        rcT.bottom -= 4;
        rcT.left += 4;
        if (rc.right < 360)
            goto DrawTheName;

        l = GetTextExtent(hdc, "ID #000", 7);
        dxHole = LOWORD(l) + 6;
        rcT.right = LOWORD(l) + 6 + rcT.left;
        DrawLockLight(hdc, &rcT, fFullRedraw);
        if (grobj & grobjPlanet) {
            if (psbar) {
                id = psbar->id;
            } else {
                id = sel.scan.idpl;
            }
            if (id != idplNone) {
                c = wsprintf(szWork, "ID #%d", id + 1);
                SetBkMode(hdc, OPAQUE);
                TextOut(hdc, rcT.left + 3, rcT.top + 2, szWork, c);
            }
        } else if (grobj == grobjOther) {
            if (psbar) {
                id = psbar->id;
            } else {
                id = sel.scan.iwp;
            }
            c = wsprintf(szWork, "WP #%d", id);
            TextOut(hdc, rcT.left + 3, rcT.top + 2, szWork, c);
        }
        l = GetTextExtent(hdc, "X: 8888", 7);
        rcT.left += dxHole + 4;
        dxHole = LOWORD(l) + 6;
        rcT.right = LOWORD(l) + 6 + rcT.left;
        DrawLockLight(hdc, &rcT, fFullRedraw);
        if (psbar) {
            pt = psbar->pt;
            goto GotCoords;
        } else if (grobj != grobjNone) {
            pt = sel.scan.pt;
            goto GotCoords;
        } else {
            pt.y = 0;
            pt.x = 0;
        }
    GotCoords:
        if (pt.x > 0) {
            c = wsprintf(szWork, "X: %d", pt.x);
            SetBkMode(hdc, OPAQUE);
            TextOut(hdc, rcT.left + 3, rcT.top + 2, szWork, c);
        }
        rcT.left += dxHole + 4;
        dxHole = LOWORD(l) + 6;
        rcT.right = LOWORD(l) + 6 + rcT.left;
        DrawLockLight(hdc, &rcT, fFullRedraw);
        if (pt.y > 0) {
            c = wsprintf(szWork, "Y: %d", pt.y);
            SetBkMode(hdc, OPAQUE);
            TextOut(hdc, rcT.left + 3, rcT.top + 2, szWork, c);
        }
        rcT.left += dxHole + 4;

    DrawTheName:
        dxHole = rc.right - rcT.left - 4;
        rcT.right = rc.right - rcT.left - 4 + rcT.left;
        DrawLockLight(hdc, &rcT, fFullRedraw);
        rcClip = rcT;
        ExpandRc(&rcClip, -2, -2);
        grReal = grobj == grobjOther ? sel.scan.grobjFull : grobj;
        if (psbar && psbar->psz) {
            strcpy(szWork, psbar->psz);
        } else if (grReal & 1) {
            PszGetPlanetName(!psbar ? sel.scan.idpl : psbar->id);
        } else if (grReal & 2) {
            PszGetFleetName(!psbar ? rglpfl[sel.scan.ifl]->id : psbar->id);
        } else if (grReal & 8) {
            PszGetThingName(!psbar ? lpThings[sel.scan.ith].idFull : psbar->id);
        } else if (grReal == 0 && grobj == grobjOther) {
            CchGetString(idsDeepSpaceWaypoint, szWork);
        } else {
            fDoName = FALSE;
        }
        if (fDoName) {
            ExtTextOut(hdc, rcT.left + 3, rcT.top + 2, ETO_CLIPPED, &rcClip, szWork, lstrlen(szWork), NULL);
        }
        OffsetRc(&rc, 0, dySBar >> 1);
        rcT = rc;
        rcT.top += 4;
        rcT.bottom -= 4;
        rcT.left += 4;
        rcT.right -= 4;
        DrawLockLight(hdc, &rcT, fFullRedraw);
        rcClip = rcT;
        ExpandRc(&rcClip, -2, -2);
        if (psbar) {
            pt = psbar->pt;
        } else if (grobj != grobjNone) {
            pt = sel.scan.pt;
        } else {
            pt.y = 0;
            pt.x = 0;
        }
        if (psbar && psbar->pscan) {
            pt2 = psbar->pscan->pt;
        } else if (sel.grobj == grobjFleet || sel.grobj == grobjPlanet) {
            pt2 = sel.pt;
        } else {
            pt2.x = -1;
        }
        if (pt.x != -1 && pt2.x != -1 && (pt.x != pt2.x || pt.y != pt2.y)) {
            strcpy(szBuf, PszGetDistance(pt.x, pt.y, pt2.x, pt2.y));
            for (psz = szBuf; *psz != ' '; psz++) {
            }
            CchGetString(idsLy + (rc.right >= 350), psz + 1);
            if (!psbar || !psbar->pscan) {
                CchGetString(idsFrom, psz + strlen(psz));
                strcat(psz + 8, PszGetLocName(sel.grobj, sel.id, pt2.x, pt2.y));
            }
            ExtTextOut(hdc, rcT.left + 3, rcT.top + 2, ETO_CLIPPED, &rcClip, szBuf, strlen(szBuf), NULL);
        }
        SelectObject(hdc, hbrSav);
        SetBkMode(hdc, iBkPrev);
        SetTextColor(hdc, crButtonText);
        SetBkColor(hdc, crBk);
        SelectObject(hdc, hfontSav);
        if (fhdc) {
            ReleaseDC(hwndScanner, hdc);
        }
    }
    return;
}

void DrawLockLight(HDC hdc, RECT *prc, int16_t fFullRedraw) {
    int16_t dy;
    int16_t dx;
    RECT    rc;

    dx = prc->right - prc->left;
    dy = prc->bottom - prc->top;
    if (fFullRedraw) {
        SelectObject(hdc, hbrButtonShadow);
        PatBlt(hdc, prc->left, prc->top, 1, dy, PATCOPY);
        PatBlt(hdc, prc->left, prc->top, dx, 1, PATCOPY);
        SelectObject(hdc, hbrButtonHilite);
        PatBlt(hdc, prc->right, prc->top, 1, dy + 1, PATCOPY);
        PatBlt(hdc, prc->left, prc->bottom, dx, 1, PATCOPY);
    } else {
        rc = *prc;
        ExpandRc(&rc, -2, -2);
        FillRect(hdc, &rc, hbrButtonFace);
    }
    return;
}

void SetScanScrollBars(HWND hwnd) {
    int16_t xMax;
    int16_t dy;
    int16_t yMax;
    int16_t dx;
    RECT    rc;

    fInScrollSet = TRUE;
    GetClientRect(hwnd, &rc);
    dx = ScanToPt(rc.right);
    dy = ScanToPt(rc.bottom - dySBar);
    xMax = ((1000 <= dGalInv - 1000 - dx ? dGalInv - 0x3e8 - dx : 0x3e8) + 3) & 0xfffc;
    yMax = ((1000 <= dGalInv - 1000 - dy ? dGalInv - 0x3e8 - dy : 0x3e8) + 3) & 0xfffc;
    SetScrollRange(hwnd, SB_HORZ, 1000, xMax, TRUE);
    if (fInScrollSet) {
        SetScrollRange(hwnd, SB_VERT, 1000, yMax, TRUE);
        if (fInScrollSet) {
            dScanPage = (int16_t)(dx >= dy ? dy : dx) / 3 & 0xfffc;
            dScanInc = (dScanPage / 8 + 2) & 0xfffc;
            fInScrollSet = FALSE;
        }
    }
    return;
}

void ScrollScanner(int16_t dx, int16_t dy) {
    HDC     hdc;
    RECT    rcUpd;
    RECT    rcUpd2;
    RECT    rc;
    int16_t fPending;

    if ((dx != 0 || dy != 0) && IsWindowVisible(hwndScanner) != 0 && !gd.fNoScannerDraw) {
        hdc = GetDC(hwndScanner);
        GetClientRect(hwndScanner, &rc);
        // The scroll handlers move the scanner's top before calling here, so
        // a repaint still pending would be drawn at the new position and
        // then scrolled with the rest, leaving a band drawn out of place.
        // A wheel zoom invalidates the whole scanner, and a trackpad's next
        // scroll comes before its WM_PAINT. Redraw the map instead.
        fPending = GetUpdateRect(hwndScanner, NULL, FALSE) != 0;
        if (abs(dx) > rc.right >> 1 || abs(dy) > rc.bottom >> 1 || fDlgUp || hwndBrowser || fPending) {
            DrawScanner(hdc, &rc);
            if (fPending) {
                rc.bottom -= dySBar;
                ValidateRect(hwndScanner, &rc);
            }
            goto RelDC;
        }
        rc.bottom -= dySBar;
        UpdateWindow(hwndScanner);
        ScrollWindow(hwndScanner, dx, dy, &rc, &rc);
        if (dy > 0) {
            SetRect(&rcUpd2, 0, 0, rc.right, dy);
            rc.top += dy;
        } else if (dy < 0) {
            SetRect(&rcUpd2, 0, rc.bottom + dy, rc.right, rc.bottom);
            rc.bottom += dy;
        }
        if (dx > 0) {
            SetRect(&rcUpd, 0, rc.top, dx, rc.bottom);
            rc.right -= dx;
        } else if (dx < 0) {
            rcUpd = rc;
            rcUpd.left = rc.right + dx;
            rc.left -= dx;
        }
        if (dx != 0) {
            ValidateRect(hwndScanner, &rcUpd);
        }
        if (dy != 0) {
            ValidateRect(hwndScanner, &rcUpd2);
        }
        if (dx != 0) {
            DrawScanner(hdc, &rcUpd);
        }
        if (dy != 0) {
            DrawScanner(hdc, &rcUpd2);
        }
        UpdateWindow(hwndScanner);
    RelDC:
        ReleaseDC(hwndScanner, hdc);
    }
    return;
}

void RedrawScanSel(HDC hdc, int16_t fVis) {
    int16_t sel_grobj;
    int16_t fhdc;
    int16_t dOff;
    POINT16 pt;
    int16_t sel_id;
    int16_t fNoSelRedraw;
    SCAN    sel_scan;
    RECT    rc;
    int16_t sel_grobjFull;

    fhdc = 0;
    sel_id = sel.id;
    sel_grobj = sel.grobj;
    sel_grobjFull = sel.grobjFull;
    sel_scan = sel.scan;
    if (hwndScanner && IsWindowVisible(hwndScanner) != 0) {
        if (fVis == -1) {
            fVis = 0;
            fNoSelRedraw = TRUE;
        } else {
            fNoSelRedraw = FALSE;
        }
        if (!hdc) {
            fhdc = 1;
            hdc = GetDC(hwndScanner);
        }
        DrawShipScanPath(hdc, fVis);
        if (!fVis) {
            sel.scan.iwp = iwpNone;
            sel.scan.ifl = iflNone;
            sel.scan.idpl = idplNone;
            sel.id = -1;
            sel.grobjFull = grobjNone;
            sel.grobj = grobjNone;
            sel.scan.grobjFull = grobjNone;
            sel.scan.grobj = grobjNone;
        }
        dOff = iScanZoom >= zoom200 && (grbitScan & grbitScanViewMask) == 4 ? 11 : (grbitScan & grbitScanShipCounts) ? 7 : 0;
        if (sel_grobj != 0 && !fNoSelRedraw) {
            pt = sel.pt;
            LogicalToScan(&pt);
            SetRect(&rc, pt.x - 11 - dOff, pt.y - 11 - dOff, pt.x + 12 + dOff, pt.y + 23 + dOff);
            DrawScanner(hdc, &rc);
        }
        if (sel_scan.grobj != grobjNone && (sel_scan.pt.x != sel.pt.x || sel_scan.pt.y != sel.pt.y)) {
            pt = sel_scan.pt;
            LogicalToScan(&pt);
            SetRect(&rc, pt.x - 6 - dOff, pt.y - 6 - dOff, pt.x + 7 + dOff, pt.y + 15 + dOff);
            DrawScanner(hdc, &rc);
        }
        if (!fVis) {
            sel.id = sel_id;
            sel.grobj = sel_grobj;
            sel.grobjFull = sel_grobjFull;
            sel.scan = sel_scan;
        }
        if (fhdc) {
            ReleaseDC(hwndScanner, hdc);
        }
    }
    return;
}

int16_t FEnsurePointOnScreen(POINT16 pt, int16_t fScroll) {
    int16_t cy;
    int16_t fFix;
    int16_t cx;
    POINT16 ptCtr;
    RECT    rc;

    fFix = FALSE;
    GetClientRect(hwndScanner, &rc);
    rc.bottom -= dySBar;
    cx = ScanToPt(rc.right);
    cy = ScanToPt(rc.bottom);
    rc.left = xScanTop + 10;
    rc.right = xScanTop + cx - 20;
    rc.bottom = dGalInv - yScanTop - 10;
    rc.top = dGalInv - yScanTop - 10 - cy + 20;
    if (PtInRect(&rc, PointFrom16(pt)) != 0) {
        return TRUE;
    }
    ptCtr.x = (cx >> 1) + xScanTop;
    ptCtr.y = dGalInv - yScanTop - (cy >> 1);
    if (pt.x < rc.left) {
        ptCtr.x -= rc.left - pt.x;
    } else if (pt.x > rc.right) {
        ptCtr.x += pt.x - rc.right;
    }
    if (pt.y < rc.top) {
        ptCtr.y -= rc.top - pt.y;
    } else if (pt.y > rc.bottom) {
        ptCtr.y += pt.y - rc.bottom;
    }
    CtrPointScan(ptCtr, fScroll);
    return FALSE;
}

void CtrPointScan(POINT16 pt, int16_t fScroll) {
    int16_t dxCur;
    int16_t cy;
    int16_t y;
    int16_t cx;
    int16_t dyCur;
    int16_t x;
    RECT    rc;

    x = pt.x;
    y = pt.y;
    if (hwndScanner) {
        GetClientRect(hwndScanner, &rc);
        rc.bottom -= dySBar;
        cx = ScanToPt(rc.right);
        cy = ScanToPt(rc.bottom);
        x = x - (cx >> 1) <= 1000 ? 1000 : x - (cx >> 1);
        y = (cy >> 1) + y >= dGalInv - 1000 ? dGalInv - 1000 : (cy >> 1) + y;
        x = x >= dGalInv - 1000 - cx ? dGalInv - 1000 - cx : x;
        y = dGalInv - (y <= cy + 1000 ? cy + 1000 : y);
        x = x <= 1000 ? 1000 : x;
        y = y <= 1000 ? 1000 : y;
        dxCur = xScanTop;
        dyCur = yScanTop;
        x = (x + 2) & 0xfffc;
        y = (y + 2) & 0xfffc;
        if (dxCur != x || dyCur != y) {
            xScanTop = x;
            SetScrollPos(hwndScanner, SB_HORZ, x, TRUE);
            yScanTop = y;
            SetScrollPos(hwndScanner, SB_VERT, y, TRUE);
            if (fScroll) {
                ScrollScanner(PtToScan(dxCur - x), PtToScan(dyCur - y));
            } else {
                InvalidateRect(hwndScanner, NULL, FALSE);
            }
        }
    }
    return;
}

// ZoomScanAt sets the scanner zoom to iScanNew and keeps the galaxy point
// under ptScan, a scanner client point, where it is on screen. The menu's
// zoom command centers on the selection instead.
void ZoomScanAt(ScanZoom iScanNew, POINT16 ptScan) {
    POINT16 pt;
    RECT    rc;

    GetClientRect(hwndScanner, &rc);
    rc.bottom -= dySBar;
    if (ptScan.y >= rc.bottom) {
        ptScan.y = rc.bottom - 1;
    }
    pt = ptScan;
    ScanToLogical(&pt);
    SendMessage(hwndFrame, WM_COMMAND, iScanNew + 3905, 0);
    if (iScanZoom != iScanNew)
        return;
    pt.x += (ScanToPt(rc.right) >> 1) - ScanToPt(ptScan.x);
    pt.y += ScanToPt(ptScan.y) - (ScanToPt(rc.bottom) >> 1);
    CtrPointScan(pt, FALSE);
    return;
}

void LogicalToScan(POINT16 *ppt) {
    ppt->x = PtToScan(ppt->x - xScanTop);
    ppt->y = PtToScan(dGalInv - ppt->y - yScanTop);
    return;
}

void ScanToLogical(POINT16 *ppt) {
    ppt->x = ScanToPt(ppt->x) + xScanTop;
    ppt->y = dGalInv - (ScanToPt(ppt->y) + yScanTop);
    if (ppt->x > dGal + 1000) {
        ppt->x = dGal + 1000;
    }
    if (ppt->y < 1000) {
        ppt->y = 1000;
    }
    return;
}

int16_t FAddWayPoint(POINT16 ptIn, SCAN *pscan) {
    HDC     hdc;
    int16_t id;
    int16_t dy;
    ORDER  *lpord;
    int16_t lDist;
    POINT16 rgpt[3];
    int16_t dx;
    int16_t cpt;
    int16_t ipt;
    RECT    rc;

    if (sel.fl.cord == 87) {
        MessageBeep(MB_ICONASTERISK);
        wsprintf(szWork, PszGetCompressedString(idsCantHaveDWaypoints), 86);
        AlertSz(szWork, MB_ICONHAND);
        return FALSE;
    }
    if (grbitScan & grbitScanFleetPaths) {
        rgpt[0] = ptIn;
        rgpt[1] = pscan->pt;
        if (sel.iwpAct < sel.fl.cord - 1) {
            cpt = 3;
            rgpt[2] = sel.fl.lpplord->rgord[sel.iwpAct + 1].pt;
        } else {
            cpt = 2;
        }
    }
    dx = ptIn.x - pscan->pt.x;
    dy = ptIn.y - pscan->pt.y;
    lDist = ScanToPt(20);
    if ((int32_t)((uint32_t)(dx * dx) + (uint32_t)(dy * dy)) > (int16_t)(lDist * lDist)) {
        pscan->grobjFull = grobjOther;
        pscan->grobj = grobjOther;
        pscan->idpl = idplNone;
        pscan->ifl = iflNone;
        pscan->iwp = sel.iwpAct + 1;
        pscan->pt = ptIn;
    }
    lpord = &sel.fl.lpplord->rgord[sel.iwpAct];
    if (pscan->pt.x == lpord->pt.x && pscan->pt.y == lpord->pt.y) {
        return FALSE;
    }
    if (sel.iwpAct < sel.fl.cord - 1 && pscan->pt.x == lpord[1].pt.x && pscan->pt.y == lpord[1].pt.y) {
        return FALSE;
    }
    hdc = GetDC(hwndScanner);
    DrawShipScanPath(hdc, FALSE);
    if (sel.fl.lpplord->iordMax == sel.fl.cord) {
        sel.fl.lpplord = (PLORD *)LpplReAlloc((PL *)sel.fl.lpplord, sel.fl.cord + 3);
        lpord = &sel.fl.lpplord->rgord[sel.iwpAct + 1];
    } else {
        lpord++;
    }
    if (sel.iwpAct != sel.fl.cord - 1) {
        memmove(lpord + 1, lpord, (sel.fl.cord - sel.iwpAct - 1) * sizeof(ORDER));
    }
    *lpord = *(lpord - 1);
    lpord->pt = pscan->pt;
    switch (pscan->grobj) {
    case grobjPlanet:
        id = pscan->idpl;
        break;
    case grobjOther:
        id = pscan->iwp;
        break;
    case grobjFleet:
        id = rglpfl[pscan->ifl]->id;
        break;
    case grobjThing:
        id = lpThings[pscan->ith].idFull;
    }
    lpord->id = id;
    lpord->grobj = pscan->grobj;
    sel.fl.cord++;
    sel.fl.lpplord->iordMac++;
    lpord->iWarp = FAltDown() ? IWarpFastestForWaypoint(&sel.fl, lpord) : IWarpBestForWaypoint(&sel.fl, lpord);
    pscan->grobj = grobjOther;
    pscan->grobjFull |= grobjOther;
    pscan->iwp = sel.iwpAct + 1;
    RedrawScanSel(NULL, 0);
    FLookupFleet(idWriteBack, &sel.fl);
    if (lpord[-1].grTask == grTaskLayMines && lpord[-1].tsell.iPlrX == 5) {
        memset((uint8_t *)lpord - 10, 0, 10);
        lpord[-1].grTask = grTaskNone;
        FLookupFleet(idWriteBack, &sel.fl);
    }
    ChangeScanSel(pscan, 1);
    ReleaseDC(hwndScanner, hdc);
    if (grbitScan & grbitScanFleetPaths) {
        for (ipt = 0; ipt < cpt; ipt++) {
            LogicalToScan(&rgpt[ipt]);
        }
        BoundPoints(&rc, rgpt, cpt);
        InvalidateRect(hwndScanner, &rc, FALSE);
    }
    return TRUE;
}

int16_t FNearAWayPoint(POINT16 pt, int16_t fLogical) {
    ORDER  *lpord;
    int16_t i;
    SCAN    scan;

    if (sel.grobj != grobjFleet) {
        return FALSE;
    }
    if (!fLogical) {
        ScanToLogical(&pt);
    }
    if (!FFindNearestObject(pt, grobjPlanet | grobjFleet | grobjOther | grobjThing | mdScanRadius, &scan)) {
        return FALSE;
    }
    if (scan.grobjFull & grobjOther) {
        if (scan.pt.x == sel.pt.x && scan.pt.y == sel.pt.y) {
            lpord = &sel.fl.lpplord->rgord[1];
            i = 1;
            for (; i < sel.fl.cord && (lpord->pt.x != scan.pt.x || lpord->pt.y != scan.pt.y); lpord++) {
                i++;
            }
            if (i != sel.fl.cord) {
                return TRUE;
            }
            return FALSE;
        }
        return TRUE;
    }
    return FALSE;
}

int16_t FHandleWayPointDrag(POINT16 pt) {
    int16_t fChg;
    HDC     hdc;
    HPEN    hpenSav;
    SBAR    sbar;
    int16_t fMarker;
    char    szDeepSpace[40];
    int16_t fDup;
    int16_t grTypeIn;
    HCURSOR hcurSav;
    ORDER  *lpord;
    int16_t i;
    POINT16 ptLogical;
    POINT16 ptNext;
    int16_t fDel;
    POINT16 rgpt[4];
    POINT16 ptNew;
    int16_t cpt;
    POINT16 ptPrev;
    SCAN    scan;
    int16_t fFirst;
    RECT    rc;

    fFirst = TRUE;
    fMarker = FALSE;
    LogicalToScan(&pt);
    if (sel.iwpAct == 0) {
        lpord = &sel.fl.lpplord->rgord[1];
        i = 1;
        for (; i < sel.fl.cord && (sel.fl.pt.x != lpord->pt.x || sel.fl.pt.y != lpord->pt.y); lpord++) {
            i++;
        }
        SetScanWp(i);
    }
    rgpt[2] = sel.fl.lpplord->rgord[sel.iwpAct - 1].pt;
    ptPrev = rgpt[2];
    if (sel.iwpAct == sel.fl.cord - 1) {
        cpt = 3;
    } else {
        cpt = 4;
        rgpt[3] = sel.fl.lpplord->rgord[sel.iwpAct + 1].pt;
        ptNext = rgpt[3];
    }
    rgpt[0] = sel.fl.lpplord->rgord[sel.iwpAct].pt;
    rgpt[1] = rgpt[0];
    for (i = 0; i < cpt; i++) {
        LogicalToScan(&rgpt[i]);
    }
    GetClientRect(hwndScanner, &rc);
    rc.bottom -= dySBar;
    hdc = GetDC(hwndScanner);
    hcurSav = SetCursor(hcurCloseGrab);
    SetCapture(hwndScanner);
    ptNew = pt;
    while (FGetMouseMove(&ptNew)) {
        ptNew.x = max(0, min(rc.right, ptNew.x));
        ptNew.y = max(0, min(rc.bottom, ptNew.y));
        if (pt.x != ptNew.x || pt.y != ptNew.y) {
            if (fFirst && FNearAWayPoint(ptNew, FALSE))
                goto DoNext;

            fFirst = FALSE;
            ptLogical = ptNew;
            ScanToLogical(&ptLogical);
            grTypeIn = !(GetAsyncKeyState(VK_SHIFT) & 0xfffe) ? 79 : 143;
            FFindNearestObject(ptLogical, grTypeIn, &scan);
            DrawScanXorLines(hdc, rgpt, cpt);
            if (scan.grobj == grobjNone) {
                rgpt[0] = ptLogical;
                sbar.id = -1;
                CchGetString(idsDeepSpace, szDeepSpace);
                sbar.psz = szDeepSpace;
            } else {
                rgpt[0] = scan.pt;
                switch (scan.grobj) {
                case grobjPlanet:
                    sbar.id = scan.idpl;
                    break;
                case grobjFleet:
                    sbar.id = rglpfl[scan.ifl]->id;
                    break;
                case grobjThing:
                    sbar.id = lpThings[scan.ith].idFull;
                    break;
                default:
                    sbar.id = scan.iwp;
                }
                sbar.psz = 0;
            }
            sbar.pt = rgpt[0];
            sbar.grbit = scan.grobj;
            LogicalToScan(rgpt);
            DrawScanXorLines(hdc, rgpt, cpt);
            sbar.pscan = 0;
            DrawScannerSBar(hdc, NULL, &sbar, FALSE);

        DoNext:
            pt = ptNew;
        }
    }
    fChg = rgpt[0].x != rgpt[1].x || rgpt[0].y != rgpt[1].y;
    if (fChg) {
        if (scan.grobj == grobjNone) {
            scan.pt = ptLogical;
            scan.grobj = grobjOther;
            scan.iwp = sel.iwpAct;
        }
        fDup = scan.pt.x == ptPrev.x && scan.pt.y == ptPrev.y;
        if (!fDup && sel.iwpAct < sel.fl.cord - 1) {
            fDup = (scan.pt.x == sel.fl.lpplord->rgord[sel.iwpAct + 1].pt.x && scan.pt.y == sel.fl.lpplord->rgord[sel.iwpAct + 1].pt.y) * 2;
        }
        GetClientRect(hwndScanner, &rc);
        if (fDup) {
            fDel = AlertSz(PszFormatIds(idsSureWantDeleteCurrentWaypoint, NULL), MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL) == IDYES;
            if (grbitScan & grbitScanFleetPaths) {
                hpenSav = SelectObject(hdc, hpenStarbase);
                MoveToEx(hdc, rgpt[2].x, rgpt[2].y, NULL);
                LineTo(hdc, rgpt[0].x, rgpt[0].y);
                if (cpt > 3) {
                    ExcludeClipRect(hdc, 0, rc.bottom - dySBar, rc.right, rc.bottom);
                    LineTo(hdc, rgpt[3].x, rgpt[3].y);
                }
                SelectObject(hdc, hpenSav);
            }
            DrawScanXorLines(hdc, rgpt, cpt);
            rgpt[0] = rgpt[1];
            if (grbitScan & grbitScanFleetPaths) {
                hpenSav = SelectObject(hdc, hpenStarbase);
                MoveToEx(hdc, rgpt[2].x, rgpt[2].y, NULL);
                LineTo(hdc, rgpt[0].x, rgpt[0].y);
                if (cpt > 3) {
                    ExcludeClipRect(hdc, 0, rc.bottom - dySBar, rc.right, rc.bottom);
                    LineTo(hdc, rgpt[3].x, rgpt[3].y);
                }
                SelectObject(hdc, hpenSav);
            }
            DrawScanXorLines(hdc, rgpt, cpt);
            if (!fDel)
                goto Done;
            DeleteCurWayPoint(fDup == 1);
            goto Done;
        }
        if (grbitScan & grbitScanFleetPaths) {
            ExcludeClipRect(hdc, 0, rc.bottom - dySBar, rc.right, rc.bottom);
            hpenSav = SelectObject(hdc, hpenStarbase);
            MoveToEx(hdc, rgpt[2].x, rgpt[2].y, NULL);
            LineTo(hdc, rgpt[0].x, rgpt[0].y);
            if (cpt > 3) {
                LineTo(hdc, rgpt[3].x, rgpt[3].y);
            }
            SelectObject(hdc, hpenSav);
        }
        DrawScanXorLines(hdc, rgpt, cpt);
        rgpt[0] = rgpt[1];
        if (grbitScan & grbitScanFleetPaths) {
            ExcludeClipRect(hdc, 0, rc.bottom - dySBar, rc.right, rc.bottom);
            hpenSav = SelectObject(hdc, hpenStarbase);
            MoveToEx(hdc, rgpt[2].x, rgpt[2].y, NULL);
            LineTo(hdc, rgpt[0].x, rgpt[0].y);
            if (cpt > 3) {
                LineTo(hdc, rgpt[3].x, rgpt[3].y);
            }
            SelectObject(hdc, hpenSav);
        }
        DrawScanXorLines(hdc, rgpt, cpt);
        RedrawScanSel(NULL, 0);
        switch (scan.grobj) {
        case grobjPlanet:
            i = scan.idpl;
            break;
        case grobjFleet:
            i = rglpfl[scan.ifl]->id;
            break;
        case grobjThing:
            i = lpThings[scan.ith].idFull;
            break;
        default:
            i = scan.iwp;
        }
        lpord = &sel.fl.lpplord->rgord[sel.iwpAct];
        lpord->grobj = scan.grobj;
        lpord->id = i;
        lpord->pt = scan.pt;
        lpord->iWarp = FAltDown() ? IWarpFastestForWaypoint(&sel.fl, lpord) : IWarpBestForWaypoint(&sel.fl, lpord);
        FLookupFleet(idWriteBack, &sel.fl);
        scan.iwp = sel.iwpAct;
        scan.grobjFull |= grobjOther;
        sel.iwpAct = -2;
        ChangeScanSel(&scan, 1);
    }
    DrawScannerSBar(hdc, NULL, NULL, FALSE);
    ReleaseCapture();
    SetCursor(hcurSav);
    InvalidateRect(hwndMine, NULL, TRUE);
    SetMineralTitleBar(hwndMine);
Done:
    ReleaseDC(hwndScanner, hdc);
    if (fChg && (grbitScan & grbitScanFleetPaths)) {
        rgpt[1] = ptNew;
        BoundPoints(&rc, rgpt, cpt);
        hdc = GetDC(hwndScanner);
        DrawScanner(hdc, &rc);
        ReleaseDC(hwndScanner, hdc);
    }
    return fChg;
}

void DrawScanXorLines(HDC hdc, POINT16 *rgpt, int16_t cpt) {
    HPEN    hpenSav;
    int16_t iRopSav;
    int16_t i;
    RECT    rc;

    GetClientRect(hwndScanner, &rc);
    ExcludeClipRect(hdc, 0, rc.bottom - dySBar, rc.right, rc.bottom);
    if (cpt == 4 && rgpt[2].x == rgpt[3].x && rgpt[2].y == rgpt[3].y) {
        cpt--;
        hpenSav = GetStockObject(WHITE_PEN);
    } else {
        hpenSav = hpenShip;
    }
    for (i = 1; i < cpt; i++) {
        ExcludeClipRect(hdc, rgpt[i].x - 5, rgpt[i].y - 5, rgpt[i].x + 6, rgpt[i].y + 6);
    }
    hpenSav = SelectObject(hdc, hpenSav);
    iRopSav = SetROP2(hdc, R2_XORPEN);
    MoveToEx(hdc, rgpt[2].x, rgpt[2].y, NULL);
    LineTo(hdc, rgpt->x, rgpt->y);
    if (cpt > 3) {
        LineTo(hdc, rgpt[3].x, rgpt[3].y);
    }
    SetROP2(hdc, iRopSav);
    SelectObject(hdc, hpenSav);
    SelectClipRgn(hdc, hrgnHuge);
    return;
}

int16_t SetScanWp(int16_t iNew) {
    SCAN scan;

    if (iNew == sel.iwpAct) {
        return iNew;
    }
    FFindNearestObject(sel.fl.lpplord->rgord[iNew].pt, grobjOther, &scan);
    scan.iwp = iNew;
    ChangeScanSel(&scan, 1);
    return iNew;
}

int16_t FGetNextObjHere(SCAN *pscan, int16_t fOnlyOurs) {
    FLEET  *lpfl;
    int16_t i;
    int16_t fFound;

    fFound = sel.grobj != grobjFleet;
    for (i = 0; i < cFleet; i++) {
        lpfl = rglpfl[i];
        if (!rglpfl[i])
            break;
        if (!fFound) {
            if (lpfl->id == sel.id) {
                fFound = TRUE;
            }
        } else if (sel.pt.x == lpfl->pt.x && sel.pt.y == lpfl->pt.y && (!fOnlyOurs || lpfl->iPlayer == idPlayer)) {
            break;
        }
    }
    if (!fFound) {
        return FALSE;
    }
    if (i < cFleet) {
        pscan->ifl = i;
        pscan->grobj = grobjFleet;
    } else if ((sel.grobjFull & grobjPlanet) && sel.pl.iPlayer == idPlayer) {
        pscan->grobj = grobjPlanet;
        pscan->idpl = sel.pl.id;
    } else {
        for (i = 0; i < cFleet; i++) {
            lpfl = rglpfl[i];
            if (!rglpfl[i] || (pscan->pt.x == lpfl->pt.x && pscan->pt.y == lpfl->pt.y && (!fOnlyOurs || lpfl->iPlayer == idPlayer)))
                break;
        }
        if (i >= cFleet || lpfl->id == sel.id) {
            return FALSE;
        }
        pscan->ifl = i;
        pscan->grobj = grobjFleet;
    }
    return TRUE;
}

INT_PTR CALLBACK FindDlg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    char szName[40];
    RECT rc;

    switch (msg) {
    case WM_INITDIALOG:
        StickyDlgPos(hwnd, &ptStickyFindDlg, TRUE);
        SendDlgItemMessage(hwnd, IDC_EDIT1, EM_LIMITTEXT, 0x27, 0);
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
        if (msg == WM_CTLCOLORSTATIC) {
            SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            if (LOWORD(wParam) == IDOK) {
                GetDlgItemText(hwnd, IDC_EDIT1, szName, 40);
                if (!FSelectSz(szName)) {
                    AlertSz(PszFormatIds(idsSorryCantFindPlanetFleetName, NULL), MB_ICONHAND);
                    SetFocus(GetDlgItem(hwnd, IDC_EDIT1));
                    SendDlgItemMessage(hwnd, IDC_EDIT1, EM_SETSEL, 0, -1);
                    return 0;
                }
            }
            StickyDlgPos(hwnd, &ptStickyFindDlg, FALSE);
            EndDialog(hwnd, LOWORD(wParam) == IDOK);
            return 1;
        case IDC_HELP:
            WinHelp(hwnd, szHelpFile, HELP_CONTEXT, idhFindPlanetOrFleet);
            return 1;
        }
    }
    return 0;
}

int16_t FSelectSz(char *szName) {
    char   *pch;
    int16_t ifl;
    FLEET  *lpfl;
    int16_t ipl;
    int16_t cch;
    char    szT[20];
    int16_t iplPartial;
    SCAN    scan;

    iplPartial = -1;
    scan.iwp = iwpNone;
    for (ipl = 0; ipl < game.cPlanMax && strcmpi(PszGetCompressedPlanet(rgidPlan[ipl]), szName) != 0; ipl++) {
        if (iplPartial == -1 && strnicmp(PszGetCompressedPlanet(rgidPlan[ipl]), szName, strlen(szName)) == 0) {
            iplPartial = ipl;
        }
    }
GoWithPartial:
    if (ipl != game.cPlanMax) {
        FFindNearestObject(rgptPlan[ipl], grobjPlanet, &scan);
        ChangeScanSel(&scan, 1);
        FEnsurePointOnScreen(scan.pt, TRUE);
        UpdateWindow(hwndScanner);
        SendMessage(hwndScanner, WM_CHAR, 'v', 0);
        return TRUE;
    }

    cch = CchGetString(idsFleet, szT);
    if (strnicmp(szName, szT, cch) == 0) {
        pch = szName + 6;
    } else {
        pch = szName;
    }
    for (; *pch == ' '; pch++) {
    }
    if (*pch == '#') {
        pch++;
    }
    for (; *pch == ' '; pch++) {
    }
    if (*pch >= '1' && *pch <= '9') {
        ifl = *pch - '0';
        pch++;
        while (isdigit(*pch) != 0) {
            ifl = 10 * ifl + *pch - '0';
            pch++;
            if (ifl > 512)
                goto LNotAFleetId;
        }
        ifl--;
        ifl |= idPlayer << 9;
        lpfl = LpflFromId(ifl);
        if (lpfl) {
        LFoundFleetId:
            FFindNearestObject(lpfl->pt, grobjFleet, &scan);
            scan.ifl = IflFromLpfl(lpfl);
            ChangeScanSel(&scan, 2);
            FEnsurePointOnScreen(scan.pt, TRUE);
            UpdateWindow(hwndScanner);
            SendMessage(hwndScanner, WM_CHAR, 'v', 0);
            return TRUE;
        }
    }
LNotAFleetId:
    for (ifl = 0; ifl < cFleet; ifl++) {
        lpfl = rglpfl[ifl];
        if (!rglpfl[ifl])
            break;
        if (strcmpi(PszGetFleetName(lpfl->id), szName) == 0)
            goto LFoundFleetId;
    }
    if (iplPartial == -1) {
        return FALSE;
    }
    ipl = iplPartial;
    goto GoWithPartial;
}

void GetScanFleetOrientation(FLEET *lpfl, POINT16 *ppt, POINT16 *pptD) {
    int16_t dy;
    int16_t dx;

    if (lpfl->iPlayer == idPlayer) {
        if (lpfl->cord <= 1 || lpfl->lpplord->rgord[1].iWarp == 0) {
        NoInfo:
            dx = dy = 0;
        } else {
            dx = lpfl->lpplord->rgord[1].pt.x - lpfl->pt.x;
            dy = lpfl->lpplord->rgord[1].pt.y - lpfl->pt.y;
        }
    } else {
        if (!lpfl->fdirValid || lpfl->iwarpFlt == 0)
            goto NoInfo;
        dx = lpfl->dirFltX - 127;
        dy = lpfl->dirFltY - 127;
    }
    GetDxDyOrientation(dx, dy, ppt, pptD);
    return;
}

void GetDxDyOrientation(int16_t dx, int16_t dy, POINT16 *ppt, POINT16 *pptD) {
    double  dbl;
    int16_t iBmp;

    iBmp = 0;
    if (dx == 0 && dy == 0)
        goto LFinishUp;

    dbl = Sf64From80((Sf80Add(
        Sf80Div(Sf80Mul((Sf80Add(Sf80From64(Sf64Atan2(Sf64FromI32(dy), Sf64FromI32(dx))), Sf80From64(3.1415927))), Sf80From64(4.0)), Sf80From64(3.141592654)),
        Sf80From64(0.5))));
    iBmp = 8 - (LOWORD(Sf64ToI32(dbl)) & 7);
    iBmp = (8 - (LOWORD(Sf64ToI32(dbl)) & 7) + 1) & 7;

LFinishUp:
    pptD->x = pptD->y = iScanZoom < zoom100 ? 7 : 9;
    if (iScanZoom >= zoom100) {
        ppt->x = 7;
    } else {
        ppt->x = 0;
    }
    ppt->y = iBmp * pptD->y;
    return;
}

int16_t FHandleMeasuringTape(SCAN *pscan, POINT16 pt) {
    HDC     hdc;
    HPEN    hpenSav;
    SBAR    sbar;
    POINT16 ptLogLast;
    int16_t grTypeIn;
    POINT16 ptLogical;
    POINT16 ptBase;
    int16_t iropSav;
    POINT16 ptNew;
    char    szT[20];
    int16_t fVirgin;
    SCAN    scan;
    RECT    rc;

    fVirgin = TRUE;
    ptLogLast = pscan->pt;
    ptBase = pscan->pt;
    LogicalToScan(&ptBase);
    GetClientRect(hwndScanner, &rc);
    rc.bottom -= dySBar;
    hdc = GetDC(hwndScanner);
    SetCapture(hwndScanner);
    sbar.pscan = pscan;
    hpenSav = SelectObject(hdc, hpenShip);
    iropSav = SetROP2(hdc, R2_XORPEN);
    ptNew = pt;
    LogicalToScan(&ptNew);
    while (FGetRMouseMove(&ptNew)) {
        ptNew.x = max(0, min(rc.right, ptNew.x));
        ptNew.y = max(0, min(rc.bottom, ptNew.y));
        ptLogical = ptNew;
        ScanToLogical(&ptLogical);
        grTypeIn = !(GetAsyncKeyState(VK_SHIFT) & 0xfffe) ? 79 : 143;
        if (FFindNearestObject(ptLogical, grTypeIn, &scan)) {
            ptLogical = scan.pt;
        }
        if (ptLogLast.x != ptLogical.x || ptLogLast.y != ptLogical.y) {
            ptNew = ptLogical;
            LogicalToScan(&ptNew);
            if (fVirgin) {
                if (abs(ptLogical.x - ptLogLast.x) < 3 && abs(ptLogical.y - ptLogLast.y) < 3)
                    continue;
                fVirgin = FALSE;
            } else {
                MoveToEx(hdc, ptBase.x, ptBase.y, NULL);
                LineTo(hdc, pt.x, pt.y);
            }
            MoveToEx(hdc, ptBase.x, ptBase.y, NULL);
            LineTo(hdc, ptNew.x, ptNew.y);
            if (scan.grobj == grobjNone) {
                sbar.id = -1;
                CchGetString(idsDeepSpace, szT);
                sbar.psz = szT;
            } else {
                if (scan.grobj == grobjPlanet) {
                    sbar.id = scan.idpl;
                } else if (scan.grobj == grobjFleet) {
                    sbar.id = rglpfl[scan.ifl]->id;
                } else if (scan.grobj == grobjThing) {
                    sbar.id = lpThings[scan.ith].idFull;
                } else {
                    sbar.id = scan.iwp;
                }
                sbar.psz = 0;
            }
            sbar.pt = ptLogical;
            sbar.grbit = scan.grobj;
            DrawScannerSBar(hdc, NULL, &sbar, FALSE);
            pt = ptNew;
            ptLogLast = ptLogical;
        }
    }
    if (!fVirgin) {
        MoveToEx(hdc, ptBase.x, ptBase.y, NULL);
        LineTo(hdc, pt.x, pt.y);
    }
    SetROP2(hdc, iropSav);
    SelectObject(hdc, hpenSav);
    DrawScannerSBar(hdc, NULL, NULL, FALSE);
    ReleaseCapture();
    ReleaseDC(hwndScanner, hdc);
    if (!fVirgin) {
        return TRUE;
    }
    return FALSE;
}

// ShowScanSel draws (fVis 1) or erases (0, or -1 to leave the main
// selection's marker) the scanner's selection markers.
void ShowScanSel(int16_t fVis) {
    RedrawScanSel(NULL, fVis);
    return;
}

// ShowScanSelChange redraws the scanner, orders and mineral views after
// ChangeScanSel moved the scanner selection from *pscanOld to *pscan.
void ShowScanSelChange(SCAN *pscanOld, SCAN *pscan, int16_t fChgWp) {
    int16_t fMineFieldSel;
    RECT    rcMine;
    int16_t iRad;
    HDC     hdc;
    POINT16 ptTL; /* NATIVE: RECT corners are 32-bit; LogicalToScan takes POINT16 */
    POINT16 ptBR;

    fMineFieldSel = pscanOld->grobj == grobjThing && lpThings[pscanOld->ith].ith == ithMinefield;
    if (fMineFieldSel) {
        iRad = LOWORD(Sf80ToI32((Sf80Add(Sf80From64(Sf64Sqrt(Sf64FromI32(lpThings[pscanOld->ith].thm.cMines))), Sf80From64(1.0)))));
        rcMine.left = lpThings[pscanOld->ith].pt.x;
        rcMine.top = lpThings[pscanOld->ith].pt.y;
        rcMine.right = rcMine.left + iRad;
        rcMine.bottom = rcMine.top - iRad;
        rcMine.left -= iRad;
        rcMine.top += iRad;
        /* NATIVE: original passed (POINT16 *)&rcMine.left and &rcMine.right */
        ptTL.x = rcMine.left;
        ptTL.y = rcMine.top;
        LogicalToScan(&ptTL);
        rcMine.left = ptTL.x;
        rcMine.top = ptTL.y;
        ptBR.x = rcMine.right;
        ptBR.y = rcMine.bottom;
        LogicalToScan(&ptBR);
        rcMine.right = ptBR.x;
        rcMine.bottom = ptBR.y;
        InflateRect(&rcMine, 1, 1);
    }
    if (fChgWp) {
        FillOrdersLB();
        SetOrdersLbSel(pscan->iwp);
        UpdateOrdersDDs(0);
        DrawPlanShip(NULL, 0x122);
    }
    RedrawScanSel(NULL, 1);
    if (fChgWp) {
        FEnsurePointOnScreen(pscan->pt, TRUE);
    }
    DrawScannerSBar(NULL, NULL, NULL, FALSE);
    InvalidateRect(hwndMine, NULL, TRUE);
    SetMineralTitleBar(hwndMine);
    if (fMineFieldSel) {
        hdc = GetDC(hwndScanner);
        DrawScanner(hdc, &rcMine);
        ReleaseDC(hwndScanner, hdc);
    }
    fMineFieldSel = sel.scan.grobj == grobjThing && lpThings[sel.scan.ith].ith == ithMinefield;
    if (fMineFieldSel) {
        iRad = LOWORD(Sf80ToI32((Sf80Add(Sf80From64(Sf64Sqrt(Sf64FromI32(lpThings[sel.scan.ith].thm.cMines))), Sf80From64(1.0)))));
        rcMine.left = lpThings[sel.scan.ith].pt.x;
        rcMine.top = lpThings[sel.scan.ith].pt.y;
        rcMine.right = rcMine.left + iRad;
        rcMine.bottom = rcMine.top - iRad;
        rcMine.left -= iRad;
        rcMine.top += iRad;
        /* NATIVE: original passed (POINT16 *)&rcMine.left and &rcMine.right */
        ptTL.x = rcMine.left;
        ptTL.y = rcMine.top;
        LogicalToScan(&ptTL);
        rcMine.left = ptTL.x;
        rcMine.top = ptTL.y;
        ptBR.x = rcMine.right;
        ptBR.y = rcMine.bottom;
        LogicalToScan(&ptBR);
        rcMine.right = ptBR.x;
        rcMine.bottom = ptBR.y;
        InflateRect(&rcMine, 1, 1);
    }
    if (fMineFieldSel) {
        hdc = GetDC(hwndScanner);
        DrawScanner(hdc, &rcMine);
        ReleaseDC(hwndScanner, hdc);
    }
    if (sel.pl.id != idplNone) {
        DrawPlanShip(NULL, 0x4002);
    }
    if (gd.fTutorial && idPlayer == 0) {
        AdvanceTutor();
    }
    return;
}

// ShowSelAt scrolls the scanner to a newly selected object at pt and
// updates the scanner status bar and the mineral window.
void ShowSelAt(POINT16 pt) {
    CtrPointScan(pt, TRUE);
    DrawScannerSBar(NULL, NULL, NULL, FALSE);
    InvalidateRect(hwndMine, NULL, TRUE);
    SetMineralTitleBar(hwndMine);
    return;
}
