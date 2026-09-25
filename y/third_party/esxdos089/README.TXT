esxDOS v0.8.9
(c) 2005-2021 Papaya Dezign
---------------------------

This version has the following (notable) new features (for detailed info check the changelog):

* Support for SCL (TR-DOS) disk images
* TR-DOS MOVE and FORMAT commands are now implemented
* Z80DMA timings are configurable
* New/Updated Commands: .tar, .launcher, .te, .gsc, .drives and .rm

How to (safely) install/upgrade:
--------------------------------

1. Copy esxide.tap/esxmmc.tap/mb03plus.tap (depending on if you have a DivIDE, DivMMC or MB03+, obviously) to your CF/SD/HDD, load it and follow the instructions. Power Off and remove media.
2. Copy the SYS, BIN (and /TMP if using DivIDE) directories to your CF/SD/HDD.
3. Insert media. Power On. Reset while keeping space pressed to reinit if needed.

Notes:
------

a) If you replace the SYS directory *before* flashing the new ROM, your previous ROM of esxDOS will not boot correctly.
b) Questions? Bugreports? Complaints about lack of LFN/instructions? Ask on the usual places or send an e-mail to: bugs at esxdos dot org

Changelog
---------

[11/11/2020] SD/MMC Driver: Fixed critical bug when extending directories (lordcoxis + reported by SCjoe)
[09/01/2021] auto-LOAD: Copying arguments was using wrong syscall (lordcoxis + reported by UB880D)
[09/01/2021] FAT Driver: Fixed a *critical* bug which corrupted the root directory when the disk was full (lordcoxis + reported by UB880D)
[07/03/2021] Z80DMA: Fixed a bug in the sector write sequence (lordcoxis + reported by LMN128)
[18/03/2021] vDisk: Added support for SCL (TR-DOS) disk images (lordcoxis)
[18/03/2021] TR-DOS: Updated TR-DOS Navigator to V0.75.9b (lordcoxis)
[18/03/2021] TR-DOS: Fixed a bug affecting (at least) "Star Heritage" game (lordcoxis + reported by Alcoholics Anonymous)
[18/03/2021] TR-DOS: Fixed a bug affecting (at least) "Tiratok" demo (lordcoxis + reported by Nihirash)
[19/03/2021] TR-DOS: Fixed a bug affecting (at least) "TASM 4.12" assembler (lordcoxis + reported by Nihirash)
[19/03/2021] TR-DOS: Fixed a bug affecting (at least) "Spectrofon" diskmag (lordcoxis + reported by Nihirash)
[19/03/2021] TR-DOS: Added missing MOVE and FORMAT commands (lordcoxis)
[19/03/2021] NMI: Added support for loading SCL files from the browser (UB880D)
[20/03/2021] NMI: Added HiResColour modes reset on NMI entry and restore on exit (lordcoxis)
[20/03/2021] Z80DMA: Added "DMA_IO" and "DMA_RAM" config parameters to ESXDOS.CFG to set DMA timings (lordcoxis + requested by LMN128)
[25/03/2021] TR-DOS: Fixed a bug that afected SCL images with more than 16 files (lordcoxis)
[25/03/2021] auto-LOAD: Set default colours to black paper + white ink, configurable in ESXDOS.CFG (lordcoxis)
[25/03/2021] BASIC: Fixed 2 bugs related to (patched) 128K ROM support, it should now be (almost) fully usable (lordcoxis)
[25/03/2021] NMI: Visual improvements (UB880D)
[25/03/2021] Commands: Added .tar and updated .launcher (UB880D)
[30/03/2021] Commands: Added .te command (nagydani)
[30/03/2021] Commands: Updated .drives and .rm (Dr. Slump)
[04/04/2021] Tape Emulator: Fixed a critical bug when attaching .tap files that would cause all sorts of instability (lordcoxis+UB880D)
[04/04/2021] Kernel: Saved HL, IX and IY registers before RTC.SYS call (UB880D)
[04/04/2021] TR-DOS: Fixed a bug affecting (at least) "ZX-Format" diskmag (lordcoxis + reported by Nihirash)
[11/04/2021] BASIC: Fixed X_PTR overwrite on outside syscall entry (lordcoxis + reported by UB880D and SCjoe)
[17/04/2021] Commands: Added .gsc command (Nihirash)

Thanks/Greets
-------------

Greets to all the usual peeps (y'all know who you are - just check v0.8.5's README if in doubt!).
Special thanks to all the great "hardware benefactors": Ben Versteeg, LMN128, Pavel Urbancik, Pavel Vymetalek, SpecNext Team, TFW8b, Tygrys, UB880D, Zaxon!

Known Bugs
----------

BASIC/Tape Emulator: You can overwrite ESXDOS system using LOAD CODE that crosses $2000
Commands: Proper argument/syntax checking is not done yet on most commands
Commands/BASIC: No wildcards yet
FAT Driver: rename() isn't even remotely posix compliant
FAT Driver: Directories read-only attribute isn't always respected
BIOS: Version of system file isn't checked against the ROM version (bad things will happen if there's a mismatch)

17.04.2021 / Papaya Dezign - All rights perversed
