#include <math.h>
#include <stdio.h>
#include "acutest.h"
#include "sfnum.h"

static uint64_t ullSeed = UINT64_C(0x53a27bcd01876f09);
static uint64_t Next(void) {
    ullSeed ^= ullSeed << 13;
    ullSeed ^= ullSeed >> 7;
    ullSeed ^= ullSeed << 17;
    return ullSeed;
}

static void test_oracle(void) {
    FILE              *pf;
    unsigned long long ux, uy, ur;
    float64_t          x, y;
    double             r;
    int                i, n, c = 0;

    pf = fopen(STARS_TEST_GOLDEN_DIR "/floating-transcendentals.txt", "r");
    TEST_ASSERT(pf != NULL);
    while ((n = fscanf(pf, "%d %llx %llx %llx", &i, &ux, &uy, &ur)) == 4) {
        x.v = ux;
        y.v = uy;
        switch (i) {
        case 0:
            r = Sf64Sqrt(Sf64Value(x));
            break;
        case 1:
            r = Sf64Floor(Sf64Value(x));
            break;
        case 2:
            r = Sf64Hypot(Sf64Value(x), Sf64Value(y));
            break;
        case 3:
            r = Sf64Atan2(Sf64Value(x), Sf64Value(y));
            break;
        case 4:
            r = Sf64Sin(Sf64Value(x));
            break;
        case 5:
            r = Sf64Cos(Sf64Value(x));
            break;
        case 6:
            r = Sf64Pow(Sf64Value(x), Sf64Value(y));
            break;
        default:
            TEST_ASSERT(0);
            return;
        }
        TEST_CHECK_(Sf64Bits(r).v == ur, "oracle row %d op %d actual=%016llx expected=%016llx", c, i, (unsigned long long)Sf64Bits(r).v, ur);
        c++;
    }
    TEST_CHECK(n == EOF && c == 14000);
    fclose(pf);
}

static void test_extended_precision(void) {
    SF80   x, y, r;
    double dx, dy;
    int    i;
#if LDBL_MANT_DIG == 64 && LDBL_MAX_EXP == 16384
    volatile long double a, b, expected;
    SF80                 native;
    long double          d;
#endif

    TEST_CHECK(extF80_roundingPrecision == 80);
    TEST_CHECK(softfloat_roundingMode == softfloat_round_near_even);
    // This is a game-relevant boundary that a 53-bit implementation misses.
    TEST_CHECK(Sf80ToI32(Sf80Mul(Sf80FromI32(400), Sf80From64(0.3))) == 119);
    for (i = 0; i < 10000; i++) {
        dx = (int32_t)Next() / 127.0;
        dy = ((int32_t)Next() | 1) / 31.0;
        x = Sf80From64(dx);
        y = Sf80From64(dy);
        r = Sf80Div(Sf80Add(Sf80Mul(x, y), x), y);
#if LDBL_MANT_DIG == 64 && LDBL_MAX_EXP == 16384
        // Volatile stores enforce each original x87 rounding boundary.
        a = dx;
        b = dy;
        expected = a * b;
        expected = expected + a;
        expected = expected / b;
        d = expected;
        memcpy(&native, &d, 10);
        TEST_CHECK_(r.signExp == native.signExp && r.signif == native.signif, "x87 sequence %d", i);
        TEST_CHECK(Sf64Bits(Sf64From80(r)).v == Sf64Bits((double)expected).v);
        TEST_CHECK(Sf32Bits(Sf32From80(r)).v == Sf32Bits((float)expected).v);
#else
        (void)r;
        TEST_CHECK(Sf64Bits(Sf64From80(x)).v == Sf64Bits(dx).v);
#endif
    }
}

static void test_special_values(void) {
    TEST_CHECK(Sf64Bits(Sf64Sin(-0.0)).v == UINT64_C(0x8000000000000000));
    TEST_CHECK(Sf64Cos(-0.0) == 1);
    TEST_CHECK(Sf64Hypot(INFINITY, NAN) == INFINITY);
    TEST_CHECK(isnan(Sf64Sqrt(-1)));
    TEST_CHECK(isnan(Sf64Sin(INFINITY)));
    TEST_CHECK(Sf64Bits(Sf64Atan2(-0.0, 1)).v == UINT64_C(0x8000000000000000));
    TEST_CHECK(Sf64Bits(Sf64Atan2(0.0, -1)).v == UINT64_C(0x400921fb54442d18));
    TEST_CHECK(Sf64Bits(Sf64Atan2(-INFINITY, -INFINITY)).v == UINT64_C(0xc002d97c7f3321d2));
}

TEST_LIST = {{"software math independent oracle", test_oracle},
             {"software extended precision", test_extended_precision},
             {"software math special values", test_special_values},
             {NULL, NULL}};
