#include <assert.h>
#include "imageLayer.h"
#include "loadpng.h"
#include "timer.h"

extern DISPMANX_DISPLAY_HANDLE_T display;
extern DISPMANX_MODEINFO_T info;
extern char *image_path;

const int Layer = 30000;

IMAGE_LAYER_T imageLayer;
int xOffset, yOffset;
int batteryBlinkTimer = -1;
int currentBatLevel = -1;

void batlevel_init() {
    xOffset = info.width - 30;
    yOffset = 10;
}

void batlevel_destroy() {
    if (imageLayer.resource != 0) {
	destroyImageLayer(&imageLayer);
	imageLayer.resource = 0;
    }
    if (batteryBlinkTimer != -1) {
        timeout_unset(batteryBlinkTimer);
	batteryBlinkTimer = -1;
    }
    currentBatLevel = -1;
}

void batlevel_run() {
    if (batteryBlinkTimer != -1 && timeout_passed(batteryBlinkTimer) == 1) {
        DISPMANX_UPDATE_HANDLE_T update = vc_dispmanx_update_start(0);
        assert(update != 0);
        if (imageLayer.element) {
            vc_dispmanx_element_remove(update, imageLayer.element);
            imageLayer.element = 0;
        } else {
            addElementImageLayerOffset(&imageLayer,
                                       xOffset,
                                       yOffset,
                                       display,
                                       update);
        }
        vc_dispmanx_update_submit_sync(update);
        timeout_unset(batteryBlinkTimer);
        batteryBlinkTimer = timeout_set(0.5);
        assert(batteryBlinkTimer != -1);
    }
}

void set_batlevel(double vbat, bool charging) {
    //printf("VBat=%f\n", vbat);
    int batLevel = 0;
    if (!charging) {
        if (vbat >= 3.6) {
            if (vbat < 3.638)
                batLevel = 1;
            else if (vbat < 3.678)
                batLevel = 2;
            else if (vbat < 3.716)
                batLevel = 3;
            else if (vbat < 3.748)
                batLevel = 4;
            else if (vbat < 3.786)
                batLevel = 5;
            else if (vbat < 3.827)
                batLevel = 6;
            else if (vbat < 3.873)
                batLevel = 7;
            else if (vbat < 3.899)
                batLevel = 8;
            else if (vbat < 3.939)
                batLevel = 9;
            else
                batLevel = 10;
        }
    } else {
	//vbat *= 0.99;	// Correction factor because we don't get 5V (arduino vref) when charging
        if (vbat < 4.023)
            batLevel = 11;
        else if (vbat < 4.072)
            batLevel = 12;
        else if (vbat < 4.16)
            batLevel = 13;
        else
            batLevel = 14;
    }
    if (batLevel != currentBatLevel) {
        //printf("Changing to battery level %d\n", batLevel);
        currentBatLevel = batLevel;

        char imagePath[512];
        sprintf(imagePath, "%s/battery%d.png", image_path, batLevel);
        if (loadPng(&(imageLayer.image), imagePath) == false)
        {
            fprintf(stderr, "unable to load %s\n", imagePath);
        }
        if (imageLayer.resource == 0 || imageLayer.element == 0) {
            DISPMANX_UPDATE_HANDLE_T update = vc_dispmanx_update_start(0);
            assert(update != 0);
            
            if (imageLayer.resource == 0)
                createResourceImageLayer(&imageLayer, Layer);
            else {
                int result = vc_dispmanx_resource_write_data(imageLayer.resource,
                                                     imageLayer.image.type,
                                                     imageLayer.image.pitch,
                                                     imageLayer.image.buffer,
                                                     &(imageLayer.bmpRect));
                assert(result == 0);
            }
            
            addElementImageLayerOffset(&imageLayer,
                                       xOffset,
                                       yOffset,
                                       display,
                                       update);
        
            vc_dispmanx_update_submit_sync(update);
        }
        else {
            changeSourceAndUpdateImageLayer(&imageLayer);
        }
        if (batLevel == 0 && batteryBlinkTimer == -1) {
            batteryBlinkTimer = timeout_set(0.5);
        }
        else if (batLevel > 0 && batteryBlinkTimer != -1) {
            timeout_unset(batteryBlinkTimer);
            batteryBlinkTimer = -1;
        }
    }
}
