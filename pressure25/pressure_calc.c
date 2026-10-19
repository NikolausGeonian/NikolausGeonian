#include "pressure_calc.h"

#include <float.h>
#include <math.h>

static int representable(long double value)
{
    return isfinite(value) && value <= DBL_MAX && value >= -DBL_MAX;
}

int pressure_calculate(double p1, double t1, double p2, double t2,
                       PressureKind kind, double atmosphere,
                       PressureResult *out)
{
    long double offset, absolute1, absolute2, normalized1, normalized2;
    PressureResult result = {0.0, 0.0, 0.0, 0};

    if (out == NULL || (kind != PRESSURE_GAUGE && kind != PRESSURE_ABSOLUTE)) {
        return PRESSURE_ERROR_ARGUMENT;
    }
    if (!isfinite(p1) || !isfinite(p2) || !isfinite(t1) || !isfinite(t2) ||
        !isfinite(atmosphere)) {
        return PRESSURE_ERROR_NONFINITE;
    }
    if (t1 <= -273.15 || t2 <= -273.15) {
        return PRESSURE_ERROR_TEMPERATURE;
    }
    if (atmosphere < 0.0) {
        return PRESSURE_ERROR_ATMOSPHERE;
    }

    offset = kind == PRESSURE_GAUGE ? (long double)atmosphere : 0.0L;
    absolute1 = (long double)p1 + offset;
    absolute2 = (long double)p2 + offset;
    if (absolute1 <= 0.0L || absolute2 <= 0.0L) {
        return PRESSURE_ERROR_ABSOLUTE;
    }

    /* Preserve exact identity at 25 C, especially for a zero gauge reading. */
    normalized1 = t1 == 25.0 ? (long double)p1 :
        absolute1 * (298.15L / ((long double)t1 + 273.15L)) - offset;
    normalized2 = t2 == 25.0 ? (long double)p2 :
        absolute2 * (298.15L / ((long double)t2 + 273.15L)) - offset;
    if (!representable(normalized1) || !representable(normalized2)) {
        return PRESSURE_ERROR_RANGE;
    }

    result.pressure25_first = (double)normalized1;
    result.pressure25_second = (double)normalized2;
    if (result.pressure25_first > 0.0) {
        long double decrease =
            ((long double)result.pressure25_first - result.pressure25_second) /
            (long double)result.pressure25_first * 100.0L;
        if (!representable(decrease)) {
            return PRESSURE_ERROR_RANGE;
        }
        result.decrease_percent = (double)decrease;
        result.decrease_defined = 1;
    }
    *out = result;
    return PRESSURE_OK;
}

const wchar_t *pressure_error_message(int code)
{
    switch (code) {
    case PRESSURE_OK:
        return L"计算完成。";
    case PRESSURE_ERROR_ARGUMENT:
        return L"计算参数无效。";
    case PRESSURE_ERROR_NONFINITE:
        return L"请输入有限的数字，不能输入无穷大或非数字值。";
    case PRESSURE_ERROR_TEMPERATURE:
        return L"两组温度都必须高于 -273.15 ℃（绝对零度）。";
    case PRESSURE_ERROR_ATMOSPHERE:
        return L"大气压力不能为负数。";
    case PRESSURE_ERROR_ABSOLUTE:
        return L"绝对压力必须大于 0；表压加大气压力后也必须大于 0。";
    case PRESSURE_ERROR_RANGE:
        return L"计算结果超出数值范围，请检查输入的压力和温度。";
    default:
        return L"发生未知计算错误。";
    }
}
