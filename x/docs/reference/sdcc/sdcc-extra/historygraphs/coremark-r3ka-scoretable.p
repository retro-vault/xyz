set output "coremark-r3ka-score.svg"
set terminal svg size 640,480
set style data lines
set key bottom right
set xlabel "revision"
set ylabel "Coremark iterations / s"
set arrow from 12085, 2.238 to 12085, 2.138
set label "4.1.0" at 12085, 2.238
set arrow from 13131, 3.603 to 13131, 3.503
set label "4.2.0" at 13131, 3.603
set arrow from 14208, 3.443 to 14208, 3.243
set label "4.3.0" at 14208, 3.443
set arrow from 14648, 4.051 to 14648, 3.951
set label "4.4.0" at 14648, 4.051
set arrow from 15246, 3.906 to 15246, 3.806
set label "4.5.0" at 15246, 3.906
plot "coremark-r3ka-scoretable" using 1:4 title "default", "coremark-r3ka-scoretable" using 1:2 title "size", "coremark-r3ka-scoretable" using 1:3 title "speed"

