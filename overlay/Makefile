PREFIXDIR=/usr/local
#escaped for sed
SERIALDEV=\/dev\/ttyACM0
WIFI_IF=wlan0
OBJS=main.o soundvolume.o volumegraph.o timer.o batlevel.o wifi.o backlight.o
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
	dpkg-query -W setserial > /dev/null 2>&1 || apt-get install -q setserial
	install $(BIN) $(PREFIXDIR)/bin
	install -d $(PREFIXDIR)/share/gamepod-overlay
	install -m 644 *.png $(PREFIXDIR)/share/gamepod-overlay
	if [ -f /usr/bin/pigs ]; \
	then \
		install -m 644 gamepod-overlay.service /etc/systemd/system; \
	else \
		install -m 644 gamepod-overlay-v2.service /etc/systemd/system/gamepod-overlay.service; \
	fi
	systemctl enable gamepod-overlay.service
	systemctl restart gamepod-overlay.service

uninstall:
	-systemctl stop gamepod-overlay.service
	-systemctl disable gamepod-overlay.service
	rm -f $(PREFIXDIR)/bin/$(BIN)
	rm -rf $(PREFIXDIR)/share/gamepod-overlay
	rm -f /etc/systemd/system/gamepod-overlay.service
