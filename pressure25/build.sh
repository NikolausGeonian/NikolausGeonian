#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "$0")"
CC="${CC:-x86_64-w64-mingw32-gcc}"
WINDRES="${WINDRES:-x86_64-w64-mingw32-windres}"
mkdir -p build dist
"$WINDRES" -i app.rc -o build/app.o
"$CC" -std=c11 -O2 -Wall -Wextra -Werror -finput-charset=UTF-8 \
  -municode -mwindows -static main.c pressure_calc.c build/app.o \
  -o dist/Pressure25.exe -lcomctl32 -lgdi32 -luser32 -lm \
  -Wl,--dynamicbase,--nxcompat
"$CC" -std=c11 -O2 -Wall -Wextra -Werror -finput-charset=UTF-8 \
  -static pressure_calc.c test_calc.c -o build/test_calc.exe -lm
"$CC" -std=c11 -O2 -Wall -Wextra -Werror -finput-charset=UTF-8 \
  -municode -mconsole -static -DGUI_TEST main.c pressure_calc.c build/app.o \
  -o build/test_gui.exe -lcomctl32 -lgdi32 -luser32 -lm
printf 'Built: %s/dist/Pressure25.exe\n' "$PWD"
