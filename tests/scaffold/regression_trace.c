// Regression trace, linked into native builds configured with
// -DSTARS_TEST_TRACE=ON. Test source variants route calls from other object
// files to these wrappers, which call the original unchanged and, when the
// STARS_TRACE environment variable names a file, log one tagged line per call:
//
//   R turn player ai range result rva seed1 seed2   Random
//   P turn player ai planet result rva              PctPlanetCapacity
//
// rva is the caller's return address relative to the executable image base.
// seed1 and seed2 are the RNG state before the draw, so a stream offset can
// be replayed for any range.

#include <stdlib.h>
#include "win.h"

#ifdef _MSC_VER
#include <intrin.h>
#define STARS_TEST_RETURN_ADDRESS() _ReturnAddress()
#else
#define STARS_TEST_RETURN_ADDRESS() __builtin_return_address(0)
#endif

int16_t __real_Random(int16_t c);
int16_t __real_PctPlanetCapacity(PLANET *lppl);

static FILE   *vpfileTrace;
static int16_t vfTraceInit;

// TraceFile opens $STARS_TRACE on first use and returns it, or NULL when unset.
static FILE *TraceFile(void) {
    char *psz;

    if (vfTraceInit == 0) {
        vfTraceInit = 1;
        psz = getenv("STARS_TRACE");
        if (psz != NULL && *psz != 0) {
            vpfileTrace = fopen(psz, "w");
        }
    }
    return vpfileTrace;
}

// TraceCall logs one tagged event with the current turn and player context,
// followed by pszExtra when it is not empty.
static void TraceCall(char chTag, int32_t lArg, int32_t lResult, void *pvReturn, const char *pszExtra) {
    FILE *pfile;

    pfile = TraceFile();
    if (pfile != NULL) {
        fprintf(pfile, "%c %u %d %d %ld %ld %llx%s\n", chTag, game.turn, idPlayer, fAi, (long)lArg, (long)lResult,
                (unsigned long long)((uintptr_t)pvReturn - (uintptr_t)GetModuleHandle(NULL)), pszExtra);
    }
}

// __wrap_Random draws from the game RNG and logs the range, result, and prior seeds.
int16_t __wrap_Random(int16_t c) {
    char    szSeeds[32];
    int32_t lSeed1;
    int32_t lSeed2;
    int16_t r;

    lSeed1 = lRandSeed1;
    lSeed2 = lRandSeed2;
    r = __real_Random(c);
    snprintf(szSeeds, sizeof(szSeeds), " %ld %ld", (long)lSeed1, (long)lSeed2);
    TraceCall('R', c, r, STARS_TEST_RETURN_ADDRESS(), szSeeds);
    return r;
}

// __wrap_PctPlanetCapacity computes planet capacity and logs the planet ID and result.
int16_t __wrap_PctPlanetCapacity(PLANET *lppl) {
    int16_t pct;

    pct = __real_PctPlanetCapacity(lppl);
    TraceCall('P', lppl->id, pct, STARS_TEST_RETURN_ADDRESS(), "");
    return pct;
}
