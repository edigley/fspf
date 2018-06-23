##set terminal postscript portrait enhanced mono dashed lw 1 "Helvetica" 14 
set term postscript 
set output filename
set title "Genetic Algorithm Error"
set xlabel "time(s)"
set ylabel "Error"
set grid
plot inputFile using 1:2 title JobID with linespoints
