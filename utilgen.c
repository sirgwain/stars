#include "common.h"

int16_t rgPrimes[128] = {3,   5,   7,   11,  13,  17,  19,  23,  29,  31,  37,  41,  43,  47,  53,  59,  61,  67,  71,  73,  79,  83,  89,  97,  101, 103,
                         107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241,
                         251, 257, 263, 279, 271, 277, 281, 283, 293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359, 367, 373, 379, 383, 389, 397, 401,
                         409, 419, 421, 431, 433, 439, 443, 449, 457, 461, 463, 467, 479, 487, 491, 499, 503, 509, 521, 523, 541, 547, 557, 563, 569, 571,
                         577, 587, 593, 599, 601, 607, 613, 617, 619, 631, 641, 643, 647, 653, 659, 661, 673, 677, 683, 691, 701, 709, 719, 727};
int32_t lFileSeed1 = 0;
int32_t lFileSeed2 = 0;

void PushRandom(int32_t lNew1, int32_t lNew2) {
    rglRandStack[cRandStack][0] = lRandSeed1;
    rglRandStack[cRandStack][1] = lRandSeed2;
    cRandStack++;
    lRandSeed1 = lNew1;
    lRandSeed2 = lNew2;
    return;
}

void PopRandom() {
    cRandStack--;
    lRandSeed1 = rglRandStack[cRandStack][0];
    lRandSeed2 = rglRandStack[cRandStack][1];
    return;
}

void Randomize(uint32_t dw) {
    int16_t a;
    int16_t b;

    a = LOWORD(dw) & 0x3f;
    b = LOWORD((uint32_t)(dw >> 6)) & 0x3f;
    if (a == b) {
        b = (b + 1) & 0x3f;
    }
    lRandSeed1 = rgPrimes[a];
    lRandSeed2 = rgPrimes[b];
    return;
}

void Randomize2(uint32_t dw) {
    int16_t a;
    int16_t b;

    a = LOWORD(dw) & 0x7f;
    b = LOWORD((uint32_t)(dw >> 7)) & 0x7f;
    a ^= 0x35;
    b ^= 0x5c;
    if (a == b) {
        b = (b + 1) & 0x7f;
    }
    lRandSeed1 = rgPrimes[a];
    lRandSeed2 = rgPrimes[b];
    return;
}

int16_t Random(int16_t c) {
    int32_t z;
    int32_t s1;
    int32_t k;
    int32_t s2;

    s1 = lRandSeed1;
    s2 = lRandSeed2;
    k = (int32_t)(s1 / 53668);
    s1 = (uint32_t)((s1 - (uint32_t)(k * 53668)) * 0x9c4e) - (uint32_t)(k * 12211);
    if (s1 < 0) {
        s1 += 2147483563;
    }
    k = (int32_t)(s2 / 52774);
    s2 = (uint32_t)((s2 - (uint32_t)(k * 52774)) * 0x9ef4) - (uint32_t)(k * 3791);
    if (s2 < 0) {
        s2 += 2147483399;
    }
    z = s1 - s2;
    if (z < 1) {
        z += 2147483562;
    }
    lRandSeed1 = s1;
    lRandSeed2 = s2;
    if (c <= 0) {
        return 0;
    }
    return LOWORD((uint32_t)((uint32_t)z % c));
}

void GetFileSeeds(int32_t *pl1, int32_t *pl2) {
    *pl1 = lFileSeed1;
    *pl2 = lFileSeed2;
    return;
}

void SetFileSeeds(int32_t l1, int32_t l2) {
    lFileSeed1 = l1;
    lFileSeed2 = l2;
    return;
}

void SetFileXorStream(int32_t lid, int16_t lSalt, int16_t turn, int16_t iPlayer, int16_t fCrippled) {
    int16_t a;
    int16_t b;

    a = lSalt & 0x1f;
    b = lSalt >> 5 & 0x1f;
    if (lSalt & 0x400) {
        a += 32;
    } else {
        b += 32;
    }
    lFileSeed1 = rgPrimes[a];
    lFileSeed2 = rgPrimes[b];
    a = ((LOWORD(lid) & 3) + 1) * ((turn & 3) + 1) * ((iPlayer & 3) + 1) + fCrippled;
    while (a-- > 0) {
        LGetNextFileXor();
    }
    return;
}

int32_t LGetNextFileXor() {
    int32_t s1;
    int32_t k;
    int32_t s2;

    s1 = lFileSeed1;
    s2 = lFileSeed2;
    k = (int32_t)(s1 / 53668);
    s1 = (uint32_t)((s1 - (uint32_t)(k * 53668)) * 0x9c4e) - (uint32_t)(k * 12211);
    if (s1 < 0) {
        s1 += 2147483563;
    }
    k = (int32_t)(s2 / 52774);
    s2 = (uint32_t)((s2 - (uint32_t)(k * 52774)) * 0x9ef4) - (uint32_t)(k * 3791);
    if (s2 < 0) {
        s2 += 2147483399;
    }
    lFileSeed1 = s1;
    lFileSeed2 = s2;
    return s1 - s2;
}

void XorFileBuf(char *rgb, int16_t cb) {
    int32_t *plMac;
    int32_t *pl;
    int32_t  lPrev;
    char    *pch;

    lPrev = 0;
    pl = (int32_t *)rgb;
    plMac = pl + (cb >> 2);
    for (; pl < plMac; pl++) {
        *pl ^= LGetNextFileXor();
    }
    cb &= 3;
    if (cb > 0) {
        pch = (char *)pl;
        lPrev = LGetNextFileXor();
        while (cb-- != 0) {
            *pch ^= (int16_t)(int8_t)LOBYTE(LOWORD(lPrev) & 0xff);
            lPrev = (int32_t)(lPrev >> 8);
            pch++;
        }
    }
    return;
}

int ICompLong(int32_t *pl1, int32_t *pl2) { return (int16_t)(*pl1 - *pl2); }

char *PszGetCompressedPlanet(int16_t id) {
    int16_t  fCap;
    int16_t  iOffset;
    int16_t  fHigh;
    int16_t  iChunk;
    int16_t  i;
    int16_t  iBuild;
    int16_t  iNibble;
    uint8_t *pchLen;
    char    *pszOut;
    uint8_t *pch;
    int16_t  iLen;

    iNibble = 0;
    if (id >= cPlanetName) {
        id %= cPlanetName;
    }
    if (id == iLastGet) {
        return szLastGet;
    }
    iChunk = id >> 6;
    iOffset = id & 0x3f;
    pch = &aPNCmpr[aiPNChunkOffset[iChunk]];
    pchLen = &acPN[iChunk * 64];
    i = 0;
    while (i < iOffset) {
        iNibble += *pchLen;
        i++;
        pchLen++;
    }
    pch += iNibble >> 1;
    iLen = *pchLen;
    fHigh = (iNibble & 1) == 0;
    pszOut = szLastGet;
    iBuild = 0;
    fCap = TRUE;
    while (iLen-- != 0) {
        if (fHigh) {
            i = *pch >> 4;
        } else {
            i = *pch++ & 0xf;
        }
        fHigh = fHigh == 0;
        iBuild += i;
        if (i != 15) {
            *pszOut = rgPNLookupTable[iBuild];
            if (fCap && *pszOut >= 'a' && *pszOut <= 'z') {
                *pszOut -= ' ';
            }
            if (*pszOut == ' ' || *pszOut == '-') {
                fCap = TRUE;
            } else {
                fCap = FALSE;
            }
            pszOut++;
            iBuild = 0;
        }
    }
    *pszOut = 0;
    return szLastGet;
}

void OutputFileString(char *szFile, char *sz) {
    int16_t  fMissing;
    uint16_t w;
    int16_t  hf;

    w = 2;
    if (!FFileExists(szFile)) {
        w |= 0x1000;
    }
    hf = HfOpenFile(szFile, w, &fMissing);
    if (hf != -1) {
        LSeekFile(hf, 0, 2);
        CbWriteFile(hf, sz, strlen(sz));
        CloseFile(hf);
    }
    return;
}

void StarsCopyFile(char *szSrc, char *szDst) {
    char     rgb[2048];
    int16_t  fFileErrSav;
    int16_t  fMissing;
    jmp_buf  env;
    int16_t  hfDst;
    int32_t  cb;
    jmp_buf *penvSav;

    fFileErrSav = fFileErrSilent;
    hfDst = -1;
    fFileErrSilent = TRUE;
    penvSav = penvMem;
    penvMem = &env;
    if (setjmp(env) == 0) {
        StreamOpen(szSrc, mdRead);
        hfDst = HfOpenFile(szDst, 4114, &fMissing);
        if (hfDst == -1)
            goto LStreamError;
        for (cb = CbFileSize(hf); cb > 2048; cb -= 2048) {
            RgFromStream(rgb, 0x800);
            if (CbWriteFile(hfDst, rgb, 0x800) != 0x800)
                goto LStreamError;
        }
        if (cb != 0) {
            RgFromStream(rgb, LOWORD(cb));
            CbWriteFile(hfDst, rgb, LOWORD(cb));
        }
    }
LStreamError:
    StreamClose();
    /* The original closed hf, leaving the copy open with exclusive sharing,
       and returned early on a failed OpenFile without restoring penvMem. */
    if (hfDst != -1) {
        CloseFile(hfDst);
    }
    penvMem = penvSav;
    fFileErrSilent = fFileErrSav;
    return;
}

int16_t AlertSz(char *sz, int16_t mbType) {
    char szT[256];

    if (ini.fValidate || (ini.fLogging && ini.fGen)) {
        CchSprintf(szT, "Error: %s", sz);
        OutputSz(!ini.fValidate ? 6 : 7, szT);
        return IDYES;
    }
    return IdAlertBox(sz, mbType);
}

int16_t CchGetString(StringId ids, char *psz) {
    char *pszT;
    char *pszTT;

    pszTT = psz;
    pszT = PszGetCompressedString(ids);
    while (*pszT != 0) {
        *psz++ = *pszT++;
    }
    *psz = 0;
    return psz - pszTT;
}

char *PszFromInt(int16_t i, int16_t *pcch) {
    int16_t cch;

    cch = CchSprintf(szFormatNumber, PCTD, i);
    if (pcch) {
        *pcch = cch;
    }
    return szFormatNumber;
}

char *PszFromLong(int32_t l, int16_t *pcch) {
    int16_t cch;

    cch = CchSprintf(szFormatNumber, PCTLD, l);
    /* The original tested *pcch, reading through the report dumps' NULL. */
    if (pcch) {
        *pcch = cch;
    }
    return szFormatNumber;
}

char *PszFromLongK(int32_t l, int16_t *pcch) {
    int16_t fExtraLarge;
    int16_t fLarge;
    char   *psz;

    fLarge = l >= 10000;
    fExtraLarge = l >= 1000000;
    if (fExtraLarge) {
        l = (int32_t)((l + 500000) / 0xf4240);
        if (l > 999) {
            l = 999;
        }
    } else if (fLarge) {
        l = (int32_t)((l + 500) / 1000);
        if (l > 999) {
            l = 999;
        }
    }
    psz = PszFromInt(LOWORD(l), pcch);
    if (fExtraLarge) {
        psz[(*pcch)++] = 'M';
    } else if (fLarge) {
        psz[(*pcch)++] = 'k';
    }
    return psz;
}

int16_t CommaFormatLong(char *psz, int32_t l) {
    char    rgch[15];
    int16_t c;
    int16_t cSkip;
    char   *pchOut;
    char   *pch;

    c = CchSprintf(rgch, PCTLD, l);
    pch = rgch;
    pchOut = psz;
    cSkip = c % 3;
    if (cSkip == 0) {
        cSkip = 3;
    } else if (cSkip == 1 && l < 0) {
        cSkip = 4;
    }
    while (cSkip-- > 0) {
        *pchOut++ = *pch++;
    }
    while (*pch != 0) {
        cSkip = 3;
        *pchOut++ = ',';
        while (cSkip-- > 0) {
            *pchOut++ = *pch++;
        }
    }
    *pchOut = 0;
    return pchOut - psz;
}

void BoundPoints(RECT *prc, POINT16 *rgpt, int16_t cpt) {
    int16_t ipt;
    int16_t xMax;
    int16_t yMax;
    int16_t xMin;
    int16_t yMin;

    xMax = rgpt->x;
    xMin = rgpt->x;
    yMax = rgpt->y;
    yMin = rgpt->y;
    for (ipt = 1; ipt < cpt; ipt++) {
        xMin = xMin >= rgpt[ipt].x ? rgpt[ipt].x : xMin;
        xMax = xMax <= rgpt[ipt].x ? rgpt[ipt].x : xMax;
        yMin = yMin >= rgpt[ipt].y ? rgpt[ipt].y : yMin;
        yMax = yMax <= rgpt[ipt].y ? rgpt[ipt].y : yMax;
    }
    prc->top = yMin - 1;
    prc->bottom = yMax + 1;
    prc->left = xMin - 1;
    prc->right = xMax + 1;
    return;
}

int16_t FCompressUserString(char *szIn, char *szOut, int16_t *pcOut) {
    int16_t fHalf;
    char    szWork[1024];
    int16_t iNyb;
    char   *pchOut;
    int16_t cNyb;

    fHalf = FALSE;
    pchOut = szWork;
    for (; *szIn != 0; szIn++) {
        iNyb = NybbleFromCh(*szIn);
        if (iNyb < 11) {
            cNyb = 1;
        } else if ((iNyb & 0xf) != 0xf) {
            cNyb = 2;
        } else {
            cNyb = 3;
        }
        while (cNyb-- != 0) {
            if (!fHalf) {
                *pchOut = (iNyb & 0xf) * 0x10;
                fHalf = TRUE;
            } else {
                *pchOut |= iNyb & 0xf;
                pchOut++;
                fHalf = FALSE;
                if (pchOut - szWork >= 0x400) {
                    return FALSE;
                }
            }
            iNyb >>= 4;
        }
    }
    if (fHalf) {
        *pchOut |= 0xf;
        pchOut++;
    }
    if (pchOut - szWork > *pcOut) {
        return FALSE;
    }
    *pcOut = pchOut - szWork;
    memcpy(szOut, szWork, *pcOut);
    return TRUE;
}

int16_t FDecompressUserString(char *szIn, int16_t cIn, char *szOut, int16_t *pcOut) {
    int16_t fHalf;
    char    szWork[1024];
    int16_t iNyb;
    char   *pchOut;

    fHalf = FALSE;
    pchOut = szWork;
    while (cIn > 0) {
        if (fHalf) {
            iNyb = *szIn & 0xf;
            cIn--;
            szIn++;
            if (iNyb == 15 && cIn == 0)
                break;
        } else {
            iNyb = *szIn >> 4 & 0xf;
        }
        fHalf = fHalf == 0;
        if (iNyb >= 11) {
            if (fHalf) {
                iNyb |= (*szIn & 0xf) << 4;
                cIn--;
                szIn++;
            } else {
                iNyb |= *szIn & 0xf0;
            }
            fHalf = fHalf == 0;
            if ((iNyb & 0xf) == 0xf) {
                if (fHalf) {
                    iNyb |= (*szIn & 0xf) << 8;
                    cIn--;
                    szIn++;
                } else {
                    iNyb |= (*szIn & 0xf0) << 4;
                }
                fHalf = fHalf == 0;
            }
        }
        *pchOut = ChFromNybble(iNyb);
        pchOut++;
        if (pchOut - szWork > *pcOut) {
            return FALSE;
        }
    }
    *pchOut = 0;
    strcpy(szOut, szWork);
    return TRUE;
}

int16_t NybbleFromCh(uint8_t ch) {
    char *pch;

    if (ch >= 97 && ch <= 122) {
        return rgcompstrlower[ch - 97];
    }
    if (ch == 32) {
        return 0;
    }
    if (ch >= 65 && ch <= 80) {
        return (ch - 0x41) << 4 | 0xb;
    }
    if (ch >= 81 && ch <= 90) {
        return (ch - 0x51) << 4 | 0xc;
    }
    if (ch >= 48 && ch <= 53) {
        return (ch - 0x26) << 4 | 0xc;
    }
    if (ch >= 54 && ch <= 57) {
        return (ch - 0x36) << 4 | 0xd;
    }
    pch = strchr(rgchcomp, ch);
    if (pch) {
        return (pch - rgchcomp + 4) << 4 | 0xe;
    }
    return ch << 4 | 0xf;
}

char ChFromNybble(int16_t nyb) {
    int16_t iPage;
    int16_t iVal;

    if (nyb < 11) {
        return rgchcompstrlower[nyb];
    }
    iPage = nyb & 0xf;
    if (iPage == 15) {
        return nyb >> 4;
    }
    iVal = (iPage - 0xb) * 0x10 + (nyb >> 4);
    if (iVal < 26) {
        return iVal + 65;
    }
    if (iVal < 36) {
        return iVal + 22;
    }
    if (iVal >= 52) {
        return rgchcomp[iVal - 52];
    }
    return rgchcompstrlower[iVal - 25];
}

int16_t FIntersectCircleLine(POINT16 ptL1, POINT16 ptL2, POINT16 ptC, int32_t r2, int16_t dMax, int16_t *pdStart, int16_t *pdEnd) {
    int32_t dyT;
    int32_t dxdy;
    int16_t dCtr;
    int32_t dy2;
    int32_t dy;
    int32_t yI;
    int32_t dyI;
    int32_t dxT;
    int32_t lT;
    int32_t dx2;
    int16_t dOff;
    int32_t dx;
    int32_t dxI;
    int32_t xI;
    int32_t r2I;

    dx = (int16_t)(ptL2.x - ptL1.x);
    dy = (int16_t)(ptL2.y - ptL1.y);
    dxdy = (uint32_t)(dx * dy);
    dx2 = (uint32_t)(dx * dx);
    dy2 = (uint32_t)(dy * dy);
    if (dxdy > 500000 || dx2 > 500000 || dy2 > 500000) {
        dxI = Sf64ToI32(Sf64From80(
            (Sf80Div((Sf80Add(Sf80Add(Sf80Mul(Sf80FromI32(dxdy), Sf80FromI32((int16_t)(ptC.y - ptL1.y))), Sf80Mul(Sf80FromI32(dx2), Sf80FromI32(ptC.x))),
                              Sf80Mul(Sf80FromI32(dy2), Sf80FromI32(ptL1.x)))),
                     Sf80FromI32((dx2 + dy2))))));
        xI = (int32_t)dxI;
    } else {
        xI = (int32_t)((int32_t)((uint32_t)(dxdy * (int16_t)(ptC.y - ptL1.y)) + (uint32_t)(dx2 * ptC.x) + (uint32_t)(dy2 * ptL1.x)) / (dx2 + dy2));
    }
    if (dx != 0) {
        yI = ptL1.y + (int32_t)((int32_t)((xI - ptL1.x) * dy) / dx);
    } else {
        /* A vertical route's closest point is level with the field's center.
           The original used ptL1.y, measuring from the route's start. */
        yI = ptC.y;
    }
    dxI = xI - ptC.x;
    dyI = yI - ptC.y;
    r2I = (uint32_t)(dxI * dxI) + (uint32_t)(dyI * dyI);
    if (r2 <= r2I) {
        return FALSE;
    }
    dxT = xI - ptL1.x;
    dyT = yI - ptL1.y;
    lT = (uint32_t)(dxT * dxT) + (uint32_t)(dyT * dyT);
    dCtr = LOWORD(Sf64ToI32(Sf64Sqrt(Sf64FromI32(lT))));
    lT = r2 - r2I;
    if (lT <= 0) {
        return FALSE;
    }
    dOff = LOWORD(Sf64ToI32(Sf64Sqrt(Sf64FromI32(lT))));
    if (ptL1.x < ptL2.x) {
        if (xI < ptL1.x) {
            dCtr = -dCtr;
        }
    } else if (ptL1.x > ptL2.x) {
        if (xI > ptL1.x) {
            dCtr = -dCtr;
        }
    } else if (ptL1.y < ptL2.y) {
        if (yI < ptL1.y) {
            dCtr = -dCtr;
        }
    } else if (yI > ptL1.y) {
        dCtr = -dCtr;
    }
    *pdStart = 0 <= dCtr - dOff ? dCtr - dOff : 0;
    *pdEnd = dMax >= dCtr + dOff ? dCtr + dOff : dMax;
    if (*pdEnd <= 0 || *pdStart >= dMax) {
        return FALSE;
    }
    return TRUE;
}

void IntToRoman(int16_t i, char *pszOut) {
    if (i <= 0) {
        *pszOut = 0;
    } else {
        for (; i >= 10; i -= 10) {
            *pszOut++ = 'X';
        }
        if (i == 9) {
            *pszOut++ = 'I';
            *pszOut++ = 'X';
        } else {
            if (i >= 4) {
                if (i == 4) {
                    *pszOut++ = 'I';
                }
                *pszOut++ = 'V';
                i -= 5;
            }
            while (i-- > 0) {
                *pszOut++ = 'I';
            }
        }
        *pszOut = 0;
    }
    return;
}

int16_t FCheckPassword() {
    int32_t lSaltDef;

    if (lSaltCur == 0 || lSaltLast == lSaltCur || fAi) {
        return TRUE;
    }
    if (vszDefPass[0] != 0) {
        lSaltDef = LSaltFromSz(vszDefPass);
        if (lSaltDef == lSaltCur) {
            return TRUE;
        }
    }
    if (ini.fValidate) {
        return FALSE;
    }
    return PromptPassword();
}

int32_t LSaltFromSz(char *psz) {
    int32_t lSalt;

    lSalt = 0;
    if (*psz == 0) {
        return 0;
    }
    while (*psz != 0) {
        lSalt += (int16_t)*psz;
        psz++;
        if (*psz != 0) {
            lSalt = (uint32_t)(lSalt * (int16_t)*psz);
            psz++;
        }
    }
    if (lSalt == 0) {
        lSalt = 1;
    }
    return lSalt;
}

int32_t LDistance2(POINT16 pt1, POINT16 pt2) {
    int32_t dy;
    int32_t dx;

    dx = (int16_t)(pt1.x - pt2.x);
    dy = (int16_t)(pt1.y - pt2.y);
    return (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
}

char *PszGetLine(char **ppszBeg) {
    char *pszStart;
    char *psz;

    for (psz = *ppszBeg; *psz == ' '; psz++) {
    }
    pszStart = psz;
    for (; *psz != 0 && *psz != '\n' && *psz != '\r'; psz++) {
    }
    if (*psz == '\r' && psz[1] == '\n') {
        *ppszBeg = psz + 2;
    } else {
        *ppszBeg = psz + 1;
    }
    *psz = 0;
    return pszStart;
}

int16_t CParseNumbers(char *psz, int32_t *pl, int16_t cMax) {
    int16_t iRead;
    int16_t fValid;
    int32_t lNum;

    iRead = 0;
    lNum = 0;
    fValid = FALSE;
    for (; iRead < cMax && *psz != 0; psz++) {
        if (*psz != ' ' && (*psz < '0' || *psz > '9')) {
            return -1;
        }
        if (*psz != ' ') {
            fValid = TRUE;
            lNum = (uint32_t)(lNum * 10) + (int16_t)(*psz - '0');
        } else if (fValid) {
            pl[iRead++] = lNum;
            lNum = 0;
            fValid = FALSE;
        }
    }
    if (fValid) {
        pl[iRead++] = lNum;
    }
    return iRead;
}
