// //perfect shared_block cylcic vWARP
__global__ void comp_kernel_COO(int const* __restrict__ row_ind, int const* __restrict__ col_ind, float *val, const float * __restrict__ u, const float * __restrict__ v, 
   int nnz, int n_rows, int n_cols, int k, int *tiled_ind, int *tile_limIdx, int t_st)
{
    unsigned int tId = threadIdx.x;
    unsigned int laneId = tId & 1;
    int tile =  blockIdx.x;

    __shared__ float sh[32*180];
    int sh_tile = 180;
    int tile_st = tile_limIdx[tile];
    int tile_end = tile_limIdx[tile+1];
    int WARP_ID = tId >> 5;
    int tid_in_WARP = tId & 31;
    int step = blockDim.x >> 5;
    int slice = k >> 5;
    int tile_offset = tile * 180;

    unsigned int c = (tId >> 1 ) + tile_st;

    int t = tid_in_WARP;
    for (int i = WARP_ID; i < sh_tile && (i +(tile*sh_tile)) < n_cols; i+=step)
        sh[i *  32 + t] = v[(tile * sh_tile + i) *k + t_st + t];  

    // int t = WARP_ID;
    // for (int r = tid_in_WARP; r < sh_tile && (r +(tile*sh_tile)) < n_cols; r+=32)
    //     sh[t  * sh_tile + r] = v[ ((t+t_sh) * n_cols) + tile_offset + r];                   
    __syncthreads();
 
    for ( ;(c) < tile_end && (c)< nnz; c += (blockDim.x >> 1)){
        float sm =0, g=0, g1=0, g2=0, sm1=0, sm2=0 ;
        //int row = row_ind[c];
        //int col = col_ind[c];
        int row = row_ind[c] >> 8;
        int col = tile*sh_tile + (row_ind[c] & 0xff);
        int sh_col = col - tile * sh_tile;
       
        //for (int t = 0; t < 32; ++t){
        // for (int t = laneId; t < 32; t+=2){ 
        //     //sm += u[row*k+t] * v[col*k+t];
        //     // sm += u[row*k+t] * sh[(col - tile*180) * k + t];
        //     //g += u[row*k+ (t+t_sh)] *  sh[(t * sh_tile) + sh_col];
        //     //g += u[row*k+ (t)] * sh[sh_col * k + t];
        //     g = 1 * v[col];// * k + t];
        //     //g +=  u[row*k+(t)] *v[(t+t_sh)*n_cols+cc];
        // }
        // //__syncthreads();
        // // g += __shfl_xor(g, 4);
        // // g += __shfl_xor(g, 2);
        // // g += __shfl_xor(g, 1);
        // //__syncthreads();
        // val[c] =  (g) ;//* val[c];     


        for (int t = laneId*16; t < (laneId+1)*16; t+=8)
        //for (int t = 0; t < k; t+=8)
        {
            float4 rtmp1 = *((float4*) &sh[sh_col * 32 + t]); 
            float4 ctmp1 = *((float4*) &u[row * k + t_st + t ]);
            sm1+= rtmp1.x * ctmp1.x + rtmp1.y * ctmp1.y +  rtmp1.z * ctmp1.z +  rtmp1.w * ctmp1.w ; 
            
            float4 rtmp2 = *((float4*) &sh[sh_col * 32 + t+4]); 
            float4 ctmp2 = *((float4*) &u[ row * k + t_st + t+4]);
            sm2+= rtmp2.x * ctmp2.x + rtmp2.y * ctmp2.y +  rtmp2.z * ctmp2.z +  rtmp2.w * ctmp2.w ; 
        }
        sm1 += __shfl_xor(sm1, 1);
        sm2 += __shfl_xor(sm2, 1);
        //val[c] = val[c] * (sm1 + sm2);
        val[c] += (sm1 + sm2); 
    } 
    __syncthreads();
      
}

//perfect shared_block cylcic
// __global__ void comp_kernel_COO(int const* __restrict__ row_ind, int const* __restrict__ col_ind, float *val, const float * __restrict__ u, const float * __restrict__ v, 
//   int nnz, int n_rows, int n_cols, int k, int *tiled_ind, int *tile_limIdx)
// {
//     unsigned int tId = threadIdx.x;
//     unsigned int gId = (blockIdx.x * blockDim.x + tId) ;
//     int tile =  blockIdx.x;
//     int tile_st = tile_limIdx[blockIdx.x];
//     int tile_end = tile_limIdx[blockIdx.x+1];
//     __shared__ float sh[180*32];
//     int sh_tile = 180;
//     unsigned int c = tId + tile_st;
//     if(c < tile_end && c< nnz){
//         int col = col_ind[c];
//         //int tile  = col/sh_tile;

//         for (int t = 0; t < k; ++t){
//             for (int i = tId; i < 180 && (i +(tile*180)) < n_cols; i+=blockDim.x)
//                 sh[t * 180 + i] = v[ (t * n_cols) + (tile * 180 + i)];
//         } 
//     }          
    
//     __syncthreads();

//     for (int c = tId + tile_st; c < tile_end && c< nnz; c+=blockDim.x){
//         // int col = col_ind[c];
//         // int tile  = col/sh_tile;
//         // for (int t = 0; t < k; ++t){
//         //     for (int i = tId; i < sh_tile && (i +(tile*sh_tile)) < n_cols; i+=blockDim.x)
//         //         sh[t * sh_tile + i] = v[ (t * n_cols) + (tile * sh_tile + i)];

//         // }    
//         // __syncthreads();
//         float sm =0 ;
//         int row = row_ind[c];
//         int cc = col_ind[c];
//         for (int t = 0; t < k; ++t){
//             //sm += u[row*k+t] * v[col*k+t];
//             // sm += u[row*k+t] * sh[(col - tile*180) * k + t];
//             //sm += u[row*k+t] * v[t*n_cols+cc];
//             sm += u[row*k+t] * sh[(t * sh_tile) + (cc - tile * sh_tile)];
//             if(c == 700000) printf("GPU %d %d : %f %f \n", c, t, u[row*k+t], v[t*n_cols+cc] );  

//         }
//         val[c] = val[c] * sm;        
//     }    
// }
// __global__ void comp_kernel_COO(int const* __restrict__ row_ind, int *  col_ind, float *val, const float * __restrict__ u, const float * __restrict__ v, 
//   int nnz, int n_rows, int n_cols, int k, int *tiled_ind, int *tile_limIdx)
// {
//     unsigned int tId = threadIdx.x;
//     unsigned int c = (blockIdx.x * blockDim.x + tId) ;
//     int tile_st = tile_limIdx[blockIdx.x];
//     int tile_end = tile_limIdx[blockIdx.x+1];
//     __shared__ float sh[180*32];
//     int sh_tile = 180;
//     c = tId + tile_st;
//     int col = col_ind[c];
//     int tile  = col/sh_tile;

//     //if(tId < sh_tile && (tId +(tile*sh_tile)) < n_cols){
//     // if( c < tile_end && c < nnz ){ 
//     //     for (int t = 0; t < k; ++t){
//     //         for (int i = tId; i < 180 && (i +(tile*180)) < n_cols; i+=blockDim.x)
//     //             sh[t * 180 + i] = v[ (t * n_cols) + (tile * 180 + i)];
//     //     }           
//     // }
//     // __syncthreads();

//         //if( c < nnz ){

//     for (c = tId + tile_st; c < tile_end && c< nnz; c+=blockDim.x){
//         int col = col_ind[c];
//         int tile  = col/sh_tile;
//         for (int t = 0; t < k; ++t){
//             for (int i = tId; i < sh_tile && (i +(tile*sh_tile)) < n_cols; i+=blockDim.x)
//                 sh[t * sh_tile + i] = v[ (t * n_cols) + (tile * sh_tile + i)];

//         }    
//         __syncthreads();
//         float sm =0 ;
//         int row = row_ind[c];
//         //int col = col_ind[c];
//         for (int t = 0; t < k; ++t)
//             //sm += u[row*k+t] * v[col*k+t];
//             // sm += u[row*k+t] * sh[(col - tile*180) * k + t];
//             //sm += u[row*k+t] * v[t*n_cols+col];
//             sm += u[row*k+t] * sh[(t * sh_tile) + (col - tile * sh_tile)];
//         val[c] = val[c] * sm;
//         // if(col>130 && col< 135 && c <10000)
//             // printf("from gpu %d %d %d val: %f %f \n",c, col, tile,  sh[col/tile * k ], v[col*k] ); 
        
//     }
    
// }
// __global__ void comp_kernel_col_sorted_COO(int const* __restrict__ row_ind, int *  col_ind, float *val, const float * __restrict__ u, const float * __restrict__ v, 
//   int nnz, int n_rows, int n_cols, int k, int *tiled_ind, int *tile_limIdx)
// {
//     unsigned int tId = threadIdx.x;
//     unsigned int c = (blockIdx.x * blockDim.x + tId) ;
//     int tile_st = tile_limIdx[blockIdx.x];
//     int tile_end = tile_limIdx[blockIdx.x+1];
//     __shared__ float sh[180*32];
//     int sh_tile = 180;
//     c = tId + tile_st;
//     int col = col_ind[c];
//     int tile  = col/sh_tile;

//     //if(tId < sh_tile && (tId +(tile*sh_tile)) < n_cols){
//     //if( c < tile_end && c< nnz )
//     // if(c < tId + tile_st + blockDim.x)
//     // { 
//     //     for (int t = 0; t < k; ++t){
//     //         for (int i = tId; i < 180 && (i +(tile*180)) < n_cols; i+=blockDim.x)
//     //             sh[t * 180 + i] = v[ (t * n_cols) + (tile * 180 + i)];
//     //     }           
//     // }
//     // __syncthreads();

//         //if( c < nnz ){

//     for (c = tId + tile_st; c < tile_end && c< nnz; c+=blockDim.x){
//         int col = col_ind[c];
//         int tile  = col/sh_tile;
//         for (int t = 0; t < k; ++t){
//             for (int i = tId; i < sh_tile && (i +(tile*sh_tile)) < n_cols; i+=blockDim.x)
//                 sh[t * sh_tile + i] = v[ (t * n_cols) + (tile * sh_tile + i)];

//         }    
//         __syncthreads();
//         float sm =0 ;
//         int row = row_ind[c];
//         //int col = col_ind[c];
//         for (int t = 0; t < k; ++t)
//             //sm += u[row*k+t] * v[col*k+t];
//             // sm += u[row*k+t] * sh[(col - tile*180) * k + t];
//             //sm += u[row*k+t] * v[t*n_cols+col];
//             sm += u[row*k+t] * sh[(t * sh_tile) + (col - tile * sh_tile)];
//         val[c] = val[c] * sm;
//         // if(col>130 && col< 135 && c <10000)
//             // printf("from gpu %d %d %d val: %f %f \n",c, col, tile,  sh[col/tile * k ], v[col*k] ); 
        
//     }
    
// }

// //CSR format
// __global__ void comp_kernel_CSR(int const* __restrict__ row_ptr, int *  col_ind, float *val, const float * __restrict__ u, const float * __restrict__ v, 
//  float * p_ind, int n_rows, int k)
// {
//     unsigned int tId = threadIdx.x;
//     unsigned int laneId = tId & 31;
//     unsigned int c = (blockIdx.x * blockDim.x + tId) >> 5;
//     extern __shared__ volatile  float SD[];

//     if( c < n_rows ){
            
//         // c = rowGroupPtr[c];  float g=0,h=0;
//         unsigned int row_p = row_ptr[c], nnz_row = row_ptr[c+1] - row_ptr[c];
//         for(long i=laneId; i < nnz_row; i += 32) {
//             float sm =0 ;
//             int row = c;
//             int col = col_ind[row_p + i];
//             for (int t = 0; t < k; ++t)
//                 sm += u[row*k+t] * v[col*k+t];
//             p_ind[row_p + i] = sm * val[row_p + i];
 

//         }
//         // __syncthreads();
//         if(c>500 && c< 505  && laneId == 0)
//             printf("from gpu %d %f\n",c,  p_ind[c] );
//         // float newvj=0;
//         // g += __shfl_down(g, 16);   
//         // g += __shfl_down(g, 8);
//         // g += __shfl_down(g, 4);   
//         // g += __shfl_down(g, 2);
//         // g += __shfl_down(g, 1);
//         // __syncthreads();
       
//     } 
// }


// __global__ void comp_kernel_COO(int const* __restrict__ row_ind, int *  col_ind, float *val, 
//     const float * __restrict__ u, const float * __restrict__ v, float * p_ind, int nnz, int k){
  
//     unsigned int tId = threadIdx.x;
//     // unsigned int laneId = tId & 31;
//     unsigned int c = (blockIdx.x * blockDim.x + tId) ;//>> 5;
//     // extern __shared__ volatile  float SD[];
//     // printf("I am in \n");
//     if( c < nnz ){

//         float sm =0 ;
//         int row = row_ind[c];
//         int col = col_ind[c];
//         printf("from gpu %d %f %d %f\n", c , p_ind[c], row, v[row*k+1] );

//         for (int t = 0; t < k; ++t)
//             sm += u[row*k+t] * v[col*k+t];
//         float s = .4;
//         p_ind[c] += val[c] * s;
                    
//          // * val[c]; 

//         // // __syncthreads();
//         // if(c>5 && c< 14)
//         //     printf("from gpu %d %f\n",c,  p_ind[c] );      
//     } 



// __global__ void comp_kernel_COO(int const* __restrict__ row_ind, int *  col_ind, float *val, const float * __restrict__ u, const float * __restrict__ v, 
//   int nnz, int n_rows, int n_cols, int k, int *tiled_ind, int *tile_limIdx)
// {
//     unsigned int tId = threadIdx.x;
//     unsigned int c = (blockIdx.x * blockDim.x + tId) ;
//     int tile_st = tile_limIdx[blockIdx.x];
//     int tile_end = tile_limIdx[blockIdx.x+1];
//     __shared__ float sh[180*32];
    
//     c = tId + tile_st;
//     int cc = col_ind[c];
//     int tile  = cc/180;

//         //if( c < nnz ){
//     for (c = tId + tile_st; c < tile_end && c< nnz; c+=blockDim.x){
//         int cc = col_ind[c];
//         int tile  = cc/180;
//         for (int t = 0; t < k; ++t){
//             for (int i = tId; i < 180 && (i +(tile*180)) < n_cols; i+=blockDim.x)
//                 sh[t * 180 + i] = v[ (t * n_cols) + (tile * 180 + i)];

//         }    
//         __syncthreads();
//     }

//         float sm =0 ;
//         int row = row_ind[c];
//         int col = col_ind[c];
//         for (int t = 0; t < k; ++t)
//             sm += u[row*k+t] * sh[(t * 180) + (col - tile * 180)];
//         val[c] = val[c] * sm;
//     }
// }

//no blcok cyclic 
// __global__ void comp_kernel_COO(int const* __restrict__ row_ind, int *  col_ind, float *val, const float * __restrict__ u, const float * __restrict__ v, 
//   int nnz, int n_rows, int n_cols, int k, int *tiled_ind, int *tile_limIdx)
// {
//     unsigned int tId = threadIdx.x;
//     unsigned int c = (blockIdx.x * blockDim.x + tId) ;

//     int sh_tile = 180;
//     int cc = col_ind[c];
//     int tile  = cc/sh_tile;

//     int tile_st = tile_limIdx[tile];
//     //int tile_end = tile_limIdx[blockIdx.x+1];
//     int tile_end = tile_limIdx[tile+1];
//     // c = tId + tile_st;
//     if( c < tile_end+sh_tile ){
//         //if(c < tId + tile_st + blockDim.x)
//         __shared__ float sh[180*32];
//         for (int t = 0; t < k; ++t){
//             for (int i = tId; i < sh_tile && (i +(tile*sh_tile)) < n_cols; i+=blockDim.x){
//                 sh[t * sh_tile + i] = v[ (t * n_cols) + (tile * sh_tile + i)];
//             }
//         }   
//         __syncthreads();
//         if(c == 189520)printf("gpu 1 %d %d %d %d \n", c, cc, (0 * sh_tile) + (cc - tile * sh_tile), 0*n_cols+cc);
//         if( c < tile_end ){
       
//         //if(c % blockDim.x == 0 && blockIdx.x == 0) printf("blockcyclic %d %d \n", blockIdx.x, c);
//         //if(tId == 0 ) printf("gpu" ); 
//         float sm =0 ;
//         int row = row_ind[c];
//         int col = col_ind[c];
//         // if(sh[(0 * sh_tile) + (col - tile * sh_tile)] != v[0*n_cols+col] && col< 190){
//         //     printf("gpu 1 %d %d %f %f \n", c, col, sh[(0 * sh_tile) + (col - tile * sh_tile)], v[0*n_cols+col]);
//         //     printf("gpu 2 %d %d %f %f \n", c, col, sh[(1 * sh_tile) + (col - tile * sh_tile)], v[1*n_cols+col]);
//         // }

//         for (int t = 0; t < k; ++t){
//             //sm += u[row*k+t] * v[col*k+t];
//             //sm += u[row*k+t] * sh[(col - tile*180) * k + t];
//             //sm += u[row*k+t] * v[t*n_cols+col];
//             sm += u[row*k+t] * sh[(t * sh_tile) + (col - tile * sh_tile)];
//             //if(c =55064677 && t ==6 ) printf("gpu %d %f %f \n", c, sh[(t * sh_tile) + (col - tile * sh_tile)], v[t*n_cols+col]);
//         }
//         val[c] = val[c] * sm;
//         // if(col>130 && col< 135 && c <10000)
//             // printf("from gpu %d %d %d val: %f %f \n",c, col, tile,  sh[col/tile * k ], v[col*k] ); 
//     }
//     }
    
// }



