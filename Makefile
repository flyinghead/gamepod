OBJS=main.o soundvolume.o volumegraph.o timer.o batlevel.o
BIN=gamepod-overlay

CFLAGS+=-Wall -g -O3 -Iraspidmx/common $(shell libpng-config --cflags)
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
