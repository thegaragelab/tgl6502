10  PRINT "Calculating the first 100 prime numbers."
20  PRINT "This could take a while ..."
30  REM --- Set up the list of primes and pre-populate it
40  DIM primes(100)
50  primes(0) = 2
60  primes(1) = 3
80  index = 2
90  value = 3
100 REM --- Main loop to test each value
110 IF index = 100 GOTO 240
120 value = value + 1
130 test = 0
140 REM --- Check against known primes
150 remain = (value - INT(value / primes(test)) * primes(test))
160 IF remain = 0 GOTO 120
170 test = test + 1
180 IF test < index GOTO 150
190 REM --- Found a new prime
200 primes(index) = value
210 PRINT "Found prime #", index, " - ", value
220 index = index + 1
230 GOTO 110
240 REM --- All done
250 PRINT "Finished!"

