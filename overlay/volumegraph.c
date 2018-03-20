#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "backgroundLayer.h"
#include "imageLayer.h"
#include "loadpng.h"
#include "element_change.h"

#include "bcm_host.h"
#include "soundvolume.h"
#include "timer.h"

#define Y 5

extern DISPMANX_DISPLAY_HANDLE_T display;
extern DISPMANX_MODEINFO_T info;
extern const char *image_path;

static BACKGROUND_LAYER_T backgroundLayer;
static IMAGE_LAYER_T iconImgLayer;
static IMAGE_LAYER_T bargraphImgLayer;

static int volumeGraphTimer = -1;

void volgraph_init() {
    //printf("Volume %d\n", getAlsaMasterVolume());
    
    initBackgroundLayer(&backgroundLayer, 0x8, 0);
    char path[256];
    sprintf(path, "%s/speaker.png", image_path);
    if (loadPng(&(iconImgLayer.image), path) == false)
    {
        fprintf(stderr, "unable to load speaker.png\n");
    }
    createResourceImageLayer(&iconImgLayer, 31000);
    
    sprintf(path, "%s/bargraph.png", image_path);
    if (loadPng(&(bargraphImgLayer.image), path) == false)
    {
        fprintf(stderr, "unable to load bargraph.png\n");
    }
    createResourceImageLayer(&bargraphImgLayer, 31001);
}

void volgraph_destroy() {
    destroyBackgroundLayer(&backgroundLayer);
    destroyImageLayer(&iconImgLayer);
    destroyImageLayer(&bargraphImgLayer);
}

static void hideVolumeGraph() {
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

void volgraph_run() {
    if (volumeGraphTimer != -1 && timeout_passed(volumeGraphTimer) == 1) {
        hideVolumeGraph();
        volumeGraphTimer = -1;
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

static void showVolumeGraph(int percent) {
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

void change_volume(bool up) {
    int volume = getAlsaMasterVolume();
    showVolumeGraph(volume);
    volume += 5 * (up ? 1 : -1);
    volume = volume > 100 ? 100 : volume < 0 ? 0 : volume;
    setAlsaMasterVolume(volume);
    setBarGraphWidth(volume, 0);
    
    if (volumeGraphTimer != -1)
        timeout_unset(volumeGraphTimer);
    volumeGraphTimer = timeout_set(2);

}
