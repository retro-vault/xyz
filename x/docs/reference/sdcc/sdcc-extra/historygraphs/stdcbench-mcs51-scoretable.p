set output "stdcbench-mcs51-score.svg"
set terminal svg size 640,480
set style data lines
set key bottom right
set xlabel "revision"
set ylabel "stdcbench score"
set arrow from 9618, 151 to 9618, 150
set label "3.6.0" at 9618, 151
set arrow from 10233, 144 to 10233, 143
set label "3.7.0" at 10233, 144
set arrow from 10582, 145 to 10582, 144
set label "3.8.0" at 10582, 145
set arrow from 11214, 157 to 11214, 156
set label "3.9.0" at 11214, 157
set arrow from 11533, 157 to 11533, 156
set label "4.0.0" at 11533, 157
set arrow from 12085, 157 to 12085, 156
set label "4.1.0" at 12085, 157
set arrow from 13131, 157 to 13131, 156
set label "4.2.0" at 13131, 157
set arrow from 14208, 157 to 14208, 156
set label "4.3.0" at 14208, 157
set arrow from 14648, 167 to 14648, 166
set label "4.4.0" at 14648, 167
set arrow from 15246, 153 to 15246, 152
set label "4.5.0" at 15246, 153
plot "stdcbench-mcs51-scoretable" using 1:4 title "default", "stdcbench-mcs51-scoretable" using 1:2 title "size", "stdcbench-mcs51-scoretable" using 1:3 title "speed"

