set output "stdcbench-r3ka-score.svg"
set terminal svg size 640,480
set style data lines
set key bottom right
set xlabel "revision"
set ylabel "stdcbench score"
set arrow from 12085, 125 to 12085, 124
set label "4.1.0" at 12085, 125
set arrow from 13131, 127 to 13131, 126
set label "4.2.0" at 13131, 127
set arrow from 14208, 86 to 14208, 85
set label "4.3.0" at 14208, 86
set arrow from 14648, 67 to 14648, 66
set label "4.4.0" at 14648, 67
set arrow from 15246, 66 to 15246, 65
set label "4.5.0" at 15246, 66
plot "stdcbench-r3ka-scoretable" using 1:4 title "default", "stdcbench-r3ka-scoretable" using 1:2 title "size", "stdcbench-r3ka-scoretable" using 1:3 title "speed"

