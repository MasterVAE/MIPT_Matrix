#pragma once

#include <cstddef>
#include <vector>
#include <utility>
#include <stdexcept>
#include <type_traits>

namespace Matrix
{

namespace Detail {

// Широкий тип для числителя в дробно-свободном исключении (Bareiss).
// int        -> long long     (гарантированно хватит для всех промежуточных)
// long long  -> __int128      (числитель может достигать ~1e36)
// Остальные целые типы маппятся аналогично по размеру.
#if defined(__SIZEOF_INT128__)
using WideInt128 = __int128;
#else
using WideInt128 = long long;   // запасной вариант (MSVC и т.п.)
#endif

template <typename T>
struct WideInteger {
    using type = std::conditional_t<
        (sizeof(T) < sizeof(long long)),
        long long,
        WideInt128
    >;
};

template <typename T>
using WideInteger_t = typename WideInteger<T>::type;

} // namespace Detail

template <typename T, std::size_t N>
class IMatrix {
public:
    constexpr virtual ~IMatrix() = default;

    constexpr virtual T& At(std::size_t row, std::size_t col) = 0;
    constexpr virtual const T& At(std::size_t row, std::size_t col) const = 0;
    constexpr virtual std::size_t Size() const = 0;
    constexpr virtual T Determinant() const = 0;
};

template <typename T, std::size_t N>
class DeterminantProxy;

template <std::integral T, std::size_t N>
class DeterminantProxy<T, N> {
public:
    static constexpr T Calculate(const IMatrix<T, N>& matrix) {
        if constexpr (N == 0) {
            return T{1};
        } else {
            using Wide = detail::WideInteger_t<T>;

            std::array<std::array<T, N>, N> a{};
            for (std::size_t i = 0; i < N; ++i) {
                for (std::size_t j = 0; j < N; ++j) {
                    a[i][j] = matrix.At(i, j);
                }
            }

            T sign = T{1};
            Wide prev = Wide{1};

            for (std::size_t k = 0; k + 1 < N; ++k) {
                if (a[k][k] == T{0}) {
                    std::size_t pivot = k + 1;
                    while (pivot < N && a[pivot][k] == T{0}) {
                        ++pivot;
                    }
                    if (pivot == N) {
                        return T{0};
                    }
                    std::swap(a[k], a[pivot]);
                    sign = -sign;
                }

                const Wide pivot_val = static_cast<Wide>(a[k][k]);

                for (std::size_t i = k + 1; i < N; ++i) {
                    for (std::size_t j = k + 1; j < N; ++j) {
                        const Wide num = static_cast<Wide>(a[i][j]) * pivot_val
                                       - static_cast<Wide>(a[i][k]) * static_cast<Wide>(a[k][j]);
                        a[i][j] = static_cast<T>(num / prev);
                    }
                }
                prev = pivot_val;
            }

            return sign * a[N - 1][N - 1];
        }
    }
};

template <std::floating_point T, std::size_t N>
class DeterminantProxy<T, N> {
public:
    static constexpr T Calculate(const IMatrix<T, N>& matrix) {
        if constexpr (N == 0) {
            return T{1};
        } else {
            std::array<std::array<T, N>, N> m{};
            for (std::size_t i = 0; i < N; ++i) {
                for (std::size_t j = 0; j < N; ++j) {
                    m[i][j] = matrix.At(i, j);
                }
            }

            T det = T{1};

            for (std::size_t i = 0; i < N; ++i) {
                std::size_t pivot = i;
                T max_val = Abs(m[i][i]);
                for (std::size_t j = i + 1; j < N; ++j) {
                    if (const T val = Abs(m[j][i]); val > max_val) {
                        max_val = val;
                        pivot = j;
                    }
                }

                if (max_val == T{0}) {
                    return T{0};
                }

                if (pivot != i) {
                    std::swap(m[i], m[pivot]);
                    det = -det;
                }

                det *= m[i][i];

                for (std::size_t j = i + 1; j < N; ++j) {
                    const T factor = m[j][i] / m[i][i];
                    for (std::size_t k = i; k < N; ++k) {
                        m[j][k] -= factor * m[i][k];
                    }
                }
            }

            return det;
        }
    }

private:
    static constexpr T Abs(T val) {
        return val < T{0} ? -val : val;
    }
};

template <typename T, std::size_t N>
class Matrix : public IMatrix<T, N> {
public:
    constexpr Matrix() = default;
    constexpr ~Matrix() override = default;

    constexpr std::size_t Size() const override {
        return N;
    }

    constexpr T Determinant() const override {
        return DeterminantProxy<T, N>::Calculate(*this);
    }
};

template <typename T, std::size_t N>
class ArrayMatrix final : public Matrix<T, N> {
private:
    std::array<T, N * N> data_{};

public:
    constexpr ArrayMatrix() = default;

    constexpr explicit ArrayMatrix(const std::array<T, N * N>& values) noexcept
        : data_(values) {}

    constexpr T& At(std::size_t row, std::size_t col) override {
        return data_[row * N + col];
    }

    constexpr const T& At(std::size_t row, std::size_t col) const override {
        return data_[row * N + col];
    }
};

template <typename T, std::size_t N>
class JaggedMatrix final : public Matrix<T, N> {
private:
    std::array<std::array<T, N>, N> data_{};

public:
    constexpr JaggedMatrix() = default;

    constexpr explicit JaggedMatrix(const std::array<T, N * N>& values) {
        for (std::size_t i = 0; i < N; ++i) {
            for (std::size_t j = 0; j < N; ++j) {
                data_[i][j] = values[i * N + j];
            }
        }
    }

    constexpr JaggedMatrix(const std::array<std::array<T, N>, N>& values) noexcept
        : data_(values) {}

    constexpr T& At(std::size_t row, std::size_t col) override {
        return data_[row][col];
    }

    constexpr const T& At(std::size_t row, std::size_t col) const override {
        return data_[row][col];
    }
};

} // namespace Matrix