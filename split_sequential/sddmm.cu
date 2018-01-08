#include <iostream>
#include <fstream>
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <iterator>
#include <utility>  
#include <math.h> 
#include <omp.h>
#include <cuda.h>
#include "util.h"
#include "kernel.h"
#include <bits/stdc++.h>    
using namespace std;

long n_rows, n_cols, nnz;
int tile_sizeX = 256; 
int tile_sizeY = 99999999;
int k=100;
int BLOCKSIZE=512;
inline cudaError_t checkCuda(cudaError_t result, int s){

  if (result != cudaSuccess) {
    fprintf(stderr, "CUDA Runtime Error in line : %s - %d\n", cudaGetErrorString(result), s);
    assert(result == cudaSuccess);
  }
  return result;
}

void sddmm_GPU(int * d_row_ptr, int * d_row_ind, int *d_col_ind, float * d_val_ind, float * d_W, float *d_H, 
int *d_tiled_ind, int *d_lastIdx, long new_nnz){
    int n_tile = n_cols/tile_sizeX + 1;
    int n_tile_r = n_rows/tile_sizeY + 1;
    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    cudaStream_t stream[n_tile]; 
    for (int i = 0; i < n_tile; i++){
        cudaStreamCreate(&(stream[i]));
    }

    float mili =0, copyTime = 0 ;
  
    dim3 block(BLOCKSIZE,1,1), grid(1,1,1);
    //grid.x = (new_nnz + BLOCKSIZE - 1) / BLOCKSIZE;
    grid.x = (n_cols/tile_sizeX+1);
    // grid.x = (32 * n_rows + BLOCKSIZE - 1) / BLOCKSIZE;
    checkCuda(cudaEventRecord(start), __LINE__);

    for (int t_st = 0; t_st < k ; t_st +=32){
        for (int r_t = 0; r_t < n_tile_r; ++r_t){
            int r_st = r_t * tile_sizeY;
            comp_kernel_COO<<<grid, block, 0, stream[0]>>>(d_row_ind, d_col_ind, d_val_ind, d_W, d_H, 
        new_nnz, n_rows, n_cols, k, d_tiled_ind, d_lastIdx+n_tile*r_t, t_st, r_st);
        }
    }

    checkCuda(cudaEventRecord(stop), __LINE__);
    cudaEventSynchronize(stop);
    //cudaDeviceSynchronize();
    checkCuda(cudaEventElapsedTime(&mili, start, stop), __LINE__);
    cudaDeviceSynchronize();
    cout << "GPU time " << mili << "ms"<< endl;

}
void sddmm_CPU_CSR(int * row_ptr, int *col_ind, float * val_ind, float * W, float *H, float * p_ind){
       // reduction(+:rmse)
    long tot =0 ;
    #pragma omp parallel for reduction(+:tot)
    for (int r = 0; r < n_rows; ++r){
        tot += row_ptr[r+1] - row_ptr[r];
        float sm =0 ;
        for (int ind = row_ptr[r]; ind < row_ptr[r+1]; ++ind){
            int row = r;
            int col = col_ind[ind]; 
            int nnz = row_ptr[r+1]-row_ptr[r];

            float val = val_ind[ind];
            sm=0;
            for (int t = 0; t < k; ++t){
                sm += W[row * k + t] * H[col * k + t];
                // cout <<W[row * k + t] <<" "<<H[col * k + t]<< endl;
            }
            p_ind[ind] = sm * val_ind[ind];
            // cout << "ind " << row<<" "<<col << ":: "  <<" "<< p_ind[ind] << " = " << sm <<" * "<< val_ind[ind]<< endl;  

           
        }                
    }
    cout << "CPU tot " << tot << endl;
    for (int r = 500000; r < 500005; ++r)
        // for (int ind = row_ptr[r]; ind < row_ptr[r+1]; ++ind)
            cout << "row " << r << " "  <<" "<< p_ind[r]<< endl;  
}

void sddmm_CPU_COO(int * row_ind, int *col_ind, float * val_ind, float * W, float *H, float * p_ind){
       // reduction(+:rmse)
    double start_time = omp_get_wtime();
    omp_set_dynamic(0);
    omp_set_num_threads(28);
    #pragma omp parallel for //reduction(+:tot)
    for (int ind = 0; ind < nnz; ind++){
        float sm =0 ;
        int row = row_ind[ind];
        int col = col_ind[ind]; 
        for (int t = 0; t < k; ++t)
            // sm = H[col];// * k + t];
            sm += W[row * k + t] * H[col * k + t];
        // p_ind[ind] = sm;// * val_ind[ind];
         p_ind[ind] =  sm;
            // cout << "ind " << row<<" "<<col << ":: "  <<" "<< p_ind[ind] << " = " << sm <<" * "<< val_ind[ind]<< endl;  
               // }                
    }
    double CPU_time = omp_get_wtime() - start_time;
    //correctness check

    printf("\nomp time CPU : %.4f \n\n", CPU_time*1000);
}


void init(int *rows, int *cols, float* vals){

    int n_bin=10;
    int *count = new int[n_bin];
    int *row_ptr = new int[n_rows+1];
    float *p_ind = new float[nnz];
   
    float *W = new float[n_rows*k];
    float *W_t = new float[n_rows*k];
    float *H = new float[n_cols*k];
    float *H_t = new float[n_cols*k];
    int n_tile_c = n_cols/tile_sizeX + 1;
    int n_tile_r = n_rows/tile_sizeY + 1;
    int *lastIdx_tile = new int[n_tile_c*n_tile_r+1];
    int *row_holder = new int[n_rows];
    float *d_val, *d_W, *d_H, *d_W_t;
    int *d_row_ptr, *d_col_ind, *d_row_ind, *d_tiled_ind, *d_lastIdx;

    int n_tileX = n_cols/tile_sizeX+1; 
    int n_tileY = n_rows/tile_sizeY+1; 
    long new_nnz =0 ;
    initial(W, n_rows, k);
    initial(H, n_cols, k);
    make_HTasH(H, H_t, n_cols, k);
    make_HTasH(W, W_t, n_rows, k);
    
    cout << "n_tiles in x: " <<n_tile_c << " n_tiles in y: "<< n_tile_r<< endl;
    int *new_rows = new int[nnz];
    int *new_cols = new int[nnz ];
    float *new_vals = new float[nnz ];
    int *tiled_ind = new int [nnz ];
    // int *new_rows = new int[nnz];
    // int *new_cols = new int[nnz];
    // float *new_vals = new float[nnz];
    //converting col sorted matrix to row sorted
    //unsorted_make_CSR(rows, cols, vals, nnz, n_rows, n_cols, row_ptr);
    //assuming sorted
    make_CSR(rows, cols, vals, nnz, n_rows, row_ptr, row_holder);
    //comp_bin(n_bin, count, n_rows, row_ptr, nnz);

    rewrite_matrix_1D(row_ptr, rows, cols, vals, new_rows, new_cols, new_vals, nnz, n_rows, n_cols, 
        tile_sizeX, tile_sizeY, tiled_ind, lastIdx_tile, BLOCKSIZE, new_nnz, row_holder);

    // rewrite_col_sorted_matrix(row_ptr, rows, cols, vals, new_rows, new_cols, new_vals, nnz, n_rows, n_cols, 
    //     tile_sizeX, tiled_ind, lastIdx_tile, BLOCKSIZE, new_nnz);
    double t0 = seconds();
    // sddmm_CPU_CSR(row_ptr, cols, vals, W, H, p_ind);
    sddmm_CPU_COO(rows, cols, vals, W, H, p_ind);

   //***********Starting GPU****************
    checkCuda(cudaMalloc((void**)&d_W, k*n_rows*sizeof(float)),0); 
    checkCuda(cudaMalloc((void**)&d_H, k*n_cols*sizeof(float)),1);
    // checkCuda(cudaMalloc((void**)&d_row_ptr, (n_rows+1)*sizeof(int)),2);   
    checkCuda(cudaMalloc((void**)&d_row_ind, new_nnz*sizeof(int)),4);  
    //checkCuda(cudaMalloc((void**)&d_col_ind, new_nnz*sizeof(int)),4);
    checkCuda(cudaMalloc((void**)&d_val, new_nnz*sizeof(float)),4);
    checkCuda(cudaMalloc((void**)&d_lastIdx, (n_tile_c*n_tile_r+1)*sizeof(float)),4);
    // checkCuda(cudaMalloc((void**)&d_tiled_ind, nnz*sizeof(int)),4);
    // checkCuda(cudaMemcpy(d_row_ptr,  &(row_ptr[0]), (n_rows+1)*sizeof(int), cudaMemcpyHostToDevice),4);
    checkCuda(cudaMemcpy(d_row_ind,  &(new_rows[0]), new_nnz*sizeof(int), cudaMemcpyHostToDevice),4);
    //checkCuda(cudaMemcpy(d_col_ind, &(new_cols[0]), new_nnz*sizeof(int), cudaMemcpyHostToDevice),4);;
    checkCuda(cudaMemcpy(d_val, &(new_vals[0]), new_nnz*sizeof(float), cudaMemcpyHostToDevice),4);;    
    checkCuda(cudaMemcpy(d_lastIdx, &(lastIdx_tile[0]), (n_tile_c*n_tile_r+1)*sizeof(float), cudaMemcpyHostToDevice),4);;    

    cudaMemset(d_val, 0, nnz*sizeof(float));
    // checkCuda(cudaMemcpy(d_tiled_ind, &(tiled_ind[0]), nnz*sizeof(int), cudaMemcpyHostToDevice),4);;    

    cudaMemcpy(d_W, &(W[0]),  n_rows * k *sizeof(float), cudaMemcpyHostToDevice);
    //cudaMemcpy(d_W, &(W_t[0]),  n_rows * k *sizeof(float), cudaMemcpyHostToDevice);
    
    cudaMemcpy(d_H, &(H[0]),  n_cols * k *sizeof(float), cudaMemcpyHostToDevice);  
    //cudaMemcpy(d_H, &(H_t[0]),  n_cols * k *sizeof(float), cudaMemcpyHostToDevice);  

    sddmm_GPU(d_row_ptr, d_row_ind, d_col_ind, d_val, d_W, d_H, d_tiled_ind, d_lastIdx, new_nnz );

    //******** correctness check
    float GPU_tot = 0, CPU_tot =0, CPU_tot_orig =0 ;
    float *p_ind_temp = new float[new_nnz];
    checkCuda(cudaMemcpy(&(p_ind_temp[0]), d_val, new_nnz*sizeof(float), cudaMemcpyDeviceToHost),4);;    
    for (int i = 0; i < nnz; ++i){
        CPU_tot +=  p_ind[tiled_ind[i]];
        CPU_tot_orig +=  p_ind[i];
        // cout << "p_ind " << p_ind[tiled_ind[i]] << " " << p_ind[i] << " new,old ind: "<<tiled_ind[i] <<" "<<i<< endl;
    }

    for (int i = 511; i <  511+2; ++i)
            cout << "gp idx " << i << " "  <<" GPU "<< p_ind_temp[i] << " CPU "<< p_ind[tiled_ind[i]]<<endl;  
    for (int i = nnz-1; i > nnz-3; --i)
             cout << "gp idx " << i << " "  <<" GPU "<< p_ind_temp[i] << " CPU "<< p_ind[tiled_ind[i]]<<endl;  
    long diff_tot = 0; 
    for (int i = 0; i < new_nnz; ++i){
       if(abs(p_ind_temp[i]-p_ind[tiled_ind[i]]) > .00001){
            diff_tot ++;
            if(diff_tot < 5)
                printf("CPU GPU diff %d:  %f %f %f \n", i, p_ind_temp[i], p_ind[tiled_ind[i]],p_ind_temp[i]-p_ind[tiled_ind[i]] );
        }      
    }
    cout << "diff values in CPU and GPU: " << diff_tot << endl;

    //freeing device allocation
    cudaFree( d_row_ptr );
    cudaFree( d_row_ind);
    cudaFree( d_col_ind);
    cudaFree( d_val);
    cudaFree( d_W );
    cudaFree( d_H );
    delete(rows); delete(cols);
    delete(vals);

}

int main(int argc, char* argv[]){ 
    ifstream fp(argv[1]);   
    k = atoi(argv[2]);
    tile_sizeX = atoi(argv[3]);   
    string str;  
    getline(fp,str);
    while(!isdigit(str[0])){
        getline(fp,str);
    }
    istringstream is(str);
    is >> n_rows; 
    is >> n_cols; 
    is >> nnz; 
    //fp >> n_rows >> n_cols >> nnz;
    long orig_nnz=nnz, rid=0,cid=0; float vid=0;
  
    int *rows = new int[nnz];
    int *cols = new int[nnz];  
    float *vals = new float[nnz]; 
    long idx=0;
    for (long o_idx = 0; o_idx < orig_nnz; ++o_idx) {
        fp >> rid >> cid >> vid;
        rows[idx]=rid-1;
        cols[idx]=cid-1;
        vals[idx]=vid;
        idx++;
    }
    cout << "Rows: "<<n_rows << " Cols: "<<n_cols <<" Nnz: "<< nnz << " tile-size: " << tile_sizeX<< " k: "<<k <<  endl;
    nnz=idx;

    init(rows, cols, vals);
}


