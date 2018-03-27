//-------------------------------------------------------------------------
//
// The MIT License (MIT)
//
// Copyright (c) 2013 Andrew Duncan
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//-------------------------------------------------------------------------

#define _GNU_SOURCE

#include <assert.h>
#include <ctype.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "bcm_host.h"
#include "batlevel.h"
#include "volumegraph.h"
#include "timer.h"
#include "wifi.h"

//-------------------------------------------------------------------------

#define NDEBUG

//-------------------------------------------------------------------------

#ifndef IMAGE_PATH
#define IMAGE_PATH "."
#endif

const char *image_path = IMAGE_PATH;
const char *gpio_path = NULL;

const char *program = NULL;

//-------------------------------------------------------------------------

volatile bool run = true;

DISPMANX_DISPLAY_HANDLE_T display;
DISPMANX_MODEINFO_T info;

//-------------------------------------------------------------------------

static void
signalHandler(
    int signalNumber)
{
    switch (signalNumber)
    {
    case SIGINT:
    case SIGTERM:
        run = false;
        break;
    };
}

//-------------------------------------------------------------------------

static void usage(void)
{
    fprintf(stderr, "Usage: %s ", program);
    fprintf(stderr, "[-d <images directory>] [-c <charge indicator>] [-w <wifi interface>] [-p] <tty device>\n");
    fprintf(stderr, "    -d - set directory containing images (defauls to \"%s\")\n", IMAGE_PATH);
    fprintf(stderr, "    -c - gpio to indicate charge (e.g. /sys/class/gpio/gpio22/value)\n");
    fprintf(stderr, "    -w - set the wifi lan interface to monitor (e.g. wlan0)\n");
    fprintf(stderr, "    -p - ignore power off request\n");

    exit(EXIT_FAILURE);
}

//-------------------------------------------------------------------------

bool isCharging() {
    if (gpio_path == NULL)
        return false;
    int gpio_fd = open(gpio_path, O_RDONLY);
    if (gpio_fd == -1) {
        perror("Error opening GPIO value file");
        return false;
    }
    char charging;
    int i = read(gpio_fd, &charging, 1);
    if (i == -1) {
        perror("Error reading from GPIO file");
        close(gpio_fd);
        return false;
    }
    else if (i == 0) {
        fprintf(stderr, "EOF on GPIO file");
        close(gpio_fd);
        return false;
    }
    close(gpio_fd);
    
    return charging == '0';     // Active low
}

//-------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    uint32_t displayNumber = 0;

    program = basename(argv[0]);

    //---------------------------------------------------------------------

    int opt = 0;
    bool ignore_poweroff = false;

    while ((opt = getopt(argc, argv, "d:c:w:p")) != -1)
    {
        switch(opt)
        {
        case 'd':
            image_path = optarg;
            break;

        case 'c':
            gpio_path = optarg;
            break;
        
        case 'w':
            wifi_ifname = optarg;
            break;
            
	case 'p':
	    ignore_poweroff = true;
	    break;

        default:
            usage();
            break;
        }
    }

    //---------------------------------------------------------------------

    if (optind >= argc)
    {
        usage();
    }

    //---------------------------------------------------------------------

    if (signal(SIGINT, signalHandler) == SIG_ERR)
    {
        perror("installing SIGINT signal handler");
        exit(EXIT_FAILURE);
    }

    //---------------------------------------------------------------------

    if (signal(SIGTERM, signalHandler) == SIG_ERR)
    {
        perror("installing SIGTERM signal handler");
        exit(EXIT_FAILURE);
    }

    bcm_host_init();

    display = vc_dispmanx_display_open(displayNumber);
    assert(display != 0);

    int result = vc_dispmanx_display_get_info(display, &info);
    assert(result == 0);

    timeout_init();
    batlevel_init();
    volgraph_init();
    wifi_init();

    //---------------------------------------------------------------------

    int serialfd = -1;
    char line[256];
    char *p = line;

    while (run)
    {
        volgraph_run();
        batlevel_run();
        wifi_run();

        if (serialfd < 0) {
            serialfd = open(argv[optind], O_RDONLY|O_NOCTTY);
            if (serialfd < 0) {
                perror("Can't open serial port");
                // TODO Use timer
                sleep(1);
                continue;
            }
        }
        int i = read(serialfd, p, 1);
        switch (i) {
            case -1:
                if (errno != EINTR) {
                    perror("Error reading from serial device");
                }
                continue;
            
            case 0:
                fprintf(stderr, "EOF reading from serial device\n");
                close(serialfd);
                serialfd = -1;
                continue;
            
            case 1:
                if (*p != '\n' && *p != '\r') {
                    if (p - line < sizeof(line) - 1)
                        p++;
                    continue;
                }
                break;
        }
        *p = 0;
        p = line;

        if (!strncmp(line, "VBAT=", 5)) {
            double vbat = atof(line + 5);
            set_batlevel(vbat, isCharging());
        }
        else if (!strncmp(line, "VOLUME", 6)) {
            bool up = line[6] == '+';
            change_volume(up);
        }
	else if (!strcmp(line, "POWEROFF") && !ignore_poweroff) {
	    pid_t pid = fork();
    	    if (pid == -1) {
        	perror("fork");
	    }
    	    else if (pid == 0) {
        	// Child
		execlp("poweroff", "poweroff", "--no-wall", NULL);
        	exit(1);
        	return -1;
	    }
	}
    }

    //---------------------------------------------------------------------
    
    if (serialfd >= 0)
        close(serialfd);		// This blocks. Use: sudo setserial /dev/ttyACM0 closing_wait none

    wifi_destroy();
    batlevel_destroy();
    volgraph_destroy();
    timeout_done();

    result = vc_dispmanx_display_close(display);
    assert(result == 0);

    return 0;
}

