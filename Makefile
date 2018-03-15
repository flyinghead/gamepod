PREFIXDIR=/usr/local
#escaped for sed
SERIALDEV=\/dev\/ttyACM0
OBJS=main.o soundvolume.o volumegraph.o timer.o batlevel.o
BIN=gamepod-overlay

CFLAGS+=-Wall -g -O3 -Iraspidmx/common $(shell libpng-config --cflags) -DIMAGE_PATH='"$(PREFIXDIR)/share/gamepod-overlay"'
LDFLAGS+=-L/opt/vc/lib/ -lbcm_host -lm $(shell libpng-config --ldflags) -Lraspidmx/lib -l:libraspidmx.a -lasound -lrt

INCLUDES+=-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux

all: raspidmx/lib/libraspidmx.a $(BIN)

raspidmx/lib/libraspidmx.a:
	cd raspidmx/lib && $(MAKE)

%.o: %.c
	@rm -f $@ 
	$(CC) $(CFLAGS) $(INCLUDES) -g -c $< -o $@ -Wno-deprecated-declarations

$(BIN): $(OBJS)
	$(CC) -o $@ -Wl,--whole-archive $(OBJS) $(LDFLAGS) -Wl,--no-whole-archive -rdynamic

clean:
	@rm -f $(OBJS)
	@rm -f $(BIN)

install: $(BIN)
	install $(BIN) $(PREFIXDIR)/bin
	install -d $(PREFIXDIR)/share/gamepod-overlay
	install *.png $(PREFIXDIR)/share/gamepod-overlay
	if grep -q setserial /etc/rc.local ; then \
		sed -i "s/^.*setserial.*$$/setserial $(SERIALDEV) closing_wait none/g" /etc/rc.local ; \
	else \
		sed -i "s/^exit 0/setserial $(SERIALDEV) closing_wait none\\nexit 0/g" /etc/rc.local ; \
	fi
	if grep -q gamepod-overlay /etc/rc.local ; then \
		sed -i "s/^.*gamepod-overlay.*$$/\/usr\/local\/bin\/gamepod-overlay $(SERIALDEV) \&/g" /etc/rc.local ; \
	else \
		sed -i "s/^exit 0/\/usr\/local\/bin\/gamepod-overlay $(SERIALDEV) \&\\nexit 0/g" /etc/rc.local ; \
	fi

