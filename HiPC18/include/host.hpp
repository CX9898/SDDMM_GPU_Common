#pragma once

#include <vector>

#include "Matrix.hpp"

/**
 * error checking
 **/
bool checkData(const int num, const float *data1, const float *data2);

bool checkData(const int num, const std::vector<float> &dataHost1, const float *dataDev2);

bool checkData(const int num, const float *dataDev1, const std::vector<float> &dataHost2);

bool checkDevData(const int num, const float *dataDev1, const float *dataDev2);

void sddmm_CPU_COO(
    const Matrix<float> &matrixA,
    const Matrix<float> &matrixB,
    const SparseMatrix<float> &matrixS,
    SparseMatrix<float> &matrixP);