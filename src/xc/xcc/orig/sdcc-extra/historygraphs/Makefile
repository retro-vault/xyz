# Release revisions:
# 3.4.0: 8985 (affected by -Werror bug #2280)
# 3.5.0: 9256
# 3.6.0: 9618
# 3.7.0: 10233
# 3.8.0: 10582
# 3.9.0: 11214
# 4.0.0: 11533
# 4.1.0: 12085
# 4.2.0: 13131
# 4.4.0: 14648
# 4.5.0: 15246
# Broken: 8984-9000, 9104 (bug #2280), 9106-9114 (whetstone build segfaults), 9379-9418 (SDCC build fails), 9424 (library build fails) 9477 (SDCC build fails), 9891-9946 (stdcbench build fails), 9958-10147 (Dhrystone z80 build fails) 10233-10270 (stdcbench build segfaults), 10185-10340 (stdcbench build segfaults). 10345-10668 (sdcpp build fails). 10408 (stdcbench build segfaults). 10426-10462 (coremark build fails) 10682 (library build fails for mcs51) 11952 (library build fails). 13699 (sdcpp build fails). 14225-14238 (sdcc build fails), 14400 (build fails). 14463 (stm8 lib build fails)m 14464 (coremark build fails for z80). 16233-16331 (benchmarks issues on R4K).
# Build of large-stack-auto lib for mcs51 was enabled in 9521.

REVISIONS_R4K2 = 16340 16351 16354 16379 16400 16474 16532
REVISIONS_R4K1 = 15725 15768 15890 15891 15944 15945 15957 16016 16053 16153 16176 16220
REVISIONS_R4K = $(REVISIONS_R4K1) $(REVISIONS_R4K2)
REVISIONS_R3KA_WHETSTONE = 12891 12900 12969 13131 13200 13300 13495 13689 13709 13748 13772 13800 13804 13813 13825 13845 13885 13900 14000 14184 14208 14211 14246 14248 14253 14300 14346 14370 14398 14405 14443 14448 14453 14472 14510 14528 14648 14650 14710 14738 14817 14889 14955 14958 14996 15015 15022 15029 15032 15037 15060 15093 15104 15117 15118 15119 15172 15240 15246 15317 15418 15435 15451 15486 15550 15580 15669 15724 $(REVISIONS_R4K1) 16233 16305 16311 $(REVISIONS_R4K2)
REVISIONS_R3KA_STDCBENCH_POST = 12392 12415 12430 12450 12464 12488 12495 12508 12642 12673 12689 12692 12746 12784 12795 12827 12862 12875  $(REVISIONS_R3KA_WHETSTONE)
# Bug #3216 resulted in wrong r3ka stdcbench scores for 12100-12243. For 12121-12300, Coremark fails for z80. For 14221-14224. Whetstone fails for mcs51.
REVISIONS_R3KA_STDCBENCH = 12001 12056 12085 12098 $(REVISIONS_R3KA_STDCBENCH_POST) # Bug #3216 resulted in wrong r3ka stdcbench scores for 12100-12243.
REVISIONS_R3KA = 12001 12056 12085 12098 12100 12102 12106 12109 12114 $(REVISIONS_R3KA_STDCBENCH_POST) # Earlier than 11919 doesn't have a default crt0 that works for RCM boards. For 11919 and 11972, Dhrystone hangs. For 12121-12300, Coremark fails for z80. 
REVISIONS_MCS51 = 9521 9540 9550 9568 9581 9589 9596 9601 9602 9603 9604 9618 9624 9632 9635 9636 9637 9638 9640 9645 9646 9649 9651 9653 9654 9659 9662 9664 9665 9666 9679 9707 9730 9740 9747 9751 9761 9764 9765 9766 9767 9773 9784 9790 9791 9793 9797 9803 9807 9808 9815 9818 9819 9820 9830 9832 9836 9879 9949 9953 9955 9956 9961 10163 10165 10167 10174 10181 10182 10775 10782 10786 10787 10788 10790 10800 10900 10960 10974 11087 11100 11150 11214 11260 11330 11335 11371 11435 11438 11447 11507 11509 11533 11586 11614 11615 11625 11630 11631 11637 11638 11722 11822 $(REVISIONS_R3KA)
REVISIONS = 9105 9120 9124 9127 9138 9157 9171 9175 9182 9187 9191 9193 9207 9208 9218 9233 9256 9275 9291 9307 9311 9319 9354 9376 9438 9442 9445 9452 9461 9478 9487 9492 9493 9498 $(REVISIONS_MCS51)

BENCHMARKS = whetstone dhrystone coremark stdcbench

GOALS = size speed default

TARGETS = stm8 mcs51 r3ka r4k z80

# Adjust these according to attached cables!
STM8_TTY = /dev/ttyUSB1
MCS51_TTY = /dev/ttyUSB2
R3KA_TTY = /dev/ttyUSB3
R4K_TTY = /dev/ttyUSB3
Z80_TTY = /dev/ttyUSB0

COMPILERS = $(foreach revision, $(REVISIONS), sdcc-$(revision)/bin/sdcc)

BENCHMARKS_WHETSTONE = $(foreach target, $(TARGETS), $(foreach goal, $(GOALS), $(foreach revision, $(REVISIONS), whetstone-$(target)-$(goal)-$(revision)/image.ihx)))
$(BENCHMARKS_WHETSTONE): BENCHMARK = whetstone
BENCHMARKS_DHRYSTONE = $(foreach target, $(TARGETS), $(foreach goal, $(GOALS), $(foreach revision, $(REVISIONS), dhrystone-$(target)-$(goal)-$(revision)/image.ihx)))
$(BENCHMARKS_DHRYSTONE): BENCHMARK = dhrystone
BENCHMARKS_COREMARK = $(foreach target, $(TARGETS), $(foreach goal, $(GOALS), $(foreach revision, $(REVISIONS), coremark-$(target)-$(goal)-$(revision)/image.ihx)))
$(BENCHMARKS_COREMARK): BENCHMARK = coremark
BENCHMARKS_STDCBENCH = $(foreach target, $(TARGETS), $(foreach goal, $(GOALS), $(foreach revision, $(REVISIONS), stdcbench-$(target)-$(goal)-$(revision)/image.ihx)))
$(BENCHMARKS_STDCBENCH): BENCHMARK = stdcbench

BENCHMARKS_STM8 = $(foreach benchmark, $(BENCHMARKS), $(foreach goal, $(GOALS), $(foreach revision, $(REVISIONS), $(benchmark)-stm8-$(goal)-$(revision)/image.ihx)))
$(BENCHMARKS_STM8): TARGET = stm8
$(BENCHMARKS_STM8): SDCCFLAGS += -mstm8
BENCHMARKS_MCS51 = $(foreach benchmark, $(BENCHMARKS), $(foreach goal, $(GOALS), $(foreach revision, $(REVISIONS_MCS51), $(benchmark)-mcs51-$(goal)-$(revision)/image.ihx)))
$(BENCHMARKS_MCS51): TARGET = mcs51
$(BENCHMARKS_MCS51): SDCCFLAGS += -mmcs51 --model-large --stack-auto
BENCHMARKS_R3KA = $(foreach benchmark, $(BENCHMARKS), $(foreach goal, $(GOALS), $(foreach revision, $(REVISIONS_R3KA), $(benchmark)-r3ka-$(goal)-$(revision)/image.ihx)))
$(BENCHMARKS_R3KA): TARGET = r3ka
$(BENCHMARKS_R3KA): SDCCFLAGS += -mr3ka
BENCHMARKS_R3KA_WHETSTONE = $(foreach benchmark, $(BENCHMARKS), $(foreach goal, $(GOALS), $(foreach revision, $(REVISIONS_R3KA_WHETSTONE), $(benchmark)-r3ka-$(goal)-$(revision)/image.ihx)))
BENCHMARKS_R3KA_STDCBENCH = $(foreach benchmark, $(BENCHMARKS), $(foreach goal, $(GOALS), $(foreach revision, $(REVISIONS_R3KA_STDCBENCH), $(benchmark)-r3ka-$(goal)-$(revision)/image.ihx)))
BENCHMARKS_R4K = $(foreach benchmark, $(BENCHMARKS), $(foreach goal, $(GOALS), $(foreach revision, $(REVISIONS_R4K), $(benchmark)-r4k-$(goal)-$(revision)/image.ihx)))
$(BENCHMARKS_R4K): TARGET = r4k
$(BENCHMARKS_R4K): SDCCFLAGS += -mr4k
BENCHMARKS_Z80 = $(foreach benchmark, $(BENCHMARKS), $(foreach goal, $(GOALS), $(foreach revision, $(REVISIONS), $(benchmark)-z80-$(goal)-$(revision)/image.ihx)))
$(BENCHMARKS_Z80): TARGET = z80
$(BENCHMARKS_Z80): SDCCFLAGS += -mz80

BENCHMARKS_DEFAULT = $(foreach benchmark, $(BENCHMARKS), $(foreach target, $(TARGETS), $(foreach revision, $(REVISIONS), $(benchmark)-$(target)-default-$(revision)/image.ihx)))
$(BENCHMARKS_DEFAULT): GOAL = default
BENCHMARKS_SIZE = $(foreach benchmark, $(BENCHMARKS), $(foreach target, $(TARGETS), $(foreach revision, $(REVISIONS), $(benchmark)-$(target)-size-$(revision)/image.ihx)))
$(BENCHMARKS_SIZE): GOAL = size
$(BENCHMARKS_SIZE): SDCCFLAGS += --opt-code-size
BENCHMARKS_SPEED = $(foreach benchmark, $(BENCHMARKS), $(foreach target, $(TARGETS), $(foreach revision, $(REVISIONS), $(benchmark)-$(target)-speed-$(revision)/image.ihx)))
$(BENCHMARKS_SPEED): GOAL = speed
$(BENCHMARKS_SPEED): SDCCFLAGS += --opt-code-speed

# Default setting for --max-allocs-per-node (currently 3000)
BENCHMARKS_3000 = $(BENCHMARKS_WHETSTONE) $(BENCHMARKS_DHRYSTONE) $(BENCHMARKS_COREMARK) $(BENCHMARKS_STDCBENCH)

BENCHMARKS_WHETSTONE_SIZES = $(BENCHMARKS_WHETSTONE:image.ihx=size)
BENCHMARKS_DHRYSTONE_SIZES = $(BENCHMARKS_DHRYSTONE:image.ihx=size)
BENCHMARKS_COREMARK_SIZES = $(BENCHMARKS_COREMARK:image.ihx=size)
BENCHMARKS_STDCBENCH_SIZES = $(BENCHMARKS_STDCBENCH:image.ihx=size)
BENCHMARKS_SIZES = $(BENCHMARKS_3000:image.ihx=size)
BENCHMARKS_WHETSTONE_SCORES = $(BENCHMARKS_WHETSTONE:image.ihx=score)
BENCHMARKS_DHRYSTONE_SCORES = $(BENCHMARKS_DHRYSTONE:image.ihx=score)
BENCHMARKS_COREMARK_SCORES = $(BENCHMARKS_COREMARK:image.ihx=score)
BENCHMARKS_STDCBENCH_SCORES = $(BENCHMARKS_STDCBENCH:image.ihx=score)
BENCHMARKS_SCORES = $(BENCHMARKS_3000:image.ihx=score)

# No Coremark for the MCS-51 - Coremark is just too big.
BENCHMARKS_WHETSTONE_STM8_SIZES = $(patsubst %image.ihx, %size, $(filter $(BENCHMARKS_WHETSTONE), $(BENCHMARKS_STM8)))
BENCHMARKS_WHETSTONE_MCS51_SIZES = $(patsubst %image.ihx, %size, $(filter $(BENCHMARKS_WHETSTONE), $(BENCHMARKS_MCS51)))
BENCHMARKS_WHETSTONE_R3KA_SIZES = $(patsubst %image.ihx, %size, $(filter $(BENCHMARKS_WHETSTONE), $(BENCHMARKS_R3KA)))
BENCHMARKS_WHETSTONE_R4K_SIZES = $(patsubst %image.ihx, %size, $(filter $(BENCHMARKS_WHETSTONE), $(BENCHMARKS_R4K)))
BENCHMARKS_WHETSTONE_Z80_SIZES = $(patsubst %image.ihx, %size, $(filter $(BENCHMARKS_WHETSTONE), $(BENCHMARKS_Z80)))
BENCHMARKS_DHRYSTONE_STM8_SIZES = $(patsubst %image.ihx, %size, $(filter $(BENCHMARKS_DHRYSTONE), $(BENCHMARKS_STM8)))
BENCHMARKS_DHRYSTONE_MCS51_SIZES = $(patsubst %image.ihx, %size, $(filter $(BENCHMARKS_DHRYSTONE), $(BENCHMARKS_MCS51)))
BENCHMARKS_DHRYSTONE_R3KA_SIZES = $(patsubst %image.ihx, %size, $(filter $(BENCHMARKS_DHRYSTONE), $(BENCHMARKS_R3KA)))
BENCHMARKS_DHRYSTONE_R4K_SIZES = $(patsubst %image.ihx, %size, $(filter $(BENCHMARKS_DHRYSTONE), $(BENCHMARKS_R4K)))
BENCHMARKS_DHRYSTONE_Z80_SIZES = $(patsubst %image.ihx, %size, $(filter $(BENCHMARKS_DHRYSTONE), $(BENCHMARKS_Z80)))
BENCHMARKS_COREMARK_STM8_SIZES = $(patsubst %image.ihx, %size, $(filter $(BENCHMARKS_COREMARK), $(BENCHMARKS_STM8)))
BENCHMARKS_COREMARK_R3KA_SIZES = $(patsubst %image.ihx, %size, $(filter $(BENCHMARKS_COREMARK), $(BENCHMARKS_R3KA)))
BENCHMARKS_COREMARK_R4K_SIZES = $(patsubst %image.ihx, %size, $(filter $(BENCHMARKS_COREMARK), $(BENCHMARKS_R4K)))
BENCHMARKS_COREMARK_Z80_SIZES = $(patsubst %image.ihx, %size, $(filter $(BENCHMARKS_COREMARK), $(BENCHMARKS_Z80)))
BENCHMARKS_STDCBENCH_STM8_SIZES = $(patsubst %image.ihx, %size, $(filter $(BENCHMARKS_STDCBENCH), $(BENCHMARKS_STM8)))
BENCHMARKS_STDCBENCH_MCS51_SIZES = $(patsubst %image.ihx, %size, $(filter $(BENCHMARKS_STDCBENCH), $(BENCHMARKS_MCS51)))
BENCHMARKS_STDCBENCH_R3KA_SIZES = $(patsubst %image.ihx, %size, $(filter $(BENCHMARKS_STDCBENCH), $(BENCHMARKS_R3KA)))
BENCHMARKS_STDCBENCH_R4K_SIZES = $(patsubst %image.ihx, %size, $(filter $(BENCHMARKS_STDCBENCH), $(BENCHMARKS_R4K)))
BENCHMARKS_STDCBENCH_Z80_SIZES = $(patsubst %image.ihx, %size, $(filter $(BENCHMARKS_STDCBENCH), $(BENCHMARKS_Z80)))
BENCHMARKS_WHETSTONE_STM8_SCORES = $(patsubst %image.ihx, %score, $(filter $(BENCHMARKS_WHETSTONE), $(BENCHMARKS_STM8)))
BENCHMARKS_WHETSTONE_MCS51_SCORES = $(patsubst %image.ihx, %score, $(filter $(BENCHMARKS_WHETSTONE), $(BENCHMARKS_MCS51)))
BENCHMARKS_WHETSTONE_R3KA_SCORES = $(patsubst %image.ihx, %score, $(filter $(BENCHMARKS_WHETSTONE), $(BENCHMARKS_R3KA_WHETSTONE)))
BENCHMARKS_WHETSTONE_R4K_SCORES = $(patsubst %image.ihx, %score, $(filter $(BENCHMARKS_WHETSTONE), $(BENCHMARKS_R4K)))
BENCHMARKS_DHRYSTONE_STM8_SCORES = $(patsubst %image.ihx, %score, $(filter $(BENCHMARKS_DHRYSTONE), $(BENCHMARKS_STM8)))
BENCHMARKS_DHRYSTONE_MCS51_SCORES = $(patsubst %image.ihx, %score, $(filter $(BENCHMARKS_DHRYSTONE), $(BENCHMARKS_MCS51)))
BENCHMARKS_DHRYSTONE_R3KA_SCORES = $(patsubst %image.ihx, %score, $(filter $(BENCHMARKS_DHRYSTONE), $(BENCHMARKS_R3KA)))
BENCHMARKS_DHRYSTONE_R4K_SCORES = $(patsubst %image.ihx, %score, $(filter $(BENCHMARKS_DHRYSTONE), $(BENCHMARKS_R4K)))
BENCHMARKS_COREMARK_STM8_SCORES = $(patsubst %image.ihx, %score, $(filter $(BENCHMARKS_COREMARK), $(BENCHMARKS_STM8)))
BENCHMARKS_COREMARK_R3KA_SCORES = $(patsubst %image.ihx, %score, $(filter $(BENCHMARKS_COREMARK), $(BENCHMARKS_R3KA)))
BENCHMARKS_COREMARK_R4K_SCORES = $(patsubst %image.ihx, %score, $(filter $(BENCHMARKS_COREMARK), $(BENCHMARKS_R4K)))
BENCHMARKS_STDCBENCH_STM8_SCORES = $(patsubst %image.ihx, %score, $(filter $(BENCHMARKS_STDCBENCH), $(BENCHMARKS_STM8)))
BENCHMARKS_STDCBENCH_MCS51_SCORES = $(patsubst %image.ihx, %score, $(filter $(BENCHMARKS_STDCBENCH), $(BENCHMARKS_MCS51)))
BENCHMARKS_STDCBENCH_R3KA_SCORES = $(patsubst %image.ihx, %score, $(filter $(BENCHMARKS_STDCBENCH), $(BENCHMARKS_R3KA_STDCBENCH)))
BENCHMARKS_STDCBENCH_R4K_SCORES = $(patsubst %image.ihx, %score, $(filter $(BENCHMARKS_STDCBENCH), $(BENCHMARKS_R4K)))

.PHONY: all clean

# Do not include Coremark (historically non-free) or score graphs (require STM8AF / C8051 V2.1 C8051F120 / RCM3319 / RCM4110 boards) in all.
all: whetstone-stm8-size.svg whetstone-mcs51-size.svg whetstone-r3ka-size.svg whetstone-r4k-size.svg dhrystone-stm8-size.svg dhrystone-mcs51-size.svg dhrystone-r3ka-size.svg dhrystone-r4k-size.svg dhrystone-z80-size.svg stdcbench-stm8-size.svg stdcbench-mcs51-size.svg stdcbench-z80-size.svg

clean:
	echo $(BENCHMARKS_SIZES)

# coremark-1.01.tar.gz is free (identical to old non-free coremark_v1.0.tgz, except for the license and documentation), can be downloaded from https://www.eembc.org/coremark/
coremark-1.01: coremark-1.01.tar.gz
	rm -rf coremark-1.01
	tar -xzf coremark-1.01.tar.gz
	touch coremark-1.01

coremark-stm8: coremark-1.01.tar.gz coremark-1.0-stm8.patch
	rm -rf coremark-stm8
	cp -r coremark-1.01 coremark-stm8
	cd coremark-stm8 && cp barebones/core_portme.? ./ && cat ../coremark-1.0-stm8.patch | patch -p1
	cp Makefile.coremark coremark-stm8/Makefile

coremark-r3ka: coremark-1.01 coremark-1.0-r3ka.patch
	rm -rf coremark-r3ka
	cp -r coremark-1.01 coremark-r3ka
	cd coremark-r3ka && cp barebones/core_portme.? ./ && cat ../coremark-1.0-r3ka.patch | patch -p1
	cp Makefile.coremark coremark-r3ka/Makefile

coremark-r4k: coremark-1.01 coremark-1.0-r4k.patch
	rm -rf coremark-r4k
	cp -r coremark-1.01 coremark-r4k
	cd coremark-r4k && cp barebones/core_portme.? ./ && cat ../coremark-1.0-r4k.patch | patch -p1
	cp Makefile.coremark coremark-r4k/Makefile

coremark-z80: coremark-1.01 coremark-1.0-z80.patch
	rm -rf coremark-z80
	cp -r coremark-1.01 coremark-z80
	cd coremark-z80 && cp barebones/core_portme.? ./ && cat ../coremark-1.0-z80.patch | patch -p1
	cp Makefile.coremark-z80 coremark-z80/Makefile
	cp crt.0.s.coremark-SDCC-Z80 coremark-z80/crt0.s

# stdcbench-0.8.tar.gz is free, can be downloaded from http://stdcbench.sourceforge.net
stdcbench-0.8: stdcbench-0.8.tar.gz
	rm -rf stdcbench-0.8
	tar -xzf stdcbench-0.8.tar.gz
	touch stdcbench-0.8

stdcbench-stm8: stdcbench-0.8 stdcbench-0.8-stm8.patch
	rm -rf stdcbench-stm8
	cp -r stdcbench-0.8 stdcbench-stm8
	cd stdcbench-stm8 && cp examples/Makefile.SDCC-STM8AF Makefile && cat ../stdcbench-0.8-stm8.patch | patch -p1

stdcbench-mcs51: stdcbench-0.8 stdcbench-0.8-mcs51.patch
	rm -rf stdcbench-mcs51
	cp -r stdcbench-0.8 stdcbench-mcs51
	cd stdcbench-mcs51 && cp examples/Makefile.SDCC-C8051 Makefile && cat ../stdcbench-0.8-mcs51.patch | patch -p1

stdcbench-r3ka: stdcbench-0.8 stdcbench-0.8-r3ka.patch
	rm -rf stdcbench-r3ka
	cp -r stdcbench-0.8 stdcbench-r3ka
	cd stdcbench-r3ka && cp examples/Makefile.SDCC-R3KA Makefile && cat ../stdcbench-0.8-r3ka.patch | patch -p1

stdcbench-r4k: stdcbench-0.8 stdcbench-0.8-r4k.patch
	rm -rf stdcbench-r4k
	cp -r stdcbench-0.8 stdcbench-r4k
	cd stdcbench-r4k && cp examples/Makefile.SDCC-R4K Makefile && cat ../stdcbench-0.8-r4k.patch | patch -p1

stdcbench-z80: stdcbench-0.8 stdcbench-0.8-z80.patch
	rm -rf stdcbench-z80
	cp -r stdcbench-0.8 stdcbench-z80
	cd stdcbench-z80 && cp examples/Makefile.SDCC-Z80 Makefile && cat ../stdcbench-0.8-z80.patch | patch -p1

%-size.svg: %-sizetable %-sizetable.p
	gnuplot $*-sizetable.p
	
%-score.svg: %-scoretable %-scoretable.p
	gnuplot $*-scoretable.p

whetstone-stm8-sizetable: $(BENCHMARKS_WHETSTONE_STM8_SIZES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS); do \
		SIZE_SIZE=`cat whetstone-stm8-size-$$r/size`; \
		SIZE_SPEED=`cat whetstone-stm8-speed-$$r/size`; \
		SIZE_DEFAULT=`cat whetstone-stm8-default-$$r/size `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

whetstone-mcs51-sizetable: $(BENCHMARKS_WHETSTONE_MCS51_SIZES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS_MCS51); do \
		SIZE_SIZE=`cat whetstone-mcs51-size-$$r/size`; \
		SIZE_SPEED=`cat whetstone-mcs51-speed-$$r/size`; \
		SIZE_DEFAULT=`cat whetstone-mcs51-default-$$r/size `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

whetstone-r3ka-sizetable: $(BENCHMARKS_WHETSTONE_R3KA_SIZES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS_R3KA); do \
		SIZE_SIZE=`cat whetstone-r3ka-size-$$r/size`; \
		SIZE_SPEED=`cat whetstone-r3ka-speed-$$r/size`; \
		SIZE_DEFAULT=`cat whetstone-r3ka-default-$$r/size `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

whetstone-r4k-sizetable: $(BENCHMARKS_WHETSTONE_R4K_SIZES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS_R4K); do \
		SIZE_SIZE=`cat whetstone-r4k-size-$$r/size`; \
		SIZE_SPEED=`cat whetstone-r4k-speed-$$r/size`; \
		SIZE_DEFAULT=`cat whetstone-r4k-default-$$r/size `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

whetstone-z80-sizetable: $(BENCHMARKS_WHETSTONE_Z80_SIZES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS); do \
		SIZE_SIZE=`cat whetstone-z80-size-$$r/size`; \
		SIZE_SPEED=`cat whetstone-z80-speed-$$r/size`; \
		SIZE_DEFAULT=`cat whetstone-z80-default-$$r/size `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

dhrystone-stm8-sizetable: $(BENCHMARKS_DHRYSTONE_STM8_SIZES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS); do \
		SIZE_SIZE=`cat dhrystone-stm8-size-$$r/size`; \
		SIZE_SPEED=`cat dhrystone-stm8-speed-$$r/size`; \
		SIZE_DEFAULT=`cat dhrystone-stm8-default-$$r/size `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

dhrystone-mcs51-sizetable: $(BENCHMARKS_DHRYSTONE_MCS51_SIZES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS_MCS51); do \
		SIZE_SIZE=`cat dhrystone-mcs51-size-$$r/size`; \
		SIZE_SPEED=`cat dhrystone-mcs51-speed-$$r/size`; \
		SIZE_DEFAULT=`cat dhrystone-mcs51-default-$$r/size `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

dhrystone-r3ka-sizetable: $(BENCHMARKS_DHRYSTONE_R3KA_SIZES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS_R3KA); do \
		SIZE_SIZE=`cat dhrystone-r3ka-size-$$r/size`; \
		SIZE_SPEED=`cat dhrystone-r3ka-speed-$$r/size`; \
		SIZE_DEFAULT=`cat dhrystone-r3ka-default-$$r/size `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

dhrystone-r4k-sizetable: $(BENCHMARKS_DHRYSTONE_R4K_SIZES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS_R4K); do \
		SIZE_SIZE=`cat dhrystone-r4k-size-$$r/size`; \
		SIZE_SPEED=`cat dhrystone-r4k-speed-$$r/size`; \
		SIZE_DEFAULT=`cat dhrystone-r4k-default-$$r/size `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

dhrystone-z80-sizetable: $(BENCHMARKS_DHRYSTONE_Z80_SIZES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS); do \
		SIZE_SIZE=`cat dhrystone-z80-size-$$r/size`; \
		SIZE_SPEED=`cat dhrystone-z80-speed-$$r/size`; \
		SIZE_DEFAULT=`cat dhrystone-z80-default-$$r/size `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

coremark-stm8-sizetable: $(BENCHMARKS_COREMARK_STM8_SIZES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS); do \
		SIZE_SIZE=`cat coremark-stm8-size-$$r/size`; \
		SIZE_SPEED=`cat coremark-stm8-speed-$$r/size`; \
		SIZE_DEFAULT=`cat coremark-stm8-default-$$r/size `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

coremark-r3ka-sizetable: $(BENCHMARKS_COREMARK_R3KA_SIZES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS_R3KA); do \
		SIZE_SIZE=`cat coremark-r3ka-size-$$r/size`; \
		SIZE_SPEED=`cat coremark-r3ka-speed-$$r/size`; \
		SIZE_DEFAULT=`cat coremark-r3ka-default-$$r/size `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

coremark-r4k-sizetable: $(BENCHMARKS_COREMARK_R4K_SIZES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS_R4K); do \
		SIZE_SIZE=`cat coremark-r4k-size-$$r/size`; \
		SIZE_SPEED=`cat coremark-r4k-speed-$$r/size`; \
		SIZE_DEFAULT=`cat coremark-r4k-default-$$r/size `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

coremark-z80-sizetable: $(BENCHMARKS_COREMARK_Z80_SIZES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS); do \
		SIZE_SIZE=`cat coremark-z80-size-$$r/size`; \
		SIZE_SPEED=`cat coremark-z80-speed-$$r/size`; \
		SIZE_DEFAULT=`cat coremark-z80-default-$$r/size `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

stdcbench-stm8-sizetable: $(BENCHMARKS_STDCBENCH_STM8_SIZES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS); do \
		SIZE_SIZE=`cat stdcbench-stm8-size-$$r/size`; \
		SIZE_SPEED=`cat stdcbench-stm8-speed-$$r/size`; \
		SIZE_DEFAULT=`cat stdcbench-stm8-default-$$r/size `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

stdcbench-mcs51-sizetable: $(BENCHMARKS_STDCBENCH_MCS51_SIZES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS_MCS51); do \
		SIZE_SIZE=`cat stdcbench-mcs51-size-$$r/size`; \
		SIZE_SPEED=`cat stdcbench-mcs51-speed-$$r/size`; \
		SIZE_DEFAULT=`cat stdcbench-mcs51-default-$$r/size `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

stdcbench-r3ka-sizetable: $(BENCHMARKS_STDCBENCH_R3KA_SIZES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS_R3KA_STDCBENCH); do \
		SIZE_SIZE=`cat stdcbench-r3ka-size-$$r/size`; \
		SIZE_SPEED=`cat stdcbench-r3ka-speed-$$r/size`; \
		SIZE_DEFAULT=`cat stdcbench-r3ka-default-$$r/size `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

stdcbench-r4k-sizetable: $(BENCHMARKS_STDCBENCH_R4K_SIZES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS_R4K); do \
		SIZE_SIZE=`cat stdcbench-r4k-size-$$r/size`; \
		SIZE_SPEED=`cat stdcbench-r4k-speed-$$r/size`; \
		SIZE_DEFAULT=`cat stdcbench-r4k-default-$$r/size `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

stdcbench-z80-sizetable: $(BENCHMARKS_STDCBENCH_Z80_SIZES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS); do \
		SIZE_SIZE=`cat stdcbench-z80-size-$$r/size`; \
		SIZE_SPEED=`cat stdcbench-z80-speed-$$r/size`; \
		SIZE_DEFAULT=`cat stdcbench-z80-default-$$r/size `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

whetstone-stm8-scoretable: $(BENCHMARKS_WHETSTONE_STM8_SCORES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS); do \
		SIZE_SIZE=`cat whetstone-stm8-size-$$r/score`; \
		SIZE_SPEED=`cat whetstone-stm8-speed-$$r/score`; \
		SIZE_DEFAULT=`cat whetstone-stm8-default-$$r/score `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

whetstone-mcs51-scoretable: $(BENCHMARKS_WHETSTONE_MCS51_SCORES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS_MCS51); do \
		SIZE_SIZE=`cat whetstone-mcs51-size-$$r/score`; \
		SIZE_SPEED=`cat whetstone-mcs51-speed-$$r/score`; \
		SIZE_DEFAULT=`cat whetstone-mcs51-default-$$r/score `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

whetstone-r3ka-scoretable: $(BENCHMARKS_WHETSTONE_R3KA_SCORES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS_R3KA_WHETSTONE); do \
		SIZE_SIZE=`cat whetstone-r3ka-size-$$r/score`; \
		SIZE_SPEED=`cat whetstone-r3ka-speed-$$r/score`; \
		SIZE_DEFAULT=`cat whetstone-r3ka-default-$$r/score `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

whetstone-r4k-scoretable: $(BENCHMARKS_WHETSTONE_R4K_SCORES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS_R4K); do \
		SIZE_SIZE=`cat whetstone-r4k-size-$$r/score`; \
		SIZE_SPEED=`cat whetstone-r4k-speed-$$r/score`; \
		SIZE_DEFAULT=`cat whetstone-r4k-default-$$r/score `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

dhrystone-stm8-scoretable: $(BENCHMARKS_DHRYSTONE_STM8_SCORES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS); do \
		SIZE_SIZE=`cat dhrystone-stm8-size-$$r/score`; \
		SIZE_SPEED=`cat dhrystone-stm8-speed-$$r/score`; \
		SIZE_DEFAULT=`cat dhrystone-stm8-default-$$r/score `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

dhrystone-mcs51-scoretable: $(BENCHMARKS_DHRYSTONE_MCS51_SCORES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS_MCS51); do \
		SIZE_SIZE=`cat dhrystone-mcs51-size-$$r/score`; \
		SIZE_SPEED=`cat dhrystone-mcs51-speed-$$r/score`; \
		SIZE_DEFAULT=`cat dhrystone-mcs51-default-$$r/score `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

dhrystone-r3ka-scoretable: $(BENCHMARKS_DHRYSTONE_R3KA_SCORES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS_R3KA); do \
		SIZE_SIZE=`cat dhrystone-r3ka-size-$$r/score`; \
		SIZE_SPEED=`cat dhrystone-r3ka-speed-$$r/score`; \
		SIZE_DEFAULT=`cat dhrystone-r3ka-default-$$r/score `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

dhrystone-r4k-scoretable: $(BENCHMARKS_DHRYSTONE_R4K_SCORES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS_R4K); do \
		SIZE_SIZE=`cat dhrystone-r4k-size-$$r/score`; \
		SIZE_SPEED=`cat dhrystone-r4k-speed-$$r/score`; \
		SIZE_DEFAULT=`cat dhrystone-r4k-default-$$r/score `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

coremark-stm8-scoretable: $(BENCHMARKS_COREMARK_STM8_SCORES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS); do \
		SIZE_SIZE=`cat coremark-stm8-size-$$r/score`; \
		SIZE_SPEED=`cat coremark-stm8-speed-$$r/score`; \
		SIZE_DEFAULT=`cat coremark-stm8-default-$$r/score `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

coremark-r3ka-scoretable: $(BENCHMARKS_COREMARK_R3KA_SCORES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS_R3KA); do \
		SIZE_SIZE=`cat coremark-r3ka-size-$$r/score`; \
		SIZE_SPEED=`cat coremark-r3ka-speed-$$r/score`; \
		SIZE_DEFAULT=`cat coremark-r3ka-default-$$r/score `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

coremark-r4k-scoretable: $(BENCHMARKS_COREMARK_R4K_SCORES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS_R4K); do \
		SIZE_SIZE=`cat coremark-r4k-size-$$r/score`; \
		SIZE_SPEED=`cat coremark-r4k-speed-$$r/score`; \
		SIZE_DEFAULT=`cat coremark-r4k-default-$$r/score `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

stdcbench-stm8-scoretable: $(BENCHMARKS_STDCBENCH_STM8_SCORES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS); do \
		SIZE_SIZE=`cat stdcbench-stm8-size-$$r/score`; \
		SIZE_SPEED=`cat stdcbench-stm8-speed-$$r/score`; \
		SIZE_DEFAULT=`cat stdcbench-stm8-default-$$r/score `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

stdcbench-mcs51-scoretable: $(BENCHMARKS_STDCBENCH_MCS51_SCORES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS_MCS51); do \
		SIZE_SIZE=`cat stdcbench-mcs51-size-$$r/score`; \
		SIZE_SPEED=`cat stdcbench-mcs51-speed-$$r/score`; \
		SIZE_DEFAULT=`cat stdcbench-mcs51-default-$$r/score `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

stdcbench-r3ka-scoretable: $(BENCHMARKS_STDCBENCH_R3KA_SCORES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS_R3KA_STDCBENCH); do \
		SIZE_SIZE=`cat stdcbench-r3ka-size-$$r/score`; \
		SIZE_SPEED=`cat stdcbench-r3ka-speed-$$r/score`; \
		SIZE_DEFAULT=`cat stdcbench-r3ka-default-$$r/score `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

stdcbench-r4k-scoretable: $(BENCHMARKS_STDCBENCH_R4K_SCORES)
	echo "#Revision size speed default" > $@
	for r in $(REVISIONS_R4K); do \
		SIZE_SIZE=`cat stdcbench-r4k-size-$$r/score`; \
		SIZE_SPEED=`cat stdcbench-r4k-speed-$$r/score`; \
		SIZE_DEFAULT=`cat stdcbench-r4k-default-$$r/score `; \
		echo $$r $$SIZE_SIZE $$SIZE_SPEED $$SIZE_DEFAULT >> $@; \
	done

# Download (with some retries to improve stability on bad connections) and compile an sdcc revision
$(COMPILERS): sdcc-%/bin/sdcc:
	rm -rf sdcc-$*
	svn co -r $* https://svn.code.sf.net/p/sdcc/code/trunk/sdcc sdcc-$* || echo "svn checkout incomplete. Second attempt in 8s." && sleep 8 && svn cleanup sdcc-$* && svn co -r $* https://svn.code.sf.net/p/sdcc/code/trunk/sdcc sdcc-$* || echo "svn checkout incomplete. Third attempt in 8s." && sleep 8 && svn cleanup sdcc-$* && svn co -r $* https://svn.code.sf.net/p/sdcc/code/trunk/sdcc sdcc-$* || echo "svn checkout incomplete. Last attempt in 8s." && sleep 8 && svn cleanup sdcc-$* && svn co -r $* https://svn.code.sf.net/p/sdcc/code/trunk/sdcc sdcc-$*
	cd sdcc-$* && ./configure --disable-pic14-port --disable-pic16-port CC="gcc --std=gnu17" && $(MAKE) && rm -rf device/non-free .svn && rm src/*.o src/*/*.o src/*/*.a

%: REVISION=$(lastword $(subst -, ,$*))

.SECONDEXPANSION:
$(BENCHMARKS_3000): %/image.ihx: $$(BENCHMARK)-$$(TARGET) sdcc-$$(REVISION)/bin/sdcc
	rm -rf $(BENCHMARK)-$(TARGET)-$(GOAL)-$(REVISION)
	cp --reflink=auto -r $(BENCHMARK)-$(TARGET) $(BENCHMARK)-$(TARGET)-$(GOAL)-$(REVISION)
	if [ "$(TARGET)" = "mcs51" ] ; \
	then \
		cd $(BENCHMARK)-$(TARGET)-$(GOAL)-$(REVISION) && $(MAKE) SDCC="../sdcc-$(REVISION)/bin/sdcc" SDCCFLAGS="$(SDCCFLAGS) --nostdinc -I../sdcc-$(REVISION)/device/include -I../sdcc-$(REVISION)/device/include/$(TARGET) -L ../sdcc-$(REVISION)/device/lib/build/large-stack-auto"; \
	else \
		cd $(BENCHMARK)-$(TARGET)-$(GOAL)-$(REVISION) && $(MAKE) SDAS="../sdcc-$(REVISION)/bin/sdas$(TARGET)" SDCC="../sdcc-$(REVISION)/bin/sdcc" SDCCFLAGS="$(SDCCFLAGS) --nostdinc -I../sdcc-$(REVISION)/device/include -L ../sdcc-$(REVISION)/device/lib/build/$(TARGET)"; \
	fi

$(BENCHMARKS_SIZES): %/size: %/image.ihx
	echo `size $< | tail -n 1 | sed -e "s/ *0[ 	]*\([0-9]*\).*/\1/"`> $@

# The benchmarks are executed on external boards, and thus cannot be run in parallel. Also this might require root, so don't target all depend on it.
.NOTPARALLEL: $(BENCHMARKS_SCORES)

$(BENCHMARKS_WHETSTONE_STM8_SCORES): %/score: %/image.ihx
	echo "scale=3;" 1000000 / `./execute_benchmark-stm8 $(STM8_TTY) $< | sed -e "s/Loops: 10, Iterations: 1, Duration: //" | sed -e "s/ msec.//"` | bc > $@

$(BENCHMARKS_WHETSTONE_MCS51_SCORES): %/score: %/image.ihx
	echo "scale=3;" 1000000 / `./execute_benchmark-mcs51 $(MCS51_TTY) $< | sed -e "s/Loops: 10, Iterations: 1, Duration: //" | sed -e "s/ msec.//"` | bc > $@

$(BENCHMARKS_WHETSTONE_R3KA_SCORES): %/score: %/image.ihx
	echo "scale=3;" 1000000 / `./execute_benchmark-r3ka $(R3KA_TTY) $< | sed -e "s/Loops: 10, Iterations: 1, Duration: //" | sed -e "s/ msec.*//"` | bc > $@

$(BENCHMARKS_WHETSTONE_R4K_SCORES): %/score: %/image.ihx
	echo "scale=3;" 1000000 / `./execute_benchmark-r4k $(R4K_TTY) $< | sed -e "s/Loops: 10, Iterations: 1, Duration: //" | sed -e "s/ msec.*//"` | bc > $@

$(BENCHMARKS_DHRYSTONE_STM8_SCORES): %/score: %/image.ihx
	./execute_benchmark-stm8 $(STM8_TTY) $< | sed -e "s/Dhrystones per Second: //" > $@

$(BENCHMARKS_DHRYSTONE_MCS51_SCORES): %/score: %/image.ihx
	./execute_benchmark-mcs51 $(MCS51_TTY) $< | sed -e "s/Dhrystones per Second: //" > $@

$(BENCHMARKS_DHRYSTONE_R3KA_SCORES): %/score: %/image.ihx
	./execute_benchmark-r3ka $(R3KA_TTY) $< | sed -e "s/Dhrystones per Second: //" > $@

$(BENCHMARKS_DHRYSTONE_R4K_SCORES): %/score: %/image.ihx
	./execute_benchmark-r4k $(R4K_TTY) $< | sed -e "s/Dhrystones per Second: //" > $@

$(BENCHMARKS_COREMARK_STM8_SCORES): %/score: %/image.ihx
	echo "scale=3;" 60000 / `./execute_benchmark-stm8 $(STM8_TTY) $< | sed -e "s/Total ticks : //"` | bc > $@

$(BENCHMARKS_COREMARK_R3KA_SCORES): %/score: %/image.ihx
	echo "scale=3;" 1966080 / `./execute_benchmark-r3ka $(R3KA_TTY) $< | sed -e "s/Total ticks : //"` | bc > $@

$(BENCHMARKS_COREMARK_R4K_SCORES): %/score: %/image.ihx
	echo "scale=3;" 1966080 / `./execute_benchmark-r4k $(R4K_TTY) $< | sed -e "s/Total ticks : //"` | bc > $@

$(BENCHMARKS_STDCBENCH_STM8_SCORES): %/score: %/image.ihx
	./execute_benchmark-stm8 $(STM8_TTY) $< | sed -e "s/stdcbench final score: //" > $@

$(BENCHMARKS_STDCBENCH_MCS51_SCORES): %/score: %/image.ihx
	./execute_benchmark-mcs51 $(MCS51_TTY) $< | sed -e "s/stdcbench final score: //" > $@

$(BENCHMARKS_STDCBENCH_R3KA_SCORES): %/score: %/image.ihx
	./execute_benchmark-r3ka $(R3KA_TTY) $< | sed -e "s/stdcbench final score: //" > $@

$(BENCHMARKS_STDCBENCH_R4K_SCORES): %/score: %/image.ihx
	./execute_benchmark-r4k $(R4K_TTY) $< | sed -e "s/stdcbench final score: //" > $@

