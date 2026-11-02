#!/usr/bin/env python3
"""
E2E-тесты для библиотеки матриц (без pytest).

Требуется собранный бинарник matrix_cli. Путь задаётся через:
  * переменную окружения MATRIX_CLI
  * либо аргумент --cli PATH
  * по умолчанию ищется ./build/matrix_cli и ../build/matrix_cli

Запуск:
  python test/test_e2e.py
  python test/test_e2e.py --cli ./build/matrix_cli
  python test/test_e2e.py -v            # печатать каждый тест подробно
  python test/test_e2e.py -k random     # запускать только тесты, содержащие "random"
  python test/test_e2e.py --no-color    # без ANSI-раскраски
"""

from __future__ import annotations

import argparse
import math
import os
import random
import subprocess
import sys
import time
import traceback
from fractions import Fraction

# ===========================================================================
# Конфигурация / утилиты
# ===========================================================================

STORAGES = ["array", "jagged"]
INT_TYPES = ["int", "longlong"]
FLOAT_TYPES = ["float", "double"]

DEFAULT_CLI_CANDIDATES = [
    os.environ.get("MATRIX_CLI"),
    os.path.join(os.path.dirname(__file__), "..", "build", "matrix_cli"),
    os.path.join(os.path.dirname(__file__), "build", "matrix_cli"),
    "./build/matrix_cli",
]


# ---- ANSI-цвета -----------------------------------------------------------

class Color:
    enabled = True
    RESET  = "\033[0m"
    BOLD   = "\033[1m"
    RED    = "\033[31m"
    GREEN  = "\033[32m"
    YELLOW = "\033[33m"
    BLUE   = "\033[34m"
    CYAN   = "\033[36m"
    GRAY   = "\033[90m"

    @classmethod
    def wrap(cls, code: str, s: str) -> str:
        return f"{code}{s}{cls.RESET}" if cls.enabled else s

    @classmethod
    def ok(cls, s):    return cls.wrap(cls.GREEN,  s)
    @classmethod
    def fail(cls, s):  return cls.wrap(cls.RED,    s)
    @classmethod
    def warn(cls, s):  return cls.wrap(cls.YELLOW, s)
    @classmethod
    def info(cls, s):  return cls.wrap(cls.CYAN,   s)
    @classmethod
    def dim(cls, s):   return cls.wrap(cls.GRAY,   s)
    @classmethod
    def bold(cls, s):  return cls.wrap(cls.BOLD,   s)


# ---- Реестр тестов --------------------------------------------------------

TESTS: list[tuple[str, callable]] = []
CLI_PATH: str = ""


def test(fn):
    """Декоратор: регистрирует функцию как тест."""
    TESTS.append((fn.__name__, fn))
    return fn


class TestFailure(AssertionError):
    """Явный провал теста с сообщением."""
    pass


def check(cond: bool, msg: str = ""):
    if not cond:
        raise TestFailure(msg or "assertion failed")


def check_eq(a, b, msg: str = ""):
    if a != b:
        raise TestFailure(f"{msg}\n    expected: {b!r}\n    got:      {a!r}")


def check_close(a: float, b: float, rel: float = 1e-9, abs_: float = 1e-12, msg: str = ""):
    if not (math.isfinite(a) and math.isfinite(b)) or not math.isclose(a, b, rel_tol=rel, abs_tol=abs_):
        raise TestFailure(f"{msg}\n    expected: {b!r}\n    got:      {a!r}\n    rel={rel} abs={abs_}")


# ---- Запуск CLI -----------------------------------------------------------

def run_cli(storage: str, typ: str, n: int, values) -> str:
    inp = "\n".join(str(v) for v in values)
    try:
        proc = subprocess.run(
            [CLI_PATH, storage, typ, str(n)],
            input=inp,
            capture_output=True,
            text=True,
            timeout=20,
        )
    except subprocess.TimeoutExpired:
        raise TestFailure(f"CLI timeout: {CLI_PATH} {storage} {typ} {n}")
    if proc.returncode != 0:
        raise TestFailure(
            f"CLI exited with code {proc.returncode}\n"
            f"    cmd:    {CLI_PATH} {storage} {typ} {n}\n"
            f"    stderr: {proc.stderr.strip()!r}"
        )
    return proc.stdout.strip()


def run_cli_expect_failure(storage: str, typ: str, n: int, values) -> subprocess.CompletedProcess:
    inp = "\n".join(str(v) for v in values)
    proc = subprocess.run(
        [CLI_PATH, storage, typ, str(n)],
        input=inp, capture_output=True, text=True, timeout=20,
    )
    if proc.returncode == 0:
        raise TestFailure(
            f"expected failure but CLI succeeded:\n"
            f"    cmd:    {CLI_PATH} {storage} {typ} {n}\n"
            f"    stdout: {proc.stdout.strip()!r}"
        )
    return proc


# ---- Эталонные определители ----------------------------------------------

def det_exact_int(rows) -> int:
    """Точный определитель целочисленной матрицы через Fraction."""
    n = len(rows)
    if n == 0:
        return 1
    a = [[Fraction(x) for x in row] for row in rows]
    det = Fraction(1)
    for i in range(n):
        pivot = None
        for j in range(i, n):
            if a[j][i] != 0:
                pivot = j
                break
        if pivot is None:
            return 0
        if pivot != i:
            a[i], a[pivot] = a[pivot], a[i]
            det = -det
        det *= a[i][i]
        for j in range(i + 1, n):
            f = a[j][i] / a[i][i]
            for k in range(i, n):
                a[j][k] -= f * a[i][k]
    assert det.denominator == 1
    return int(det)


def det_float_ref(rows) -> float:
    """Опорный определитель на float (Гаусс с частичным выбором)."""
    n = len(rows)
    if n == 0:
        return 1.0
    a = [[float(x) for x in row] for row in rows]
    det = 1.0
    for i in range(n):
        pivot = max(range(i, n), key=lambda j: abs(a[j][i]))
        if a[pivot][i] == 0.0:
            return 0.0
        if pivot != i:
            a[i], a[pivot] = a[pivot], a[i]
            det = -det
        det *= a[i][i]
        for j in range(i + 1, n):
            f = a[j][i] / a[i][i]
            for k in range(i, n):
                a[j][k] -= f * a[i][k]
    return det


def flat(rows):
    return [x for row in rows for x in row]


# ===========================================================================
#                               НАБОР ТЕСТОВ
# ===========================================================================

# ---- Базовые --------------------------------------------------------------

@test
def empty_matrix():
    for storage in STORAGES:
        for typ in INT_TYPES:
            got = int(run_cli(storage, typ, 0, []))
            check_eq(got, 1, f"empty {storage}/{typ}")


@test
def one_by_one():
    vals = [0, 1, -1, 42, -17, 1_000_000, -999_999]
    for storage in STORAGES:
        for typ in INT_TYPES:
            for v in vals:
                got = int(run_cli(storage, typ, 1, [v]))
                check_eq(got, v, f"1x1 {storage}/{typ} value={v}")


@test
def two_by_two_known():
    cases = [
        ([1, 2, 3, 4], -2),
        ([2, 0, 0, 3], 6),
        ([1, 2, 2, 4], 0),
        ([0, 1, 1, 0], -1),
    ]
    for storage in STORAGES:
        for vals, expected in cases:
            got = int(run_cli(storage, "int", 2, vals))
            check_eq(got, expected, f"2x2 {storage} vals={vals}")


@test
def three_by_three_known():
    rows = [[1, 2, 3], [4, 5, 6], [7, 8, 10]]
    for storage in STORAGES:
        got = int(run_cli(storage, "int", 3, flat(rows)))
        check_eq(got, -3, f"3x3 {storage}")


@test
def identity_matrices():
    for n in range(1, 7):
        vals = [1 if i == j else 0 for i in range(n) for j in range(n)]
        for storage in STORAGES:
            got = int(run_cli(storage, "int", n, vals))
            check_eq(got, 1, f"identity {n}x{n} {storage}")


@test
def diagonal_matrices():
    for diag in ([2], [2, 3], [2, 3, 4], [11, 13, 17, 19, 23]):
        n = len(diag)
        vals = [diag[i] if i == j else 0 for i in range(n) for j in range(n)]
        expected = 1
        for d in diag:
            expected *= d
        for storage in STORAGES:
            got = int(run_cli(storage, "int", n, vals))
            check_eq(got, expected, f"diag {diag} {storage}")


@test
def triangular_matrices():
    upper = [[1, 2, 3, 4],
             [0, 5, 6, 7],
             [0, 0, 8, 9],
             [0, 0, 0, 10]]
    lower = [[2, 0, 0],
             [5, 3, 0],
             [7, 8, 4]]
    for storage in STORAGES:
        check_eq(int(run_cli(storage, "int", 4, flat(upper))), 400, f"upper {storage}")
        check_eq(int(run_cli(storage, "int", 3, flat(lower))),  24, f"lower {storage}")


@test
def pivot_needed():
    rows = [[0, 1, 0, 0],
            [1, 0, 0, 0],
            [0, 0, 2, 0],
            [0, 0, 0, 3]]
    for storage in STORAGES:
        got = int(run_cli(storage, "int", 4, flat(rows)))
        check_eq(got, -6, f"pivot {storage}")


@test
def singular_matrices():
    # Матрица ранга 1: a[i][j] = (i+1)*(j+1).
    # Все строки пропорциональны [1, 2, ..., n], det == 0 при любом n >= 2.
    # (Прежний вариант a[i][j] = i+j при n=2 давал [[0,1],[1,2]] с det=-1.)
    for n in range(2, 6):
        rows = [[(i + 1) * (j + 1) for j in range(n)] for i in range(n)]
        for storage in STORAGES:
            got = int(run_cli(storage, "int", n, flat(rows)))
            check_eq(got, 0, f"singular {n}x{n} {storage} rows={rows}")


# ---- Рандомизированные целочисленные --------------------------------------

@test
def random_integer_exact():
    for storage in STORAGES:
        for typ in INT_TYPES:
            for n in (1, 2, 3, 4, 5):
                rng = random.Random(1000 * n + (1 if storage == "array" else 2))
                for _ in range(40):
                    rows = [[rng.randint(-20, 20) for _ in range(n)] for _ in range(n)]
                    expected = det_exact_int(rows)
                    got = int(run_cli(storage, typ, n, flat(rows)))
                    check_eq(got, expected, f"random int {storage}/{typ} n={n} rows={rows}")


@test
def random_large_entries():
    rng = random.Random(777)
    n = 4
    for storage in STORAGES:
        for _ in range(15):
            rows = [[rng.randint(-10_000, 10_000) for _ in range(n)] for _ in range(n)]
            expected = det_exact_int(rows)
            got = int(run_cli(storage, "longlong", n, flat(rows)))
            check_eq(got, expected, f"big entries {storage} rows={rows}")


# ---- Плавающая запятая ----------------------------------------------------

@test
def random_floating():
    for storage in STORAGES:
        for typ in FLOAT_TYPES:
            for n in (1, 2, 3, 4, 5):
                rng = random.Random(2000 * n + (1 if storage == "array" else 2))
                for _ in range(25):
                    rows = [[rng.uniform(-5.0, 5.0) for _ in range(n)] for _ in range(n)]
                    expected = det_float_ref(rows)
                    got = float(run_cli(storage, typ, n, flat(rows)))

                    # float: ~7 значащих цифр, ошибка накапливается за O(n^3) операций.
                    # double: практически машинная точность.
                    if typ == "float":
                        rel, abs_ = 1e-3, 1e-6
                    else:
                        rel, abs_ = 1e-9, 1e-12

                    check_close(
                        got, expected, rel=rel, abs_=abs_,
                        msg=f"random float {storage}/{typ} n={n} rows={rows}",
                    )
@test
def scaled_identity():
    for storage in STORAGES:
        for typ in FLOAT_TYPES:
            for k in (0.5, 1.0, 2.5, -3.0):
                for n in (2, 3, 4):
                    vals = [k if i == j else 0.0 for i in range(n) for j in range(n)]
                    expected = k ** n
                    got = float(run_cli(storage, typ, n, vals))
                    check_close(got, expected, rel=1e-6, abs_=1e-12,
                                msg=f"scaled I {storage}/{typ} k={k} n={n}")


@test
def floating_singular():
    rows = [[1.0, 2.0, 3.0],
            [2.0, 4.0, 6.0],
            [1.0, 1.0, 1.0]]
    for storage in STORAGES:
        got = float(run_cli(storage, "double", 3, flat(rows)))
        check(abs(got) < 1e-9, f"floating singular {storage}: got {got}")


# ---- Согласованность хранилищ --------------------------------------------

@test
def storage_consistency_int():
    for typ in INT_TYPES:
        for n in (1, 2, 3, 4, 5, 6):
            rng = random.Random(n * 31)
            for _ in range(10):
                rows = [[rng.randint(-15, 15) for _ in range(n)] for _ in range(n)]
                a = run_cli("array",  typ, n, flat(rows))
                j = run_cli("jagged", typ, n, flat(rows))
                check_eq(a, j, f"consistency {typ} n={n}")


@test
def storage_consistency_double():
    for n in (2, 3, 4, 5):
        rng = random.Random(n * 41)
        for _ in range(10):
            rows = [[rng.uniform(-1.0, 1.0) for _ in range(n)] for _ in range(n)]
            a = float(run_cli("array",  "double", n, flat(rows)))
            j = float(run_cli("jagged", "double", n, flat(rows)))
            check_close(a, j, rel=1e-9, abs_=1e-12,
                        msg=f"consistency double n={n}")


# ---- Ошибки ---------------------------------------------------------------

@test
def ctor_size_mismatch():
    for storage in STORAGES:
        proc = run_cli_expect_failure(storage, "int", 2, [1, 2, 3])
        err = (proc.stderr + proc.stdout).lower()
        check("mismatch" in err or "error" in err or "invalid" in err,
              f"expected error message, got: {proc.stderr!r}")


@test
def bad_type():
    for storage in STORAGES:
        run_cli_expect_failure(storage, "quux", 1, [0])


@test
def bad_storage():
    run_cli_expect_failure("quux", "int", 1, [0])


@test
def invalid_dimensions():
    for n in ("-1", "2junk", "", "1.5", "18446744073709551616", "18446744073709551615"):
        proc = run_cli_expect_failure("array", "int", n, [])
        check(proc.returncode > 0, "must reject input without crashing")

@test
def missing_arguments():
    proc = subprocess.run([CLI_PATH], capture_output=True, text=True, timeout=20)
    check(proc.returncode > 0, "missing arguments must fail")
    check("usage" in proc.stderr, "usage must be printed")

@test
def result_overflow():
    for storage in STORAGES:
        proc = run_cli_expect_failure(storage, "int", 2, [2147483647, 0, 0, 2])
        check("overflow" in proc.stderr or "fit" in proc.stderr, "overflow diagnostic")

# ===========================================================================
#                               РАННЕР
# ===========================================================================

def find_cli(explicit: str | None) -> str:
    if explicit:
        return explicit
    for c in DEFAULT_CLI_CANDIDATES:
        if not c:
            continue
        if os.path.exists(c):
            return c
    return ""


def print_header():
    print(Color.bold("=" * 72))
    print(Color.bold("  Matrix library E2E tests"))
    print(Color.bold("=" * 72))
    print(f"  CLI:      {Color.info(CLI_PATH)}")
    print(f"  storages: {', '.join(STORAGES)}")
    print(f"  types:    {', '.join(INT_TYPES + FLOAT_TYPES)}")
    print(f"  tests:    {len(TESTS)}")
    print(Color.bold("=" * 72))
    print()


def run_all(verbose: bool, filter_substr: str | None):
    selected = TESTS
    if filter_substr:
        selected = [(n, f) for n, f in TESTS if filter_substr in n]
        if not selected:
            print(Color.warn(f"no tests match filter {filter_substr!r}"))
            return 0, 0, 0

    passed = 0
    failed = 0
    failures: list[tuple[str, str]] = []
    t_start = time.perf_counter()

    for name, fn in selected:
        line_start = f"{Color.dim('  RUN')} {name:<40}"
        sys.stdout.write(line_start)
        sys.stdout.flush()
        t0 = time.perf_counter()
        try:
            fn()
            dt = (time.perf_counter() - t0) * 1000
            passed += 1
            sys.stdout.write(f" {Color.ok('OK')}  {Color.dim(f'{dt:6.1f} ms')}\n")
        except Exception as e:
            dt = (time.perf_counter() - t0) * 1000
            failed += 1
            sys.stdout.write(f" {Color.fail('FAIL')} {Color.dim(f'{dt:6.1f} ms')}\n")
            tb = traceback.format_exc()
            failures.append((name, tb))
            if verbose:
                print(Color.fail("    " + "-" * 68))
                for ln in (tb.rstrip().splitlines()):
                    print(Color.fail(f"    {ln}"))
                print(Color.fail("    " + "-" * 68))
            else:
                # короткая однострочная причина
                msg = str(e).splitlines()
                if msg:
                    print(Color.fail(f"      {msg[0]}"))

    total_dt = (time.perf_counter() - t_start) * 1000
    print()
    print(Color.bold("=" * 72))
    print(f"  Total: {len(selected)}   "
          f"{Color.ok(f'passed: {passed}')}   "
          f"{Color.fail(f'failed: {failed}')}   "
          f"time: {total_dt:.1f} ms")
    print(Color.bold("=" * 72))

    if failures:
        print()
        print(Color.fail(Color.bold("  Failed tests:")))
        for name, _ in failures:
            print(Color.fail(f"    - {name}"))
        if not verbose:
            print()
            print(Color.dim("  Re-run with -v to see full tracebacks of failures."))

    return passed, failed, len(selected)


def main(argv=None):
    parser = argparse.ArgumentParser(description="E2E tests for Matrix library")
    parser.add_argument("--cli", default=None, help="path to matrix_cli binary")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="print full tracebacks of failures")
    parser.add_argument("-k", "--filter", default=None,
                        help="run only tests whose name contains this substring")
    parser.add_argument("--no-color", action="store_true", help="disable ANSI colors")
    parser.add_argument("--list", action="store_true", help="list tests and exit")
    args = parser.parse_args(argv)

    if args.no_color or not sys.stdout.isatty():
        Color.enabled = False

    global CLI_PATH
    CLI_PATH = find_cli(args.cli)

    if args.list:
        for name, _ in TESTS:
            print(name)
        return 0

    if not CLI_PATH or not os.path.exists(CLI_PATH):
        print(Color.fail("ERROR: matrix_cli binary not found."))
        print("Searched:")
        for c in DEFAULT_CLI_CANDIDATES:
            if c:
                print(f"  - {c}")
        print()
        print("Set MATRIX_CLI env var or pass --cli PATH.")
        return 2

    print_header()
    _, failed, _ = run_all(verbose=args.verbose, filter_substr=args.filter)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())