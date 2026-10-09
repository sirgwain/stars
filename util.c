#include "common.h"
#include "version.h"

uint32_t rgcrDrawStars2b[5] = {8355711, 127, 32512, 8323072};
uint32_t rgcrDrawStars2a[5] = {12632256, 255, 65280, 16711680};
uint32_t rgcrDrawStars[5] = {8355711, 16777215, 255, 65280, 16711680};
int32_t  rgDSDivCnt2[5] = {80000, 210000, 310000, 260000};
int32_t  rgDSDivCnt[5] = {28000, 28000, 63000, 95000, 73000};
uint8_t  vrgbTachyon[18] = {100, 95, 93, 91, 90, 89, 88, 87, 86, 86, 85, 84, 84, 83, 83, 82, 82, 81};

int16_t FLookupSelPlanet(PLANET *ppl) {
    if (sel.scan.grobj != grobjPlanet) {
        return FALSE;
    }
    return FLookupPlanet(sel.scan.idpl, ppl);
}

int16_t FDupPlanet(PLANET *lppl, PLANET *ppl) {
    PLPROD *lpplprodT;

    lpplprodT = ppl->lpplprod;
    *ppl = *lppl;
    ppl->lpplprod = lpplprodT;
    if (!lppl->lpplprod) {
        if (ppl->lpplprod) {
            FreePl((PL *)ppl->lpplprod);
            ppl->lpplprod = NULL;
        }
        return TRUE;
    }
    if (!ppl->lpplprod) {
        ppl->lpplprod = (PLPROD *)LpplAlloc(4, lppl->lpplprod->iprodMax, htOrd);
    } else if ((int16_t)ppl->lpplprod->iprodMax < lppl->lpplprod->iprodMac) {
        ppl->lpplprod = (PLPROD *)LpplReAlloc((PL *)ppl->lpplprod, lppl->lpplprod->iprodMax);
    }
    memcpy(ppl->lpplprod->rgprod, lppl->lpplprod->rgprod, lppl->lpplprod->iprodMac * 4);
    ppl->lpplprod->iprodMac = lppl->lpplprod->iprodMac;
    return TRUE;
}

THING *LpthFromId(int16_t idth) {
    THING *lpth;
    THING *lpthMac;

    lpth = lpThings;
    lpthMac = lpThings + cThing;
    for (; lpth < lpthMac; lpth++) {
        if (lpth->idFull == idth) {
            return lpth;
        }
    }
    return NULL;
}

PLANET *LpplFromId(int16_t idPlanet) {
    int16_t idGuess;
    int16_t iLo;
    PLANET *lppl;
    int16_t iGuess;
    int16_t iHi;

    if (idPlanet < 0 || idPlanet >= game.cPlanMax) {
        return NULL;
    }
    if (cPlanet == game.cPlanMax) {
        lppl = lpPlanets + idPlanet;
        return lppl;
    }
    iLo = -1;
    iHi = cPlanet;
    while (iLo + 1 < iHi) {
        iGuess = (iLo + iHi) >> 1;
        lppl = lpPlanets + iGuess;
        idGuess = lpPlanets[iGuess].id;
        if (idGuess < idPlanet) {
            iLo = iGuess;
        } else {
            if (idGuess <= idPlanet) {
                return lppl;
            }
            iHi = iGuess;
        }
    }
    return NULL;
}

void CalcPctSurvive(PLANET *lppl, float *ppct, float *ppctSmart) {
    int16_t iPlrSav;
    int32_t cDefenses;
    float   pct;
    PART    part;
    int16_t cMax;

    if (ppctSmart) {
        *ppctSmart = Sf32From64(1.0);
    }
    if (lppl->iPlayer != iplrNone && lppl->cDefenses != 0) {
        iPlrSav = idPlayer;
        idPlayer = lppl->iPlayer;
        if (FGetBestDefensePart(&part)) {
            cDefenses = lppl->cDefenses;
            cMax = CMaxOperableDefenses(lppl, lppl->iPlayer, FALSE);
            if (cMax < cDefenses) {
                cDefenses = cMax;
            }
            pct = Sf32From64(
                Sf64Pow(Sf64From80((Sf80Sub(Sf80From64(1.0), Sf80Div(Sf80FromI32(part.pplanetary->grAbility), Sf80From64(1000.0))))), Sf64FromI32(cDefenses)));
            if (ppctSmart) {
                *ppctSmart = Sf32From64(Sf64Pow(Sf64From80((Sf80Sub(Sf80From64(1.0), Sf80Div(Sf80FromI32(part.pplanetary->grAbility), Sf80From64(2000.0))))),
                                                Sf64FromI32(cDefenses)));
            }
        } else {
            pct = Sf32From64(1.0);
        }
        idPlayer = iPlrSav;
    } else {
        pct = Sf32From64(1.0);
    }
    *ppct = pct;
    return;
}

int16_t FLookupPlanet(int16_t iPlanet, PLANET *ppl) {
    PLANET *lpPl;
    int16_t fWrite;

    fWrite = FALSE;
    if (cPlanet <= 0) {
        return FALSE;
    }
    if (iPlanet < 0) {
        iPlanet = ppl->id;
        if (iPlanet == idplNone) {
            LogChangePlanet(NULL, ppl);
            return TRUE;
        }
        fWrite = TRUE;
    }
    lpPl = LpplFromId(iPlanet);
    if (lpPl && ppl) {
        if (fWrite) {
            InvalidateReport(rptPlanets, 0);
            LogChangePlanet(lpPl, ppl);
            if (lpPl->lpplprod != ppl->lpplprod) {
                if (!lpPl->lpplprod) {
                    if (ppl->lpplprod) {
                        lpPl->lpplprod = (PLPROD *)LpplAlloc(4, ppl->lpplprod->iprodMac, htOrd);
                    }
                } else if (!ppl->lpplprod) {
                    FreePl((PL *)lpPl->lpplprod);
                    lpPl->lpplprod = NULL;
                    goto FinishCopy;
                }
                if ((int16_t)lpPl->lpplprod->iprodMax < ppl->lpplprod->iprodMac) {
                    lpPl->lpplprod = (PLPROD *)LpplReAlloc((PL *)lpPl->lpplprod, ppl->lpplprod->iprodMac + 2);
                }
                memcpy(lpPl->lpplprod->rgprod, ppl->lpplprod->rgprod, ppl->lpplprod->iprodMac * 4);
                lpPl->lpplprod->iprodMac = ppl->lpplprod->iprodMac;
            }
        FinishCopy:
            memcpy(lpPl, ppl, 52);
            if (gd.fTutorial && idPlayer == 0) {
                AdvanceTutor();
            }
        } else if (ppl == &sel.pl) {
            FDupPlanet(lpPl, ppl);
        } else {
            *ppl = *lpPl;
        }
    }
    if (lpPl) {
        return TRUE;
    }
    return FALSE;
}

int32_t DpOfLpflIshdef(FLEET *lpfl, int16_t ishdef) {
    int16_t dpShdef;
    int32_t dp;

    dpShdef = rglpshdef[lpfl->iPlayer][ishdef].hul.dp;
    dp = (int32_t)(lpfl->rgcsh[ishdef] * (int16_t)((int16_t)(lpfl->rgdv[ishdef].pctSh * dpShdef) / 10 * lpfl->rgdv[ishdef].pctDp)) / 5000;
    return dp;
}

int16_t FLookupThing(int16_t idth, THING *pth) {
    THING  *lpth;
    int16_t fWrite;

    fWrite = FALSE;
    if (cThing <= 0) {
        return FALSE;
    }
    if (idth < 0) {
        idth = pth->idFull;
        fWrite = TRUE;
    }
    lpth = LpthFromId(idth);
    if (lpth && pth) {
        if (fWrite) {
            LogChangeThing(lpth, pth);
            memcpy(lpth, pth, sizeof(THING));
            if (gd.fTutorial && idPlayer == 0) {
                AdvanceTutor();
            }
        } else {
            *pth = *lpth;
        }
    }
    if (lpth) {
        return TRUE;
    }
    return FALSE;
}

int32_t LComputePower(SHDEF *lpshdef) {
    int16_t dSpeed;
    int16_t dxRange;
    int16_t ihs;
    int32_t dpTorps;
    int16_t i;
    int32_t pctCap;
    int32_t dpBeams;
    int32_t dpBombs;
    int32_t dp;
    PART    part;

    dpBombs = 0;
    dpBeams = 0;
    dpTorps = 0;
    dxRange = 999;
    pctCap = 1000;
    for (ihs = 0; ihs < lpshdef->hul.chs; ihs++) {
        part.hs = lpshdef->hul.rghs[ihs];
        if (part.hs.cItem != 0 && FLookupPart(&part)) {
            switch (part.hs.grhst) {
            default:
                break;
            case hstBeam:
                dp = (int32_t)((uint32_t)(part.pbeam->dp * part.hs.cItem) * (int16_t)(part.pbeam->dRangeMax + 3)) / 4;
                if (part.pbeam->grfAbilities & beamSapper) {
                    dp = (int32_t)(dp / 3);
                }
                dpBeams += dp;
                break;
            case hstTorp:
                dpTorps += (int32_t)((uint32_t)(part.ptorp->dp * part.hs.cItem) * (int16_t)(part.ptorp->dRangeMax - 2)) / 2;
                break;
            case hstBomb:
                dpBombs += (uint32_t)((part.pbomb->dDmgCol + part.pbomb->dDmgBldg) * part.hs.cItem * 2);
                break;
            case hstSpecialE:
                if (part.hs.iItem == ispecialEEnergyCapacitor || part.hs.iItem == ispecialEFluxCapacitor) {
                    for (i = part.hs.cItem; i > 0; i--) {
                        pctCap = (int32_t)(pctCap * (part.pspecial->grAbility + 100)) / 100;
                    }
                }
            }
        }
    }
    if (pctCap != 1000) {
        pctCap = (int32_t)(pctCap / 10);
        if (pctCap > 255) {
            pctCap = 255;
        }
        dpBeams = (int32_t)(dpBeams * pctCap) / 100;
    }
    dSpeed = SpdOfShip(NULL, 0, NULL, FALSE, lpshdef);
    dpBeams += (int32_t)(dpBeams * (int16_t)(dSpeed - 4)) / 10;
    return dpBombs + dpBeams + dpTorps;
}

void ComputeShdefPowers() {
    int16_t iplr;
    int16_t ishdef;

    for (iplr = 0; iplr < game.cPlayer; iplr++) {
        if (rglpshdef[iplr]) {
            for (ishdef = 0; ishdef < 16; ishdef++) {
                if (!rglpshdef[iplr][ishdef].fFree) {
                    rglpshdef[iplr][ishdef].lPower = LComputePower(rglpshdef[iplr] + ishdef);
                }
            }
        }
    }
    return;
}

int32_t DpShieldOfShdef(SHDEF *lpshdef, int16_t iplr) {
    int16_t chs;
    HS     *lphs;
    int16_t ihs;
    int32_t dpShdef;
    HUL    *lphul;
    PART    part;

    dpShdef = 0;
    lphul = &lpshdef->hul;
    lphs = lphul->rghs;
    chs = lphul->chs;
    ihs = 0;
    while (ihs < chs) {
        if (lphs->grhst == hstShield && lphs->cItem > 0) {
            part.hs = *lphs;
            FLookupPart(&part);
            dpShdef += (uint32_t)(part.pshield->dp * lphs->cItem);
        } else if (lphs->grhst == hstArmor && lphs->cItem > 0 && lphs->iItem == iarmorFieldedKelarium) {
            dpShdef += (uint32_t)(lphs->cItem * 50);
        } else if (lphs->grhst == hstArmor && lphs->iItem == iarmorMegaPolyShell) {
            dpShdef += (uint32_t)(lphs->cItem * 100);
        }
        ihs++;
        lphs++;
    }
    if (GetRaceGrbit(&rgplr[iplr], ibitRaceRegeneratingShields) != 0) {
        dpShdef += (int32_t)(dpShdef * 2) / 5;
    }
    if (dpShdef & 0xffff0000) {
        dpShdef = 65535;
    }
    return (uint32_t)LOWORD(dpShdef);
}

int16_t IStargateFromLppl(PLANET *lppl) {
    int16_t chs;
    HS     *lphs;
    int16_t ihs;
    HUL    *lphul;

    if (!lppl || !lppl->fStarbase) {
        return -1;
    }
    lphul = &rglpshdefSB[lppl->iPlayer][lppl->isb].hul;
    lphs = lphul->rghs;
    chs = lphul->chs;
    ihs = 0;
    while (ihs < chs) {
        if (lphs->grhst == hstSpecialSB && lphs->cItem > 0 && lphs->iItem < ispecialSBMassDriver5) {
            return lphs->iItem;
        }
        ihs++;
        lphs++;
    }
    return -1;
}

char *PszPlayerName(int16_t iPlayer, int16_t fCapital, int16_t fPlural, int16_t fThe, int16_t grWord, PLAYER *pplr) {
    char *pchEnd;
    char  szName[50];

    if (!pplr) {
        pplr = &rgplr[iPlayer];
    }
    if (pplr->szName[0] != 0) {
        if (fThe) {
            strcpy(szName, "the ");
            if (fCapital) {
                szName[0] = 'T';
            }
        } else {
            szName[0] = 0;
        }
        if (fPlural && pplr->szNames[0] != 0) {
            strcat(szName, pplr->szNames);
        } else {
            strcat(szName, pplr->szName);
        }
        pchEnd = &szName[strlen(szName) - 1];
        while (*pchEnd == ' ' && pchEnd >= szName) {
            *pchEnd-- = 0;
        }
        if (pchEnd < szName) {
            CchGetString(idsName, szName);
        }
        if (fPlural && pplr->szNames[0] == 0) {
            pchEnd = &szName[strlen(szName) - 1];
            if (*pchEnd != 's' && (*pchEnd != 'e' || pchEnd[-1] != 's')) {
                strcat(szName, "s");
            }
        }
        if (grWord == 1) {
            CchGetString(idsHave2, &szName[strlen(szName)]);
        } else if (grWord == 2) {
            CchGetString(idsAre, &szName[strlen(szName)]);
        }
    } else {
        CchSprintf(szName, PszGetCompressedString(idsPlayerD2), iPlayer + 1);
        if (!fPlural) {
            strcat(szName, "'s");
        }
        if (grWord == 1) {
            CchGetString(idsHas, &szName[strlen(szName)]);
        } else if (grWord == 2) {
            CchGetString(idsIs2, &szName[strlen(szName)]);
        }
    }
    strcpy(szWork, szName);
    return szWork;
}

int16_t FCalcFleetBombDamage(FLEET *lpfl, int32_t *pdmgPeople, int32_t *pdmgPeopleMin, int32_t *pdmgPeopleSmart, int32_t *pdmgBldg, int32_t *ppctTerra,
                             int16_t *pfMulti) {
    int16_t iplr;
    int16_t cfl;
    FLEET  *lpflNext;
    int16_t dmgFloor;
    FLEET  *lpflHead;
    double  dmgSmart;
    int16_t fBomber;
    int16_t j;
    int16_t ishdef;
    PART    part;
    int32_t cIter;
    double  dmgT;

    iplr = lpfl->iPlayer;
    cfl = 0;
    dmgSmart = 1.0;
    lpflHead = lpfl;
    *ppctTerra = 0;
    *pdmgPeopleMin = 0;
    *pdmgBldg = 0;
    *pdmgPeopleSmart = 0;
    *pdmgPeople = 0;
    while (lpfl) {
        fBomber = FALSE;
        lpfl->fBombed = TRUE;
        for (ishdef = 0; ishdef < 16; ishdef++) {
            if (lpfl->rgcsh[ishdef] > 0) {
                for (j = 0; j < rglpshdef[iplr][ishdef].hul.chs; j++) {
                    if (rglpshdef[iplr][ishdef].hul.rghs[j].grhst == hstBomb) {
                        part.hs = rglpshdef[iplr][ishdef].hul.rghs[j];
                        FLookupPart(&part);
                        fBomber = TRUE;
                        if (part.hs.iItem == ibombRetroBomb) {
                            *ppctTerra += (uint32_t)(part.hs.cItem * lpfl->rgcsh[ishdef]);
                        } else if (part.hs.cItem > 0) {
                            if (part.pbomb->dDmgBldg == 0) {
                                cIter = (uint32_t)(part.hs.cItem * lpfl->rgcsh[ishdef]);
                                dmgT = Sf64From80((Sf80Sub(Sf80From64(1.0), Sf80Div(Sf80FromI32(part.pbomb->dDmgCol), Sf80From64(1000.0)))));
                                while (cIter-- > 0) {
                                    dmgSmart = Sf64From80((Sf80Mul(Sf80From64(dmgSmart), Sf80From64(dmgT))));
                                }
                            } else {
                                *pdmgPeople += (uint32_t)((uint32_t)(part.hs.cItem * lpfl->rgcsh[ishdef]) * part.pbomb->dDmgCol);
                                *pdmgBldg += (uint32_t)((uint32_t)(part.hs.cItem * lpfl->rgcsh[ishdef]) * part.pbomb->dDmgBldg);
                                if (part.hs.iItem >= ibombLadyFingerBomb && part.hs.iItem <= ibombCherryBomb) {
                                    dmgFloor = 3;
                                    *pdmgPeopleMin += (uint32_t)((uint32_t)(part.hs.cItem * lpfl->rgcsh[ishdef]) * dmgFloor);
                                }
                            }
                        }
                    } else if (rglpshdef[iplr][ishdef].hul.rghs[j].grhst == hstBeam &&
                               rglpshdef[iplr][ishdef].hul.rghs[j].iItem == ibeamMultiContainedMunition) {
                        part.hs = rglpshdef[iplr][ishdef].hul.rghs[j];
                        fBomber = TRUE;
                        *pdmgPeople += (uint32_t)((uint32_t)(part.hs.cItem * lpfl->rgcsh[ishdef]) * 20);
                        *pdmgBldg += (uint32_t)((uint32_t)(part.hs.cItem * lpfl->rgcsh[ishdef]) * 5);
                        *pdmgPeopleMin += (uint32_t)((uint32_t)(part.hs.cItem * lpfl->rgcsh[ishdef]) * 3);
                    } else if (rglpshdef[iplr][ishdef].hul.rghs[j].grhst == hstSpecialM &&
                               rglpshdef[iplr][ishdef].hul.rghs[j].iItem == ispecialMOrbitalConstructionModule) {
                        part.hs = rglpshdef[iplr][ishdef].hul.rghs[j];
                        fBomber = TRUE;
                        *pdmgPeopleMin += (uint32_t)((uint32_t)(part.hs.cItem * lpfl->rgcsh[ishdef]) * 20);
                    }
                }
            }
        }
        cfl++;
        for (lpflNext = lpfl->lpflNext; lpflNext && (lpflNext->fDead || lpflNext->iPlayer != iplr); lpflNext = lpflNext->lpflNext) {
        }
        if (lpflNext && lpflNext != lpflHead && lpflNext->idPlanet == lpfl->idPlanet) {
            lpfl = lpflNext;
        } else {
            lpfl = NULL;
        }
    }
    *pdmgPeopleSmart = Sf80ToI32((Sf80Add(Sf80Sub(Sf80From64(1000.0), Sf80Mul(Sf80From64(dmgSmart), Sf80From64(1000.0))), Sf80From64(0.5))));
    if (*pdmgPeopleSmart >= 1000) {
        *pdmgPeopleSmart = 1000;
    }
    *pfMulti = cfl > 1;
    if (*pdmgPeople != 0 || *pdmgPeopleMin != 0 || *pdmgPeopleSmart != 0 || *pdmgBldg != 0 || *ppctTerra != 0) {
        return TRUE;
    }
    return FALSE;
}

void LinkFleets(int16_t fUnused) {
    FLEET **pSearch;
    POINT16 pt;
    FLEET  *rglpflSrc[63];
    FLEET  *lpflTail;
    FLEET  *lpflHead;
    int16_t i;
    int16_t iflTail;
    int16_t iflHead;
    int16_t cSrc;

    for (iflHead = 0; iflHead < cFleet; iflHead++) {
        lpflHead = rglpfl[iflHead];
        if (!rglpfl[iflHead])
            break;
        lpflHead->lpflNext = NULL;
        lpflHead->fDone = FALSE;
    }
    cSrc = 0;
    for (iflHead = 0; iflHead < cFleet; iflHead++) {
        lpflHead = rglpfl[iflHead];
        if (!lpflHead->fDead && !lpflHead->lpflNext) {
            for (i = 0; i < cSrc; i++) {
                if (lpflHead->pt.y <= rglpflSrc[i]->pt.y && (lpflHead->pt.y < rglpflSrc[i]->pt.y || (uint16_t)lpflHead->pt.x <= (uint16_t)rglpflSrc[i]->pt.x)) {
                    if (lpflHead->pt.x == rglpflSrc[i]->pt.x && lpflHead->pt.y == rglpflSrc[i]->pt.y) {
                        lpflHead->lpflNext = rglpflSrc[i]->lpflNext;
                        rglpflSrc[i]->lpflNext = lpflHead;
                        i = -1;
                    }
                    break;
                }
            }
            if (i != -1) {
                lpflHead->lpflNext = lpflHead;
                if (i < cSrc) {
                    memmove(&rglpflSrc[i + 1], &rglpflSrc[i], (cSrc - i) * sizeof(FLEET *));
                }
                rglpflSrc[i] = lpflHead;
                cSrc++;
                if (cSrc >= 63) {
                    for (iflTail = iflHead + 1; iflTail < cFleet; iflTail++) {
                        lpflTail = rglpfl[iflTail];
                        if (!lpflTail->fDead && !lpflTail->lpflNext) {
                            pt = lpflTail->pt;
                            pSearch = bsearch(&pt, rglpflSrc, cSrc, sizeof(FLEET *), (QSORTCOMPARE)ICompFleetPoint2);
                            if (pSearch) {
                                lpflHead = *pSearch;
                                lpflTail->lpflNext = lpflHead->lpflNext;
                                lpflHead->lpflNext = lpflTail;
                            }
                        }
                    }
                    cSrc = 0;
                }
            }
        }
    }
    gd.fFleetLinkValid = TRUE;
    return;
}

int ICompFleetPoint(FLEET **ppfl1, FLEET **ppfl2) {
    int32_t l2;
    int32_t l1;

    l1 = ((uint32_t)(*ppfl1)->pt.x & 0xffff) | ((uint32_t)(*ppfl1)->pt.y & 0xffff) << 0x10;
    l2 = ((uint32_t)(*ppfl2)->pt.x & 0xffff) | ((uint32_t)(*ppfl2)->pt.y & 0xffff) << 0x10;
    l1 -= l2;
    if (l1 < 0) {
        l1 = -1;
    } else if (l1 > 0) {
        l1 = 1;
    }
    return (int16_t)LOWORD(l1);
}

int ICompFleetPoint2(int32_t *pl, FLEET **ppfl) {
    int32_t l2;
    int32_t l1;

    l1 = *pl;
    l2 = ((uint32_t)(*ppfl)->pt.x & 0xffff) | ((uint32_t)(*ppfl)->pt.y & 0xffff) << 0x10;
    l1 -= l2;
    if (l1 < 0) {
        l1 = -1;
    } else if (l1 > 0) {
        l1 = 1;
    }
    return (int16_t)LOWORD(l1);
}

int16_t FLookupSelShip(FLEET *pfl) {
    if (sel.scan.grobj != grobjFleet) {
        return FALSE;
    }
    return FLookupFleet(rglpfl[sel.scan.ifl]->id, pfl);
}

FLEET *LpflFromId(int16_t idFleet) {
    int16_t iplr;
    int16_t idGuess;
    int16_t iLo;
    int16_t iGuess;
    int16_t i;
    FLEET  *lpfl;
    int16_t iplrCur;
    int16_t iHi;

    i = 0;
    iplr = (uint16_t)idFleet >> 9 & 0xf;
    idFleet &= 0x1fff;
    for (iplrCur = 0; iplrCur < iplr; iplrCur++) {
        i += rgplr[iplrCur].cFleet;
    }
    iLo = i - 1;
    iHi = cFleet;
    while (iLo + 1 < iHi) {
        iGuess = (iLo + iHi) >> 1;
        lpfl = rglpfl[iGuess];
        idGuess = rglpfl[iGuess]->id;
        if (idGuess < idFleet) {
            iLo = iGuess;
        } else {
            if (idGuess <= idFleet) {
                return lpfl;
            }
            iHi = iGuess;
        }
    }
    return NULL;
}

int16_t FLookupFleet(int16_t idFleet, FLEET *pfl) {
    FLEET  *lpfl;
    int16_t fWrite;

    fWrite = FALSE;
    if (cFleet <= 0) {
        return FALSE;
    }
    if (idFleet < 0) {
        idFleet = pfl->id;
        fWrite = TRUE;
    }
    lpfl = LpflFromId(idFleet);
    if (lpfl && pfl) {
        if (fWrite) {
            InvalidateReport(rptFleets, 0);
            LogChangeFleet(lpfl, pfl);
            if (lpfl->lpplord != pfl->lpplord) {
                if (lpfl->lpplord->iordMax < pfl->cord) {
                    lpfl->lpplord = (PLORD *)LpplReAlloc((PL *)lpfl->lpplord, pfl->cord + 3);
                }
                memcpy(lpfl->lpplord->rgord, pfl->lpplord->rgord, pfl->lpplord->iordMac * 18);
                lpfl->lpplord->iordMac = pfl->lpplord->iordMac;
            }
            memcpy(lpfl, pfl, 100);
            if (gd.fTutorial && idPlayer == 0) {
                AdvanceTutor();
            }
        } else if (pfl == &sel.fl) {
            FDupFleet(lpfl, pfl);
        } else {
            *pfl = *lpfl;
        }
    }
    if (lpfl) {
        return TRUE;
    }
    return FALSE;
}

int16_t FDupFleet(FLEET *lpfl, FLEET *pfl) {
    PLORD *lpplordT;

    lpplordT = pfl->lpplord;
    *pfl = *lpfl;
    if (!lpfl->lpplord) {
        if (lpplordT) {
            FreePl((PL *)lpplordT);
        }
        return TRUE;
    }
    pfl->lpplord = lpplordT;
    if (!pfl->lpplord) {
        pfl->lpplord = (PLORD *)LpplAlloc(18, lpfl->lpplord->iordMax, htOrd);
    } else if ((int16_t)pfl->lpplord->iordMax < lpfl->lpplord->iordMac) {
        pfl->lpplord = (PLORD *)LpplReAlloc((PL *)pfl->lpplord, lpfl->lpplord->iordMax);
    }
    memcpy(pfl->lpplord->rgord, lpfl->lpplord->rgord, lpfl->lpplord->iordMac * 18);
    pfl->lpplord->iordMac = lpfl->lpplord->iordMac;
    return TRUE;
}

int16_t FLookupObject(GrobjClass grobj, int16_t id, void *pobj) {
    if (grobj == grobjFleet) {
        return FLookupFleet(id, pobj);
    }
    if (grobj == grobjPlanet) {
        return FLookupPlanet(id, pobj);
    }
    return FLookupThing(id, pobj);
}

int16_t FLookupOrbitingXfer(int16_t idPlanet, int16_t iNth, XFER *pxf, int16_t idSkip) {
    int16_t i;
    THING  *lpth;
    FLEET  *lpfl;
    THING  *lpthMac;

    if (cFleet <= 0) {
        return FALSE;
    }
    if (cFleet != 0) {
        for (i = 0; i < cFleet; i++) {
            lpfl = rglpfl[i];
            if (!rglpfl[i])
                break;
            if (lpfl->idPlanet == idPlanet && lpfl->id != idSkip &&
                ((idSkip == idflNone || (lpfl->pt.x == sel.pt.x && lpfl->pt.y == sel.pt.y)) && iNth-- == 0)) {
                if (pxf) {
                    pxf->fl = *lpfl;
                    pxf->grobj = grobjFleet;
                    pxf->id = lpfl->id;
                }
                return TRUE;
            }
        }
    }
    lpth = lpThings;
    lpthMac = lpThings + cThing;
    for (; lpth < lpthMac; lpth++) {
        if (lpth->ith == ithMineralPacket && lpth->pt.x == sel.pt.x && lpth->pt.y == sel.pt.y && iNth-- == 0) {
            if (pxf) {
                pxf->th = *lpth;
                pxf->grobj = grobjThing;
                pxf->id = lpth->idFull;
            }
            return TRUE;
        }
    }
    return FALSE;
}

char *PszGetThingName(int16_t id) {
    THING *lpth;
    char   szPlr[54];

    lpth = LpthFromId(id);
    if (!lpth) {
        szWork[0] = 0;
        return szWork;
    }
    switch (lpth->ith) {
    case ithMinefield:
        if (lpth->iplr != idPlayer) {
            CchSprintf(szPlr, "%s ", PszPlayerName(lpth->iplr, FALSE, FALSE, FALSE, 0, NULL));
        } else {
            szPlr[0] = 0;
        }
        CchSprintf(szWork, PszGetCompressedString(idsSSMineField), szPlr, rgszMineField[lpth->thm.iType]);
        break;
    case ithMineralPacket:
        if (lpth->thp.iWarp == 0) {
            CchGetString(idsSalvage, szWork);
            return szWork;
        }
        if (lpth->iplr != idPlayer) {
            CchSprintf(szPlr, "%s ", PszPlayerName(lpth->iplr, FALSE, FALSE, FALSE, 0, NULL));
        } else {
            szPlr[0] = 0;
        }
        CchSprintf(szWork, PszGetCompressedString(idsSmineralPacket), szPlr);
        break;
    case ithWormhole:
        strcpy(szWork, PszGetCompressedString(idsWormhole));
        break;
    case ithMysteryTrader:
        strcpy(szWork, PszGetCompressedString(idsMysteryTrader));
        break;
    default:
        strcpy(szWork, PszGetCompressedString(idsMysteryObject));
    }
    return szWork;
}

char *PszGetFleetName(int16_t id) {
    int16_t cshdef;
    int16_t iplr;
    char   *lpsz;
    char    szShdef[34];
    int16_t ifl;
    FLEET  *lpfl;
    char    szPlr[34];
    int16_t ishdef;
    int16_t cch;

    lpfl = LpflFromId(id & 0x7fff);
    id &= 0x7fff;
    iplr = (uint16_t)id >> 9 & 0xf;
    ifl = id & 0x1ff;
    if (iplr != idPlayer) {
        CchSprintf(szPlr, "%s ", PszPlayerName(iplr, FALSE, FALSE, FALSE, 0, NULL));
    } else {
        szPlr[0] = 0;
    }
    if (lpfl && lpfl->lpszName) {
        CchSprintf(szWork, "%s%s", szPlr, lpfl->lpszName);
    } else {
        if (lpfl) {
            ishdef = IshdefPrimaryFromLpfl(lpfl, &cshdef);
            if (ishdef == 16) {
                lpsz = PszGetCompressedString(idsFleet);
            } else {
                strcpy(szShdef, rglpshdef[iplr][ishdef].hul.szClass);
                cch = strlen(szShdef);
                if (cch > 28) {
                    cch = 28;
                }
                if (cshdef > 1) {
                    szShdef[cch] = '+';
                    szShdef[cch + 1] = 0;
                }
                lpsz = szShdef;
            }
        } else {
            lpsz = PszGetCompressedString(idsFleet);
        }
        CchSprintf(szWork, "%s%s #%d", szPlr, lpsz, ifl + 1);
    }
    return szWork;
}

uint16_t WFromLpfl(FLEET *lpfl) {
    int16_t  cshdef;
    uint16_t w;
    int16_t  ishdef;

    ishdef = IshdefPrimaryFromLpfl(lpfl, &cshdef);
    w = ishdef << 9 | lpfl->ifl;
    if (cshdef > 1) {
        w |= 0x2000;
    }
    return w;
}

char *PszFleetNameFromWord(uint16_t w) {
    char   *lpsz;
    char    szShdef[34];
    int16_t ishdef;
    int16_t cch;

    ishdef = w >> 9 & 0xf;
    if (rglpshdef[idPlayer][ishdef].fFree) {
        lpsz = PszGetCompressedString(idsFleet);
    } else {
        strcpy(szShdef, rglpshdef[idPlayer][ishdef].hul.szClass);
        cch = strlen(szShdef);
        if (cch > 28) {
            cch = 28;
        }
        if (w & 0x2000) {
            szShdef[cch] = '+';
            szShdef[cch + 1] = 0;
        }
        lpsz = szShdef;
    }
    CchSprintf(szWork, "%s #%d", lpsz, (w & 0x1ff) + 1);
    return szWork;
}

char *PszGetPlanetName(int16_t id) {
    int16_t fInOrbit;
    char   *psz;

    fInOrbit = id & 0x8000;
    id &= 0x7fff;
    id = rgidPlan[id];
    psz = PszGetCompressedPlanet(id);
    if (fInOrbit) {
        CchSprintf(szWork, PszGetCompressedString(idsOrbitingS), psz);
    } else {
        strcpy(szWork, psz);
    }
    return szWork;
}

int16_t IflFromLpfl(FLEET *lpfl) {
    int16_t i;

    for (i = 0; i < cFleet; i++) {
        if (rglpfl[i] == lpfl) {
            return i;
        }
    }
    return iflNone;
}

int16_t FDeleteFleet(int16_t idFleet, GrobjClass grobjSel, int16_t idSel) {
    int16_t i;
    FLEET  *lpfl;
    int16_t iPlr;
    int16_t idDel;
    PLANET *lppl;

    for (i = 0; i < cFleet; i++) {
        lpfl = rglpfl[i];
        if (!rglpfl[i] || lpfl->id == idFleet)
            break;
        if (lpfl->id > idFleet) {
            return FALSE;
        }
    }
    if (i == cFleet) {
        return FALSE;
    }
    if (idFleet == sel.fl.id) {
        ShowScanSel(0);
    }
    lpfl->fDead = TRUE;
    FleetOrdersChangeTarget(lpfl);
    FreePl((PL *)lpfl->lpplord);
    if (lpfl->lpszName) {
        FreeLp(lpfl->lpszName, htString);
    }
    cFleet--;
    iPlr = lpfl->iPlayer;
    idDel = lpfl->id;
    rgplr[iPlr].cFleet--;
    if (grobjSel == grobjNone && lpfl->idPlanet != idPlanetDeepSpace) {
        lppl = LpplFromId(lpfl->idPlanet);
        if (lppl && lppl->iPlayer == idPlayer) {
            grobjSel = grobjPlanet;
            idSel = lpfl->idPlanet;
        }
    }
    if (cFleet != i) {
        memmove(rglpfl + i, rglpfl + (i + 1), (cFleet - i) * sizeof(FLEET *));
    }
    FreeLp(lpfl, htFleets);
    gd.fFleetLinkValid = FALSE;
    if (sel.fl.id != idflNone) {
        if (i < sel.scan.ifl) {
            sel.scan.ifl--;
        } else if (idDel == sel.fl.id) {
            if (grobjSel == grobjNone) {
                sel.grobj = grobjNone;
                sel.scan.grobj = grobjNone;
                FFindSomethingAndSelectIt();
                return TRUE;
            }
            sel.grobj = grobjNone;
            if (grobjSel == grobjFleet) {
                SelectAdjFleet(0, idSel);
            } else {
                SelectAdjPlanet(0, idSel);
            }
        }
    }
    if (!gd.fGeneratingTurn) {
        UpdateMsgTitle();
    }
    return TRUE;
}

int16_t FFindSomethingAndSelectIt() {
    PLANET *lpplMac;
    PLANET *lppl;
    int16_t i;
    FLEET  *lpfl;

    lppl = LpplFromId(rgplr[idPlayer].idPlanetHome);
    if (!lppl || lppl->iPlayer != idPlayer) {
        lppl = lpPlanets;
        lpplMac = lpPlanets + cPlanet;
        for (; lppl < lpplMac && lppl->iPlayer != idPlayer; lppl++) {
        }
        if (lppl == lpplMac) {
            lppl = NULL;
        }
    }
    if (lppl) {
        SelectAdjPlanet(0, lppl->id);
        return TRUE;
    }
    for (i = 0; i < cFleet; i++) {
        lpfl = rglpfl[i];
        if (!rglpfl[i])
            break;
        if (lpfl->iPlayer == idPlayer) {
            SelectAdjFleet(0, lpfl->id);
            return TRUE;
        }
    }
    return FALSE;
}

FLEET *LpflNew(int16_t iPlr, int16_t idPl) {
    int16_t i;
    ORDER  *lpord;
    FLEET  *lpfl;
    int16_t iflPrev;

    iflPrev = -1;
    for (i = 0; i < cFleet; i++) {
        lpfl = rglpfl[i];
        if (!rglpfl[i])
            break;
        if (lpfl->iPlayer >= iPlr) {
            if (lpfl->iPlayer > iPlr || lpfl->ifl != iflPrev + 1)
                break;
            iflPrev = lpfl->ifl;
        }
    }
    rglpfl = LpReAlloc(rglpfl, (cFleet + 1) * sizeof(FLEET *), htMisc);
    if (cFleet != i) {
        memmove(rglpfl + (i + 1), rglpfl + i, (cFleet - i) * sizeof(FLEET *));
    }
    rglpfl[i] = lpfl = LpAlloc(sizeof(FLEET), htFleets);
    cFleet++;
    rgplr[iPlr].cFleet++;
    memset(lpfl, 0, sizeof(FLEET));
    lpfl->ifl = iflPrev + 1;
    lpfl->iPlayer = iPlr;
    lpfl->iplr = iPlr;
    lpfl->det = detAll;
    lpfl->idPlanet = idPl;
    if (idPl != idPlanetDeepSpace) {
        lpfl->pt = rgptPlan[idPl];
    }
    lpfl->cord = 1;
    lpfl->fRepOrders = FALSE;
    lpfl->lpplord = (PLORD *)LpplAlloc(18, 3, htOrd);
    lpfl->lpplord->iordMac = 1;
    lpfl->fdirValid = 0;
    lpord = lpfl->lpplord->rgord;
    lpord->pt = lpfl->pt;
    lpord->id = lpfl->idPlanet;
    lpord->grobj = lpfl->idPlanet == idPlanetDeepSpace ? 4 : 1;
    lpord->iWarp = 0;
    lpord->fValidTask = TRUE;
    lpord->grTask = grTaskNone;
    if (sel.scan.ifl != iflNone && i <= sel.scan.ifl) {
        sel.scan.ifl++;
    }
    gd.fFleetLinkValid = FALSE;
    return lpfl;
}

FLEET *LpflNewSplit(FLEET *pfl) {
    int16_t iordMac;
    FLEET  *lpflNew;

    lpflNew = LpflNew(pfl->iPlayer, pfl->idPlanet);
    if (pfl->idPlanet == idPlanetDeepSpace) {
        lpflNew->pt = pfl->pt;
    }
    lpflNew->fRepOrders = pfl->fRepOrders;
    lpflNew->det = pfl->det;
    lpflNew->iplan = pfl->iplan;
    iordMac = pfl->lpplord->iordMac;
    if (lpflNew->lpplord->iordMax < iordMac) {
        lpflNew->lpplord = (PLORD *)LpplReAlloc((PL *)lpflNew->lpplord, pfl->lpplord->iordMax);
    }
    memcpy(lpflNew->lpplord->rgord, pfl->lpplord->rgord, iordMac * 18);
    lpflNew->lpplord->iordMac = iordMac;
    lpflNew->cord = pfl->cord;
    LogSplitFleet(pfl->id);
    return lpflNew;
}

int16_t FFleetMergeAll(FLEET *pfl) {
    int16_t iplr;
    int32_t dpT;
    int16_t fCshOverflow;
    int16_t rgcshDamaged[16];
    int16_t cflMerge;
    int16_t i;
    FLEET  *lpfl;
    int16_t cshT;
    SHDEF  *lpshdef;
    FLEET  *lpflMerge;
    int32_t rgdp[16];
    int16_t j;

    lpflMerge = NULL;
    cflMerge = 0;
    fCshOverflow = FALSE;
    memset(rgdp, 0, 64);
    memset(rgcshDamaged, 0, 32);
    iplr = pfl->iPlayer;
    for (i = 0; i < vcflMerge; i++) {
        if (vrgiflMerge[i] != iflNone) {
            lpfl = LpflFromId(vrgiflMerge[i]);
            /* Ship counts are int16_t. The original clamped a count that
               passed 32767 to 32766 and lost the rest; a fleet that doesn't
               fit is left out of the merge. */
            if (lpfl && lpfl->ifl != pfl->ifl) {
                for (j = 0; j < 16 && (int32_t)pfl->rgcsh[j] + lpfl->rgcsh[j] <= 32767; j++) {
                }
                if (j < 16) {
                    continue;
                }
            }
            if (lpfl) {
                lpshdef = rglpshdef[iplr];
                j = 0;
                while (j < 16) {
                    if (lpfl->rgcsh[j] != 0) {
                        if (lpfl->rgdv[j].dp != 0) {
                            cshT = LOWORD((int32_t)(lpfl->rgcsh[j] * (int16_t)lpfl->rgdv[j].pctSh) / 100);
                            if (cshT == 0) {
                                cshT = 1;
                            }
                            rgcshDamaged[j] += cshT;
                            dpT = (uint32_t)(lpfl->rgdv[j].pctDp * cshT);
                            rgdp[j] += dpT;
                        }
                        if (lpfl->ifl != pfl->ifl) {
                            pfl->rgcsh[j] += lpfl->rgcsh[j];
                            lpfl->rgcsh[j] = 0;
                        }
                    }
                    j++;
                    lpshdef++;
                }
                if (lpfl->ifl == pfl->ifl) {
                    lpflMerge = lpfl;
                } else {
                    for (j = 0; j < 5; j++) {
                        pfl->rgwtMin[j] += lpfl->rgwtMin[j];
                    }
                    FDeleteFleet(lpfl->id, grobjNone, -1);
                    cflMerge++;
                }
            }
        }
    }
    lpshdef = rglpshdef[iplr];
    j = 0;
    while (j < 16) {
        lpflMerge->rgcsh[j] = pfl->rgcsh[j];
        if (rgcshDamaged[j] == 0 || lpflMerge->rgcsh[j] == 0) {
            lpflMerge->rgdv[j].dp = 0;
        } else {
            lpflMerge->rgdv[j].pctSh =
                LOWORD((int32_t)((int32_t)((uint32_t)(rgcshDamaged[j] * 100) + (int16_t)(lpflMerge->rgcsh[j] - 1)) / lpflMerge->rgcsh[j]));
            lpflMerge->rgdv[j].pctDp = LOWORD((int32_t)(rgdp[j] / rgcshDamaged[j]));
        }
        j++;
        lpshdef++;
    }
    for (j = 0; j < 5; j++) {
        lpflMerge->rgwtMin[j] = pfl->rgwtMin[j];
    }
    LogMergeFleet(pfl->id);
    InvalidateReport(rptFleets, 2);
    if (cflMerge != 0) {
        return TRUE;
    }
    return FALSE;
}

int16_t FFleetSplitAll(FLEET *pfl) {
    FLEET   flNew;
    int16_t cSplit;
    int16_t c;
    int16_t i;
    FLEET  *lpflNew;

    cSplit = 0;
    for (i = 0; i < 16; i++) {
        c = pfl->rgcsh[i];
        while (c-- != 0) {
            if (cSplit++ != 0) {
                lpflNew = LpflNewSplit(pfl);
                flNew = *lpflNew;
                pfl->rgcsh[i]--;
                flNew.rgcsh[i]++;
                FleetTransferCargoBalance(pfl, &flNew);
                FLookupFleet(idWriteBack, pfl);
                FLookupFleet(idWriteBack, &flNew);
            }
        }
    }
    InvalidateReport(rptFleets, 2);
    if (cSplit > 1) {
        return TRUE;
    }
    return FALSE;
}

char *PszGetLocName(GrobjClass grobj, int16_t id, int16_t x, int16_t y) {
    if (id == -1)
        goto NoObj;
    if (grobj == grobjPlanet)
        return PszGetPlanetName(id);
    if (grobj == grobjFleet)
        return PszGetFleetName(id);
    if (grobj == grobjThing)
        return PszGetThingName(id);
NoObj:
    if (x == -1 && y == -1) {
        strcpy(szWork, PszGetCompressedString(idsDeepSpace));
    } else {
        CchSprintf(szWork, PszGetCompressedString(idsSpaceDD), x, y);
    }
    return szWork;
}

int16_t IshdefPrimaryFromLpfl(FLEET *lpfl, int16_t *pcDiff) {
    int16_t cDiff;
    int16_t ish;
    int16_t i;
    int16_t csh;
    HulDef  ihul;

    cDiff = 0;
    csh = 0;
    ish = 16;
    for (i = 0; i < 16; i++) {
        if (lpfl->rgcsh[i] > 0) {
            cDiff++;
            if (lpfl->rgcsh[i] > csh) {
                ish = i;
                csh = lpfl->rgcsh[i];
                ihul = rglpshdef[lpfl->iPlayer][i].hul.ihuldef;
                if (ihul == ihuldefFuelTransport || ihul == ihuldefSuperFuelXport) {
                    csh--;
                }
            }
        }
    }
    if (pcDiff) {
        *pcDiff = cDiff;
    }
    return ish;
}

char *PszGetDistance(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
    int32_t d;
    int16_t fStarted;
    int32_t d2;

    fStarted = FALSE;
    d = Sf80ToI32((Sf80Add(Sf80Mul(Sf80From64(DGetDistance(x1, y1, x2, y2)), Sf80From64(100.0)), Sf80From64(0.5))));
    d2 = (int32_t)(d / 100);
    d -= (uint32_t)(d2 * 100);
    if (dyArial8 <= 14) {
        CchSprintf(szWork, PszGetCompressedString(idsLdLdLightYears), d2, d);
    } else {
        CchSprintf(szWork, PszGetCompressedString(idsLdLdLY), d2, d);
    }
    return szWork;
}

double DGetDistance(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
    int32_t dy;
    int32_t dx;
    int32_t l;

    dx = (int16_t)(x2 - x1);
    dy = (int16_t)(y2 - y1);
    l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
    return Sf64Sqrt(Sf64FromI32(l));
}

int16_t FFindNearestObject(POINT16 pt, GrobjClass grobj, SCAN *pscan) {
    POINT16  ptWp;
    POINT16 *ppt;
    int16_t  dy;
    int32_t  lTry;
    THING   *lpth;
    FLEET   *lpfl;
    int16_t  i;
    THING   *lpthMac;
    int32_t  lSquare;
    SCAN     scanT;
    int16_t  iNearest;
    int16_t  dx;
    SCAN     scan;

    iNearest = -1;
    lSquare = 1000000000;
    ppt = rgptPlan;
    scan.grobjFull = grobjNone;
    scan.grobj = grobjNone;
    scan.pt.y = 0;
    scan.pt.x = 0;
    scan.ith = ithNone;
    scan.iwp = iwpNone;
    scan.ifl = iflNone;
    scan.idpl = idplNone;
    if (grobj & mdScanRadius) {
        lSquare = ScanToPt(20);
        lSquare = (uint32_t)(lSquare * lSquare);
    } else if (grobj & mdExact) {
        lSquare = 0;
    }
    if (grobj & grobjPlanet) {
        i = 0;
        while (i < game.cPlanMax) {
            dx = pt.x - ppt->x;
            dy = pt.y - ppt->y;
            lTry = (uint32_t)(dx * dx);
            if ((int32_t)(uint32_t)(dx * dx) <= lSquare) {
                lTry += (uint32_t)(dy * dy);
                if (lTry <= lSquare && (lSquare != lTry || lSquare == 0)) {
                    lSquare = lTry;
                    scan.pt = *ppt;
                    scan.idpl = i;
                    scan.grobjFull = grobjPlanet;
                    scan.grobj = grobjPlanet;
                }
            }
            i++;
            ppt++;
        }
    }
    if (grobj & grobjFleet) {
        for (i = 0; i < cFleet; i++) {
            lpfl = rglpfl[i];
            if (!rglpfl[i])
                break;
            dx = pt.x - lpfl->pt.x;
            dy = pt.y - lpfl->pt.y;
            lTry = (uint32_t)(dx * dx);
            if ((int32_t)(uint32_t)(dx * dx) <= lSquare) {
                lTry += (uint32_t)(dy * dy);
                if (lTry <= lSquare) {
                    if ((lTry == lSquare && !(scan.grobj & grobjPlanet)) || (lpfl->pt.x == scan.pt.x && lpfl->pt.y == scan.pt.y)) {
                        if ((scan.grobjFull & grobjFleet) && (rglpfl[scan.ifl]->iPlayer == idPlayer || lpfl->iPlayer != idPlayer))
                            continue;
                        scan.ifl = i;
                        scan.grobjFull |= grobjFleet;
                        if (scan.grobj == grobjNone)
                            goto SelectShip;
                        continue;
                    } else {
                        if (lTry >= lSquare)
                            continue;
                        lSquare = lTry;
                    }
                SelectShip:
                    scan.pt = lpfl->pt;
                    scan.ifl = i;
                    scan.idpl = idplNone;
                    scan.grobjFull = grobjFleet;
                    scan.grobj = grobjFleet;
                }
            }
        }
    }
    if (grobj & grobjThing) {
        lpth = lpThings;
        lpthMac = lpThings + cThing;
        for (; lpth < lpthMac; lpth++) {
            dx = pt.x - lpth->pt.x;
            dy = pt.y - lpth->pt.y;
            lTry = (uint32_t)(dx * dx);
            if ((int32_t)(uint32_t)(dx * dx) <= lSquare) {
                lTry += (uint32_t)(dy * dy);
                if (lTry <= lSquare) {
                    if ((lTry == lSquare && !(scan.grobj & (grobjPlanet | grobjFleet))) || (lpth->pt.x == scan.pt.x && lpth->pt.y == scan.pt.y)) {
                        if (scan.grobjFull & grobjThing)
                            continue;
                        scan.ith = (int16_t)((uint8_t *)lpth - (uint8_t *)lpThings) / 18;
                        scan.grobjFull |= grobjThing;
                        if (scan.grobj == grobjNone)
                            goto SelectThing;
                        continue;
                    } else {
                        if (lTry >= lSquare)
                            continue;
                        lSquare = lTry;
                    }
                SelectThing:
                    scan.pt = lpth->pt;
                    scan.ith = (int16_t)((uint8_t *)lpth - (uint8_t *)lpThings) / 18;
                    scan.idpl = idplNone;
                    scan.ifl = iflNone;
                    scan.grobjFull = grobjThing;
                    scan.grobj = grobjThing;
                }
            }
        }
    }
    if ((grobj & grobjOther) && sel.grobj == grobjFleet) {
        for (i = sel.fl.cord - 1; i >= 0; i--) {
            ptWp = sel.fl.lpplord->rgord[i].pt;
            dx = pt.x - ptWp.x;
            dy = pt.y - ptWp.y;
            lTry = (uint32_t)(dx * dx);
            if ((int32_t)(uint32_t)(dx * dx) <= lSquare) {
                lTry += (uint32_t)(dy * dy);
                if (lTry <= lSquare) {
                    if ((lTry == lSquare && !(scan.grobj & (grobjPlanet | grobjFleet | grobjThing))) || (ptWp.x == scan.pt.x && ptWp.y == scan.pt.y)) {
                        if ((scan.grobjFull & grobjOther) && i != sel.scan.iwp)
                            continue;
                        scan.iwp = i;
                        scan.grobjFull |= grobjOther;
                        if (scan.grobj == grobjNone)
                            goto SelectSpace;
                        continue;
                    } else {
                        if (lTry >= lSquare)
                            continue;
                        lSquare = lTry;
                    }
                SelectSpace:
                    scan.pt = ptWp;
                    scan.iwp = i;
                    scan.ith = ithNone;
                    scan.idpl = idplNone;
                    scan.ifl = iflNone;
                    scan.grobjFull = grobjOther;
                    scan.grobj = grobjOther;
                }
            }
        }
    }
    if (!(grobj & mdNoRecurse) && scan.grobj != grobjNone &&
        FFindNearestObject(scan.pt, ((grobj & (grobjPlanet | grobjFleet | grobjOther | grobjThing)) ^ 0xf) | 0xa0, &scanT)) {
        if (scanT.idpl != idplNone) {
            scan.idpl = scanT.idpl;
        }
        if (scanT.ifl != iflNone) {
            scan.ifl = scanT.ifl;
        }
        if (scanT.iwp != iwpNone) {
            scan.iwp = scanT.iwp;
        }
        if (scanT.ith != ithNone) {
            scan.ith = scanT.ith;
        }
        scan.grobjFull |= scanT.grobjFull;
    }
    if (pscan) {
        *pscan = scan;
    }
    if (scan.grobj != grobjNone) {
        return TRUE;
    }
    return FALSE;
}

int16_t PtToScan(int16_t d) {
    if (iScanZoom == zoom100) {
        return d;
    }
    switch (iScanZoom) {
    case zoom400:
        d *= 4;
        break;
    case zoom200:
        d *= 2;
        break;
    case zoom50:
        d >>= 1;
        break;
    case zoom25:
        d >>= 2;
        break;
    case zoom38:
        d = ((d << 1) + d) >> 3;
        break;
    case zoom75:
        d = ((d << 1) + d) >> 2;
        break;
    case zoom125:
        d = ((d << 2) + d) >> 2;
        break;
    case zoom150:
        d = ((d << 1) + d) >> 1;
    }
    return d;
}

int16_t ScanToPt(int16_t d) {
    if (iScanZoom == zoom100) {
        return d;
    }
    switch (iScanZoom) {
    case zoom400:
        d >>= 2;
        break;
    case zoom200:
        d >>= 1;
        break;
    case zoom50:
        d *= 2;
        break;
    case zoom25:
        d *= 4;
        break;
    case zoom38:
        d = (int16_t)(d * 8) / 3;
        break;
    case zoom75:
        d = (int16_t)(d * 4) / 3;
        break;
    case zoom125:
        d = (int16_t)(d * 4) / 5;
        break;
    case zoom150:
        d = (int16_t)(d * 2) / 3;
    }
    return d;
}

void ChangeScanSel(SCAN *pscan, int16_t fValidScan) {
    int16_t fChgWp;
    SCAN    scanOld;

    if (!fValidScan) {
        FFindNearestObject(pscan->pt, pscan->grobj, pscan);
    }
    if (memcmp(pscan, &sel.scan, sizeof(SCAN)) != 0) {
        fChgWp = pscan->iwp != iwpNone && pscan->iwp != sel.iwpAct;
        scanOld = sel.scan;
        ShowScanSel(-1);
        sel.scan = *pscan;
        if ((sel.scan.grobjFull & grobjPlanet) && fValidScan != 2) {
            sel.scan.grobj = grobjPlanet;
        }
        if (fChgWp) {
            sel.iwpAct = pscan->iwp;
        }
        ShowScanSelChange(&scanOld, pscan, fChgWp);
    }
    return;
}

void UpdateShdefCost(SHDEF *lpshdef) {
    int32_t  dpT;
    uint32_t wt;
    int16_t  k;
    int16_t  c;
    uint16_t rgCosts[4];
    int16_t  fWeakArmor;
    HUL     *lphul;
    uint32_t resCost;
    uint32_t rgMin[3];
    PART     part;

    if (lpshdef->det == detAll) {
        fWeakArmor = GetRaceGrbit(&rgplr[idPlayer], ibitRaceRegeneratingShields) != 0;
    } else {
        fWeakArmor = FALSE;
    }
    lphul = &LphuldefFromId(lpshdef->hul.ihuldef)->hul;
    part.hs.grhst = hstNone;
    part.phul = lphul;
    GetTruePartCost(idPlayer, &part, rgCosts);
    for (c = 0; c < 3; c++) {
        rgMin[c] = (uint32_t)rgCosts[c];
    }
    resCost = (uint32_t)rgCosts[3];
    wt = (uint32_t)lphul->wtEmpty;
    lpshdef->hul.dp = lphul->dp;
    lphul = &lpshdef->hul;
    for (c = 0; c < lphul->chs; c++) {
        if (lphul->rghs[c].cItem > 0) {
            part.hs = lphul->rghs[c];
            FLookupPart(&part);
            GetTruePartCost(idPlayer, &part, rgCosts);
            for (k = 0; k < 3; k++) {
                rgMin[k] += (uint32_t)(lphul->rghs[c].cItem * rgCosts[k]);
            }
            resCost += (uint32_t)(lphul->rghs[c].cItem * rgCosts[3]);
            wt += (uint32_t)(lphul->rghs[c].cItem * part.pcom->cMass);
            switch (lphul->rghs[c].grhst) {
            default:
                break;
            case hstArmor:
                /* 32-bit: the original's int16_t overflowed for large slots
                   (24 Superlatanium), and RS then halved the negative value. */
                dpT = (int32_t)lphul->rghs[c].cItem * part.parmor->dp;
                if (fWeakArmor) {
                    dpT >>= 1;
                }
                lphul->dp += dpT;
                break;
            case hstShield:
                if (lphul->rghs[c].iItem != ishieldCrobySharmor && lphul->rghs[c].iItem != ishieldLangstonShell)
                    break;
                lphul->dp += lphul->rghs[c].cItem * 65;
                break;
            case hstSpecialM:
                if (lphul->rghs[c].iItem == ispecialMMultiCargoPod) {
                    lphul->dp += lphul->rghs[c].cItem * 50;
                }
            }
        }
    }
    for (c = 0; c < 3; c++) {
        lphul->rgwtOreCost[c] = LOWORD(rgMin[c]);
    }
    lphul->resCost = LOWORD(resCost);
    lphul->wtEmpty = LOWORD(wt);
    lpshdef->lPower = -1;
    return;
}

uint16_t WPackLong(int32_t l) {
    uint16_t exp;

    exp = 0;
    while (l & 0xffffe000) {
        l = (uint32_t)((uint32_t)l >> 2);
        exp++;
    }
    return exp << 0xd | LOWORD(l);
}

int16_t GetPlanetScannerRange(PLANET *lppl, int16_t *pDeep) {
    int16_t iPlrSav;
    int16_t dRange;
    PART    part;

    iPlrSav = idPlayer;
    idPlayer = lppl->iPlayer;
    if (pDeep) {
        *pDeep = 0;
    }
    if (GetRaceStat(&rgplr[lppl->iPlayer], rsMajorAdv) == raMacintosh) {
        dRange = LOWORD(Sf64ToI32(Sf64Sqrt(Sf64FromU32((uint32_t)(lppl->rgwtMin[3] * 10)))));
        if (GetRaceGrbit(&rgplr[idPlayer], ibitRaceNoAdvScanner) != 0) {
            dRange = LOWORD((int32_t)(dRange * 1412) / 1000);
            if (pDeep) {
                *pDeep = 0;
            }
        } else if (pDeep && lppl->fStarbase && (int16_t)rglpshdefSB[lppl->iPlayer][lppl->isb].hul.ihuldef >= ihuldefUltraStation) {
            *pDeep = dRange / 2;
        }
        goto LFinishUp;
    } else {
        if (lppl->iScanner == 31) {
            idPlayer = iPlrSav;
            return 0;
        }
        LookupBestPlanetaryScanner(&part);
        if (pDeep && part.pplanetary->grAbility < 0) {
            *pDeep = -part.pplanetary->grAbility >> 1;
        }
        dRange = abs(part.pplanetary->grAbility);
        if (GetRaceGrbit(&rgplr[idPlayer], ibitRaceNoAdvScanner) != 0) {
            dRange *= 2;
        }
    }
LFinishUp:
    idPlayer = iPlrSav;
    return dRange;
}

int16_t GetCachedFleetScannerRange(FLEET *lpfl, int16_t *pdPlanRange, int16_t *ppctDetect, int16_t *piSteal) {
    int16_t dT;
    int16_t dPlanRange;
    int16_t i;
    int16_t iPlr;
    int16_t dRange;
    int16_t iSteal;
    int16_t pctDetect;

    iPlr = lpfl->iPlayer;
    dRange = -1;
    dPlanRange = 0;
    iSteal = 0;
    pctDetect = 100;
    if (!gd.fGeneratingTurn) {
        return GetFleetScannerRange(lpfl, pdPlanRange, ppctDetect, piSteal);
    }
    for (i = 0; i < 16; i++) {
        if (lpfl->rgcsh[i] > 0) {
            dT = rglpshdef[iPlr][i].dScanRange;
            if (dT != 2047 && dT > dRange) {
                dRange = dT;
            }
            if ((int16_t)rglpshdef[iPlr][i].dScanRange2 > dPlanRange) {
                dPlanRange = rglpshdef[iPlr][i].dScanRange2;
            }
            if (rglpshdef[iPlr][i].pctDetect < pctDetect) {
                pctDetect = rglpshdef[iPlr][i].pctDetect;
            }
            iSteal |= rglpshdef[iPlr][i].iSteal;
        }
    }
    if (pdPlanRange) {
        *pdPlanRange = dPlanRange;
    }
    if (ppctDetect) {
        *ppctDetect = pctDetect;
    }
    if (piSteal) {
        *piSteal = iSteal;
    }
    return dRange;
}

int16_t GetFleetScannerRange(FLEET *lpfl, int16_t *pdPlanRange, int16_t *ppctDetect, int16_t *piSteal) {
    int16_t iplr;
    int16_t dPlanRange;
    int16_t i;
    int16_t dRange;
    int16_t iSteal;
    int16_t dPlanRangeBest;
    int16_t dRangeBest;
    int16_t pctDetect;

    iplr = lpfl->iPlayer;
    dRange = -1;
    dPlanRange = 0;
    iSteal = 0;
    if (ppctDetect) {
        *ppctDetect = 100;
    }
    for (i = 0; i < 16; i++) {
        if (lpfl->rgcsh[i] > 0) {
            dRangeBest = GetShdefScannerRange(rglpshdef[iplr] + i, iplr, &dPlanRangeBest, &pctDetect, &iSteal);
            if (ppctDetect && pctDetect < *ppctDetect) {
                *ppctDetect = pctDetect;
            }
            if (piSteal) {
                *piSteal |= iSteal;
            }
            if (dRangeBest > dRange) {
                dRange = dRangeBest;
            }
            if (dPlanRange < dPlanRangeBest) {
                dPlanRange = dPlanRangeBest;
            }
        }
    }
    if (pdPlanRange) {
        *pdPlanRange = dPlanRange;
    }
    return dRange;
}

int16_t GetShdefScannerRange(SHDEF *lpshdef, int16_t iplr, int16_t *pdPlanRange, int16_t *ppctDetect, int16_t *piSteal) {
    int16_t  chs;
    HS      *lphs;
    int16_t  dRangeT2;
    double   lBIR4;
    int16_t  dRangeT;
    int16_t  fHasScanner;
    iscanner iScanner;
    int16_t  fBuiltIn;
    int16_t  cDetectors;
    double   lPlanRange4;
    int16_t  dRange;
    double   lT;
    int16_t  iSteal;
    int16_t  j;
    double   lBIPR4;
    double   lRange4;

    lRange4 = Sf64FromI32(0);
    lPlanRange4 = Sf64FromI32(0);
    fHasScanner = FALSE;
    iSteal = 0;
    cDetectors = 0;
    fBuiltIn = iplr != iplrNone && GetRaceStat(&rgplr[iplr], rsMajorAdv) == raNone;
    lBIR4 = Sf64Neg(1.0);
    lBIPR4 = Sf64Neg(1.0);
    if (ppctDetect) {
        *ppctDetect = 100;
    }
    if (fBuiltIn) {
        switch (lpshdef->hul.ihuldef) {
        case ihuldefScout:
        case ihuldefDestroyer:
        case ihuldefFrigate:
            if (Sf80Lt(Sf80From64(lBIR4), Sf80FromI32(0))) {
                if (game.fTutorial) {
                    lBIR4 = 2.56e+06;
                    lBIPR4 = 160000.0;
                } else {
                    lBIPR4 = Sf64FromI32((int16_t)(rgplr[iplr].rgTech[4] * 10));
                    lBIR4 = Sf64From80((Sf80Mul(Sf80From64(lBIPR4), Sf80FromI32(2))));
                    lBIPR4 = Sf64From80((Sf80Mul(Sf80From64(lBIPR4), Sf80From64(lBIPR4))));
                    lBIPR4 = Sf64From80((Sf80Mul(Sf80From64(lBIPR4), Sf80From64(lBIPR4))));
                    lBIR4 = Sf64From80((Sf80Mul(Sf80From64(lBIR4), Sf80From64(lBIR4))));
                    lBIR4 = Sf64From80((Sf80Mul(Sf80From64(lBIR4), Sf80From64(lBIR4))));
                }
            }
            lRange4 = lBIR4;
            lPlanRange4 = lBIPR4;
        }
    }
    lphs = lpshdef->hul.rghs;
    chs = lpshdef->hul.chs;
    for (j = 0; j < chs; j++, lphs++) {
        if (lphs->cItem == 0)
            continue;
        if (lphs->grhst == hstScanner) {
            fHasScanner = TRUE;
            iScanner = lphs->iItem;
            lT = Sf64FromI32(dRangeT = LpscannerFromId(lphs->iItem)->dRange);
            lT = Sf64From80((Sf80Mul(Sf80From64(lT), Sf80From64(lT))));
            lT = Sf64From80((Sf80Mul(Sf80From64(lT), Sf80From64(lT))));
            lT = Sf64From80((Sf80Mul(Sf80From64(lT), Sf80FromU32((uint32_t)lphs->cItem))));
            lRange4 = Sf64From80((Sf80Add(Sf80From64(lRange4), Sf80From64(lT))));
            dRangeT = LpscannerFromId(iScanner)->grfAbilities;
            if (iScanner == iscannerChameleonScanner) {
                dRangeT = 45;
                goto LPlanScan;
            } else if (iScanner == iscannerPickPocketScanner) {
                dRangeT = 0;
                iSteal |= 1;
                goto LPlanScan;
            } else if (iScanner == iscannerRobberBaronScanner) {
                dRangeT = 120;
                iSteal |= 3;
                goto LPlanScan;
            }
            if (dRangeT <= 0)
                continue;
            dRangeT = dRangeT == 1 ? 50 : dRangeT == 2 ? 100 : 200;
        LPlanScan:
            lT = Sf64FromI32(dRangeT);
            lT = Sf64From80((Sf80Mul(Sf80From64(lT), Sf80From64(lT))));
            lT = Sf64From80((Sf80Mul(Sf80From64(lT), Sf80From64(lT))));
            lT = Sf64From80((Sf80Mul(Sf80From64(lT), Sf80FromU32((uint32_t)lphs->cItem))));
            lPlanRange4 = Sf64From80((Sf80Add(Sf80From64(lPlanRange4), Sf80From64(lT))));
        } else if (lphs->grhst == hstArmor && lphs->iItem == iarmorMegaPolyShell) {
            dRangeT = 80;
            dRangeT2 = 40;
        LOddBallScanners:
            lT = Sf64FromI32(dRangeT);
            lT = Sf64From80((Sf80Mul(Sf80From64(lT), Sf80From64(lT))));
            lT = Sf64From80((Sf80Mul(Sf80From64(lT), Sf80From64(lT))));
            lT = Sf64From80((Sf80Mul(Sf80From64(lT), Sf80FromU32((uint32_t)lphs->cItem))));
            lRange4 = Sf64From80((Sf80Add(Sf80From64(lRange4), Sf80From64(lT))));
            dRangeT = dRangeT2;
            goto LPlanScan;
        } else if (lphs->grhst == hstBeam && lphs->iItem == ibeamMultiContainedMunition) {
            dRangeT = 150;
            dRangeT2 = 75;
            goto LOddBallScanners;
        } else if (lphs->grhst == hstShield && lphs->iItem == ishieldLangstonShell) {
            dRangeT = 50;
            dRangeT2 = 25;
            goto LOddBallScanners;
        } else if (ppctDetect && lphs->grhst == hstSpecialE && lphs->iItem == ispecialETachyonDetector) {
            cDetectors += lphs->cItem;
        }
    }
    if (Sf80Lt(Sf80FromI32(0), Sf80From64(lRange4)) || fHasScanner) {
        dRange = LOWORD(Sf64ToI32(Sf64Sqrt(Sf64Sqrt(lRange4))));
        if (iplr != iplrNone && GetRaceGrbit(&rgplr[iplr], ibitRaceNoAdvScanner) != 0) {
            dRange *= 2;
        }
    } else {
        dRange = -1;
    }
    if (pdPlanRange) {
        *pdPlanRange = LOWORD(Sf64ToI32(Sf64Sqrt(Sf64Sqrt(lPlanRange4))));
    }
    if (piSteal) {
        *piSteal = iSteal;
    }
    if (ppctDetect) {
        if (cDetectors >= 18) {
            cDetectors = 17;
        }
        *ppctDetect = vrgbTachyon[cDetectors];
    }
    return dRange;
}

int32_t LCalcFuelGainFromRamScoops(FLEET *lpfl, int16_t iWarp, int32_t dTravel) {
    int16_t  i;
    int16_t *rgiFuel;
    SHDEF   *lpshdef;
    int32_t  pct10;
    int32_t  pctShip10;

    pct10 = 0;
    if (iWarp > 10) {
        return 0;
    }
    i = 0;
    lpshdef = rglpshdef[lpfl->iPlayer];
    while (i < 16) {
        if (lpfl->rgcsh[i] != 0) {
            rgiFuel = LpengineFromId(lpshdef->hul.rghs[0].iItem)->rgcFuelUsed;
            pctShip10 = 0;
            if (iWarp <= 9) {
                if (rgiFuel[iWarp] == 0) {
                    pctShip10 += lpshdef->hul.rghs[0].cItem;
                    if (rgiFuel[iWarp + 1] == 0) {
                        pctShip10 += (uint32_t)(lpshdef->hul.rghs[0].cItem * 2);
                        if (iWarp < 9 && rgiFuel[iWarp + 2] == 0) {
                            pctShip10 += (uint32_t)(lpshdef->hul.rghs[0].cItem * 3);
                            if (iWarp < 8 && rgiFuel[iWarp + 3] == 0) {
                                pctShip10 += (uint32_t)(lpshdef->hul.rghs[0].cItem * 4);
                            }
                        }
                    }
                }
                pct10 += (uint32_t)(pctShip10 * lpfl->rgcsh[i]);
            }
        }
        i++;
        lpshdef++;
    }
    pct10 = (uint32_t)(pct10 * dTravel);
    return pct10;
}

int32_t CalcPlayerScore(int16_t iPlr, SCORE *pscore) {
    int32_t rgcsh[3];
    int32_t lTemp;
    SCORE   score;
    PLANET *lpplMac;
    PLANET *lppl;
    int16_t i;
    int16_t ifl;
    FLEET  *lpfl;
    int16_t iTech;
    int32_t lPower;
    int16_t rgType[16];
    PLANET  pl;

    memset(&score, 0, sizeof(SCORE));
    lppl = lpPlanets;
    lpplMac = lpPlanets + cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->iPlayer == iPlr) {
            score.cPlanet++;
            /* Score a planet the player owned through production at its
               population then, not after waypoint 1 cargo moves. */
            pl = *lppl;
            if (vrgiplrPopScore && vrgiplrPopScore[lppl->id] == iPlr) {
                pl.rgwtMin[3] = vrgwtPopScore[lppl->id];
            }
            lTemp = (int32_t)((pl.rgwtMin[3] + 999) / 1000);
            if (lTemp > 6) {
                lTemp = 6;
            }
            score.lScore += lTemp;
            if (lppl->fStarbase && LphuldefFromId(rglpshdefSB[iPlr][lppl->isb].hul.ihuldef)->hul.wtCargoMax != 0) {
                score.cStarbase++;
            }
            score.cResources += CResourcesAtPlanet(&pl, iPlr);
        }
    }
    score.lScore += (int32_t)(score.cResources / 30);
    score.lScore += (int16_t)(3 * score.cStarbase);
    if (!rgplr[iPlr].fDead) {
        for (i = 0; i < 6; i++) {
            iTech = rgplr[iPlr].rgTech[i];
            score.cTechLevels += rgplr[iPlr].rgTech[i];
            if (iTech < 4) {
                score.lScore += iTech;
            } else if (iTech < 7) {
                score.lScore += (int16_t)(iTech * 2 - 3);
            } else if (iTech < 10) {
                score.lScore += (int16_t)(3 * iTech - 9);
            } else {
                score.lScore += (int16_t)(iTech * 4 - 18);
            }
        }
    }
    for (i = 0; i < 16; i++) {
        if (rglpshdef[iPlr][i].fFree) {
            rgType[i] = -1;
        } else {
            lPower = LComputePower(rglpshdef[iPlr] + i);
            if (lPower <= 0) {
                rgType[i] = 0;
            } else if (lPower < 2000) {
                rgType[i] = 1;
            } else {
                rgType[i] = 2;
            }
        }
    }
    for (i = 0; i < 3; i++) {
        rgcsh[i] = 0;
    }
    for (ifl = 0; ifl < cFleet; ifl++) {
        lpfl = rglpfl[ifl];
        if (!rglpfl[ifl])
            break;
        if (lpfl->iPlayer == iPlr && !lpfl->fDead) {
            for (i = 0; i < 16; i++) {
                if (lpfl->rgcsh[i] > 0 && rgType[i] != -1) {
                    rgcsh[rgType[i]] = rgcsh[rgType[i]] + lpfl->rgcsh[i];
                }
            }
        }
    }
    score.lScore += (int32_t)((rgcsh[0] < score.cPlanet ? rgcsh[0] : score.cPlanet) / 2) + (int32_t)((rgcsh[1] < score.cPlanet ? rgcsh[1] : score.cPlanet) * 2);
    if (rgcsh[2] > 0) {
        score.lScore += (int32_t)((int32_t)((int32_t)(rgcsh[2] * 8) * score.cPlanet) / (score.cPlanet + rgcsh[2]));
    }
    for (i = 0; i < 3; i++) {
        score.rgcsh[i] = WPackLong(rgcsh[i]);
    }
    if (pscore) {
        *pscore = score;
    }
    return score.lScore;
}

void GetTrueHullCost(int16_t iPlayer, HUL *lphul, uint16_t *rgCost) {
    int16_t i;

    for (i = 0; i < 3; i++) {
        rgCost[i] = lphul->rgwtOreCost[i];
    }
    rgCost[3] = lphul->resCost;
    return;
}

void DecorateHullName(int16_t iplr, int16_t ish, char *psz) {
    int16_t i;
    int16_t c;
    SHDEF  *lpshdef;
    int16_t iVal;

    lpshdef = rglpshdef[iplr] + ish;
    if (lpshdef->fFree) {
        *psz = 0;
    } else {
        strcpy(psz, lpshdef->hul.szClass);
        if (iplr != idPlayer) {
            c = 0;
            iVal = 1;
            for (i = 0; i < 16; i++) {
                if (i != ish && !rglpshdef[iplr][i].fFree && strcmp(psz, rglpshdef[iplr][i].hul.szClass) == 0) {
                    c++;
                    if (i < ish) {
                        iVal++;
                    }
                }
            }
            if (c != 0) {
                c = strlen(psz);
                psz[c] = ' ';
                psz[c + 1] = '(';
                IntToRoman(iVal, psz + (c + 2));
                strcat(psz, ")");
            }
        }
    }
    return;
}

int16_t FMatchTarget(FLEET *lpflTarget, MdTarget mdTarget, int16_t fExact) {
    int16_t imd;
    int16_t ish;

    switch (mdTarget) {
    default:
        if (!fExact)
            break;
        return FALSE;
    case mdTargetArmedShips:
    case mdTargetUnarmedShips:
        for (ish = 0; ish < 16; ish++) {
            if (lpflTarget->rgcsh[ish] != 0) {
                imd = LphuldefFromId(rglpshdef[lpflTarget->iPlayer][ish].hul.ihuldef)->imdCategory;
                if (imd >= 2 && imd <= 4)
                    break;
            }
        }
        if (mdTarget == mdTargetArmedShips) {
            if (ish != 16)
                break;
            return FALSE;
        }
        if (ish == 16)
            break;
        return FALSE;
    case mdTargetBombersFreighters:
        for (ish = 0; ish < 16; ish++) {
            if (lpflTarget->rgcsh[ish] != 0) {
                imd = LphuldefFromId(rglpshdef[lpflTarget->iPlayer][ish].hul.ihuldef)->imdCategory;
                if (imd == 1 || imd == 5)
                    break;
            }
        }
        if (ish != 16)
            break;
        return FALSE;
    case mdTargetFuelTransports:
        for (ish = 0; ish < 16; ish++) {
            if (lpflTarget->rgcsh[ish] != 0) {
                imd = LphuldefFromId(rglpshdef[lpflTarget->iPlayer][ish].hul.ihuldef)->imdCategory;
                if (imd == 7)
                    break;
            }
        }
        if (ish != 16)
            break;
        return FALSE;
    case mdTargetFreighters:
        for (ish = 0; ish < 16; ish++) {
            if (lpflTarget->rgcsh[ish] != 0) {
                imd = LphuldefFromId(rglpshdef[lpflTarget->iPlayer][ish].hul.ihuldef)->imdCategory;
                if (imd == 1)
                    break;
            }
        }
        if (ish == 16) {
            return FALSE;
        }
    }
    return TRUE;
}

void ValidateWaypoints() {
    MdTarget mdTarget;
    FLEET   *lpflTarget;
    int16_t  ifl2;
    int32_t  wt;
    FLEET   *lpflMatch;
    int32_t  wtMatch;
    ORDER   *lpord;
    int16_t  ifl;
    THING   *lpth;
    FLEET   *lpfl;
    int16_t  cFound;
    int16_t  iord;
    FLEET   *lpfl2;
    int16_t  iplrHi;

    for (ifl = 0; ifl < cFleet; ifl++) {
        lpfl = rglpfl[ifl];
        if (!rglpfl[ifl])
            break;
        if (lpfl->lpplord->rgord[0].grobj != grobjPlanet && lpfl->lpplord->rgord[0].grTask != grTaskXfer && lpfl->lpplord->rgord[0].grTask != grTaskMerge &&
            !lpfl->fMark) {
            if (lpfl->idPlanet == idPlanetDeepSpace) {
                lpfl->lpplord->rgord[0].grobj = grobjOther;
                lpfl->lpplord->rgord[0].id = 0;
            } else {
                lpfl->lpplord->rgord[0].grobj = grobjPlanet;
                lpfl->lpplord->rgord[0].id = lpfl->idPlanet;
            }
        }
        if (!lpfl->fDead) {
            if (lpfl->pt.x < 1000) {
                lpfl->pt.x = 1000;
                lpfl->lpplord->rgord[0].pt.x = 1000;
            } else if (lpfl->pt.x > dGal + 1000) {
                lpfl->pt.x = dGal + 1000;
                lpfl->lpplord->rgord[0].pt.x = dGal + 1000;
            }
            if (lpfl->pt.y < 1000) {
                lpfl->pt.y = 1000;
                lpfl->lpplord->rgord[0].pt.y = 1000;
            } else if (lpfl->pt.y > dGal + 1000) {
                lpfl->pt.y = dGal + 1000;
                lpfl->lpplord->rgord[0].pt.y = dGal + 1000;
            }
            if (lpfl->cord > 1 || lpfl->fMark) {
                iord = lpfl->fMark == 0;
                lpord = &lpfl->lpplord->rgord[iord];
                while (iord < lpfl->cord) {
                    if (lpord->grobj == grobjThing) {
                        lpth = LpthFromId(lpord->id);
                        if (!lpth || (lpth->ith == ithWormhole && !(1 << lpfl->iPlayer & lpth->thw.grbitPlr) &&
                                      (lpth->pt.x != lpord->pt.x || lpth->pt.y != lpord->pt.y))) {
                            if (lpth) {
                                FSendPlrMsg(lpfl->iPlayer, idmWormholeHeadingHasVanishedOrdersHaveChanged, 0x8000 | lpfl->id, lpfl->id, 0, 0, 0, 0, 0, 0);
                            }
                            lpord->grobj = grobjOther;
                            lpord->id = iord;
                        } else {
                            lpord->pt = lpth->pt;
                        }
                    } else if (lpord->grobj == grobjFleet && !lpord->fNoAutoTrack) {
                        lpflTarget = LpflFromId(lpord->id);
                        if (!lpflTarget || lpflTarget->pt.x != lpord->pt.x || lpflTarget->pt.y != lpord->pt.y ||
                            (lpflTarget->fCompChg && lpflTarget->iPlayer != lpfl->iPlayer)) {
                            lpflTarget = NULL;
                            iplrHi = lpord->id & 0xfe00;
                            cFound = 0;
                            mdTarget = rglpbtlplan[lpfl->iPlayer][lpfl->iplan].mdTarget1;
                            wtMatch = 0;
                            lpflMatch = NULL;
                            /* As in the jrc4 binary (0008:497a): one pass, taking the
                               heaviest matching fleet even if another pursuer already
                               has it. jrc3 first looked only at untargeted fleets. */
                            for (ifl2 = 0; ifl2 < cFleet; ifl2++) {
                                lpfl2 = rglpfl[ifl2];
                                if (!rglpfl[ifl2])
                                    break;
                                if (lpfl2->pt.x == lpord->pt.x && lpfl2->pt.y == lpord->pt.y && iplrHi == (lpfl2->id & 0xfe00)) {
                                    if (FMatchTarget(lpfl2, mdTarget, TRUE)) {
                                        wt = WtFromLpfl(lpfl2);
                                        if (wt > wtMatch || (wt == wtMatch && Random(2) == 0)) {
                                            wtMatch = wt;
                                            lpflMatch = lpfl2;
                                        }
                                    }
                                    cFound++;
                                    if (Random(cFound) == 0 && (cFound == 1 || !lpfl2->fTargeted || Random(2) != 0)) {
                                        lpflTarget = lpfl2;
                                    }
                                }
                            }
                            if (lpflMatch) {
                                lpflTarget = lpflMatch;
                            }
                            if (lpflTarget) {
                                lpord->id = lpflTarget->id;
                                lpord->pt = lpflTarget->pt;
                                lpflTarget->fTargeted = TRUE;
                            }
                        } else if (lpflTarget) {
                            lpflTarget->fTargeted = TRUE;
                        }
                    }
                    iord++;
                    lpord++;
                }
            }
        }
    }
    return;
}

int32_t ChgPopFromPlanet(PLANET *lppl, int16_t fUpdate) {
    int32_t lMaxPop;
    int16_t fPopDied;
    int32_t lPopIncDelta;
    int16_t DeltaCur;
    int32_t pctGrow100;
    int16_t pctDesire;
    int32_t lPopInc100;
    int32_t lPopInc;
    int32_t lPopOld;
    int32_t pctRetard;
    int32_t pctFull;

    fPopDied = FALSE;
    if (lppl->iPlayer == iplrNone || lppl->rgwtMin[3] == 0) {
        return 0;
    }
    pctDesire = PctPlanetDesirability(lppl, lppl->iPlayer);
    lPopOld = lppl->rgwtMin[3];
    fPopDied = TRUE;
    if (pctDesire < 0) {
        lPopInc100 = 1 <= (int32_t)(lPopOld * (int16_t)-pctDesire) / 10 ? (int32_t)(lPopOld * (int16_t)-pctDesire) / 10 : 1;
        lPopInc = (int32_t)(lPopInc100 / 100);
        lPopIncDelta = (int32_t)(lPopInc100 % 100);
        if (lPopInc == 0 && lPopIncDelta == 0) {
            lPopIncDelta = 1;
        }
        DeltaCur = lppl->iDeltaPop - LOWORD(lPopIncDelta);
        if (DeltaCur < 0) {
            lPopInc++;
            DeltaCur += 100;
        }
        lPopInc = -lPopInc;
        goto LUpdateAndExit;
    }
    lMaxPop = CalcPlanetMaxPop(lppl->id, lppl->iPlayer);
    pctGrow100 = (int16_t)(PctTrueMaxGrowth(lppl->iPlayer) * pctDesire);
    if (lPopOld > (int32_t)(lMaxPop / 4)) {
        pctFull = (int32_t)((int32_t)(lPopOld * 1000) / lMaxPop);
        if (lPopOld >= lMaxPop) {
            if (lPopOld < lMaxPop + 10) {
                return 0;
            }
            pctRetard = -300 <= 99 - (int32_t)(pctFull / 10) ? 99 - (int32_t)(pctFull / 10) : -300;
            pctGrow100 = (int32_t)(pctRetard * 4);
        } else {
            pctRetard = 1000 - pctFull;
            pctRetard = (uint32_t)(pctRetard * pctRetard);
            if (pctGrow100 < 1000) {
                pctGrow100 = (int32_t)(pctGrow100 * pctRetard) / 562500;
            } else {
                pctGrow100 = (uint32_t)((int32_t)((int32_t)(pctGrow100 / 10) * pctRetard) / 562500 * 10);
            }
        }
    }
    lPopInc100 = (uint32_t)(lPopOld * (int32_t)(pctGrow100 / 100));
    if (lPopInc100 < 10000000) {
        lPopInc100 = (int32_t)(lPopOld * pctGrow100) / 100;
    }
    lPopInc = (int32_t)(lPopInc100 / 100);
    lPopIncDelta = (int32_t)(lPopInc100 % 100);
    if (lPopInc == 0 && lPopIncDelta == 0) {
        lPopIncDelta = 1;
    }
    DeltaCur = lppl->iDeltaPop + LOWORD(lPopIncDelta);
    if (DeltaCur >= 100) {
        lPopInc++;
        DeltaCur -= 100;
    } else if (DeltaCur < 0) {
        lPopInc--;
        DeltaCur += 100;
    }
LUpdateAndExit:
    if (fUpdate) {
        lppl->iDeltaPop = (uint16_t)DeltaCur;
        lppl->rgwtMin[3] += lPopInc;
    }
    return lPopInc;
}

int16_t FCanFleetUseStargates(FLEET *lpfl, POINT16 ptSrc, POINT16 ptDst) {
    int16_t dTravel;
    PLANET *lpplDst;
    int16_t pctDmg;
    int16_t fSrcPlanet;
    int16_t fUncertain;
    int16_t i;
    int16_t fDanger;
    PLANET *lpplSrc;
    int16_t isbsDst;
    int16_t fCargo;
    int16_t ishdef;
    int16_t isbsSrc;
    SCAN    scan;

    fUncertain = FALSE;
    if (!FFindNearestObject(ptDst, grobjPlanet | mdExact, &scan)) {
        return 0;
    }
    lpplDst = LpplFromId(scan.idpl);
    if (!lpplDst || lpfl->iPlayer != lpplDst->iPlayer) {
        if (lpplDst && lpplDst->iPlayer == iplrNone && lpplDst->turn == game.turn) {
            return 0;
        }
        fUncertain = TRUE;
        goto LSrcChk;
    }
    isbsDst = IStargateFromLppl(lpplDst);
    if (isbsDst < 0) {
        return 0;
    }
LSrcChk:
    lpplSrc = NULL;
    fSrcPlanet = FALSE;
    if (FFindNearestObject(ptSrc, grobjPlanet | mdExact, &scan)) {
        fSrcPlanet = TRUE;
        lpplSrc = LpplFromId(scan.idpl);
        if (!lpplSrc || lpfl->iPlayer != lpplSrc->iPlayer)
            goto LJumpgate;
        isbsSrc = IStargateFromLppl(lpplSrc);
        if (isbsSrc < 0)
            goto LJumpgate;
        if (fUncertain) {
            return -1;
        }
    } else {
    LJumpgate:
        if (FFleetCanJumpgate(lpfl)) {
            if (fUncertain) {
                return -1;
            }
            isbsSrc = isbsDst;
        } else {
            if (!fSrcPlanet || (lpplSrc && lpplSrc->iPlayer == iplrNone && lpplSrc->turn == game.turn)) {
                return 0;
            }
            if (!lpplSrc || lpplSrc->iPlayer != lpfl->iPlayer) {
                return -1;
            }
            return 0;
        }
    }
    fCargo = FALSE;
    if (GetRaceStat(&rgplr[lpfl->iPlayer], rsMajorAdv) != raStargate) {
        for (i = 0; i <= 3; i++) {
            if (lpfl->rgwtMin[i] > 0) {
                fCargo = TRUE;
            }
        }
    }
    dTravel = LOWORD(Sf64ToI32(DGetDistance(ptSrc.x, ptSrc.y, ptDst.x, ptDst.y)));
    fDanger = FALSE;
    for (ishdef = 0; ishdef < 16; ishdef++) {
        if (lpfl->rgcsh[ishdef] != 0) {
            switch (MdCalcStargateDamage(isbsSrc, isbsDst, dTravel, rglpshdef[lpfl->iPlayer][ishdef].hul.wtEmpty, &pctDmg)) {
            case -2:
            case -1:
            case 0:
                return 0;
            case 1:
                if (pctDmg > 0) {
                    fDanger = TRUE;
                }
            }
        }
    }
    return (!fDanger ? 0 : 2) + 1 + (!fCargo ? 0 : 4);
}

int16_t FFleetCanJumpgate(FLEET *lpfl) {
    HS     *lphs;
    int16_t chs;
    int16_t i;
    int16_t j;

    for (i = 0; i < 16; i++) {
        if (lpfl->rgcsh[i] > 0) {
            lphs = rglpshdef[lpfl->iPlayer][i].hul.rghs;
            chs = rglpshdef[lpfl->iPlayer][i].hul.chs;
            j = 0;
            for (; j < chs && (lphs->cItem == 0 || lphs->grhst != hstSpecialM || lphs->iItem != ispecialMJumpGate); lphs++) {
                j++;
            }
            if (j == chs) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

int32_t WtFromLpfl(FLEET *lpfl) {
    int32_t cMass;
    int16_t i;

    cMass = 0;
    for (i = 0; i < 16; i++) {
        if (lpfl->rgcsh[i] > 0) {
            cMass += (uint32_t)(lpfl->rgcsh[i] * (uint32_t)rglpshdef[lpfl->iPlayer][i].hul.wtEmpty);
        }
    }
    for (i = 0; i <= 3; i++) {
        cMass += lpfl->rgwtMin[i];
    }
    return cMass;
}

int16_t FCanBuildShdef(SHDEF *lpshdef, int16_t iplr) {
    int16_t j;
    int16_t iplrSav;
    PART    part;

    iplrSav = idPlayer;
    if ((int16_t)lpshdef->hul.ihuldef >= ihuldefOrbitalFort) {
        part.hs.grhst = hstSBHull;
        part.hs.iItem = lpshdef->hul.ihuldef - ihuldefOrbitalFort;
    } else {
        part.hs.grhst = hstHull;
        part.hs.iItem = lpshdef->hul.ihuldef;
    }
    idPlayer = iplr;
    if (FLookupPart(&part) != mdPartAvailAvailable) {
    LFail:
        idPlayer = iplrSav;
        return FALSE;
    }
    for (j = 0; j < lpshdef->hul.chs; j++) {
        if (lpshdef->hul.rghs[j].cItem > 0) {
            part.hs = lpshdef->hul.rghs[j];
            if (FLookupPart(&part) != mdPartAvailAvailable)
                goto LFail;
        }
    }
    idPlayer = iplrSav;
    return TRUE;
}

int16_t IshFindSimilarDesign(HUL *lphul, int16_t iPlrDst) {
    SHDEF  *lpshdefDest;
    int16_t i;
    int16_t j;

    lpshdefDest = rglpshdef[iPlrDst];
    i = 0;
    while (i < 16) {
        if (!lpshdefDest->fFree && lpshdefDest->fGift && lpshdefDest->hul.ihuldef == lphul->ihuldef && lpshdefDest->hul.chs == lphul->chs) {
            for (j = 0; j < lphul->chs && lphul->rghs[j].cItem == lpshdefDest->hul.rghs[j].cItem &&
                        (lphul->rghs[j].cItem <= 0 ||
                         (lphul->rghs[j].iItem == lpshdefDest->hul.rghs[j].iItem && lphul->rghs[j].grhst == lpshdefDest->hul.rghs[j].grhst));
                 j++) {
            }
            if (j == lphul->chs) {
                return i;
            }
        }
        i++;
        lpshdefDest++;
    }
    return ishdefNone;
}

void ClearFile(int16_t dt) {
    char *pch;
    char  szFile[256];

    strcpy(szFile, szBase);
    pch = strrchr(szFile, 46);
    if (pch) {
        pch[1] = 0;
    } else {
        strcat(szFile, ".");
    }
    strcat(szFile, mpdtsz[dt]);
    remove(szFile);
    return;
}

void OutputSz(int16_t dt, char *sz) {
    char szTime[100];
    char szFile[256];
    char szDate[100];
    char szTemp[256];

    CchSprintf(szFile, "%s.%s", szBase, mpdtsz[dt]);
    if (!FFileExists(szFile)) {
        CchSprintf(szTemp, "Stars! %s\r\n\r\n", SzVersion());
        OutputFileString(szFile, szTemp);
    }
    GetDateTimeSz(szDate, szTime);
    CchSprintf(szTemp, "%s %s - %s\r\n", szDate, szTime, sz);
    OutputFileString(szFile, szTemp);
    return;
}

void TurnLog(StringId ids) {
    char szTemp[256];

    if (ini.fLogging) {
        CchSprintf(szTemp, PszFormatIds(ids, NULL), game.turn + 2401);
        OutputSz(6, szTemp);
    }
    return;
}

char *SzVersion() {
    /* The original formatted idsVersionD02dC ("Version %d.%02d%c") with 2, 60, 'j'. */
    CchSprintf(szWork, "Version %s", STARS_VERSION_DISPLAY);
    return szWork;
}

int16_t CchGetETA(HDC hdc, FLEET *lpfl, char *sz, int16_t iwp, int16_t fSmall) {
    int16_t  iWarp;
    double   dbl;
    ORDER   *lpord;
    int16_t  i;
    int16_t  c;
    int16_t  iSpeed;
    int16_t  j;
    int16_t  cYears;
    StringId ids;

    cYears = 0;
    lpord = lpfl->lpplord->rgord;
    i = 0;
    while (i < iwp) {
        dbl = DGetDistance(lpord->pt.x, lpord->pt.y, lpord[1].pt.x, lpord[1].pt.y);
        iWarp = lpord[1].iWarp;
        if (iWarp < 11) {
            iSpeed = iWarp * iWarp;
        } else {
            j = FCanFleetUseStargates(lpfl, lpord->pt, lpord[1].pt);
            switch (j) {
            case -1:
                iSpeed = -3;
                break;
            case 0:
                iSpeed = 0;
                break;
            case 1:
                iSpeed = 8000;
                break;
            default:
                if (j & 2) {
                    iSpeed = -1;
                } else {
                    iSpeed = -2;
                }
            }
        }
        if (iSpeed == 0) {
            if (hdc) {
                SetHdcTextColor(hdc, 0xff);
            }
            c = CchGetString(idsNever, sz);
            return c;
        }
        if (iSpeed < 0) {
            if (hdc) {
                SetHdcTextColor(hdc, 32639);
            }
            if (iSpeed == -1) {
                ids = idsDanger;
            } else if (iSpeed == -2) {
                ids = idsUnload2;
            } else {
                ids = idsUncertain;
            }
            c = CchGetString(ids, sz);
            return c;
        }
        if (iSpeed >= (int16_t)LOWORD(Sf64ToI32(dbl))) {
            iSpeed = 1;
        } else {
            iSpeed = (int16_t)(LOWORD(Sf64ToI32(dbl)) + iSpeed - 1) / iSpeed;
        }
        cYears += iSpeed;
        i++;
        lpord++;
    }
    c = CchSprintf(sz, PszGetCompressedString(!fSmall ? idsDYear : idsDy), cYears);
    if (cYears != 1 && !fSmall) {
        sz[c++] = 's';
    }
    return c;
}
