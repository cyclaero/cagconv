### [ACTION REQUIRED] Your GitHub account, cyclaero, will soon require 2FA

Here is the deal: https://obsigna.com/articles/1693258424.html

---
 
# cagconv, sarconv and cyclasar

The time series of Global Temperature Anomalies which can be downloaded from the [NOAA site Climate at a Glance (CAG)](https://www.ncdc.noaa.gov/cag/global/time-series) and the time series of the Solar Active Regions which can be downloaded from [Solar Cycle Science](http://solarcyclescience.com/index.html) come with date formats which are unsuitable for mathematical series analysis.  
   
## The CAG file
In the NOAA file, the Year column looks like it contains normal numerical values, but the years and months are simply joined together character by character, and so 201812 means December 2018, and 201901 stands for the following month, January 2019. The problem here is that the numerical difference 201901-201812 is not 1 but 89. So the series with the character-wise formatted time values is not continuous, but jumps at each year change by 88+1. In order to get a time scale with continuous decimal years, the Year column needs to be converted.  
   
### The decimal year
The NOAA table lists calendar years/months. This means that the conversion must take into account the different length of February in normal and leap years.
Furthermore, the monthly mean of a temperature means that its base point is neither at the beginning nor at the end of the month, but of course in its middle.  
   
The year 2008 was a leap year, and the decimal time of the middle of the month of March 2008 is therefore 2008 + (31 + 29 + 15.5)/366 = 2008.206284.
On the other hand, in the normal year 2009, this is 2009 + (31 + 28 + 15.5)/365 = 2009.204110. The decimal difference is small but yet significant.  
   
`cagconv` does exactly that conversion on NOAA's CAG file.  

### Usage:
1. Compile `cagconv.c` on either of FreeBSD, Linux or macOS:  
   
    `cc -g0 -O3 cagconv.c -Wno-parentheses -lm -o cagconv`  
   
2. Download the monthly time series of the global surface temperature anomalies from the [NOAA site Climate at a Glance (CAG)](https://www.ncdc.noaa.gov/cag/global/time-series):  
   
   `curl -O https://www.ncdc.noaa.gov/cag/global/time-series/globe/land_ocean/all/12/1880-2021.csv`  
   
3. Convert the YYYYMM date literals to decimal years and write it out together with the temperature anomalies to the TSV output file:  
   
    `./cagconv 1880-2021.csv gta-1880-2021.tsv`  
   
4. Open the resulting TSV file with your favorite graphing and/or data analysis application, for example with [CVA](https://cyclaero.com/en/downloads/CVA)  
   
## The SAR file
Likewise the date column of daily time series of the Solar Active Regions comes with calendar dates whereby years, months and days are separated by spaces, and it must be converted to decimal years in similar fashion as NOAA's CAG file.  
   
`sarconv` does exactly that conversion on the SAR file of Solar Cycle Science.  

### Usage:
1. Compile `sarconv.c` on either of FreeBSD, Linux or macOS:  
   
   `cc -g0 -O3 sarconv.c -Wno-parentheses -lm -o sarconv`  
   
2. Download the daily time series of the sun's active regions from [Solar Cycle Science](http://solarcyclescience.com/index.html):  
   
   `curl -O http://solarcyclescience.com/AR_Database/daily_area.txt`  
   
3. Convert the YYYY MM DD date tuples to decimal years and write it out together with the daily sunspot areas to the TSV output file:  
   
   `./sarconv daily_area.txt sar-1880-2021.tsv`  
   
4. Open the resulting TSV file with your favorite graphing and/or data analysis application, for example with [CVA](https://cyclaero.com/en/downloads/CVA)  
   

## Fourier analysis of the SAR time series
Solar Active Regions are recorded in terms of the area of the [Sun](https://en.wikipedia.org/wiki/Sun) which is seized by [Sunspots](https://en.wikipedia.org/wiki/Sunspot). Sunspots are caused by magnetic field flux and its number and extend vary by the [11-year solar cycle](https://en.wikipedia.org/wiki/Solar_cycle). In general, cycles cry for [Fourier analysis](https://en.wikipedia.org/wiki/Fourier_analysis). Now, why not, let’s do it.  
   
`cyclasar` serves for this purpose when applied to the output of `sarconv`.
   
### Prerequisite:
   [FFTS - The Fastest Fourier Transform in the South](https://github.com/anthonix/ffts)  
    
### Usage:
1. Compile `cyclasar.c` on either of FreeBSD, Linux or macOS:  
   
   `cc -g0 -O3 cyclasar.c -Wno-parentheses -I/usr/local/include/ffts -L/usr/local/lib -lffts -lm -o cyclasar`  
   
2. Download the daily time series of solar acitve regions from [Solar Cycle Science](http://solarcyclescience.com/index.html):  
   
   `curl -O http://solarcyclescience.com/AR_Database/daily_area.txt`  
   
3. Convert the YYYY MM DD date tuples to decimal years and write it out together with the daily sunspot areas to the TSV output file:  
   
   `./sarconv daily_area.txt sar-1880-2021.tsv`  
   
4. Generate a spectrum of the time series:  
   
   `./cyclasar spectrum sar-1880-2021.tsv spectral-sar-1880-2021.tsv`  
   
5. Pass the time series through a digital filter:  
   
   `./cyclasar filter 0 0.001 10 sar-1880-2021.tsv filtered-sar-1880-2021.tsv`  
   
6. Open the resulting TSV files with your favorite graphing and/or data analysis application, for example with [CVA](https://cyclaero.com/en/downloads/CVA)  
