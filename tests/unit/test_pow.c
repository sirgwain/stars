#include "acutest.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "sfnum.h"

// An optional candidate can still be checked against the frozen oracle.
#ifdef STARS_TEST_POW_CUSTOM
extern double DStarsTestPow(double x, double y);
#else
static double DStarsTestPow(double x, double y) {
    return Sf64Pow(x, y);
}
#endif
#ifndef STARS_TEST_POW_MAX_ULP
#define STARS_TEST_POW_MAX_ULP 0
#endif

static uint64_t Bits(double d) {
    uint64_t ull;
    memcpy(&ull, &d, sizeof(ull));
    return ull;
}

static void CheckCorpus(int fAccuracy) {
    FILE *pf;
    unsigned long long x, y, native, correct;
    uint64_t ullActual, ullExpected, ullDiff;
    double dx, dy;
    int c = 0, cNativeCorrection = 0;
    int n;

    pf = fopen(STARS_TEST_GOLDEN_DIR "/floating-pow.txt", "r");
    TEST_ASSERT(pf != NULL);
    while ((n = fscanf(pf, "%llx %llx %llx %llx", &x, &y, &native, &correct)) == 4) {
        memcpy(&dx, &x, sizeof(dx));
        memcpy(&dy, &y, sizeof(dy));
        ullActual = Bits(DStarsTestPow(dx, dy));
        ullExpected = fAccuracy ? correct : native;
        // The frozen GCC library missed this rounding by one ULP. Keep the
        // historical row intact and require the independent oracle's answer.
        if (!fAccuracy && x == UINT64_C(0x3fe8000000000000) && y == UINT64_C(0x4029ffffffffffff)) {
            TEST_CHECK(native == UINT64_C(0x3f9853d300000004) && correct == UINT64_C(0x3f9853d300000003));
            ullExpected = correct;
            cNativeCorrection++;
        }
        ullDiff = ullActual > ullExpected ? ullActual - ullExpected : ullExpected - ullActual;
        TEST_CHECK_((ullExpected == UINT64_C(0x7ff0000000000000) ? ullActual == ullExpected :
                     ullActual < UINT64_C(0x7ff0000000000000) && ullDiff <= (fAccuracy ? STARS_TEST_POW_MAX_ULP : 0)),
                    "row %d x=%a y=%a actual=%016llx expected=%016llx difference=%llu ULP", c, dx, dy,
                    (unsigned long long)ullActual, (unsigned long long)ullExpected, (unsigned long long)ullDiff);
        c++;
    }
    TEST_CHECK(n == EOF);
    TEST_CHECK(c == 2117);
    TEST_CHECK(fAccuracy || cNativeCorrection == 1);
    fclose(pf);
}

static void test_native_compatibility(void) { CheckCorpus(0); }
static void test_mathematical_accuracy(void) { CheckCorpus(1); }

static void test_packet_integer_boundaries(void) {
    static const int32_t rgMineral[] = {1, 7, 99, 100, 101, 999, 1000, 1001, 32767, 45000};
    FILE *pf;
    unsigned long long x, y, native, correct;
    double dx, dy, dNative, dActual;
    long double dExpected, dResult;
    int i, c = 0, n;

    pf = fopen(STARS_TEST_GOLDEN_DIR "/floating-pow.txt", "r");
    TEST_ASSERT(pf != NULL);
    while ((n = fscanf(pf, "%llx %llx %llx %llx", &x, &y, &native, &correct)) == 4) {
        memcpy(&dx, &x, sizeof(dx));
        memcpy(&dy, &y, sizeof(dy));
        memcpy(&dNative, &native, sizeof(dNative));
        if (dx != 0.75 && dx != 0.875)
            continue;
        dActual = DStarsTestPow(dx, dy);
        TEST_ASSERT(isfinite(dActual) && dActual > 0);
        for (i = 0; i < 10; i++) {
            dExpected = (long double)rgMineral[i] * dNative;
            dResult = (long double)rgMineral[i] * dActual;
            TEST_CHECK_(dResult >= 0 && dResult < 2147483648.0L && (int32_t)dResult == (int32_t)dExpected,
                        "packet multiplication x=%a y=%a minerals=%ld", dx, dy, (long)rgMineral[i]);
            dExpected = (long double)rgMineral[i] / dNative;
            dResult = (long double)rgMineral[i] / dActual;
            if (dExpected < 2147483648.0L)
                TEST_CHECK_(dResult >= 0 && dResult < 2147483648.0L && (int32_t)dResult == (int32_t)dExpected,
                            "packet division x=%a y=%a minerals=%ld", dx, dy, (long)rgMineral[i]);
        }
        c++;
    }
    TEST_CHECK(n == EOF && c == 1086);
    fclose(pf);
}

static void test_special_values(void) {
    TEST_CHECK(DStarsTestPow(0.75, 0) == 1);
    TEST_CHECK(DStarsTestPow(0.875, 1) == 0.875);
    TEST_CHECK(DStarsTestPow(4, 0.5) == 2);
    TEST_CHECK(DStarsTestPow(-2, 3) == -8);
    TEST_CHECK(DStarsTestPow(-2, 4) == 16);
    TEST_CHECK(isnan(DStarsTestPow(-2, 0.5)));
    TEST_CHECK(isnan(DStarsTestPow(NAN, 2)));
    TEST_CHECK(DStarsTestPow(NAN, 0) == 1);
    TEST_CHECK(DStarsTestPow(1, NAN) == 1);
    TEST_CHECK(Bits(DStarsTestPow(-0.0, 3)) == Bits(-0.0));
    TEST_CHECK(Bits(DStarsTestPow(-0.0, 2)) == Bits(0.0));
    TEST_CHECK(DStarsTestPow(0, -1) == INFINITY);
    TEST_CHECK(DStarsTestPow(-0.0, -3) == -INFINITY);
    TEST_CHECK(DStarsTestPow(INFINITY, -1) == 0);
    TEST_CHECK(DStarsTestPow(0.75, INFINITY) == 0);
    TEST_CHECK(DStarsTestPow(0.75, -INFINITY) == INFINITY);
}

TEST_LIST = {{"pow native compatibility", test_native_compatibility},
             {"pow mathematical accuracy", test_mathematical_accuracy},
             {"pow packet integer boundaries", test_packet_integer_boundaries},
             {"pow special values", test_special_values}, {NULL, NULL}};
