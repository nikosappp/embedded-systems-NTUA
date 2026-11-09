#include "hal_data.h"
#include "stdio.h"
#include "string.h"
#include "images.h"

#define N 144    /*Frame dimension for QCIF format*/
#define M 176    /*Frame dimension for QCIF format*/

#define Bx 16    /*Block size in X dimension (must divide N)*/
#define By 16    /*Block size in Y dimension (must divide M)*/

#define p 7      /*Search space. Restricted in a [-p,p] region around the
                   original location of the block.*/

int local_current[Bx][By];                  // Local RAM array for the Bx x By block
int local_previous[Bx + 2*p][By + 2*p];     // Local RAM array for the search window


void phods_motion_estimation(const int current[N][M], const int previous[N][M],
        int vectors_x[N/Bx][M/By],int vectors_y[N/Bx][M/By])
{
  int x, y, i, j, k, l, p1, p2, q2, distx, disty, S, min1, min2, bestx, besty;

  // --- DRT: Declaration of local RAM variables and arrays ---
  int row, col, gx, gy; 
  
  distx = 0;
  disty = 0;

  /*Initialize the vector motion matrices*/
  for(i=0; i<N/Bx; i++)
  {
    for(j=0; j<M/By; j++)
    {
      vectors_x[i][j] = 0;
      vectors_y[i][j] = 0;
    }
  }

  /*For all blocks in the current frame*/
  for(x=0; x<N/Bx; x++)
  {
    for(y=0; y<M/By; y++)
    {
      // --- DRT: Copying data from Flash to RAM ---
      
      // Copy the current block 
      for(row = 0; row < Bx; row++) {
          for(col = 0; col < By; col++) {
              local_current[row][col] = current[Bx*x + row][By*y + col];
          }
      }

      // Copy the previous search window and handle image boundaries (zero padding) 
      for(row = 0; row < (Bx + 2*p); row++) {
          for(col = 0; col < (By + 2*p); col++) {
              gx = (Bx*x) - p + row;
              gy = (By*y) - p + col;
              
              if(gx < 0 || gx >= N || gy < 0 || gy >= M) {
                  local_previous[row][col] = 0; // Out of bounds pixels become 0
              } else {
                  local_previous[row][col] = previous[gx][gy];
              }
          }
      }

      // ==========================================
      // UNROLL ITERATION 1: S = 4
      // ==========================================
      S = 4;
      min1 = 255*Bx*By;
      min2 = 255*Bx*By;

      /*For all candidate blocks in X dimension*/
      for(i=-S; i<S+1; i+=S)
      {
        distx = 0;

        /* 
        * NOTE: 
        * We did not apply Loop Interchange. 
        * C stores 2D arrays in row-major order. The current loops (k, l) 
        * already read data row by row, which is the best for cache memory. 
        * Swapping them would ruin spatial locality and make the code slower. 
        */
        
        /*For all pixels in the block - LOOP REVERSAL APPLIED*/
        for(k = Bx - 1; k >= 0; k--)
        {
          for(l = By - 1; l >= 0; l--)
          {
            // --- DRT: Read from local RAM instead of Flash ---
            p1 = local_current[k][l];

            // Complex bounds checking is replaced by local_previous access.
            // Index logic: actual_x - (block_start_x - p) => actual_x - Bx*x + p.
            p2 = local_previous[vectors_x[x][y] + i + k + p][vectors_y[x][y] + l + p];
            
            if(p1-p2>0){
                distx += p1-p2;
            }
            else{
                distx += p2-p1;
            }
          }
        }

        if(distx < min1)
        {
          min1 = distx;
          bestx = i;
        }
      }

      /*For all candidate blocks in Y dimension*/
      for(i=-S; i<S+1; i+=S)
      {
        disty = 0;

        /*For all pixels in the block - LOOP REVERSAL APPLIED*/
        for(k = Bx - 1; k >= 0; k--)
        {
          for(l = By - 1; l >= 0; l--)
          {
            // --- DRT: Read from local RAM ---
            p1 = local_current[k][l];
            q2 = local_previous[vectors_x[x][y] + k + p][vectors_y[x][y] + i + l + p];

            if(p1-q2>0){
                 disty += p1-q2;
             }
             else{
                 disty += q2-p1;
             }
          }
        }

        if(disty < min2)
        {
          min2 = disty;
          besty = i;
        }
      }

      vectors_x[x][y] += bestx;
      vectors_y[x][y] += besty;

      // ==========================================
      // UNROLL ITERATION 2: S = 2
      // ==========================================
      S = 2;
      min1 = 255*Bx*By;
      min2 = 255*Bx*By;

      /*For all candidate blocks in X dimension*/
      for(i=-S; i<S+1; i+=S)
      {
        distx = 0;

        /*For all pixels in the block - LOOP REVERSAL APPLIED*/
        for(k = Bx - 1; k >= 0; k--)
        {
          for(l = By - 1; l >= 0; l--)
          {
            // ---DRT: Read from local RAM ---
            p1 = local_current[k][l];
            p2 = local_previous[vectors_x[x][y] + i + k + p][vectors_y[x][y] + l + p];
            
            if(p1-p2>0){
                distx += p1-p2;
            }
            else{
                distx += p2-p1;
            }
          }
        }

        if(distx < min1)
        {
          min1 = distx;
          bestx = i;
        }
      }

      /*For all candidate blocks in Y dimension*/
      for(i=-S; i<S+1; i+=S)
      {
        disty = 0;

        /*For all pixels in the block - LOOP REVERSAL APPLIED*/
        for(k = Bx - 1; k >= 0; k--)
        {
          for(l = By - 1; l >= 0; l--)
          {
            // --- DRT: Read from local RAM ---
            p1 = local_current[k][l];
            q2 = local_previous[vectors_x[x][y] + k + p][vectors_y[x][y] + i + l + p];

            if(p1-q2>0){
                 disty += p1-q2;
             }
             else{
                 disty += q2-p1;
             }
          }
        }

        if(disty < min2)
        {
          min2 = disty;
          besty = i;
        }
      }

      vectors_x[x][y] += bestx;
      vectors_y[x][y] += besty;

      // ==========================================
      // UNROLL ITERATION 3: S = 1
      // ==========================================
      S = 1;
      min1 = 255*Bx*By;
      min2 = 255*Bx*By;

      /*For all candidate blocks in X dimension*/
      for(i=-S; i<S+1; i+=S)
      {
        distx = 0;

        /*For all pixels in the block - LOOP REVERSAL APPLIED*/
        for(k = Bx - 1; k >= 0; k--)
        {
          for(l = By - 1; l >= 0; l--)
          {
            // --- DRT: Read from local RAM ---
            p1 = local_current[k][l];
            p2 = local_previous[vectors_x[x][y] + i + k + p][vectors_y[x][y] + l + p];
            
            if(p1-p2>0){
                distx += p1-p2;
            }
            else{
                distx += p2-p1;
            }
          }
        }

        if(distx < min1)
        {
          min1 = distx;
          bestx = i;
        }
      }

      /*For all candidate blocks in Y dimension*/
      for(i=-S; i<S+1; i+=S)
      {
        disty = 0;

        /*For all pixels in the block - LOOP REVERSAL APPLIED*/
        for(k = Bx - 1; k >= 0; k--)
        {
          for(l = By - 1; l >= 0; l--)
          {
            // --- DRT: Read from local RAM ---
            p1 = local_current[k][l];
            q2 = local_previous[vectors_x[x][y] + k + p][vectors_y[x][y] + i + l + p];

            if(p1-q2>0){
                 disty += p1-q2;
             }
             else{
                 disty += q2-p1;
             }
          }
        }

        if(disty < min2)
        {
          min2 = disty;
          besty = i;
        }
      }

      vectors_x[x][y] += bestx;
      vectors_y[x][y] += besty;

    }
  }
}

int motion_vectors_x[N/Bx][M/By], motion_vectors_y[N/Bx][M/By], i, j;


void hal_entry(void) {

  // Code to initialize the DWT->CYCCNT register
    CoreDebug->DEMCR |= 0x01000000;
       ITM->LAR = 0xC5ACCE55;
       DWT->CYCCNT = 0;
       DWT->CTRL |= 1;

     /* Initialize your variables here */
    
     // Variables to hold the number of cycles
     unsigned int start_cycles, end_cycles, diff_cycles;

     unsigned int min_cycles = 0xFFFFFFFF; 
     unsigned int max_cycles = 0;
     unsigned int sum_cycles = 0;
     unsigned int avg_cycles;

      for(i = 0; i < 10; i++){
         /* Add timer code here */
        
         start_cycles = DWT->CYCCNT; // Cycles before execution
         
         phods_motion_estimation(current, previous, motion_vectors_x, motion_vectors_y);

         end_cycles = DWT->CYCCNT;  // Cycles after execution

         diff_cycles = end_cycles - start_cycles; // Calculate cycles of the iteration

         sum_cycles += diff_cycles;
         if (diff_cycles < min_cycles){
            min_cycles = diff_cycles;
         }
         if (diff_cycles > max_cycles){
            max_cycles = diff_cycles;
         }
      }

      // Calculate average number of cycles
      avg_cycles = sum_cycles / 10;

      // Convert cycles to seconds
      float freq = 240000000.0;
      float min_time = min_cycles / freq;
      float max_time = max_cycles / freq;
      float avg_time = avg_cycles / freq;

      while(1){}
}