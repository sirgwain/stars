// Read-only UI test observer. Linked only with STARS_TEST_TUTORIAL=ON.
#include <stdarg.h>
#include "win.h"

int16_t __real_InitInstance(int16_t nCmdShow);
int16_t __real_CchGetString(StringId ids, char *psz);

// Test builds keep settings beside the staged game, never in Windows/Stars.ini.
int16_t __wrap_CchGetString(StringId ids, char *psz) {
    if (ids == idsStarsIni) {
        strcpy(psz, ".\\Stars.ini");
        return 11;
    }
    return __real_CchGetString(ids, psz);
}

static HWND      stateWindow;
static HWND      stateEdit;
static char      stateText[65536];
static size_t    stateLength;
LRESULT CALLBACK __real_ScannerWndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
static WPARAM    lastMouseKeys;
static LPARAM    lastMousePoint;

// __wrap_ScannerWndProc records delivered clicks and delegates all behavior to Stars!.
LRESULT CALLBACK __wrap_ScannerWndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_LBUTTONDOWN) {
        lastMouseKeys = wParam;
        lastMousePoint = lParam;
    }
    return __real_ScannerWndProc(window, message, wParam, lParam);
}

// StateAppend appends one formatted observation, rejecting truncated snapshots.
static void StateAppend(const char *format, ...) {
    va_list args;
    int     count;
    va_start(args, format);
    count = vsnprintf(stateText + stateLength, sizeof(stateText) - stateLength, format, args);
    va_end(args);
    if (count < 0 || (size_t)count >= sizeof(stateText) - stateLength) {
        fputs("Tutorial observer snapshot overflow\n", stderr);
        ExitProcess(2);
    }
    stateLength += (size_t)count;
}

// StateRect records a rectangle in its owning window's client coordinates.
static void StateRect(const char *name, int index, HWND owner, const RECT *rect) {
    StateAppend("%s.%d=%llu|%ld|%ld|%ld|%ld\n", name, index, (unsigned long long)(uintptr_t)owner, (long)rect->left, (long)rect->top, (long)rect->right,
                (long)rect->bottom);
}

// StateWindow records the HWND of a game window or dynamically created control.
static void StateWindow(const char *name, HWND window) { StateAppend("hwnd.%s=%llu\n", name, (unsigned long long)(uintptr_t)window); }

// StateFindDialog finds a visible dialog owned by this game's process.
static HWND StateFindDialog(const char *title) {
    HWND  window;
    DWORD process;
    char  caption[256], className[64];
    for (window = GetTopWindow(NULL); window != NULL; window = GetWindow(window, GW_HWNDNEXT)) {
        GetWindowThreadProcessId(window, &process);
        if (process != GetCurrentProcessId() || !IsWindowVisible(window))
            continue;
        GetClassNameA(window, className, sizeof(className));
        GetWindowTextA(window, caption, sizeof(caption));
        if (strcmp(className, "#32770") == 0 && strcmp(caption, title) == 0)
            return window;
    }
    return NULL;
}

// StateTimer publishes state on the UI thread, including during modal dialogs.
static void CALLBACK StateTimer(HWND window, UINT message, UINT_PTR timer, DWORD tick) {
    int     i, j, x;
    RECT    rect;
    HWND    researchWindow, popupWindow;
    DWORD   popupProcess;
    HMENU   popupMenu;
    char    menuText[256];
    POINT16 point;
    FLEET  *fleet;
    PLANET *planet;
    ORDER  *order;
    char    savedWork[sizeof(szWork)];
    (void)window;
    (void)message;
    (void)timer;
    (void)tick;
    stateLength = 0;
    memcpy(savedWork, szWork, sizeof(savedWork));
    StateAppend("version=1\nturn=%u\nyear=%u\nactive=%u\npage=%d\nbold=%d\nturnDone=%u\nautoComplete=%u\nerror=%d\nselection=%d|%d|%d\nmessage=%d\nfontHeight=%"
                "d\nscannerMode=%u\n",
                game.turn, 2400 + game.turn, gd.fTutorial, tutor.idt / 8 + 1, tutor.idtBold, tutor.fTurnDone, tutor.fAutoComplete, tutor.idsError, sel.grobj,
                sel.id, sel.iwpAct, iMsgCur, dyArial8, grbitScan);
    StateAppend("scanSelection=%d|%d|%d\nkeyShift=%d\nkeyCtrl=%d\n", sel.scan.grobj, sel.scan.idpl, sel.scan.ifl, GetKeyState(VK_SHIFT),
                GetKeyState(VK_CONTROL));
    StateWindow("frame", hwndFrame);
    StateWindow("title", hwndTitle);
    StateWindow("scanner", hwndScanner);
    StateAppend("scannerClick=%llu|%d|%d\n", (unsigned long long)lastMouseKeys, (int16_t)LOWORD(lastMousePoint), (int16_t)HIWORD(lastMousePoint));
    if (hwndScanner != NULL) {
        GetClientRect(hwndScanner, &rect);
        StateRect("scannerBounds", 0, hwndScanner, &rect);
        rect.bottom -= dySBar;
        StateRect("scannerViewport", 0, hwndScanner, &rect);
    }
    StateWindow("command", hwndPlanet);
    StateWindow("summary", hwndMine);
    StateWindow("messages", hwndMessage);
    StateWindow("toolbar", hwndTb);
    StateWindow("tutor", tutor.hwnd);
    StateWindow("production", hwndProdDlg);
    StateWindow("designer", hwndSlotDlg);
    StateWindow("browser", hwndBrowser);
    StateWindow("score", hwndScoreXDlg);
    StateWindow("report", hwndReportDlg);
    StateWindow("waypoints", hwndShipLB);
    StateWindow("repeat", hwndRepCB);
    StateWindow("composition", hwndFleetCompLB);
    StateWindow("orbit", hwndShipDD);
    StateWindow("planetQueue", hwndPlanetProdLB);
    StateAppend("messageCount=%d\nprogress=%u\n", cMsg, tutor.fProgress);
    researchWindow = StateFindDialog("Research");
    StateWindow("research", researchWindow);
    if (researchWindow != NULL && pctResGlob >= 0) {
        StateRect("researchUp", 0, researchWindow, &rcSpinTop);
        StateRect("researchDown", 0, researchWindow, &rcSpinBot);
        StateAppend("researchBudget=%d\nresearchFuture=%llu|%d|%d|%d\n", pctResGlob, (unsigned long long)(uintptr_t)researchWindow, dxResLeft, yTopFutureTech,
                    cFutureTech);
    }
    popupWindow = FindWindowA("#32768", NULL);
    if (popupWindow != NULL) {
        GetWindowThreadProcessId(popupWindow, &popupProcess);
        if (popupProcess == GetCurrentProcessId()) {
            popupMenu = (HMENU)SendMessage(popupWindow, 0x01e1, 0, 0);
            for (i = 0; i < GetMenuItemCount(popupMenu); i++) {
                GetMenuStringA(popupMenu, i, menuText, sizeof(menuText), MF_BYPOSITION);
                if (GetMenuItemRect(NULL, popupMenu, i, &rect)) {
                    StateAppend("menu.%d=%ld|%ld|%ld|%ld|%s\n", i, (long)rect.left, (long)rect.top, (long)rect.right, (long)rect.bottom, menuText);
                }
            }
        }
    }
    x = 4;
    for (i = 0; i < 29; i++) {
        int button = (int8_t)vrgTBBtn[i];
        int width = DxOfBtn(button);
        if (button >= 0) {
            SetRect(&rect, x, 0, x + width, 28);
            StateRect("toolbarButton", button, hwndTb, &rect);
        }
        x += width;
    }
    if (hwndReportDlg != NULL && vprptCur != NULL) {
        StateAppend("reportSort=%d|%d|%d\n", vprptCur->icolSort, vprptCur->iSubsort, vprptCur->fAscending);
        x = 2;
        for (i = 0; i < vprptCur->cFields; i++) {
            if ((vprptCur->grbitVisible & (1 << i)) != 0 && (i == 0 || i >= vprptCur->cFieldFirst)) {
                SetRect(&rect, x, 2, x + vprptCur->rgbdx[i] * 2, dyArial8 + 6);
                StateRect("reportColumn", i, hwndReportDlg, &rect);
                x = rect.right;
            }
        }
    }
    StateRect("messageTitle", 0, hwndMessage, &rcMsgTitle);
    StateRect("productionDiamond", 0, hwndProdDlg, &rcProdDiamond);
    for (i = 0; i < 13; i++)
        StateAppend("button.%d=%llu\n", i, (unsigned long long)(uintptr_t)rghwndBtn[i]);
    for (i = 0; i < 4; i++)
        StateAppend("messageButton.%d=%llu\n", i, (unsigned long long)(uintptr_t)rghwndMsgBtn[i]);
    for (i = 0; i < 3; i++)
        StateAppend("orderControl.%d=%llu\n", i, (unsigned long long)(uintptr_t)rghwndOrderDD[i]);
    for (i = 0; i < 19; i++)
        StateRect("reference", i, hwndPlanet, &rgrcRef[i]);
    if (game.lid != 0 && idPlayer >= 0 && hwndScanner != NULL) {
        for (i = 0; i < game.cPlanMax; i++) {
            point = rgptPlan[i];
            LogicalToScan(&point);
            StateAppend("planet.%d=%s|%d|%d\n", i, PszGetPlanetName(i), point.x, point.y);
        }
        for (i = 0; i < cPlanet; i++) {
            planet = &lpPlanets[i];
            StateAppend("planetSettings.%d=%d|%u\n", planet->id, planet->iPlayer, planet->fNoResearch);
            if (planet->lpplprod != NULL) {
                for (j = 0; j < planet->lpplprod->iprodMac; j++) {
                    PROD *prod = &planet->lpplprod->rgprod[j];
                    StateAppend("production.%d.%d=%u|%u|%u\n", planet->id, j, prod->grobj, prod->iItem, prod->cItem);
                }
            }
        }
        for (i = 0; i < cFleet; i++) {
            fleet = rglpfl[i];
            // The fleet table contains vacant entries after fleets are recycled.
            if (fleet == NULL)
                continue;
            point = fleet->pt;
            LogicalToScan(&point);
            StateAppend("fleet.%d=%s|%d|%d|%ld|%ld|%ld|%ld|%ld|%u\n", fleet->id, PszGetFleetName(fleet->id), point.x, point.y, (long)fleet->rgwtMin[0],
                        (long)fleet->rgwtMin[1], (long)fleet->rgwtMin[2], (long)fleet->rgwtMin[3], (long)fleet->rgwtMin[4], fleet->fRepOrders);
            for (j = 0; j < fleet->cord; j++) {
                order = &fleet->lpplord->rgord[j];
                point = order->pt;
                LogicalToScan(&point);
                StateAppend("waypoint.%d.%d=%d|%d|%u|%u|%d|%d\n", fleet->id, j, order->grobj, order->id, order->grTask, order->iWarp, point.x, point.y);
            }
        }
        StateAppend("research=%u|%u\n", rgplr[idPlayer].iTechCur, rgplr[idPlayer].pctResearch);
        for (i = 0; i < rgplr[idPlayer].cShDef; i++) {
            StateAppend("design.%d=%s|%d\n", i, rgshdef[i].hul.szClass, rgshdef[i].hul.ihuldef);
            for (j = 0; j < rgshdef[i].hul.chs; j++) {
                HS *slot = &rgshdef[i].hul.rghs[j];
                StateAppend("designSlot.%d.%d=%u|%u|%u\n", i, j, slot->grhst, slot->iItem, slot->cItem);
            }
        }
    }
    if (mdXferDlg != mdXferNone) {
        StateAppend("transferMode=%d\n", mdXferDlg);
        for (i = 0; i < 2; i++) {
            if (pxfer[i].grobj == grobjFleet) {
                StateAppend("transferFleet.%d=%d|%ld|%ld|%ld\n", i, pxfer[i].fl.id, (long)pxfer[i].fl.rgwtMin[3], (long)pxfer[i].fl.rgwtMin[4],
                            (long)LGetFleetStat(&pxfer[i].fl, 1));
            }
        }
        for (i = 0; i < crgbtnXfer; i++) {
            StateAppend("transferButton.%d=%d|%d|%u|%u|%ld|%ld|%ld|%ld\n", i, rgbtnXfer[i].iVal & 0x7f, rgbtnXfer[i].iSide, rgbtnXfer[i].fVisible,
                        rgbtnXfer[i].fDisabled, (long)rgbtnXfer[i].rc.left, (long)rgbtnXfer[i].rc.top, (long)rgbtnXfer[i].rc.right,
                        (long)rgbtnXfer[i].rc.bottom);
        }
    }
    if (hwndSlotDlg != NULL && lpshdefBuild != NULL) {
        StateAppend("buildMode=%d\nbuildHull=%d\n", mdBuild, lpshdefBuild->hul.ihuldef);
        for (i = 0; i < lpshdefBuild->hul.chs; i++) {
            StateRect("slot", i, hwndSlotDlg, &vrgrcSlot[i]);
            StateAppend("component.%d=%u|%u|%u\n", i, lpshdefBuild->hul.rghs[i].grhst, lpshdefBuild->hul.rghs[i].iItem, lpshdefBuild->hul.rghs[i].cItem);
        }
        for (i = 0; i < 2; i++)
            StateRect("imageArrow", i, hwndSlotDlg, &rgrcBuildSpin[i]);
    }
    SetWindowTextA(stateEdit, stateText);
    memcpy(szWork, savedWork, sizeof(savedWork));
}

// StateWindowProc refreshes observations synchronously without changing game state.
static LRESULT CALLBACK StateWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_APP + 1) {
        StateTimer(window, WM_TIMER, 1, GetTickCount());
        return 0;
    }
    return DefWindowProcA(window, message, wParam, lParam);
}

// __wrap_InitInstance installs an observer after the real application initializes.
int16_t __wrap_InitInstance(int16_t nCmdShow) {
    WNDCLASSA observerClass = {0};
    int16_t   result = __real_InitInstance(nCmdShow);
    if (result == 0)
        return result;
    observerClass.lpfnWndProc = StateWindowProc;
    observerClass.hInstance = hInst;
    observerClass.lpszClassName = "StarsTutorialObserver";
    if (RegisterClassA(&observerClass) == 0) {
        fputs("Cannot register tutorial observer window\n", stderr);
        ExitProcess(2);
    }
    stateWindow = CreateWindowExA(0, "StarsTutorialObserver", "Stars Tutorial Test State", WS_POPUP, 0, 0, 1, 1, hwndFrame, NULL, hInst, NULL);
    stateEdit = CreateWindowExA(0, "EDIT", "", WS_CHILD | ES_MULTILINE | ES_READONLY, 0, 0, 1, 1, stateWindow, (HMENU)(uintptr_t)50000, hInst, NULL);
    if (stateWindow == NULL || stateEdit == NULL || SetTimer(stateWindow, 1, 100, StateTimer) == 0) {
        fputs("Cannot create tutorial test observer\n", stderr);
        ExitProcess(2);
    }
    SendMessage(stateEdit, EM_SETLIMITTEXT, sizeof(stateText), 0);
    return result;
}
