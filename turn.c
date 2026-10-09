#include "common.h"

int16_t rgpctMineHit[3] = {3, 10, 35};
int16_t rgiWarpSafe[3] = {4, 6, 5};
int16_t rgrgdmgMinMine[3][2] = {{500, 600}, {2000, 2500}};
int16_t rgrgdmgMine[3][2] = {{100, 125}, {500, 600}};

// InitGameStuff sets up what the game code needs before it loads a game:
// the log and message buffers, the report id lists and the default
// production template, which ReadIniSettings also sets when stars.ini has
// none. FCreateStuff calls it for the Windows game.
void InitGameStuff() {
    gd.fNoIdleChecks = FALSE;
    gd.fAisDone = FALSE;
    vplr = vrgplrDef[0];
    lpLog = LpAlloc(32000, htLog);
    lpMsg = LpAlloc(0xffc8, htMsg);
    vlprgidPlanet = LpAlloc(0x800, htPerm);
    vlprgidFleet = LpAlloc(0x800, htPerm);
    CchGetString(idsDefault, vrgZipProd[0].szName);
    vrgZipProd[0].fValid = TRUE;
    return;
}

int16_t FGenerateTurn() {
    int16_t  fErrSav;
    char    *pchT;
    int16_t  ish;
    int16_t  j;
    uint8_t  mpiplr2[16];
    uint8_t  rgfNoXFile[16];
    jmp_buf *penvMemSav;
    int16_t  ifl;
    FLEET   *lpfl;
    char    *pchCur;
    int16_t  i;
    jmp_buf  env;
    char     szT[256];
    int16_t  idCur;
    int16_t  fFollow;
    char    *pchBak;
    int16_t  fSuccess;
    int16_t  fDone;
    FLEET   *lpflTarget;
    ORDER    ord;
    int16_t  cAdv;
    PLANET  *lppl;
    PLANET  *lpplMac;
    int16_t  dPlanRange;
    int16_t  dRange;
    int16_t  iSteal;
    int16_t  pctDetect;

    idCur = idPlayer;
    fSuccess = FALSE;
    DestroyCurGame();
    if (gd.fTutorial) {
        Randomize(1234567890);
    }
    fErrSav = fFileErrSilent;
    fFileErrSilent = TRUE;
    UpdateProgressGauge(360);
    if (!FLoadGame(szBase, "hst")) {
        fFileErrSilent = fErrSav;
        TurnLog(idsCantFindHostFile);
        return FALSE;
    }
    TurnLog(idsGeneratingYearD);
    fFileErrSilent = fErrSav;
    if (((VERS *)&wVersFile)->verMajor <= 0) {
        for (i = 0; i < game.cPlayer && rgplr[i].iPlrBmp == 0; i++) {
        }
        if (i == game.cPlayer) {
            for (i = 0; i < game.cPlayer; i++) {
                rgplr[i].iPlrBmp = i;
            }
        }
    }
    penvMemSav = penvMem;
    penvMem = &env;
    if (setjmp(env) == 0) {
        lpcd = LpAlloc(1000 * sizeof(COLDROP), htMisc);
        lpxf = LpAlloc(1000 * sizeof(XFERFULL), htMisc);
        vrgPlanResExtra = LpAlloc(game.cPlanMax * 2, htMisc);
        memset(vrgPlanResExtra, 0, game.cPlanMax * 2);
        vrgwtPopScore = LpAlloc(game.cPlanMax * sizeof(int32_t), htMisc);
        vrgiplrPopScore = LpAlloc(game.cPlanMax * sizeof(int16_t), htMisc);
        memset(vrgiplrPopScore, 0xff, game.cPlanMax * sizeof(int16_t));
        UpdateProgressGauge(370);
        cColDrop = 0;
        cXferFull = 0;
        gd.fGeneratingTurn = TRUE;
        gd.fRetryOpens = TRUE;
        imemMsgCur = 0;
        for (i = 0; i < game.cPlayer; i++) {
            mpiplr2[i] = i;
        }
        for (i = 0; i < game.cPlayer; i++) {
            j = Random(game.cPlayer - i) + i;
            if (j != i) {
                idCur = mpiplr2[j];
                mpiplr2[j] = mpiplr2[i];
                mpiplr2[i] = idCur;
            }
        }
        for (i = 0; i < game.cPlayer; i++) {
            j = mpiplr2[i];
            CchSprintf(szWork, "%s.x%d", szBase, j + 1);
            idPlayer = j;
            if (FLoadLogFile(szWork) && !FRunLogFile()) {
                AlertSz(PszFormatIds(idsPlayerLogFileAppearsCorruptUnableLoad, NULL), MB_ICONHAND);
                goto FreeStuffUp;
            }
            UpdateProgressGauge(LMulDiv(60, i + 1, game.cPlayer) + 370);
        }
        idPlayer = iplrNone;
        for (i = 0; i < game.cPlayer; i++) {
            for (ish = 0; ish < 16; ish++) {
                if (!rglpshdef[i][ish].fFree && rglpshdef[i][ish].hul.rghs[0].grhst != hstEngine) {
                    rglpshdef[i][ish].hul.rghs[0].grhst = hstEngine;
                    rglpshdef[i][ish].hul.rghs[0].iItem = iengineQuickJump5;
                    if (rglpshdef[i][ish].hul.rghs[0].cItem < 1) {
                        rglpshdef[i][ish].hul.rghs[0].cItem = 1;
                    }
                }
            }
        }
        fFollow = FALSE;
        for (ifl = 0; ifl < cFleet; ifl++) {
            lpfl = rglpfl[ifl];
            if (!rglpfl[ifl])
                break;
            lpfl->fNoHeal = FALSE;
            if (lpfl->cord == 1 && lpfl->lpplord->rgord[0].grobj == grobjFleet) {
                fFollow = TRUE;
                lpfl->fMark = TRUE;
            } else {
                if (lpfl->lpplord->rgord[0].grobj == grobjFleet && lpfl->cord == 1) {
                    FSendPlrMsg(lpfl->iPlayer, idmHadOrdersFollowFleetWhichDidntMove, lpfl->id | 0x8000, lpfl->id, 0, 0, 0, 0, 0, 0);
                }
                lpfl->fMark = FALSE;
            }
        }
        ValidateWaypoints();
        if (fFollow) {
            fFollow = TRUE;
            for (i = 0; i < 8 && fFollow != 0; i++) {
                fFollow = FALSE;
                for (ifl = 0; ifl < cFleet; ifl++) {
                    lpfl = rglpfl[ifl];
                    if (!rglpfl[ifl])
                        break;
                    if (lpfl->fMark && lpfl->cord == 1) {
                        ord = lpfl->lpplord->rgord[0];
                        if (ord.grobj != grobjFleet) {
                        LUnmark:
                            lpfl->fMark = FALSE;
                            continue;
                        }
                        lpflTarget = LpflFromId(ord.id);
                        if (!lpflTarget || (lpflTarget->cord == 1 && lpflTarget->lpplord->rgord[0].grobj != grobjFleet)) {
                            FSendPlrMsg(lpfl->iPlayer, idmHadOrdersFollowFleetWhichDidntMove, lpfl->id | 0x8000, lpfl->id, 0, 0, 0, 0, 0, 0);
                            goto LUnmark;
                        }
                        if (lpflTarget->cord == 1)
                            continue;
                        fFollow = TRUE;
                        if (lpfl->lpplord->iordMax <= 1) {
                            lpfl->lpplord = (PLORD *)LpplReAlloc((PL *)lpfl->lpplord, 2);
                        }
                        lpfl->lpplord->rgord[1] = lpflTarget->lpplord->rgord[1];
                        lpfl->lpplord->rgord[1].txp = lpfl->lpplord->rgord[0].txp;
                        lpfl->cord = 2;
                        lpfl->lpplord->iordMac = 2;
                    }
                }
            }
        }
        UpdateProgressGauge(440);
        DoOrders(FALSE);
        UpdateProgressGauge(530);
        for (i = 0; i < game.cPlayer; i++) {
            for (j = 0; j < 16; j++) {
                SetRaceStat(&rgplr[i], j, GetRaceStat(&rgplr[i], j));
            }
            if (rgplr[i].pctResearch < 0 || rgplr[i].pctResearch > 100) {
                rgplr[i].pctResearch = 15;
            }
            if (rgplr[i].pctIdealGrowth < 0) {
                rgplr[i].pctIdealGrowth = 1;
            }
            if (rgplr[i].pctIdealGrowth > 20) {
                rgplr[i].pctIdealGrowth = 20;
            }
            j = rgplr[i].fHacker;
            cAdv = CAdvantagePoints(&rgplr[i]);
            if ((cAdv < 0 || j != rgplr[i].fHacker) && !rgplr[i].fAi) {
                FSendPlrMsg2(i, idmRaceDefinitionHasTamperedStatisticsHaveAltered, gotoNone, 0, 0);
                for (j = 0; j < game.cPlayer; j++) {
                    if (i != j && !rgplr[i].fAi) {
                        FSendPlrMsg2(j, idmHackedRaceDiscoveredRaceStatisticsHaveAltered, gotoNone, i, 0);
                    }
                }
                rgplr[i].fHacker = TRUE;
                if (cAdv < 500) {
                    while (rgplr[i].rgAttr[0] < 25) {
                        rgplr[i].rgAttr[0]++;
                        cAdv = CAdvantagePoints(&rgplr[i]);
                        if (cAdv >= 500)
                            break;
                    }
                }
                if (cAdv < 500) {
                    while (rgplr[i].pctIdealGrowth > 1) {
                        rgplr[i].pctIdealGrowth--;
                        cAdv = CAdvantagePoints(&rgplr[i]);
                        if (cAdv >= 500)
                            break;
                    }
                }
                if (cAdv < 500) {
                    for (j = 8; j <= 13; j++) {
                        rgplr[i].rgAttr[j] = 0;
                        cAdv = CAdvantagePoints(&rgplr[i]);
                        if (cAdv >= 500)
                            break;
                    }
                }
            }
        }
        UnmarkMineFields();
        MoveThings(FALSE);
        UpdateProgressGauge(550);
        MoveFleets();
        lppl = lpPlanets;
        lpplMac = lpPlanets + cPlanet;
        for (; lppl < lpplMac; lppl++) {
            lppl->fHomeworld = FALSE;
        }
        for (i = 0; i < game.cPlayer; i++) {
            lpPlanets[rgplr[i].idPlanetHome].fHomeworld = TRUE;
        }
        UpdateProgressGauge(650);
        ThingDecay();
        BreedColonistsInTransit();
        UpdateProgressGauge(700);
        Produce();
        UpdateProgressGauge(750);
        MoveThings(TRUE);
        UpdateProgressGauge(770);
        FuelFleets();
        DoOrders(TRUE);
        SweepForMines();
        HealShips();
        AutoTerraform();
        RemoteTerraforming();
        UpdateProgressGauge(850);
        ValidateWaypoints();
        UpdateGuesses();
        UpdateProgressGauge(852);
        FMarkFile(dtHost, iplrNone, mdMarkInUse, FALSE);
        CreateBackupDir();
        game.turn++;
        pchCur = &szBase[strlen(szBase)];
        pchT = strrchr(szBase, chDirSep);
        strcpy(szT, szBackup);
        if (!pchT) {
            strcat(szT, szBase);
        } else {
            strcat(szT, pchT + 1);
        }
        pchBak = &szT[strlen(szT)];
        UpdateProgressGauge(854);
        UpdatePlayerScores();
        for (i = 0; i < game.cPlayer; i++) {
            for (j = 0; j < 10; j++) {
                if (!rglpshdefSB[i][j].fFree) {
                    rglpshdefSB[i][j].lVisible = (int16_t)(100 - PctCloakFromHuldef(&rglpshdefSB[i][j].hul, i, NULL));
                    rglpshdefSB[i][j].lVisible = (uint32_t)(rglpshdefSB[i][j].lVisible * rglpshdefSB[i][j].lVisible);
                }
            }
            for (j = 0; j < 16; j++) {
                if (!rglpshdef[i][j].fFree) {
                    dRange = GetShdefScannerRange(rglpshdef[i] + j, i, &dPlanRange, &pctDetect, &iSteal);
                    rglpshdef[i][j].dScanRange = dRange;
                    rglpshdef[i][j].dScanRange2 = dPlanRange;
                    rglpshdef[i][j].pctDetect = pctDetect;
                    rglpshdef[i][j].iSteal = iSteal;
                    if (!FCanBuildShdef(rglpshdef[i] + j, i)) {
                        rglpshdef[i][j].fGift = TRUE;
                    }
                }
            }
        }
        j = 856;
        fDone = FALSE;
        memset(rgfNoXFile, 0, 16);
        i = 0;
        while (!fDone) {
            UpdateProgressGauge(j);
            j += 17 / (game.cPlayer + 1);
            if (i >= game.cPlayer) {
                i = -1;
                fDone = TRUE;
            }
            if (i >= 0) {
                CchSprintf(pchCur, ".x%d", i + 1);
                strcpy(pchBak, pchCur);
                remove(szT);
                if (!FFileExists(szBase)) {
                    rgfNoXFile[i] = TRUE;
                } else {
                    rename(szBase, szT);
                }
                pchBak[1] = 'm';
                pchCur[1] = 'm';
            } else {
                strcpy(pchCur, ".hst");
                strcpy(pchBak, ".hst");
            }
            remove(szT);
            if (i >= 0 && rgfNoXFile[i]) {
                StarsCopyFile(szBase, szT);
            } else {
                rename(szBase, szT);
            }
            *pchCur = 0;
            i++;
        }
        j = 875;
        fDone = FALSE;
        game.wGen = (uint16_t)Random(8);
        i = 0;
        while (!fDone) {
            UpdateProgressGauge(j);
            j += 122 / (game.cPlayer + 1);
            if (i >= game.cPlayer) {
                i = -1;
                fDone = TRUE;
            }
            FWriteDataFile(szBase, i, i != -1 && rgfNoXFile[i]);
            i++;
        }
        UpdateProgressGauge(998);
        imemLogCur = 0;
        fSuccess = TRUE;
    }
FreeStuffUp:
    UpdateProgressGauge(1000);
    FreeLp(vrgPlanResExtra, htMisc);
    vrgPlanResExtra = NULL;
    FreeLp(vrgwtPopScore, htMisc);
    vrgwtPopScore = NULL;
    FreeLp(vrgiplrPopScore, htMisc);
    vrgiplrPopScore = NULL;
    FreeLp(lpcd, htMisc);
    lpcd = NULL;
    FreeLp(lpxf, htMisc);
    lpxf = NULL;
    gd.fGeneratingTurn = FALSE;
    gd.fRetryOpens = FALSE;
    idPlayer = iplrNone;
    if (fSuccess && ini.fGen) {
        vretExitValue = 1;
    }
    TurnLog(idsFailed + fSuccess);
    return fSuccess;
}

void EnsureAis() {
    int16_t fHostSav;
    int16_t fErrSav;
    int16_t fOpened;
    int16_t fWorkDone;
    int16_t fSubmitSav;
    int16_t iPlayer;
    MDPLR   rgmdplr[16];

    fSubmitSav = gd.fSubmit;
    fWorkDone = FALSE;
    if (!gd.fAisDone) {
        fHostSav = gd.fHostMode;
        if (!gd.fHostMode) {
            DestroyCurGame();
            FLoadGame(szBase, "hst");
        }
        for (iPlayer = 0; iPlayer < game.cPlayer; iPlayer++) {
            *(uint16_t *)&rgmdplr[iPlayer] = rgplr[iPlayer].wMdPlr;
        }
        gd.fSubmit = TRUE;
        fErrSav = fFileErrSilent;
        fFileErrSilent = TRUE;
        for (iPlayer = 0; iPlayer < game.cPlayer; iPlayer++) {
            UpdateProgressGauge(LMulDiv(340, iPlayer + 1, game.cPlayer));
            if (rgmdplr[iPlayer].fAi) {
                fWorkDone = TRUE;
                gd.fGeneratingTurn = TRUE;
                gd.fHostMode = TRUE;
                fOpened = FOpenFile(dtLog, iPlayer, 32);
                gd.fGeneratingTurn = FALSE;
                gd.fHostMode = fHostSav;
                if (fOpened) {
                    StreamClose();
                } else {
                    DoAiTurn(iPlayer, *(uint16_t *)&rgmdplr[iPlayer]);
                }
            }
        }
        gd.fSubmit = fSubmitSav;
        if (fWorkDone) {
            DestroyCurGame();
            FLoadGame(szBase, "hst");
        }
        fFileErrSilent = fErrSav;
        gd.fAisDone = TRUE;
    }
    return;
}

void VerifyTurns() {
    int16_t idsError;
    int16_t idCur;
    int16_t cAi;
    int16_t i;
    int16_t cOut;
    int16_t fOut;

    idCur = idPlayer;
    cOut = 0;
    cAi = 0;
    lpcd = LpAlloc(1000 * sizeof(COLDROP), htMisc);
    lpxf = LpAlloc(1000 * sizeof(XFERFULL), htMisc);
    vrgPlanResExtra = LpAlloc(game.cPlanMax * 2, htMisc);
    memset(vrgPlanResExtra, 0, game.cPlanMax * 2);
    cColDrop = 0;
    cXferFull = 0;
    imemMsgCur = 0;
    for (i = 0; i < game.cPlayer; i++) {
        fOut = rgOut[i];
        idsError = 0;
        if (rgplr[i].fAi || FCheckLogFile(i, &idsError)) {
            if (rgplr[i].fAi) {
                cAi++;
                rgOut[i] = 0;
            } else {
                CchSprintf(szWork, "%s.x%d", szBase, i + 1);
                idPlayer = i;
                if (FLoadLogFile(szWork) && !FRunLogFile()) {
                    rgOut[i] = 3;
                } else {
                    rgOut[i] = 0;
                }
            }
        } else if (idsError != 0) {
            switch (idsError) {
            case idsFileGame:
                rgOut[i] = 5;
                break;
            case idsFileDate:
                rgOut[i] = 4;
                break;
            default:
                rgOut[i] = 3;
                break;
            }
            cOut++;
        } else if (rgplr[i].fDead) {
            rgOut[i] = -1;
        } else if (gd.fPartialTurn) {
            rgOut[i] = 2;
            cOut++;
        } else {
            rgOut[i] = 1;
            cOut++;
        }
        if (ctickLast == 0 || rgOut[i] != fOut) {
            ctickLast = DwTickCount();
        }
    }
    FreeLp(vrgPlanResExtra, htMisc);
    vrgPlanResExtra = NULL;
    FreeLp(lpcd, htMisc);
    lpcd = NULL;
    FreeLp(lpxf, htMisc);
    lpxf = NULL;
    idPlayer = idCur;
    return;
}

int16_t CTurnsOutSafe() {
    int16_t idPlayerSav;
    int16_t fHostModeSav;
    int16_t fGenSav;
    int16_t cturn;

    fHostModeSav = gd.fHostMode;
    fGenSav = gd.fGeneratingTurn;
    idPlayerSav = idPlayer;
    idPlayer = iplrNone;
    gd.fHostMode = TRUE;
    gd.fGeneratingTurn = FALSE;
    cturn = CFindTurnsOutstanding();
    gd.fGeneratingTurn = fGenSav;
    gd.fHostMode = fHostModeSav;
    idPlayer = idPlayerSav;
    return cturn;
}

int16_t CFindTurnsOutstanding() {
    int16_t idsError;
    int16_t cAi;
    int16_t i;
    int16_t cOut;
    int16_t fSav;
    int16_t fOut;

    cOut = 0;
    cAi = 0;
    fSav = fFileErrSilent;
    fFileErrSilent = TRUE;
    gd.fGeneratingTurn = TRUE;
    for (i = 0; i < game.cPlayer; i++) {
        fOut = rgOut[i];
        idsError = 0;
        if (rgplr[i].fAi || FCheckLogFile(i, &idsError)) {
            if (rgplr[i].fAi) {
                cAi++;
            }
            rgOut[i] = 0;
        } else if (idsError != 0) {
            switch (idsError) {
            case idsFileGame:
                rgOut[i] = 5;
                break;
            case idsFileDate:
                rgOut[i] = 4;
                break;
            default:
                rgOut[i] = 3;
                break;
            }
            cOut++;
        } else if (rgplr[i].fDead) {
            rgOut[i] = -1;
        } else if (gd.fPartialTurn) {
            rgOut[i] = 2;
            cOut++;
        } else {
            rgOut[i] = 1;
            cOut++;
        }
        if (ctickLast == 0 || rgOut[i] != fOut) {
            ctickLast = DwTickCount();
        }
    }
    gd.fGeneratingTurn = FALSE;
    gd.fAllAis = cAi == game.cPlayer;
    fFileErrSilent = FALSE;
    return cOut;
}

int16_t FSetUpBatchProcessing() {
    char   *pch;
    jmp_buf env;
    int16_t fSuccess;
    int16_t cb;

    fSuccess = FALSE;
    penvMem = &env;
    if (setjmp(env) != 0)
        goto LError;
    StreamOpen(szBase, mdRead);
    cb = LOWORD(CbFileSize(hf));
    lpchBatch = LpAlloc(cb, htPerm);
    RgFromStream(lpchBatch, cb);
    lpchBatchMac = lpchBatch + cb;
    pch = szBase;
    while (*lpchBatch != '\n' && lpchBatch != lpchBatchMac) {
        *pch = *lpchBatch;
        lpchBatch++;
        pch++;
    }
    lpchBatch++;
    pch[-1] = 0;
    fSuccess = TRUE;
LError:
    penvMem = 0;
    StreamClose();
    if (!fSuccess) {
        szBase[0] = 0;
    }
    return fSuccess;
}

// ParseCmdLine reads the command line's switches into ini and gd and its
// file name into szBase; *pfSeed and *plSeed get a -s<seed>. The Windows
// game and stars-host take the same command line.
void ParseCmdLine(char *lpCmdLine, int16_t *pfSeed, uint32_t *plSeed) {
    char   *pch;
    char   *lpT;
    int16_t i;

    lpT = lpCmdLine;
    while (*lpT != 0) {
        for (; *lpT == ' '; lpT++) {
        }
        /* Windows switches may start with '/'; where '/' separates
           directories, only '-' does. */
        if (*lpT == '-' || (*lpT == '/' && chDirSep != '/')) {
            for (lpT++; *lpT != 0 && *lpT != ' '; lpT++) {
                switch (*lpT) {
                case 'W':
                case 'w':
                    ini.fWait = TRUE;
                    break;
                case 'D':
                case 'd':
                    for (lpT++; *lpT != 0 && *lpT != ' '; lpT++) {
                        switch (*lpT) {
                        case 'F':
                        case 'f':
                            ini.fDumpFleets = TRUE;
                            break;
                        case 'P':
                        case 'p':
                            ini.fDumpPlanets = TRUE;
                            break;
                        case 'M':
                        case 'm':
                            ini.fDumpMap = TRUE;
                        }
                    }
                    lpT--;
                    break;
                case 'G':
                case 'g':
                    ini.fGen = TRUE;
                    i = 0;
                    while (lpT[1] >= '0' && lpT[1] <= '9') {
                        lpT++;
                        i = 10 * i + *lpT - '0';
                        if (i > 1000) {
                            i = 1000;
                            for (; lpT[1] >= '0' && lpT[1] <= '9'; lpT++) {
                            }
                            break;
                        }
                    }
                    if (i <= 0)
                        break;
                    ini.cTurnGen = i - 1;
                    break;
                case 'A':
                case 'a':
                    ini.fNewGame = TRUE;
                    break;
                case 'H':
                case 'h':
                    gd.fHotSeat = TRUE;
                    break;
                case 'X':
                case 'x':
                    gd.fExitWindows = TRUE;
                    break;
                case 'B':
                case 'b':
                    for (lpT++; *lpT == ' '; lpT++) {
                    }
                    pch = szBase;
                    for (; *lpT != 0 && *lpT != ' '; lpT++) {
                        *pch = *lpT;
                        pch++;
                    }
                    *pch = 0;
                    lpT--;
                    if (!FSetUpBatchProcessing())
                        break;
                    ini.fBatch = TRUE;
                    ini.fGen = TRUE;
                    ini.fStartupFile = TRUE;
                    ini.fCmdLine = TRUE;
                    break;
                case 'V':
                case 'v':
                    ini.fValidate = TRUE;
                    break;
                case 'L':
                case 'l':
                    ini.fLogging = TRUE;
                    break;
                case 'T':
                case 't':
                    ini.fTry = TRUE;
                    break;
                case 'C':
                case 'c':
                    ini.fCmdLine = szBase[0] != 0;
                    break;
                case 'S':
                case 's':
                    /* -s<seed>: a fixed startup seed instead of the clock, so
                       regression runs repeat exactly. Not in the original. */
                    *pfSeed = TRUE;
                    *plSeed = 0;
                    while (lpT[1] >= '0' && lpT[1] <= '9') {
                        lpT++;
                        *plSeed = 10 * *plSeed + (uint32_t)(*lpT - '0');
                    }
                    break;
                case 'P':
                case 'p':
                    for (lpT++; *lpT == ' '; lpT++) {
                    }
                    pch = szPassLast;
                    for (; *lpT != 0 && *lpT != ' ' && pch < &szPassLast[15]; lpT++) {
                        *pch = *lpT;
                        pch++;
                    }
                    *pch = 0;
                    lpT--;
                    lSaltLast = LSaltFromSz(szPassLast);
                }
            }
        } else {
            pch = szBase;
            while (*lpT != 0 && *lpT != ' ') {
                *pch = *lpT;
                lpT++;
                pch++;
            }
            *pch = 0;
            ini.fStartupFile = TRUE;
            ini.fCmdLine = TRUE;
        }
    }
    return;
}

// FRunCmdLine carries out a command line's host work: validating the host
// file (-v), creating a universe (-a), or generating turns (-g, with -b
// for a batch of games and -t to generate only when every turn is in). It
// returns FALSE when there is none to do now: the command line opens a
// game, or waits (-w) for turns still out, which the Windows game's host
// timer does.
int16_t FRunCmdLine() {
    char   *pch;
    char    szTemp[80];
    int16_t ich;
    int16_t i;

    if (ini.fValidate) {
        fFileErrSilent = TRUE;
        ClearFile(7);
        if (FLoadGame(szBase, "hst")) {
            VerifyTurns();
            DestroyCurGame();
            EnsureAis();
            CchSprintf(szTemp, "\"%s\" Year: %d", game.szName, game.turn + 2400);
            OutputSz(7, szTemp);
            for (i = 0; i < game.cPlayer; i++) {
                if (rgOut[i] + 1 > 3) {
                    ich = CchSprintf(szTemp, "Error: %d: ", i + 1);
                } else {
                    ich = CchSprintf(szTemp, "%d: ", i + 1);
                }
                if (!gd.fNoHostNames) {
                    ich += CchSprintf(&szTemp[ich], "\"%s\" ", PszPlayerName(i, TRUE, TRUE, TRUE, 0, NULL));
                }
                strcat(szTemp, PszGetCompressedString(idsTurned + rgOut[i]));
                if (rgplr[i].fHacker) {
                    strcat(szTemp, " - HACKER");
                }
                OutputSz(7, szTemp);
            }
        }
        return TRUE;
    }
    if (ini.fNewGame) {
        GenNewGameFromFile(szBase);
        return TRUE;
    }
    if (!ini.fGen) {
        return FALSE;
    }
LBatchNext:
    if ((!ini.fWait && !ini.fTry) || CTurnsOutSafe() == 0) {
        EnsureAis();
        FGenerateTurn();
        if (ini.fBatch && lpchBatch < lpchBatchMac) {
        LTryNextBatch:
            DestroyCurGame();
            pch = szBase;
            while (*lpchBatch != '\n' && lpchBatch != lpchBatchMac) {
                *pch = *lpchBatch;
                lpchBatch++;
                pch++;
            }
            lpchBatch++;
            pch[-1] = 0;
            ini.fStartupFile = TRUE;
            goto LBatchNext;
        }
        if (ini.cTurnGen == 0)
            return TRUE;
        ini.cTurnGen--;
        goto LBatchNext;
    }
    if (ini.fTry) {
        if (ini.fBatch && lpchBatch < lpchBatchMac)
            goto LTryNextBatch;
        return TRUE;
    }
    return FALSE;
}

void DoOrders(int16_t fPostMovement) {
    PLANET *lppl;
    PLANET *lpplMac;

    lppl = lpPlanets;
    lpplMac = lpPlanets + cPlanet;
    for (; lppl < lpplMac; lppl++) {
        lppl->fWasInhabited = lppl->iPlayer != iplrNone;
    }
    if (fPostMovement) {
        idBattle = (game.turn & 0xf) * 0x100 + 1;
        DoBattles(fPostMovement);
    }
    DoThingInteractions(fPostMovement);
    if (fPostMovement) {
        lppl = lpPlanets;
        lpplMac = lpPlanets + cPlanet;
        for (; lppl < lpplMac; lppl++) {
            lppl->turn = 0;
        }
    }
    SatisfyOrders(!fPostMovement ? 1 : 3);
    DropColonists();
    UpdateResearchStatus(FALSE);
    SatisfyOrders(!fPostMovement ? 2 : 4);
    if (!fPostMovement) {
        TransferToOthers();
    }
    return;
}

void MoveThings(int16_t fPostProd) {
    int16_t   k;
    int16_t   dUni;
    double    d;
    POINT16   pt;
    int16_t   iMax;
    POINT16   ptDst;
    int16_t   dLeft;
    THING    *lpth;
    int16_t   fAnythingMoved;
    int16_t   fMajorMove;
    MessageId idm;
    int16_t   iLow;
    POINT16   ptSrc;
    THING    *lpthMac;
    int16_t   dRange;
    POINT16   ptBase;
    int16_t   iX;
    int16_t   rgC[2];
    int16_t   rgwtTerra[3];
    int32_t   wtTot;
    int16_t   iWarp2;
    int16_t   iWarp;
    int16_t   fTerra;
    PLANET   *lppl;
    int16_t   wtCur;
    int16_t   pctMinKeep;
    int16_t   fTwoMAs;
    int32_t   lDefKilled;
    int32_t   lColKilled;
    int16_t   i;
    int16_t   pctCaught;
    float     pct;
    int32_t   dmgRaw;
    int16_t   iWarpPacket;
    int16_t   iWarpPacket2;
    THING    *lpth2;
    THING    *lpth2Mac;
    int16_t   pctRate;
    int16_t   iplr;
    int16_t   rgMin[3];
    int16_t   cTerraPerm;
    int16_t   cTerraTemp;
    int16_t   rgMax[3];
    int16_t   rgCost[3];
    double    dyRound;
    double    dxRound;
    double    r;

    fAnythingMoved = FALSE;
    lpth = lpThings;
    lpthMac = lpThings + cThing;
    for (; lpth < lpthMac; lpth++) {
        if (lpth->ith == ithWormhole && fPostProd) {
            k = 0;
            ptBase = lpth->pt;
            fMajorMove = Random(100) < PctWormholeMoves(lpth);
            if (fMajorMove) {
                lpth->thw.grbitPlr = 0;
                dUni = 400 * game.mdSize + 400;
                lpth->thw.cLastMove = 0;
            } else {
                lpth->thw.cLastMove++;
            }
            iMax = 16;
            while (k++ < 100) {
                if (fMajorMove) {
                    lpth->pt.x = Random(dUni) + 1000;
                    lpth->pt.y = Random(dUni) + 1000;
                } else {
                    lpth->pt.x = Random(25) + ptBase.x - 12;
                    lpth->pt.y = Random(25) + ptBase.y - 12;
                }
                if (lpth->pt.x != ptBase.x || lpth->pt.y != ptBase.y) {
                    iLow = IValidateWormholePos(lpth);
                    if (iLow == 0)
                        break;
                    if (iLow < iMax) {
                        iMax = iLow;
                        pt = lpth->pt;
                    }
                }
            }
            if (iLow != 0) {
                lpth->pt = pt;
            }
        } else if (lpth->ith == ithMysteryTrader && !fPostProd) {
            dRange = lpth->tht.iWarp;
            if (dRange < 13 && Random(25) == 0) {
                idm = idmMysteryTraderHasUnexplicablyChangedHisCourse;
                if (Random(3) != 0)
                    goto LSpeedUpOnly;
            LRetargetFreighter:
                if (Random(2) == 0) {
                    rgC[0] = 400 * game.mdSize + 1380;
                } else {
                    rgC[0] = 1020;
                }
                rgC[1] = Random(400 * game.mdSize + 361) + 1020;
                iX = Random(2);
                lpth->tht.ptDest.x = rgC[iX];
                lpth->tht.ptDest.y = rgC[iX == 0];
            LSpeedUpOnly:
                dRange++;
                lpth->tht.iWarp = dRange;
                for (k = 0; k < game.cPlayer; k++) {
                    FSendPlrMsg2(k, idm, gotoThing, lpth->idFull, 0);
                }
            }
            dRange = lpth->tht.iWarp;
            dRange *= dRange;
            ptDst = lpth->tht.ptDest;
            fAnythingMoved = TRUE;
            if (idm == idmMysteryTraderHasDecidedMakeAnotherPass)
                continue;
            goto MoveTh;
        } else {
            if (lpth->ith != ithMineralPacket || lpth->thp.iWarp == 0 || (fPostProd && lpth->thp.fMoved))
                continue;
            if (lpth->thp.rgwtMin[0] == 0 && lpth->thp.rgwtMin[1] == 0 && lpth->thp.rgwtMin[2] == 0)
                goto LFreeThePacket;
            lpth->thp.fMoved = TRUE;
            fAnythingMoved = TRUE;
            dRange = lpth->thp.iWarp + 4;
            dRange *= dRange;
            if (fPostProd) {
                dRange >>= 1;
            }
            ptDst = rgptPlan[lpth->thp.idPlanet];
        MoveTh:
            ptSrc = lpth->pt;
            d = DGetDistance(ptSrc.x, ptSrc.y, ptDst.x, ptDst.y);
            dLeft = LOWORD(Sf64ToI32(d));
            if (dLeft <= dRange) {
            MadeItThere:
                if (lpth->ith == ithMysteryTrader) {
                    lpth2 = lpThings;
                    lpth2Mac = lpThings + cThing;
                    for (; lpth2 < lpth2Mac && (lpth2->ith != ithMysteryTrader || lpth2 == lpth); lpth2++) {
                    }
                    if (lpth2 != lpth2Mac || Random(2) == 0)
                        goto LFreeThePacket;
                    lpth->pt = lpth->tht.ptDest;
                    dRange = lpth->tht.iWarp - 2;
                    if (dRange < 6) {
                        dRange = 6;
                    }
                    idm = idmMysteryTraderHasDecidedMakeAnotherPass;
                    goto LRetargetFreighter;
                }
                if (lpth->ith == ithMineralPacket) {
                    pctRate = LMulDiv(dLeft, 100, dRange);
                    if (pctRate < 0) {
                        pctRate = 0;
                    } else if (pctRate > 100) {
                        pctRate = 100;
                    }
                    if (fPostProd) {
                        pctRate >>= 1;
                    }
                    if (FPacketDecay(lpth, pctRate))
                        goto LPacketAlreadyFreed;
                }
                lppl = lpPlanets + lpth->thp.idPlanet;
                iWarpPacket = lpth->thp.iWarp + 4;
                iWarp = IWarpMAFromLppl(lppl, &fTwoMAs);
                if (fTwoMAs) {
                    iWarp++;
                }
                if (iWarp > 0 && GetRaceStat(&rgplr[lpth->iplr], rsMajorAdv) == raMassAccel) {
                    rglpshdefSB[lppl->iPlayer][lppl->isb].grbitPlr = rglpshdefSB[lppl->iPlayer][lppl->isb].grbitPlr | 1 << lpth->iplr;
                }
                fTerra = GetRaceStat(&rgplr[lpth->iplr], rsMajorAdv) == raMassAccel;
                iWarp2 = iWarp * iWarp;
                iWarpPacket2 = iWarpPacket * iWarpPacket;
                if (GetRaceStat(&rgplr[lppl->iPlayer], rsMajorAdv) == raStargate) {
                    iWarp2 /= 2;
                }
                if (iWarp2 >= iWarpPacket2) {
                    pctCaught = 1000;
                } else if (iWarp > 0) {
                    iWarp = iWarp2;
                    pctCaught = LOWORD((int32_t)((int32_t)(iWarp * 1000) / iWarpPacket2));
                } else {
                    pctCaught = 0;
                }
                pctMinKeep = 1000 - pctCaught;
                for (i = 0; i < 3; i++) {
                    rgwtTerra[i] = LOWORD((int32_t)(lpth->thp.rgwtMin[i] * pctMinKeep) / 1000);
                }
                pctMinKeep = (int16_t)(1000 - pctCaught) / 9 + pctCaught;
                wtTot = 0;
                for (i = 0; i < 3; i++) {
                    if (lpth->thp.rgwtMin[i] < 0) {
                        lpth->thp.rgwtMin[i] = 0;
                    }
                    wtTot += lpth->thp.rgwtMin[i];
                    lppl->rgwtMin[i] += (int32_t)(lpth->thp.rgwtMin[i] * pctMinKeep) / 1000;
                }
                if (pctCaught == 1000)
                    goto LAllSafe;
                dmgRaw = (int32_t)((int16_t)(iWarpPacket * iWarpPacket - iWarp) * wtTot) / 160;
                if (fTerra) {
                    iplr = lpth->iplr;
                    for (i = 0; i < 3; i++) {
                        cTerraTemp = 0;
                        cTerraPerm = 0;
                        while (rgwtTerra[i] > 0) {
                            wtCur = rgwtTerra[i] >= 100 ? 100 : rgwtTerra[i];
                            if (Random(200) < wtCur) {
                                cTerraTemp++;
                                if (Random(10) == 0) {
                                    cTerraPerm++;
                                }
                            }
                            rgwtTerra[i] -= 100;
                        }
                        if (cTerraPerm > 0) {
                            if (rgplr[iplr].rgEnvVarMin[i] < 0) {
                                if (lppl->rgEnvVarOrig[i] < 50) {
                                    cTerraPerm = -(cTerraPerm >= lppl->rgEnvVarOrig[i] - 1 ? lppl->rgEnvVarOrig[i] - 1 : cTerraPerm);
                                } else {
                                    cTerraPerm = cTerraPerm >= 99 - lppl->rgEnvVarOrig[i] ? 99 - lppl->rgEnvVarOrig[i] : cTerraPerm;
                                }
                            } else if (lppl->rgEnvVarOrig[i] < rgplr[iplr].rgEnvVar[i]) {
                                if (lppl->rgEnvVarOrig[i] + cTerraPerm > rgplr[iplr].rgEnvVar[i]) {
                                    cTerraPerm = rgplr[iplr].rgEnvVar[i] - lppl->rgEnvVarOrig[i];
                                }
                            } else if (lppl->rgEnvVarOrig[i] <= rgplr[iplr].rgEnvVar[i]) {
                                cTerraPerm = 0;
                            } else if (lppl->rgEnvVarOrig[i] - cTerraPerm < rgplr[iplr].rgEnvVar[i]) {
                                cTerraPerm = rgplr[iplr].rgEnvVar[i] - lppl->rgEnvVarOrig[i];
                            } else {
                                cTerraPerm = -cTerraPerm;
                            }
                            if (cTerraPerm != 0) {
                                FSendPlrMsg(iplr, idmMineralPacketHasPermanentlyDefault, lppl->id, cTerraPerm > 0, i, lppl->id, abs(cTerraPerm), 0, 0, 0);
                                if (lppl->iPlayer != iplrNone && lppl->iPlayer != iplr) {
                                    FSendPlrMsg(iplr, idmMineralPacketHasPermanentlyDefault2, lppl->id, cTerraPerm > 0, i, lppl->id, abs(cTerraPerm), 0, 0, 0);
                                }
                                lppl->rgEnvVarOrig[i] += cTerraPerm;
                            }
                        }
                        if (cTerraTemp > 0) {
                            idPlayer = iplr;
                            if (!FCanTerraformLppl(lppl, rgMin, rgMax, rgCost, TRUE)) {
                                idPlayer = iplrNone;
                            } else {
                                idPlayer = iplrNone;
                                if (rgplr[iplr].rgEnvVarMin[i] < 0) {
                                    cTerraTemp /= 2;
                                    if (lppl->rgEnvVarOrig[i] < 50) {
                                        cTerraTemp = -(cTerraTemp >= lppl->rgEnvVar[i] - 1 ? lppl->rgEnvVar[i] - 1 : cTerraTemp);
                                    } else {
                                        cTerraTemp = cTerraTemp >= 99 - lppl->rgEnvVar[i] ? 99 - lppl->rgEnvVar[i] : cTerraTemp;
                                    }
                                } else if (rgMin[i] != envNone) {
                                    if (cTerraTemp > lppl->rgEnvVar[i] - rgMin[i]) {
                                        cTerraTemp = rgMin[i] - lppl->rgEnvVar[i];
                                    } else {
                                        cTerraTemp = -cTerraTemp;
                                    }
                                } else if (rgMax[i] == envNone) {
                                    cTerraTemp = 0;
                                } else if (cTerraTemp > rgMax[i] - lppl->rgEnvVar[i]) {
                                    cTerraTemp = rgMax[i] - lppl->rgEnvVar[i];
                                }
                                if (cTerraTemp != 0) {
                                    lppl->rgEnvVar[i] += cTerraTemp;
                                    FSendPlrMsg(iplr, idmMineralPacketHas, lppl->id, cTerraTemp > 0, i, lppl->id, i << 8 | lppl->rgEnvVar[i], 0, 0, 0);
                                    if (lppl->iPlayer != iplrNone && lppl->iPlayer != iplr) {
                                        FSendPlrMsg(iplr, idmMineralPacketHas2, lppl->id, cTerraTemp > 0, i, lppl->id, i << 8 | lppl->rgEnvVar[i], 0, 0, 0);
                                    }
                                }
                            }
                        }
                    }
                }
                if (lppl->iPlayer == iplrNone)
                    goto LFreeThePacket;
                CalcPctSurvive(lppl, &pct, NULL);
                dmgRaw = Sf80ToI32((Sf80Mul(Sf80From32(pct), Sf80FromI32(dmgRaw))));
                if (dmgRaw == 0 || GetRaceStat(&rgplr[lppl->iPlayer], rsMajorAdv) == raMacintosh) {
                LAllSafe:
                    FSendPlrMsg(lppl->iPlayer,
                                iWarp <= 0 ? idmBombardedPacketContainingKtMineralsHoweverPacket : idmMassAcceleratorHasSuccessfullyCapturedPacketCont,
                                lppl->id, lppl->id, lpth->iplr, LOWORD(wtTot), HIWORD(wtTot), 0, 0, 0);
                    goto LFreeThePacket;
                }
                lColKilled = lppl->rgwtMin[3];
                if (lColKilled != 0) {
                    lColKilled = (int32_t)(lColKilled * dmgRaw) / 1000;
                    if (lColKilled < dmgRaw) {
                        lColKilled = dmgRaw;
                    }
                    if (lppl->rgwtMin[3] > 0 && (lColKilled >= lppl->rgwtMin[3] || lColKilled < 0)) {
                        FSendPlrMsg2(lppl->iPlayer, idmAnnihilatedMineralPacketColonistsKilled, lppl->id, lppl->id, lpth->iplr);
                        UninhabitPlanet(lppl);
                        goto LFreeThePacket;
                    }
                    lDefKilled = (int32_t)(lppl->cDefenses * dmgRaw) / 1000;
                    if (lDefKilled == 0 && lppl->cDefenses != 0) {
                        lDefKilled = (uint32_t)(Random(20) < dmgRaw);
                    }
                    if (lDefKilled < (int32_t)(dmgRaw / 20)) {
                        lDefKilled = (int32_t)(dmgRaw / 20);
                    }
                    if (lDefKilled > (int32_t)lppl->cDefenses) {
                        lDefKilled = lppl->cDefenses;
                    }
                    if (lDefKilled == 0) {
                        idm = iWarp == 0 ? idmBombardedKtMineralPacketColonistsKilledCollision : idmMassAcceleratorPartiallySuccessfullyCapturingKtM;
                        FSendPlrMsg(lppl->iPlayer, idm, lppl->id, lppl->id, LOWORD(wtTot), HIWORD(wtTot), lpth->iplr, LOWORD(lColKilled), 0, 0);
                    } else {
                        idm = iWarp == 0 ? idmBombardedKtMineralPacketColonistsDefensesDestroy : idmMassAcceleratorPartiallySuccessfullyCapturingKtM2;
                        FSendPlrMsg(lppl->iPlayer, idm, lppl->id, lppl->id, LOWORD(wtTot), HIWORD(wtTot), lpth->iplr, LOWORD(lColKilled), LOWORD(lDefKilled),
                                    0);
                        lppl->cDefenses -= LOWORD(lDefKilled);
                    }
                } else {
                    FSendPlrMsg2(lppl->iPlayer, idmBombardedKtMineralPacketFortunatelyOneHome, lppl->id, lppl->id, lpth->iplr);
                    lDefKilled = lppl->cDefenses;
                    lColKilled = 0;
                }
                lppl->rgwtMin[3] -= lColKilled;
            LFreeThePacket:
                FreeLpth(lpth);
            LPacketAlreadyFreed:
                lpth--;
                lpthMac--;
            } else {
                dxRound = Sf64From80((ptDst.x <= ptSrc.x ? Sf80From64(Sf64Neg(0.5)) : Sf80From64(0.5)));
                dyRound = Sf64From80((ptDst.y <= ptSrc.y ? Sf80From64(Sf64Neg(0.5)) : Sf80From64(0.5)));
                if (Sf80Lt(Sf80From64(0.0001), Sf80From64(d)) || Sf80Lt(Sf80From64(d), Sf80From64(Sf64Neg(0.0001)))) {
                    r = Sf64From80((Sf80Div(Sf80FromI32(dRange), Sf80From64(d))));
                    ptSrc.x = LOWORD(Sf80ToI32((Sf80Add(Sf80Mul(Sf80From64(r), Sf80FromI32((int16_t)(ptDst.x - ptSrc.x))), Sf80From64(dxRound))))) + ptSrc.x;
                    ptSrc.y = LOWORD(Sf80ToI32((Sf80Add(Sf80Mul(Sf80From64(r), Sf80FromI32((int16_t)(ptDst.y - ptSrc.y))), Sf80From64(dyRound))))) + ptSrc.y;
                    if (ptSrc.x == ptDst.x && ptSrc.y == ptDst.y)
                        goto MadeItThere;
                    lpth->pt = ptSrc;
                }
                if (fPostProd && lpth->ith == ithMineralPacket && FPacketDecay(lpth, 50))
                    goto LPacketAlreadyFreed;
            }
        }
    }
    if (fAnythingMoved) {
        ValidateWaypoints();
    }
    return;
}

void FuelFleets() {
    int16_t j;
    int32_t cPods;
    PLANET *lppl;
    int16_t i;
    int16_t ifl;
    FLEET  *lpfl;
    SHDEF  *lpshdef;
    int32_t csh;
    HUL    *lphul;

    for (ifl = 0; ifl < cFleet; ifl++) {
        lpfl = rglpfl[ifl];
        if (!rglpfl[ifl])
            break;
        if (lpfl->fDead)
            continue;
        if (lpfl->idPlanet == idPlanetDeepSpace || !lpPlanets[lpfl->idPlanet].fStarbase)
            goto LChkFuelTransport;
        lppl = lpPlanets + lpfl->idPlanet;
        if (lppl->iPlayer == iplrNone)
            goto LChkFuelTransport;
        if (lpfl->iPlayer != lppl->iPlayer && rgplr[lppl->iPlayer].rgmdRelation[lpfl->iPlayer] != 1)
            goto LChkFuelTransport;
        if (LphuldefFromId(rglpshdefSB[lppl->iPlayer][lppl->isb].hul.ihuldef)->hul.wtCargoMax == 0)
            goto LChkFuelTransport;
        lpfl->rgwtMin[4] = LGetFleetStat(lpfl, 1);
        continue;

    LChkFuelTransport:
        csh = 0;
        cPods = 0;
        for (i = 0; i < 16; i++) {
            if (lpfl->rgcsh[i] != 0) {
                lphul = &rglpshdef[lpfl->iPlayer][i].hul;
                for (j = lphul->chs - 1; j >= 0; j--) {
                    if (lphul->rghs[j].grhst == hstSpecialE && lphul->rghs[j].iItem == ispecialEAntiMatterGenerator) {
                        cPods += (uint32_t)(lpfl->rgcsh[i] * lphul->rghs[j].cItem);
                    }
                }
                lpshdef = rglpshdef[lpfl->iPlayer] + i;
                if (lpshdef->hul.ihuldef == ihuldefFuelTransport || lpshdef->hul.ihuldef == ihuldefSuperFuelXport) {
                    csh += (uint32_t)(lpfl->rgcsh[i] * 200);
                }
            }
        }
        if (csh != 0 || cPods != 0) {
            lpfl->rgwtMin[4] = min(LGetFleetStat(lpfl, 1), lpfl->rgwtMin[4] + csh + (uint32_t)(cPods * 50));
        }
    }
    return;
}

void MoveFleets() {
    int32_t dTravel;
    int16_t cPass;
    int32_t wtFuel2Dest;
    double  d;
    int16_t fGotEnufFuel;
    int16_t fRanOutOfFuel;
    ORDER  *lpord;
    POINT16 ptEnd;
    int16_t ifl;
    FLEET  *lpfl;
    double  r;
    int32_t pct;
    int16_t dMineTravel;
    int32_t dRange;
    POINT16 ptBeg;
    int32_t wtFuelUsed;
    int32_t dActTravel;
    int32_t lFuelGain;
    int16_t fDone;
    SCAN    scan;
    PLANET *lpplDst;
    int32_t wtColonists;
    int16_t i;
    PLANET *lpplSrc;
    int16_t fJumpgate;
    int16_t isbsDst;
    int16_t isbsSrc;
    POINT16 ptMsg;
    int32_t wtMinerals;
    int32_t cDie;
    int16_t cKill;
    int16_t ish;
    FLEET   flSrc;
    int16_t cTry;
    FLEET   flDead;
    int16_t cKillTot;
    int16_t fDead;
    int16_t dy;
    int16_t dx;
    int32_t lFuelGainAct;
    double  dyRound;
    double  dxRound;
    int16_t iCtr;
    THING  *lpthDest;
    THING  *lpth;
    int16_t grbitPlr;

    cPass = 0;
    if (cFleet > 0) {
    MoveUnfinishedFleets:
        fDone = TRUE;
        for (ifl = 0; ifl < cFleet; ifl++) {
            lpfl = rglpfl[ifl];
            if (!rglpfl[ifl])
                break;
            if (cPass == 0) {
                lpfl->dirLong = 0;
                lpfl->fHereAllTurn = TRUE;
            }
            if (!lpfl->fDead && (cPass <= 0 || !lpfl->fDone)) {
                lpfl->fDone = TRUE;
                lpord = lpfl->lpplord->rgord;
                if (lpord->grTask != grTaskXfer && lpord->grTask != grTaskLayMines && lpfl->cord > 1 && lpord[1].iWarp != 0) {
                    if (cPass == 0 && lpord[1].iWarp > 6 && lpord[1].iWarp != 11 && GetRaceGrbit(&rgplr[lpfl->iPlayer], ibitRaceCheapEngines) != 0 &&
                        Random(10) == 0) {
                        FSendPlrMsg2(lpfl->iPlayer, idmUnableEngageEnginesDueBalkyEquipmentEngineers, lpfl->id | 0x8000, lpfl->id, 0);
                    } else {
                        if (lpord[1].iWarp >= 11) {
                            fJumpgate = FALSE;
                            gd.fRadiatingEngine = FALSE;
                            ptMsg = lpord->pt;
                            ptBeg = lpord->pt;
                            if (lpord->grobj == grobjPlanet) {
                                ptMsg.x = -1;
                                ptMsg.y = lpord->id;
                                lpplSrc = LpplFromId(lpord->id);
                                isbsSrc = IStargateFromLppl(lpplSrc);
                            } else {
                                isbsSrc = -1;
                            }
                            if (isbsSrc == -1) {
                                if (FFleetCanJumpgate(lpfl))
                                    goto LNoGateNeeded;
                                FSendPlrMsg(lpfl->iPlayer, idmAttemptedUseStargateStargateExistsThere, lpfl->id | 0x8000, lpfl->id, ptMsg.x, ptMsg.y, 0, 0, 0,
                                            0);
                                continue;
                            }
                            if (lpplSrc->iPlayer != lpfl->iPlayer && rgplr[lpplSrc->iPlayer].rgmdRelation[lpfl->iPlayer] != 1) {
                                FSendPlrMsg(lpfl->iPlayer, idmAttemptedUseStargateCouldBecauseStarbaseOwned, lpfl->id | 0x8000, lpfl->id, lpplSrc->id,
                                            lpplSrc->id, 0, 0, 0, 0);
                                continue;
                            }
                        LNoGateNeeded:
                            ptMsg = lpord[1].pt;
                            ptEnd = lpord[1].pt;
                            if (lpord[1].grobj == grobjPlanet) {
                                ptMsg.x = -1;
                                ptMsg.y = lpord[1].id;
                                lpplDst = LpplFromId(lpord[1].id);
                                isbsDst = IStargateFromLppl(lpplDst);
                            } else {
                                for (i = 0; i < game.cPlanMax && (ptEnd.x != rgptPlan[i].x || ptEnd.y != rgptPlan[i].y); i++) {
                                }
                                if (i < game.cPlanMax) {
                                    ptMsg.x = -1;
                                    ptMsg.y = i;
                                    lpplDst = LpplFromId(i);
                                    isbsDst = IStargateFromLppl(lpplDst);
                                } else {
                                    FSendPlrMsg(lpfl->iPlayer, idmAttemptedReachViaStargateCouldBecauseStargate, lpfl->id | 0x8000, lpfl->id, ptEnd.x, ptEnd.y,
                                                0, 0, 0, 0);
                                    continue;
                                }
                            }
                            if (isbsDst == -1) {
                                FSendPlrMsg(lpfl->iPlayer, idmAttemptedUseStargateReachCouldBecauseStargate, lpfl->id | 0x8000, lpfl->id, lpplDst->id, ptMsg.x,
                                            ptMsg.y, 0, 0, 0);
                                continue;
                            }
                            if (lpplDst->iPlayer != lpfl->iPlayer && rgplr[lpplDst->iPlayer].rgmdRelation[lpfl->iPlayer] != 1) {
                                FSendPlrMsg(lpfl->iPlayer, idmAttemptedUseStargateReachCouldBecauseStarbase, lpfl->id | 0x8000, lpfl->id, lpplDst->id,
                                            lpplDst->id, lpplDst->id, 0, 0, 0);
                                continue;
                            }
                            if (isbsSrc == -1) {
                                fJumpgate = TRUE;
                                isbsSrc = isbsDst;
                            }
                            if (!fJumpgate && GetRaceStat(&rgplr[lpfl->iPlayer], rsMajorAdv) != raStargate) {
                                if (lpfl->rgwtMin[3] > 0 && lpplSrc->iPlayer != lpfl->iPlayer) {
                                    FSendPlrMsg2(lpfl->iPlayer, idmUnableUseStargateBecauseHadColonistsBoard, lpfl->id | 0x8000, lpfl->id, lpplSrc->id);
                                    continue;
                                }
                                wtMinerals = 0;
                                for (i = 0; i <= 2; i++) {
                                    if (lpfl->rgwtMin[i] != 0) {
                                        wtMinerals += lpfl->rgwtMin[i];
                                        lpplSrc->rgwtMin[i] += lpfl->rgwtMin[i];
                                        lpfl->rgwtMin[i] = 0;
                                    }
                                }
                                wtColonists = lpfl->rgwtMin[3];
                                lpplSrc->rgwtMin[3] += lpfl->rgwtMin[3];
                                lpfl->rgwtMin[3] = 0;
                                if (wtColonists != 0) {
                                    if (wtMinerals != 0) {
                                        FSendPlrMsg(lpfl->iPlayer, idmHasUnloadedColonistsKtMineralsPreparationJumping, lpfl->id | 0x8000, lpfl->id,
                                                    LOWORD(wtColonists), HIWORD(wtColonists), LOWORD(wtMinerals), HIWORD(wtMinerals), lpplSrc->id, 0);
                                        if (lpfl->iPlayer != lpplSrc->iPlayer) {
                                            FSendPlrMsg(lpplSrc->iPlayer, idmHasUnloadedColonistsKtMineralsPreparationJumping, lpplSrc->id, lpfl->id,
                                                        LOWORD(wtColonists), HIWORD(wtColonists), LOWORD(wtMinerals), HIWORD(wtMinerals), lpplSrc->id, 0);
                                        }
                                    } else {
                                        FSendPlrMsg(lpfl->iPlayer, idmHasUnloadedColonistsPreparationJumpingThroughSta, lpfl->id | 0x8000, lpfl->id,
                                                    LOWORD(wtColonists), HIWORD(wtColonists), lpplSrc->id, 0, 0, 0);
                                        if (lpfl->iPlayer != lpplSrc->iPlayer) {
                                            FSendPlrMsg(lpplSrc->iPlayer, idmHasUnloadedColonistsPreparationJumpingThroughSta, lpplSrc->id, lpfl->id,
                                                        LOWORD(wtColonists), HIWORD(wtColonists), lpplSrc->id, 0, 0, 0);
                                        }
                                    }
                                } else if (wtMinerals != 0) {
                                    FSendPlrMsg(lpfl->iPlayer, idmHasUnloadedKtMineralsPreparationJumpingThrough, lpfl->id | 0x8000, lpfl->id,
                                                LOWORD(wtMinerals), HIWORD(wtMinerals), lpplSrc->id, 0, 0, 0);
                                    if (lpfl->iPlayer != lpplSrc->iPlayer) {
                                        FSendPlrMsg(lpplSrc->iPlayer, idmHasUnloadedKtMineralsPreparationJumpingThrough, lpplSrc->id, lpfl->id,
                                                    LOWORD(wtMinerals), HIWORD(wtMinerals), lpplSrc->id, 0, 0, 0);
                                    }
                                }
                            }
                            dTravel = Sf64ToI32(DGetDistance(ptBeg.x, ptBeg.y, ptEnd.x, ptEnd.y));
                            if (!FStargateJump(lpfl, isbsSrc, isbsDst, LOWORD(dTravel)))
                                continue;
                            lpfl->fHereAllTurn = FALSE;
                            NoAutoTrackFleet(lpfl);
                            goto LMakeItToDest;
                        } else {
                            ptBeg = lpfl->pt;
                            if (cPass > 0 && !lpord[1].fNoAutoTrack) {
                                lpord[1].pt = lpfl->lpflNext->pt;
                            }
                            ptEnd = lpord[1].pt;
                            dRange = EstFuelUse(lpfl, 0, -1, -1, TRUE);
                            wtFuel2Dest = EstFuelUse(lpfl, 0, -1, -1, FALSE);
                            fGotEnufFuel = wtFuel2Dest <= lpfl->rgwtMin[4];
                            fRanOutOfFuel = FALSE;
                            if (fGotEnufFuel) {
                                dRange = dRange <= (int32_t)(uint32_t)(lpord[1].iWarp * lpord[1].iWarp) ? (uint32_t)(lpord[1].iWarp * lpord[1].iWarp) : dRange;
                            }
                            if (cPass == 0) {
                                if (lpfl->rgwtMin[3] > 10 && GetRaceStat(&rgplr[lpfl->iPlayer], rsMajorAdv) == raMacintosh) {
                                    cDie = (int32_t)((uint32_t)(lpfl->rgwtMin[3] * 3) + 33) / 100;
                                    if (cDie > 0) {
                                        lpfl->rgwtMin[3] -= cDie;
                                        FSendPlrMsg(lpfl->iPlayer, idmDueRigorsWarpAccelerationColonistsHaveDied, lpfl->id | 0x8000, LOWORD(cDie), HIWORD(cDie),
                                                    lpfl->id, 0, 0, 0, 0);
                                    }
                                }
                                if (lpord[1].iWarp == 10) {
                                    flSrc = *lpfl;
                                    fDead = TRUE;
                                    cKillTot = 0;
                                    memset(&flDead, 0, sizeof(FLEET));
                                    for (ish = 0; ish < 16; ish++) {
                                        if (flSrc.rgcsh[ish] != 0) {
                                            if (rglpshdef[lpfl->iPlayer][ish].hul.rghs[0].iItem == iengineInterspace10 ||
                                                rglpshdef[lpfl->iPlayer][ish].hul.rghs[0].iItem == iengineTransStar10 ||
                                                rglpshdef[lpfl->iPlayer][ish].hul.rghs[0].iItem == iengineTransGalacticMizerScoop ||
                                                rglpshdef[lpfl->iPlayer][ish].hul.rghs[0].iItem == iengineGalaxyScoop ||
                                                rglpshdef[lpfl->iPlayer][ish].hul.rghs[0].iItem == iengineEnigmaPulsar)
                                                goto LWarp10Kill;
                                            cKill = 0;
                                            cTry = flSrc.rgcsh[ish];
                                            while (cTry-- != 0) {
                                                if (Random(10) == 0) {
                                                    cKill++;
                                                }
                                            }
                                            if (cKill > 0) {
                                                cKillTot += cKill;
                                                flSrc.rgcsh[ish] -= cKill;
                                                flDead.rgcsh[ish] = cKill;
                                            }
                                        LWarp10Kill:
                                            if (flSrc.rgcsh[ish] > 0) {
                                                fDead = FALSE;
                                            }
                                        }
                                    }
                                    if (fDead) {
                                        lpfl->fDead = TRUE;
                                        FSendPlrMsg2(lpfl->iPlayer, idmDestroyedMassiveReactorAccidentDueUnsafeOperatin, lpfl->id | 0x8000, lpfl->id, 0);
                                        continue;
                                    }
                                    if (cKillTot > 0) {
                                        flDead.iPlayer = flSrc.iPlayer;
                                        flDead.fDead = TRUE;
                                        flDead.det = detAll;
                                        FleetTransferCargoBalance(&flSrc, &flDead);
                                        *lpfl = flSrc;
                                        if (cKillTot == 1) {
                                            FSendPlrMsg2(lpfl->iPlayer, idmOneShipsDestroyedWhenEnginesReactedTrying, lpfl->id | 0x8000, lpfl->id, 0);
                                        } else {
                                            FSendPlrMsg2(lpfl->iPlayer, idmShipsDestroyedDueEngineStrain, lpfl->id | 0x8000, cKillTot, lpfl->id);
                                        }
                                    }
                                }
                                dTravel = (uint32_t)(lpord[1].iWarp * lpord[1].iWarp);
                                if (lpord[1].grobj == grobjFleet) {
                                    lpfl->lpflNext = LpflFromId(lpord[1].id);
                                    if (lpfl->lpflNext) {
                                        fDone = FALSE;
                                        lpfl->fDone = FALSE;
                                        lpfl->lPower = (uint32_t)LOWORD(dTravel);
                                        lpfl->lFuelUsed = 0;
                                        continue;
                                    }
                                }
                            } else {
                                if (lpfl->lpflNext->fDone) {
                                    dTravel = lpfl->dMoveLeft;
                                } else {
                                    dTravel = lpfl->dMoveLeft >= (int16_t)(lpfl->dMoveLeft + lpfl->dMoveUsed + 4) / 5
                                                  ? (int16_t)((int16_t)(lpfl->dMoveLeft + lpfl->dMoveUsed + 4) / 5)
                                                  : lpfl->dMoveLeft;
                                }
                                dRange -= lpfl->dMoveUsed;
                                if (dRange < 0) {
                                    dRange = 0;
                                }
                            }
                            d = DGetDistance(ptBeg.x, ptBeg.y, ptEnd.x, ptEnd.y);
                            dTravel = dTravel < (int16_t)LOWORD(Sf80ToI32((Sf80Add(Sf80From64(d), Sf80From64(0.9999)))))
                                          ? dTravel
                                          : (int16_t)LOWORD(Sf80ToI32((Sf80Add(Sf80From64(d), Sf80From64(0.9999)))));
                            if (dTravel > dRange) {
                                lpfl->rgwtMin[4] = 0;
                                wtFuelUsed = 1;
                                dTravel = dRange;
                            } else {
                                if (cPass > 0) {
                                    lpfl->rgwtMin[4] += lpfl->lFuelUsed;
                                    dTravel += lpfl->dMoveUsed;
                                }
                                wtFuelUsed = EstFuelUse(lpfl, 0, -1, dTravel, FALSE);
                                if (cPass > 0) {
                                    lpfl->lFuelUsed = wtFuelUsed;
                                    dTravel -= lpfl->dMoveUsed;
                                }
                                lpfl->rgwtMin[4] = 0 <= lpfl->rgwtMin[4] - wtFuelUsed ? lpfl->rgwtMin[4] - wtFuelUsed : 0;
                            }
                            if (lpfl->rgwtMin[4] == 0 && wtFuelUsed > 0 &&
                                ((Sf80Le(Sf80FromI32(dTravel), Sf80Sub(Sf80From64(d), Sf80From64(0.99999))) || dRange == 0) && !fGotEnufFuel)) {
                                i = 0;
                                do {
                                    i++;
                                } while (EstFuelUse(lpfl, 0, i, -1, FALSE) == 0 && i < 10);
                                if (i > 1) {
                                    lpfl->lpplord->rgord[1].iWarp = i - 1;
                                    FSendPlrMsg2(lpfl->iPlayer, idmHasRunFuelFleetsSpeedHasDecreased, lpfl->id | 0x8000, lpfl->id, i - 1);
                                } else {
                                    FSendPlrMsg2(lpfl->iPlayer, idmHasRunFuel, lpfl->id | 0x8000, lpfl->id, 0);
                                }
                                fRanOutOfFuel = TRUE;
                            }
                            if (dRange == 0)
                                continue;
                            lpfl->fHereAllTurn = FALSE;
                            dx = ptEnd.x - ptBeg.x;
                            dy = ptEnd.y - ptBeg.y;
                            if (dx != 0 || dy != 0) {
                                lpfl->fdirValid = 1;
                                for (; abs(dx) > 127 || abs(dy) > 127; dy /= 2) {
                                    dx /= 2;
                                }
                                lpfl->dirFltX = dx + 127;
                                lpfl->dirFltY = dy + 127;
                                lpfl->iwarpFlt = lpfl->lpplord->rgord[1].iWarp;
                            }
                            dActTravel = Sf80ToI32((Sf80Sub(Sf80From64(d), Sf80From64(0.99999))));
                            dMineTravel = dTravel < dActTravel ? LOWORD(dTravel) : LOWORD(dActTravel);
                            if (lpord[1].iWarp < 11 && !FTravelThroughMineFields(lpfl, &dMineTravel, NULL)) {
                                lpfl->dMoveLeft = 0;
                                if (lpfl->fDead)
                                    continue;
                                if (dMineTravel < dActTravel) {
                                    dTravel = dMineTravel;
                                }
                            } else if (!fRanOutOfFuel && GetFuelFree(lpfl) > 0) {
                                lFuelGain = LCalcFuelGainFromRamScoops(lpfl, lpord[1].iWarp, dTravel < dActTravel ? dTravel : dActTravel);
                                if (lFuelGain > 0) {
                                    lFuelGainAct = ChgCargo(grobjFleet, lpfl->id, Fuel, lFuelGain, NULL);
                                    if (lFuelGain > 32500) {
                                        lFuelGain = 32500;
                                    }
                                    FSendPlrMsg2(lpfl->iPlayer, idmSRamScoopsHaveProducedMgFuel, lpfl->id | 0x8000, lpfl->id, LOWORD(lFuelGain));
                                }
                            }
                            if (dActTravel < dTravel || dActTravel <= 0) {
                            LMakeItToDest:
                                lpfl->pt = ptEnd;
                                if (lpord[1].grobj == grobjPlanet) {
                                    lpfl->idPlanet = lpord[1].id;
                                } else {
                                    lpfl->idPlanet = idPlanetDeepSpace;
                                }
                                /* A pursuer that catches a fleet still on its
                                   own pursuit keeps following it with the rest
                                   of its move. The original marked the caught
                                   fleet done instead, which stopped it short of
                                   its own target. */
                                if (cPass > 0 && !lpfl->lpflNext->fDone) {
                                    lpfl->dMoveUsed += LOWORD(dTravel);
                                    lpfl->dMoveLeft -= LOWORD(dTravel);
                                    if (lpfl->dMoveLeft > 0 && !fRanOutOfFuel) {
                                        fDone = FALSE;
                                        lpfl->fDone = FALSE;
                                    }
                                }
                            } else {
                                dxRound = Sf64From80((ptEnd.x <= ptBeg.x ? Sf80From64(Sf64Neg(0.5)) : Sf80From64(0.5)));
                                dyRound = Sf64From80((ptEnd.y <= ptBeg.y ? Sf80From64(Sf64Neg(0.5)) : Sf80From64(0.5)));
                                if (Sf80Lt(Sf80From64(0.0001), Sf80From64(d)) || Sf80Lt(Sf80From64(d), Sf80From64(Sf64Neg(0.0001)))) {
                                    r = Sf64From80((Sf80Div(Sf80FromI32(dTravel), Sf80From64(d))));
                                    lpfl->pt.x =
                                        LOWORD(Sf80ToI32((Sf80Add(Sf80Mul(Sf80From64(r), Sf80FromI32((int16_t)(ptEnd.x - ptBeg.x))), Sf80From64(dxRound))))) +
                                        ptBeg.x;
                                    lpfl->pt.y =
                                        LOWORD(Sf80ToI32((Sf80Add(Sf80Mul(Sf80From64(r), Sf80FromI32((int16_t)(ptEnd.y - ptBeg.y))), Sf80From64(dyRound))))) +
                                        ptBeg.y;
                                    lpfl->idPlanet = idPlanetDeepSpace;
                                }
                                if (cPass > 0 && lpfl->dMoveLeft > 0) {
                                    lpfl->dMoveUsed += LOWORD(dTravel);
                                    lpfl->dMoveLeft -= LOWORD(dTravel);
                                    if (lpfl->dMoveLeft > 0 && !fRanOutOfFuel) {
                                        fDone = FALSE;
                                        lpfl->fDone = FALSE;
                                    }
                                }
                            }
                        }
                        if (gd.fRadiatingEngine && lpfl->rgwtMin[3] > 0 && cPass <= 1 &&
                            rgplr[lpfl->iPlayer].rgEnvVarMin[2] + rgplr[lpfl->iPlayer].rgEnvVarMax[2] < 170 &&
                            rgplr[lpfl->iPlayer].rgEnvVarMax[2] != envImmune) {
                            iCtr = (int16_t)(rgplr[lpfl->iPlayer].rgEnvVarMin[2] + rgplr[lpfl->iPlayer].rgEnvVarMax[2]) / 2;
                            pct = (int16_t)((0x56 - iCtr) >> 1);
                            pct = (int32_t)(pct * lpfl->rgwtMin[3]) / 100;
                            if (lpfl->rgwtMin[3] < (int16_t)(1 <= pct ? pct : 1)) {
                                pct = lpfl->rgwtMin[3];
                            } else if (1 > pct) {
                                pct = 1;
                            }
                            FSendPlrMsg2(lpfl->iPlayer, idmEngineRadiationHasKilledColonistsTraveling, lpfl->id | 0x8000, LOWORD(pct), lpfl->id);
                            lpfl->rgwtMin[3] -= pct;
                        }
                        if (ptEnd.x == lpfl->pt.x && ptEnd.y == lpfl->pt.y && lpord[1].grobj == grobjThing) {
                            lpth = LpthFromId(lpord[1].id);
                            if (lpth && lpth->ith == ithWormhole) {
                                grbitPlr = 1 << lpfl->iPlayer;
                                lpthDest = LpthFromId(lpth->thw.idPartner);
                                NoAutoTrackFleet(lpfl);
                                lpth->thw.grbitPlrTrav |= grbitPlr;
                                lpthDest->thw.grbitPlrTrav |= grbitPlr;
                                lpthDest->thw.grbitPlr |= grbitPlr;
                                lpfl->pt = lpthDest->pt;
                                lpord[1].pt = lpthDest->pt;
                            }
                        }
                        if (lpfl->idPlanet == idPlanetDeepSpace && FFindNearestObject(lpfl->pt, grobjPlanet | mdExact, &scan)) {
                            lpfl->idPlanet = scan.idpl;
                        }
                        lpord->pt = lpfl->pt;
                        lpord->id = lpfl->idPlanet;
                        lpord->grobj = lpfl->idPlanet == idPlanetDeepSpace ? 4 : 1;
                        if (fGotEnufFuel) {
                            wtFuel2Dest = EstFuelUse(lpfl, 0, -1, -1, FALSE);
                            if (wtFuel2Dest > lpfl->rgwtMin[4]) {
                                if (LGetFleetStat(lpfl, 1) > wtFuel2Dest) {
                                    lpfl->rgwtMin[4] = wtFuel2Dest;
                                } else {
                                    lpfl->rgwtMin[4] = LGetFleetStat(lpfl, 1);
                                }
                            }
                        }
                    }
                }
            }
        }
        if (!fDone) {
            if (cPass++ < 10)
                goto MoveUnfinishedFleets;
        }
        KillUsedWaypoints();
    }
    return;
}

int16_t FTravelThroughMineFields(FLEET *lpfl, int16_t *pdTravel, THING *lpthHit) {
    int32_t       d2Closest;
    int16_t       rgishInc[16];
    int16_t       dTravel;
    POINT16       ptAct;
    int16_t       iWarp;
    POINT16       ptDst;
    int16_t       dy;
    int32_t       d2;
    int16_t       j;
    int16_t       dEnd;
    FLEET         flSrc;
    int32_t       dpsh;
    int16_t       cshT;
    int32_t       dmgReduce;
    int32_t       dmgToApply;
    int16_t       i;
    THING        *lpth;
    int16_t       dmgExtra;
    int16_t       cshDamaged;
    int16_t       fMineExpert;
    POINT16       ptSrc;
    int16_t       iPlayer;
    int16_t       cFields;
    int16_t       dStart;
    FLEET         flDead;
    THING        *lpthMac;
    int32_t       csh;
    int16_t       rgi[3];
    int16_t       pct;
    int32_t       dmgTot;
    int16_t       cshDead;
    int16_t       rgcField[3];
    RaceAttribute raMajor;
    int16_t       dx;
    int32_t       dpShield;
    MineFieldType iType;
    int16_t       rgFieldE[3][8];
    THING        *lpthClosest;
    int16_t       cishInc;
    THING        *lpthSalvage;
    int16_t       fHasRamScoop;
    int16_t       dmgPer;
    int16_t       rgFieldS[3][8];
    int16_t       cEngines;
    int32_t       dmgPerShip;
    uint16_t      ibit;

    lpthSalvage = NULL;
    cshDead = 0;
    /* A detonation (lpthHit) passes no travel distance. The original read
       through the NULL near pointer anyway, from the start of its data
       segment. */
    dTravel = lpthHit ? 0 : *pdTravel;
    cishInc = 0;
    iPlayer = lpfl->iPlayer;
    raMajor = GetRaceStat(&rgplr[iPlayer], rsMajorAdv);
    fMineExpert = (raMajor == raMines) * 2 + (raMajor == raStealth);
    if (lpthHit) {
        iWarp = 0;
        goto LHitSkip1;
    }
    ptSrc = lpfl->pt;
    ptDst = lpfl->lpplord->rgord[1].pt;
    for (iWarp = 3; iWarp < 10 && iWarp * iWarp < dTravel - 1; iWarp++) {
    }
    if (iWarp <= fMineExpert + 3 || (ptSrc.x == ptDst.x && ptSrc.y == ptDst.y)) {
        return TRUE;
    }
    for (i = 0; i < 3; i++) {
        rgcField[i] = 0;
    }
    lpth = lpThings;
    lpthMac = lpThings + cThing;
    for (; lpth < lpthMac; lpth++) {
        if (lpth->iplr != iPlayer && lpth->ith == ithMinefield && rgplr[lpth->iplr].rgmdRelation[iPlayer] != 1 &&
            FIntersectCircleLine(ptSrc, ptDst, lpth->pt, lpth->thm.cMines, dTravel, &dStart, &dEnd)) {
            iType = lpth->thm.iType;
            for (i = 0; i < rgcField[iType] && rgFieldE[iType][i] < dStart; i++) {
            }
            if (i == rgcField[iType]) {
                if (i < 8) {
                    rgFieldS[iType][i] = dStart;
                    rgFieldE[iType][i] = dEnd;
                    rgcField[iType]++;
                }
            } else if (dEnd < rgFieldS[iType][i] - 1) {
                if (rgcField[iType] < 8) {
                    for (j = rgcField[iType]; j > i; j--) {
                        rgFieldS[iType][j] = rgFieldS[iType][j - 1];
                        rgFieldE[iType][j] = rgFieldE[iType][j - 1];
                    }
                    rgFieldS[iType][i] = dStart;
                    rgFieldE[iType][i] = dEnd;
                    rgcField[iType]++;
                }
            } else {
                if (dStart < rgFieldS[iType][i]) {
                    rgFieldS[iType][i] = dStart;
                }
                if (dEnd > rgFieldE[iType][i]) {
                    rgFieldE[iType][i] = dEnd;
                    for (j = i + 1; j < rgcField[iType] && rgFieldS[iType][j] <= dEnd; j++) {
                    }
                    if (rgFieldE[iType][j - 1] > dEnd) {
                        rgFieldE[iType][i] = rgFieldE[iType][j - 1];
                    }
                    i++;
                    for (; j < rgcField[iType]; j++) {
                        rgFieldS[iType][i] = rgFieldS[iType][j];
                        rgFieldE[iType][i] = rgFieldE[iType][j];
                        i++;
                    }
                    rgcField[iType] -= j - i;
                }
            }
        }
    }
    cFields = rgcField[0] + rgcField[1] + rgcField[2];
    if (cFields == 0) {
        return TRUE;
    }
LHitSkip1:
    fHasRamScoop = FALSE;
    csh = 0;
    for (i = 0; i < 16; i++) {
        if (lpfl->rgcsh[i] > 0) {
            csh += lpfl->rgcsh[i];
            j = rglpshdef[iPlayer][i].hul.rghs[0].iItem;
            if (LpengineFromId(j)->rgcFuelUsed[4] == 0) {
                fHasRamScoop = TRUE;
            }
        }
    }
    if (lpthHit) {
        iType = lpthHit->thm.iType;
        goto LHitSkip2;
    }
    rgi[2] = 0;
    rgi[1] = 0;
    rgi[0] = 0;
    while (cFields > 0) {
        dStart = 10000;
        iType = mineNone;
        for (i = 0; i < 3; i++) {
            if (rgi[i] < rgcField[i] && dStart > rgFieldS[i][rgi[i]]) {
                dStart = rgFieldS[i][rgi[i]];
                iType = i;
            }
        }
        dEnd = rgFieldE[iType][rgi[iType]] - rgFieldS[iType][rgi[iType]];
        if (iWarp <= rgiWarpSafe[iType] + fMineExpert)
            goto LDoNext;
        pct = (iWarp - rgiWarpSafe[iType] - fMineExpert) * rgpctMineHit[iType];
        for (i = 0; i < dEnd; i++) {
            if (Random(1000) < pct)
                break;
        }
        if (i == dEnd)
            goto LDoNext;
        dEnd = dStart + i;
    LHitSkip2:
        cshDead = 0;
        dmgTot = 0;
        if (rgrgdmgMine[iType][fHasRamScoop] == 0)
            goto LFinishHit;
        dmgPer = rgrgdmgMine[iType][fHasRamScoop];
        dmgExtra = rgrgdmgMinMine[iType][fHasRamScoop] - rgrgdmgMine[iType][fHasRamScoop] * LOWORD(csh);
        if (csh >= 5 || dmgExtra <= 0) {
            dmgExtra = 0;
        }
        flSrc = *lpfl;
        memset(&flDead, 0, sizeof(FLEET));
        flDead.iPlayer = flSrc.iPlayer;
        flDead.fDead = TRUE;
        flDead.det = detAll;
        for (i = 0; i < 16; i++) {
            if (lpfl->rgcsh[i] > 0 &&
                (!lpthHit || lpthHit->iplr != lpfl->iPlayer ||
                 (rglpshdef[lpfl->iPlayer][i].hul.ihuldef != ihuldefMiniMineLayer && rglpshdef[lpfl->iPlayer][i].hul.ihuldef != ihuldefSuperMineLayer))) {
                cshT = lpfl->rgcsh[i];
                rgishInc[cishInc++] = i;
                cEngines = rglpshdef[lpfl->iPlayer][i].hul.rghs[0].cItem;
                dpShield = (uint32_t)(cshT * DpShieldOfShdef(rglpshdef[iPlayer] + i, iPlayer));
                dpsh = (uint32_t)rglpshdef[iPlayer][i].hul.dp;
                dmgToApply = (uint32_t)(((uint32_t)(cshT * dmgPer) + dmgExtra) * cEngines);
                dmgTot += dmgToApply;
                dmgReduce = dpShield < (int32_t)(dmgToApply >> 1) ? dpShield : (int32_t)(dmgToApply >> 1);
                dmgToApply -= dmgReduce;
                cshDamaged = LOWORD((int32_t)(lpfl->rgcsh[i] * lpfl->rgdv[i].pctSh) / 100);
                dmgToApply += (int32_t)((uint32_t)(dpsh * lpfl->rgdv[i].pctDp) * cshDamaged) / 500;
                dmgExtra = 0;
                dmgPerShip = (int32_t)(dmgToApply / cshT);
                if (dmgPerShip > dpsh) {
                    cshDead += cshT;
                    flDead.rgcsh[i] = cshT;
                    flSrc.rgcsh[i] = 0;
                    cshT = 0;
                } else {
                    flSrc.rgdv[i].pctSh = 100;
                    flSrc.rgdv[i].pctDp = LOWORD((int32_t)((int32_t)(dmgPerShip * 500) / dpsh));
                    if (flSrc.rgdv[i].pctDp == 0) {
                        flSrc.rgdv[i].pctDp = 1;
                    }
                }
                dmgToApply = 0;
            }
        }
        /* Every ship is immune to this detonation (the owner's mine layers
           in its own field). TRUE leaves ThingDecay free to check the fleet
           against other detonating fields. */
        if (lpthHit && dmgTot == 0) {
            return TRUE;
        }
        if (cshDead != csh) {
            FleetTransferCargoBalance(&flSrc, &flDead);
        }
        *lpfl = flSrc;
        if (cshDead == csh) {
            lpfl->fDead = TRUE;
        }
    LFinishHit:
        if (!lpthHit) {
            dx = ptDst.x - ptSrc.x;
            dy = ptDst.y - ptSrc.y;
            dTravel = LOWORD(Sf80ToI32(
                (Sf80Add(Sf80From64(Sf64Sqrt(Sf64FromU32(((uint32_t)(dx * dx) + (uint32_t)(dy * (int16_t)(ptDst.y - ptSrc.y)))))), Sf80From64(0.5)))));
            ptAct.x = LMulDiv(dx, dEnd, dTravel) + ptSrc.x;
            ptAct.y = LMulDiv(dy, dEnd, dTravel) + ptSrc.y;
            if (cshDead != 0) {
                lpthSalvage = lpThings;
                lpthMac = lpThings + cThing;
                for (; lpthSalvage < lpthMac &&
                       (lpthSalvage->pt.x != ptAct.x || lpthSalvage->pt.y != ptAct.y || lpthSalvage->ith != ithMineralPacket || lpthSalvage->thp.iWarp != 0);
                     lpthSalvage++) {
                }
                if (lpthSalvage == lpthMac) {
                    lpthSalvage = NULL;
                }
                DropSalvage(&lpthSalvage, lpfl->rgwtMin, flSrc.iplr, &ptAct);
            }
            d2Closest = 100000000;
            lpth = lpThings;
            lpthMac = lpThings + cThing;
            for (; lpth < lpthMac; lpth++) {
                if (lpth->iplr != iPlayer && lpth->ith == ithMinefield && rgplr[lpth->iplr].rgmdRelation[iPlayer] != 1 && lpth->thm.iType == iType) {
                    dx = lpth->pt.x - ptAct.x;
                    dy = lpth->pt.y - ptAct.y;
                    d2 = (uint32_t)(dx * dx) + (uint32_t)(dy * (int16_t)(lpth->pt.y - ptAct.y)) - lpth->thm.cMines;
                    if (d2 < d2Closest) {
                        d2Closest = d2;
                        lpthClosest = lpth;
                    }
                }
            }
            d2 = (int32_t)(lpthClosest->thm.cMines / 20);
            if (d2 > 50) {
                d2 = (int32_t)(lpthClosest->thm.cMines / 100);
                if (d2 < 50) {
                    d2 = 50;
                }
            } else if (d2 < 10) {
                d2 = 10;
            }
        } else {
            ptAct = lpthHit->pt;
            lpthClosest = lpthHit;
        }
        if (GetRaceStat(&rgplr[lpthClosest->iplr], rsMajorAdv) == raMines) {
            ibit = 1 << lpthClosest->iplr;
            if (cishInc == 0) {
                for (i = 0; i < 16; i++) {
                    if (lpfl->rgcsh[i] > 0) {
                        rglpshdef[iPlayer][i].grbitPlr |= ibit;
                    }
                }
            } else {
                for (i = 0; i < cishInc; i++) {
                    rglpshdef[iPlayer][rgishInc[i]].grbitPlr = rglpshdef[iPlayer][rgishInc[i]].grbitPlr | ibit;
                }
            }
        }
        if (dmgTot > 32760) {
            dmgTot = 32760;
        }
        if (dmgTot == 0) {
            if (iPlayer != lpthClosest->iplr) {
                FSendPlrMsg(iPlayer, idmHasStoppedMineField, lpfl->id | 0x8000, lpfl->id | 0x8000, lpthClosest->iplr, iType, ptAct.x, ptAct.y, 0, 0);
            }
            FSendPlrMsg(lpthClosest->iplr, idmHasStoppedMineField2, lpfl->id | 0x8000, lpfl->id | 0x8000, iType, ptAct.x, ptAct.y, 0, 0, 0);
        } else if (cshDead == 0) {
            if (iPlayer != lpthClosest->iplr) {
                FSendPlrMsg(iPlayer, !lpthHit ? idmHasStoppedMineFieldFleetHasTaken : idmHasDamagedDetonatingMineFieldFleetHas, lpfl->id | 0x8000,
                            lpfl->id | 0x8000, lpthClosest->iplr, iType, ptAct.x, ptAct.y, LOWORD(dmgTot), 0);
            }
            FSendPlrMsg(lpthClosest->iplr, !lpthHit ? idmHasStoppedMineFieldMinesHaveInflicted : idmHasDamagedDetonatingMineFieldMinesHave, lpfl->id | 0x8000,
                        lpfl->id | 0x8000, iType, ptAct.x, ptAct.y, LOWORD(dmgTot), 0, 0);
        } else if (cshDead < csh) {
            if (iPlayer != lpthClosest->iplr) {
                FSendPlrMsg(iPlayer, !lpthHit ? idmHasStoppedMineFieldFleetHasTaken2 : idmHasTakenDamageDetonatingMineFieldFleet, lpfl->id | 0x8000,
                            lpfl->id | 0x8000, lpthClosest->iplr, iType, ptAct.x, ptAct.y, LOWORD(dmgTot), cshDead);
            }
            FSendPlrMsg(lpthClosest->iplr, !lpthHit ? idmHasStoppedMineFieldMinesHaveInflicted2 : idmHasDamagedDetonatingMineFieldMinesHave2, lpfl->id | 0x8000,
                        lpfl->id | 0x8000, iType, ptAct.x, ptAct.y, LOWORD(dmgTot), cshDead, 0);
        } else if (lpthSalvage) {
            flDead.id = lpfl->id;
            if (iPlayer != lpthClosest->iplr) {
                FSendPlrMsg(iPlayer, idmHasAnnihilatedMineField, gotoThing, lpthSalvage->idFull, WFromLpfl(&flDead), lpthClosest->iplr, iType, ptAct.x, ptAct.y,
                            0);
            }
            FSendPlrMsg(lpthClosest->iplr, idmHasAnnihilatedMineField2, gotoThing, lpthSalvage->idFull, lpfl->id, iType, ptAct.x, ptAct.y, 0, 0);
        } else {
            flDead.id = lpfl->id;
            if (iPlayer != lpthClosest->iplr) {
                FSendPlrMsg(iPlayer, idmHasAnnihilatedMineField3, gotoNone, WFromLpfl(&flDead), lpthClosest->iplr, iType, ptAct.x, ptAct.y, 0, 0);
            }
            if (lpfl->iPlayer == lpthClosest->iplr) {
                FSendPlrMsg(lpthClosest->iplr, idmHasAnnihilatedMineField4, gotoNone, WFromLpfl(&flDead), iType, ptAct.x, ptAct.y, 0, 0, 0);
            } else {
                FSendPlrMsg(lpthClosest->iplr, idmHasAnnihilatedMineField2, gotoThing, lpthClosest->idFull, lpfl->id, iType, ptAct.x, ptAct.y, 0, 0);
            }
        }
        if (!lpthHit) {
            if (d2 >= lpthClosest->thm.cMines) {
                FreeLpth(lpthClosest);
            } else {
                lpthClosest->thm.cMines -= d2;
                lpthClosest->thm.grbitPlrNow |= 1 << iPlayer;
                lpthClosest->thm.grbitPlr |= 1 << iPlayer;
            }
            *pdTravel = dEnd;
        }
        lpfl->fNoHeal = TRUE;
        return FALSE;
    LDoNext:
        rgi[iType]++;
        cFields--;
    }
    return TRUE;
}
