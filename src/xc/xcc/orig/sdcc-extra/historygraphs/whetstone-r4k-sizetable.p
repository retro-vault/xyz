set output "whetstone-r4k-size.svg"
set terminal svg size 640,480
set style data lines
set xlabel "revision"
set ylabel "size [B]"
set arrow from 15246, 10815 to 15246, 10715
set label "4.5.0" at 15246, 10815
plot "whetstone-r4k-sizetable" using 1:4 title "default", "whetstone-r4k-sizetable" using 1:2 title "size", "whetstone-r4k-sizetable" using 1:3 title "speed"

