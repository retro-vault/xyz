set output "dhrystone-r4k-size.svg"
set terminal svg size 640,480
set style data lines
set xlabel "revision"
set ylabel "size [B]"
set arrow from 15246, 8296 to 15246, 8196
set label "4.5.0" at 15246, 8296
plot "dhrystone-r4k-sizetable" using 1:4 title "default", "dhrystone-r4k-sizetable" using 1:2 title "size", "dhrystone-r4k-sizetable" using 1:3 title "speed"

