set output "dhrystone-stm8-score.svg"
set terminal svg size 640,480
set style data lines
set key bottom right
set xlabel "revision"
set ylabel "Dhrystones per second"
set arrow from 9256, 4825 to 9256, 4225
set label "3.5.0" at 9256, 4825
set arrow from 9618, 5138 to 9618, 4538
set label "3.6.0" at 9618, 5138
set arrow from 10233, 10833 to 10233, 10233
set label "3.7.0" at 10233, 10833
set arrow from 10582, 11617 to 10582, 10817
set label "3.8.0" at 10582, 11617
set arrow from 11214, 11641 to 11214, 10841
set label "3.9.0" at 11214, 11641
set arrow from 11533, 11641 to 11533, 10841
set label "4.0.0" at 11533, 11641
set arrow from 12085, 11636 to 12085, 10836
set label "4.1.0" at 12085, 11636
set arrow from 13131, 12013 to 13131, 11013
set label "4.2.0" at 13131, 12013
set arrow from 14208, 12116 to 14208, 11116
set label "4.3.0" at 14208, 12116
set arrow from 14648, 12574 to 14648, 11574
set label "4.4.0" at 14648, 12574
set arrow from 15246, 12536 to 15246, 11536
set label "4.5.0" at 15246, 12536
plot "dhrystone-stm8-scoretable" using 1:4 title "default", "dhrystone-stm8-scoretable" using 1:2 title "size", "dhrystone-stm8-scoretable" using 1:3 title "speed"

