set output "coremark-r4k-score.svg"
set terminal svg size 640,480
set style data lines
set key bottom right
set xlabel "revision"
set ylabel "Coremark iterations / s"
plot "coremark-r4k-scoretable" using 1:4 title "default", "coremark-r4k-scoretable" using 1:2 title "size", "coremark-r4k-scoretable" using 1:3 title "speed"

