set output "stdcbench-stm8-score.svg"
set terminal svg size 640,480
set style data lines
set key bottom right
set xlabel "revision"
set ylabel "stdcbench score"
trans(x) = x < 135 ? x : x - 60
set yrange [100:200]
set ytics ("100" 100, "110" 110, "120" 120, "190" 130, "200" 140, "210" 150, "220" 160, "230" 170, "240" 180, "250" 190, "260" 200)
set arrow from graph 0, first 125 to graph 1, first 125 nohead lt 500 lw 20 lc bgnd
set label "c90lib module enabled" at 10221, 125 front
set arrow from 9256, 111 to 9256, 106
set label "3.5.0" at 9256, 111
set arrow from 9618, 113 to 9618, 108
set label "3.6.0" at 9618, 113
set arrow from 10233, trans(209) to 10233, trans(204)
set label "3.7.0" at 10233, trans(209)
set arrow from 10582, trans(212) to 10582, trans(207)
set label "3.8.0" at 10582, trans(212)
set arrow from 11214, trans(222) to 11214, trans(217)
set label "3.9.0" at 11214, trans(222)
set arrow from 11533, trans(224) to 11533, trans(219)
set label "4.0.0" at 11533, trans(224)
set arrow from 12085, trans(233) to 12085, trans(228)
set label "4.1.0" at 12085, trans(233)
set arrow from 13131, trans(248) to 13131, trans(243)
set label "4.2.0" at 13131, trans(248)
set arrow from 14208, trans(200) to 14208, trans(195)
set label "4.3.0" at 14208, trans(200)
set arrow from 14648, trans(248) to 14648, trans(244)
set label "4.4.0" at 14648, trans(248)
plot "stdcbench-stm8-scoretable" using 1:(trans($4)) title "default", "stdcbench-stm8-scoretable" using 1:(trans($2)) title "size", "stdcbench-stm8-scoretable" using 1:(trans($3)) title "speed", 125 lt rgb "white" lw 20 notitle

