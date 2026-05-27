all: dhrystone

dhrystone: crt0.rel dhry_1.rel dhry_2.rel portme.rel
	$(SDCC) $(SDCCFLAGS) --no-std-crt0 $^ -o image.ihx

crt0.rel: crt0.s
	$(SDAS) -plosgff $<

%.rel: %.c dhry.h
	$(SDCC) --fverbose-asm -c -DNOSTRUCTASSIGN $(SDCCFLAGS) $<

clean:
	rm -rf *.rel *.map *.lst *.noi *.lk *.lnk *.sym *.asm *.ihx *.o *.rst

