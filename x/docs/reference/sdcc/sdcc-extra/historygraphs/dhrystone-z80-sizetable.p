set output "dhrystone-z80-size.svg"
set terminal svg size 640,480
set style data lines
set xlabel "revision"
set ylabel "size [B]"
set arrow from 9256, 11002 to 9256, 10902
set label "3.5.0" at 9256, 11002
set arrow from 9618, 10819 to 9618, 10719
set label "3.6.0" at 9618, 10819
set arrow from 10582, 11111 to 10582, 11011
set label "3.8.0" at 10582, 11111
set arrow from 11214, 11356 to 11214, 11256
set label "3.9.0" at 11214, 11356
set arrow from 11533, 11470 to 11533, 11370
set label "4.0.0" at 11533, 11470
set arrow from 12085, 10894 to 12085, 10794
set label "4.1.0" at 12085, 10894
set arrow from 13131, 9632 to 13131, 9532
set label "4.2.0" at 13131, 9632
set arrow from 14208, 9606 to 14208, 9506
set label "4.3.0" at 14208, 9606
set arrow from 14648, 9431 to 14648, 9331
set label "4.4.0" at 14648, 9431
set arrow from 15246, 9225 to 15246, 9125
set label "4.5.0" at 15246, 9225
plot "dhrystone-z80-sizetable" using 1:4 title "default", "dhrystone-z80-sizetable" using 1:2 title "size", "dhrystone-z80-sizetable" using 1:3 title "speed"

