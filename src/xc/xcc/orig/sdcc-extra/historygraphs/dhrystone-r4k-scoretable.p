set output "dhrystone-r4k-score.svg"
set terminal svg size 640,480
set style data lines
set key bottom right
set xlabel "revision"
set ylabel "Dhrystones per second"
set arrow from 15246, 5427 to 15246, 5327
set label "4.5.0" at 15246, 5427
plot "dhrystone-r4k-scoretable" using 1:4 title "default", "dhrystone-r4k-scoretable" using 1:2 title "size", "dhrystone-r4k-scoretable" using 1:3 title "speed"

