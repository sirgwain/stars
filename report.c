#include "common.h"

uint16_t mpicolgrbitBU[12] = {255, 255, 255, 255, 255, 255, 255, 8, 16, 32, 64, 128};

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
                SetHdcTextColor(hdc, 127);
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
            SetHdcTextColor(hdc, 0xff);
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
        ids = idsTaskHere + ord.grTask;
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
                if (idsAction + opOrd > (int16_t)ids) {
                    ids = idsAction + opOrd;
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
                    CchSprintf(szWork, "%s %d%%", psz, ord.txp.rgia[icr].cQuan);
                } else {
                    CchSprintf(szWork, icr == 4 ? "%s %dmg" : "%s %dkT", psz, ord.txp.rgia[icr].cQuan);
                }
                return szWork;
            default:
                return szDblDash;
            }
        case grTaskLayMines:
            if (ord.tlm.cTime < 5) {
                CchSprintf(szWork, "%s  %dy", PszGetCompressedString(ids), ord.tlm.cTime + 1);
            } else {
                CchGetString(ids, szWork);
            }
            return szWork;
        case grTaskPatrol:
            if (ord.tptl.iDist < 11) {
                CchSprintf(szWork, "%s  %dly", PszGetCompressedString(ids), (ord.tptl.iDist + 1) * 50);
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
            CchSprintf(szWork, "%s.map", szBase);
            StreamOpen(szWork, mdCreate);
            fOpen = TRUE;
            RgToStream("#\tX\tY\tName\r\n", 12);
            for (i = 0; i < game.cPlanMax; i++) {
                cch = CchSprintf(szWork, "%d\t%d\t%d\t%s\r\n", i + 1, rgptPlan[i].x, rgptPlan[i].y, PszGetCompressedPlanet(rgidPlan[i]));
                RgToStream(szWork, cch);
            }
            StreamClose();
        }
    }
DisplayStatus:
    ids = !fSuccess ? idsUnableWriteUniverseDefinitionSMapOperation : idsUniverseDefinitionHasSuccessfullyWrittenSMap;
    CchSprintf(szWork, PszGetCompressedString(ids), szBase);
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
                CchSprintf(szFile, "%s.p%d", szBase, idPlayer + 1);
            } else {
                CchSprintf(szFile, "%s.pla", szBase);
            }
            StreamOpen(szFile, mdCreate);
            fOpen = TRUE;
            j = gd.fPerPlayerDumps + 2;
            for (i = 0; i < j; i++) {
                cch = CchGetString(idsPlanetNameOwnerStarbaseTypeReportAge + i, szForm);
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
                    strcpy(&szForm[1], rglpshdefSB[lppl->iPlayer][lppl->isb].hul.szClass);
                    cch = strlen(szForm);
                }
                RgToStream(szForm, cch);
                CchSprintf(&szForm[1], "%d", game.turn - lppl->turn);
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
                    CchSprintf(&szForm[1], PCTDPCTPCT, i);
                }
                RgToStream(szForm, strlen(szForm));
                szForm[1] = 0;
                if (lppl->det == detAll) {
                    PszProdQueueTop(lppl, NULL);
                    strcpy(&szForm[1], szWork);
                }
                RgToStream(szForm, strlen(szForm));
                if (lppl->det == detAll) {
                    CalcPctSurvive(lppl, &pct, NULL);
                    pct = Sf32From80((Sf80Sub(Sf80From64(1.0), Sf80From32(pct))));
                    CchSprintf(&szForm[1], "%ld\t%ld\t%d.%d%%", (int32_t)lppl->cMines, (int32_t)lppl->cFactories,
                               LOWORD(Sf80ToI32((Sf80Mul(Sf80From32(pct), Sf80FromI32(100))))),
                               LOWORD(Sf80ToI32((Sf80Mul(
                                   (Sf80Sub(Sf80From32(pct),
                                            Sf80Div(Sf80FromI32((int16_t)LOWORD(Sf80ToI32((Sf80Mul(Sf80From32(pct), Sf80FromI32(100)))))), Sf80From64(100.0)))),
                                   Sf80FromI32(10000))))));
                } else {
                    szForm[2] = '\t';
                    szForm[1] = '\t';
                    szForm[3] = 0;
                    if (gd.fPerPlayerDumps && lppl->uDefGuess != 0) {
                        cch = CchSprintf(&szForm[3], "%d%%", lppl->uDefGuess * 6 + 3);
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
    CchSprintf(szWork, PszGetCompressedString(ids), szFile);
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
                CchSprintf(szFile, "%s.f%d", szBase, idPlayer + 1);
            } else {
                CchSprintf(szFile, "%s.fle", szBase);
            }
            StreamOpen(szFile, mdCreate);
            fOpen = TRUE;
            j = gd.fPerPlayerDumps + 2;
            for (i = 0; i < j; i++) {
                cch = CchGetString(idsFleetNameXYPlanetDestinationBattle + i, szForm);
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
                    strcpy(&szForm[1], rglpbtlplan[lpfl->iplr][lpfl->iplan].szName);
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
    CchSprintf(szWork, PszGetCompressedString(ids), szFile);
    if (fSuccess) {
        AlertSz(szWork, MB_ICONASTERISK);
    } else {
        AlertSz(szWork, MB_ICONHAND);
    }
    fFileErrSilent = fSilentSav;
    penvMem = penvMemSav;
    return;
}

// FDumpCmdLineGame writes the dumps a command line asks for (-dm, -dp, -df)
// from the player turn file it names (szBase), as the Windows game does
// when it opens the file (FOpenGame): universe, then planets, then fleets.
// It returns FALSE if the file isn't a player's turn file it can load.
int16_t FDumpCmdLineGame() {
    char  szFile[256];
    char *pchDot;
    char *pchDir;

    strcpy(szFile, szBase);
    pchDot = strrchr(szFile, '.');
    pchDir = strrchr(szFile, chDirSep);
    if (!pchDot || (pchDir && pchDir > pchDot)) {
        return FALSE;
    }
    *pchDot = 0;
    DestroyCurGame();
    strcpy(szBase, szFile);
    fFileErrSilent = TRUE;
    if (!FLoadGame(szFile, pchDot + 1)) {
        return FALSE;
    }
    fFileErrSilent = FALSE;
    if (game.lid == 0 || idPlayer == iplrNone) {
        return FALSE;
    }
    if (ini.fDumpMap) {
        DumpUniverse();
    }
    if (ini.fDumpPlanets) {
        DumpPlanets();
    }
    if (ini.fDumpFleets) {
        DumpFleets();
    }
    return TRUE;
}
