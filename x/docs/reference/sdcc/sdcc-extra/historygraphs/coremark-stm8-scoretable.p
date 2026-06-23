set output "coremark-stm8-score.svg"
set terminal svg size 640,480
set style data lines
set key bottom right
set xlabel "revision"
set ylabel "Coremark iterations / s"
set arrow from 9256, 3.257 to 9256, 2.957
set label "3.5.0" at 9256, 3.257
set arrow from 9618, 3.384 to 9618, 2.984
set label "3.6.0" at 9618, 3.384
set arrow from 10233, 6.324 to 10233, 6.024
set label "3.7.0" at 10233, 6.324
set arrow from 10582, 6.373 to 10582, 6.073
set label "3.8.0" at 10582, 6.373
set arrow from 11214, 6.231 to 11214, 5.931
set label "3.9.0" at 11214, 6.231
set arrow from 11533, 6.451 to 11533, 6.151
set label "4.0.0" at 11533, 6.451
set arrow from 12085, 6.567 to 12085, 6.167
set label "4.1.0" at 12085, 6.567
set arrow from 13131, 6.805 to 13131, 6.505
set label "4.2.0" at 13131, 6.805
set arrow from 14208, 6.711 to 14208, 6.411
set label "4.3.0" at 14208, 6.711
set arrow from 14648, 6.938 to 14648, 6.638
set label "4.4.0" at 14648, 6.938
set arrow from 15246, 6.859 to 15246, 6.559
set label "4.5.0" at 15246, 6.859
plot "coremark-stm8-scoretable" using 1:4 title "default", "coremark-stm8-scoretable" using 1:2 title "size", "coremark-stm8-scoretable" using 1:3 title "speed"

