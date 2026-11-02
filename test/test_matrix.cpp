#include "matrix.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
int checks = 0;
void Check(bool ok, const char* message) {
    ++checks;
    if (!ok) throw std::runtime_error(message);
}
template<class E, class F> void Throws(F action, const char* message) {
    bool caught = false;
    try { action(); } catch (const E&) { caught = true; }
    Check(caught, message);
}
template<template<class> class S, class T> void Test() {
    S<T> empty(0);
    Check(empty.Size() == 0 && empty.Determinant() == T{1}, "empty");
    S<T> zero(3);
    Check(zero.At(2, 2) == T{} && zero.Determinant() == T{}, "zero");
    S<T> one(1, {T{7}});
    Check(one.Determinant() == T{7}, "1x1");
    S<T> a(2, {1, 2, 3, 4});
    const Matrix::IMatrix<T>& view = a;
    Check(view.Size() == 2 && view.At(1, 0) == T{3}, "polymorphic access");
    Check(std::abs(static_cast<double>(view.Determinant()) + 2) < 1e-4, "2x2");
    Check(a.At(0, 0) == T{1} && a.At(1, 1) == T{4}, "non-mutating determinant");
    a.At(0, 0) = T{2};
    Check(std::abs(static_cast<double>(a.Determinant()) - 2) < 1e-4, "mutable access");
    S<T> pivot(3, {0, 1, 0, 1, 0, 0, 0, 0, 3});
    Check(pivot.Determinant() == T{-3}, "pivot sign");
    S<T> singular(3, {1, 2, 3, 2, 4, 6, 0, 0, 0});
    Check(singular.Determinant() == T{}, "singular");
    S<T> known(3, {1, 2, 3, 4, 5, 6, 7, 8, 10});
    Check(std::abs(static_cast<double>(known.Determinant()) + 3) < 1e-4, "3x3");
    for (std::size_t n : {64, 65}) {
        S<T> identity(n);
        for (std::size_t i = 0; i < n; ++i) identity.At(i, i) = T{1};
        Check(identity.Determinant() == T{1}, "scratch buffer boundary");
    }
    Throws<std::invalid_argument>([] { S<T> bad(2, {1, 2, 3}); }, "list mismatch");
    Throws<std::invalid_argument>([] { S<T> bad(2, std::vector<T>{1, 2, 3, 4, 5}); }, "vector mismatch");
    Throws<std::length_error>([] { S<T> bad(std::numeric_limits<std::size_t>::max()); }, "size overflow");
    if constexpr (std::is_integral_v<T>) {
        S<T> overflow(2, {std::numeric_limits<T>::max(), 0, 0, 2});
        Throws<std::overflow_error>([&] { (void)overflow.Determinant(); }, "result overflow");
        S<T> cancellation(3, {50000, 1, 0, 0, 50000, 1, 1, 0, 0});
        Check(cancellation.Determinant() == 1, "wide intermediate");
    } else {
        S<T> fraction(2, {T{0.5}, T{1.5}, T{2}, T{3}});
        Check(std::abs(fraction.Determinant() + T{1.5}) < T{0.0001}, "fractional");
    }
}
}
int main() {
    try {
        Test<Matrix::ArrayMatrix, int>();
        Test<Matrix::ArrayMatrix, long long>();
        Test<Matrix::ArrayMatrix, float>();
        Test<Matrix::ArrayMatrix, double>();
        Test<Matrix::JaggedMatrix, int>();
        Test<Matrix::JaggedMatrix, long long>();
        Test<Matrix::JaggedMatrix, float>();
        Test<Matrix::JaggedMatrix, double>();
        std::cout << checks << " unit checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << "Unit check failed: " << e.what() << '\n';
        return 1;
    }
}
