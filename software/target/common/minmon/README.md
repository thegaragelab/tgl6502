20-Dec-2014 ShaneG

This contains source for a minimal monitor program (MinMon). This is designed to
make it relatively easy to manipulate memory and IO on the target device while
remaining portable. It does require a C compiler for the target.

# Memory Layout

MinMon is designed to be placed in the ROM of the target but will copy itself
to a RAM page on initialisation.

