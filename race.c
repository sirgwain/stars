#include "common.h"

int16_t rgRacePrimaryTrait[10] = {40, 95, 45, 10, -100, -150, 120, 180, 90, -66};
char    rgRW3Spacing[7] = {4, 3, 3, 3, 3, 3, 3};
int16_t rgRaceAdvDisPts[14] = {-235, -25, -159, -201, 40, -240, -155, 160, 240, 255, 325, 180, 70, 30};
char    rgRW3IStat[7] = {0, 1, 2, 3, 4, 5, 6};
int16_t rgRaceDisEnvPts[6] = {150, 330, 540, 780, 1050, 1380};
char    rgRW3Width[7] = {-2, 2, 2, 2, -2, 2, 2};
char    rgRaceStatMax[16] = {25, 15, 25, 25, 25, 15, 25, 6, 2, 2, 2, 2, 2, 2, 9};
char    rgRaceStatMin[16] = {7, 5, 5, 5, 5, 2, 5};

int16_t GetRaceStat(PLAYER *pplr, RaceStat iStat) { return pplr->rgAttr[iStat]; }

int16_t SetRaceStat(PLAYER *pplr, RaceStat iStat, int16_t iVal) {
    if (iVal < rgRaceStatMin[iStat]) {
        iVal = rgRaceStatMin[iStat];
    }
    if (iVal > rgRaceStatMax[iStat]) {
        iVal = rgRaceStatMax[iStat];
    }
    pplr->rgAttr[iStat] = iVal;
    return iVal;
}

int16_t GetRaceGrbit(PLAYER *pplr, RaceGrbit ibit) {
    if (1 << ibit & pplr->grbitAttr) {
        return TRUE;
    }
    return FALSE;
}

void SetRaceGrbit(PLAYER *pplr, RaceGrbit ibit, int16_t fSet) {
    uint32_t grMask;

    grMask = 1 << ibit;
    if (fSet) {
        pplr->grbitAttr |= grMask;
    } else {
        pplr->grbitAttr &= ~grMask;
    }
    return;
}

void BoundsCheckPlayer(PLAYER *pplr) {
    int16_t i;

    for (i = 0; i < 3; i++) {
        if (pplr->rgEnvVarMin[i] == envImmune) {
            if (pplr->rgEnvVarMax[i] != envImmune || pplr->rgEnvVar[i] != envImmune) {
                pplr->rgEnvVar[i] = envImmune;
                pplr->rgEnvVarMax[i] = envImmune;
                pplr->fHacker = TRUE;
            }
        } else {
            if (pplr->rgEnvVarMin[i] < 0) {
                pplr->rgEnvVarMin[i] = 0;
                pplr->fHacker = TRUE;
            }
            if (pplr->rgEnvVarMin[i] > 100) {
                pplr->rgEnvVarMin[i] = 100;
                pplr->fHacker = TRUE;
            }
            if (pplr->rgEnvVarMax[i] > 100) {
                pplr->rgEnvVarMax[i] = 100;
                pplr->fHacker = TRUE;
            }
            if (pplr->rgEnvVarMax[i] < pplr->rgEnvVarMin[i]) {
                pplr->rgEnvVarMax[i] = pplr->rgEnvVarMin[i];
                pplr->fHacker = TRUE;
            }
            if (pplr->rgEnvVar[i] != pplr->rgEnvVarMin[i] + (int16_t)(pplr->rgEnvVarMax[i] - pplr->rgEnvVarMin[i]) / 2) {
                pplr->rgEnvVar[i] = pplr->rgEnvVarMin[i] + (int16_t)(pplr->rgEnvVarMax[i] - pplr->rgEnvVarMin[i]) / 2;
                pplr->fHacker = TRUE;
            }
        }
    }
    if (pplr->pctIdealGrowth > 20) {
        pplr->pctIdealGrowth = 20;
        pplr->fHacker = TRUE;
    }
    for (i = 0; i < 16; i++) {
        if (pplr->rgAttr[i] < rgRaceStatMin[i]) {
            pplr->rgAttr[i] = rgRaceStatMin[i];
            pplr->fHacker = TRUE;
        }
        if (pplr->rgAttr[i] > rgRaceStatMax[i]) {
            pplr->rgAttr[i] = rgRaceStatMax[i];
            pplr->fHacker = TRUE;
        }
    }
    return;
}

int16_t CAdvantagePoints(PLAYER *pplr) {
    int16_t       pctGrowth;
    int16_t       iSpread;
    int32_t       cPoints;
    int16_t       cBad;
    int16_t       cCur;
    int16_t       i;
    int16_t       rgi[3];
    int16_t       cGood;
    int32_t       lInnate;
    RaceAttribute raMajor;
    int16_t       cOperate;
    int16_t       cProduce;

    cPoints = 0;
    cPoints = 1650;
    BoundsCheckPlayer(pplr);
    raMajor = GetRaceStat(pplr, rsMajorAdv);
    lInnate = (int32_t)(LInnateRaceHabitability(pplr) / 2000);
    iSpread = 1 > (20 >= pplr->pctIdealGrowth ? pplr->pctIdealGrowth : 20) ? 1 : 20 < pplr->pctIdealGrowth ? 20 : pplr->pctIdealGrowth;
    if (iSpread != pplr->pctIdealGrowth) {
        iSpread = 1;
        pplr->pctIdealGrowth = 1;
        pplr->fHacker = TRUE;
    }
    pctGrowth = iSpread;
    if (iSpread <= 5) {
        cPoints += (uint32_t)((int16_t)(6 - iSpread) * 4200);
    } else if (iSpread > 13) {
        if (iSpread < 20) {
            iSpread = (iSpread - 13) * 3 + 21;
        } else {
            iSpread = 45;
        }
    } else {
        switch (iSpread) {
        case 6:
            cPoints += 3600;
            break;
        case 7:
            cPoints += 2250;
            break;
        case 8:
            cPoints += 600;
            break;
        case 9:
            cPoints += 225;
        }
        iSpread = (iSpread - 5) * 2 + 5;
    }
    lInnate = (int32_t)(lInnate * iSpread) / 24;
    cPoints -= lInnate;
    cGood = 0;
    for (i = 0; i < 3; i++) {
        if (pplr->rgEnvVar[i] >= 0) {
            cPoints += (int16_t)(abs(pplr->rgEnvVar[i] - 50) * 4);
        } else {
            cGood++;
        }
    }
    if (cGood > 1) {
        cPoints -= 150;
    }
    cOperate = GetRaceStat(pplr, rsFactOperate);
    cProduce = GetRaceStat(pplr, rsFactProd);
    if (cOperate > 10 || cProduce > 10) {
        cOperate = 1 <= cOperate - 9 ? cOperate - 9 : 1;
        cProduce = 1 <= cProduce - 9 ? cProduce - 9 : 1;
        cProduce = (raMajor == raCheapCol ? 3 : 2) * cProduce;
        if (cGood >= 2) {
            cPoints -= (int32_t)((uint32_t)(cOperate * cProduce) * pctGrowth) / 2;
        } else {
            cPoints -= (int32_t)((uint32_t)(cOperate * cProduce) * pctGrowth) / 9;
        }
    }
    i = GetRaceStat(pplr, rsResGen);
    i = i >= 25 ? 25 : i;
    if (i <= 7) {
        cPoints -= 2400;
    } else if (i == 8) {
        cPoints -= 1260;
    } else if (i == 9) {
        cPoints -= 600;
    } else if (i > 10) {
        cPoints += (int16_t)((i - 10) * 120);
    }
    if (raMajor != raMacintosh) {
        rgi[0] = 10 - GetRaceStat(pplr, rsFactProd);
        rgi[1] = 10 - GetRaceStat(pplr, rsFactBuild);
        rgi[2] = 10 - GetRaceStat(pplr, rsFactOperate);
        cCur = 0;
        if (rgi[0] > 0) {
            cCur += 100 * rgi[0];
        } else {
            cCur += 121 * rgi[0];
        }
        if (rgi[1] < 0) {
            cCur -= 55 * rgi[1];
        } else {
            cCur -= rgi[1] * rgi[1] * 60;
        }
        if (rgi[2] > 0) {
            cCur += 40 * rgi[2];
        } else {
            cCur += 35 * rgi[2];
        }
        if (cCur > 700) {
            cCur = (int16_t)(cCur - 700) / 3 + 700;
        }
        if (rgi[2] <= -7) {
            if (rgi[2] >= -11) {
                cCur -= (-6 - rgi[2]) * 30;
            } else if (rgi[2] >= -14) {
                cCur -= (-12 - rgi[2]) * 45 + 225;
            } else {
                cCur -= 360;
            }
        }
        if (rgi[0] <= -3) {
            cCur -= (-2 - rgi[0]) * 20 * 3;
        }
        cPoints += cCur;
        if (GetRaceGrbit(pplr, ibitRaceCheapFact) != 0) {
            cPoints -= 175;
        }
        rgi[0] = 10 - GetRaceStat(pplr, rsMineProd);
        rgi[1] = 3 - GetRaceStat(pplr, rsMineBuild);
        rgi[2] = 10 - GetRaceStat(pplr, rsMineOperate);
        cCur = 0;
        if (rgi[0] > 0) {
            cCur += 100 * rgi[0];
        } else {
            cCur += 169 * rgi[0];
        }
        if (rgi[1] <= 0) {
            cCur -= 65 * rgi[1] - 80;
        } else {
            cCur -= 360;
        }
        if (rgi[2] > 0) {
            cCur += 40 * rgi[2];
        } else {
            cCur += 35 * rgi[2];
        }
        cPoints += cCur;
    } else {
        cPoints += 210;
    }
    cPoints -= rgRacePrimaryTrait[raMajor];
    cBad = 0;
    cGood = 0;
    for (i = 0; i <= 13; i++) {
        if (GetRaceGrbit(pplr, i) != 0) {
            if (rgRaceAdvDisPts[i] < 0) {
                cBad++;
            } else {
                cGood++;
            }
            cPoints += rgRaceAdvDisPts[i];
        }
    }
    if (cBad + cGood > 4) {
        cPoints -= (int16_t)((cBad + cGood) * 10 * (cBad + cGood - 4));
    }
    if (cGood - cBad > 3) {
        cPoints -= (int16_t)((cGood - cBad - 3) * 60);
    }
    if (cBad - cGood > 3) {
        cPoints -= (int16_t)((cBad - cGood - 3) * 40);
    }
    if (GetRaceGrbit(pplr, ibitRaceNoAdvScanner) != 0) {
        switch (raMajor) {
        case raMassAccel:
            cPoints -= 280;
            break;
        case raStealth:
            cPoints -= 200;
            break;
        case raNone:
            cPoints -= 40;
        }
    }
    cCur = 0;
    for (i = 8; i <= 13; i++) {
        cCur += GetRaceStat(pplr, i) - 1;
    }
    if (cCur > 0) {
        cPoints -= (int16_t)(cCur * cCur * 130);
        if (cCur == 6) {
            cPoints += 1430;
        } else if (cCur == 5) {
            cPoints += 520;
        }
    } else if (cCur < 0) {
        cPoints += rgRaceDisEnvPts[-cCur - 1];
        if (-cCur > 4 && GetRaceStat(pplr, rsResGen) < 10) {
            cPoints -= 190;
        }
    }
    if (GetRaceGrbit(pplr, ibitRaceTech3) != 0) {
        cPoints -= 180;
    }
    if (raMajor == raMacintosh && GetRaceStat(pplr, rsTechBonus1) == 2) {
        cPoints -= 100;
    }
    return LOWORD((int32_t)(cPoints / 3));
}

int32_t LInnateRaceHabitability(PLAYER *pplr) {
    int16_t iTry;
    PLANET  pl;
    double  l2;
    int16_t rgSteps[3];
    PLAYER  plrT;
    int16_t rgDelta[3];
    int16_t fTotalTerra;
    int16_t rgInc[3];
    int16_t i;
    int16_t iTerra;
    int16_t j;
    int32_t l1;
    int16_t rgBase[3];
    double  l3;
    int16_t iDelta;
    int32_t pctDesire;
    int16_t k;
    double  lInnate;
    int16_t pctTerra;

    plrT = rgplr[0];
    lInnate = Sf64FromI32(0);
    fTotalTerra = GetRaceGrbit(pplr, ibitRaceTT);
    rgplr[0] = *pplr;
    rgDelta[2] = 0;
    rgDelta[1] = 0;
    rgDelta[0] = 0;
    for (iTerra = 0; iTerra < 3; iTerra++) {
        if (iTerra == 0) {
            pctTerra = 0;
        } else if (iTerra == 1) {
            pctTerra = !fTotalTerra ? 5 : 8;
        } else {
            pctTerra = !fTotalTerra ? 15 : 17;
        }
        for (i = 0; i < 3; i++) {
            if ((pplr->rgEnvVar[i] > 100 || pplr->rgEnvVarMin[i] > 100 || pplr->rgEnvVarMax[i] > 100 || pplr->rgEnvVar[i] < 0 || pplr->rgEnvVarMin[i] < 0 ||
                 pplr->rgEnvVarMax[i] < 0) &&
                (pplr->rgEnvVar[i] != envImmune || pplr->rgEnvVarMin[i] != envImmune || pplr->rgEnvVarMax[i] != envImmune)) {
                pplr->rgEnvVarMax[i] = envImmune;
                pplr->rgEnvVarMin[i] = envImmune;
                pplr->rgEnvVar[i] = envImmune;
                pplr->fHacker = TRUE;
                rgplr[0] = *pplr;
            }
            if (pplr->rgEnvVar[i] < 0) {
                rgBase[i] = 50;
                rgInc[i] = 11;
                rgSteps[i] = 1;
            } else {
                rgBase[i] = pplr->rgEnvVarMin[i] - pctTerra;
                if (rgBase[i] < 0) {
                    rgBase[i] = 0;
                }
                iTry = pplr->rgEnvVarMax[i] + pctTerra;
                if (iTry > 100) {
                    iTry = 100;
                }
                rgInc[i] = iTry - rgBase[i];
                rgSteps[i] = 11;
            }
        }
        l3 = Sf64FromI32(0);
        for (i = 0; i < rgSteps[0]; i++) {
            if (i == 0 || rgSteps[0] <= 1) {
                iTry = rgBase[0];
            } else {
                iTry = (int16_t)(i * rgInc[0]) / (rgSteps[0] - 1) + rgBase[0];
            }
            if (iTerra != 0 && pplr->rgEnvVar[0] >= 0) {
                iDelta = pplr->rgEnvVar[0] - iTry;
                if (abs(iDelta) <= pctTerra) {
                    iDelta = 0;
                } else if (iDelta < 0) {
                    iDelta += pctTerra;
                } else {
                    iDelta -= pctTerra;
                }
                rgDelta[0] = iDelta;
                iTry = pplr->rgEnvVar[0] - iDelta;
            }
            pl.rgEnvVar[0] = iTry;
            l2 = Sf64FromI32(0);
            for (j = 0; j < rgSteps[1]; j++) {
                if (j == 0 || rgSteps[1] <= 1) {
                    iTry = rgBase[1];
                } else {
                    iTry = (int16_t)(j * rgInc[1]) / (rgSteps[1] - 1) + rgBase[1];
                }
                if (iTerra != 0 && pplr->rgEnvVar[1] >= 0) {
                    iDelta = pplr->rgEnvVar[1] - iTry;
                    if (abs(iDelta) <= pctTerra) {
                        iDelta = 0;
                    } else if (iDelta < 0) {
                        iDelta += pctTerra;
                    } else {
                        iDelta -= pctTerra;
                    }
                    rgDelta[1] = iDelta;
                    iTry = pplr->rgEnvVar[1] - iDelta;
                }
                pl.rgEnvVar[1] = iTry;
                l1 = 0;
                for (k = 0; k < rgSteps[2]; k++) {
                    if (k == 0 || rgSteps[2] <= 1) {
                        iTry = rgBase[2];
                    } else {
                        iTry = (int16_t)(k * rgInc[2]) / (rgSteps[2] - 1) + rgBase[2];
                    }
                    if (iTerra != 0 && pplr->rgEnvVar[2] >= 0) {
                        iDelta = pplr->rgEnvVar[2] - iTry;
                        if (abs(iDelta) <= pctTerra) {
                            iDelta = 0;
                        } else if (iDelta < 0) {
                            iDelta += pctTerra;
                        } else {
                            iDelta -= pctTerra;
                        }
                        rgDelta[2] = iDelta;
                        iTry = pplr->rgEnvVar[2] - iDelta;
                    }
                    pl.rgEnvVar[2] = iTry;
                    pctDesire = PctPlanetDesirability(&pl, 0);
                    iDelta = rgDelta[0] + rgDelta[1] + rgDelta[2];
                    if (iDelta > pctTerra) {
                        pctDesire -= (int16_t)(iDelta - pctTerra);
                        if (pctDesire < 0) {
                            pctDesire = 0;
                        }
                    }
                    pctDesire = (uint32_t)(pctDesire * pctDesire);
                    if (iTerra == 0) {
                        pctDesire = (uint32_t)(pctDesire * 7);
                    } else if (iTerra == 1) {
                        pctDesire = (uint32_t)(pctDesire * 5);
                    } else {
                        pctDesire = (uint32_t)(pctDesire * 6);
                    }
                    l1 += pctDesire;
                }
                if (pplr->rgEnvVar[2] >= 0) {
                    l1 = (int32_t)(l1 * rgInc[2]) / 100;
                } else {
                    l1 = (uint32_t)(l1 * 11);
                }
                l2 = Sf64From80((Sf80Add(Sf80FromI32(l1), Sf80From64(l2))));
            }
            if (pplr->rgEnvVar[1] >= 0) {
                l2 = Sf64From80((Sf80Div(Sf80Mul(Sf80From64(l2), Sf80FromI32(rgInc[1])), Sf80FromI32(100))));
            } else {
                l2 = Sf64From80((Sf80Mul(Sf80From64(l2), Sf80FromI32(11))));
            }
            l3 = Sf64From80((Sf80Add(Sf80From64(l3), Sf80From64(l2))));
        }
        if (pplr->rgEnvVar[0] >= 0) {
            l3 = Sf64From80((Sf80Div(Sf80Mul(Sf80From64(l3), Sf80FromI32(rgInc[0])), Sf80FromI32(100))));
        } else {
            l3 = Sf64From80((Sf80Mul(Sf80From64(l3), Sf80FromI32(11))));
        }
        lInnate = Sf64From80((Sf80Add(Sf80From64(lInnate), Sf80From64(l3))));
    }
    if (pplr != rgplr) {
        rgplr[0] = plrT;
    }
    return Sf80ToI32((Sf80Add(Sf80Div(Sf80From64(lInnate), Sf80From64(10.0)), Sf80From64(0.5))));
}

uint16_t IRaceChecksum(PLAYER *pplr) {
    uint16_t  ick;
    uint16_t *p;
    int16_t   i;
    int16_t   cs;
    size_t    cch;
    PLAYER    plr;

    /* Checksum the names as ReadRtPlr restores them, zeroed after the
       terminator. RaceWizardDlg1 reads the names into buffers that can
       still hold a longer earlier name; the original checksummed those
       stale bytes, which aren't saved, so the race file failed its
       checksum when loaded. */
    plr = *pplr;
    cch = strnlen(plr.szName, sizeof(plr.szName));
    memset(plr.szName + cch, 0, sizeof(plr.szName) - cch);
    cch = strnlen(plr.szNames, sizeof(plr.szNames));
    memset(plr.szNames + cch, 0, sizeof(plr.szNames) - cch);
    p = (uint16_t *)&plr;
    cs = 96;
    ick = 0;
    for (i = 0; i < cs; i++) {
        ick ^= p[i];
    }
    return ick;
}

int16_t FWasRaceFile(char *szFile, int16_t fChkPass) {
    int16_t  idsError;
    int32_t  lSaltSav;
    PLAYER   plr;
    jmp_buf *penvMemSav;
    jmp_buf  env;
    int16_t  fRet;
    int16_t  fSav;
    char    *pch;

    idsError = -1;
    fRet = 0;
    fSav = fFileErrSilent;
    fFileErrSilent = TRUE;
    penvMemSav = penvMem;
    penvMem = &env;
    if (setjmp(env) != 0) {
    LBadFile:
        StreamClose();
        penvMem = penvMemSav;
        fFileErrSilent = fSav;
        if (!fFileErrSilent && idsError != -1) {
            /* NATIVE: GenNewGameFromFile passes szWork itself; see FLoadGame. */
            if (szFile != szWork) {
                strcpy(szWork, szFile);
            }
            AlertSz(PszFormatIds(idsError, NULL), MB_ICONHAND);
        }
        return fRet;
    }
    {
        StreamOpen(szFile, mdRead);
        ReadRt();
        if (hdrCur.rt != rtBOF || ((RTBOF *)rgbCur)->verMajor != 2 || ((RTBOF *)rgbCur)->verMinor < 49 || ((RTBOF *)rgbCur)->verMinor >= 85) {
            idsError = idsFileDoesBelongVersionStars;
            fRet = -1;
            goto LBadFile;
        } else {
            wVersFile = ((RTBOF *)rgbCur)->wVersion;
            if (((RTBOF *)rgbCur)->dt == 5) {
                ReadRt();
                if (hdrCur.rt == rtPlr) {
                    idsError = idsGameFileAppearsCorruptUnableLoadFile;
                    ReadRtPlr(&plr, rgbCur);
                    ReadRt();
                    if (hdrCur.rt == rtEOF && RawLoad16(rgbCur) == IRaceChecksum(&plr)) {
                        lSaltSav = lSaltCur;
                        lSaltCur = plr.lSalt;
                        if (fChkPass && !FCheckPassword()) {
                            lSaltCur = lSaltSav;
                            fRet = -1;
                            goto LBadFile;
                        } else {
                            lSaltCur = lSaltSav;
                            if (plr.lSalt != 0) {
                                strcpy(szRacePass, szPassLast);
                            } else {
                                szRacePass[0] = 0;
                            }
                            vplr = plr;
                            /* The original copied the whole path into the
                               16-byte szRaceFile, overrunning the globals
                               after it when a universe definition names a
                               race by path. Keep the file name, as the race
                               dialogs do. */
                            pch = strrchr(szFile, chDirSep);
                            strncpy(szRaceFile, pch ? pch + 1 : szFile, sizeof(szRaceFile) - 1);
                            szRaceFile[sizeof(szRaceFile) - 1] = 0;
                            StreamClose();
                            fFileErrSilent = fSav;
                            penvMem = penvMemSav;
                            return 1;
                        }
                    }
                }
            }
        }
    }
    goto LBadFile;
}

void CreateRandomRace(PLAYER *pplr) {
    int16_t cPts;
    int16_t i;
    int16_t cPass;
    int16_t j;
    int16_t iVal;
    int16_t dAwayNew;
    int16_t dAwayCur;
    int16_t k;
    PLAYER  plrT;

    pplr->szNames[0] = 0;
    iVal = Random(25);
    if (iVal < 4) {
        for (i = 0; i < 3; i++) {
            pplr->rgEnvVarMax[i] = envImmune;
            pplr->rgEnvVarMin[i] = envImmune;
            pplr->rgEnvVar[i] = envImmune;
        }
        pplr->pctIdealGrowth = Random(4) + 2;
    } else if (iVal < 7) {
        for (i = 0; i < 3; i++) {
            pplr->rgEnvVar[i] = 50;
            pplr->rgEnvVarMin[i] = 0;
            pplr->rgEnvVarMax[i] = 100;
        }
        pplr->pctIdealGrowth = Random(4) + 3;
    } else if (iVal < 9) {
        for (i = 0; i < 3; i++) {
            j = Random(2);
            if (i == 2 && pplr->rgEnvVar[0] == pplr->rgEnvVar[1]) {
                j = pplr->rgEnvVar[0] != 0;
            }
            if (j == 0) {
                pplr->rgEnvVar[i] = 50;
                pplr->rgEnvVarMin[i] = 0;
                pplr->rgEnvVarMax[i] = 100;
            } else {
                pplr->pctIdealGrowth = Random(4) + 2;
            }
        }
        pplr->pctIdealGrowth = Random(5) + 2;
    } else {
        for (i = 0; i < 3; i++) {
            j = Random(40) * 2 + 20;
            k = Random(100 - j + 1);
            pplr->rgEnvVar[i] = j / 2 + k;
            pplr->rgEnvVarMin[i] = k;
            pplr->rgEnvVarMax[i] = k + j;
        }
        if (iVal < 12) {
            i = Random(3);
            pplr->rgEnvVarMax[i] = envImmune;
            pplr->rgEnvVarMin[i] = envImmune;
            pplr->rgEnvVar[i] = envImmune;
        } else if (iVal < 14) {
            i = Random(3);
            pplr->rgEnvVar[i] = 50;
            pplr->rgEnvVarMin[i] = 0;
            pplr->rgEnvVarMax[i] = 100;
        } else if (iVal < 17) {
            i = Random(3);
            j = Random(81);
            pplr->rgEnvVar[i] = j + 10;
            pplr->rgEnvVarMin[i] = j;
            pplr->rgEnvVarMax[i] = j + 20;
        }
        pplr->pctIdealGrowth = Random(9) + 7;
    }
    iVal = Random(3);
    for (i = 8; i <= 13; i++) {
        SetRaceStat(pplr, i, iVal != 0 ? Random(3) : 1);
    }
    SetRaceStat(pplr, rsMajorAdv, Random(10));
    iVal = Random(4);
    for (i = 0; i <= 13; i++) {
        SetRaceGrbit(pplr, i, iVal != 0 ? Random(2) : FALSE);
    }
    SetRaceGrbit(pplr, ibitRaceTech3, Random(2));
    SetRaceGrbit(pplr, ibitRaceCheapFact, Random(2));
    iVal = Random(3);
    if (iVal == 0) {
        for (i = 0; i <= 6; i++) {
            pplr->rgAttr[i] = vrgplrDef[0].rgAttr[i];
        }
        pplr->rgAttr[7] = Random(5);
    } else {
        for (i = 0; i <= 7; i++) {
            pplr->rgAttr[i] = rgRaceStatMin[i] + Random(rgRaceStatMax[i] + 1 - rgRaceStatMin[i]);
        }
    }
    if (strcmp(pplr->szName, PszGetCompressedString(idsRandom2)) == 0) {
        CchGetString(idsBerserker + Random(24), pplr->szName);
    }
    cPts = CAdvantagePoints(pplr);
    if (cPts < 0 || cPts > 50) {
        cPass = 0;
        while ((cPts = CAdvantagePoints(pplr)) < 0 || cPts > 50) {
            if (cPass++ > 250) {
                plrT = *pplr;
                *pplr = vrgplrDef[0];
                strcpy(pplr->szName, plrT.szName);
                break;
            }
            iVal = Random(10);
            dAwayCur = -cPts <= cPts - 50 ? cPts - 50 : -cPts;
            if (iVal < 3) {
                i = Random(6);
                j = pplr->rgAttr[i + 8];
                if (j > 0) {
                    pplr->rgAttr[i + 8] = pplr->rgAttr[i + 8] - 1;
                    cPts = CAdvantagePoints(pplr);
                    dAwayNew = -cPts <= cPts - 50 ? cPts - 50 : -cPts;
                    if (dAwayNew < dAwayCur)
                        continue;
                    pplr->rgAttr[i + 8] = j;
                }
                if (j < 2) {
                    pplr->rgAttr[i + 8] = pplr->rgAttr[i + 8] + 1;
                    cPts = CAdvantagePoints(pplr);
                    dAwayNew = -cPts <= cPts - 50 ? cPts - 50 : -cPts;
                    if (dAwayNew >= dAwayCur) {
                        pplr->rgAttr[i + 8] = j;
                    }
                }
            } else if (iVal < 6) {
                iVal = Random(14);
                j = GetRaceGrbit(pplr, iVal);
                for (i = 0; i < 2; i++) {
                    SetRaceGrbit(pplr, iVal, i);
                    cPts = CAdvantagePoints(pplr);
                    dAwayNew = -cPts <= cPts - 50 ? cPts - 50 : -cPts;
                    if (dAwayNew < dAwayCur)
                        break;
                }
                if (i >= 2) {
                    SetRaceGrbit(pplr, iVal, j);
                }
            } else if (iVal < 9) {
                iVal = Random(7);
                j = GetRaceStat(pplr, iVal);
                for (i = -1; i <= 1; i += 2) {
                    SetRaceStat(pplr, iVal, j + i);
                    cPts = CAdvantagePoints(pplr);
                    dAwayNew = -cPts <= cPts - 50 ? cPts - 50 : -cPts;
                    if (dAwayNew < dAwayCur)
                        break;
                }
                if (i > 1) {
                    SetRaceStat(pplr, iVal, j);
                }
            } else if (Random(2) != 0) {
                j = pplr->pctIdealGrowth;
                if (j > 1) {
                    pplr->pctIdealGrowth = j - 1;
                    cPts = CAdvantagePoints(pplr);
                    dAwayNew = -cPts <= cPts - 50 ? cPts - 50 : -cPts;
                    if (dAwayNew < dAwayCur)
                        continue;
                }
                if (j < 15) {
                    pplr->pctIdealGrowth = j + 1;
                    cPts = CAdvantagePoints(pplr);
                    dAwayNew = -cPts <= cPts - 50 ? cPts - 50 : -cPts;
                    if (dAwayNew < dAwayCur)
                        continue;
                }
                pplr->pctIdealGrowth = j;
            } else {
                iVal = Random(3);
                if (pplr->rgEnvVar[iVal] < 0) {
                    j = Random(31);
                    pplr->rgEnvVar[iVal] = j + 35;
                    pplr->rgEnvVarMin[iVal] = j;
                    pplr->rgEnvVarMax[iVal] = j + 70;
                    cPts = CAdvantagePoints(pplr);
                    dAwayNew = -cPts <= cPts - 50 ? cPts - 50 : -cPts;
                    if (dAwayNew >= dAwayCur) {
                        pplr->rgEnvVarMax[iVal] = envImmune;
                        pplr->rgEnvVarMin[iVal] = envImmune;
                        pplr->rgEnvVar[iVal] = envImmune;
                    }
                } else {
                    plrT = *pplr;
                    pplr->rgEnvVarMax[iVal] = envImmune;
                    pplr->rgEnvVarMin[iVal] = envImmune;
                    pplr->rgEnvVar[iVal] = envImmune;
                    cPts = CAdvantagePoints(pplr);
                    dAwayNew = -cPts <= cPts - 50 ? cPts - 50 : -cPts;
                    if (dAwayNew >= dAwayCur) {
                        *pplr = plrT;
                    }
                }
            }
        }
    }
    return;
}

int16_t PctTrueMaxGrowth(int16_t iplr) {
    if (GetRaceStat(&rgplr[iplr], rsMajorAdv) == raCheapCol) {
        return rgplr[iplr].pctIdealGrowth * 2;
    }
    return rgplr[iplr].pctIdealGrowth;
}
