set output "dhrystone-r3ka-size.svg"
set terminal svg size 640,480
set style data lines
set xlabel "revision"
set ylabel "size [B]"
set arrow from 12085, 10020 to 12085, 9920
set label "4.1.0" at 12085, 10020
set arrow from 13131, 8968 to 13131, 8868
set label "4.2.0" at 13131, 8968
set arrow from 14208, 8936 to 14208, 8836
set label "4.3.0" at 14208, 8936
set arrow from 14648, 8493 to 14648, 8393
set label "4.4.0" at 14648, 8493
set arrow from 15246, 8296 to 15246, 8196
set label "4.5.0" at 15246, 8296
plot "dhrystone-r3ka-sizetable" using 1:4 title "default", "dhrystone-r3ka-sizetable" using 1:2 title "size", "dhrystone-r3ka-sizetable" using 1:3 title "speed"

