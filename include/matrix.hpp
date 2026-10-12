#pragma once

#include <cstddef>
#include <vector>
#include <utility>
#include <stdexcept>
#include <type_traits> 

namespace Matrix
{

template <typename T>
class Matrix {
protected:
    std::size_t size_;

    T DeterminantFloating()
    {
        const std::size_t n = size_;
        if (n == 0) return T{1};

        std::vector<std::vector<T>> m(n, std::vector<T>(n, T{}));
        for (std::size_t i = 0; i < n; ++i)
            for (std::size_t j = 0; j < n; ++j)
                m[i][j] = At(i, j);

        T det = T{1};

        for (std::size_t i = 0; i < n; ++i) {
            std::size_t pivot = n;
            for (std::size_t j = i; j < n; ++j) {
                if (m[j][i] != T{}) { pivot = j; break; }
            }
            if (pivot == n) return T{};

            if (pivot != i) {
                std::swap(m[i], m[pivot]);
                det = -det;
            }

            det *= m[i][i];

            for (std::size_t j = i + 1; j < n; ++j) {
                T factor = m[j][i] / m[i][i];
                for (std::size_t k = i; k < n; ++k)
                    m[j][k] -= factor * m[i][k];
            }
        }

        return det;
    }

    T DeterminantInteger()
    {
        const std::size_t n = size_;
        if (n == 0) return T{1};

        std::vector<std::vector<T>> a(n, std::vector<T>(n, T{}));
        for (std::size_t i = 0; i < n; ++i)
            for (std::size_t j = 0; j < n; ++j)
                a[i][j] = At(i, j);

        T sign = T{1};
        T prev = T{1};

        for (std::size_t k = 0; k + 1 < n; ++k) {
            if (a[k][k] == T{}) {
                std::size_t pivot = k + 1;
                while (pivot < n && a[pivot][k] == T{})
                    ++pivot;

                if (pivot == n)
                    return T{};

                std::swap(a[k], a[pivot]);
                sign = -sign;
            }

            for (std::size_t i = k + 1; i < n; ++i) {
                for (std::size_t j = k + 1; j < n; ++j) {
                    a[i][j] = (a[i][j] * a[k][k] - a[i][k] * a[k][j]) / prev;
                }
            }

            prev = a[k][k];
        }

        return sign * a[n - 1][n - 1];
    }

public:
    Matrix(std::size_t size) : size_(size) {}
    virtual ~Matrix() = default;
    virtual T& At(std::size_t, std::size_t) = 0;

    T Determinant()
    {
        if constexpr (std::is_integral_v<T>) {
            return DeterminantInteger();
        } else {
            return DeterminantFloating();
        }
    }
};


template <typename T>
class ArrayMatrix : public Matrix<T> {
private:
    std::vector<T> data_;

public:
    ArrayMatrix(std::size_t size)
        : Matrix<T>(size), data_(size * size, T{}) {}

    ArrayMatrix(std::size_t size, const std::vector<T>& values)
        : Matrix<T>(size), data_(values)
    {
        if (values.size() != size * size)
            throw std::invalid_argument("ArrayMatrix: values size mismatch");
    }

    T& At(std::size_t row, std::size_t col) override {
        return data_[row * this->size_ + col];
    }
};

template <typename T>
class JaggedMatrix : public Matrix<T> {
private:
    std::vector<std::vector<T>> data_;

public:
    JaggedMatrix(std::size_t size)
        : Matrix<T>(size), data_(size, std::vector<T>(size, T{})) {}

    JaggedMatrix(std::size_t size, const std::vector<T>& values)
        : Matrix<T>(size), data_(size, std::vector<T>(size, T{}))
    {
        if (values.size() != size * size)
            throw std::invalid_argument("JaggedMatrix: values size mismatch");

        for (std::size_t i = 0; i < size; ++i)
            for (std::size_t j = 0; j < size; ++j)
                data_[i][j] = values[i * size + j];
    }

    T& At(std::size_t row, std::size_t col) override {
        return data_[row][col];
    }
};

} // namespace Matrix