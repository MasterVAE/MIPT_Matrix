#include <iostream>

#include "matrix.hpp"

int main(int argc, char* argv[])
{
    int size = 0;
    std::cin >> size;

    std::vector<float> values;
    for(size_t i = 0; i < size * size; i++)
    {
        float value;
        std::cin >> value;
        values.push_back(value);
    }

    Matrix::ArrayMatrix<float>* ar_matrix = new Matrix::ArrayMatrix<float>(size, values);
    Matrix::JaggedMatrix<float>* jg_matrix = new Matrix::JaggedMatrix<float>(size, values);

    std::cout << ar_matrix->Determinant() << std::endl;
    std::cout << jg_matrix->Determinant() << std::endl;

    delete ar_matrix;
    delete jg_matrix;
    return 0;
}