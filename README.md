# pokecalc
a command line pokemon stat calculator for those command line wizards who are also pokemon masters
## USAGE  
Pokecalc was designed with a sister tool that's also on my github, pokepull. Typically when I use pokecalc I pipe the output of pokepull into it.  
Example usage:  
`$ pokepull garchomp | pokecalc -s attack,252 -s speed,252 -n adamant`
Example output:  
```
== garchomp ==
level: 100
nature: adamant
hp: 357
attack: 394
defense: 226
spattack: 185
spdefense: 196
speed: 303
```
Example usage:  
`$ pokepull ferrothorn | pokecalc -s attack,252 -s speed,0,0 -n sassy`
Example output:  
```
== ferrothorn ==
level: 100
nature: sassy
hp: 289
attack: 287
defense: 361
spattack: 268
spdefense: 158
speed: 40
```

## OPTIONS
-v: verbose flag  
-p: specify json in a string on the command line
-f: specify a json file to read from [atm either -f or -p is required]  
 (( default behavior is to read json from stdin ))  
-s: specify EVs and IVs for a stat, use in the form `-s attack,252,31`
	where attack is the name of the stat, 252 is the EV, and 31 is the IV.
	The IV will default to 31, so the same stat definition could be written
	as `-s attack,252`; however, an EV is always required.  
-l: specify pokemon's level (( defaults to 100 ))  
-n: specify pokemon's nature (( defaults to serious ))  
