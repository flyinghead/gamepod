//
//  batlevel.h
//  
//
//  Created by Raphael Jean-Leconte on 15/03/2018.
//

#ifndef batlevel_h
#define batlevel_h

void batlevel_init();
void batlevel_destroy();
void batlevel_run();
void set_batlevel(double volts, bool charging);

#endif /* batlevel_h */
