#include "win.h"

void NewGameWizard(HWND hwnd, int16_t fReadOnly) {
    int16_t iStepMaxSoFar;
    int16_t mdRet;
    FARPROC lpProc;
    int16_t fIdleSav;
    int16_t rgplrbmp[16];
    int16_t i;
    int16_t c;
    char    szFile[256];
    AiRace  idAi;
    int16_t fEasy;
    char    szFileLocal[16 * sizeof(szRaceFile)];
    int16_t j;
    RECT    rgrcStack[20];
    PLAYER  rgplrLocal[16];
    int16_t lvlAi;
    GAME    gameT;

    iStepMaxSoFar = 0;
    fEasy = FALSE;
    vrgrcRCW = rgrcStack;
    iPanelActive = rwPageNone;
    fRCWReadOnly = fReadOnly;
    fIdleSav = gd.fNoIdleChecks;
    gd.fNoIdleChecks = TRUE;
    vrgplrNew = rgplrLocal;
    vrgszFileNew = szFileLocal;
    vcplrNew = 0;
    memset(vrgplrTypeNew, 0, 16);
    if (!fReadOnly) {
        gameT = game;
        memset(&game, 0, sizeof(GAME));
        game.cPlanMax = gameT.cPlanMax;
        game.cPlayer = gameT.cPlayer;
        vrgplrTypeNew[0] = 1;
        vrgplrTypeNew[1] = 35;
        vrgplrTypeNew[2] = 39;
        vrgplrTypeNew[3] = 139;
        lpProc = MakeProcInstance(SimpleNewGameDlg, hInst);
        mdRet = DialogBox(hInst, MAKEINTRESOURCE(IDD_SIMPLE_NEW_GAME), hwnd, lpProc);
        FreeProcInstance(lpProc);
        game.rgvc[0] = 136;
        game.rgvc[1] = 142;
        game.rgvc[2] = 130;
        game.rgvc[3] = 10;
        game.rgvc[4] = 136;
        game.rgvc[5] = 9;
        game.rgvc[6] = 9;
        game.rgvc[7] = 7;
        game.rgvc[8] = 1;
        switch (mdRet) {
        case 212:
            game = gameT;
            StartTutor(FALSE);
            gd.fNoIdleChecks = fIdleSav;
            break;
        case 1072:
        case 211:
            memset(vrgplrTypeNew, 0, 16);
            if (game.turn < 7) {
                vrgplrTypeNew[0] = game.turn << 2 | 1;
            } else {
                vrgplrTypeNew[0] = 2;
                *vrgszFileNew = 0;
                vcplrNew = 1;
            }
            lvlAi = game.mdDensity;
            InitNewGamePlr(iStepMaxSoFar, lvlAi);
            game.mdDensity = densityNormal;
            game.mdStartDist = lvlAi < 2 ? startDistModerate : startDistFarther;
            game.fExtraFuel = FALSE;
            game.fSlowTech = FALSE;
            game.fBBSPlay = FALSE;
            game.fNoRandom = FALSE;
            game.fAisBand = lvlAi == 3;
            game.fVisScores = FALSE;
            CchGetString(idsShootingFishBarrel + 5 * lvlAi + game.mdSize, game.szName);
            if (mdRet == 1072) {
                fEasy = TRUE;
                goto Finish;
            }
        default:
            goto Step1;
        case 2:
            goto Cancel;
        }
        return;
    }
    for (i = 0; i < game.cPlayer; i++) {
        if (rgplr[i].fInclude) {
            vrgplrTypeNew[i] = i << 2 | 2;
            vrgplrNew[i] = rgplr[i];
            vrgszFileNew[i * sizeof(szRaceFile)] = 0;
        } else {
            vrgplrTypeNew[i] = 25;
        }
    }
Step1:
    lpProc = MakeProcInstance(NewGameDlg, hInst);
    mdRet = DialogBox(hInst, MAKEINTRESOURCE(IDD_NEW_GAME_1), hwnd, lpProc);
    FreeProcInstance(lpProc);
    if (mdRet == 0) {
    Cancel:
        if (!fRCWReadOnly) {
            game = gameT;
        }
        gd.fNoIdleChecks = fIdleSav;
        return;
    }
    InitNewGamePlr(iStepMaxSoFar, lvlAi);
    if (mdRet == 3)
        goto Finish;
Step2:
    lpProc = MakeProcInstance(NewGameDlg2, hInst);
    mdRet = DialogBox(hInst, MAKEINTRESOURCE(IDD_NEW_GAME_2), hwnd, lpProc);
    FreeProcInstance(lpProc);
    if (mdRet == 0)
        goto Cancel;
    if (iStepMaxSoFar < 1) {
        iStepMaxSoFar = 1;
    }
    InitNewGamePlr(iStepMaxSoFar, lvlAi);
    if (mdRet == 1)
        goto Step1;
    if (mdRet == 3)
        goto Finish;
    lpProc = MakeProcInstance(NewGameDlg3, hInst);
    mdRet = DialogBox(hInst, MAKEINTRESOURCE(IDD_NEW_GAME_3), hwnd, lpProc);
    FreeProcInstance(lpProc);
    if (mdRet == 0)
        goto Cancel;
    if (iStepMaxSoFar < 2) {
        iStepMaxSoFar = 2;
    }
    if (mdRet == 1)
        goto Step2;
    if (mdRet == 3)
        goto Finish;
Finish:
    if (fRCWReadOnly)
        goto Cancel;
    CchGetString(idsGameXy, szFile);
    if (!FGetNewGameName(szFile)) {
        if (fEasy)
            goto Cancel;
        goto Step1;
    }
    gameT = game;
    gameT.turn = 0;
    DestroyCurGame();
    game = gameT;
    strcpy(szBase, szFile);
    szBase[strlen(szBase) - 3] = 0;
    for (i = 0; i < 16 && vrgplrTypeNew[i] != 0; i++) {
        switch (vrgplrTypeNew[i] & 3) {
        case 1:
            if (vrgplrTypeNew[i] >> 2 > 6) {
                c = Random(7);
                rgplr[i] = vrgplrDef[c];
                rgplr[i].fAi = TRUE;
                rgplr[i].lvlAi = 0;
                rgplr[i].idAi = idAiMaid;
                rgplr[i].lSalt = -1;
            } else {
                c = vrgplrTypeNew[i] >> 2;
                rgplr[i] = vrgplrDef[c];
            }
            CchGetString(idsHumanoid + c, rgplr[i].szName);
            wsprintf(rgplr[i].szNames, "%ss", rgplr[i].szName);
            break;
        case 2:
            rgplr[i] = rgplrLocal[vrgplrTypeNew[i] >> 2];
            break;
        case 3:
            idAi = vrgplrTypeNew[i] >> 2 & 7;
            lvlAi = vrgplrTypeNew[i] >> 5;
            if (lvlAi >= 4) {
                lvlAi = Random(4);
            }
            if ((int16_t)idAi >= idAiRandom) {
                idAi = Random(6);
            }
            rgplr[i] = *LpplrComp(idAi, lvlAi);
            rgplr[i].fAi = TRUE;
            rgplr[i].lvlAi = lvlAi;
            rgplr[i].idAi = idAi;
        }
    }
    game.cPlayer = i;
    for (i = 0; i < game.cPlayer; i++) {
        if (!rgplr[i].fAi && CAdvantagePoints(&rgplr[i]) < 0) {
            rgplr[i] = vrgplrDef[0];
            rgplr[i].fHacker = TRUE;
        }
        if (rgplr[i].szName[0] == 0) {
            CchGetString(idsBerserker + Random(24), rgplr[i].szName);
        }
        if (rgplr[i].szNames[0] == 0) {
            wsprintf(rgplr[i].szNames, "%ss", rgplr[i].szName);
        }
    }
    for (i = 1; i < game.cPlayer; i++) {
        for (j = 0; j < i && strcmp(rgplr[i].szName, rgplr[j].szName) != 0; j++) {
        }
        if (j < i) {
            c = Random(24);
            while (1) {
                for (j = 0; j < game.cPlayer && strcmp(rgplr[j].szName, PszGetCompressedString(idsBerserker + c)) != 0; j++) {
                }
                if (j == game.cPlayer)
                    break;
                c++;
                if (c >= 24) {
                    c = 0;
                }
            }
            CchGetString(idsBerserker + c, rgplr[i].szName);
            strcpy(rgplr[i].szNames, rgplr[i].szName);
            strcat(rgplr[i].szNames, "s");
        }
    }
    for (i = 0; i < game.cPlayer; i++) {
        rgplrbmp[i] = rgplr[i].iPlrBmp;
        if (rgplrbmp[i] < 0 || rgplrbmp[i] >= 32 || rgplr[i].fAi) {
            rgplrbmp[i] = -1;
        }
    }
    for (i = 1; i < game.cPlayer; i++) {
        if (rgplrbmp[i] != -1) {
            for (j = 0; j < i && rgplrbmp[j] != rgplrbmp[i]; j++) {
            }
            if (j < i) {
                rgplrbmp[Random(2) == 0 ? j : i] = -1;
            }
        }
    }
    for (i = 0; i < game.cPlayer; i++) {
        if (rgplrbmp[i] == -1) {
            rgplrbmp[i] = Random(32);
            while (1) {
                for (j = 0; j < game.cPlayer && (j == i || rgplrbmp[i] != rgplrbmp[j]); j++) {
                }
                if (j == game.cPlayer)
                    break;
                rgplrbmp[i]++;
                if (rgplrbmp[i] >= 32) {
                    rgplrbmp[i] = 0;
                }
            }
        }
    }
    for (i = 0; i < game.cPlayer; i++) {
        rgplr[i].iPlrBmp = rgplrbmp[i];
    }
    if (!fFreeingTitle) {
        fFreeingTitle = TRUE;
        DestroyWindow(hwndTitle);
        hwndTitle = 0;
        ShowWindow(hwndFrame, SW_SHOW);
    }
    GenerateWorld(FALSE);
    if (idPlayer == iplrNone) {
        BringUpHostDlg();
    }
    gd.fNoIdleChecks = fIdleSav;
    return;
}

int16_t FGetNewGameName(char *szFileSuggest) {
    char         szXY[3];
    uint16_t     i;
    char         szFileTitle[256];
    char         szFile[256];
    char         szFilter[256];
    OPENFILENAME ofn;

    if (szFileSuggest) {
        strcpy(szFile, szFileSuggest);
    } else {
        szFile[0] = 0;
    }
    CchGetString(idsStarsGameFilesXy, szFilter);
    for (i = 0; szFilter[i] != 0; i++) {
        if (szFilter[i] == '|') {
            szFilter[i] = 0;
        }
    }
    memset(&ofn, 0, sizeof(OPENFILENAME));
    szXY[0] = 'x';
    szXY[1] = 'y';
    szXY[2] = 0;
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwndFrame;
    ofn.lpstrFilter = szFilter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = 0x100;
    ofn.lpstrFileTitle = szFileTitle;
    ofn.nMaxFileTitle = 0x100;
    ofn.lpstrInitialDir = szDirName;
    ofn.lpstrTitle = "Choose New Game Name";
    ofn.lpstrDefExt = szXY;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_NOREADONLYRETURN;
    if (GetSaveFileName(&ofn) != 0) {
        if (ofn.nFileExtension != 0) {
            strcpy(&szFile[ofn.nFileExtension], szXY);
        } else {
            strcat(szFile, szXY);
        }
        strcpy(szFileSuggest, szFile);
        return TRUE;
    }
    return FALSE;
}

INT_PTR CALLBACK SimpleNewGameDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    int16_t     i;
    RECT        rc;
    HWND        hwndDD;
    HDC         hdc;
    RECT        rcGBox;
    int16_t     dy;
    int16_t     c;
    PAINTSTRUCT ps;
    RECT       *prcSav;

    switch (message) {
    case WM_INITDIALOG:
        CheckRadioButton(hwnd, IDC_SIMPLE_NEW_GAME_EASY, IDC_SIMPLE_NEW_GAME_EXPERT, IDC_SIMPLE_NEW_GAME_STANDARD);
        CheckRadioButton(hwnd, IDC_SIMPLE_NEW_GAME_TINY, IDC_SIMPLE_NEW_GAME_HUGE, IDC_SIMPLE_NEW_GAME_SMALL);
        hwndDD = GetDlgItem(hwnd, IDC_COMBOBOX);
        SendMessage(hwndDD, CB_RESETCONTENT, 0, 0);
        for (i = 0; i < 7; i++) {
            SendMessage(hwndDD, CB_ADDSTRING, 0, (LPARAM)PszGetCompressedString(idsHumanoid + i));
        }
        SendMessage(hwndDD, CB_SETCURSEL, 0, 0);
        StickyDlgPos(hwnd, &ptStickyNewDlg, TRUE);
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
        for (i = 200; i <= 203; i++) {
            if ((HWND)lParam == GetDlgItem(hwnd, i)) {
                i = -1;
                break;
            }
        }
        if (i != -1) {
            for (i = 1000; i <= 1004; i++) {
                if ((HWND)lParam == GetDlgItem(hwnd, i)) {
                    i = -1;
                    break;
                }
            }
        }
        if (i == -1 || message == WM_CTLCOLORSTATIC) {
            SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        GetWindowRect(GetDlgItem(hwnd, IDC_SIMPLE_NEW_GAME_EASY), &rcGBox);
        ScreenToClient(hwnd, (POINT *)&rcGBox);
        GetWindowRect(GetDlgItem(hwnd, IDC_SIMPLE_NEW_GAME_EXPERT), &rc);
        ScreenToClient(hwnd, (POINT *)&rc.right);
        rcGBox.right = rc.right;
        rcGBox.bottom = rc.bottom;
        ExpandRc(&rcGBox, dyArial8, dyArial8 >> 1);
        _Draw3dFrame(hdc, &rcGBox, -1);
        SelectObject(hdc, rghfontArial8[1]);
        SetBkColor(hdc, crButtonFace);
        c = CchGetString(idsDifficultyLevel, szWork);
        TextOut(hdc, rcGBox.left + 8, rcGBox.top - (dyArial8 >> 1), szWork, c);
        GetWindowRect(GetDlgItem(hwnd, IDC_SIMPLE_NEW_GAME_TINY), &rcGBox);
        ScreenToClient(hwnd, (POINT *)&rcGBox);
        GetWindowRect(GetDlgItem(hwnd, IDC_SIMPLE_NEW_GAME_HUGE), &rc);
        ScreenToClient(hwnd, (POINT *)&rc.right);
        rcGBox.right = rc.right;
        rcGBox.bottom = rc.bottom;
        ExpandRc(&rcGBox, dyArial8, dyArial8 >> 1);
        _Draw3dFrame(hdc, &rcGBox, -1);
        SelectObject(hdc, rghfontArial8[1]);
        c = CchGetString(idsUniverseSize, szWork);
        TextOut(hdc, rcGBox.left + 8, rcGBox.top - (dyArial8 >> 1), szWork, c);
        rcGBox.top = rcGBox.bottom + 8;
        GetWindowRect(GetDlgItem(hwnd, IDC_COMBOBOX), &rcGBox);
        MapWindowPoints(NULL, hwnd, (POINT *)&rcGBox, 2);
        ExpandRc(&rcGBox, dyArial8, dyArial8 >> 1);
        rcGBox.bottom += dyArial8 * 2;
        _Draw3dFrame(hdc, &rcGBox, -1);
        SelectObject(hdc, rghfontArial8[1]);
        c = CchGetString(idsPlayerRace, szWork);
        TextOut(hdc, rcGBox.left + 8, rcGBox.top - (dyArial8 >> 1), szWork, c);
        rcGBox.top = 3 * dyArial8 + rcGBox.bottom;
        rcGBox.bottom = 1000;
        ExpandRc(&rcGBox, -dyArial8, 0);
        c = CchGetString(idsButtonAllowsConfigureMultiPlayerGamesCustom, szWork);
        dy = DrawText(hdc, szWork, c, &rcGBox, DT_WORDBREAK | DT_NOPREFIX);
        SetWindowPos(GetDlgItem(hwnd, IDC_SIMPLE_NEW_GAME_ADVANCED), NULL, rcGBox.left, rcGBox.top + dy + dyArial8 / 2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        rcGBox.bottom = rcGBox.top + dy + dyArial8 * 2;
        ExpandRc(&rcGBox, dyArial8, dyArial8 >> 1);
        _Draw3dFrame(hdc, &rcGBox, -2);
        c = CchGetString(idsAdvancedGame, szWork);
        TextOut(hdc, rcGBox.left + 8, rcGBox.top - (dyArial8 >> 1), szWork, c);
        EndPaint(hwnd, &ps);
        return 1;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_FINISH:
        case IDCANCEL:
        case IDC_SIMPLE_NEW_GAME_ADVANCED:
            for (i = 200; i <= 203 && IsDlgButtonChecked(hwnd, i) == 0; i++) {
            }
            game.mdDensity = i - 200;
            for (i = 1000; i <= 1004 && IsDlgButtonChecked(hwnd, i) == 0; i++) {
            }
            game.mdSize = i - 1000;
            game.turn = LOWORD(SendMessage(GetDlgItem(hwnd, IDC_COMBOBOX), CB_GETCURSEL, 0, 0));
            StickyDlgPos(hwnd, &ptStickyNewDlg, FALSE);
            EndDialog(hwnd, LOWORD(wParam));
            return 1;
        case IDC_SIMPLE_NEW_GAME_TUTORIAL:
            StickyDlgPos(hwnd, &ptStickyNewDlg, FALSE);
            EndDialog(hwnd, LOWORD(wParam));
            return 1;
        case IDC_SIMPLE_NEW_GAME_CUSTOMIZE_RACE:
            hwndDD = GetDlgItem(hwnd, IDC_COMBOBOX);
            game.turn = LOWORD(SendMessage(hwndDD, CB_GETCURSEL, 0, 0));
            if (game.turn < 7) {
                vplr = vrgplrDef[game.turn];
                CchGetString(idsHumanoid + game.turn, vplr.szName);
                wsprintf(vplr.szNames, "%ss", vplr.szName);
            } else {
                vplr = *vrgplrNew;
            }
            prcSav = vrgrcRCW;
            if (RaceCreationWizard(hwnd, FALSE, TRUE) != 0) {
                if (SendMessage(hwndDD, CB_GETCOUNT, 0, 0) > 7) {
                    SendMessage(hwndDD, CB_DELETESTRING, 7, 0);
                }
                SendMessage(hwndDD, CB_ADDSTRING, 0, (LPARAM)vplr.szName);
                SendMessage(hwndDD, CB_SETCURSEL, 7, 0);
                *vrgplrNew = vplr;
            }
            vrgrcRCW = prcSav;
            SetFocus(hwnd);
            break;
        case IDC_HELP:
            WinHelp(hwnd, szHelpFile, HELP_CONTEXT, idhNewGameSetupBasic);
            return 1;
        }
        break;
    }
    return 0;
}

INT_PTR CALLBACK NewGameDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    int16_t     i;
    RECT        rc;
    HDC         hdc;
    RECT        rcGBox;
    int16_t     c;
    PAINTSTRUCT ps;
    int16_t     iRet;

    switch (message) {
    case WM_INITDIALOG:
        SetNGWTitle(hwnd, 1);
        CheckRadioButton(hwnd, IDC_NEW_GAME_TINY, IDC_NEW_GAME_HUGE, game.mdSize + 1000);
        CheckRadioButton(hwnd, IDC_NEW_GAME_SPARSE, IDC_NEW_GAME_PACKED, game.mdDensity + 1005);
        CheckRadioButton(hwnd, IDC_NEW_GAME_CLOSE, IDC_NEW_GAME_DISTANT, game.mdStartDist + 1009);
        SetWindowText(GetDlgItem(hwnd, IDC_NEW_GAME_NAME), game.szName);
        SendDlgItemMessage(hwnd, IDC_NEW_GAME_NAME, EM_LIMITTEXT, 0x1f, 0);
        SendMessage(GetDlgItem(hwnd, IDC_NEW_GAME_MAX_MINERALS), BM_SETCHECK, game.fExtraFuel, 0);
        SendMessage(GetDlgItem(hwnd, IDC_NEW_GAME_SLOWER_TECH), BM_SETCHECK, game.fSlowTech, 0);
        SendMessage(GetDlgItem(hwnd, IDC_NEW_GAME_ACCELERATED_BBS), BM_SETCHECK, game.fBBSPlay, 0);
        SendMessage(GetDlgItem(hwnd, IDC_NEW_GAME_NO_RANDOM_EVENTS), BM_SETCHECK, game.fNoRandom, 0);
        SendMessage(GetDlgItem(hwnd, IDC_NEW_GAME_AI_ALLIANCES), BM_SETCHECK, game.fAisBand, 0);
        SendMessage(GetDlgItem(hwnd, IDC_NEW_GAME_PUBLIC_SCORES), BM_SETCHECK, game.fVisScores, 0);
        SendMessage(GetDlgItem(hwnd, IDC_NEW_GAME_GALAXY_CLUMPING), BM_SETCHECK, game.fClumping, 0);
        if (fRCWReadOnly) {
            for (i = 1000; i <= 1004; i++) {
                EnableWindow(GetDlgItem(hwnd, i), FALSE);
            }
            for (i = 1005; i <= 1008; i++) {
                EnableWindow(GetDlgItem(hwnd, i), FALSE);
            }
            for (i = 1009; i <= 1012; i++) {
                EnableWindow(GetDlgItem(hwnd, i), FALSE);
            }
            EnableWindow(GetDlgItem(hwnd, IDC_NEW_GAME_MAX_MINERALS), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_NEW_GAME_SLOWER_TECH), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_NEW_GAME_ACCELERATED_BBS), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_NEW_GAME_NO_RANDOM_EVENTS), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_NEW_GAME_GALAXY_CLUMPING), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_NEW_GAME_AI_ALLIANCES), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_NEW_GAME_NAME), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_NEW_GAME_PUBLIC_SCORES), FALSE);
        }
        StickyDlgPos(hwnd, &ptStickyNewDlg, TRUE);
        return 1;
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        for (i = 1000; i <= 1021 && (HWND)lParam != GetDlgItem(hwnd, i); i++) {
        }
        if (i <= 1021 || message == WM_CTLCOLORSTATIC || (HWND)lParam == GetDlgItem(hwnd, IDC_NEW_GAME_GALAXY_CLUMPING)) {
            SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case WM_ERASEBKGND:
        GetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        GetWindowRect(GetDlgItem(hwnd, IDC_NEW_GAME_TINY), &rcGBox);
        ScreenToClient(hwnd, (POINT *)&rcGBox);
        GetWindowRect(GetDlgItem(hwnd, IDC_NEW_GAME_HUGE), &rc);
        ScreenToClient(hwnd, (POINT *)&rc.right);
        rcGBox.right = rc.right;
        rcGBox.bottom = rc.bottom;
        ExpandRc(&rcGBox, dyArial8, dyArial8 >> 1);
        _Draw3dFrame(hdc, &rcGBox, -1);
        SelectObject(hdc, rghfontArial8[1]);
        SetBkColor(hdc, crButtonFace);
        c = CchGetString(idsUniverseSize, szWork);
        TextOut(hdc, rcGBox.left + 8, rcGBox.top - (dyArial8 >> 1), szWork, c);
        GetWindowRect(GetDlgItem(hwnd, IDC_NEW_GAME_SPARSE), &rcGBox);
        ScreenToClient(hwnd, (POINT *)&rcGBox);
        GetWindowRect(GetDlgItem(hwnd, IDC_NEW_GAME_PACKED), &rc);
        ScreenToClient(hwnd, (POINT *)&rc.right);
        rcGBox.right = rc.right;
        rcGBox.bottom = rc.bottom;
        ExpandRc(&rcGBox, dyArial8, dyArial8 >> 1);
        _Draw3dFrame(hdc, &rcGBox, -1);
        SelectObject(hdc, rghfontArial8[1]);
        SetBkColor(hdc, crButtonFace);
        c = CchGetString(idsDensity, szWork);
        TextOut(hdc, rcGBox.left + 8, rcGBox.top - (dyArial8 >> 1), szWork, c);
        GetWindowRect(GetDlgItem(hwnd, IDC_NEW_GAME_CLOSE), &rcGBox);
        ScreenToClient(hwnd, (POINT *)&rcGBox);
        GetWindowRect(GetDlgItem(hwnd, IDC_NEW_GAME_DISTANT), &rc);
        ScreenToClient(hwnd, (POINT *)&rc.right);
        rcGBox.right = rc.right;
        rcGBox.bottom = rc.bottom;
        ExpandRc(&rcGBox, dyArial8, dyArial8 >> 1);
        _Draw3dFrame(hdc, &rcGBox, -1);
        SelectObject(hdc, rghfontArial8[1]);
        SetBkColor(hdc, crButtonFace);
        c = CchGetString(idsPlayerPositions, szWork);
        TextOut(hdc, rcGBox.left + 8, rcGBox.top - (dyArial8 >> 1), szWork, c);
        EndPaint(hwnd, &ps);
        return 1;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_HELP) {
            WinHelp(hwnd, szHelpFile, HELP_CONTEXT, idhStep1SpecifyingTheUniverse);
            return 1;
        }
        for (iRet = 0; iRet < 4 && LOWORD(wParam) != rgidRaceBtn[iRet]; iRet++) {
        }
        if (iRet < 4) {
            if (iRet != 0) {
                for (i = 1000; i <= 1004 && IsDlgButtonChecked(hwnd, i) == 0; i++) {
                }
                game.mdSize = i - 1000;
                for (i = 1005; i <= 1008 && IsDlgButtonChecked(hwnd, i) == 0; i++) {
                }
                game.mdDensity = i - 1005;
                for (i = 1009; i <= 1012 && IsDlgButtonChecked(hwnd, i) == 0; i++) {
                }
                game.mdStartDist = i - 1009;
                game.fExtraFuel = IsDlgButtonChecked(hwnd, IDC_NEW_GAME_MAX_MINERALS);
                game.fSlowTech = IsDlgButtonChecked(hwnd, IDC_NEW_GAME_SLOWER_TECH);
                game.fBBSPlay = IsDlgButtonChecked(hwnd, IDC_NEW_GAME_ACCELERATED_BBS);
                game.fNoRandom = IsDlgButtonChecked(hwnd, IDC_NEW_GAME_NO_RANDOM_EVENTS);
                game.fAisBand = IsDlgButtonChecked(hwnd, IDC_NEW_GAME_AI_ALLIANCES);
                game.fVisScores = IsDlgButtonChecked(hwnd, IDC_NEW_GAME_PUBLIC_SCORES);
                game.fClumping = IsDlgButtonChecked(hwnd, IDC_NEW_GAME_GALAXY_CLUMPING);
                i = GetWindowText(GetDlgItem(hwnd, IDC_NEW_GAME_NAME), game.szName, 32);
                if (i == 0) {
                    strncpy(game.szName, szBase, sizeof(game.szName) - 1);
                    game.szName[sizeof(game.szName) - 1] = 0;
                }
            }
            StickyDlgPos(hwnd, &ptStickyNewDlg, FALSE);
            EndDialog(hwnd, iRet);
            return 1;
        }
        break;
    }
    return 0;
}

INT_PTR CALLBACK NewGameDlg2(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    int16_t     i;
    RECT        rc;
    RECT        rcT;
    int16_t     dyBut;
    int16_t     dy;
    int16_t     dyCur;
    HWND        hwndBtn;
    POINT16     pt;
    int16_t     iDiamond;
    int16_t     iNewVal;
    int16_t     j;
    char       *psz;
    int16_t     tpm;
    int16_t     iChecked;
    HMENU       rghmenuSubPopup[14];
    HMENU       hmenuPopup;
    MSG         msg;
    int16_t     iCurVal;
    RECT       *prcSav;
    HDC         hdc;
    PAINTSTRUCT ps;

    switch (message) {
    case WM_INITDIALOG:
        SetNGWTitle(hwnd, 2);
        dy = (dyArial8 + 4) * 16 + 8;
        GetWindowRect(GetDlgItem(hwnd, rgidRaceBtn[0]), &rc);
        dyBut = rc.bottom - rc.top;
        GetWindowRect(hwnd, &rcT);
        dyCur = rcT.bottom - rcT.top;
        if (dyCur < dy + dyBut + 6) {
            for (i = 0; i < 5; i++) {
                hwndBtn = GetDlgItem(hwnd, rgidRaceBtn[i]);
                GetWindowRect(hwndBtn, &rc);
                MapWindowPoints(NULL, hwnd, (POINT *)&rc, 2);
                OffsetRect(&rc, 0, dy - rc.top);
                SetWindowPos(hwndBtn, NULL, rc.left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            }
            dy = rc.bottom + 6;
            GetClientRect(hwnd, &rc);
            if (dyCur < dy) {
                SetWindowPos(hwnd, NULL, 0, 0, rcT.right - rcT.left, dy + rcT.bottom - rcT.top - rc.bottom, SWP_NOMOVE | SWP_NOZORDER);
            }
        }
        StickyDlgPos(hwnd, &ptStickyNewDlg, TRUE);
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
        for (i = 401; i <= 448 && (HWND)lParam != GetDlgItem(hwnd, i); i++) {
        }
        if (i <= 448 || message == WM_CTLCOLORSTATIC) {
            SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case WM_SETCURSOR:
        if (fRCWReadOnly)
            break;
        GetCursorPos16(&pt);
        ScreenToClient16(hwnd, &pt);
        if (pt.x < xNewGameDiamond || pt.x >= xNewGameDiamond + dyArial8 + 1 || pt.y < 6)
            break;
        iDiamond = (int16_t)(pt.y - 6) / (dyArial8 + 4);
        if (iDiamond >= 16 || (int16_t)(pt.y - 6) % (dyArial8 + 4) >= dyArial8 + 1)
            break;
        SetCursor(hcurHand);
        return 1;
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
        if (fRCWReadOnly)
            break;
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        if (pt.x < xNewGameDiamond || pt.x >= xNewGameDiamond + dyArial8 + 1 || pt.y < 6)
            break;
        iDiamond = (int16_t)(pt.y - 6) / (dyArial8 + 4);
        if (iDiamond >= 16 || (int16_t)(pt.y - 6) % (dyArial8 + 4) >= dyArial8 + 1)
            break;
        iCurVal = vrgplrTypeNew[iDiamond];
        ClientToScreen16(hwnd, &pt);
        rghmenuSubPopup[0] = CreatePopupMenu();
        for (i = 0; i < 6; i++) {
            iChecked = iCurVal == i * 4 + 1 ? 8 : 0;
            psz = PszGetCompressedString(idsHumanoid + i);
            AppendMenu(rghmenuSubPopup[0], iChecked, i + 15016, psz);
        }
        AppendMenu(rghmenuSubPopup[0], iChecked, i + 15016, PszGetCompressedString(idsRandom));
        AppendMenu(rghmenuSubPopup[0], iChecked, i + 15017, PszGetCompressedString(idsExpansionPlayer));
        rghmenuSubPopup[1] = CreatePopupMenu();
        AppendMenu(rghmenuSubPopup[1], MF_BYCOMMAND, 0x3a98, PszGetCompressedString(idsNew));
        AppendMenu(rghmenuSubPopup[1], MF_BYCOMMAND, 0x3a99, PszGetCompressedString(idsOpen));
        for (i = 0; i < vcplrNew; i++) {
            iChecked = iCurVal == i * 4 + 2 ? 8 : 0;
            psz = PszPlayerName(0, TRUE, TRUE, TRUE, 0, vrgplrNew + i);
            AppendMenu(rghmenuSubPopup[1], iChecked, i + 15032, psz);
        }
        rghmenuSubPopup[2] = CreatePopupMenu();
        for (i = 0; i <= 6; i++) {
            rghmenuSubPopup[i + 5] = CreatePopupMenu();
            for (j = 0; j < 5; j++) {
                iChecked = iCurVal == j * 32 + i * 4 + 5 ? 8 : 0;
                AppendMenu(rghmenuSubPopup[i + 5], iChecked, i + 15048 + j * 8, vrgszComputerLevel[j]);
            }
            iChecked = (iCurVal & 0x1f) == i * 4 + 5 ? 8 : 0;
            AppendMenu(rghmenuSubPopup[2], 0x10 | iChecked, (UINT_PTR)rghmenuSubPopup[i + 5], vrgszComputerPlayers[i]);
        }
        hmenuPopup = CreatePopupMenu();
        iPopMenuSel = -1;
        iChecked = (iCurVal & 3) == 1 ? 8 : 0;
        AppendMenu(hmenuPopup, 0x10 | iChecked, (UINT_PTR)rghmenuSubPopup[0], PszGetCompressedString(idsPredefinedRace));
        iChecked = (iCurVal & 3) == 2 ? 8 : 0;
        AppendMenu(hmenuPopup, 0x10 | iChecked, (UINT_PTR)rghmenuSubPopup[1], PszGetCompressedString(idsCustomRace));
        if ((iCurVal & 3) == 1 || (iCurVal & 3) == 2) {
            AppendMenu(hmenuPopup, MF_BYCOMMAND, 0x3a9a, PszGetCompressedString(idsEditRace));
        }
        AppendMenu(hmenuPopup, MF_SEPARATOR, 0, NULL);
        iChecked = (iCurVal & 3) == 3 ? 8 : 0;
        AppendMenu(hmenuPopup, 0x10 | iChecked, (UINT_PTR)rghmenuSubPopup[2], PszGetCompressedString(idsComputerPlayer));
        AppendMenu(hmenuPopup, MF_SEPARATOR, 0, NULL);
        iChecked = !(iCurVal & 3) ? 8 : 0;
        AppendMenu(hmenuPopup, iChecked, 0x3a9b, PszGetCompressedString(idsPlayer));
        tpm = message == WM_LBUTTONDOWN ? TPM_LEFTBUTTON : TPM_RIGHTBUTTON;
        TrackPopupMenu(hmenuPopup, TPM_CENTERALIGN | tpm, pt.x, pt.y, 0, hwnd, NULL);
        DestroyMenu(hmenuPopup);
        for (i = 0; i < 6; i++) {
            DestroyMenu(rghmenuSubPopup[i]);
        }
        if (PeekMessage(&msg, hwnd, 273, 273, 2) != 0 && msg.wParam >= 15000 && msg.wParam < 15100) {
            iPopMenuSel = msg.wParam - 15000;
        }
        iNewVal = -1;
        if (iPopMenuSel != -1) {
            if (iPopMenuSel == 0) {
                vplr = vrgplrDef[0];
                CchGetString(idsHumanoid, vplr.szName);
                wsprintf(vplr.szNames, "%ss", vplr.szName);
                prcSav = vrgrcRCW;
                if (RaceCreationWizard(hwnd, FALSE, FALSE) != 0) {
                    vrgrcRCW = prcSav;
                    goto PlaceNew;
                }
                vrgrcRCW = prcSav;
            }
            if (iPopMenuSel == 1) {
                if (FOpenGame(hwnd, TRUE) > 0) {
                PlaceNew:
                    if (vcplrNew < 16) {
                        iNewVal = vcplrNew++;
                    } else {
                        for (iNewVal = 15; iNewVal >= 0; iNewVal--) {
                            for (i = 0; i < 16 && ((vrgplrTypeNew[i] & 3) != 2 || (vrgplrTypeNew[i] >> 2 & 0xf) != iNewVal); i++) {
                            }
                            if (i == 16)
                                break;
                        }
                    }
                    vrgplrNew[iNewVal] = vplr;
                    strcpy(vrgszFileNew + iNewVal * sizeof(szRaceFile), szRaceFile);
                    iNewVal = iNewVal << 2 | 2;
                }
            } else if (iPopMenuSel == 2) {
                if ((iCurVal & 3) == 1) {
                    vplr = vrgplrDef[iCurVal >> 2];
                    CchGetString((iCurVal >> 2) + 0x567, vplr.szName);
                    wsprintf(vplr.szNames, "%ss", vplr.szName);
                } else {
                    vplr = vrgplrNew[iCurVal >> 2];
                    strcpy(szRaceFile, vrgszFileNew + (iCurVal >> 2) * sizeof(szRaceFile));
                }
                lSaltCur = vplr.lSalt;
                lSaltLast = 0;
                if (!FCheckPassword())
                    goto FinishClick;
                if (vplr.lSalt != 0) {
                    strcpy(szRacePass, szPassLast);
                } else {
                    szRacePass[0] = 0;
                }
                prcSav = vrgrcRCW;
                if (RaceCreationWizard(hwnd, FALSE, FALSE) != 0) {
                    vrgrcRCW = prcSav;
                    if ((iCurVal & 3) == 1 || strcmp(szRaceFile, vrgszFileNew + (iCurVal >> 2) * sizeof(szRaceFile)) != 0)
                        goto PlaceNew;
                    vrgplrNew[iCurVal >> 2] = vplr;
                    strcpy(vrgszFileNew + (iCurVal >> 2) * sizeof(szRaceFile), szRaceFile);
                    iNewVal = iCurVal;
                    iCurVal = -1;
                }
                vrgrcRCW = prcSav;
            } else if (iPopMenuSel == 3) {
                iNewVal = 0;
            } else if (iPopMenuSel >= 16 && iPopMenuSel < 32) {
                iNewVal = (iPopMenuSel - 0x10) << 2 | 1;
            } else if (iPopMenuSel >= 32 && iPopMenuSel < 48) {
                iNewVal = (iPopMenuSel - 0x20) << 2 | 2;
            } else if (iPopMenuSel >= 48) {
                iNewVal = (iPopMenuSel - 0x30) << 2 | 3;
            }
        }
        if (iNewVal != -1 && iNewVal != iCurVal) {
            vrgplrTypeNew[iDiamond] = iNewVal;
            if (iNewVal == 0) {
                while (++iDiamond < 16) {
                    vrgplrTypeNew[iDiamond] = 0;
                }
            } else if (iDiamond > 0) {
                while (--iDiamond >= 0 && vrgplrTypeNew[iDiamond] == 0) {
                    vrgplrTypeNew[iDiamond] = iNewVal;
                }
            }
            InvalidateRect(hwnd, NULL, TRUE);
        }
    FinishClick:
        SetFocus(hwnd);
        break;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        DrawNewGame2(hwnd, hdc, -1);
        EndPaint(hwnd, &ps);
        return 1;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_HELP) {
            WinHelp(hwnd, szHelpFile, HELP_CONTEXT, idhStep2SpecifyingThePlayers);
            return 1;
        }
        for (i = 0; i < 4 && LOWORD(wParam) != rgidRaceBtn[i]; i++) {
        }
        if (i < 4) {
            if (i != 0 && vrgplrTypeNew[0] == 0 && !fRCWReadOnly) {
                AlertSz(PszFormatIds(idsMustHaveLeastOnePlayerGame, NULL), MB_ICONHAND);
            } else {
                StickyDlgPos(hwnd, &ptStickyNewDlg, FALSE);
                EndDialog(hwnd, i);
                return 1;
            }
        }
    }
    return 0;
}

void DrawNewGame2(HWND hwnd, HDC hdc, int16_t iDraw) {
    int16_t  fCreatedDC;
    int16_t  yCur;
    int16_t  i;
    int16_t  iPlr;
    int16_t  bkMode;
    int16_t  cch;
    RECT     rcDiamond;
    RECT     rc;
    StringId ids;
    char     szT[20];

    fCreatedDC = FALSE;
    if (!hdc) {
        fCreatedDC = TRUE;
        hdc = GetDC(hwnd);
    }
    GetClientRect(hwnd, &rc);
    bkMode = SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, rghfontArial8[1]);
    xNewGameDiamond = LOWORD(GetTextExtent(hdc, PszGetCompressedString(idsPlayer16), 11)) + 12;
    yCur = 6;
    SetRect(&rcDiamond, xNewGameDiamond, yCur, xNewGameDiamond + dyArial8 + 1, yCur + dyArial8 + 1);
    for (i = 0; i < 16 && (!fRCWReadOnly || i < game.cPlayer); i++) {
        cch = wsprintf(szWork, PszGetCompressedString(idsPlayerD), i + 1);
        szWork[cch++] = ':';
        szWork[cch] = 0;
        RightTextOut(hdc, xNewGameDiamond - 6, yCur, szWork, cch, 0);
        DrawDiamond(hdc, &rcDiamond, hbrBBlue);
        iPlr = vrgplrTypeNew[i] >> 2 & 0xf;
        if (fRCWReadOnly) {
            if (rgplr[i].fInclude) {
                PszPlayerName(i, TRUE, TRUE, TRUE, 0, NULL);
                if (!rgplr[i].fDead)
                    goto DisplayName;
                wsprintf(&szWork[strlen(szWork)], " (%s)", PszGetCompressedString(idsDeceased));
                SetTextColor(hdc, 127);
                goto DisplayName;
            }
            CchGetString(idsUnknownPlayer, szWork);
            goto DisplayName;
        }
        switch (vrgplrTypeNew[i] & 3) {
        case 0:
            CchGetString(idsPlayer2, szWork);
            break;
        case 1:
            if (iPlr >= 6) {
                ids = iPlr == 6 ? (fRCWReadOnly ? idsUnknownPlayer : idsRandom) : idsExpansionPlayer;
                CchGetString(ids, szWork);
                break;
            }
            CchGetString(idsS, szT);
            wsprintf(szWork, szT, PszGetCompressedString(idsHumanoid + iPlr));
            break;
        case 2:
            if (!fRCWReadOnly && gd.fNoHostNames && vrgszFileNew[iPlr * sizeof(szRaceFile)] != 0) {
                wsprintf(szWork, " %s", vrgszFileNew + iPlr * sizeof(szRaceFile));
                break;
            }
            if (vrgszFileNew[iPlr * sizeof(szRaceFile)] != 0) {
                wsprintf(szWork, PszGetCompressedString(!fRCWReadOnly ? idsSS2 : idsS), vrgplrNew[iPlr].szNames, vrgszFileNew + iPlr * sizeof(szRaceFile));
                break;
            }
            wsprintf(szWork, PszGetCompressedString(idsS), vrgplrNew[iPlr].szNames);
            break;
        case 3:
            wsprintf(szWork, PszGetCompressedString(idsSSComputerPlayer), vrgszComputerPlayers[iPlr & 7], vrgszComputerLevel[vrgplrTypeNew[i] >> 5]);
        }
    DisplayName:
        TextOut(hdc, rcDiamond.right + 6, yCur, szWork, strlen(szWork));
        SetTextColor(hdc, crButtonText);
        OffsetRect(&rcDiamond, 0, dyArial8 + 4);
        yCur += dyArial8 + 4;
    }
    SetBkMode(hdc, bkMode);
    if (fCreatedDC) {
        ReleaseDC(hwnd, hdc);
    }
    return;
}

INT_PTR CALLBACK NewGameDlg3(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    int16_t     i;
    RECT        rc;
    POINT16     pt;
    HDC         hdc;
    PAINTSTRUCT ps;

    switch (message) {
    case WM_INITDIALOG:
        SetNGWTitle(hwnd, 3);
        for (i = 0; i < 7; i++) {
            SendMessage(GetDlgItem(hwnd, i + 291), BM_SETCHECK, GetVCCheck(&game, (i >= 2) + i), 0);
            if (fRCWReadOnly) {
                EnableWindow(GetDlgItem(hwnd, i + 291), FALSE);
            }
        }
        StickyDlgPos(hwnd, &ptStickyNewDlg, TRUE);
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
        for (i = 291; i <= 297 && (HWND)lParam != GetDlgItem(hwnd, i); i++) {
        }
        if (i <= 297 || message == WM_CTLCOLORSTATIC) {
            SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case WM_SETCURSOR:
        GetCursorPos16(&pt);
        ScreenToClient16(hwnd, &pt);
        if (IrcRaceDlgHitTest(pt) < 0)
            break;
        SetCursor(hcurHand);
        return 1;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        return FTrackNewGameDlg3(hwnd, pt, wParam);
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        DrawNewGame3(hwnd, hdc, -1);
        EndPaint(hwnd, &ps);
        return 1;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_HELP) {
            WinHelp(hwnd, szHelpFile, HELP_CONTEXT, idhStep3VictoryConditions);
            return 1;
        }
        for (i = 0; i < 4 && LOWORD(wParam) != rgidRaceBtn[i]; i++) {
        }
        if (i < 4) {
            StickyDlgPos(hwnd, &ptStickyNewDlg, FALSE);
            EndDialog(hwnd, i);
            return 1;
        }
        if (LOWORD(wParam) >= IDC_VC_OWNS_PLANETS && LOWORD(wParam) <= IDC_VC_HIGHEST_SCORE) {
            i = LOWORD(SendMessage(GetDlgItem(hwnd, LOWORD(wParam)), BM_GETCHECK, 0, 0));
            SetVCCheck(&game, LOWORD(wParam) - 291 + ((uint16_t)(LOWORD(wParam) - 291) >= 2), i);
            DrawNewGame3(hwnd, NULL, 8);
        }
    }
    return 0;
}

void DrawNewGame3(HWND hwnd, HDC hdc, int16_t iDraw) {
    int16_t  yTop;
    int16_t  bt;
    int16_t  vcCur;
    int16_t  irc;
    StringId ids;
    int16_t  fCreatedDC;
    int16_t  i;
    int16_t  dxItem;
    RECT     rcCBox;
    int16_t  j;
    COLORREF crBkSav;
    int16_t  bkMode;
    int16_t  dxDig;
    int16_t  xLeft;
    int16_t  cch;
    RECT     rc;

    fCreatedDC = FALSE;
    bt = !fRCWReadOnly ? 0 : 4;
    if (!hdc) {
        fCreatedDC = TRUE;
        hdc = GetDC(hwnd);
    }
    GetClientRect(hwnd, &rc);
    bkMode = SetBkMode(hdc, OPAQUE);
    crBkSav = SetBkColor(hdc, crButtonFace);
    SelectObject(hdc, rghfontArial8[1]);
    yTop = 3 * dyArial8 + 6;
    dxDig = LOWORD(GetTextExtent(hdc, "9", 1));
    ids = idsOwns;
    irc = 0;
    vcCur = 0;
    for (i = 0; i < 9; i++) {
        if (i < 7) {
            GetWindowRect(GetDlgItem(hwnd, i + 291), &rcCBox);
            MapWindowPoints(NULL, hwnd, (POINT *)&rcCBox, 2);
            xLeft = rcCBox.right + 2;
            yTop = (int16_t)(rcCBox.bottom - rcCBox.top) / 2 + rcCBox.top - (dyArial8 >> 1);
        } else if (i == 7) {
            xLeft = rcCBox.left;
            yTop = rcCBox.bottom + 6;
        } else {
            xLeft = rcCBox.left;
            yTop = rcCBox.bottom + 6 + (int16_t)(3 * dyArial8) / 2;
        }
        cch = CchGetString(ids++, szWork);
        if (iDraw == -1) {
            TextOut(hdc, xLeft, yTop, szWork, cch);
        }
        xLeft += LOWORD(GetTextExtent(hdc, szWork, cch));
        for (j = 0; j < 2; j++) {
            dxItem = abs(rgNG3Width[i][j]) * dxDig;
            if (dxItem == 0) {
                ids++;
                break;
            }
            wsprintf(szWork, PCTD, GetVCVal(&game, vcCur, FALSE));
            if (rgNG3Width[i][j] < 0) {
                dxItem += (int16_t)(3 * dxDig) / 2;
                strcat(szWork, "%");
            }
            xLeft += dxItem;
            if (iDraw == -1 || iDraw == vcCur || vcCur == 8) {
                RightTextOut(hdc, xLeft, yTop, szWork, 0, dxItem);
            }
            vcCur++;
            vrgrcRCW[irc].left = xLeft + 4;
            vrgrcRCW[irc].top = yTop - 3;
            vrgrcRCW[irc].right = vrgrcRCW[irc].left + 15;
            vrgrcRCW[irc].bottom = (dyArial8 >> 1) + vrgrcRCW[irc].top + 3;
            vrgrcRCW[irc + 1] = vrgrcRCW[irc];
            OffsetRect((RECT *)&vrgrcRCW[irc + 1].left, 0, vrgrcRCW[irc].bottom - vrgrcRCW[irc].top - 1);
            if (iDraw == -1) {
                DrawBtn(hdc, vrgrcRCW + irc, 0xa0 | bt, FALSE, NULL);
                DrawBtn(hdc, vrgrcRCW + (irc + 1), 0xa1 | bt, FALSE, NULL);
            }
            xLeft = vrgrcRCW[irc].right + 4;
            cch = CchGetString(ids, szWork);
            if (iDraw == -1) {
                TextOut(hdc, xLeft, yTop, szWork, cch);
            }
            xLeft += LOWORD(GetTextExtent(hdc, szWork, cch));
            ids++;
            irc += 2;
        }
    }
    crcRCW = irc;
    SetBkColor(hdc, crBkSav);
    SetBkMode(hdc, bkMode);
    if (fCreatedDC) {
        ReleaseDC(hwnd, hdc);
    }
    return;
}

int16_t FTrackNewGameDlg3(HWND hwnd, POINT16 pt, int16_t kbd) {
    int16_t bt;
    int16_t irc;
    BTNT    btnt;
    int16_t i;
    int16_t dShift;
    int16_t iMod;
    int16_t iStat;

    irc = IrcRaceDlgHitTest(pt);
    if (irc < 0) {
        return FALSE;
    }
    iMod = irc & 1;
    i = irc >> 1;
    if (iMod == 0) {
        dShift = 1;
        bt = 160;
    } else {
        dShift = -1;
        bt = 161;
    }
    InitBtnTrack(&btnt, hwnd, NULL, vrgrcRCW + irc, bt, 80, FALSE, FALSE, NULL);
    if (kbd & 0xc) {
        dShift = 5 * dShift;
    }
    while (FTrackBtn(&btnt)) {
        iStat = GetVCVal(&game, i, TRUE);
        if (SetVCVal(&game, i, iStat + dShift) != iStat) {
            DrawNewGame3(hwnd, btnt.hdc, i);
        }
    }
    return TRUE;
}

void SetNGWTitle(HWND hwnd, int16_t iStep) {
    int16_t cch;
    char    szBuf[50];

    cch = CchGetString(idsAdvancedNewGameWizardStepD3 + fRCWReadOnly, szBuf);
    cch = wsprintf(szWork, szBuf, iStep);
    SetWindowText(hwnd, szWork);
    return;
}
