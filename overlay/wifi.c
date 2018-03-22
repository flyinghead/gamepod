//
//  wifi.c
//  
//
//  Created by Raphael Jean-Leconte on 15/03/2018.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <assert.h>

#include "imageLayer.h"
#include "loadpng.h"
#include "wifi.h"
#include "timer.h"

// Update period in seconds
#define UPDATE_PERIOD 3.0

// wlan0 down:
//# iwconfig lan0
// wlan0  IEEE802.11 ESSID:off/any
//        Mode:Managed Access Point: Not-Associated
//        Retry short limit...
//        ...

extern DISPMANX_DISPLAY_HANDLE_T display;
extern DISPMANX_MODEINFO_T info;
extern char *image_path;

const char *wifi_ifname = NULL;

static int connected;
static int quality_percent = 0;

static const int WifiLayer = 30020;
static IMAGE_LAYER_T wifiImgLayer;
static int wifi_timer = -1;
static int current_wifi_level = -1;

void wifi_init() {
    if (wifi_ifname != NULL)
        wifi_timer = timeout_set(0.1);  // Do the first update ASAP
}

void wifi_destroy() {
    if (wifiImgLayer.resource != 0)
        destroyImageLayer(&wifiImgLayer);
    if (wifi_timer != -1)
        timeout_unset(wifi_timer);
}

static int run_iwconfig(const char *ifname) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return -1;
    }
    if (pid == 0) {
        // Child
        close(1);
        open("/tmp/iwconfig.tmp", O_WRONLY|O_TRUNC|O_CREAT, 0644);
        if (execlp("iwconfig", "iwconfig", ifname, NULL)) {
            perror("exec iwconfig failed");
        }
        exit(1);
        return -1;
    } else {
        // Parent
        int status;
        waitpid(pid, &status, 0);
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            if (WIFEXITED(status))
                fprintf(stderr, "iwconfig command failed rc=%d\n", WEXITSTATUS(status));
            else
                fprintf(stderr, "iwconfig killed by signal?\n");
            return -1;
        }
        // Success
        FILE *fp = fopen("/tmp/iwconfig.tmp", "r");
        if (!fp) {
            perror("Can't open iwconfig output file");
            return -1;
        }
        char line[256];
                
        while (fgets(line, sizeof(line), fp)) {
            //printf("iwconfig out: %s", line);
            char *p = strstr(line, "Access Point:");
            if (p) {
                p += 14;
                if (!strncmp(p, "Not-Associated", 14)) {
                    connected = 0;
                    quality_percent = 0;
                    break;
                }
            }
            p = strstr(line, "Link Quality=");
            if (p) {
                p += 13;
                char *slash = strchr(p, '/');
                if (slash == NULL) {
                    fprintf(stderr, "Unparseable iwconfig output: %s", line);
                } else {
                    char *end = strchr(slash, ' ');
                    if (end == NULL)
                        fprintf(stderr, "Unparseable iwconfig output: %s", line);
                    else {
                        *slash = '\0';
                        *end = '\0';
                        connected = 1;
                        quality_percent = atoi(p) * 100 / atoi(slash + 1);
                    }
                }
            }
        }
        fclose(fp);
        unlink("/tmp/iwconfig.tmp");

        return 0;
    }
}

void wifi_run() {
    if (wifi_ifname == NULL)
        // Shouldn't happen
        return;
    if (timeout_passed(wifi_timer) == 1) {
        if (!run_iwconfig(wifi_ifname)) {
            int wifi_level = -1;
            if (!connected)
                wifi_level = 0;
            else if (quality_percent >= 71)
                wifi_level = 3;
            else if (quality_percent >= 43)
                wifi_level = 2;
            else
                wifi_level = 1;
            if (wifi_level != current_wifi_level) {
                printf("Changing wifi level to %d\n", wifi_level);
                current_wifi_level = wifi_level;
                char imagePath[512];
                sprintf(imagePath, "%s/wifi%d.png", image_path, wifi_level);
                if (loadPng(&(wifiImgLayer.image), imagePath) == false)
                {
                    fprintf(stderr, "unable to load %s\n", imagePath);
                }
                if (wifiImgLayer.resource == 0) {
                    createResourceImageLayer(&wifiImgLayer, WifiLayer);
                    DISPMANX_UPDATE_HANDLE_T update = vc_dispmanx_update_start(0);
                    assert(update != 0);
                    addElementImageLayerOffset(&wifiImgLayer,
                                               10,
                                               10,
                                               display,
                                               update);
                    
                    vc_dispmanx_update_submit_sync(update);
                }
                else {
                    changeSourceAndUpdateImageLayer(&wifiImgLayer);
                }
            }
        }
        timeout_unset(wifi_timer);
        wifi_timer = timeout_set(UPDATE_PERIOD);
    }
}

