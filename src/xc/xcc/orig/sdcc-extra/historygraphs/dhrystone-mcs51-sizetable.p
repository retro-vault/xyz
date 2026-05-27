set output "dhrystone-mcs51-size.svg"
set terminal svg size 640,480
set style data lines
set xlabel "revision"
set ylabel "size [B]"
set arrow from 9618, 12845 to 9618, 12795
set label "3.6.0" at 9618, 12845
set arrow from 10233, 13154 to 10233, 13104
set label "3.7.0" at 10233, 13154
set arrow from 10582, 12936 to 10582, 12886
set label "3.8.0" at 10582, 12936
set arrow from 11214, 13673 to 11214, 13573
set label "3.9.0" at 11214, 13673
set arrow from 11533, 13680 to 11533, 13580
set label "4.0.0" at 11533, 13680
set arrow from 12085, 13711 to 12085, 13611
set label "4.1.0" at 12085, 13711
set arrow from 13131, 13729 to 13131, 13629
set label "4.2.0" at 13131, 13729
set arrow from 14208, 13701 to 14208, 13601
set label "4.3.0" at 14208, 13701
set arrow from 14648, 13591 to 14648, 13491
set label "4.4.0" at 14648, 13591
set arrow from 15246, 13047 to 15246, 12947
set label "4.5.0" at 15246, 13047
plot "dhrystone-mcs51-sizetable" using 1:4 title "default", "dhrystone-mcs51-sizetable" using 1:2 title "size", "dhrystone-mcs51-sizetable" using 1:3 title "speed"

