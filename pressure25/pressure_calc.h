#ifndef PRESSURE_CALC_H
#define PRESSURE_CALC_H

#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PRESSURE_GAUGE = 0,
    PRESSURE_ABSOLUTE = 1
} PressureKind;

typedef struct {
    double pressure25_first;
    double pressure25_second;
    double decrease_percent;
    int decrease_defined;
} PressureResult;

typedef enum {
    PRESSURE_OK = 0,
    PRESSURE_ERROR_ARGUMENT = 1,
    PRESSURE_ERROR_NONFINITE = 2,
    PRESSURE_ERROR_TEMPERATURE = 3,
    PRESSURE_ERROR_ATMOSPHERE = 4,
    PRESSURE_ERROR_ABSOLUTE = 5,
    PRESSURE_ERROR_RANGE = 6
} PressureStatus;

/*
 * Ideal gas at constant volume. Temperatures are degrees Celsius. All pressure
 * values, including atmosphere, must use the same unit. Returned pressures use
 * the selected gauge/absolute convention. Decrease is relative to the first
 * returned pressure and is undefined when that pressure is nonpositive.
 * The output is modified only on success.
 */
int pressure_calculate(double p1, double t1, double p2, double t2,
                       PressureKind kind, double atmosphere,
                       PressureResult *out);

const wchar_t *pressure_error_message(int code);

#ifdef __cplusplus
}
#endif

#endif
