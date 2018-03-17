# pokecalc
a command line pokemon stat calculator for those command line wizards who are also pokemon masters
## USAGE
Example usage:  
`$ pokecalc -f bulbasaur.json -s spattack,252 -l 50 -n modest -s speed,252`  
Example output:  
```
== Bulbasaur ==
level: 50
nature: modest
hp: 231
attack: 120
defense: 134
spattack: 251
spdefense: 166
speed: 189
```

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
