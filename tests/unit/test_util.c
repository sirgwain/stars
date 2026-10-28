#include "acutest.h"

#include "stars_test.h"

// SpaceDockWithArmor returns a Space Dock design with cArmor Superlatanium
// (1500 dp each) in its shield/armor slot.
static SHDEF SpaceDockWithArmor(int16_t cArmor) {
    SHDEF shdef;

    memset(&shdef, 0, sizeof(shdef));
    shdef.hul = LphuldefFromId(ihuldefSpaceDock)->hul;
    shdef.det = detAll;
    shdef.hul.rghs[2].grhst = hstArmor;
    shdef.hul.rghs[2].iItem = iarmorSuperlatanium;
    shdef.hul.rghs[2].cItem = cArmor;
    return shdef;
}

// LoadPlayer loads player 1's turn file of a new tiny game, with or without
// Regenerating Shields.
static void LoadPlayer(const char *pszTest, int16_t fRS) {
    char szDir[MAX_PATH];

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir(pszTest, szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(szDir, 12345, NULL, 0));
    TEST_ASSERT(FStarsTestLoadPlayer(0));
    SetRaceGrbit(&rgplr[0], ibitRaceRegeneratingShields, fRS);
}

static void test_UpdateShdefCost_armor(void) {
    SHDEF shdef;

    LoadPlayer("UpdateShdefCost_armor", FALSE);
    shdef = SpaceDockWithArmor(20);
    UpdateShdefCost(&shdef);
    TEST_CHECK_(shdef.hul.dp == 250 + 30000, "dp %u", shdef.hul.dp);
    shdef = SpaceDockWithArmor(24);
    UpdateShdefCost(&shdef);
    TEST_CHECK_(shdef.hul.dp == 250 + 36000, "dp %u", shdef.hul.dp);
}

// Space Dock armor overflow: with RS the armor slot's strength was halved
// after it had overflowed signed 16 bits, so 24 Superlatanium (36000 dp)
// gave the hull about 50000 dp instead of 18250.
static void test_UpdateShdefCost_rs_armor_overflow(void) {
    SHDEF shdef;

    LoadPlayer("UpdateShdefCost_rs_armor_overflow", TRUE);
    shdef = SpaceDockWithArmor(20);
    UpdateShdefCost(&shdef);
    TEST_CHECK_(shdef.hul.dp == 250 + 15000, "dp %u", shdef.hul.dp);
    shdef = SpaceDockWithArmor(24);
    UpdateShdefCost(&shdef);
    TEST_CHECK_(shdef.hul.dp == 250 + 18000, "dp %u", shdef.hul.dp);
}

static void test_PszPlayerName_all_spaces(void) {
    PLAYER plr;

    TEST_ASSERT(FStarsTestInit());
    memset(&plr, 0, sizeof(plr));
    strcpy(plr.szName, "   ");
    TEST_CHECK(strcmp(PszPlayerName(0, FALSE, FALSE, FALSE, 0, &plr), "No Name") == 0);
}

// AddPursuer adds a player-0 fleet in deep space at pt whose second waypoint
// pursues fleet idTarget, last seen at ptTarget.
static FLEET *AddPursuer(int16_t ish, POINT16 pt, int16_t idTarget, POINT16 ptTarget) {
    FLEET *lpfl;
    ORDER *lpord;

    lpfl = LpflStarsTestAddFleet(0, idPlanetDeepSpace, ish, 1);
    lpfl->pt = pt;
    lpfl->lpplord->rgord[0].pt = pt;
    lpfl->lpplord->iordMac = 2;
    lpfl->cord = 2;
    lpord = &lpfl->lpplord->rgord[1];
    memset(lpord, 0, sizeof(*lpord));
    lpord->pt = ptTarget;
    lpord->id = idTarget;
    lpord->grobj = grobjFleet;
    lpord->iWarp = 5;
    return lpfl;
}

// When a pursued fleet is gone, ValidateWaypoints retargets each pursuer at a
// fleet where it was last seen. 2.6j RC3 added a first pass that skipped
// fleets another pursuer had already taken, spreading pursuers over the
// pieces of a split fleet; the jrc4 binary (0008:497a) dropped it again, so
// every pursuer takes the heaviest matching fleet.
static void test_ValidateWaypoints_pursuers_take_heaviest(void) {
    char    szDir[MAX_PATH];
    const char *rgszAi[] = {"#1 4"};
    int16_t ish0;
    int16_t ish1;
    POINT16 ptFled;
    POINT16 ptStart;
    FLEET  *lpflHeavy;
    FLEET  *lpflLight;
    FLEET  *lpflA;
    FLEET   *lpflB;
    MdTarget mdTarget;
    int16_t  i;

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir("ValidateWaypoints_pursuers", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(szDir, 12345, rgszAi, 1));
    TEST_ASSERT(FStarsTestLoadHost());
    for (ish0 = 0; ish0 < 16 && rglpshdef[0][ish0].fFree; ish0++) {
    }
    for (ish1 = 0; ish1 < 16 && rglpshdef[1][ish1].fFree; ish1++) {
    }
    TEST_ASSERT(ish0 < 16 && ish1 < 16);

    ptFled.x = 1200;
    ptFled.y = 1200;
    ptStart.x = 1100;
    ptStart.y = 1100;
    lpflHeavy = LpflStarsTestAddFleet(1, idPlanetDeepSpace, ish1, 5);
    lpflLight = LpflStarsTestAddFleet(1, idPlanetDeepSpace, ish1, 1);
    lpflHeavy->pt = lpflLight->pt = ptFled;
    // Target the AI design's category ("Any" never matches exactly).
    mdTarget = FMatchTarget(lpflHeavy, mdTargetArmedShips, TRUE) ? mdTargetArmedShips : mdTargetUnarmedShips;
    TEST_ASSERT(FMatchTarget(lpflHeavy, mdTarget, TRUE) && FMatchTarget(lpflLight, mdTarget, TRUE));
    for (i = 0; i < rgcbtlplan[0]; i++)
        rglpbtlplan[0][i].mdTarget1 = mdTarget;
    lpflA = AddPursuer(ish0, ptStart, (1 << 9) | 400, ptFled);
    lpflB = AddPursuer(ish0, ptStart, (1 << 9) | 401, ptFled);

    ValidateWaypoints();
    TEST_CHECK_(lpflA->lpplord->rgord[1].id == lpflHeavy->id, "first pursuer took %#x, heavy %#x", lpflA->lpplord->rgord[1].id, lpflHeavy->id);
    TEST_CHECK_(lpflB->lpplord->rgord[1].id == lpflHeavy->id, "second pursuer took %#x, heavy %#x light %#x", lpflB->lpplord->rgord[1].id, lpflHeavy->id,
                lpflLight->id);
}

TEST_LIST = {{"ValidateWaypoints pursuers take the heaviest fleet", test_ValidateWaypoints_pursuers_take_heaviest},
             {"UpdateShdefCost armor", test_UpdateShdefCost_armor},
             {"UpdateShdefCost RS armor overflow", test_UpdateShdefCost_rs_armor_overflow},
             {"PszPlayerName all spaces", test_PszPlayerName_all_spaces},
             {NULL, NULL}};
