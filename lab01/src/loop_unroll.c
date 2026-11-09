#include "hal_data.h"
#include "stdio.h"
#include "string.h"
#include "images.h"


#define N 144    /*Frame dimension for QCIF format*/
#define M 176     /*Frame dimension for QCIF format*/

#define B 16      /*Block size*/

#define p 7       /*Search space. Restricted in a [-p,p] region around the
                    original location of the block.*/


void phods_motion_estimation(const int current[N][M], const int previous[N][M],
        int vectors_x[N/B][M/B],int vectors_y[N/B][M/B])
{
  int x, y, i, j, k, l, p1, p2, q2, distx, disty, S, min1, min2, bestx, besty;

  distx = 0;
  disty = 0;

  /*Initialize the vector motion matrices*/
  for(i=0; i<N/B; i++)
  {
    for(j=0; j<M/B; j++)
    {
      vectors_x[i][j] = 0;
      vectors_y[i][j] = 0;
    }
  }

  /*For all blocks in the current frame*/
  for(x=0; x<N/B; x++)
  {
    for(y=0; y<M/B; y++)
    {
      // ==========================================
      // UNROLL ITERATION 1: S = 4
      // ==========================================
      S = 4;
      min1 = 255*B*B;
      min2 = 255*B*B;

      /*For all candidate blocks in X dimension*/
      for(i=-S; i<S+1; i+=S)
      {
        distx = 0;
        /*For all pixels in the block*/
        for(k=0; k<B; k++)
        {
          for(l=0; l<B; l++)
          {
            p1 = current[B*x+k][B*y+l];
            if((B*x + vectors_x[x][y] + i + k) < 0 ||
                (B*x + vectors_x[x][y] + i + k) > (N-1) ||
                (B*y + vectors_y[x][y] + l) < 0 ||
                (B*y + vectors_y[x][y] + l) > (M-1))
            {
              p2 = 0;
            } else {
              p2 = previous[B*x+vectors_x[x][y]+i+k][B*y+vectors_y[x][y]+l];
            }
            if(p1-p2>0){ distx += p1-p2; }
            else{ distx += p2-p1; }
          }
        }
        if(distx < min1) { min1 = distx; bestx = i; }
      }

      /*For all candidate blocks in Y dimension*/
      for(i=-S; i<S+1; i+=S)
      {
        disty = 0;
        /*For all pixels in the block*/
        for(k=0; k<B; k++)
        {
          for(l=0; l<B; l++)
          {
            p1 = current[B*x+k][B*y+l];
            if((B*x + vectors_x[x][y] + k) <0 ||
                (B*x + vectors_x[x][y] + k) > (N-1) ||
                (B*y + vectors_y[x][y] + i + l) < 0 ||
                (B*y + vectors_y[x][y] + i + l) > (M-1))
            {
              q2 = 0;
            } else {
              q2 = previous[B*x+vectors_x[x][y]+k][B*y+vectors_y[x][y]+i+l];
            }
            if(p1-q2>0){ disty += p1-q2; }
            else{ disty += q2-p1; }
          }
        }
        if(disty < min2) { min2 = disty; besty = i; }
      }
      vectors_x[x][y] += bestx;
      vectors_y[x][y] += besty;

      // ==========================================
      // UNROLL ITERATION 2: S = 2
      // ==========================================
      S = 2;
      min1 = 255*B*B;
      min2 = 255*B*B;

      /*For all candidate blocks in X dimension*/
      for(i=-S; i<S+1; i+=S)
      {
        distx = 0;
        /*For all pixels in the block*/
        for(k=0; k<B; k++)
        {
          for(l=0; l<B; l++)
          {
            p1 = current[B*x+k][B*y+l];
            if((B*x + vectors_x[x][y] + i + k) < 0 ||
                (B*x + vectors_x[x][y] + i + k) > (N-1) ||
                (B*y + vectors_y[x][y] + l) < 0 ||
                (B*y + vectors_y[x][y] + l) > (M-1))
            { p2 = 0; } else {
              p2 = previous[B*x+vectors_x[x][y]+i+k][B*y+vectors_y[x][y]+l];
            }
            if(p1-p2>0){ distx += p1-p2; }
            else{ distx += p2-p1; }
          }
        }
        if(distx < min1) { min1 = distx; bestx = i; }
      }

      /*For all candidate blocks in Y dimension*/
      for(i=-S; i<S+1; i+=S)
      {
        disty = 0;
        /*For all pixels in the block*/
        for(k=0; k<B; k++)
        {
          for(l=0; l<B; l++)
          {
            p1 = current[B*x+k][B*y+l];
            if((B*x + vectors_x[x][y] + k) <0 ||
                (B*x + vectors_x[x][y] + k) > (N-1) ||
                (B*y + vectors_y[x][y] + i + l) < 0 ||
                (B*y + vectors_y[x][y] + i + l) > (M-1))
            { q2 = 0; } else {
              q2 = previous[B*x+vectors_x[x][y]+k][B*y+vectors_y[x][y]+i+l];
            }
            if(p1-q2>0){ disty += p1-q2; }
            else{ disty += q2-p1; }
          }
        }
        if(disty < min2) { min2 = disty; besty = i; }
      }
      vectors_x[x][y] += bestx;
      vectors_y[x][y] += besty;

      // ==========================================
      // UNROLL ITERATION 3: S = 1
      // ==========================================
      S = 1;
      min1 = 255*B*B;
      min2 = 255*B*B;

      /*For all candidate blocks in X dimension*/
      for(i=-S; i<S+1; i+=S)
      {
        distx = 0;
        /*For all pixels in the block*/
        for(k=0; k<B; k++)
        {
          for(l=0; l<B; l++)
          {
            p1 = current[B*x+k][B*y+l];
            if((B*x + vectors_x[x][y] + i + k) < 0 ||
                (B*x + vectors_x[x][y] + i + k) > (N-1) ||
                (B*y + vectors_y[x][y] + l) < 0 ||
                (B*y + vectors_y[x][y] + l) > (M-1))
            { p2 = 0; } else {
              p2 = previous[B*x+vectors_x[x][y]+i+k][B*y+vectors_y[x][y]+l];
            }
            if(p1-p2>0){ distx += p1-p2; }
            else{ distx += p2-p1; }
          }
        }
        if(distx < min1) { min1 = distx; bestx = i; }
      }

      /*For all candidate blocks in Y dimension*/
      for(i=-S; i<S+1; i+=S)
      {
        disty = 0;
        /*For all pixels in the block*/
        for(k=0; k<B; k++)
        {
          for(l=0; l<B; l++)
          {
            p1 = current[B*x+k][B*y+l];
            if((B*x + vectors_x[x][y] + k) <0 ||
                (B*x + vectors_x[x][y] + k) > (N-1) ||
                (B*y + vectors_y[x][y] + i + l) < 0 ||
                (B*y + vectors_y[x][y] + i + l) > (M-1))
            { q2 = 0; } else {
              q2 = previous[B*x+vectors_x[x][y]+k][B*y+vectors_y[x][y]+i+l];
            }
            if(p1-q2>0){ disty += p1-q2; }
            else{ disty += q2-p1; }
          }
        }
        if(disty < min2) { min2 = disty; besty = i; }
      }
      vectors_x[x][y] += bestx;
      vectors_y[x][y] += besty;

    }
  }
}


void hal_entry(void) {

	// Code to initialize the DWT->CYCCNT register
    CoreDebug->DEMCR |= 0x01000000;
       ITM->LAR = 0xC5ACCE55;
       DWT->CYCCNT = 0;
       DWT->CTRL |= 1;

	   /* Initialize your variables here */
     int motion_vectors_x[N/B][M/B], motion_vectors_y[N/B][M/B], i, j;

     // variables to hold the number of cycles
     unsigned int start_cycles, end_cycles, diff_cycles;

	   unsigned int min_cycles = 0xFFFFFFFF; 
     unsigned int max_cycles = 0;
     unsigned int sum_cycles = 0;
     unsigned int avg_cycles;

      for(i = 0; i < 10; i++){
		 /* Add timer code here */
        
         start_cycles = DWT->CYCCNT; // cycles before execution
         
         phods_motion_estimation(current, previous, motion_vectors_x, motion_vectors_y);

         end_cycles = DWT->CYCCNT;  // cycles before execution

         diff_cycles = end_cycles - start_cycles; // culculate cycles of the iteration

         sum_cycles += diff_cycles;
         if (diff_cycles < min_cycles){
            min_cycles = diff_cycles;
         }
         if (diff_cycles > max_cycles){
            max_cycles = diff_cycles;
         }
      }

      // calculater average number of cycles
      avg_cycles = sum_cycles / 10;

      // convert cycles to seconds
      float freq = 240000000.0;
      float min_time = min_cycles / freq;
      float max_time = max_cycles / freq;
      float avg_time = avg_cycles / freq;

      while(1){}
}
