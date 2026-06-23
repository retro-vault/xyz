        ;; time_mdays.s
        ;; Split from gmtime_r.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module time_mdays
        .optsdcc -mz80 sdcccall(1)

        .globl  __time_leap
        .globl  __time_mdays

        .area   _CODE
__time_mdays::
        .db     31,28,31,30,31,30,31,31,30,31,30,31

        ; __time_leap: HL = full year -> A = 1 if leap, else 0 (Z if not leap)
__time_leap::
        ld      a,l
        and     #3
        jr      nz,leap_no
        ld      a,#1
        ret
leap_no:
        xor     a
        ret

        ; __gm_diy: HL = full year, BC = days in year (366 if leap else 365)
