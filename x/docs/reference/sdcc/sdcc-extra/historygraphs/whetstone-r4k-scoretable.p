set output "whetstone-r4k-score.svg"
set terminal svg size 640,480
set style data lines
set key bottom right
set xlabel "revision"
set ylabel "Whetstone KIPS"
set arrow from 15246, 40.758 to 15246, 40.558
set label "4.5.0" at 15246, 40.758
plot "whetstone-r4k-scoretable" using 1:4 title "default", "whetstone-r4k-scoretable" using 1:2 title "size", "whetstone-r4k-scoretable" using 1:3 title "speed"

