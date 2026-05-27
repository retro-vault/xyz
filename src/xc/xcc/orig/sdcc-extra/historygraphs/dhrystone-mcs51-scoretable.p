set output "dhrystone-mcs51-score.svg"
set terminal svg size 640,480
set style data lines
set key bottom right
set xlabel "revision"
set ylabel "Dhrystones per second"
set arrow from 9618, 6070 to 9618, 6060
set label "3.6.0" at 9618, 6070
set arrow from 10233, 6073 to 10233, 6063
set label "3.7.0" at 10233, 6073
set arrow from 10582, 5983 to 10582, 5973
set label "3.8.0" at 10582, 5983
set arrow from 11214, 5978 to 11214, 5968
set label "3.9.0" at 11214, 5978
set arrow from 11533, 5963 to 11533, 5953
set label "4.0.0" at 11533, 5963
set arrow from 12085, 6000 to 12085, 5990
set label "4.1.0" at 12085, 6000
set arrow from 13131, 6002 to 13131, 5992
set label "4.2.0" at 13131, 6002
set arrow from 14208, 6026 to 14208, 6016
set label "4.3.0" at 14208, 6026
set arrow from 14648, 6292 to 14648, 6282
set label "4.4.0" at 14648, 6292
set arrow from 15246, 6101 to 15246, 6091
set label "4.5.0" at 15246, 6101
plot "dhrystone-mcs51-scoretable" using 1:4 title "default", "dhrystone-mcs51-scoretable" using 1:2 title "size", "dhrystone-mcs51-scoretable" using 1:3 title "speed"

