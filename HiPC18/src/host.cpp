#include <omp.h>

#include "Matrix.hpp"
#include "host.hpp"

void sddmm_CPU_COO(
    const Matrix<float> &matrixA,
    const Matrix<float> &matrixB,
    const SparseMatrix<float> &matrixS,
    SparseMatrix<float> &matrixP) {
    const int K = matrixA.col();

    // reduction(+:rmse)
    double start_time = omp_get_wtime();
    omp_set_dynamic(0);
    omp_set_num_threads(28);
#pragma omp parallel for //reduction(+:tot)
    for (int idx = 0; idx < matrixS.nnz(); ++idx) {
        float sm = 0;
        const int row = matrixS.rowIndex()[idx];
        const int col = matrixS.colIndex()[idx];
        for (int t = 0; t < K; ++t)
            sm += matrixA.values()[row * K + t] * matrixB.values()[col * K + t];
        matrixP.setValues()[idx] = sm;//* val_ind[ind];
        // cout << "ind " << row<<" "<<col << ":: "  <<" "<< p_ind[ind] << " = " << sm <<" * "<< val_ind[ind]<< endl;
    }
    double CPU_time = omp_get_wtime() - start_time;
    //correctness check

    printf("\nomp time CPU : %.4f \n\n", CPU_time * 1000);
}