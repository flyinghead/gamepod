#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <dirent.h>

#include "backgroundLayer.h"
#include "imageLayer.h"
#include "loadpng.h"
#include "element_change.h"

#include "bcm_host.h"
#include "timer.h"
#include "volumegraph.h"

#define Y 5

extern DISPMANX_DISPLAY_HANDLE_T display;
extern DISPMANX_MODEINFO_T info;
extern const char *image_path;

static BACKGROUND_LAYER_T backgroundLayer;
static IMAGE_LAYER_T iconImgLayer;
static IMAGE_LAYER_T bargraphImgLayer;

static int backlightTimer = -1;

static char backlight_sysfs[PATH_MAX];
static int max_brightness;

static int read_sysfs_int(const char *path) {
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
	perror(path);
	return -1;
    }
    char line[10];
    int value =  -1;
    if (fgets(line, sizeof(line), fp) != NULL) {
	value = atoi(line);
    }
    fclose(fp);

    return value;
}

static int write_sysfs_int(const char *path, int i) {
    FILE *fp = fopen(path, "w");
    if (fp == NULL) {
	perror(path);
	return -1;
    }
    char value[10];
    sprintf(value, "%d", i);
    int rc = 0;
    if (fputs(value, fp) < 0) {
	perror(path);
	rc = -1;
    }
    fclose(fp);

    return rc;
}

void backlight_init() {
    initBackgroundLayer(&backgroundLayer, 0x8, 0);
    char path[PATH_MAX];
    sprintf(path, "%s/light.png", image_path);
    if (loadPng(&(iconImgLayer.image), path) == false)
    {
        fprintf(stderr, "unable to load light.png\n");
    }
    createResourceImageLayer(&iconImgLayer, 31000);

    sprintf(path, "%s/bargraph.png", image_path);
    if (loadPng(&(bargraphImgLayer.image), path) == false)
    {
        fprintf(stderr, "unable to load bargraph.png\n");
    }
    createResourceImageLayer(&bargraphImgLayer, 31001);

    DIR *pdir = opendir("/sys/class/backlight");
    if (pdir == NULL) {
	perror("opendir(/sys/class/backlight)");
	return;
    }
    const char *backlight_dev = NULL;
    while (true) {
	struct dirent *pentry = readdir(pdir);
	if (pentry == NULL) {
		if (errno != 0)
			perror("readdir");
		break;
	}
	if (!strcmp(".", pentry->d_name) || !strcmp("..", pentry->d_name))
		continue;
	backlight_dev = pentry->d_name;
	break;
    }
    closedir(pdir);

    if (backlight_dev == NULL)
	return;

    sprintf(path, "/sys/class/backlight/%s/max_brightness", backlight_dev);
    max_brightness = read_sysfs_int(path);

    sprintf(backlight_sysfs, "/sys/class/backlight/%s/brightness", backlight_dev);
}

static int getBrightness() {
    if (backlight_sysfs[0] == '\0')
	return -1;
    return read_sysfs_int(backlight_sysfs);
}

void backlight_destroy() {
    destroyBackgroundLayer(&backgroundLayer);
    destroyImageLayer(&iconImgLayer);
    destroyImageLayer(&bargraphImgLayer);
}

void backlight_hide() {
    if (bargraphImgLayer.element == 0)
        return;
    int result = 0;

    DISPMANX_UPDATE_HANDLE_T update = vc_dispmanx_update_start(0);
    result = vc_dispmanx_element_remove(update, backgroundLayer.element);
    assert(result == 0);
    backgroundLayer.element = 0;
    result = vc_dispmanx_element_remove(update, iconImgLayer.element);
    assert(result == 0);
    iconImgLayer.element = 0;
    result = vc_dispmanx_element_remove(update, bargraphImgLayer.element);
    bargraphImgLayer.element = 0;
    assert(result == 0);
    result = vc_dispmanx_update_submit_sync(update);
}

void backlight_run() {
    if (backlightTimer != -1 && timeout_passed(backlightTimer) == 1) {
        backlight_hide();
        backlightTimer = -1;
    }
}

static void setBarGraphWidth(int percent, DISPMANX_UPDATE_HANDLE_T update) {
    if (bargraphImgLayer.element == 0)
        return;
    vc_dispmanx_rect_set(&(bargraphImgLayer.srcRect),
                         0 << 16,
                         0 << 16,
                         (bargraphImgLayer.image.width * percent / 100) << 16,
                         bargraphImgLayer.image.height << 16);
    int width = bargraphImgLayer.image.width * percent / 100;
    if (width == 0)
        width = 1;
    vc_dispmanx_rect_set(&(bargraphImgLayer.dstRect),
                         (info.width + iconImgLayer.image.width + 20 - bargraphImgLayer.image.width) / 2,
                         Y + (iconImgLayer.image.height - bargraphImgLayer.image.height) / 2,
                         width,
                         bargraphImgLayer.image.height);
    bool localUpdate = false;
    if (update == 0) {
        update = vc_dispmanx_update_start(0);
        assert(update != 0);
        localUpdate = true;
    }
    int result = vc_dispmanx_element_change_attributes(update,
                                          bargraphImgLayer.element,
                                          ELEMENT_CHANGE_DEST_RECT,
                                          0,
                                          255,
                                          &(bargraphImgLayer.dstRect),
                                          &(bargraphImgLayer.srcRect),
                                          0,
                                          DISPMANX_NO_ROTATE);
    assert(result == 0);
    if (localUpdate) {
        result = vc_dispmanx_update_submit_sync(update);
        assert(result == 0);
    }
}

static void showBacklightGraph(int percent) {
    volgraph_hide();

    if (bargraphImgLayer.element != 0)
        return;
    int result = 0;

    DISPMANX_UPDATE_HANDLE_T update = vc_dispmanx_update_start(0);
    addElementBackgroundLayer(&backgroundLayer, display, update);
    addElementImageLayerOffset(&iconImgLayer,
			       (info.width - iconImgLayer.image.width - bargraphImgLayer.image.width) / 2,
			       Y,
			       display,
			       update);
    addElementImageLayerOffset(&bargraphImgLayer,
			       (info.width + iconImgLayer.image.width + 20 - bargraphImgLayer.image.width) / 2,
			       Y + (iconImgLayer.image.height - bargraphImgLayer.image.height) / 2,
			       display,
			       update);
    setBarGraphWidth(percent, update);
    result = vc_dispmanx_update_submit_sync(update);
    assert(result == 0);
}

void change_backlight(bool up) {
    int brightness = getBrightness();

    int percent = 0;
    if (brightness != -1 && max_brightness != 0)
	percent = brightness * 100 / max_brightness;

    showBacklightGraph(percent);
    if (brightness != -1) {
	brightness += (up ? 1 : -1);
	brightness = brightness > max_brightness ? max_brightness : brightness < 0 ? 0 : brightness;
	write_sysfs_int(backlight_sysfs, brightness);

	if (max_brightness != 0)
	    percent = brightness * 100 / max_brightness;
	setBarGraphWidth(percent, 0);
    }
    if (backlightTimer != -1)
        timeout_unset(backlightTimer);
    backlightTimer = timeout_set(2);

}
