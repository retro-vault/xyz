all: whetstone

whetstone: crt0.rel whetstone.rel portme.rel
	$(SDCC) $(SDCCFLAGS) --no-std-crt0 $^ -o image.ihx

crt0.rel: crt0.s
	$(SDAS) -plosgff $<

%.rel: %.c
	$(SDCC) --fverbose-asm -c -DNOSTRUCTASSIGN $(SDCCFLAGS) $<

clean:
	rm -rf *.rel *.map *.lst *.noi *.lk *.lnk *.sym *.asm *.ihx *.o

