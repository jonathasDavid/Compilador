# Changes

- Added unit test `tests/t_symtab_unit.c` for symtab_get_last_decimal_parts/set.
- Integrated `t_symtab_unit` into `run_tests.ps1`.
- Rebuilt `tests/t10.exe` from current sources to remove leftover debug prints.
- Ensured all tests pass via `run_tests.ps1`.
- Minor refactors: replaced magic number `16` with `MAX_IO_ARGS` in `parser.c` and improved `symtab_print()` to show last assigned values.

All changes tested locally on Windows (PowerShell) with gcc and test runner.
