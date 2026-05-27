set output "dhrystone-r3ka-score.svg"
set terminal svg size 640,480
set style data lines
set key bottom right
set xlabel "revision"
set ylabel "Dhrystones per second"
set arrow from 12085, 4207 to 12085, 4107
set label "4.1.0" at 12085, 4207
set arrow from 13131, 5025 to 13131, 4925
set label "4.2.0" at 13131, 5025
set arrow from 14208, 5095 to 14208, 4995
set label "4.3.0" at 14208, 5095
set arrow from 14648, 5642 to 14648, 5542
set label "4.4.0" at 14648, 5642
set arrow from 15246, 5427 to 15246, 5327
set label "4.5.0" at 15246, 5427
plot "dhrystone-r3ka-scoretable" using 1:4 title "default", "dhrystone-r3ka-scoretable" using 1:2 title "size", "dhrystone-r3ka-scoretable" using 1:3 title "speed"

