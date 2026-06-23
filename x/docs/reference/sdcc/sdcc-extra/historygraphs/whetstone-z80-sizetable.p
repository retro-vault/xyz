set output "whetstone-z80-size.svg"
set terminal svg size 640,480
set style data lines
set xlabel "revision"
set ylabel "size [B]"
set arrow from 9256, 17844 to 9256, 17744
set label "3.5.0" at 9256, 17844
set arrow from 9618, 17621 to 9618, 17521
set label "3.6.0" at 9618, 17621
set arrow from 10582, 17310 to 10582, 17210
set label "3.8.0" at 10582, 17310
set arrow from 11214, 17572 to 11214, 17472
set label "3.9.0" at 11214, 17572
set arrow from 11533, 17244 to 11533, 17144
set label "4.0.0" at 11533, 17244
set arrow from 12085, 16605 to 12085, 16505
set label "4.1.0" at 12085, 16605
set arrow from 13131, 14641 to 13131, 14541
set label "4.2.0" at 13131, 14641
set arrow from 14208, 14732 to 14208, 14632
set label "4.3.0" at 14208, 14732
set arrow from 14648, 13702 to 14648, 13602
set label "4.4.0" at 14648, 13702
set arrow from 15246, 13444 to 15246, 13344
set label "4.5.0" at 15246, 13444
plot "whetstone-z80-sizetable" using 1:4 title "default", "whetstone-z80-sizetable" using 1:2 title "size", "whetstone-z80-sizetable" using 1:3 title "speed"

