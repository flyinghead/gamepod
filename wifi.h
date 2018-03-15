//
//  wifi.h
//  
//
//  Created by Raphael Jean-Leconte on 15/03/2018.
//

#ifndef wifi_h
#define wifi_h

extern const char *wifi_ifname;

void wifi_init();
void wifi_destroy();
void wifi_run();

#endif /* wifi_h */
