//  cyclasar.c
//  cyclasar
//
//  Created by Dr. Rolf Jansen on 2021-08-14.
//  Copyright © 2021 cyclaero.com. All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without modification,
//  are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
//  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
//  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
//  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
//  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
//  OF THE POSSIBILITY OF SUCH DAMAGE.
//
//
//  Prerequisite: FFTS - The Fastest Fourier Transform in the South
//
//  1. clone the ffts sources from the GitHub repository https://github.com/anthonix/ffts:
//
//     cd ~/install
//     git clone https://github.com/anthonix/ffts.git ffts
//     cd ffts
//     sed -e 's/CMAKE_COMPILER_IS_GNUCC/CMAKE_C_COMPILER_ID MATCHES "GNU|Clang"/g' -i ".orig" CMakeLists.txt
//     mkdir build; cd build
//     cmake -DDISABLE_DYNAMIC_CODE=ON -DENABLE_SHARED=ON ..
//     make
//     sudo make install clean
//
//  Usage:
//
//  1. Compile this file on either of FreeBSD, Linux or macOS:
//
//     cc -g0 -O3 cyclasar.c -Wno-parentheses -I/usr/local/include/ffts -L/usr/local/lib -lffts -lm -o cyclasar
//
//  2. Download the daily time series of the sun's acitve regions
//     from solarcyclescience.com - http://solarcyclescience.com/AR_Database/daily_area.txt
//
//     curl -O http://solarcyclescience.com/AR_Database/daily_area.txt
//
//  3. Convert the YYYY MM DD date tuples to decimal years and write it
//     out together with the daily sunspot areas to the TSV output file:
//
//     ./sarconv daily_area.txt sar-1880-2021.tsv
//
//  4. Generate a spectrum of the time series:
//
//     ./cyclasar spectrum sar-1880-2021.tsv spectral-sar-1880-2021.tsv
//
//  5. Pass the time series through a digital filter:
//
//     ./cyclasar filter 0 0.001 10 sar-1880-2021.tsv filtered-sar-1880-2021.tsv
//
//  6. Open the resulting TSV files with your favorite graphing and/or data analysis application,
//     for example with CVA - https://cyclaero.com/en/downloads/CVA



#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <x86intrin.h>
#include <math.h>

#include "ffts.h"


int usage(void)
{
   printf(" Usage:\n"
          "   ./cyclasar <method> [filter args] <infile> <outfile>\n"
          "     method:        either of 'spectrum' or 'filter'\n"
          "     filter args:   <low> <high> <kT>  (apply for the filter method only)\n"
          "             low:   0 .. +inf -- frequency in unit of the reciprocal base time\n"
          "            high:   0 .. +inf -- frequency in unit of the reciprocal base time\n"
          "              kT:   0 .. 100  -- blur of the cut(s) in percent of the passed frequency range\n"
          "\n");

   return 1;
}


static inline float sqrf(float x)
{
   return x*x;
}

float blurfunc(float f, float lowCut, float highCut, float kT, bool invert)
{
   float result;

   if (lowCut == highCut)
      return 0;

   if (0 < kT && kT <= 100)
      if (lowCut == 0)
         result = 1/(1 + expf((f - highCut)/kT));
      else
         result = 1/(1 + expf((f - highCut)/kT))/(1 + expf((lowCut - f)/kT));

   else if (lowCut <= f && (f <= highCut || isinf(highCut)))
      result = 1;

   else
      result = 0;

   return (invert) ? 1 - result : result;
}


enum { spectrum = 1, filter = 0 };

int main(int argc, const char *argv[])
{
   FILE *infile, *outfile;

   int   rc      = 0,
         argidx  = 1,
         method  = 0;

   float lowCut  = 0.0f,
         highCut = INFINITY,
         kT      = 0.0f;

   if (argc == 4 && strcmp(argv[argidx], "spectrum") == 0)
      method = spectrum;

   else if (argc == 7 && strcmp(argv[argidx], "filter") == 0)
   {
      method = filter;
      lowCut  = strtof(argv[++argidx], NULL);
      highCut = strtof(argv[++argidx], NULL);
      kT      = strtof(argv[++argidx], NULL);

      if ( lowCut < 0 || isnan(lowCut)
       || highCut < 0 || isnan(highCut)
       || kT < 0 || 100 < kT)
         return usage();
   }

   else
      return usage();

   if (infile = (*(uint16_t *)argv[++argidx] == *(uint16_t *)"-")
                 ? stdin
                 : fopen(argv[argidx], "r"))
   {
      if (outfile = (*(uint16_t *)argv[++argidx] == *(uint16_t *)"-")
                    ? stdout
                    : fopen(argv[argidx], "w"))
      {
         int i, n = 65536;

         char  *line, buf[256];
         while (*(line = fgets(buf, 256, infile)) == '#')
            if (strstr(line, "# Point count: "))
               n = (int)strtol(line+15, NULL, 10);

         if (n > 2)
         {
            // the line with the column titles has just been read in, and will be implicitely skiped below
            float *time = malloc(n*sizeof(float));

            float *input, *output;
            posix_memalign((void **)&input,  32, 2*n*sizeof(float));
            posix_memalign((void **)&output, 32, 2*n*sizeof(float));
            for (i = 0; i < n; i++)
            {
               line = fgets(buf, 256, infile);
               time[i]        = strtof(line, &line);     // first column is the time - simply pass through
               input[2*i    ] = strtof(line, &line);     // second column is the daily total active area of the sun
               input[2*i + 1] = 0;
            }

            // trend correction
            bool   trend;
            double a = 0, b = 0, d;
            for (i = 0; i < 10; i++)
               a += input[2*i];
            for (i = n-10; i < n; i++)
               b += input[2*i];
            a /= 10;
            b /= 10;
            d = fabsf(input[2*(n-1)] - input[0]);
            if (trend = (d > fabs(a - input[0]) || d > fabs(b - input[2*(n-1)])))
            {
               a = input[0];
               b = (input[2*(n-1)] - a)/n;
               for (i = 0; i < n; i++)
                  input[2*i] -= a + b*i;
            }

            ffts_plan_t *p = ffts_init_1d(n, FFTS_FORWARD);
            ffts_execute(p, input, output);
            ffts_free(p);

            if (method == spectrum)
            {
               fprintf(outfile, "freq/1/d\tAt/µhsp\n");
               int n2 = n >> 1;
               for (i = 0; i <= n2; i++)
                  fprintf(outfile, "%.9f\t%.9f\n", (double)i/n, sqrtf(sqrf(output[2*i]) + sqrf(output[2*i+1]))/n2);
            }

            else if (method == filter)
            {
               bool invert;
               if (invert = lowCut > highCut)
                  d = lowCut, lowCut = highCut, highCut = d;
               kT *= (highCut - lowCut)/100;


               int n2p1 = ((n & 0x1) ? (n + 1) >> 1 : n >> 1) + 1;

               // positive frequencies
               for (i = 0; i < n2p1; i++)
               {
                  float bf = blurfunc((float)i/(n - 1), lowCut, highCut, kT, invert);
                  output[2*i    ] *= bf;
                  output[2*i + 1] *= bf;
               }

               // negative frequencies
               for (i = n2p1; i < n; i++)
               {
                  float bf = blurfunc((float)(n - i)/(n - 1), lowCut, highCut, kT, invert);
                  output[2*i    ] *= bf;
                  output[2*i + 1] *= bf;
               }

               p = ffts_init_1d(n, FFTS_BACKWARD);
               ffts_execute(p, output, input);
               ffts_free(p);

               fprintf(outfile, "t/a\tAt/µhsp\n");
               if (trend)
                  for (i = 0; i < n; i++)
                     fprintf(outfile, "%.9f\t%.9f\n", time[i], input[2*i]/n + a + b*i);
               else
                  for (i = 0; i < n; i++)
                     fprintf(outfile, "%.9f\t%.9f\n", time[i], input[2*i]/n);
            }

            free(output);
            free(input);
            free(time);
         }

         else
         {
            printf("Invalid number of Points\n");
            rc = usage();
         }

         if (outfile != stdout)
            fclose(outfile);
      }

      else
         rc = usage();

      if (infile != stdin)
         fclose(infile);
   }

   else
      rc = usage();

   return rc;
}
