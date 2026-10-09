// Frozen MinGW results for the SoftFloat migration. Expected files are read
// only: a failed run leaves *.actual.txt in the build's test directory.
#include "acutest.h"
#include "stars_test.h"

static FILE *pfActual;
static FILE *pfExpected;
static int   cValues;
#ifdef STARS_TEST_RANDOM_WRAP
static int     cRandom;
static int32_t lRandomFirst;
static int16_t fWatchRandom;

int16_t __real_Random(int16_t lMax);
int16_t __wrap_Random(int16_t lMax) {
    if (fWatchRandom && cRandom++ == 0)
        lRandomFirst = lMax;
    return __real_Random(lMax);
}
#endif

static void BeginValues(const char *pszName) {
    char szPath[MAX_PATH];

    snprintf(szPath, sizeof(szPath), "%s.actual.txt", pszName);
    pfActual = fopen(szPath, "w");
    TEST_ASSERT(pfActual != NULL);
    snprintf(szPath, sizeof(szPath), "%s/%s.txt", STARS_TEST_GOLDEN_DIR, pszName);
    pfExpected = fopen(szPath, "r");
    cValues = 0;
}

static void CheckValue(const char *pszName, uint64_t ullValue) {
    char szActual[128];
    char szExpected[128];

    snprintf(szActual, sizeof(szActual), "%04d %-24s %016llx\n", cValues++, pszName, (unsigned long long)ullValue);
    fputs(szActual, pfActual);
    if (pfExpected) {
        TEST_ASSERT(fgets(szExpected, sizeof(szExpected), pfExpected) != NULL);
        TEST_CHECK_(strcmp(szActual, szExpected) == 0, "actual %sexpected %s", szActual, szExpected);
    }
}

static void EndValues(void) {
    fclose(pfActual);
    TEST_CHECK_(pfExpected != NULL, "missing frozen reference; inspect the .actual.txt file");
    if (pfExpected) {
        TEST_CHECK_(fgetc(pfExpected) == EOF, "unused reference rows");
        fclose(pfExpected);
    }
}

static void CheckText(const char *pszName, const char *pszValue) {
    char szActual[128];
    char szExpected[128];

    snprintf(szActual, sizeof(szActual), "%04d %-24s %s\n", cValues++, pszName, pszValue);
    fputs(szActual, pfActual);
    if (pfExpected) {
        TEST_ASSERT(fgets(szExpected, sizeof(szExpected), pfExpected) != NULL);
        TEST_CHECK_(strcmp(szActual, szExpected) == 0, "actual %sexpected %s", szActual, szExpected);
    }
}

#ifdef STARS_TEST_RANDOM_WRAP
static int16_t fRecordMessages;
static int cMessages;
int16_t __real_FSendPlrMsg(int16_t iPlr, MessageId iMsg, MsgGoto iObj, int16_t p1, int16_t p2, int16_t p3, int16_t p4,
                         int16_t p5, int16_t p6, int16_t p7);
int16_t __wrap_FSendPlrMsg(int16_t iPlr, MessageId iMsg, MsgGoto iObj, int16_t p1, int16_t p2, int16_t p3, int16_t p4,
                         int16_t p5, int16_t p6, int16_t p7) {
    if (fRecordMessages) {
        CheckValue("message player", (uint16_t)iPlr);
        CheckValue("message id", (uint16_t)iMsg);
        CheckValue("message p3", (uint16_t)p3);
        CheckValue("message p4", (uint16_t)p4);
        CheckValue("message p5", (uint16_t)p5);
        cMessages++;
    }
    return __real_FSendPlrMsg(iPlr, iMsg, iObj, p1, p2, p3, p4, p5, p6, p7);
}
#endif

static uint64_t BitsDouble(double d) {
    uint64_t ullBits;

    memcpy(&ullBits, &d, sizeof(ullBits));
    return ullBits;
}

static uint32_t BitsFloat(float pct) {
    uint32_t dwBits;

    memcpy(&dwBits, &pct, sizeof(dwBits));
    return dwBits;
}

static PLANET *LoadHomeworld(const char *pszName) {
    char    szDir[MAX_PATH];
    PLANET *lppl;

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir(pszName, szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(szDir, 12345, NULL, 0));
    TEST_ASSERT(FStarsTestLoadHost());
    lppl = LpplStarsTestHomeworld(0);
    TEST_ASSERT(lppl != NULL);
    idPlayer = 0;
    return lppl;
}

static void test_distance_bits(void) {
    static const int16_t rgxy[][2] = {{0, 0},   {1, 0},     {1, 1},       {3, 4},       {80, 1},    {81, 1},    {99, 14},
                                      {100, 1}, {400, 399}, {1200, 1199}, {3000, 4000}, {10000, 1}, {32767, 0}, {32767, 32766}};
    int                  i;
    int                  sx;
    int                  sy;
    double               d;

    TEST_ASSERT(sizeof(double) == sizeof(uint64_t));
    BeginValues("floating-distance");
    for (i = 0; i < (int)(sizeof(rgxy) / sizeof(rgxy[0])); i++) {
        for (sx = -1; sx <= 1; sx += 2) {
            for (sy = -1; sy <= 1; sy += 2) {
                d = DGetDistance(0, 0, sx * rgxy[i][0], sy * rgxy[i][1]);
                CheckValue("distance bits", BitsDouble(d));
                CheckValue("reversed bits", BitsDouble(DGetDistance(sx * rgxy[i][0], sy * rgxy[i][1], 0, 0)));
            }
        }
    }
    TEST_CHECK(DGetDistance(0, 0, 3, 4) == 5.0);
    EndValues();
}

static void test_defense_bits(void) {
    static const int16_t rgTech[] = {0, 4, 7, 10, 16, 26};
    static const int32_t rgPop[] = {0, 1, 24, 25, 26, 2499, 2500, 2501};
    PLANET              *lppl;
    float                pct;
    float                pctSmart;
    float                pctPlain;
    int                  i;
    int                  j;

    lppl = LoadHomeworld("floating-defense");
    TEST_ASSERT(sizeof(float) == sizeof(uint32_t));
    BeginValues("floating-defense");
    lppl->rgwtMin[3] = 10000;
    for (i = 0; i < (int)(sizeof(rgTech) / sizeof(rgTech[0])); i++) {
        for (j = 0; j < 6; j++)
            rgplr[0].rgTech[j] = rgTech[i];
        for (j = 0; j <= 101; j++) {
            lppl->cDefenses = j;
            CalcPctSurvive(lppl, &pct, &pctSmart);
            CalcPctSurvive(lppl, &pctPlain, NULL);
            TEST_CHECK(BitsFloat(pct) == BitsFloat(pctPlain));
            TEST_CHECK(pct >= 0 && pct <= 1 && pctSmart >= pct && pctSmart <= 1);
            CheckValue("defense bits", BitsFloat(pct));
            CheckValue("smart defense bits", BitsFloat(pctSmart));
        }
    }
    for (i = 0; i < (int)(sizeof(rgPop) / sizeof(rgPop[0])); i++) {
        lppl->rgwtMin[3] = rgPop[i];
        CalcPctSurvive(lppl, &pct, &pctSmart);
        CheckValue("population cap bits", BitsFloat(pct));
        CheckValue("smart cap bits", BitsFloat(pctSmart));
    }
    lppl->iPlayer = iplrNone;
    CalcPctSurvive(lppl, &pct, &pctSmart);
    TEST_CHECK(pct == 1 && pctSmart == 1);
    EndValues();
}

static void test_planet_and_race_rounding(void) {
    static const int32_t rgPop[] = {0, 1, 2, 24, 25, 26, 99, 100, 101, 9999, 10000, 10001, 25000};
    static const int16_t rgTech[] = {0, 1, 7, 26};
    PLANET              *lppl;
    PLAYER               plr;
    int                  i;
    int                  j;
    int                  k;

    lppl = LoadHomeworld("floating-planet");
    BeginValues("floating-planet");
    for (i = 0; i < 6; i++) {
        plr = vrgplrDef[i];
        CheckValue("innate habitability", (uint32_t)LInnateRaceHabitability(&plr));
        SetRaceGrbit(&plr, ibitRaceTT, TRUE);
        CheckValue("innate with TT", (uint32_t)LInnateRaceHabitability(&plr));
    }
    for (i = 0; i <= 100; i++) {
        for (j = 0; j < 3; j++)
            lppl->rgEnvVar[j] = i;
        CheckValue("planet habitability", (uint16_t)PctPlanetDesirability(lppl, 0));
    }
    SetRaceStat(&rgplr[0], rsMajorAdv, raMacintosh);
    for (i = 0; i < 3; i++)
        lppl->rgEnvVar[i] = rgplr[0].rgEnvVar[i];
    for (i = 0; i < (int)(sizeof(rgTech) / sizeof(rgTech[0])); i++) {
        rgplr[0].rgTech[0] = rgTech[i];
        for (j = 0; j < (int)(sizeof(rgPop) / sizeof(rgPop[0])); j++) {
            lppl->rgwtMin[3] = rgPop[j];
            CheckValue("AR resources", (uint16_t)CResourcesAtPlanet(lppl, 0));
            CheckValue("AR mines", (uint16_t)CMinesOperating(lppl));
        }
    }
    // Exercise unequal positive environmental contributions to the sqrt.
    for (i = 30; i <= 70; i += 10) {
        for (j = 30; j <= 70; j += 10) {
            for (k = 30; k <= 70; k += 10) {
                lppl->rgEnvVar[0] = i;
                lppl->rgEnvVar[1] = j;
                lppl->rgEnvVar[2] = k;
                CheckValue("mixed habitability", (uint16_t)PctPlanetDesirability(lppl, 0));
            }
        }
    }
    EndValues();
}

static void test_scanner_and_cloak_rounding(void) {
    static const int16_t rgCount[] = {1, 2, 3, 16};
    static const int16_t rgShips[] = {499, 500, 501, 2000, 32767};
    SHDEF                shdef;
    FLEET                fl;
    int16_t              dPlan;
    int16_t              pctDetect;
    int16_t              iSteal;
    int                  i;
    int                  j;

    LoadHomeworld("floating-scanner");
    BeginValues("floating-scanner");
    memset(&shdef, 0, sizeof(shdef));
    shdef.hul.ihuldef = ihuldefScout;
    shdef.hul.chs = 1;
    shdef.hul.rghs[0].grhst = hstScanner;
    for (i = 0; i < iscannerCount; i++) {
        shdef.hul.rghs[0].iItem = i;
        for (j = 0; j < (int)(sizeof(rgCount) / sizeof(rgCount[0])); j++) {
            shdef.hul.rghs[0].cItem = rgCount[j];
            CheckValue("scanner range", (uint16_t)GetShdefScannerRange(&shdef, 0, &dPlan, &pctDetect, &iSteal));
            CheckValue("penetrating range", (uint16_t)dPlan);
            CheckValue("scanner detection", (uint16_t)pctDetect);
            CheckValue("scanner stealing", (uint16_t)iSteal);
        }
    }
    memset(&fl, 0, sizeof(fl));
    fl.iPlayer = 0;
    memset(&rglpshdef[0][0], 0, sizeof(SHDEF));
    rglpshdef[0][0].hul.wtEmpty = 1000;
    rglpshdef[0][0].hul.chs = 1;
    rglpshdef[0][0].hul.rghs[0].grhst = hstSpecialE;
    rglpshdef[0][0].hul.rghs[0].iItem = ispecialEUltraStealthCloak;
    rglpshdef[0][0].hul.rghs[0].cItem = 1;
    for (i = 0; i < (int)(sizeof(rgShips) / sizeof(rgShips[0])); i++) {
        fl.rgcsh[0] = rgShips[i];
        for (j = 0; j < 4; j++) {
            fl.rgwtMin[j] = 12345 + j;
            CheckValue("loaded cloak", (uint16_t)PctCloakFromLpfl(&fl));
        }
    }
    EndValues();
}

#ifdef STARS_TEST_RANDOM_WRAP
static void test_AI_random_rounding(void) {
    PLANET              *lppl;
    int                  i;
    static const int16_t rgExpected[] = {119, 239, 359, 479, 599};

    lppl = LoadHomeworld("floating-AI");
    for (i = 0; i < 5; i++) {
        game.mdSize = i;
        cRandom = 0;
        fWatchRandom = TRUE;
        IdGetBestScannerDest(lppl, dirEast);
        fWatchRandom = FALSE;
        TEST_CHECK(cRandom >= 2);
        TEST_CHECK_(lRandomFirst == rgExpected[i], "size %d: Random(%ld), expected %d", i, (long)lRandomFirst, rgExpected[i]);
    }
}
#endif

static void test_freighter_destination(void) {
    static const int16_t rgDistance[] = {24, 25, 26, 49, 50, 51};
    PLANET *lppl;
    FLEET *lpfl;
    int16_t idTarget;
    int i;

    lppl = LoadHomeworld("floating-freighter");
    vlpbAiPlanet = LpAlloc(game.cPlanMax * 16, htMisc);
    TEST_ASSERT(vlpbAiPlanet != NULL);
    memset(vlpbAiPlanet, 0, game.cPlanMax * 16);
    rglpshdef[0][0].hul = LphuldefFromId(ihuldefLargeFreighter)->hul;
    lpfl = LpflStarsTestAddFleet(0, lppl->id, 0, 1);
    idTarget = (lppl->id + 1) % cPlanet;
    lpPlanets[idTarget].iPlayer = iplrNone;
    vlpbAiPlanet[idTarget * 16 + 1] = 0x80 | 30;
    for (i = 0; i < 3; i++)
        lppl->rgwtMin[i] = 1000;
    BeginValues("floating-freighter");
    for (i = 0; i < 6; i++) {
        lpfl->cord = lpfl->lpplord->iordMac = 1;
        rgptPlan[idTarget].x = lpfl->pt.x + rgDistance[i];
        rgptPlan[idTarget].y = lpfl->pt.y + 1;
        CheckValue("freighter destination", (uint16_t)IdTargetFreighter(lpfl, lppl));
        TEST_CHECK(lpfl->cord == 2);
        CheckValue("destination warp", lpfl->lpplord->rgord[1].iWarp);
        TEST_CHECK(lpfl->lpplord->rgord[1].id == idTarget);
    }
    EndValues();
}

static void test_defense_unavailable(void) {
    PLANET *lppl;
    float pct, pctSmart;
    PART part;

    lppl = LoadHomeworld("floating-no-defense");
    SetRaceStat(&rgplr[0], rsMajorAdv, raMacintosh);
    lppl->cDefenses = 100;
    TEST_ASSERT(!FGetBestDefensePart(&part));
    CalcPctSurvive(lppl, &pct, &pctSmart);
    TEST_CHECK(pct == 1.0f && pctSmart == 1.0f);
    TEST_CHECK(idPlayer == 0);
}

static void test_gate_distance(void) {
    static const int16_t rgDistance[] = {249, 250, 251, 499, 500, 501};
    PLANET *lppl;
    PLANET *lpplDst;
    FLEET *lpfl;
    ORDER *lpord;
    int i;
    int j;
    int16_t id;
    int16_t md;

    lppl = LoadHomeworld("floating-gates");
    lpplDst = &lpPlanets[(lppl->id + 1) % cPlanet];
    lpplDst->iPlayer = 0;
    lpplDst->isb = lppl->isb;
    lpplDst->fStarbase = lppl->fStarbase = TRUE;
    rglpshdefSB[0][lppl->isb].hul.chs = 1;
    rglpshdefSB[0][lppl->isb].hul.rghs[0].grhst = hstSpecialSB;
    rglpshdefSB[0][lppl->isb].hul.rghs[0].iItem = ispecialSBStargate100250;
    rglpshdefSB[0][lppl->isb].hul.rghs[0].cItem = 1;
    rglpshdef[0][0].hul.wtEmpty = 99;
    BeginValues("floating-gates");
    for (i = 0; i < 6; i++) {
        rgptPlan[lpplDst->id].x = rgptPlan[lppl->id].x + rgDistance[i];
        rgptPlan[lpplDst->id].y = rgptPlan[lppl->id].y + 1;
        lpfl = LpflStarsTestAddFleet(0, lppl->id, 0, 10);
        id = lpfl->id;
        for (j = 99; j <= 101; j++) {
            rglpshdef[0][0].hul.wtEmpty = j;
            md = FCanFleetUseStargates(lpfl, lpfl->pt, rgptPlan[lpplDst->id]);
            TEST_CHECK(md == 1 || md == 3);
            CheckValue("gate eligibility", md);
        }
        lpfl->cord = lpfl->lpplord->iordMac = 2;
        lpord = &lpfl->lpplord->rgord[1];
        memset(lpord, 0, sizeof(*lpord));
        lpord->grobj = grobjPlanet;
        lpord->id = lpplDst->id;
        lpord->pt = rgptPlan[lpplDst->id];
        lpord->iWarp = 11;
        lpord->fValidTask = TRUE;
        MoveFleets();
        lpfl = LpflFromId(id);
        TEST_ASSERT(lpfl != NULL);
        TEST_CHECK(lpfl->pt.x == rgptPlan[lpplDst->id].x && lpfl->pt.y == rgptPlan[lpplDst->id].y);
        CheckValue("gate ships", lpfl->rgcsh[0]);
        CheckValue("gate damage", lpfl->rgdv[0].dp);
    }
    EndValues();
}

static void test_beam_overflow_damage(void) {
    char szDir[MAX_PATH];
    const char *rgszAi[] = {"#1 4"};
    PLANET *lppl;
    FLEET *lpfl;
    TOK rgtok[3];
    uint16_t rgLosses[256];
    uint8_t rgbBattle[4096];
    BTLREC btlrec;
    PART part;
    int i;

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir("floating-beam", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(szDir, 12345, rgszAi, 1));
    TEST_ASSERT(FStarsTestLoadHost());
    lppl = LpplStarsTestHomeworld(0);
    memset(rgtok, 0, sizeof(rgtok));
    memset(rgLosses, 0, sizeof(rgLosses));
    memset(&btlrec, 0, sizeof(btlrec));
    vrgtok = rgtok;
    vctok = 3;
    vrgPlrLosses = rgLosses;
    lpbBattleCur = rgbBattle;
    for (i = 0; i < 3; i++) {
        rglpshdef[i != 0][i].hul = LphuldefFromId(ihuldefScout)->hul;
        rglpshdef[i != 0][i].hul.dp = 301;
        rglpshdef[i != 0][i].hul.resCost = 100;
        lpfl = LpflStarsTestAddFleet(i != 0, lppl->id, i, i == 0 ? 400 : i == 1 ? 10 : 1000);
        rgtok[i].id = lpfl->id;
        rgtok[i].iplr = i != 0;
        rgtok[i].grobj = grobjFleet;
        rgtok[i].ishdef = i;
        rgtok[i].csh = lpfl->rgcsh[i];
        rgtok[i].fActive = TRUE;
        rgtok[i].pctBeamDef = 73;
        rgtok[i].brc = i == 0 ? 0x11 : 0x12;
        rgtok[i].mdTarget1 = rgtok[i].mdTarget2 = mdTargetAny;
    }
    rglpshdef[1][1].hul.resCost = 1000;
    rglpshdef[0][0].hul.chs = 1;
    rglpshdef[0][0].hul.rghs[0].grhst = hstBeam;
    rglpshdef[0][0].hul.rghs[0].iItem = ibeamLaser;
    rglpshdef[0][0].hul.rghs[0].cItem = 20;
    part.hs = rglpshdef[0][0].hul.rghs[0];
    FLookupPart(&part);
    TEST_ASSERT((int32_t)part.pbeam->dp * 20 * 400 >= 65536);
    BeginValues("floating-beam");
    TEST_CHECK(FAttack(0, part.pbeam->init, &btlrec, 2));
    TEST_CHECK(btlrec.ctok == 2);
    TEST_CHECK(rgtok[1].csh == 0 && rgtok[2].csh > 0);
    CheckValue("first attacked", btlrec.itokAttack);
    CheckValue("targets hit", btlrec.ctok);
    for (i = 1; i < 3; i++) {
        CheckValue("remaining ships", rgtok[i].csh);
        CheckValue("remaining damage", rgtok[i].dv.dp);
        CheckValue("losses", rgLosses[16 + i]);
    }
    EndValues();
    vrgtok = NULL;
    vrgPlrLosses = NULL;
    lpbBattleCur = NULL;
}

#ifdef STARS_TEST_RANDOM_WRAP
static void test_bombing_boundaries(void) {
    static const int16_t rgBomb[] = {ibombRetroBomb, ibombLBU17Bomb, ibombCherryBomb, ibombSmartBomb};
    char szDir[MAX_PATH];
    const char *rgszAi[] = {"#1 4"};
    PLANET *lppl;
    PLANET pl;
    FLEET *lpfl;
    int i, j, k;

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir("floating-bombing", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(szDir, 12345, rgszAi, 1));
    TEST_ASSERT(FStarsTestLoadHost());
    lppl = LpplStarsTestHomeworld(1);
    TEST_ASSERT(lppl != NULL);
    pl = *lppl;
    for (i = 0; i < 6; i++)
        rgplr[1].rgTech[i] = 26;
    rglpbtlplan[0][0].iplrAttack = iplrAttackEveryone;
    rglpshdef[0][0].hul = LphuldefFromId(ihuldefMiniBomber)->hul;
    rglpshdef[0][0].hul.chs = 1;
    rglpshdef[0][0].hul.rghs[0].grhst = hstBomb;
    rglpshdef[0][0].hul.rghs[0].cItem = 1;
    lpfl = LpflStarsTestAddFleet(0, lppl->id, 0, 1);
    lpfl->lpflNext = lpfl;
    BeginValues("floating-bombing");
    for (i = 0; i < 4; i++) {
        rglpshdef[0][0].hul.rghs[0].iItem = rgBomb[i];
        for (j = 0; j < 3; j++) {
            *lppl = pl;
            lppl->fStarbase = FALSE;
            lppl->cFactories = lppl->cMines = 101;
            lppl->rgwtMin[3] = 10001;
            lppl->cDefenses = j * 50;
            for (k = 0; k < 3; k++) {
                lppl->rgEnvVarOrig[k] = 50;
                lppl->rgEnvVar[k] = 40 + k * 10;
            }
            lpfl->fBombed = FALSE;
#ifdef STARS_TEST_RANDOM_WRAP
            fRecordMessages = TRUE;
            cMessages = 0;
#endif
            DoBombing();
#ifdef STARS_TEST_RANDOM_WRAP
            fRecordMessages = FALSE;
            TEST_CHECK(cMessages >= 2);
#endif
            CheckValue("bomb population", lppl->rgwtMin[3]);
            CheckValue("bomb factories", lppl->cFactories);
            CheckValue("bomb mines", lppl->cMines);
            CheckValue("bomb defenses", lppl->cDefenses);
            for (k = 0; k < 3; k++)
                CheckValue("bomb environment", lppl->rgEnvVar[k]);
            TEST_CHECK(lpfl->fBombed);
            TEST_CHECK(lppl->rgwtMin[3] <= 10001);
            if (i == 1 && j > 0)
                TEST_CHECK(lppl->rgwtMin[3] == 10001);
        }
    }
    EndValues();
}
#endif

static void test_large_circle_projection(void) {
    static const POINT16 rgpt[] = {{300, 400}, {301, 400}, {300, 409}, {300, 410}, {300, 411}, {600, 800}, {610, 800}};
    POINT16              ptFrom = {0, 0};
    POINT16              ptTo = {600, 800};
    int16_t              dStart;
    int16_t              dEnd;
    int16_t              fHit;
    int                  i;

    BeginValues("floating-geometry");
    for (i = 0; i < (int)(sizeof(rgpt) / sizeof(rgpt[0])); i++) {
        dStart = dEnd = -1;
        fHit = FIntersectCircleLine(ptFrom, ptTo, rgpt[i], 100, 1000, &dStart, &dEnd);
        CheckValue("forward hit", fHit);
        CheckValue("forward start", (uint16_t)dStart);
        CheckValue("forward end", (uint16_t)dEnd);
        dStart = dEnd = -1;
        fHit = FIntersectCircleLine(ptTo, ptFrom, rgpt[i], 100, 1000, &dStart, &dEnd);
        CheckValue("reverse hit", fHit);
        CheckValue("reverse start", (uint16_t)dStart);
        CheckValue("reverse end", (uint16_t)dEnd);
    }
    EndValues();
}

static void test_large_cargo_balance(void) {
    PLANET *lppl;
    FLEET  *lpflSrc;
    FLEET  *lpflDst;
    FLEET   flSrc;
    FLEET   flDst;
    int     i;
    int     j;

    lppl = LoadHomeworld("floating-cargo");
    rglpshdef[0][0].hul = LphuldefFromId(ihuldefLargeFreighter)->hul;
    TEST_ASSERT(WtMaxShdefStat(&rglpshdef[0][0], 2) > 0);
    lpflSrc = LpflStarsTestAddFleet(0, lppl->id, 0, 300);
    lpflDst = LpflStarsTestAddFleet(0, lppl->id, 0, 10);
    BeginValues("floating-cargo");
    for (i = 0; i < 3; i++) {
        lpflSrc->rgwtMin[0] = 44999 + i;
        lpflSrc->rgwtMin[1] = 137;
        lpflSrc->rgwtMin[2] = 277;
        lpflSrc->rgwtMin[3] = 337;
        lpflSrc->rgwtMin[4] = 164999 + i;
        flSrc = *lpflSrc;
        flDst = *lpflDst;
        flSrc.rgcsh[0] -= 129;
        flDst.rgcsh[0] += 129;
        FleetTransferCargoBalance(&flSrc, &flDst);
        TEST_CHECK(flDst.rgwtMin[0] > lpflDst->rgwtMin[0]);
        for (j = 0; j < 5; j++) {
            CheckValue("source cargo", (uint32_t)flSrc.rgwtMin[j]);
            CheckValue("destination cargo", (uint32_t)flDst.rgwtMin[j]);
            TEST_CHECK(flSrc.rgwtMin[j] + flDst.rgwtMin[j] == lpflSrc->rgwtMin[j] + lpflDst->rgwtMin[j]);
        }
    }
    EndValues();
}

static void test_route_and_distance_display(void) {
    static const int16_t rgxy[][2] = {{0, 0}, {1, 0}, {3, 4}, {7, 1}, {80, 1}, {81, 1}, {100, 50}};
    PLANET              *lppl;
    FLEET               *lpfl;
    SHDEF                shdef;
    int16_t              idRoute;
    int16_t              dPlan;
    char                 sz[80];
    int                  i;

    lppl = LoadHomeworld("floating-route");
    idRoute = (lppl->id + 1) % cPlanet;
    lppl->idRoute = idRoute + 1;
    lppl->fStarbase = FALSE;
    lpPlanets[idRoute].iPlayer = 0;
    lpfl = LpflStarsTestAddFleet(0, lppl->id, 0, 1);
    lpfl->pt.x = lpfl->pt.y = 1200;
    lpfl->lpplord->rgord[0].pt = lpfl->pt;
    lpfl->rgwtMin[4] = 1000;
    BeginValues("floating-route");
    for (i = 0; i < (int)(sizeof(rgxy) / sizeof(rgxy[0])); i++) {
        rgptPlan[idRoute].x = 1200 + rgxy[i][0];
        rgptPlan[idRoute].y = 1200 + rgxy[i][1];
        AutoRouteFleet(lpfl, lppl);
        CheckValue("automatic route warp", lpfl->lpplord->rgord[1].iWarp);
        CchGetETA(NULL, lpfl, sz, 1, FALSE);
        CheckText("ETA", sz);
        dyArial8 = 14;
        CheckText("distance long", PszGetDistance(0, 0, rgxy[i][0], rgxy[i][1]));
        dyArial8 = 15;
        CheckText("distance short", PszGetDistance(0, 0, rgxy[i][0], rgxy[i][1]));
    }
    SetRaceStat(&rgplr[0], rsMajorAdv, raNone);
    game.fTutorial = TRUE;
    memset(&shdef, 0, sizeof(shdef));
    shdef.hul.ihuldef = ihuldefScout;
    CheckValue("tutorial scanner", (uint16_t)GetShdefScannerRange(&shdef, 0, &dPlan, NULL, NULL));
    CheckValue("tutorial penetrating", (uint16_t)dPlan);
    EndValues();
}

#ifdef _WIN32
static int16_t fRecordText;
static int16_t fRecordUI;
static int16_t fRecordClip;
static int     cText;
static int     cEllipse;
DWORD          __real_GetTextExtent(HDC hdc, LPCSTR psz, int cch);
extern BOOL(WINAPI *__real___imp_TextOutA)(HDC, int, int, LPCSTR, int);

DWORD __wrap_GetTextExtent(HDC hdc, LPCSTR psz, int cch) {
    if (fRecordText || fRecordUI)
        return MAKELONG(40, 12);
    return __real_GetTextExtent(hdc, psz, cch);
}

static BOOL WINAPI RecordTextOut(HDC hdc, int x, int y, LPCSTR psz, int cch) {
    LOGFONT lf;
    char sz[128];

    if (fRecordUI) {
        snprintf(sz, sizeof(sz), "%.*s", cch, psz);
        CheckText("UI text", sz);
        CheckValue("UI text x", (uint16_t)x);
        CheckValue("UI text y", (uint16_t)y);
        cText++;
        return TRUE;
    }

    if (!fRecordText)
        return __real___imp_TextOutA(hdc, x, y, psz, cch);
    TEST_ASSERT(GetObject(GetCurrentObject(hdc, OBJ_FONT), sizeof(lf), &lf) == sizeof(lf));
    CheckValue("diagonal x", (uint16_t)x);
    CheckValue("diagonal y", (uint16_t)y);
    CheckValue("diagonal rotation", (uint16_t)lf.lfEscapement);
    cText++;
    return TRUE;
}

BOOL(WINAPI *__wrap___imp_TextOutA)(HDC, int, int, LPCSTR, int) = RecordTextOut;

extern BOOL(WINAPI *__real___imp_Ellipse)(HDC, int, int, int, int);
static BOOL WINAPI RecordEllipse(HDC hdc, int x1, int y1, int x2, int y2) {
    if (fRecordUI) {
        CheckValue("ellipse left", (uint16_t)x1);
        CheckValue("ellipse top", (uint16_t)y1);
        CheckValue("ellipse right", (uint16_t)x2);
        CheckValue("ellipse bottom", (uint16_t)y2);
        cEllipse++;
        return TRUE;
    }
    return __real___imp_Ellipse(hdc, x1, y1, x2, y2);
}
BOOL(WINAPI *__wrap___imp_Ellipse)(HDC, int, int, int, int) = RecordEllipse;

extern BOOL(WINAPI *__real___imp_InflateRect)(LPRECT, int, int);
static BOOL WINAPI RecordInflateRect(LPRECT prc, int dx, int dy) {
    BOOL fResult;

    fResult = __real___imp_InflateRect(prc, dx, dy);
    if (fRecordClip) {
        CheckValue("redraw left", (uint16_t)prc->left);
        CheckValue("redraw top", (uint16_t)prc->top);
        CheckValue("redraw right", (uint16_t)prc->right);
        CheckValue("redraw bottom", (uint16_t)prc->bottom);
    }
    return fResult;
}
BOOL(WINAPI *__wrap___imp_InflateRect)(LPRECT, int, int) = RecordInflateRect;

static void test_UI_mines_and_victory(void) {
    static const int32_t rgMines[] = {99, 100, 101, 9999, 10000, 10001};
    THING *lpth;
    SCOREX rgScore[16];
    HDC hdc;
    RECT rc;
    SCAN scanOld;
    int i;

    LoadHomeworld("floating-ui-mines");
    gd.fGeneratingTurn = gd.fNoScannerDraw = FALSE;
    hwndScanner = CreateWindowA("STATIC", "numerical test", WS_POPUP, 0, 0, 640, 480, NULL, NULL, hInst, NULL);
    hwndMine = CreateWindowA("STATIC", "numerical test", WS_POPUP, 0, 0, 640, 480, NULL, NULL, hInst, NULL);
    TEST_ASSERT(hwndScanner != NULL && hwndMine != NULL);
    hdc = GetDC(hwndScanner);
    TEST_ASSERT(hdc != NULL);
    memset(rgScore, 0, sizeof(rgScore));
    vlprgScoreX = rgScore;
    vdxScoreX = 200;
    dyArial8 = 12;
    BeginValues("floating-ui-mines");
    fRecordUI = TRUE;
    cText = 0;
    DrawVCReport(hdc);
    TEST_CHECK(cText > 0);
    lpth = LpthNew(0, ithMinefield);
    lpth->pt.x = lpth->pt.y = 1200;
    lpth->thm.iType = 0;
    memset(&sel, 0, sizeof(sel));
    sel.scan.grobj = grobjThing;
    sel.scan.ith = lpth - lpThings;
    sel.scan.pt = lpth->pt;
    scanOld = sel.scan;
    grbitScan = grbitScanMineFields;
    grbitScanMines = 0;
    iScanZoom = zoom100;
    xScanTop = 1000;
    yScanTop = dGalInv - 1400;
    for (i = 0; i < 6; i++) {
        lpth->thm.cMines = rgMines[i];
        SetRect(&rc, 0, 0, 640, 480);
        DrawMineSurvey(hdc, &rc);
        SetRect(&rc, 0, 0, 640, 480);
        cEllipse = 0;
        DrawScanner(hdc, &rc);
        TEST_CHECK(cEllipse > 0);
        fRecordClip = TRUE;
        ShowScanSelChange(&scanOld, &sel.scan, FALSE);
        fRecordClip = FALSE;
    }
    fRecordUI = FALSE;
    EndValues();
    vlprgScoreX = NULL;
    ReleaseDC(hwndScanner, hdc);
    DestroyWindow(hwndScanner);
    DestroyWindow(hwndMine);
    hwndScanner = hwndMine = NULL;
}

static void test_UI_text_and_sort(void) {
    static const RECT rgrc[] = {{0, 0, 100, 100}, {10, 20, 210, 100}, {0, 0, 80, 240}, {-40, -30, 120, 70}};
    PLANET           *lppl;
    RPT               rpt;
    HDC               hdc;
    RECT              rc;
    uint16_t          id1;
    uint16_t          id2;
    int               i;
    int               j;
    int               cmp;

    lppl = LoadHomeworld("floating-ui-text");
    BeginValues("floating-ui-text");
    hdc = CreateCompatibleDC(NULL);
    TEST_ASSERT(hdc != NULL);
    fRecordText = TRUE;
    cText = 0;
    for (i = 0; i < (int)(sizeof(rgrc) / sizeof(rgrc[0])); i++) {
        rc = rgrc[i];
        DiaganolTextOut(hdc, &rc, "Stars", 5);
    }
    fRecordText = FALSE;
    TEST_CHECK(cText == 4);
    DeleteDC(hdc);
    id1 = lppl->id;
    id2 = (id1 + 1) % cPlanet;
    lpPlanets[id2] = *lppl;
    lpPlanets[id2].id = id2;
    memset(&rpt, 0, sizeof(rpt));
    rpt.irpt = rptPlanets;
    rpt.icolSort = colPlanetDefense;
    vprptCur = &rpt;
    vicolSortPrev = -1;
    for (i = 0; i < 6; i++)
        rgplr[0].rgTech[i] = 26;
    for (i = 0; i <= 100; i += 25) {
        lppl->cDefenses = i;
        lppl->rgwtMin[3] = lpPlanets[id2].rgwtMin[3] = 10000;
        for (j = 0; j < 2; j++) {
            lpPlanets[id2].cDefenses = i + j;
            rpt.fAscending = TRUE;
            cmp = ICompReport(&id1, &id2);
            CheckValue("defense ascending", (uint16_t)cmp);
            rpt.fAscending = FALSE;
            CheckValue("defense descending", (uint16_t)ICompReport(&id1, &id2));
        }
    }
    EndValues();
}

static void test_UI_numeric_results(void) {
    static const int16_t rgxy[][2] = {{0, 0}, {1, 0}, {0, 1}, {1, 1}, {1000, 414}, {1000, 415}, {414, 1000}, {415, 1000}};
    POINT16              pt;
    POINT16              ptD;
    DRAWCIR              dc;
    RECT                 rc;
    int16_t              rgx[4];
    int16_t              rgy[4];
    int16_t              rgrad[4];
    int                  i;
    int                  j;
    int                  sx;
    int                  sy;
    int                  z;

    BeginValues("floating-ui");
    for (z = zoom50; z <= zoom100; z++) {
        iScanZoom = z;
        for (i = 0; i < (int)(sizeof(rgxy) / sizeof(rgxy[0])); i++) {
            for (sx = -1; sx <= 1; sx += 2) {
                for (sy = -1; sy <= 1; sy += 2) {
                    GetDxDyOrientation(sx * rgxy[i][0], sy * rgxy[i][1], &pt, &ptD);
                    CheckValue("orientation x", (uint16_t)pt.x);
                    CheckValue("orientation y", (uint16_t)pt.y);
                    CheckValue("sprite width", (uint16_t)ptD.x);
                    CheckValue("sprite height", (uint16_t)ptD.y);
                }
            }
        }
    }
    // Tangency and one pixel on either side, in both insertion orders.
    // All circles stay inside the clip so this checks batching without GDI.
    for (i = 9; i <= 11; i++) {
        for (j = 0; j < 2; j++) {
            memset(&dc, 0, sizeof(dc));
            memset(rgrad, 0, sizeof(rgrad));
            dc.rgx = rgx;
            dc.rgy = rgy;
            dc.rgrad = rgrad;
            dc.cMax = 4;
            SetRect(&dc.rcClip, -100, -100, 100, 100);
            SetRect(&rc, -20, -20, 20, 20);
            if (j != 0)
                SetRect(&rc, i - 10, -10, i + 10, 10);
            DrawRadarCircle(&dc, &rc);
            SetRect(&rc, i - 10, -10, i + 10, 10);
            if (j != 0)
                SetRect(&rc, -20, -20, 20, 20);
            DrawRadarCircle(&dc, &rc);
            CheckValue("circle count", dc.cCur);
            for (z = 0; z < dc.cCur; z++) {
                CheckValue("circle radius", (uint16_t)rgrad[z]);
                CheckValue("circle x", (uint16_t)rgx[z]);
                CheckValue("circle y", (uint16_t)rgy[z]);
            }
        }
    }
    EndValues();
}
#endif

TEST_LIST = {{"distance result bits", test_distance_bits},
             {"freighter destination thresholds", test_freighter_destination},
             {"unavailable defense fallback", test_defense_unavailable},
             {"gate distance and movement", test_gate_distance},
#ifdef STARS_TEST_RANDOM_WRAP
             {"bombing boundaries", test_bombing_boundaries},
#endif
             {"large beam damage carryover", test_beam_overflow_damage},
             {"defense power result bits", test_defense_bits},
             {"planet and race rounding", test_planet_and_race_rounding},
             {"scanner and cloak rounding", test_scanner_and_cloak_rounding},
#ifdef STARS_TEST_RANDOM_WRAP
             {"AI random bound rounding", test_AI_random_rounding},
#endif
             {"large circle projection", test_large_circle_projection},
             {"large cargo balance", test_large_cargo_balance},
             {"route and distance display", test_route_and_distance_display},
#ifdef _WIN32
             {"UI minefield and victory geometry", test_UI_mines_and_victory},
             {"UI orientation and radar boundaries", test_UI_numeric_results},
             {"UI diagonal text and defense sorting", test_UI_text_and_sort},
#endif
             {NULL, NULL}};
