#include "win.h"

INT_PTR CALLBACK RelationsDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    int16_t     i;
    RECT        rc;
    HDC         hdc;
    PAINTSTRUCT ps;
    RECT        rcGBox;
    ScanView    mdSBase;

    switch (message) {
    case WM_INITDIALOG:
        StickyDlgPos(hwnd, &ptStickyRelationsDlg, TRUE);
        CheckRadioButton(hwnd, IDC_RELATIONS_NEUTRAL, IDC_RELATIONS_ENEMY, rgplr[idPlayer].rgmdRelation[idPlayer == 0] + 2004);
        for (i = 0; i < game.cPlayer; i++) {
            if (i != idPlayer) {
                SendMessage(GetDlgItem(hwnd, IDC_RELATIONS_PLAYER_LIST), LB_ADDSTRING, 0, (LPARAM)PszPlayerName(i, FALSE, FALSE, FALSE, 0, NULL));
            }
        }
        SendMessage(GetDlgItem(hwnd, IDC_RELATIONS_PLAYER_LIST), LB_SETCURSEL, 0, 0);
        fDirtyPlan = FALSE;
        /* fallthrough */
    case WM_ERASEBKGND:
        GetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        GetWindowRect(GetDlgItem(hwnd, IDC_RELATIONS_FRIEND), &rcGBox);
        ScreenToClient(hwnd, (POINT *)&rcGBox);
        GetWindowRect(GetDlgItem(hwnd, IDC_RELATIONS_ENEMY), &rc);
        ScreenToClient(hwnd, (POINT *)&rc.right);
        rcGBox.right = rc.right;
        rcGBox.bottom = rc.bottom;
        ExpandRc(&rcGBox, dyArial8, dyArial8 >> 1);
        _Draw3dFrame(hdc, &rcGBox, -1);
        SetBkColor(hdc, crButtonFace);
        SelectObject(hdc, rghfontArial8[1]);
        i = CchGetString(idsRelation, szWork);
        TextOut(hdc, rcGBox.left + 8, rcGBox.top - (dyArial8 >> 1), szWork, i);
        SelectObject(hdc, rghfontArial8[0]);
        EndPaint(hwnd, &ps);
        return 1;
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        if ((HWND)lParam != GetDlgItem(hwnd, IDC_RELATIONS_PLAYER_LIST)) {
            SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDCANCEL) {
            StickyDlgPos(hwnd, &ptStickyRelationsDlg, FALSE);
            i = LOWORD(SendMessage(GetDlgItem(hwnd, IDC_RELATIONS_PLAYER_LIST), LB_GETCURSEL, 0, 0));
            if (i >= idPlayer) {
                i++;
            }
            EndDialog(hwnd, i + 3);
            return 1;
        }
        if (LOWORD(wParam) >= IDC_RELATIONS_NEUTRAL && LOWORD(wParam) <= IDC_RELATIONS_ENEMY) {
            i = LOWORD(SendMessage(GetDlgItem(hwnd, IDC_RELATIONS_PLAYER_LIST), LB_GETCURSEL, 0, 0));
            if (i >= idPlayer) {
                i++;
            }
            rgplr[idPlayer].rgmdRelation[i] = LOWORD(wParam) - 2004;
            fDirtyPlan = TRUE;
        } else if (LOWORD(wParam) == IDC_RELATIONS_PLAYER_LIST) {
            i = LOWORD(SendMessage(GetDlgItem(hwnd, IDC_RELATIONS_PLAYER_LIST), LB_GETCURSEL, 0, 0));
            if (i >= idPlayer) {
                i++;
            }
            CheckRadioButton(hwnd, IDC_RELATIONS_NEUTRAL, IDC_RELATIONS_ENEMY, rgplr[idPlayer].rgmdRelation[i] + 2004);
        } else if (LOWORD(wParam) == IDC_HELP) {
            WinHelp(hwnd, szHelpFile, HELP_CONTEXT, idhPlayerRelationsDialog);
            return 1;
        }
        break;
    case WM_DESTROY:
        if (!fDirtyPlan)
            break;
        mdSBase = grbitScan & grbitScanViewMask;
        LogChangeRelations();
        InvalidateRect(hwndScanner, NULL, TRUE);
        break;
    }
    return 0;
}

INT_PTR CALLBACK NewPlanNameDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    RECT rc;

    switch (message) {
    case WM_INITDIALOG:
        SetWindowPos(hwnd, NULL, ptStickyBattlePlansDlg.x + 70, ptStickyBattlePlansDlg.y + 70, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        SendDlgItemMessage(hwnd, IDC_EDIT1, EM_LIMITTEXT, 0x1f, 0);
        SetDlgItemText(hwnd, IDC_EDIT1, btlplan.szName);
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
                GetDlgItemText(hwnd, IDC_EDIT1, btlplan.szName, 32);
                fDirtyPlan = TRUE;
            }
            EndDialog(hwnd, LOWORD(wParam) == IDOK);
            return 1;
        case IDC_HELP:
            WinHelp(hwnd, szHelpFile, HELP_CONTEXT, idhBattlePlansDialog);
            return 1;
        }
        break;
    }
    return 0;
}

INT_PTR CALLBACK BattlePlansDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    FARPROC lpProc;
    int16_t idc;
    int16_t i;
    int16_t fRet;
    RECT    rc;
    int16_t cLen;

    switch (message) {
    case WM_INITDIALOG:
        StickyDlgPos(hwnd, &ptStickyBattlePlansDlg, TRUE);
        iPlanSelDlg = 0;
        if (sel.grobj == grobjFleet) {
            iPlanSelDlg = sel.fl.iplan;
        }
        btlplan = rglpbtlplan[idPlayer][iPlanSelDlg];
        for (i = 0; i < rgcbtlplan[idPlayer]; i++) {
            SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), CB_ADDSTRING, 0, (LPARAM)rglpbtlplan[idPlayer][i].szName);
        }
        SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), CB_SETCURSEL, iPlanSelDlg, 0);
        EnableWindow(GetDlgItem(hwnd, IDC_RENAME), iPlanSelDlg > 0);
        EnableWindow(GetDlgItem(hwnd, IDC_DELETE), iPlanSelDlg > 0);
        for (i = idsDisengage; i <= idsMaximizeDamage; i++) {
            SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_TACTIC), CB_ADDSTRING, 0, (LPARAM)PszGetCompressedString(i));
        }
        SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_TACTIC), CB_SETCURSEL, btlplan.mdTactic, 0);
        for (i = idsNoneDisengage; i <= idsFreighters; i++) {
            SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_PRIMARY_TARGET), CB_ADDSTRING, 0, (LPARAM)PszGetCompressedString(i));
        }
        SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_PRIMARY_TARGET), CB_SETCURSEL, btlplan.mdTarget1, 0);
        if (!game.fSinglePlr) {
            for (i = idsNobody; i <= idsEveryone; i++) {
                SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_ATTACK_WHO), CB_ADDSTRING, 0, (LPARAM)PszGetCompressedString(i));
            }
            for (i = 0; i < game.cPlayer; i++) {
                if (i != idPlayer) {
                    SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_ATTACK_WHO), CB_ADDSTRING, 0, (LPARAM)PszPlayerName(i, FALSE, TRUE, FALSE, 0, NULL));
                }
            }
            i = btlplan.iplrAttack;
            if (i >= idPlayer + 4) {
                i--;
            }
            SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_ATTACK_WHO), CB_SETCURSEL, i, 0);
        } else {
            SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_ATTACK_WHO), CB_ADDSTRING, 0, (LPARAM)PszGetCompressedString(idsEveryone));
            SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_ATTACK_WHO), CB_SETCURSEL, 0, 0);
            EnableWindow(GetDlgItem(hwnd, IDC_BATTLE_PLAN_ATTACK_WHO), FALSE);
        }
        for (i = idsNoneDisengage; i <= idsFreighters; i++) {
            SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_SECONDARY_TARGET), CB_ADDSTRING, 0, (LPARAM)PszGetCompressedString(i));
        }
        SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_SECONDARY_TARGET), CB_SETCURSEL, btlplan.mdTarget2, 0);
        SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_DUMP_CARGO), BM_SETCHECK, btlplan.fDumpCargo, 0);
        fDirtyPlan = FALSE;
        if (gd.fTutorial) {
            AdvanceTutor();
        }
        return 1;
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        for (idc = 1053; idc <= 1058 && (HWND)lParam != GetDlgItem(hwnd, idc); idc++) {
        }
        if (idc >= 1053 || message == WM_CTLCOLORSTATIC) {
            SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case WM_ERASEBKGND:
        GetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            if (fDirtyPlan) {
                rglpbtlplan[idPlayer][iPlanSelDlg] = btlplan;
                LogChangeBtlplan(&btlplan);
            }
            StickyDlgPos(hwnd, &ptStickyBattlePlansDlg, FALSE);
            EndDialog(hwnd, iPlanSelDlg);
            if (sel.grobj == grobjFleet) {
                FillBattleDD(sel.fl.iplan + 1);
            }
            iPlanSelDlg = -1;
            return 1;
        case IDC_BATTLE_PLAN_DUMP_CARGO:
            btlplan.fDumpCargo = LOWORD(SendDlgItemMessage(hwnd, IDC_BATTLE_PLAN_DUMP_CARGO, BM_GETCHECK, 0, 0));
            fDirtyPlan = TRUE;
            break;
        case IDC_DELETE:
            if (fDirtyPlan) {
                rglpbtlplan[idPlayer][iPlanSelDlg] = btlplan;
                LogChangeBtlplan(&btlplan);
                fDirtyPlan = FALSE;
            }
            btlplan.fDelete = TRUE;
            rglpbtlplan[idPlayer][iPlanSelDlg] = btlplan;
            btlplan.iplan = iPlanSelDlg;
            if (FDeleteBattlePlan(iPlanSelDlg, TRUE)) {
                LogChangeBtlplan(&btlplan);
                SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), CB_SETCURSEL, iPlanSelDlg - 1, 0);
                SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), CB_RESETCONTENT, 0, 0);
                for (i = 0; i < rgcbtlplan[idPlayer]; i++) {
                    SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), CB_ADDSTRING, 0, (LPARAM)rglpbtlplan[idPlayer][i].szName);
                }
                SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), CB_SETCURSEL, iPlanSelDlg - 1, 0);
                goto LSelectName;
            }
            btlplan.fDelete = FALSE;
            rglpbtlplan[idPlayer][iPlanSelDlg] = btlplan;
            break;
        case IDC_BATTLE_PLAN_PRIMARY_TARGET:
            i = LOWORD(SendMessage(GetDlgItem(hwnd, LOWORD(wParam)), CB_GETCURSEL, 0, 0));
            btlplan.mdTarget1 = i;
            fDirtyPlan = TRUE;
            break;
        case IDC_BATTLE_PLAN_SECONDARY_TARGET:
            i = LOWORD(SendMessage(GetDlgItem(hwnd, LOWORD(wParam)), CB_GETCURSEL, 0, 0));
            btlplan.mdTarget2 = i;
            fDirtyPlan = TRUE;
            break;
        case IDC_BATTLE_PLAN_ATTACK_WHO:
            i = LOWORD(SendMessage(GetDlgItem(hwnd, LOWORD(wParam)), CB_GETCURSEL, 0, 0));
            if (game.fSinglePlr) {
                i = 3;
            } else if (i >= idPlayer + 4) {
                i++;
            }
            btlplan.iplrAttack = i;
            fDirtyPlan = TRUE;
            break;
        case IDC_BATTLE_PLAN_TACTIC:
            i = LOWORD(SendMessage(GetDlgItem(hwnd, LOWORD(wParam)), CB_GETCURSEL, 0, 0));
            btlplan.mdTactic = i;
            fDirtyPlan = TRUE;
            break;
        case IDC_RENAME:
        LRename:
            StickyDlgPos(hwnd, &ptStickyBattlePlansDlg, FALSE);
            lpProc = MakeProcInstance(NewPlanNameDlg, hInst);
            fRet = DialogBox(hInst, MAKEINTRESOURCE(IDD_RENAME), hwndFrame, lpProc);
            FreeProcInstance(lpProc);
            SetFocus(hwnd);
            if (fRet) {
                rglpbtlplan[idPlayer][iPlanSelDlg] = btlplan;
                SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), CB_RESETCONTENT, 0, 0);
                for (i = 0; i < rgcbtlplan[idPlayer]; i++) {
                    SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), CB_ADDSTRING, 0, (LPARAM)rglpbtlplan[idPlayer][i].szName);
                }
                SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), CB_SETCURSEL, iPlanSelDlg, 0);
            }
            EnableWindow(GetDlgItem(hwnd, IDC_RENAME), iPlanSelDlg > 0);
            EnableWindow(GetDlgItem(hwnd, IDC_DELETE), iPlanSelDlg > 0);
            break;
        case IDC_BATTLE_PLAN_COPY:
            if (rgcbtlplan[idPlayer] == 15) {
                return 0;
            }
            if (fDirtyPlan) {
                rglpbtlplan[idPlayer][iPlanSelDlg] = btlplan;
                LogChangeBtlplan(&btlplan);
                fDirtyPlan = FALSE;
            }
            iPlanSelDlg = rgcbtlplan[idPlayer]++;
            cLen = strlen(btlplan.szName);
            if (cLen <= 27) {
                if (cLen < 3 || btlplan.szName[cLen - 1] != ')' || isdigit(btlplan.szName[cLen - 2]) == 0 || btlplan.szName[cLen - 3] != '(') {
                    strcpy(&btlplan.szName[cLen], " (2)");
                } else if (btlplan.szName[cLen - 2] == '9') {
                    btlplan.szName[cLen - 2] = '0';
                } else {
                    btlplan.szName[cLen - 2] = btlplan.szName[cLen - 2] + 1;
                }
            }
            btlplan.iplan = iPlanSelDlg;
            rglpbtlplan[idPlayer][iPlanSelDlg] = btlplan;
            SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_TACTIC), CB_SETCURSEL, btlplan.mdTactic, 0);
            SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), CB_RESETCONTENT, 0, 0);
            for (i = 0; i < rgcbtlplan[idPlayer]; i++) {
                SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), CB_ADDSTRING, 0, (LPARAM)rglpbtlplan[idPlayer][i].szName);
            }
            SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), CB_SETCURSEL, iPlanSelDlg, 0);
            SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_PRIMARY_TARGET), CB_SETCURSEL, btlplan.mdTarget1, 0);
            SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_SECONDARY_TARGET), CB_SETCURSEL, btlplan.mdTarget2, 0);
            SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_DUMP_CARGO), BM_SETCHECK, btlplan.fDumpCargo, 0);
            i = btlplan.iplrAttack;
            if (i >= idPlayer + 4) {
                i--;
            }
            SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_ATTACK_WHO), CB_SETCURSEL, i, 0);
            fDirtyPlan = TRUE;
            wParam = IDC_BATTLE_PLAN_PRIMARY_TARGET;
            EnableWindow(GetDlgItem(hwnd, IDC_RENAME), TRUE);
            goto LRename;
        case IDC_BATTLE_PLAN_SELECT:
        LSelectName:
            i = LOWORD(SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), CB_GETCURSEL, 0, 0));
            if (i == iPlanSelDlg)
                break;
            if (fDirtyPlan) {
                rglpbtlplan[idPlayer][iPlanSelDlg] = btlplan;
                LogChangeBtlplan(&btlplan);
                fDirtyPlan = FALSE;
            }
            iPlanSelDlg = i;
            btlplan = rglpbtlplan[idPlayer][iPlanSelDlg];
            SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_PRIMARY_TARGET), CB_SETCURSEL, btlplan.mdTarget1, 0);
            SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_SECONDARY_TARGET), CB_SETCURSEL, btlplan.mdTarget2, 0);
            SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_DUMP_CARGO), BM_SETCHECK, btlplan.fDumpCargo, 0);
            wParam = IDC_BATTLE_PLAN_PRIMARY_TARGET;
            EnableWindow(GetDlgItem(hwnd, IDC_RENAME), iPlanSelDlg > 0);
            EnableWindow(GetDlgItem(hwnd, IDC_DELETE), iPlanSelDlg > 0);
            SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_TACTIC), CB_SETCURSEL, btlplan.mdTactic, 0);
            i = btlplan.iplrAttack;
            if (i >= idPlayer + 4) {
                i--;
            }
            SendMessage(GetDlgItem(hwnd, IDC_BATTLE_PLAN_ATTACK_WHO), CB_SETCURSEL, i, 0);
            break;
        case IDC_HELP:
            WinHelp(hwnd, szHelpFile, HELP_CONTEXT, idhBattlePlansDialog);
            return 1;
        }
        break;
    }
    return 0;
}
