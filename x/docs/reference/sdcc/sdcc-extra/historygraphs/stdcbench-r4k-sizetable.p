set output "stdcbench-r4k-size.svg"
set terminal svg size 640,480
set style data lines
set xlabel "revision"
set ylabel "size [B]"
set arrow from 15246, 24536 to 15246, 24436
set label "4.5.0" at 15246, 24536
plot "stdcbench-r4k-sizetable" using 1:4 title "default", "stdcbench-r4k-sizetable" using 1:2 title "size", "stdcbench-r4k-sizetable" using 1:3 title "speed"

