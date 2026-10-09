#include "win.h"

BTLDATA       *vlpbdVCR = 0;
BTLDATA       *vlpbdVCRNext = 0;
BTLPLAN        btlplan = {0};
BTLREC        *vlpbrVCR = 0;
BTN           *rgbtnXfer = 0;
COLORREF       crButtonFace = 0;
COLORREF       crButtonHilite = 0;
COLORREF       crButtonShadow = 0;
COLORREF       crButtonText = 0;
COLORREF       crWindow = 0;
COLORREF       crWindowText = 0;
ControlId      rgidRaceBtn[5] = {IDCANCEL, IDC_BACK, IDC_NEXT, IDC_FINISH, IDC_HELP};
FARPROC        lpfnBrowserDlgProc = 0;
FARPROC        lpfnGaugeDlgProc = 0;
FARPROC        lpfnTutorDlgProc = 0;
FRAMESTUFF     vfs = {0};
GrbitScan      grbitScan = 0;
HACCEL         hAccel = 0;
HACCEL         hAccelTitle = 0;
HBITMAP        hbmpBackBld = 0;
HBITMAP        hbmpMono = 0;
HBITMAP        hbmpMsg = 0;
HBITMAP        hbmpNumbers = 0;
HBITMAP        hbmpScanShip = 0;
HBITMAP        hbmpScanner = 0;
HBITMAP        hbmpUnknownPlanet = 0;
HBRUSH         hbr50Screen = 0;
HBRUSH         hbrBBlue = 0;
HBRUSH         hbrBlue = 0;
HBRUSH         hbrButtonFace = 0;
HBRUSH         hbrButtonHilite = 0;
HBRUSH         hbrButtonShadow = 0;
HBRUSH         hbrButtonText = 0;
HBRUSH         hbrCargo = 0;
HBRUSH         hbrDesktop = 0;
HBRUSH         hbrDkYellow = 0;
HBRUSH         hbrDock = 0;
HBRUSH         hbrEnemy = 0;
HBRUSH         hbrGray = 0;
HBRUSH         hbrGreen = 0;
HBRUSH         hbrLightGray = 0;
HBRUSH         hbrPurple = 0;
HBRUSH         hbrRadar = 0;
HBRUSH         hbrRadarNear = 0;
HBRUSH         hbrRed = 0;
HBRUSH         hbrSelect = 0;
HBRUSH         hbrShip = 0;
HBRUSH         hbrStarbase = 0;
HBRUSH         hbrTooltip = 0;
HBRUSH         hbrWindow = 0;
HBRUSH         hbrWindowFrame = 0;
HBRUSH         hbrWindowText = 0;
HBRUSH         hbrYellow = 0;
HBRUSH         rghbrCache[32] = {0};
HBRUSH         rghbrMinSum[4][2] = {0};
HBRUSH         rghbrMineral[5] = {0};
HBRUSH         rghbrPat[3] = {0};
HBRUSH         rghbrPlanetAttr[3][2] = {0};
HCURSOR        hcurArrowHelp = 0;
HCURSOR        hcurCloseGrab = 0;
HCURSOR        hcurHand = 0;
HCURSOR        hcurNoWay = 0;
HCURSOR        hcurOpenGrab = 0;
HCURSOR        hcurResize4Way = 0;
HCURSOR        hcurResizeNS = 0;
HCURSOR        hcurResizeWE = 0;
HCURSOR        hcurScanAdd = 0;
HCURSOR        hcurScanner = 0;
HCURSOR        hcurTrashCan = 0;
HFONT          rghfontArial10[2] = {0};
HFONT          rghfontArial6[1] = {0};
HFONT          rghfontArial7[1] = {0};
HFONT          rghfontArial8[5] = {0};
HGLOBAL        hdibPlanets = 0;
HGLOBAL        hdibPlaque = 0;
HGLOBAL        hdibRaces = 0;
HGLOBAL        hdibRacesT = 0;
HGLOBAL        hdibRacesX = 0;
HGLOBAL        hdibThings = 0;
HGLOBAL        hdibToolbar = 0;
HGLOBAL        rghdibInventory[7] = {0};
HGLOBAL        rghdibShipsT[5] = {0};
HGLOBAL        rghdibShips[5] = {0};
HGLOBAL        vhdibTitle = 0;
HICON          hiconHost = 0;
HICON          hiconStars = 0;
HICON          hiconWait = 0;
HICON          rghiconVCR[7] = {0};
HINSTANCE      hInst = 0;
HPALETTE       vhpal = 0;
HPALETTE       vhpalSplash = 0;
HPEN           hpenDkBlue = 0;
HPEN           hpenDkGreen = 0;
HPEN           hpenDkPurple = 0;
HPEN           hpenDkYellow = 0;
HPEN           hpenEnemy = 0;
HPEN           hpenMassPath = 0;
HPEN           hpenRadar = 0;
HPEN           hpenRadarNear = 0;
HPEN           hpenShip = 0;
HPEN           hpenStarbase = 0;
HPEN           hpenYellow = 0;
HRGN           hrgnHuge = 0;
HRGN           hrgnScratch = 0;
HS             rghsFutureTech[8] = {0};
HWND           hwndActive = 0;
HWND           hwndBattleDD = 0;
HWND           hwndBrowser = 0;
HWND           hwndBrowserChild = 0;
HWND           hwndFleetCompLB = 0;
HWND           hwndFrame = 0;
HWND           hwndMDIClient = 0;
HWND           hwndMain = 0;
HWND           hwndMessage = 0;
HWND           hwndMine = 0;
HWND           hwndMineCB = 0;
HWND           hwndMsgDrop = 0;
HWND           hwndMsgEdit = 0;
HWND           hwndMsgScroll = 0;
HWND           hwndOrderED = 0;
HWND           hwndPlanet = 0;
HWND           hwndPlanetProdLB = 0;
HWND           hwndPopup = 0;
HWND           hwndProdDlg = 0;
HWND           hwndProgressGauge = 0;
HWND           hwndRaceParent = 0;
HWND           hwndRepCB = 0;
HWND           hwndReportDlg = 0;
HWND           hwndScanner = 0;
HWND           hwndScoreXDlg = 0;
HWND           hwndShipDD = 0;
HWND           hwndShipLB = 0;
HWND           hwndSlotDlg = 0;
HWND           hwndTBRadar = 0;
HWND           hwndTb = 0;
HWND           hwndTitle = 0;
HWND           hwndTooltip = 0;
HWND           hwndVCRDlg = 0;
HWND           hwndZipOrderDlg = 0;
HWND           rghwndBtnSplash[4] = {0};
HWND           rghwndBtn[13] = {0};
HWND           rghwndMsgBtn[4] = {0};
HWND           rghwndOrderDD[3] = {0};
HostTimer      uTimerType = 0;
HullSlotType   rgmapBuildBmps[21] = {hstEnabled,  hstEngine,    hstScanner, hstShield,   hstWeapon,   hstSome,        hstSpecialEM,
                                     hstScanSpec, hstBomb,      hstShArm,   hstArmor,    hstMining,   hstScanSpecArm, hstShWeap,
                                     hstMines,    hstSpecialSB, hstSomeSB,  hstSpecMine, hstSpecialE, hstSpecialM,    hstShSpec};
MdBuild        mdBuild = mdBuildShdef;
MdMsgObj       mdMsgObj = mdMsgObjNone;
MdXfer         mdXferDlg = mdXferNone;
PART           vpartBrowser = {0};
PLAYER        *vrgplrNew = 0;
POINT16        ptPlaque = {0};
POINT16        ptSpeedVCR = {0};
POINT16        ptStickyBattlePlansDlg = {.x = -1, .y = -1};
POINT16        ptStickyBrowserDlg = {.x = -1, .y = -1};
POINT16        ptStickyFindDlg = {.x = -1, .y = -1};
POINT16        ptStickyHostModeDlg = {.x = -1, .y = -1};
POINT16        ptStickyMergeFleetsDlg = {.x = -1, .y = -1};
POINT16        ptStickyNewDlg = {.x = -1, .y = -1};
POINT16        ptStickyPrintMapDlg = {.x = -1, .y = -1};
POINT16        ptStickyProduceDlg = {.x = -1, .y = -1};
POINT16        ptStickyRaceDlg = {.x = -1, .y = -1};
POINT16        ptStickyRelationsDlg = {.x = -1, .y = -1};
POINT16        ptStickyRenameDlg = {.x = -1, .y = -1};
POINT16        ptStickyResDlg = {.x = -1, .y = -1};
POINT16        ptStickyScoreXDlg = {.x = -1, .y = -1};
POINT16        ptStickySlotDlg = {.x = -1, .y = -1};
POINT16        ptStickyTransferDlg = {.x = -1, .y = -1};
POINT16        ptStickyTutorDlg = {.x = -1, .y = -1};
POINT16        ptStickyVCRDlg = {.x = -1, .y = -1};
POINT16        ptStickyZipOrderDlg = {.x = -1, .y = -1};
POINT16        ptStickyZipProdDlg = {.x = -1, .y = -1};
POINT16        ptslotGlob = {0};
POINT16        rgptArrow[5] = {{.x = 3}, {.y = 3}, {.x = -1, .y = 3}, {.x = 2, .y = 3}, {.x = -3, .y = 6}};
POINT16        rgptTriangle[3] = {{.x = 4}, {.y = 4}, {.x = -1, .y = 4}};
POINT16        vptMsg = {0};
POINT16        vptTbLast = {.x = -1, .y = -1};
POPUPDATA      GlobalPD = {0};
RECT          *vrgrcRCW = 0;
RECT           rcCargo = {0};
RECT           rcMsgText = {0};
RECT           rcMsgTitle = {0};
RECT           rcProdDiamond = {0};
RECT           rcSpinBot = {0};
RECT           rcSpinTop = {0};
RECT           rgrcBuildSpin[2] = {0};
RECT           rgrcRef[19] = {0};
RECT           vrcTooltip = {0};
RECT           vrgrcSlot[16] = {0};
RPT           *vprptCur = 0;
RaceWizardPage iPanelActive = 0;
SHDEF         *lpshdefBuild = 0;
TILE           rgtilePlanet[6] = {{
                                      .yTop = 1,
                                      .dyFull = 85,
                                      .grbit = tileBitmap,
                                      .pfn = DrawPlanShipBitmap,
                                      .fPopped = TRUE,
                                      .fMinTitle = TRUE,
                                      .idh = idhPlanetTile,
                        },
                                  {
                                      .yTop = 6,
                                      .dyFull = 5,
                                      .grbit = tileMineralsOrCargo,
                                      .pfn = DrawPlanetMinSum,
                                      .id = 1,
                                      .fPopped = TRUE,
                                      .idh = idhMineralsOnHandTile,
                        },
                                  {
                                      .yTop = 8,
                                      .dyFull = 6,
                                      .grbit = tilePlanetStats,
                                      .pfn = DrawPlanetStats,
                                      .id = 4,
                                      .fPopped = TRUE,
                                      .idh = idhStatusTile,
                        },
                                  {
                                      .yTop = 6,
                                      .dyFull = 22,
                                      .grbit = tileShipList,
                                      .pfn = DrawPlanetShipList,
                                      .iCol = 1,
                                      .id = 5,
                                      .fPopped = TRUE,
                                      .fNullPtr = TRUE,
                                      .idh = idhFleetsInOrbitTile,
                        },
                                  {
                                      .yTop = 10,
                                      .dyFull = 20,
                                      .grbit = tileProductionOrOrbit,
                                      .pfn = DrawPlanetProduction,
                                      .iCol = 1,
                                      .id = 6,
                                      .fPopped = TRUE,
                                      .idh = idhProductionTile,
                        },
                                  {
                                      .yTop = 8,
                                      .dyFull = 15,
                                      .grbit = tileStarbaseOrWaypoint,
                                      .pfn = DrawPlanetStarbase,
                                      .iCol = 1,
                                      .id = 7,
                                      .fPopped = TRUE,
                                      .fNullPtr = TRUE,
                                      .fMinTitle = TRUE,
                                      .idh = idhStarbaseTile,
                        }};
TILE           rgtileShip[7] = {{
                                    .yTop = 1,
                                    .dyFull = 85,
                                    .grbit = tileBitmap,
                                    .pfn = DrawPlanShipBitmap,
                                    .fPopped = TRUE,
                                    .fMinTitle = TRUE,
                                    .idh = idhFleetTile,
                      },
                                {
                                    .yTop = 3,
                                    .dyFull = 5,
                                    .grbit = tileProductionOrOrbit,
                                    .pfn = DrawShipPlanet,
                                    .id = 5,
                                    .fPopped = TRUE,
                                    .fMinTitle = TRUE,
                                    .idh = idhLocationTile,
                      },
                                {
                                    .yTop = 11,
                                    .dyFull = 19,
                                    .grbit = tileFleetOrders,
                                    .pfn = DrawShipOrders,
                                    .id = 3,
                                    .fPopped = TRUE,
                                    .idh = idhFleetWaypointsTile,
                      },
                                {
                                    .yTop = 6,
                                    .dyFull = 12,
                                    .grbit = tileStarbaseOrWaypoint,
                                    .pfn = DrawShipWayPtOrders,
                                    .id = 4,
                                    .fPopped = TRUE,
                                    .idh = idhWaypointTaskTile,
                      },
                                {
                                    .yTop = 7,
                                    .dyFull = 14,
                                    .grbit = tileMineralsOrCargo,
                                    .pfn = DrawShipCargo,
                                    .iCol = 1,
                                    .id = 1,
                                    .fPopped = TRUE,
                                    .idh = idhFuelAndCargoTile,
                      },
                                {
                                    .yTop = 12,
                                    .dyFull = 16,
                                    .grbit = tileFleetComp,
                                    .pfn = DrawFleetComp,
                                    .iCol = 1,
                                    .id = 9,
                                    .fPopped = TRUE,
                                    .idh = idhFleetCompositionTile,
                      },
                                {
                                    .yTop = 6,
                                    .dyFull = 22,
                                    .grbit = tileShipList,
                                    .pfn = DrawPlanetShipList,
                                    .iCol = 1,
                                    .id = 8,
                                    .fPopped = TRUE,
                                    .idh = idhOtherFleetsHereTile,
                      }};
TIMER          vtimer = {0};
TIMERPROC      lpfnHostTimerProc = 0;
TechFieldType  iResTechNow = Energy;
WNDPROC        lpfnFakeCEProc = 0;
WNDPROC        lpfnFakeComboProc = 0;
WNDPROC        lpfnFakeEditProc = 0;
WNDPROC        lpfnFakeListProc = 0;
WNDPROC        lpfnRealCEProc = 0;
WNDPROC        lpfnRealComboProc = 0;
WNDPROC        lpfnRealEditProc = 0;
WNDPROC        lpfnRealListProc = 0;
WNDPROC        lpfnReportDlgProc = 0;
WindowLayout   iWindowLayout = layoutLarge;
char          *PCTDKT = "%dkT";
char          *PCTLD00 = "%ld00";
char          *rgszPlanetAttrAbbr[3] = {"Grav", "Temp", "Rad"};
char          *szButton = "BUTTON";
char          *szCombobox = "COMBOBOX";
char          *szEdit = "EDIT";
char          *szHelpFile = "stars!.hlp";
char          *szListbox = "LISTBOX";
char          *vrgszComputerLevel[5] = {"Easy", "Standard", "Tough", "Expert", "Random"};
char          *vrgszComputerPlayers[7] = {"Robotoids", "Turindrones", "Automitrons", "Rototills", "Cybertrons", "Macinti", "Random"};
char          *vrgszFileNew = 0;
char          *vrgszMRU = 0;
char          *vrgszRCWWidth[2] = {"<<     >>", ">>     <<"};
char           rgszArial[4][32] = {0};
char           szBrowser[13] = "starsbrowser";
char           szDirName[256] = "";
char           szFrame[11] = "starsframe";
char           szMessage[13] = "starsmessage";
char           szMine[10] = "starsmine";
char           szMineralTitle[90] = "";
char           szMsgTitle[90] = "";
char           szPlanet[12] = "starsplanet";
char           szPopupBuffer[256] = "";
char           szPopup[11] = "starspopup";
char           szReport[12] = "starsreport";
char           szScan[10] = "starsscan";
char           szTb[8] = "starstb";
char           szTitle[11] = "starstitle";
char           szTooltip[8] = "starstt";
int16_t       *rgXferValidHulls = 0;
int16_t        cFutureTech = 0;
int16_t        cMinGrafMax = 5000;
int16_t        cXferValidHulls = 0;
int16_t        chbrCache = 0;
int16_t        crcRCW = 0;
int16_t        crgbtnXfer = 0;
int16_t        csh = 0;
int16_t        dScanInc = 0;
int16_t        dScanPage = 0;
int16_t        dxBattleDD = 0;
int16_t        dxFleetCompLB = 0;
int16_t        dxMaxMineralQuan = 0;
int16_t        dxOrderED = 0;
int16_t        dxPlanetProdLB = 0;
int16_t        dxResLeft = 0;
int16_t        dxResRadio = 0;
int16_t        dxResRight = 0;
int16_t        dxResStrRight = 0;
int16_t        dxShipDD = 0;
int16_t        dxShipLB = 0;
int16_t        dxTip = 0;
int16_t        dxWinFrame = 0;
int16_t        dxyVCRBoard = 0;
int16_t        dxyVCRSquare = 0;
int16_t        dyArial10 = 0;
int16_t        dyArial6 = 0;
int16_t        dyArial7 = 0;
int16_t        dyFleetCompLB = 0;
int16_t        dyPlanetProdLB = 0;
int16_t        dySBar = 0;
int16_t        dyShipDD = 0;
int16_t        dyShipLB = 0;
int16_t        dySysFont = 0;
int16_t        dyTitleBar = 0;
int16_t        dyWinFrame = 0;
int16_t        fAnimate = FALSE;
int16_t        fBrowserValid = FALSE;
int16_t        fDirtyPlan = FALSE;
int16_t        fDlgUp = FALSE;
int16_t        fFreeingTitle = FALSE;
int16_t        fHullCopy = FALSE;
int16_t        fInEditUpdate = FALSE;
int16_t        fInScoreDialog = FALSE;
int16_t        fInScrollSet = FALSE;
int16_t        fLogOut = TRUE;
int16_t        fOrdersVis = FALSE;
int16_t        fProcessingTimer = FALSE;
int16_t        iAbout1st = 0;
int16_t        iAboutPartial = 0;
int16_t        iLastTutGet = -1;
int16_t        iPassCnt = 0;
int16_t        iPlanSelDlg = -1;
int16_t        iPopMenuSel = 0;
int16_t        idMsgObj = 0;
int16_t        irowEFleetCur = -1;
int16_t        iselProd = 0;
int16_t        iselSlot = -2;
int16_t        ishdefBuild = 0;
int16_t        pctResGlob = -1;
int16_t        rgdxOrderDD[3] = {0};
int16_t        vcRound = 0;
int16_t        vcScreenColors = 0;
int16_t        vcStepVCR = 0;
int16_t        vcplrNew = 0;
int16_t        vdxScoreX = 0;
int16_t        vfAscendingPrev = 0;
int16_t        viInRe = 0;
int16_t        viRound = 0;
int16_t        viSpeedVCR = 1;
int16_t        viStepVCRCur = 0;
int16_t        viStore = 0;
int16_t        viSubsortPrev = -1;
int16_t        viVCRFocus = 0;
int16_t        vicolSortPrev = -1;
int16_t        vidTimerTooltip = -1;
int16_t        vidsTooltip = 0;
int16_t        vpctProgressGauge = 0;
int16_t        vpctRadarView = 100;
int16_t        vrgScanPO[2][5] = {{7, 12, 19, 4, 6}, {3, 10, 11, 2, 3}};
int16_t        vrgcPrintMapPage[2] = {1, 1};
int16_t        vyZPDStatic = -1;
int16_t        xNewGameDiamond = 0;
int16_t        xScanTop = 0;
int16_t        yBuildInfoSum = 0;
int16_t        yScanTop = 0;
int16_t        yTopFutureTech = 0;
int16_t        yTopTechNote = -1;
int32_t       *vrgdpVCR = 0;
int32_t        lResBudget = 0;
int32_t        lResTotal = 0;
uint16_t      *vlprgidMisc = 0;
uint16_t      *vlprgidRep = 0;
uint16_t       grbitScanEShip = 0;
uint16_t       grbitScanMines = 0;
uint16_t       grbitScanShip = 0;
uint16_t       uDateInstalled = 0;
uint16_t       uTimerId = 0;
uint16_t       vcPasswordFailures = 0;
uint32_t       rgcrCache[32] = {0};
uint32_t       rgcrMinerals[6] = {16711680, 32512, 65535, 16777215, 255};
uint32_t       rgcrPlrHistory[16] = {4190448, 255,     65280,   16711680, 65535,   16711935, 16776960, 127,
                                     32512,   8323072, 8355711, 6801465,  2326527, 16744227, 32639,    6316128};
uint32_t       vtickTooltip1stVis = 0;
uint32_t       vtickTooltipLast = 0;
uint8_t       *lpb2k = 0;
uint8_t        mpiTypeiItem[3] = {iminesMineDispenser40, iminesHeavyDispenser50, iminesSpeedTrap20};
uint8_t        rghbrCacheUse[32] = {0};
uint8_t        rgszSpeed[30] = {45, 45, 0, 189, 0, 0, 190, 0, 0, 49, 0, 0, 49, 188, 0, 49, 189, 0, 49, 190, 0, 50, 0, 0, 50, 188, 0, 50, 189};
uint8_t        vbrcVCRFocus = 0;
uint8_t        vrgbEnvCur[11] = {0};
uint8_t        vrgbMachineConfig[11] = {0};
