# BOOT-AUTO R3 Hardware Test

1. Extract the R3 FAT32 USB package to the test USB and boot Trinity normally.
2. Do **not** run `MC2START`, `MC3START`, `MC4START`, `MC5START`, `MC6START`, or `MC7START`.
3. After the desktop appears, wait about 3 seconds.
4. Open Console and run `MC` once.

## Required success state

The first summary should show:

`MC discovered=8 preparedAP=0 hw-online=8 sched-online=8 cap=8`

CPU2-CPU7 must each show `hw=1 sched=1 tlb=1 tm=1 irq=1`.

R3 also adds a new automatic-admission line. On this Acer Nitro 5 the success target is:

`AUTO status=2 admitted=6 next=8 target=8 failcpu=none code=0`

If `AUTO status=1`, wait two seconds and run `MC` once more.

If `AUTO status=0`, the Desktop coordinator did not issue the automatic sequence. Stop and capture the complete MC screen.

If `AUTO status=3`, automatic admission contained. The same line identifies `failcpu=` and `code=`. Do not manually start any later AP on that boot; capture the complete MC screen.

5. If automatic admission passed, open Writer, Files, and Browser. They must launch normally.
6. Use the desktop for several minutes and run `MC` again. All eight CPUs must remain online.
7. For automatic-boot production confirmation, `MC8SOAK START`, normal use for at least 120 seconds, then `MC8SOAK FINISH` may be used. Target remains `proof=65535/65535`.

## Qualification result

R3 passed the core automatic-admission hardware test on the Acer Nitro 5: all eight logical CPUs were online automatically and the `AUTO` diagnostic reported complete with no failing CPU and code 0.
