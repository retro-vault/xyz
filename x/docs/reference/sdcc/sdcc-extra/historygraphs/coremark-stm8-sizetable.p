set output "coremark-stm8-size.svg"
set terminal svg size 640,480
set style data lines
set xlabel "revision"
set ylabel "size [B]"
set arrow from 9256, 16246 to 9256, 16046
set label "3.5.0" at 9256, 16246
set arrow from 9618, 16316 to 9618, 16116
set label "3.6.0" at 9618, 16316
set arrow from 10233, 15145 to 10233, 14945
set label "3.7.0" at 10233, 15145
set arrow from 10582, 14698 to 10582, 14498
set label "3.8.0" at 10582, 14698
set arrow from 11214, 14731 to 11214, 14531
set label "3.9.0" at 11214, 14731
set arrow from 11533, 14042 to 11533, 13842
set label "4.0.0" at 11533, 14042
set arrow from 12085, 13952 to 12085, 13752
set label "4.1.0" at 12085, 13952
set arrow from 13131, 13189 to 13131, 12989
set label "4.2.0" at 13131, 13189
set arrow from 14208, 12677 to 14208, 12477
set label "4.3.0" at 14208, 12677
set arrow from 14648, 13059 to 14648, 12859
set label "4.4.0" at 14648, 13059
set arrow from 15246, 12701 to 15246, 12501
set label "4.5.0" at 15246, 12701
plot "coremark-stm8-sizetable" using 1:4 title "default", "coremark-stm8-sizetable" using 1:2 title "size", "coremark-stm8-sizetable" using 1:3 title "speed"

