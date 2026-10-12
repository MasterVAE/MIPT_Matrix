#include <gtest/gtest.h>
#include "matrix.hpp"

#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <random>
#include <stdexcept>
#include <vector>

using Matrix::ArrayMatrix;
using Matrix::JaggedMatrix;

namespace {

// ---------------------------------------------------------------------------
// Вспомогательные функции
// ---------------------------------------------------------------------------

template <typename T>
::testing::AssertionResult NearEq(T a, T b, double eps = 1e-6) {
    const double da = static_cast<double>(a);
    const double db = static_cast<double>(b);
    const double scale = std::max(1.0, std::max(std::fabs(da), std::fabs(db)));
    if (std::fabs(da - db) <= eps * scale) return ::testing::AssertionSuccess();
    return ::testing::AssertionFailure() << "expected " << b << ", got " << a;
}

// ---------------------------------------------------------------------------
// Общие проверки для ArrayMatrix и JaggedMatrix
//
// ВАЖНО (инварианты тестов):
//   * все ОЖИДАЕМЫЕ определители гарантированно помещаются в проверяемый тип;
//   * матрицы подобраны так, что и промежуточные значения корректной
//     реализации (Bareiss с точным делением на предыдущий пивот) тоже
//     помещаются в проверяемый тип.
// ---------------------------------------------------------------------------

// ---- int ------------------------------------------------------------------

template <template <typename> class M>
void testCommonInt() {
    // Пустая матрица -> 1 по соглашению
    EXPECT_EQ(M<int>(0).Determinant(), 1);

    // 1x1
    EXPECT_EQ(M<int>(1, { 5}).Determinant(),   5);
    EXPECT_EQ(M<int>(1, {-5}).Determinant(),  -5);
    EXPECT_EQ(M<int>(1, { 0}).Determinant(),   0);
    EXPECT_EQ(M<int>(1, {42}).Determinant(),  42);

    // 2x2
    EXPECT_EQ(M<int>(2, {1, 2, 3, 4}).Determinant(), -2);
    EXPECT_EQ(M<int>(2, {2, 0, 0, 3}).Determinant(),  6);
    EXPECT_EQ(M<int>(2, {1, 2, 2, 4}).Determinant(),  0);   // singular
    EXPECT_EQ(M<int>(2, {0, 1, 1, 0}).Determinant(), -1);   // pivot needed
    EXPECT_EQ(M<int>(2, {0, 0, 0, 0}).Determinant(),  0);

    // 3x3 известное значение: det = -3
    EXPECT_EQ(M<int>(3, {1, 2, 3,
                         4, 5, 6,
                         7, 8, 10}).Determinant(), -3);

    // 3x3 единичная
    EXPECT_EQ(M<int>(3, {1, 0, 0,
                         0, 1, 0,
                         0, 0, 1}).Determinant(), 1);

    // 3x3 верхнетреугольная: 2 * 3 * 6 = 36
    EXPECT_EQ(M<int>(3, {2, 7, 5,
                         0, 3, 4,
                         0, 0, 6}).Determinant(), 36);

    // 3x3, требующая перестановки строк (нуль на диагонали):
    //   det = 0·(0·0−4·6) − 2·(1·0−4·5) + 3·(1·6−0·5) = 40 + 18 = 58
    EXPECT_EQ(M<int>(3, {0, 2, 3,
                         1, 0, 4,
                         5, 6, 0}).Determinant(), 58);

    // 4x4 Vandermonde (x = 2,3,4,5):
    //   det = ∏_{i<j}(x_j − x_i) = 1·2·3·1·2·1 = 12
    EXPECT_EQ(M<int>(4, {1, 1,  1,   1,
                         2, 3,  4,   5,
                         4, 9, 16,  25,
                         8, 27, 64, 125}).Determinant(), 12);

    // 4x4 верхнетреугольная: 1·5·8·10 = 400
    EXPECT_EQ(M<int>(4, {1, 2, 3, 4,
                         0, 5, 6, 7,
                         0, 0, 8, 9,
                         0, 0, 0, 10}).Determinant(), 400);

    // 4x4 с перестановкой: block-diag([[0,1],[1,0]], [2], [3]) => -1·2·3 = -6
    EXPECT_EQ(M<int>(4, {0, 1, 0, 0,
                         1, 0, 0, 0,
                         0, 0, 2, 0,
                         0, 0, 0, 3}).Determinant(), -6);

    // 4x4 с линейно-зависимыми строками (строка i = (j+i)) -> 0
    EXPECT_EQ(M<int>(4, {1, 2, 3, 4,
                         2, 3, 4, 5,
                         3, 4, 5, 6,
                         4, 5, 6, 7}).Determinant(), 0);

    // 5x5 диагональная: 11·13·17·19·23 = 1_062_347 (помещается в int).
    // Корректный Bareiss держит промежуточные ≤ 1_062_347.
    EXPECT_EQ(M<int>(5, {11, 0,  0,  0,  0,
                          0,13,  0,  0,  0,
                          0, 0, 17,  0,  0,
                          0, 0,  0, 19,  0,
                          0, 0,  0,  0, 23}).Determinant(), 1062347);

    // Отрицательные значения: det = -det(положительный аналог) = 3
    EXPECT_EQ(M<int>(3, {-1, -2, -3,
                         -4, -5, -6,
                         -7, -8, -10}).Determinant(), 3);

    // Изменение через At()
    M<int> m(2);
    m.At(0, 0) = 3;
    m.At(1, 1) = 7;
    EXPECT_EQ(m.Determinant(), 21);
}

// ---- long long ------------------------------------------------------------
//
// Все ожидания и промежуточные (при корректном Bareiss) укладываются
// в long long (гарантированный максимум ~9.22e18).

template <template <typename> class M>
void testCommonLongLong() {
    EXPECT_EQ(M<long long>(0).Determinant(), 1LL);
    EXPECT_EQ(M<long long>(1, {1234567890123LL}).Determinant(), 1234567890123LL);

    // 4x4 diag(1e5, 1e4, 1e3, 1e2): det = 1e14 — влезает в long long
    M<long long> m(4, {100000, 0,     0,    0,
                       0,      10000, 0,    0,
                       0,      0,     1000, 0,
                       0,      0,     0,    100});
    EXPECT_EQ(m.Determinant(), 100000LL * 10000 * 1000 * 100);
}

// ---- float / double -------------------------------------------------------

template <template <typename> class M>
void testCommonFloating() {
    // Пустая
    EXPECT_DOUBLE_EQ(static_cast<double>(M<double>(0).Determinant()), 1.0);

    // 1x1
    EXPECT_DOUBLE_EQ(M<double>(1, {2.5}).Determinant(),   2.5);
    EXPECT_DOUBLE_EQ(M<double>(1, {-3.25}).Determinant(), -3.25);

    // 2x2: 1·4 − 2·3 = -2
    EXPECT_DOUBLE_EQ(M<double>(2, {1.0, 2.0,
                                   3.0, 4.0}).Determinant(), -2.0);

    // Единичная 5x5
    {
        std::vector<double> v(25, 0.0);
        for (int i = 0; i < 5; ++i) v[i * 5 + i] = 1.0;
        EXPECT_DOUBLE_EQ(M<double>(5, v).Determinant(), 1.0);
    }

    // Диагональная 4x4: 2·3·4·5 = 120
    EXPECT_TRUE(NearEq(M<double>(4, {2.0, 0,   0,   0,
                                     0,   3.0, 0,   0,
                                     0,   0,   4.0, 0,
                                     0,   0,   0,   5.0}).Determinant(), 120.0));

    // Известное 3x3: det = -3
    EXPECT_TRUE(NearEq(M<double>(3, {1.0, 2.0, 3.0,
                                     4.0, 5.0, 6.0,
                                     7.0, 8.0, 10.0}).Determinant(), -3.0));

    // Сингулярная (3-я строка = комбинация первых двух)
    EXPECT_TRUE(NearEq(M<double>(3, {1.0, 2.0, 3.0,
                                     2.0, 4.0, 6.0,
                                     1.0, 1.0, 1.0}).Determinant(), 0.0));

    // Требуется перестановка: det = 58 (см. целочисленный аналог)
    EXPECT_TRUE(NearEq(M<double>(3, {0.0, 2.0, 3.0,
                                     1.0, 0.0, 4.0,
                                     5.0, 6.0, 0.0}).Determinant(), 58.0));

    // Скалярная: k·I_n, det = k^n
    {
        constexpr int n = 4;
        std::vector<double> v(n * n, 0.0);
        for (int i = 0; i < n; ++i) v[i * n + i] = 1.5;
        EXPECT_TRUE(NearEq(M<double>(n, v).Determinant(),
                           std::pow(1.5, n), 1e-9));
    }

    // float
    EXPECT_TRUE(NearEq(M<float>(3, {1.f, 2.f, 3.f,
                                    4.f, 5.f, 6.f,
                                    7.f, 8.f, 10.f}).Determinant(), -3.f, 1e-4f));
}

// ---- Исключения в конструкторе --------------------------------------------

template <template <typename> class M>
void testConstructorExceptions() {
    EXPECT_THROW(M<int>(2, {1, 2, 3}),     std::invalid_argument);
    EXPECT_THROW(M<int>(3, {1, 2}),        std::invalid_argument);
    EXPECT_THROW(M<int>(0, {1}),           std::invalid_argument);
    EXPECT_THROW(M<double>(2, {1.0, 2.0}), std::invalid_argument);
}

// ---- Согласованность Array vs Jagged на одних данных ----------------------

template <typename T>
void testArrayVsJaggedConsistency(const std::vector<std::vector<T>>& rows) {
    const std::size_t n = rows.size();
    std::vector<T> flat;
    flat.reserve(n * n);
    for (const auto& r : rows)
        for (auto x : r) flat.push_back(x);

    ArrayMatrix<T>  a(n, flat);
    JaggedMatrix<T> j(n, flat);

    EXPECT_EQ(static_cast<long double>(a.Determinant()),
              static_cast<long double>(j.Determinant()));
}

// ===========================================================================
//                                        Наборы тестов для ArrayMatrix
// ===========================================================================

TEST(ArrayMatrixInt,        All) { testCommonInt<ArrayMatrix>(); }
TEST(ArrayMatrixLongLong,   All) { testCommonLongLong<ArrayMatrix>(); }
TEST(ArrayMatrixFloating,   All) { testCommonFloating<ArrayMatrix>(); }
TEST(ArrayMatrixCtorThrows, All) { testConstructorExceptions<ArrayMatrix>(); }

TEST(ArrayMatrix, AtReturnsReference) {
    ArrayMatrix<int> m(2);
    int& ref = m.At(1, 1);
    ref = 99;
    EXPECT_EQ(m.At(1, 1), 99);
}

TEST(ArrayMatrix, AtLayoutRowMajor) {
    ArrayMatrix<int> m(3, {1, 2, 3,
                           4, 5, 6,
                           7, 8, 9});
    EXPECT_EQ(m.At(0, 0), 1);
    EXPECT_EQ(m.At(0, 2), 3);
    EXPECT_EQ(m.At(1, 0), 4);
    EXPECT_EQ(m.At(2, 2), 9);
}

TEST(ArrayMatrix, NegativeDeterminantSign) {
    ArrayMatrix<int> a(2, {1, 0, 0, 1});
    ArrayMatrix<int> b(2, {0, 1, 1, 0});
    EXPECT_EQ(a.Determinant(),  1);
    EXPECT_EQ(b.Determinant(), -1);
}

// Точный тест на большие целые.
// ВНИМАНИЕ: ожидание гарантированно помещается в long long.
//   a*b       ≈ 1.00004e12 < 9.22e18
//   a*b*c     ≈ 1.00007e18 < 9.22e18
TEST(ArrayMatrix, LargeIntegerExactness) {
    constexpr long long a = 1000003LL;
    constexpr long long b = 1000033LL;
    constexpr long long c = 1000037LL;

    // sanity: эталон не переполняется на этапе компиляции
    static_assert(a * b > 0, "a*b should fit in long long");
    static_assert(a * b * c > 0, "a*b*c should fit in long long");

    ArrayMatrix<long long> m(3, {a, 0, 0,
                                 0, b, 0,
                                 0, 0, c});

    EXPECT_EQ(m.Determinant(), a * b * c);
}

// ===========================================================================
//                                        Наборы тестов для JaggedMatrix
// ===========================================================================

TEST(JaggedMatrixInt,        All) { testCommonInt<JaggedMatrix>(); }
TEST(JaggedMatrixLongLong,   All) { testCommonLongLong<JaggedMatrix>(); }
TEST(JaggedMatrixFloating,   All) { testCommonFloating<JaggedMatrix>(); }
TEST(JaggedMatrixCtorThrows, All) { testConstructorExceptions<JaggedMatrix>(); }

TEST(JaggedMatrix, AtReturnsReference) {
    JaggedMatrix<double> m(2);
    m.At(0, 0) = 1.0;
    m.At(1, 1) = 2.0;
    EXPECT_DOUBLE_EQ(m.Determinant(), 2.0);
}

// ===========================================================================
//                 Параметризованный вариант: одни тесты на оба класса
// ===========================================================================

class StorageVariants :
    public ::testing::TestWithParam<std::function<std::unique_ptr<Matrix::Matrix<int>>(
        std::size_t, const std::vector<int>&)>> {};

TEST_P(StorageVariants, RandomSmallMatricesMatchReference) {
    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> dist(-5, 5);

    for (int iter = 0; iter < 200; ++iter) {
        const std::size_t n = 1 + (rng() % 4);  // 1..4
        std::vector<int> v(n * n);
        for (auto& x : v) x = dist(rng);

        auto m = GetParam()(n, v);
        const int det = m->Determinant();

        std::function<int(std::vector<std::vector<int>>&)> ref =
            [&](std::vector<std::vector<int>>& a) -> int {
                const std::size_t k = a.size();
                if (k == 0) return 1;
                if (k == 1) return a[0][0];
                if (k == 2) return a[0][0] * a[1][1] - a[0][1] * a[1][0];
                int s = 0;
                int sign = 1;
                for (std::size_t j = 0; j < k; ++j) {
                    std::vector<std::vector<int>> sub(k - 1, std::vector<int>(k - 1));
                    for (std::size_t r = 1; r < k; ++r) {
                        std::size_t cc = 0;
                        for (std::size_t c = 0; c < k; ++c) {
                            if (c == j) continue;
                            sub[r - 1][cc++] = a[r][c];
                        }
                    }
                    s += sign * a[0][j] * ref(sub);
                    sign = -sign;
                }
                return s;
            };

        std::vector<std::vector<int>> a(n, std::vector<int>(n));
        for (std::size_t i = 0; i < n; ++i)
            for (std::size_t j = 0; j < n; ++j)
                a[i][j] = v[i * n + j];

        EXPECT_EQ(det, ref(a)) << "n = " << n;
    }
}

static std::unique_ptr<Matrix::Matrix<int>>
makeArray(std::size_t n, const std::vector<int>& v) {
    return std::make_unique<ArrayMatrix<int>>(n, v);
}
static std::unique_ptr<Matrix::Matrix<int>>
makeJagged(std::size_t n, const std::vector<int>& v) {
    return std::make_unique<JaggedMatrix<int>>(n, v);
}

INSTANTIATE_TEST_SUITE_P(
    Storages, StorageVariants,
    ::testing::Values(makeArray, makeJagged));

} // namespace