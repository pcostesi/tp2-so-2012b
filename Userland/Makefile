all:	libc shell

libc:
	@cd libc; make
	@echo "LibC built"

shell:	libc
	@cd shell; make
	@echo "Shell built"

tpl:	libc
	@cd tpl; make
	@echo "Template built"

clean:
	@cd shell; make clean
	@cd libc; make clean
	@cd tpl; make clean
	- rm -rf *.bin


.PHONY: clean all libc shell tpl
