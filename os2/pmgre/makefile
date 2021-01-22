all: pmgre.lib

pmgre.lib: pmgre.exp
	@echo * Creating PMGRE import library
	wlib -q -b -n -c -pa -s -t -zld -ii -io pmgre.lib @pmgre.exp

clean: .symbolic
	rm -f pmgre.lib
