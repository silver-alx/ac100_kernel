
all:
	make -C UTIL/
#	$(SHELL) cp_util.sh
	cp UTIL/os/linux/modules.order MODULE/os/linux/
	cp UTIL/os/linux/Module.symvers MODULE/os/linux/
	make -C MODULE/
	$(SHELL) cp_module.sh
	cp MODULE/os/linux/modules.order NETIF/os/linux/
	cp MODULE/os/linux/Module.symvers NETIF/os/linux/
	make -C NETIF/

clean:
	make -C UTIL/ clean
	make -C MODULE/ clean
	make -C NETIF/ clean

install:
	make -C UTIL/ install
	make -C MODULE/ install
	make -C NETIF/ install

uninstall:
	make -C UTIL/ uninstall
	make -C MODULE/ uninstall
	make -C NETIF/ uninstall
