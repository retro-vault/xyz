set output "stdcbench-mcs51-size.svg"
set terminal svg size 640,480
set style data lines
set xlabel "revision"
set ylabel "size [B]"
set arrow from 9618, 13050 to 9618, 12950
set label "3.6.0" at 9618, 13050
set arrow from 10233, 13805 to 10233, 13705
set label "3.7.0" at 10233, 13805
set arrow from 10582, 13480 to 10582, 13380
set label "3.8.0" at 10582, 13480
set arrow from 11214, 14064 to 11214, 13964
set label "3.9.0" at 10582, 14064
set arrow from 11533, 14080 to 11533, 13980
set label "4.0.0" at 11533, 14080
set arrow from 12085, 14089 to 12085, 13989
set label "4.1.0" at 12085, 14089
set arrow from 13131, 14101 to 13131, 14001
set label "4.2.0" at 13131, 14101
set arrow from 14208, 14150 to 14208, 14050
set label "4.3.0" at 14208, 14150
set arrow from 14648, 14119 to 14648, 14019
set label "4.4.0" at 14648, 14119
set arrow from 15246, 13544 to 15246, 13444
set label "4.5.0" at 15246, 13544
plot "stdcbench-mcs51-sizetable" using 1:4 title "default", "stdcbench-mcs51-sizetable" using 1:2 title "size", "stdcbench-mcs51-sizetable" using 1:3 title "speed"

