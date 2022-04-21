//  eopconv.c
//  eopconv
//
//  Created by Dr. Rolf Jansen on 2022-04-20.
//  Copyright © 2019-2022 Dr. Rolf Jansen. All rights reserved.
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
//     cc -g0 -O3 eopconv.c -Wno-parentheses -lm -o eopconv
//
//  2. Download the EOP(IERS) C 01 series of the earth orientation parameters
//     from IERS's site - https://datacenter.iers.org
//
//     curl -O https://datacenter.iers.org/data/186/eopc01.iau2000.1846-now
//
//  3. Convert the days of the besselian year to decimal years and interpolate the data
//     in the range of 1846 to 1889 from 10 to 20 intervals per year and write it out together
//     with the earth orientation parameters to the TSV foutput file:
//
//     ./eopconv eopc01.iau2000.1846-now eop-1846-2022.tsv
//
//  4. Open the TSV file with your favorite graphing and/or data analysis application,
//     for example with CVA - https://cyclaero.com/en/downloads/CVA


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>


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
         char  data[512];

         fprintf(tsv, "# Time base:   18.26210994\n"
                      "# Time unit:   d\n\n");

         // Copy over the first descriptive text lines to the output file.
         bool writeHeader = true;
         while ((line = (unsigned char *)fgets(data, 512, txt)) && *line == '#')
            if (writeHeader)
            {
               fprintf(tsv, "# %s\n", skip(line+1));
               writeHeader = false;
            }

         line = skip(line);
         if (line)
         {
            // Write the column header using SI formular symbols and units.
            // - the formular symbol of time is 't'
            //   the unit symbol of year is 'a'
            // - the unit symbol of arc second is ″
            fprintf(tsv, "t/a\tx/″\ty/″\n");

            char  *p, *q ;
            double d00 = 0.0, d0 = 0.0, d, x0 = 0.0, x, y0 = 0.0, y;
            do
               if ('0' <= *line && *line <= '9' || *line == '-')
               {
                  // Read the data.
                  // Convert the besselian days to decimal years and write it
                  // out together with the earth orientation parameters.
                  d = strtod((char *)line, &q);
                  x = strtod(p = q, &q);
                  y = strtod(p = q, &q);

                  if (d00 == 0.0)
                     d00 = d0 = d, x0 = x, y0 = y;

                  else if (d < 11368.0)
                  {
                     d0 = (d + d0)/2.0;
                     x0 = (x + x0)/2.0;
                     y0 = (y + y0)/2.0;
                     fprintf(tsv, "%.6f\t%.6f\t%.6f\n", (d0 - d00)/365.242198781 + 1846.0, x0, y0);
                     d0 = d, x0 = x, y0 = y;
                  }

                  fprintf(tsv, "%.6f\t%.6f\t%.6f\n", (d - d00)/365.242198781 + 1846.0, x, y);
               }
            while ((line = (unsigned char *)fgets(data, 512, txt))
                && *(line = skip(line)));
         }

         if (tsv != stdout)
            fclose(tsv);
      }

      if (txt != stdin)
         fclose(txt);
   }

   return 0;
}
