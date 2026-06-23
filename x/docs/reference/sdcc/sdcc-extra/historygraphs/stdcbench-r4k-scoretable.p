set output "stdcbench-r4k-score.svg"
set terminal svg size 640,480
set style data lines
set key bottom right
set xlabel "revision"
set ylabel "stdcbench score"
plot "stdcbench-r4k-scoretable" using 1:4 title "default", "stdcbench-r4k-scoretable" using 1:2 title "size", "stdcbench-r4k-scoretable" using 1:3 title "speed"

