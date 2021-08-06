# cagconv & sarconv

The time series of Global Temperature Anomlaies which can be downloaded from the [NOAA site Climate at a Glance (CAG)](https://www.ncdc.noaa.gov/cag/global/time-series) and the time seires
of the Solar Active Regions which can be downloaded from [Solar Cycle Science](http://solarcyclescience.com/index.html) come with date fomats which are unsuitable for mathematical series analysis.  
   
## The CAG file
In the NOAA file, the Year column looks like it contains normal numerical values, but the years and months are simply joined together character by character, and so 201812 means December 2018
and 201901 stands for the following month, January 2019. The problem here is that the numerical difference 201901-201812 is not 1 but 89. So the series with the character-wise formatted time values
is not continuous, but jumps at each year change by 88+1. In order to get a time scale with continuous decimal years, the Year column needs to be converted.  
   
### The decimal year
The NOAA table lists calendar years/months. This means that the conversion must take into account the different length of February in normal and leap years.
Furthermore, the monthly mean of a temperature means that its base point is neither at the beginning nor at the end of the month, but of course in it's middle.  
   
The year 2008 was a leap year, and the decimal time of the middle of the month of March 2008 is therefore 2008 + (31 + 29 + 15.5)/366 = 2008.206284.
On the other hand, in the normal year 2009, this is 2009 + (31 + 28 + 15.5)/365 = 2009.204110. The decimal difference is small but yet significant.  
   
`cagconv` does exactly that conversion on NOAA's CAG file.

### Usage:

1. Compile this file on either of FreeBSD, Linux or macOS:  
   
    `cc -g0 -O3 -march=native cagconv.c -Wno-parentheses -lm -o cagconv`  
   
2. Download the monthly time series of the global surface temperature anomalies from the [NOAA site Climate at a Glance (CAG)](https://www.ncdc.noaa.gov/cag/global/time-series)
   
   `curl -O https://www.ncdc.noaa.gov/cag/global/time-series/globe/land_ocean/all/12/1880-2021.csv`  
   
3. Convert the YYYYMM date literals to decimal calendar years and write it out together with the temperature anomalies to the TSV foutput file:
   
    `./cagconv 1880-2021.csv 1880-2021.tsv`  
   
4. Open the TSV file with you favorite graphing and/or data analysis application, for example with [CVA](https://cyclaero.com/en/downloads/articles/1571499655.html).  
   
## The SAR file
Likewise the date column of daily time series of the Solar Active Regions comes with calendar dates separated by spaces, and it must be converted to decimal years in similar fashion as NOAA's CAG file.  
   
`sarconv` does exactly that conversion on the SAR file of Solar Cycle Science.  
   
### Usage:
1. Compile this file on either of FreeBSD, Linux or macOS:  
   
   `cc -g0 -O3 -march=native sarconv.c -Wno-parentheses -lm -o sarconv`  
   
2. Download the daily time series of the sun's acitve regions from [Solar Cycle Science](http://solarcyclescience.com/index.html)  
   
   `curl -O http://solarcyclescience.com/AR_Database/daily_area.txt`  
   
3. Convert the YYYY MM DD date tupels to decimal calendar years and write it out together with the daily sunspot areas to the TSV foutput file:  
   
   `./sarconv 1daily_area.txt 1880-2021.tsv`  
   
4. Open the TSV file with you favorite graphing and/or data analysis application, for example with [CVA](https://cyclaero.com/en/downloads/articles/1571499655.html).
