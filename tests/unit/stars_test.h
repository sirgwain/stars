#ifndef STARS_TEST_H
#define STARS_TEST_H

// Helpers for unit tests that link the game code. Acutest runs every test
// in its own process, so each test starts from the game's initial globals
// and calls FStarsTestInit first. Helpers return FALSE on failure.

#ifdef _WIN32
#include "win.h"
#else
#include "common.h"
#endif

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

// getcwd names the working directory on either platform.
#ifdef _WIN32
#include <direct.h>
#define getcwd _getcwd
#else
#include <unistd.h>
#endif

// The game's message boxes are recorded here instead of shown, so nothing
// waits for a click: through a test-only AlertSz replacement on Windows, and IdAlertBox in
// the native tests.
extern int  cStarsTestAlert;
extern char szStarsTestAlert[512];

// FStarsTestInit prepares the game state WinMain sets up before it creates
// windows: on Windows the instance, brushes and fonts (FCreateStuff), and
// elsewhere the game's buffers (InitGameStuff); and a fixed RNG seed.
int16_t FStarsTestInit(void);

// FStarsTestDir creates, or empties, work\<pszName> under the test working
// directory and returns its absolute path in szDir.
int16_t FStarsTestDir(const char *pszName, char *szDir, size_t cchDir);

// FStarsTestNewGame creates a game in szDir from a one-player tiny universe
// definition (the tutorial-sized test race, seed lSeed) and sets szBase to
// it. rgszAi lists extra players as "#<race> <skill>" definition lines.
int16_t FStarsTestNewGame(const char *szDir, uint32_t lSeed, const char **rgszAi, int16_t cAi);

// FStarsTestLoadHost unloads the current game and loads szBase's host file.
int16_t FStarsTestLoadHost(void);

// FStarsTestLoadPlayer unloads the current game and loads player iPlr's
// turn file (game.m<iPlr + 1>) as that player.
int16_t FStarsTestLoadPlayer(int16_t iPlr);

// LpplStarsTestHomeworld returns iPlr's homeworld in the loaded game, or NULL.
PLANET *LpplStarsTestHomeworld(int16_t iPlr);

// LpflStarsTestAddFleet adds a fleet of csh ships of iPlr's design ishdef
// orbiting planet idPlanet in the loaded game, with no orders.
FLEET *LpflStarsTestAddFleet(int16_t iPlr, int16_t idPlanet, int16_t ishdef, int16_t csh);

// FStarsTestSaveHost writes the loaded game back to szBase's host file, so a
// following FStarsTestGenerate sees the test's changes.
int16_t FStarsTestSaveHost(void);

// FStarsTestSaveGame writes every player's turn file and then the host
// file from the loaded game, as turn generation does, so the game can be
// opened in Stars! as it is.
int16_t FStarsTestSaveGame(void);

// FStarsTestGenerate generates one turn from szBase's host file and turn
// files, as the -g command line does.
int16_t FStarsTestGenerate(void);

#endif
