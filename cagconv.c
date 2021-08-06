//  cagconv.c
//  cagconv
//
//  Created by Dr. Rolf Jansen on 2019-06-07.
//  Copyright © 2019-2021 Dr. Rolf Jansen. All rights reserved.
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
//     cc -g0 -O3 -march=native cagconv.c -Wno-parentheses -lm -o cagconv
//
//  2. Download the monthly time series of the global surface temperature anomalies
//     from NOAA's site Climate at a Glance - https://www.ncdc.noaa.gov/cag/global/time-series
//
//     curl -O https://www.ncdc.noaa.gov/cag/global/time-series/globe/land_ocean/all/12/1880-2021.csv
//
//  3. Convert the YYYYMM date literals to decimal years and write it
//     out together with the temperature anomalies to the TSV foutput file:
//
//     ./cagconv 1880-2021.csv gta-1880-2021.tsv
//
//  4. Open the TSV file with your favorite graphing and/or data analysis application,
//     for example with CVA -- https://cyclaero.com/en/downloads/articles/1571499655.html.


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

double normYearMids[13] = {182.5, 15.5, 45.0, 74.5, 105.0, 135.5, 166.0, 196.5, 227.5, 258.0, 288.5, 319.0, 349.5};
double leapYearMids[13] = {183.0, 15.5, 45.5, 75.5, 106.0, 136.5, 167.0, 197.5, 228.5, 259.0, 289.5, 320.0, 350.5};

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

int main(int argc, char *const argv[])
{
   FILE *csv, *tsv;

   if (csv = (*(uint16_t *)argv[1] == *(uint16_t *)"-")
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
         while ((line = (unsigned char *)fgets(data, 256, csv))
             && (*(line = skip(line)) < '0' || '9' < *line))
            fprintf(tsv, "# %s", line);

         if (line)
         {
            // Write the column header using SI formular symbols and units.
            // - the formular symbol of time is 't'
            //   the unit symbol of year is 'a'
            // - formular symbol of celsius temperatures is '𝜗' (lower case theta)
            //   in general, differences are designated by '∆' (capital letter delta)
            //   the unit symbol of (Celsius) centigrade is '°C'
            fprintf(tsv, "t/a\t∆𝜗/°C\n");

            do
               if ('0' <= *line && *line <= '9' || *line == '-')
               {
                  // Read the CSV data.
                  // Convert the YYYYMM date literals to decimal years and write it
                  // out together with the temperature anomalies to the TSV foutput file.
                  char  *p, *q;
                  double ym = strtod((char *)line, &p);
                  if (ym > 0.0)                             // missing values are designated by -999
                  {
                     ym /= 100.0;
                     int    y = (int)lround(floor(ym)),
                            m = (int)lround((ym - y)*100.0);
                     double t = y + ((isLeapYear(y))
                                    ? leapYearMids[m]/366.0
                                    : normYearMids[m]/365.0);

                     while (*p && *p++ != ',');
                     double an = strtod(q = p, &p);
                     if (an > -999.0)                       // missing values are designated by -999
                        if (p != q)
                           fprintf(tsv, "%.5f\t%.3f\n", t, an);
                        else
                           break;
                  }
                  else if (ym == 0.0 && p == (char *)line)
                     break;                                 // a number conversion error occurred
               }
            while ((line = (unsigned char *)fgets(data, 256, csv))
                && *(line = skip(line)));
         }

         if (tsv != stdout)
            fclose(tsv);
      }

      if (csv != stdin)
         fclose(csv);
   }

   return 0;
}
