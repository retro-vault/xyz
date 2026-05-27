set output "whetstone-r3ka-size.svg"
set terminal svg size 640,480
set style data lines
set xlabel "revision"
set ylabel "size [B]"
set arrow from 12085, 13756 to 12085, 13656
set label "4.1.0" at 12085, 13756
set arrow from 13131, 12541 to 13131, 12441
set label "4.2.0" at 13131, 12541
set arrow from 14208, 12556 to 14208, 12456
set label "4.3.0" at 14208, 12556
set arrow from 14648, 10987 to 14648, 10887
set label "4.4.0" at 14648, 10987
set arrow from 15246, 10815 to 15246, 10715
set label "4.5.0" at 15246, 10815
plot "whetstone-r3ka-sizetable" using 1:4 title "default", "whetstone-r3ka-sizetable" using 1:2 title "size", "whetstone-r3ka-sizetable" using 1:3 title "speed"

