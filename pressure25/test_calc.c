#include "pressure_calc.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>

static void near(double actual, double expected, double tolerance)
{
    if (fabs(actual - expected) > tolerance) {
        fprintf(stderr, "Expected %.17g, got %.17g (tolerance %.3g)\n",
                expected, actual, tolerance);
        assert(0);
    }
}

int main(void)
{
    PressureResult r;
    const double atm = 0.101325;

    /* At the reference temperature, gauge readings remain unchanged. */
    assert(pressure_calculate(0.5, 25, 0.4, 25, PRESSURE_GAUGE, atm, &r) == 0);
    near(r.pressure25_first, 0.5, 0.0);
    near(r.pressure25_second, 0.4, 0.0);
    assert(r.decrease_defined);
    near(r.decrease_percent, 20.0, 1e-12);

    /* Warming the same indicated pressure gives a lower normalized reading. */
    assert(pressure_calculate(0.5, 25, 0.5, 35, PRESSURE_GAUGE, atm, &r) == 0);
    near(r.pressure25_second, 0.4804859646276164, 1e-14);
    near(r.decrease_percent, 3.902807074476722, 1e-12);

    /* Independent constant-volume, fixed-mass pair: no pressure loss. */
    assert(pressure_calculate(0.5, 25, 0.5201685393258427, 35,
                              PRESSURE_GAUGE, atm, &r) == 0);
    near(r.pressure25_second, 0.5, 1e-14);
    near(r.decrease_percent, 0.0, 1e-12);

    /* Exactly double absolute temperature and pressure preserve the state. */
    assert(pressure_calculate(0.5, 25, 1.101325, 323.15,
                              PRESSURE_GAUGE, atm, &r) == 0);
    near(r.pressure25_second, 0.5, 1e-14);
    near(r.decrease_percent, 0.0, 1e-12);

    /* Absolute mode has no atmospheric correction. */
    assert(pressure_calculate(0.6, 25, 1.2, 323.15,
                              PRESSURE_ABSOLUTE, atm, &r) == 0);
    near(r.pressure25_second, 0.6, 1e-14);
    near(r.decrease_percent, 0.0, 1e-12);

    /* Units scale both outputs but leave the percentage unchanged. */
    assert(pressure_calculate(500.0, 25, 400.0, 25,
                              PRESSURE_GAUGE, 101.325, &r) == 0);
    near(r.pressure25_first, 500.0, 0.0);
    near(r.pressure25_second, 400.0, 0.0);
    near(r.decrease_percent, 20.0, 1e-12);

    /* A rise is reported as a negative decrease, not clamped to zero. */
    assert(pressure_calculate(0.5, 25, 0.6, 25, PRESSURE_GAUGE, atm, &r) == 0);
    near(r.decrease_percent, -20.0, 1e-12);

    /* Zero/negative first normalized gauge pressures have no decrease ratio. */
    assert(pressure_calculate(0.0, 25, 0.0, 35, PRESSURE_GAUGE, atm, &r) == 0);
    near(r.pressure25_first, 0.0, 0.0);
    near(r.pressure25_second, -0.00328817134512413, 1e-15);
    assert(!r.decrease_defined);
    assert(pressure_calculate(-0.05, 25, -0.06, 25,
                              PRESSURE_GAUGE, atm, &r) == 0);
    near(r.pressure25_first, -0.05, 0.0);
    assert(!r.decrease_defined);

    /* Finite extreme input should not overflow an unnecessary intermediate. */
    assert(pressure_calculate(DBL_MAX, 25, DBL_MAX, 25,
                              PRESSURE_ABSOLUTE, atm, &r) == 0);
    near(r.pressure25_first, DBL_MAX, 0.0);
    near(r.decrease_percent, 0.0, 0.0);

    assert(pressure_calculate(1, -273.15, 1, 25, PRESSURE_GAUGE, atm, &r) ==
           PRESSURE_ERROR_TEMPERATURE);
    assert(pressure_calculate(1, 25, 1, -274, PRESSURE_GAUGE, atm, &r) ==
           PRESSURE_ERROR_TEMPERATURE);
    assert(pressure_calculate(-atm, 25, 1, 25, PRESSURE_GAUGE, atm, &r) ==
           PRESSURE_ERROR_ABSOLUTE);
    assert(pressure_calculate(0, 25, 1, 25, PRESSURE_ABSOLUTE, atm, &r) ==
           PRESSURE_ERROR_ABSOLUTE);
    assert(pressure_calculate(1, 25, 1, 25, PRESSURE_GAUGE, -1, &r) ==
           PRESSURE_ERROR_ATMOSPHERE);
    assert(pressure_calculate(NAN, 25, 1, 25, PRESSURE_GAUGE, atm, &r) ==
           PRESSURE_ERROR_NONFINITE);
    assert(pressure_calculate(1, INFINITY, 1, 25, PRESSURE_GAUGE, atm, &r) ==
           PRESSURE_ERROR_NONFINITE);
    assert(pressure_calculate(1, 25, 1, 25, PRESSURE_GAUGE, NAN, &r) ==
           PRESSURE_ERROR_NONFINITE);
    assert(pressure_calculate(1, 25, 1, 25, (PressureKind)99, atm, &r) ==
           PRESSURE_ERROR_ARGUMENT);
    assert(pressure_calculate(1, 25, 1, 25, PRESSURE_GAUGE, atm, NULL) ==
           PRESSURE_ERROR_ARGUMENT);

    /* Actual normalized pressure or percentage overflow is rejected. */
    assert(pressure_calculate(DBL_MAX, -273.0, 1, 25,
                              PRESSURE_ABSOLUTE, atm, &r) == PRESSURE_ERROR_RANGE);
    assert(pressure_calculate(DBL_MIN, 25, DBL_MAX, 25,
                              PRESSURE_ABSOLUTE, atm, &r) == PRESSURE_ERROR_RANGE);

    assert(pressure_error_message(PRESSURE_OK)[0] != L'\0');
    assert(pressure_error_message(999)[0] != L'\0');
    puts("All pressure calculation tests passed.");
    return 0;
}
