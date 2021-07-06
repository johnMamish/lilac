all: serialmic.bin

serialmic.bin: serialmic.v serialmic.pcf serialmic.yosys
	yosys -s serialmic.yosys
	nextpnr-ice40 --up5k --package sg48 --pcf serialmic.pcf --json serialmic.json --asc serialmic.asc
	icepack serialmic.asc serialmic.bin

clean:
	rm serialmic.bin serialmic.asc serialmic.json

.PHONY: clean
