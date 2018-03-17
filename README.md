# pokecalc
a command line pokemon stat calculator for those command line wizards who are also pokemon masters
## OPTIONS
-v: verbose flag  
-p: specify json in a string on the command line [won't stick around]  
-f: specify json file to read from [atm either -f or -p is required]  
-s: specify EVs and IVs for a stat, use in the form `-s attack,252,31`
	where attack is the name of the stat, 252 is the EV, and 31 is the IV.
	The IV will default to 31, so the same stat definition could be written
	as `-s attack,252`; however, and EV is always required.  
-l: specify pokemon's level  
-n: specify pokemon's nature  
