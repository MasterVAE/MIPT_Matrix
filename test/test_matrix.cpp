#ifndef MATRIX_HPP_
#define MATRIX_HPP_

#include <algorithm>
#include <array>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace Matrix {

namespace detail {

template <typename T>
struct WideInteger {
    using type = T;
};

template <> struct WideInteger<std::int8_t>  { using type = std::int16_t; };
template <> struct WideInteger<std::int16_t> { using type = std::int32_t; };
template <> struct WideInteger<std::int32_t> { using type = std::int64_t; };
#ifdef __SIZEOF_INT128__
template <> struct WideInteger<std::int64_t> { using type = __int128_t; };
#endif
template <> struct WideInteger<long long> {
#ifdef __SIZEOF_INT128__
    using type = __int128_t;
#else
    using type = long double;
#endif
};

template <typename T>
using WideInteger_t = typename WideInteger<T>::type;

// Максимальный размер матриц для вычислений в буфере std::array без аллокаций
inline constexpr std::size_t kMaxStaticMatrixDim = 64;

} // namespace detail

// ============================================================================
// 1. Интерфейс IMatrix
// ============================================================================
template <typename T>
class IMatrix {
public:
    virtual ~IMatrix() = default;

    [[nodiscard]] virtual T& At(std::size_t row, std::size_t col) = 0;
    [[nodiscard]] virtual const T& At(std::size_t row, std::size_t col) const = 0;
    [[nodiscard]] virtual std::size_t Size() const noexcept = 0;
    [[nodiscard]] virtual T Determinant() const = 0;
};

// Предварительное объявление базового класса Matrix
template <typename T>
class Matrix;

// ============================================================================
// 2. Прокси-класс DeterminantProxy
// ============================================================================
template <typename T>
class DeterminantProxy;

// Специализация для целочисленных типов (Алгоритм Барейса на std::array)
template <std::integral T>
class DeterminantProxy<T> {
public:
    [[nodiscard]] static T Calculate(const IMatrix<T>& matrix) {
        const std::size_t n = matrix.Size();
        if (n == 0) return T{1};

        // Замена std::vector<std::vector<T>> на плоский std::array в стеке
        // Избавляет от динамических аллокаций при расчете определителя
        if (n <= detail::kMaxStaticMatrixDim) {
            std::array<T, detail::kMaxStaticMatrixDim * detail::kMaxStaticMatrixDim> a{};
            for (std::size_t i = 0; i < n; ++i) {
                for (std::size_t j = 0; j < n; ++j) {
                    a[i * n + j] = matrix.At(i, j);
                }
            }
            return ComputeBareiss(a.data(), n);
        }

        // Fallback на динамический плоский буфер для экстремально больших N
        std::vector<T> a(n * n);
        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t j = 0; j < n; ++j) {
                a[i * n + j] = matrix.At(i, j);
            }
        }
        return ComputeBareiss(a.data(), n);
    }

private:
    static constexpr T ComputeBareiss(T* a, std::size_t n) {
        using Wide = detail::WideInteger_t<T>;

        T sign = T{1};
        Wide prev = Wide{1};

        for (std::size_t k = 0; k + 1 < n; ++k) {
            if (a[k * n + k] == T{0}) {
                std::size_t pivot = k + 1;
                while (pivot < n && a[pivot * n + k] == T{0}) {
                    ++pivot;
                }
                if (pivot == n) {
                    return T{0};
                }

                // Перестановка строк без аллокаций
                for (std::size_t j = 0; j < n; ++j) {
                    std::swap(a[k * n + j], a[pivot * n + j]);
                }
                sign = -sign;
            }

            const Wide pivot_val = static_cast<Wide>(a[k * n + k]);

            for (std::size_t i = k + 1; i < n; ++i) {
                for (std::size_t j = k + 1; j < n; ++j) {
                    const Wide num = static_cast<Wide>(a[i * n + j]) * pivot_val
                                   - static_cast<Wide>(a[i * n + k]) * static_cast<Wide>(a[k * n + j]);
                    a[i * n + j] = static_cast<T>(num / prev);
                }
            }

            prev = pivot_val;
        }

        return sign * a[(n - 1) * n + (n - 1)];
    }
};

// Специализация для вещественных типов (Метод Гаусса на std::array)
template <std::floating_point T>
class DeterminantProxy<T> {
public:
    [[nodiscard]] static T Calculate(const IMatrix<T>& matrix) {
        const std::size_t n = matrix.Size();
        if (n == 0) return T{1};

        if (n <= detail::kMaxStaticMatrixDim) {
            std::array<T, detail::kMaxStaticMatrixDim * detail::kMaxStaticMatrixDim> m{};
            for (std::size_t i = 0; i < n; ++i) {
                for (std::size_t j = 0; j < n; ++j) {
                    m[i * n + j] = matrix.At(i, j);
                }
            }
            return ComputeGaussian(m.data(), n);
        }

        std::vector<T> m(n * n);
        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t j = 0; j < n; ++j) {
                m[i * n + j] = matrix.At(i, j);
            }
        }
        return ComputeGaussian(m.data(), n);
    }

private:
    static constexpr T Abs(T val) noexcept {
        return val < T{0} ? -val : val;
    }

    static constexpr T ComputeGaussian(T* m, std::size_t n) {
        T det = T{1};

        for (std::size_t i = 0; i < n; ++i) {
            std::size_t pivot = i;
            T max_val = Abs(m[i * n + i]);
            for (std::size_t j = i + 1; j < n; ++j) {
                if (const T val = Abs(m[j * n + i]); val > max_val) {
                    max_val = val;
                    pivot = j;
                }
            }

            if (max_val == T{0}) {
                return T{0};
            }

            if (pivot != i) {
                for (std::size_t k = 0; k < n; ++k) {
                    std::swap(m[i * n + k], m[pivot * n + k]);
                }
                det = -det;
            }

            det *= m[i * n + i];

            for (std::size_t j = i + 1; j < n; ++j) {
                const T factor = m[j * n + i] / m[i * n + i];
                for (std::size_t k = i; k < n; ++k) {
                    m[j * n + k] -= factor * m[i * n + k];
                }
            }
        }

        return det;
    }
};

// ============================================================================
// Базовый класс Matrix
// ============================================================================
template <typename T>
class Matrix : public IMatrix<T> {
protected:
    std::size_t size_{0};

public:
    constexpr explicit Matrix(std::size_t size) noexcept : size_(size) {}
    constexpr ~Matrix() override = default;

    [[nodiscard]] constexpr std::size_t Size() const noexcept override {
        return size_;
    }

    [[nodiscard]] T Determinant() const override {
        return DeterminantProxy<T>::Calculate(*this);
    }
};

// ============================================================================
// Реализация ArrayMatrix
// ============================================================================
template <typename T>
class ArrayMatrix : public Matrix<T> {
private:
    std::vector<T> data_;

public:
    constexpr explicit ArrayMatrix(std::size_t size)
        : Matrix<T>(size), data_(size * size, T{}) {}

    ArrayMatrix(std::size_t size, const std::vector<T>& values)
        : Matrix<T>(size), data_(values) {
        if (values.size() != size * size) {
            throw std::invalid_argument("ArrayMatrix: values size mismatch");
        }
    }

    ArrayMatrix(std::size_t size, std::initializer_list<T> values)
        : Matrix<T>(size), data_(values) {
        if (values.size() != size * size) {
            throw std::invalid_argument("ArrayMatrix: values size mismatch");
        }
    }

    [[nodiscard]] constexpr T& At(std::size_t row, std::size_t col) noexcept override {
        return data_[row * this->size_ + col];
    }

    [[nodiscard]] constexpr const T& At(std::size_t row, std::size_t col) const noexcept override {
        return data_[row * this->size_ + col];
    }
};

// ============================================================================
// Реализация JaggedMatrix
// ============================================================================
template <typename T>
class JaggedMatrix : public Matrix<T> {
private:
    std::vector<std::vector<T>> data_;

public:
    constexpr explicit JaggedMatrix(std::size_t size)
        : Matrix<T>(size), data_(size, std::vector<T>(size, T{})) {}

    JaggedMatrix(std::size_t size, const std::vector<T>& values)
        : Matrix<T>(size), data_(size, std::vector<T>(size, T{})) {
        if (values.size() != size * size) {
            throw std::invalid_argument("JaggedMatrix: values size mismatch");
        }
        for (std::size_t i = 0; i < size; ++i) {
            for (std::size_t j = 0; j < size; ++j) {
                data_[i][j] = values[i * size + j];
            }
        }
    }

    JaggedMatrix(std::size_t size, std::initializer_list<T> values)
        : Matrix<T>(size), data_(size, std::vector<T>(size, T{})) {
        if (values.size() != size * size) {
            throw std::invalid_argument("JaggedMatrix: values size mismatch");
        }
        auto it = values.begin();
        for (std::size_t i = 0; i < size; ++i) {
            for (std::size_t j = 0; j < size; ++j) {
                data_[i][j] = *it++;
            }
        }
    }

    [[nodiscard]] constexpr T& At(std::size_t row, std::size_t col) noexcept override {
        return data_[row][col];
    }

    [[nodiscard]] constexpr const T& At(std::size_t row, std::size_t col) const noexcept override {
        return data_[row][col];
    }
};

} // namespace Matrix

#endif // MATRIX_HPP_