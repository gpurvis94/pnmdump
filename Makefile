all: pnmdump.exe

pnmdump.exe: pnmdumpmain.c
	gcc -std=c99 -Wall $< -o $@

test: pnmdump.exe
	python tests/runtests-1.0.py pnmdump.exe
