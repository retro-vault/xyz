set output "coremark-r3ka-size.svg"
set terminal svg size 640,480
set style data lines
set xlabel "revision"
set ylabel "size [B]"
set arrow from 12085, 16100 to 12085, 16000
set label "4.1.0" at 12085, 16100
set arrow from 13131, 15029 to 13131, 14929
set label "4.2.0" at 13131, 15029
set arrow from 14208, 14217 to 14208, 14117
set label "4.3.0" at 14208, 14217
set arrow from 14648, 14987 to 14648, 14887
set label "4.4.0" at 14648, 14987
set arrow from 15246, 14312 to 15246, 14212
set label "4.5.0" at 15246, 14312
plot "coremark-r3ka-sizetable" using 1:4 title "default", "coremark-r3ka-sizetable" using 1:2 title "size", "coremark-r3ka-sizetable" using 1:3 title "speed"

