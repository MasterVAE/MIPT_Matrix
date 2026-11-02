#include "matrix.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <charconv>
#include <system_error>

template <typename T>
static std::unique_ptr<Matrix::Matrix<T>>
make(const std::string& storage, std::size_t n, const std::vector<T>& v) {
    if (storage == "array")  return std::make_unique<Matrix::ArrayMatrix<T>>(n, v);
    if (storage == "jagged") return std::make_unique<Matrix::JaggedMatrix<T>>(n, v);
    throw std::runtime_error("unknown storage: " + storage);
}

template <typename T>
static int runTyped(const std::string& storage, std::size_t n) {
    std::vector<T> v(Matrix::detail::ElementCount(n));
    for (std::size_t i = 0; i < v.size(); ++i) {
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
        << "  " << prog << " <array|jagged> <int|longlong|float|double> <n>\n";
}

int main(int argc, char** argv) {
    if (argc != 4) {
        print_usage(argv[0]);
        return 1;
    }
    const std::string storage = argv[1];
    const std::string type    = argv[2];
    std::size_t n{};
    const std::string dimension = argv[3];
    const auto [end, error] = std::from_chars(dimension.data(), dimension.data() + dimension.size(), n);
    if (error != std::errc{} || end != dimension.data() + dimension.size()) {
        std::cerr << "invalid matrix dimension\n";
        return 1;
    }
    if (storage != "array" && storage != "jagged") {
        std::cerr << "unknown storage: " << storage << "\n";
        return 1;
    }

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