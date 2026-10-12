#include "matrix.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <gtest/gtest.h>

template <typename T>
static std::unique_ptr<Matrix::Matrix<T>>
make(const std::string& storage, std::size_t n, const std::vector<T>& v) {
    if (storage == "array")  return std::make_unique<Matrix::ArrayMatrix<T>>(n, v);
    if (storage == "jagged") return std::make_unique<Matrix::JaggedMatrix<T>>(n, v);
    throw std::runtime_error("unknown storage: " + storage);
}

template <typename T>
static int runTyped(const std::string& storage, std::size_t n) {
    std::vector<T> v(n * n);
    for (std::size_t i = 0; i < n * n; ++i) {
        if (!(std::cin >> v[i])) {
            std::cerr << "input error at position " << i << "\n";
            return 2;
        }
    }
    auto m = make<T>(storage, n, v);
    T det = m->Determinant();
    std::cout.precision(17);
    std::cout << det << "\n";
    return 0;
}

static void print_usage(const char* prog) {
    std::cerr
        << "usage:\n"
        << "  " << prog << " [--gtest|--gtest_filter=...]   # run google tests\n"
        << "  " << prog << " <array|jagged> <int|longlong|float|double> <n>\n";
}

int main(int argc, char** argv) {
    bool want_tests = (argc <= 1);
    for (int i = 1; i < argc && !want_tests; ++i) {
        const std::string a = argv[i];
        if (a == "--gtest" || a.rfind("--gtest_", 0) == 0) {
            want_tests = true;
        }
    }

    ::testing::InitGoogleTest(&argc, argv);

    if (want_tests) {
        return RUN_ALL_TESTS();
    }

    if (argc != 4) {
        print_usage(argv[0]);
        return 1;
    }
    const std::string storage = argv[1];
    const std::string type    = argv[2];
    const std::size_t n       = std::stoull(argv[3]);

    try {
        if (type == "int")      return runTyped<int>      (storage, n);
        if (type == "longlong") return runTyped<long long>(storage, n);
        if (type == "float")    return runTyped<float>    (storage, n);
        if (type == "double")   return runTyped<double>   (storage, n);
        std::cerr << "unknown type: " << type << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 3;
    }
}