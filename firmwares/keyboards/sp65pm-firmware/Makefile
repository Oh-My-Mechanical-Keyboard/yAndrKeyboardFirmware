DIRS=sp65pm-receiver-basic/custom/armgcc sp65pm-keyboard-basic/custom/armgcc

all::
	make -C sp65pm-receiver-basic/custom/armgcc; \
	make -C sp65pm-keyboard-basic/custom/armgcc keyboard_side=left; \
	make -C sp65pm-keyboard-basic/custom/armgcc keyboard_side=right; \
	make -C sp65pm-keyboard-basic/custom/armgcc keyboard_side=pad; \

clean::
	make -C sp65pm-receiver-basic/custom/armgcc clean; \
	make -C sp65pm-keyboard-basic/custom/armgcc clean; \
	make -C sp65pm-keyboard-basic/custom/armgcc clean; 