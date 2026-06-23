set output "whetstone-r3ka-score.svg"
set terminal svg size 640,480
set style data lines
set key bottom right
set xlabel "revision"
set ylabel "Whetstone KIPS"
set arrow from 13131, 37.851 to 13131, 36.851
set label "4.2.0" at 13131, 37.851
set arrow from 14208, 41.783 to 14208, 40.783
set label "4.3.0" at 14208, 41.783
set arrow from 14648, 41.997 to 14648, 40.997
set label "4.4.0" at 14648, 41.897
set arrow from 15246, 41.558 to 15246, 40.558
set label "4.5.0" at 15246, 41.558
plot "whetstone-r3ka-scoretable" using 1:4 title "default", "whetstone-r3ka-scoretable" using 1:2 title "size", "whetstone-r3ka-scoretable" using 1:3 title "speed"

