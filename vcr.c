#include "win.h"

void BattleVCR(int16_t iBattle) {
    FARPROC  lpProc;
    jmp_buf *penvMemSav;
    jmp_buf  env;
    HB      *lphb;

    lpProc = NULL;
    penvMemSav = penvMem;
    viStepVCRCur = -1;
    gd.fVCRTimer = FALSE;
    if (gd.mdScreenSize >= 2) {
        dxyVCRBoard = 673;
        dxyVCRSquare = 64;
    } else {
        dxyVCRBoard = 353;
        dxyVCRSquare = 32;
    }
    lphb = rglphb[11];
    vlpbdVCR = (BTLDATA *)((uint8_t *)lphb + (sizeof(HB) + 2));
    while (vlpbdVCR->id != iBattle) {
        if (vlpbdVCR->id == 0xffff) {
            lphb = lphb->lphbNext;
            if (!lphb) {
                return;
            }
            vlpbdVCR = (BTLDATA *)((uint8_t *)lphb + (sizeof(HB) + 2));
        } else {
            vlpbdVCR = (BTLDATA *)((uint8_t *)vlpbdVCR + vlpbdVCR->cbData);
        }
    }
    vlpbdVCRNext = (BTLDATA *)((uint8_t *)vlpbdVCR + vlpbdVCR->cbData);
    penvMem = &env;
    if (setjmp(env) != 0) {
        penvMem = penvMemSav;
        AlertSz(PszFormatIds(idsMemory, NULL), MB_ICONHAND);
        goto LCleanup;
    }
    vrgtok = LpAlloc(vlpbdVCR->ctok * sizeof(TOK), htMisc);
    vrgdpVCR = LpAlloc(vlpbdVCR->ctok * 4, htMisc);
    vcStepVCR = SetVCRBoard(30000) - 1;
    vcRound = viRound;
    SetVCRBoard(-1);
    if (gd.fTutorial) {
        AdvanceTutor();
    }
    lpProc = MakeProcInstance(VCRDlg, hInst);
    DialogBox(hInst, MAKEINTRESOURCE(IDD_VCR), hwndFrame, lpProc);

LCleanup:
    if (lpProc) {
        FreeProcInstance(lpProc);
    }
    if (vrgtok) {
        FreeLp(vrgtok, htMisc);
    }
    if (vrgdpVCR) {
        FreeLp(vrgdpVCR, htMisc);
    }
    vrgtok = NULL;
    vrgdpVCR = NULL;
    return;
}

BTLDATA *BtlDataGet(int16_t i) {
    BTLDATA *lpbd;
    HB      *lphb;

    lphb = rglphb[11];
    if (!lphb) {
        return NULL;
    }
    lpbd = (BTLDATA *)((uint8_t *)lphb + (sizeof(HB) + 2));
    while (1) {
        if (lpbd->id == 0xffff) {
            lphb = lphb->lphbNext;
            if (!lphb || lphb->ibTop <= sizeof(HB))
                break;
            lpbd = (BTLDATA *)((uint8_t *)lphb + (sizeof(HB) + 2));
        } else {
            if (lpbd->cbData == 0) {
                return NULL;
            }
            if (i-- <= 0) {
                return lpbd;
            }
            lpbd = (BTLDATA *)((uint8_t *)lpbd + lpbd->cbData);
        }
    }
    return NULL;
}

int32_t CBattleUnits(BTLDATA *lpbd, BattleUnitFlags grbitBU) {
    TOK         *lptok;
    int16_t      ctok;
    int32_t      lUnits;
    int16_t      i;
    HullCategory imd;

    ctok = lpbd->ctok;
    lUnits = 0;
    for (i = 0; i < ctok; i++) {
        lptok = &lpbd->rgtok[i];
        if (lptok->iplr == idPlayer) {
            if (!(grbitBU & grBuOurUnits))
                continue;
        } else if (!(grbitBU & grBuTheirUnits)) {
            continue;
        }
        if (!(grbitBU & grBuIncludeSb) && lptok->ishdef >= 16)
            continue;
        if ((grbitBU & grBuClassAll) != 0xf8 && lptok->ishdef < 16) {
            imd = LphuldefFromId(rglpshdef[lptok->iplr][lptok->ishdef].hul.ihuldef)->imdCategory;
            if ((int16_t)imd <= hullCatFreighter || (int16_t)imd >= hullCatMiner) {
                if (!(grbitBU & grBuClassUnarmed))
                    continue;
            } else if (imd == hullCatScout) {
                if (!(grbitBU & grBuClassScout))
                    continue;
            } else if (imd == hullCatWarship) {
                if (!(grbitBU & grBuClassWarship))
                    continue;
            } else if (imd == hullCatBomber) {
                if (!(grbitBU & grBuClassBomber))
                    continue;
            } else if (imd == hullCatUtility) {
                if (!(grbitBU & grBuClassUtility))
                    continue;
            }
        }
        lUnits += (uint32_t)lptok->csh;
    }
    return lUnits;
}

int32_t CBattleKills(BTLDATA *lpbd, int16_t fOurDead) {
    int32_t  cKilled;
    BTLDATA *lpbdNext;
    int16_t  i;
    BTLREC  *lpbr;
    int16_t  cKill;

    lpbr = (BTLREC *)&lpbd->rgtok[lpbd->ctok];
    lpbdNext = (BTLDATA *)((uint8_t *)lpbd + lpbd->cbData);
    cKilled = 0;
    for (; lpbr < (BTLREC *)lpbdNext; lpbr = (BTLREC *)&lpbr->rgkill[lpbr->ctok]) {
        cKill = lpbr->ctok;
        for (i = 0; i < cKill; i++) {
            if (lpbr->rgkill[i].cshKill > 0) {
                if (lpbd->rgtok[lpbr->rgkill[i].itok].iplr == idPlayer) {
                    if (fOurDead) {
                        cKilled += (uint32_t)lpbr->rgkill[i].cshKill;
                    }
                } else if (!fOurDead) {
                    cKilled += (uint32_t)lpbr->rgkill[i].cshKill;
                }
            }
        }
    }
    return cKilled;
}

int32_t LdpFromItokDv(int16_t itok, DV *lpdv) {
    DV       dv;
    uint16_t dpShdef;
    int16_t  csh;
    int32_t  dp;

    if (lpdv) {
        dv.dp = lpdv->dp;
    } else {
        dv.dp = 0;
    }
    dpShdef = LpshdefFromTok(vrgtok + itok)->hul.dp;
    dp = (uint32_t)((uint32_t)dpShdef * (uint32_t)vrgtok[itok].csh);
    if (dv.dp != 0) {
        csh = LOWORD((int32_t)((uint32_t)vrgtok[itok].csh * dv.pctSh) / 100);
        if (csh <= 0) {
            csh = 1;
        }
        dp -= (int32_t)((int32_t)((uint32_t)dpShdef * dv.pctDp) / 10 * csh) / 50;
    }
    return dp;
}

int16_t SetVCRBoard(int16_t iStep) {
    TOK    *ptok;
    int16_t i;
    int16_t itok;

    if (iStep == viStepVCRCur && viStepVCRCur != -1) {
        return iStep;
    }
    if (viStepVCRCur > iStep || viStepVCRCur == -1) {
        for (i = 0; i < vlpbdVCR->ctok; i++) {
            vrgtok[i] = vlpbdVCR->rgtok[i];
            vrgdpVCR[i] = LdpFromItokDv(i, &vrgtok[i].dv);
        }
        viStepVCRCur = -1;
        viRound = 0;
        viVCRFocus = 0;
        vbrcVCRFocus = vrgtok->brc;
        vlpbrVCR = (BTLREC *)&vlpbdVCR->rgtok[vlpbdVCR->ctok];
    }
    for (; viStepVCRCur < iStep && vlpbrVCR < (BTLREC *)vlpbdVCRNext; viStepVCRCur++) {
        if (viStepVCRCur >= 0) {
            for (i = 0; i < vlpbrVCR->ctok; i++) {
                itok = vlpbrVCR->rgkill[i].itok;
                ptok = vrgtok + itok;
                ptok->csh -= vlpbrVCR->rgkill[i].cshKill;
                ptok->dv.dp = vlpbrVCR->rgkill[i].dv.dp;
                vrgdpVCR[itok] = LdpFromItokDv(itok, &vlpbrVCR->rgkill[i].dv);
                if (ptok->dpShield != 0 && vlpbrVCR->rgkill[i].dpShield != 0) {
                    if ((int32_t)(uint32_t)((uint32_t)ptok->dpShield * (uint32_t)ptok->csh) >
                        (int32_t)((uint32_t)(vlpbrVCR->rgkill[i].dpShield & 0x1fff) << (vlpbrVCR->rgkill[i].dpShield >> 0xd << 1))) {
                        ptok->dpShield -= LOWORD((int32_t)((uint32_t)(vlpbrVCR->rgkill[i].dpShield & 0x1fff) << (vlpbrVCR->rgkill[i].dpShield >> 0xd << 1)) /
                                                 (int32_t)ptok->csh);
                    } else {
                        ptok->dpShield = 0;
                    }
                }
            }
            vlpbrVCR = (BTLREC *)&vlpbrVCR->rgkill[vlpbrVCR->ctok];
            if (vlpbrVCR->iRound > (uint16_t)viRound) {
                viRound = vlpbrVCR->iRound;
                ptok = vrgtok;
                i = 0;
                while (i < vlpbdVCR->ctok) {
                    if (ptok->fRegen) {
                        RegenShield(ptok);
                    }
                    ptok->fMoved = FALSE;
                    i++;
                    ptok++;
                }
            }
        }
        if (vlpbrVCR < (BTLREC *)vlpbdVCRNext) {
            vrgtok[vlpbrVCR->itok].brc = vlpbrVCR->brcDest;
            vbrcVCRFocus = vrgtok[vlpbrVCR->itok].brc;
            viVCRFocus = vlpbrVCR->itok;
            vrgtok[vlpbrVCR->itok].dzDis = vlpbrVCR->dzDis;
            if (vrgtok[vlpbrVCR->itok].dzDis == 4) {
                vrgtok[vlpbrVCR->itok].mdTactic = mdTacticDisengage;
            }
        }
    }
    EnableVCRButtons();
    return viStepVCRCur;
}

INT_PTR CALLBACK VCRDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    HDC         hdc;
    int16_t     i = 0; // The close command has no battle-replay result.
    int16_t     ibtn;
    RECT        rc;
    int16_t     dyFrame;
    RECT        rcWindow;
    int16_t     dx;
    POINT16     pt;
    uint8_t     brc;
    int16_t     bkMode;
    int16_t     bt;
    RECT       *prc;
    int16_t     iDir;
    int16_t     iCur;
    COLORREF    crBkSav;
    BTNT        btnt;
    int16_t     iSel;
    PAINTSTRUCT ps;
    int16_t     iStep;
    int16_t     dStep;

    switch (message) {
    case WM_INITDIALOG:
        hwndVCRDlg = hwnd;
        GetWindowRect(hwnd, &rcWindow);
        GetClientRect(hwnd, &rc);
        dyFrame = rcWindow.bottom - rcWindow.top - rc.bottom;
        GetWindowRect(GetDlgItem(hwnd, IDC_VCR_REW_ALL), &rc);
        SetWindowPos(hwnd, NULL, 0, 0, dxyVCRBoard + 250, dyFrame + 24 + dxyVCRBoard + (rc.bottom - rc.top), SWP_NOMOVE | SWP_NOZORDER);
        for (i = 0; i < 7; i++) {
            if (i < 5) {
                ibtn = i + 161;
            } else if (i == 5) {
                ibtn = 1;
            } else if (i == 6) {
                ibtn = 118;
            }
            GetWindowRect(GetDlgItem(hwnd, ibtn), &rc);
            MapWindowPoints(NULL, hwnd, (POINT *)&rc, 2);
            if (dxyVCRSquare >= 64) {
                dx = rc.right - rc.left;
                dx = dxyVCRBoard / 2 + 8 - (int16_t)(7 * dx) / 2 - 24 + (dx + 8) * i;
                dx -= rc.left;
            } else {
                dx = 0;
            }
            OffsetRect(&rc, dx, dxyVCRBoard + 16 - rc.top);
            SetWindowPos(GetDlgItem(hwnd, ibtn), NULL, rc.left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        EnableVCRButtons();
        StickyDlgPos(hwnd, &ptStickyVCRDlg, TRUE);
        fAnimate = TRUE;
        return 1;
    case WM_ERASEBKGND:
        GetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case WM_SETCURSOR:
        GetCursorPos16(&pt);
        ScreenToClient16(hwnd, &pt);
        if (pt.x > 8 && pt.x < (dxyVCRSquare + 3) * 10 + 8 && pt.y >= 8 && pt.y < (dxyVCRSquare + 3) * 10 + 8) {
            SetCursor(hcurHand);
            return 1;
        }
        return 0;
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        if (PtInRect(rgrcBuildSpin, PointFrom16(pt)) != 0 || PtInRect(&rgrcBuildSpin[1], PointFrom16(pt)) != 0) {
            if (PtInRect(rgrcBuildSpin, PointFrom16(pt)) != 0) {
                iDir = -1;
                bt = 34;
                prc = rgrcBuildSpin;
            } else {
                iDir = 1;
                bt = 35;
                prc = &rgrcBuildSpin[1];
            }
            iCur = viSpeedVCR;
            if (iCur > 4) {
                iCur = 4;
            } else if (iCur < 0) {
                iCur = 0;
            }
            hdc = GetDC(hwnd);
            bkMode = SetBkMode(hdc, OPAQUE);
            crBkSav = SetBkColor(hdc, crButtonFace);
            SelectObject(hdc, rghfontArial8[1]);
            InitBtnTrack(&btnt, hwnd, NULL, prc, bt, 80, FALSE, FALSE, NULL);
            while (FTrackBtn(&btnt)) {
                if ((iDir == -1 && iCur > 0) || (iDir == 1 && iCur < 4)) {
                    iCur += iDir;
                    bt = wsprintf(szWork, PszGetCompressedString(idsPlaybackSpeedD), iCur + 1);
                    TextOut(hdc, ptSpeedVCR.x, ptSpeedVCR.y, szWork, bt);
                }
            }
            viSpeedVCR = iCur;
            SelectObject(hdc, rghfontArial8[0]);
            SetBkColor(hdc, crBkSav);
            ReleaseDC(hwnd, hdc);
            if (gd.fVCRTimer) {
                KillTimer(hwnd, 2668);
                gd.fVCRTimer = SetTimer(hwnd, 2668, 570 - 120 * viSpeedVCR, NULL) != 0;
            }
            return 1;
        }
        pt.x = (int16_t)(pt.x - 8) / (dxyVCRSquare + 3);
        pt.y = (int16_t)(pt.y - 8) / (dxyVCRSquare + 3);
        if (pt.x >= 10 && pt.y >= 2 && pt.y < 10 && viVCRFocus >= 0) {
            GlobalPD.grPopup = grPopupShdef;
            if (vrgtok[viVCRFocus].grobj == grobjPlanet) {
                GlobalPD.lpshdef = rglpshdefSB[vrgtok[viVCRFocus].iplr] + (vrgtok[viVCRFocus].ishdef - 16);
            } else {
                GlobalPD.lpshdef = rglpshdef[vrgtok[viVCRFocus].iplr] + vrgtok[viVCRFocus].ishdef;
            }
            GlobalPD.fShowDamage = TRUE;
            GlobalPD.fToken = TRUE;
            GlobalPD.fHideCounts = vrgtok[viVCRFocus].iplr != idPlayer;
            Popup(hwnd, LOWORD(lParam), HIWORD(lParam));
            return 0;
        }
        if (pt.x < 0 || pt.y < 0 || pt.x >= 10 || pt.y >= 10) {
            return 0;
        }
        brc = (pt.y & 0xf) << 4 | (pt.x & 0xf);
        if (message == WM_RBUTTONDOWN) {
            iSel = PopupVCRMenu(hwnd, LOWORD(lParam), HIWORD(lParam), brc);
            if (iSel < 0) {
                return 0;
            }
            vbrcVCRFocus = brc;
            viVCRFocus = iSel;
        } else {
            if (brc != vbrcVCRFocus || viVCRFocus == -1) {
                vbrcVCRFocus = brc;
                viVCRFocus = vlpbdVCR->ctok - 1;
            }
            i = viVCRFocus;
            do {
                i++;
                if (i == vlpbdVCR->ctok) {
                    i = 0;
                }
                if (vrgtok[i].brc == brc && vrgtok[i].csh > 0) {
                    viVCRFocus = i;
                    goto GoodSel;
                }
            } while (i != viVCRFocus);
            viVCRFocus = -1;
        }
    GoodSel:
        DrawVCR(NULL, -2, -1);
        return 0;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        DrawVCR(hdc, -1, -1);
        EndPaint(hwnd, &ps);
        return 1;
    case WM_TIMER:
        if (!gd.fVCRTimer) {
            return 0;
        }
        if (viStepVCRCur != vcStepVCR)
            goto NextBtn;
        i = 2;
        goto KillTime;
    case WM_COMMAND:
        if (LOWORD(wParam) >= IDC_VCR_REW_ALL && LOWORD(wParam) <= IDC_VCR_FWD_ALL) {
            i = LOWORD(wParam) - 161;
            if (gd.fVCRTimer) {
            KillTime:
                KillTimer(hwnd, 2668);
                gd.fVCRTimer = FALSE;
                if (i == 2) {
                    return 0;
                }
            }
            if (GetAsyncKeyState(VK_CONTROL) < 0) {
                dStep = 100;
            } else if (GetAsyncKeyState(VK_SHIFT) < 0) {
                dStep = 10;
            } else {
                dStep = 1;
            }
            fAnimate = FALSE;
            switch (i) {
            case 0:
                iStep = -1;
                break;
            case 1:
                iStep = viStepVCRCur - dStep;
                if (iStep >= -1)
                    break;
                iStep = -1;
                break;
            case 2:
                gd.fVCRTimer = SetTimer(hwnd, 2668, 570 - 120 * viSpeedVCR, NULL) != 0;
                /* fallthrough */
            case 3:
            NextBtn:
                if (i == 3) {
                    iStep = viStepVCRCur + dStep;
                } else {
                    iStep = viStepVCRCur + 1;
                }
                if (iStep > vcStepVCR) {
                    iStep = vcStepVCR;
                }
                fAnimate = viSpeedVCR < 4;
                break;
            case 4:
                iStep = vcStepVCR;
                break;
            }
            SetVCRBoard(iStep);
            DrawVCR(NULL, -2, -1);
        } else if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            if (gd.fVCRTimer) {
                gd.fVCRTimer = FALSE;
                KillTimer(hwnd, 2668);
            }
            if (gd.fTutorial) {
                tutor.fProgress = TRUE;
            }
            StickyDlgPos(hwnd, &ptStickyVCRDlg, FALSE);
            EndDialog(hwnd, i);
            return 1;
        } else if (LOWORD(wParam) == IDC_HELP) {
            WinHelp(hwnd, szHelpFile, HELP_CONTEXT, idhBattleVCR);
            return 1;
        }
        break;
    case WM_DESTROY:
        hwndVCRDlg = 0;
        break;
    }
    return 0;
}

void GetVCRStats(int16_t itok, int32_t *pdpArmor, DV *pdv, int32_t *pdpShields, int16_t *pcsh) {
    int16_t  cshT;
    DV       dv;
    int32_t  dpShields;
    int32_t  dpArmor;
    int16_t  i;
    int16_t  cshKill;
    uint16_t dpShdef;

    dpArmor = 0;
    dpShields = 0;
    cshKill = 0;
    dv.dp = 0xffff;
    dpArmor = vrgdpVCR[itok];
    for (i = 0; i < vlpbrVCR->ctok; i++) {
        if (vlpbrVCR->rgkill[i].itok == itok) {
            cshKill += vlpbrVCR->rgkill[i].cshKill;
            dpShields += (int32_t)((uint32_t)(vlpbrVCR->rgkill[i].dpShield & 0x1fff) << (vlpbrVCR->rgkill[i].dpShield >> 0xd << 1));
            dv.dp = vlpbrVCR->rgkill[i].dv.dp;
        }
    }
    if (dv.dp != 0xffff) {
        dpShdef = LpshdefFromTok(vrgtok + itok)->hul.dp;
        cshT = vrgtok[itok].csh - cshKill;
        dpArmor = (uint32_t)((uint32_t)dpShdef * cshT);
        cshT = LOWORD((int32_t)(cshT * dv.pctSh) / 100);
        if (cshT <= 0) {
            cshT = 1;
        }
        dpArmor -= (int32_t)((int32_t)((uint32_t)dpShdef * dv.pctDp) / 10 * cshT) / 50;
    } else {
        dv.dp = vrgtok[itok].dv.dp;
        if (dv.pctDp > 499) {
            dv.pctDp = 499;
        }
    }
    cshT = vrgtok[itok].csh - cshKill;
    if (cshT < 1) {
        cshT = 0;
        dpArmor = 0;
        dpShields = (uint32_t)((uint32_t)vrgtok[itok].dpShield * (uint32_t)vrgtok[itok].csh);
    }
    if (pdv) {
        pdv->dp = dv.dp;
    }
    if (pcsh) {
        *pcsh = cshT;
    }
    if (pdpArmor) {
        *pdpArmor = dpArmor;
    }
    if (pdpShields) {
        *pdpShields = dpShields;
    }
    return;
}

void DrawVCR(HDC hdc, int16_t iStart, int16_t iEnd) {
    int16_t ctok;
    int16_t ibmpRace;
    int16_t bkMode;
    HBRUSH  hbrSav;
    int32_t dpShields;
    int16_t itokT;
    int32_t dpT;
    int16_t fCreatedDC;
    int32_t dpArmor;
    int16_t y;
    uint8_t rgfSeen[256];
    int16_t c;
    int16_t i;
    uint8_t brcT;
    SHDEF  *lpshdef;
    int16_t ibmp;
    int16_t csh;
    char   *psz;
    int16_t dx;
    int16_t j;
    char    szT[96];
    int16_t fJam;
    RECT    rc;
    int16_t x;
    int16_t cshT;
    int32_t dpShT;
    DV      dv;
    int16_t xT;
    int16_t cshNew;

    fCreatedDC = hdc == NULL;
    if (fCreatedDC) {
        hdc = GetDC(hwndVCRDlg);
    }
    hbrSav = SelectObject(hdc, hbrButtonFace);
    bkMode = SetBkMode(hdc, TRANSPARENT);
    memset(rgfSeen, 0, 0x100);
    GetClientRect(hwndVCRDlg, &rc);
    if (iStart == -2) {
        PatBlt(hdc, dxyVCRBoard + 10, 0, rc.right - dxyVCRBoard - 10, dxyVCRBoard + 8, PATCOPY);
        iStart = -1;
    }
    SelectObject(hdc, hbrButtonShadow);
    if (iStart == -1) {
        PatBlt(hdc, 8, 8, dxyVCRBoard, 2, PATCOPY);
        PatBlt(hdc, 8, 8, 2, dxyVCRBoard, PATCOPY);
        SelectObject(hdc, hbrButtonHilite);
        PatBlt(hdc, 9, dxyVCRBoard + 6, dxyVCRBoard - 1, 1, PATCOPY);
        PatBlt(hdc, 8, dxyVCRBoard + 7, dxyVCRBoard, 1, PATCOPY);
        PatBlt(hdc, dxyVCRBoard + 6, 9, 1, dxyVCRBoard - 3, PATCOPY);
        PatBlt(hdc, dxyVCRBoard + 7, 8, 1, dxyVCRBoard - 2, PATCOPY);
        SelectObject(hdc, hbrButtonShadow);
        for (i = 1; i < 10; i++) {
            PatBlt(hdc, (dxyVCRSquare + 3) * i + 9, 10, 1, dxyVCRBoard - 4, PATCOPY);
            PatBlt(hdc, 10, (dxyVCRSquare + 3) * i + 9, dxyVCRBoard - 4, 1, PATCOPY);
        }
        x = dxyVCRBoard + 14;
        y = 8;
        SelectObject(hdc, rghfontArial8[1]);
        c = wsprintf(szWork, PszGetCompressedString(idsPhaseDDRoundDD), viStepVCRCur + 2, vcStepVCR + 2, viRound + 1, vcRound + 1);
        TextOut(hdc, x, y, szWork, c);
        y += dyArial8 + 4;
        c = wsprintf(szWork, PszGetCompressedString(idsPlaybackSpeedD), viSpeedVCR + 1);
        dx = LOWORD(GetTextExtent(hdc, szWork, c));
        TextOut(hdc, x, y, szWork, c);
        ptSpeedVCR.x = x;
        ptSpeedVCR.y = y;
        SetRect(rgrcBuildSpin, x + dx, y, x + dx + 14, y + 14);
        rgrcBuildSpin[1] = rgrcBuildSpin[0];
        OffsetRect(&rgrcBuildSpin[1], 14, 0);
        for (i = 0; i < 2; i++) {
            DrawBtn(hdc, &rgrcBuildSpin[i], (i == 0 ? 2 : 3) | 0x20, FALSE, NULL);
        }
        if (viStepVCRCur >= 0) {
            y += dyArial8 + 4;
            psz = PszPlayerName(vrgtok[vlpbrVCR->itok].iplr, TRUE, TRUE, TRUE, 0, NULL);
            TextOut(hdc, x, y, szWork, strlen(psz));
            y += dyArial8;
            if (vlpbrVCR->itok == viVCRFocus) {
                SetTextColor(hdc, 8323072);
            }
            if (vrgtok[vlpbrVCR->itok].grobj == grobjPlanet) {
                lpshdef = rglpshdefSB[vrgtok[vlpbrVCR->itok].iplr] + (vrgtok[vlpbrVCR->itok].ishdef - 16);
            } else {
                lpshdef = rglpshdef[vrgtok[vlpbrVCR->itok].iplr] + vrgtok[vlpbrVCR->itok].ishdef;
            }
            csh = vrgtok[vlpbrVCR->itok].csh;
            if (csh > 1) {
                c = wsprintf(szWork, PszGetCompressedString(idsSD), lpshdef->hul.szClass, csh);
            } else {
                strcpy(szWork, lpshdef->hul.szClass);
                c = strlen(szWork);
            }
            TextOut(hdc, x, y, szWork, c);
            y += dyArial8;
            SetTextColor(hdc, crButtonText);
            fJam = 0;
            if (vlpbrVCR->ctok > 0) {
                psz = PszPlayerName(vrgtok[vlpbrVCR->itokAttack].iplr, FALSE, TRUE, TRUE, 0, NULL);
                c = wsprintf(szT, PszGetCompressedString(idsAttacksS), psz);
                TextOut(hdc, x, y, szT, c);
                y += dyArial8;
                if (vlpbrVCR->rgkill[0].dv.dp != 0) {
                    SetTextColor(hdc, 127);
                }
                if (vrgtok[vlpbrVCR->itokAttack].grobj == grobjPlanet) {
                    lpshdef = rglpshdefSB[vrgtok[vlpbrVCR->itokAttack].iplr] + (vrgtok[vlpbrVCR->itokAttack].ishdef - 16);
                } else {
                    lpshdef = rglpshdef[vrgtok[vlpbrVCR->itokAttack].iplr] + vrgtok[vlpbrVCR->itokAttack].ishdef;
                }
                csh = vrgtok[vlpbrVCR->itokAttack].csh;
                if (csh > 1) {
                    c = wsprintf(szWork, PszGetCompressedString(idsSD), lpshdef->hul.szClass, csh);
                } else {
                    strcpy(szWork, lpshdef->hul.szClass);
                    c = strlen(szWork);
                }
                TextOut(hdc, x, y, szWork, c);
                y += dyArial8;
                SetTextColor(hdc, crButtonText);
                brcT = vrgtok[vlpbrVCR->itokAttack].brc;
                dpArmor = 0;
                dpShields = 0;
                j = 0;
                for (i = vlpbrVCR->ctok; i > 0; i--) {
                    itokT = vlpbrVCR->rgkill[i - 1].itok;
                    if (itokT == vlpbrVCR->itokAttack && !fJam) {
                        fJam |= vlpbrVCR->rgkill[i - 1].grfWeapon & (bitFNoHit | bitFDeflected);
                    }
                    j += vlpbrVCR->rgkill[i - 1].cshKill;
                    if (!rgfSeen[itokT]) {
                        rgfSeen[itokT] = TRUE;
                        GetVCRStats(itokT, &dpT, NULL, &dpShT, &cshT);
                        dpArmor += vrgdpVCR[itokT] - dpT;
                        dpShields += dpShT;
                    }
                }
                c = wsprintf(szWork, PszGetCompressedString(idsDDDoing), brcT & 0xf, brcT >> 4);
                TextOut(hdc, x, y, szWork, c);
                y += dyArial8;
                if (dpShields != 0) {
                    CchGetString(idsAnd, szT);
                    c = wsprintf(szWork, PszGetCompressedString(idsLdDamageShieldsS), dpShields, dpArmor > 0 ? szT : j > 0 ? "," : ".");
                    TextOut(hdc, x, y, szWork, c);
                    y += dyArial8;
                }
                if (dpArmor != 0) {
                    c = wsprintf(szWork, PszGetCompressedString(idsLdDamageArmorC), dpArmor, j <= 0 ? 46 : 44);
                    TextOut(hdc, x, y, szWork, c);
                    y += dyArial8;
                }
                if ((fJam & 0x40) && dpShields == 0 && dpArmor == 0) {
                    psz = PszGetCompressedString(idsDamage3);
                    TextOut(hdc, x, y, psz, strlen(psz));
                    y += dyArial8;
                }
                if (j > 0) {
                    c = wsprintf(szWork, PszGetCompressedString(idsDestroyingDShip), j);
                    if (j == 1) {
                        strcpy(&szWork[c], ".");
                        c++;
                    } else {
                        strcpy(&szWork[c], "s.");
                        c += 2;
                    }
                    TextOut(hdc, x, y, szWork, c);
                    y += dyArial8;
                }
            }
            if (fJam) {
                SetTextColor(hdc, 127);
                c = CchGetString((fJam & 0x40) && dpArmor == 0 ? idsTorpedoesDeflected2 : idsTorpedoesDeflected, szWork);
                TextOut(hdc, x, y, szWork, c);
                y += dyArial8;
                SetTextColor(hdc, crButtonText);
            }
        }
        if (vbrcVCRFocus != 0xff) {
            y = 200;
            c = wsprintf(szWork, PszGetCompressedString(idsSelectionDD), vbrcVCRFocus & 0xf, vbrcVCRFocus >> 4);
            TextOut(hdc, x, y, szWork, c);
            y += dyArial8;
            if (viVCRFocus >= 0) {
                csh = 0;
                psz = PszPlayerName(vrgtok[viVCRFocus].iplr, TRUE, TRUE, TRUE, 0, NULL);
                c = strlen(psz);
                TextOut(hdc, x, y, szWork, c);
                y += dyArial8;
                if (vrgtok[viVCRFocus].grobj == grobjPlanet) {
                    lpshdef = rglpshdefSB[vrgtok[viVCRFocus].iplr] + (vrgtok[viVCRFocus].ishdef - 16);
                } else {
                    lpshdef = rglpshdef[vrgtok[viVCRFocus].iplr] + vrgtok[viVCRFocus].ishdef;
                }
                csh = vrgtok[viVCRFocus].csh;
                GetVCRStats(viVCRFocus, &dpT, &dv, &dpShields, &cshT);
                cshNew = cshT;
                cshT = csh - cshT;
                if (csh > 1 || cshT != 0) {
                    c = wsprintf(szWork, PszGetCompressedString(idsSD), lpshdef->hul.szClass, csh);
                } else {
                    strcpy(szWork, lpshdef->hul.szClass);
                    c = strlen(szWork);
                }
                if (cshT != 0) {
                    c += wsprintf(&szWork[c], " (-%d)", cshT);
                }
                SetTextColor(hdc, 8323072);
                TextOut(hdc, x, y, szWork, c);
                y += dyArial8;
                xT = (int16_t)(rc.right - (dxyVCRBoard + 14)) / 2 + x;
                SetTextColor(hdc, crButtonText);
                if (cshNew > 0) {
                    if (vrgtok[viVCRFocus].grobj == grobjPlanet) {
                        i = 0;
                    } else {
                        i = vrgtok[viVCRFocus].spd + 1;
                    }
                    c = wsprintf(szWork, PszGetCompressedString(idsInitiativeD), vrgtok[viVCRFocus].initMin >= 0xff ? 0 : vrgtok[viVCRFocus].initMin);
                    TextOut(hdc, x, y, szWork, c);
                    c = wsprintf(szWork, PszGetCompressedString(idsMovementS), &rgszSpeed[i * 3]);
                    TextOut(hdc, xT, y, szWork, c);
                    y += dyArial8;
                    c = wsprintf(szWork, PszGetCompressedString(idsArmorLd), dpT);
                    TextOut(hdc, x, y, szWork, c);
                    if (dv.dp == 0) {
                        c = CchGetString(idsDamageNone, szWork);
                    } else {
                        csh -= cshT;
                        csh = LOWORD((int32_t)(dv.pctSh * csh) / 100);
                        if (csh <= 0) {
                            csh = 1;
                        }
                        dpT = (uint32_t)(dv.pctDp / 5);
                        if (dpT == 0) {
                            dpT = 1;
                        }
                        SetTextColor(hdc, 127);
                        if (vrgtok[viVCRFocus].grobj == grobjPlanet) {
                            c = wsprintf(szWork, PszGetCompressedString(idsDamageD), LOWORD(dpT));
                        } else {
                            c = wsprintf(szWork, PszGetCompressedString(idsDamageLdD), csh, LOWORD(dpT));
                        }
                    }
                    TextOut(hdc, xT, y, szWork, c);
                    SetTextColor(hdc, crButtonText);
                    y += dyArial8;
                    if ((uint32_t)((uint32_t)vrgtok[viVCRFocus].dpShield * cshNew) - dpShields <= 0) {
                        c = CchGetString(idsShieldsNone, szWork);
                    } else {
                        c = wsprintf(szWork, PszGetCompressedString(idsShieldsLd), (uint32_t)((uint32_t)vrgtok[viVCRFocus].dpShield * cshNew) - dpShields);
                    }
                    TextOut(hdc, x, y, szWork, c);
                    y += dyArial8;
                    if (vrgtok[viVCRFocus].pctJam != 0) {
                        c = wsprintf(szWork, PszGetCompressedString(idsJammingD), vrgtok[viVCRFocus].pctJam);
                        TextOut(hdc, x, y, szWork, c);
                        y += dyArial8;
                    }
                    if (vrgtok[viVCRFocus].grobj != grobjPlanet) {
                        if (vbrcVCRFocus != 0xff) {
                            i = idsDisengage + vrgtok[viVCRFocus].mdTactic;
                        } else {
                            i = idsDisengage2;
                        }
                        if (i == idsDisengage) {
                            CchGetString(idsTacticSDMoves, szT);
                            c = wsprintf(szWork, szT, PszGetCompressedString(i), vrgtok[viVCRFocus].dzDis);
                        } else {
                            CchGetString(idsTacticS, szT);
                            c = wsprintf(szWork, szT, PszGetCompressedString(i));
                        }
                        TextOut(hdc, x, y, szWork, c);
                        y += dyArial8;
                    }
                    if (i != idsDisengage2) {
                        CchGetString(idsPrimayTargetS, szT);
                        i = idsNoneDisengage + vrgtok[viVCRFocus].mdTarget1;
                        c = wsprintf(szWork, szT, PszGetCompressedString(i));
                        TextOut(hdc, x, y, szWork, c);
                        y += dyArial8;
                        CchGetString(idsSecondaryTargetS, szT);
                        i = idsNoneDisengage + vrgtok[viVCRFocus].mdTarget2;
                        c = wsprintf(szWork, szT, PszGetCompressedString(i));
                        TextOut(hdc, x, y, szWork, c);
                        y += dyArial8;
                    }
                } else {
                    c = CchGetString(idsDead3, szWork);
                    TextOut(hdc, x, y, szWork, c);
                    y += 5 * dyArial8;
                }
                SetRect(&rc, x, y + 4, x + dyArial8 + 4, y + dyArial8 + 8);
                DrawBtn(hdc, &rc, 8, FALSE, "?");
            }
        }
        iStart = 0;
        iEnd = 99;
    }
    for (i = iStart; i <= iEnd; i++) {
        x = i % 10;
        y = i / 10;
        PatBlt(hdc, (dxyVCRSquare + 3) * x + 10, (dxyVCRSquare + 3) * y + 10, dxyVCRSquare + 2, 1, BLACKNESS);
        PatBlt(hdc, (dxyVCRSquare + 3) * x + 10, (dxyVCRSquare + 3) * y + 10, 1, dxyVCRSquare + 2, BLACKNESS);
        PatBlt(hdc, (dxyVCRSquare + 3) * x + 10 + dxyVCRSquare + 1, (dxyVCRSquare + 3) * y + 10, 1, dxyVCRSquare + 2, BLACKNESS);
        PatBlt(hdc, (dxyVCRSquare + 3) * x + 10, (dxyVCRSquare + 3) * y + 10 + dxyVCRSquare + 1, dxyVCRSquare + 2, 1, BLACKNESS);
        ctok = 0;
        ibmp = -1;
        dpT = 0;
        for (j = 0; j < vlpbdVCR->ctok; j++) {
            if (vrgtok[j].brc == (((y & 0xf) << 4 | (x & 0xf)) & 0xff) && vrgtok[j].csh > 0) {
                ctok++;
                dpT += (uint32_t)vrgtok[j].csh;
                if (ibmp == -1 || j == viVCRFocus) {
                    if (vrgtok[j].grobj == grobjPlanet) {
                        ibmp = rglpshdefSB[vrgtok[j].iplr][vrgtok[j].ishdef - 16].hul.ibmp;
                    } else {
                        ibmp = rglpshdef[vrgtok[j].iplr][vrgtok[j].ishdef].hul.ibmp;
                    }
                    ibmpRace = rgplr[vrgtok[j].iplr].iPlrBmp;
                }
            }
        }
        if (dpT < 32767) {
            csh = LOWORD(dpT);
        } else {
            csh = 32767;
        }
        if (ctok > 0) {
            PatBlt(hdc, (dxyVCRSquare + 3) * x + 10, (dxyVCRSquare + 3) * y + 10, dxyVCRSquare + 2, 1, BLACKNESS);
            PatBlt(hdc, (dxyVCRSquare + 3) * x + 10, (dxyVCRSquare + 3) * y + 10, 1, dxyVCRSquare + 2, BLACKNESS);
            PatBlt(hdc, (dxyVCRSquare + 3) * x + 10 + dxyVCRSquare + 1, (dxyVCRSquare + 3) * y + 10, 1, dxyVCRSquare + 2, BLACKNESS);
            PatBlt(hdc, (dxyVCRSquare + 3) * x + 10, (dxyVCRSquare + 3) * y + 10 + dxyVCRSquare + 1, dxyVCRSquare + 2, 1, BLACKNESS);
            DrawFleetBitmap(NULL, hdc, (dxyVCRSquare + 3) * x + 11, (dxyVCRSquare + 3) * y + 11, FALSE, ibmp, ctok, dxyVCRSquare < 64, ibmpRace, csh);
        } else {
            PatBlt(hdc, (dxyVCRSquare + 3) * x + 10, (dxyVCRSquare + 3) * y + 10, dxyVCRSquare + 2, dxyVCRSquare + 2, BLACKNESS);
        }
        if ((vbrcVCRFocus == (((y & 0xf) << 4 | (x & 0xf)) & 0xff) && viVCRFocus == -1) ||
            (viVCRFocus >= 0 && vrgtok[viVCRFocus].brc == (((y & 0xf) << 4 | (x & 0xf)) & 0xff) && vrgtok[viVCRFocus].csh > 0)) {
            hbrSav = SelectObject(hdc, hbrBlue);
            PatBlt(hdc, (dxyVCRSquare + 3) * x + 10, (dxyVCRSquare + 3) * y + 10, dxyVCRSquare + 1, 2, PATCOPY);
            PatBlt(hdc, (dxyVCRSquare + 3) * x + 10, (dxyVCRSquare + 3) * y + 10, 2, dxyVCRSquare + 1, PATCOPY);
            PatBlt(hdc, (dxyVCRSquare + 3) * x + 10, (dxyVCRSquare + 3) * y + 10 + dxyVCRSquare, dxyVCRSquare + 1, 2, PATCOPY);
            PatBlt(hdc, (dxyVCRSquare + 3) * x + 10 + dxyVCRSquare, (dxyVCRSquare + 3) * y + 10, 2, dxyVCRSquare + 1, PATCOPY);
            SelectObject(hdc, hbrSav);
        }
    }
    if (vlpbrVCR->ctok > 0) {
        AnimateAttack(hdc);
    }
    SetBkMode(hdc, bkMode);
    SelectObject(hdc, hbrSav);
    if (fCreatedDC) {
        ReleaseDC(hwndVCRDlg, hdc);
    }
    return;
}

void Delay(int16_t ctick) {
    uint32_t dwTickLast;
    uint32_t dwTickCur;

    dwTickLast = GetTickCount();
    do {
        dwTickCur = GetTickCount();
    } while (dwTickCur >= dwTickLast && dwTickCur < ctick + dwTickLast);
    return;
}

void AnimateAttack(HDC hdc) {
    TOK      *ptokSrc;
    TOK      *ptokAttack;
    POINT16   ptBeam1;
    int16_t   cFrame;
    int16_t   dyFrame;
    POINT16   ptRay2;
    POINT16   ptTop;
    uint32_t  dwTickLast;
    int16_t   dxFrame;
    uint32_t  dwTickCur;
    POINT16   ptBase;
    int16_t   dy;
    POINT16   ptRay1;
    int16_t   y;
    POINT16   ptRight;
    POINT16   ptDest;
    int16_t   iHit;
    GrfWeapon grfWeapon;
    POINT16   ptSrc;
    POINT16   ptTorp;
    POINT16   ptLeft;
    int16_t   iFrame;
    int16_t   dx;
    int16_t   fKill;
    POINT16   ptDestBottom;
    POINT16   ptBeam2;
    POINT16   ptBottom;
    POINT16   ptDestTop;
    POINT16   ptDestRight;
    POINT16   ptDestLeft;
    int16_t   x;
    HDC       hdcMem;
    HBITMAP   hbmpSav;
    HBITMAP   hbmpScreen;

    grfWeapon = 0;
    fKill = FALSE;
    if (viStepVCRCur >= 0) {
        ptokSrc = vrgtok + vlpbrVCR->itok;
        x = ptokSrc->brc & 0xf;
        y = ptokSrc->brc >> 4;
        ptSrc.x = (dxyVCRSquare + 3) * x + 10 + dxyVCRSquare / 2 + 1;
        ptSrc.y = (dxyVCRSquare + 3) * y + 10 + dxyVCRSquare / 2 + 1;
        ptBottom.x = ptTop.x = (dxyVCRSquare + 3) * x + 10 + dxyVCRSquare / 2 + 1;
        ptLeft.y = ptRight.y = (dxyVCRSquare + 3) * y + 10 + dxyVCRSquare / 2 + 1;
        ptBottom.y = dxyVCRSquare / 3 + ptLeft.y;
        ptTop.y = ptLeft.y - dxyVCRSquare / 3;
        ptLeft.x = ptBottom.x - dxyVCRSquare / 3;
        ptRight.x = dxyVCRSquare / 3 + ptBottom.x;
        iHit = 0;
    LNextTarget:
        grfWeapon = vlpbrVCR->rgkill[iHit].grfWeapon;
        if (vlpbrVCR->rgkill[iHit].cshKill != 0) {
            fKill = TRUE;
        }
        for (iFrame = iHit + 1; iFrame < vlpbrVCR->ctok && vlpbrVCR->rgkill[iFrame].itok == vlpbrVCR->rgkill[iHit].itok; iFrame++) {
            grfWeapon |= vlpbrVCR->rgkill[iFrame].grfWeapon;
            if (vlpbrVCR->rgkill[iFrame].cshKill != 0) {
                fKill = TRUE;
            }
        }
        ptokAttack = vrgtok + vlpbrVCR->rgkill[iHit].itok;
        x = ptokAttack->brc & 0xf;
        y = ptokAttack->brc >> 4;
        dx = (ptokSrc->brc & 0xf) - x;
        dy = (ptokSrc->brc >> 4) - y;
        ptDest.x = (dxyVCRSquare + 3) * x + 10 + dxyVCRSquare / 2 + 1;
        ptDest.y = (dxyVCRSquare + 3) * y + 10 + dxyVCRSquare / 2 + 1;
        ptDestBottom.x = ptDestTop.x = (dxyVCRSquare + 3) * x + 10 + dxyVCRSquare / 2 + 1;
        ptDestLeft.y = ptDestRight.y = (dxyVCRSquare + 3) * y + 10 + dxyVCRSquare / 2 + 1;
        ptDestBottom.y = dxyVCRSquare / 3 + ptDestLeft.y;
        ptDestTop.y = ptDestLeft.y - dxyVCRSquare / 3;
        ptDestLeft.x = ptDestBottom.x - dxyVCRSquare / 3;
        ptDestRight.x = dxyVCRSquare / 3 + ptDestBottom.x;
        iHit = iFrame;
        if (dx != 0 || dy != 0) {
            if (dx == 0 || (abs(dx) == 1 && abs(dy) > 2)) {
                ptBeam1 = ptRight;
                ptBeam2 = ptLeft;
                if (dy > 0) {
                    ptTorp = ptTop;
                } else {
                    ptTorp = ptBottom;
                }
                if (dy > 0) {
                    ptRay2 = ptDestLeft;
                } else {
                    ptRay2 = ptDestRight;
                }
                if (dy > 0) {
                    ptRay1 = ptDestRight;
                } else {
                    ptRay1 = ptDestLeft;
                }
            } else if (dy == 0 || (abs(dy) == 1 && abs(dx) > 2)) {
                ptBeam1 = ptTop;
                ptBeam2 = ptBottom;
                if (dx > 0) {
                    ptTorp = ptLeft;
                } else {
                    ptTorp = ptRight;
                }
                if (dx > 0) {
                    ptRay1 = ptDestTop;
                } else {
                    ptRay1 = ptDestBottom;
                }
                if (dx > 0) {
                    ptRay2 = ptDestBottom;
                } else {
                    ptRay2 = ptDestTop;
                }
            } else if (dx > 0) {
                if (dy > 0) {
                    ptRay2 = ptDestBottom;
                    ptRay1 = ptDestRight;
                    ptBeam1 = ptTop;
                } else {
                    ptRay2 = ptDestRight;
                    ptRay1 = ptDestTop;
                    ptBeam1 = ptBottom;
                }
                ptBeam2 = ptLeft;
                ptTorp.x = ptLeft.x;
                ptTorp.y = ptBeam1.y;
            } else {
                if (dy > 0) {
                    ptBeam1 = ptTop;
                    ptRay1 = ptDestBottom;
                    ptRay2 = ptDestLeft;
                } else {
                    ptBeam1 = ptBottom;
                    ptRay1 = ptDestLeft;
                    ptRay2 = ptDestTop;
                }
                ptBeam2 = ptRight;
                ptTorp.x = ptRight.x;
                ptTorp.y = ptBeam1.y;
            }
            if (grfWeapon & (bitFBeamLow | bitFBeamHigh)) {
                SelectObject(hdc, !(grfWeapon & bitFBeamHigh) ? hpenEnemy : hpenStarbase);
                MoveToEx(hdc, ptBeam1.x, ptBeam1.y, NULL);
                LineTo(hdc, ptDest.x, ptDest.y);
                MoveToEx(hdc, ptBeam2.x, ptBeam2.y, NULL);
                LineTo(hdc, ptDest.x, ptDest.y);
                DrawIcon(hdc, ptDest.x - 16, ptDest.y - 16, rghiconVCR[0]);
            }
            if (grfWeapon & bitFTorp) {
                if (fAnimate) {
                    hdcMem = CreateCompatibleDC(hdc);
                    if (!hdcMem)
                        goto LFinishUp;
                    hbmpScreen = CreateCompatibleBitmap(hdc, 32, 32);
                    if (!hbmpScreen) {
                        DeleteDC(hdcMem);
                        goto LFinishUp;
                    }
                    hbmpSav = SelectObject(hdcMem, hbmpScreen);
                    cFrame = max(abs(dx), abs(dy)) * (!(grfWeapon & bitFTorp) ? 4 : 8);
                    ptBase = ptTorp;
                    dxFrame = ptTorp.x - ptDest.x;
                    dyFrame = ptTorp.y - ptDest.y;
                    dwTickLast = GetTickCount();
                    for (iFrame = 0; iFrame < cFrame; iFrame++) {
                        BitBlt(hdcMem, 0, 0, 32, 32, hdc, ptTorp.x - 16, ptTorp.y - 16, SRCCOPY);
                        DrawIcon(hdc, ptTorp.x - 16, ptTorp.y - 16, rghiconVCR[(iFrame & 3) + 3]);
                        do {
                            dwTickCur = GetTickCount();
                        } while (dwTickCur >= dwTickLast && dwTickCur < dwTickLast + 35 - (int16_t)(10 * viSpeedVCR));
                        dwTickLast = dwTickCur;
                        BitBlt(hdc, ptTorp.x - 16, ptTorp.y - 16, 32, 32, hdcMem, 0, 0, SRCCOPY);
                        ptTorp.x = ptBase.x - LOWORD((int32_t)((int32_t)(dxFrame * iFrame) / cFrame));
                        ptTorp.y = ptBase.y - LOWORD((int32_t)((int32_t)(dyFrame * iFrame) / cFrame));
                    }
                    SelectObject(hdcMem, hbmpSav);
                    DeleteObject(hbmpScreen);
                    DeleteDC(hdcMem);
                }
                if (!(grfWeapon & bitFNoHit)) {
                    DrawIcon(hdc, ptDest.x - 16, ptDest.y - 16, rghiconVCR[1]);
                }
            }
        }
    LFinishUp:
        if (iHit < vlpbrVCR->ctok)
            goto LNextTarget;
        for (iFrame = 0; iFrame < vlpbrVCR->ctok; iFrame++) {
            fKill = vlpbrVCR->rgkill[iFrame].cshKill > 0;
            ptokAttack = vrgtok + vlpbrVCR->rgkill[iFrame].itok;
            ptDest.x = (ptokAttack->brc & 0xf) * (dxyVCRSquare + 3) + 10 + dxyVCRSquare / 2 + 1;
            ptDest.y = (ptokAttack->brc >> 4) * (dxyVCRSquare + 3) + 10 + dxyVCRSquare / 2 + 1;
            DrawIcon(hdc, ptDest.x - 16, ptDest.y - 16, rghiconVCR[fKill == 0 ? 0 : 2]);
        }
        fAnimate = FALSE;
    }
    return;
}

int16_t PopupVCRMenu(HWND hwnd, int16_t x, int16_t y, uint8_t brc) {
    int16_t fAttack;
    char   *rgsz[40];
    int16_t i;
    int16_t c;
    char    rgch[1536];
    SHDEF  *lpshdef;
    int16_t rgid[40];
    int16_t iChecked;
    int16_t iSel;
    int16_t j;
    char   *psz;
    int16_t cch;
    int16_t cKilled;

    c = 0;
    iChecked = -1;
    psz = rgch;
    fAttack = brc == vrgtok[vlpbrVCR->itokAttack].brc;
    for (i = 0; i < vlpbdVCR->ctok; i++) {
        if (vrgtok[i].brc == brc && vrgtok[i].csh > 0) {
            if (PszPlayerName(vrgtok[i].iplr, FALSE, FALSE, FALSE, 0, NULL) != szWork) {
            }
            cch = strlen(szWork);
            if (vrgtok[i].grobj == grobjPlanet) {
                lpshdef = rglpshdefSB[vrgtok[i].iplr] + (vrgtok[i].ishdef - 16);
            } else {
                lpshdef = rglpshdef[vrgtok[i].iplr] + vrgtok[i].ishdef;
            }
            cch += wsprintf(&szWork[cch], " %s * %d", lpshdef->hul.szClass, vrgtok[i].csh);
            if (fAttack && vlpbrVCR->ctok > 0 && viStepVCRCur >= 0) {
                cKilled = 0;
                for (j = 0; j < vlpbrVCR->ctok; j++) {
                    if (vlpbrVCR->rgkill[j].itok == i) {
                        cKilled += vlpbrVCR->rgkill[j].cshKill;
                    }
                }
                if (cKilled > 0) {
                    cch += wsprintf(&szWork[cch], " (-%d)", cKilled);
                }
            }
            if (psz + (cch + 1) >= &rgch[1535] || c >= 40)
                break;
            rgsz[c] = psz;
            rgid[c] = i;
            strcpy(psz, szWork);
            psz += cch + 1;
            if (i == viVCRFocus) {
                iChecked = c;
            }
            c++;
        }
    }
    if (c == 0) {
        return -1;
    }
    iSel = PopupMenu(hwnd, x, y, c, NULL, rgsz, iChecked, TRUE);
    if (iSel == -1) {
        return -1;
    }
    return rgid[iSel];
}

void EnableVCRButtons() {
    int16_t i;

    for (i = 161; i < 163; i++) {
        EnableWindow(GetDlgItem(hwndVCRDlg, i), viStepVCRCur > -1);
    }
    for (i = 163; i < 166; i++) {
        EnableWindow(GetDlgItem(hwndVCRDlg, i), viStepVCRCur < vcStepVCR);
    }
    if (viStepVCRCur == -1) {
        SetFocus(GetDlgItem(hwndVCRDlg, IDC_VCR_PLAY_PAUSE));
    } else if (viStepVCRCur == vcStepVCR) {
        SetFocus(GetDlgItem(hwndVCRDlg, IDOK));
    }
    return;
}
