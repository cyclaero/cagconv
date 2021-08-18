//  sarconv.c
//  sarconv
//
//  Created by Dr. Rolf Jansen on 2021-08-04.
//  Copyright © 2021 Dr. Rolf Jansen. All rights reserved.
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
//  Usage:
//
//  1. Compile this file on either of FreeBSD, Linux or macOS:
//
//     cc -g0 -O3 sarconv.c -Wno-parentheses -lm -o sarconv
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
//  4. Open the TSV file with your favorite graphing and/or data analysis application,
//     for example with CVA - https://cyclaero.com/en/downloads/CVA


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

//                           -    1    2     3     4      5      6      7      8      9     10     11     12
double commYearSteps[13] = {0.0, 0.0, 31.0, 59.0, 90.0, 120.0, 151.0, 181.0, 212.0, 243.0, 273.0, 304.0, 334.0};
double leapYearSteps[13] = {0.0, 0.0, 31.0, 60.0, 91.0, 121.0, 152.0, 182.0, 213.0, 244.0, 274.0, 305.0, 335.0};

static inline bool isLeapYear(int year)
{
   return (year % 4)
          ? false
          : (year % 100)
            ? true
            : (year % 400)
              ? false
              : true;
}

static inline unsigned char *skip(unsigned char *s)
{
   for (;;)
      switch (*s)
      {
         case '\t'...'\r':
         case ' ':
            s++;
            break;

         default:
            return s;
      }
}


static inline double linpol(double t, double t1, double y1, double t2, double y2)
{
   return (y2 - y1)/(t2 - t1)*(t - t1) + y1;
}

int main(int argc, char *const argv[])
{
   FILE *txt, *tsv;

   if (txt = (*(uint16_t *)argv[1] == *(uint16_t *)"-")
             ? stdin
             : fopen(argv[1], "r"))
   {
      if (tsv = (*(uint16_t *)argv[2] == *(uint16_t *)"-")
                ? stdout
                : fopen(argv[2], "w"))
      {
         unsigned
         char *line;
         char  data[256];

         // Copy over blank and descriptive text lines to the output file.
         while ((line = (unsigned char *)fgets(data, 256, txt))
             && (*(line = skip(line)) < '0' || '9' < *line))
            fprintf(tsv, "# %s", line);

         if (line)
         {
            int n = 0, cap = 65536;
            double *t  = malloc(cap*sizeof(double));
            double *at = malloc(cap*sizeof(double));
            double *an = malloc(cap*sizeof(double));
            double *as = malloc(cap*sizeof(double));

            do
               if ('0' <= *line && *line <= '9' || *line == '-')
               {
                  // Read the TXT data.
                  // Convert the YYYY MM DD date format to decimal years and write it
                  // out together with the daily sunspot areas to the TSV foutput file.
                  char *q, *p = (char *)line;
                  int   y = (int)strtol(p,     &q, 10),
                        m = (int)strtol(p = q, &q, 10),
                        d = (int)strtol(p = q, &q, 10);

               // if ((1931 < y || y == 1931 && (4 < m || m == 4 && d >= 15))  && y <= 2020)    // only extract 32768 tuples for doing FFT
                  if (y >= 1880)                                                                // start at 1880
                  {
                     t[n] = y + ((isLeapYear(y))
                                ? (leapYearSteps[m] + d - 0.5)/366.0
                                : (commYearSteps[m] + d - 0.5)/365.0);

                     at[n] = strtod(p = q, &q);
                     an[n] = strtod(p = q, &q);
                     as[n] = strtod(p = q, &q);

                     if (++n > cap)
                     {
                        cap += cap;
                        t  = realloc(t,  cap*sizeof(double));
                        at = realloc(at, cap*sizeof(double));
                        an = realloc(an, cap*sizeof(double));
                        as = realloc(as, cap*sizeof(double));
                     }
                  }
                  else if (y == 0 && q == (char *)line)
                     break;                                    // a number conversion error occurred
               }
            while ((line = (unsigned char *)fgets(data, 256, txt))
                && *(line = skip(line)));

            int i, k, m;

            // back-skip final zeros
            for (m = n-1; m >= 0; m--)
               if (at[m] > 0.0 || an[m] > 0.0 || as[m] > 0.0)
                  break;

            // skip initial zeros
            for (++m, i = 0; i < m; i++)
               if (at[i] > 0.0 || an[i] > 0.0 || as[i] > 0.0)
                  break;

            fprintf(tsv, "# Time base:   1\n"
                         "# Time unit:   d\n"
                         "# Point count: %d\n", m-i);

            // Write the column header using SI formular symbols and units.
            // - the formular symbol of time is 't', the unit symbol of year is 'a'
            // - formular symbol of area is 'A' in millionths of a hemisphere 'µhsp'
            fprintf(tsv, "t/a\tAt/µhsp\tAn/µhsp\tAs/µhsp\n");

            double tt1 = t[0], tn1 = t[0], ts1 = t[0],
                   at1 = 0.0,  an1 = 0.0,  as1 = 0.0;

            for (; i < m; i++)
            {
               if (at[i] < 0.0)
               {
                  for (k = i; k < n && at[k] < 0.0; k++);
                  at[i] = linpol(t[i], tt1, at1, t[k], at[k]);
               }
               else
                  tt1 = t[i], at1 = at[i];

               if (an[i] < 0.0)
               {
                  for (k = i; k < n && an[k] < 0.0; k++);
                  an[i] = linpol(t[i], tn1, an1, t[k], an[k]);
               }
               else
                  tn1 = t[i], an1 = an[i];

               if (as[i] < 0.0)
               {
                  for (k = i; k < n && as[k] < 0.0; k++);
                  as[i] = linpol(t[i], ts1, as1, t[k], as[k]);
               }
               else
                  ts1 = t[i], as1 = as[i];

               fprintf(tsv, "%.7f\t%.1f\t%.1f\t%.1f\n", t[i], at[i], an[i], as[i]);
            }

            free(as);
            free(an);
            free(at);
            free(t);
         }

         if (tsv != stdout)
            fclose(tsv);
      }

      if (txt != stdin)
         fclose(txt);
   }

   return 0;
}
