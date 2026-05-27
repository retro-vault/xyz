set output "whetstone-stm8-score.svg"
set terminal svg size 640,480
set style data lines
set key bottom right
set xlabel "revision"
set ylabel "Whetstone KIPS"
set arrow from 9256, 63.368 to 9256, 60.368
set label "3.5.0" at 9256, 62.368
set arrow from 9618, 63.085 to 9618, 60.085
set label "3.6.0" at 9618, 62.085
set arrow from 10233, 66.881 to 10233, 63.881
set label "3.7.0" at 10233, 66.881
set arrow from 10582, 67.834 to 10582, 64.834
set label "3.8.0" at 10582, 67.834
set arrow from 11214, 116.109 to 11214, 113.109
set label "3.9.0" at 11214, 116.109
set arrow from 11533, 126.793 to 11533, 123.793
set label "4.0.0" at 11533, 126.793
set arrow from 12085, 133.584 to 12085, 128.584
set label "4.1.0" at 12085, 133.584
set arrow from 13131, 142.268 to 13131, 137.268
set label "4.2.0" at 13131, 142.268
set arrow from 14208, 144.840 to 14208, 139.840
set label "4.3.0" at 14208, 144.840
set arrow from 14648, 144.840 to 14648, 139.840
set label "4.4.0" at 14648, 144.840
set arrow from 15246, 144.918 to 15246, 139.918
set label "4.5.0" at 15246, 144.918
plot "whetstone-stm8-scoretable" using 1:4 title "default", "whetstone-stm8-scoretable" using 1:2 title "size", "whetstone-stm8-scoretable" using 1:3 title "speed"

