set output "whetstone-stm8-size.svg"
set terminal svg size 640,480
set style data lines
set xlabel "revision"
set ylabel "size [B]"
set arrow from 9256, 12058 to 9256, 11858
set label "3.5.0" at 9256, 12058
set arrow from 9618, 12076 to 9618, 11876
set label "3.6.0" at 9618, 12076
set arrow from 10233, 11628 to 10233, 11428
set label "3.7.0" at 10233, 11628
set arrow from 10582, 11399 to 10582, 11199
set label "3.8.0" at 10582, 11399
set arrow from 11214, 10823 to 11214, 10623
set label "3.9.0" at 11214, 10823
set arrow from 11533, 10487 to 11533, 10287
set label "4.0.0" at 11533, 10487
set arrow from 12085, 10218 to 12085, 10018
set label "4.1.0" at 12085, 10218
set arrow from 13131, 9742 to 13131, 9542
set label "4.2.0" at 13131, 9742
set arrow from 14208, 9811 to 14208, 9611
set label "4.3.0" at 14208, 9811
set arrow from 14648, 9246 to 14648, 9046
set label "4.4.0" at 14648, 9246
set arrow from 15246, 9401 to 15246, 9201
set label "4.5.0" at 15246, 9401
plot "whetstone-stm8-sizetable" using 1:4 title "default", "whetstone-stm8-sizetable" using 1:2 title "size", "whetstone-stm8-sizetable" using 1:3 title "speed"

