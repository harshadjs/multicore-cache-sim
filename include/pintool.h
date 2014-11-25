#ifndef __PINTOOL_H
#define __PINTOOL_H

#include "pin.H"
#include "pin_profile.H"

void pin_instruction_handler(INS ins, void *v);
void pin_image_handler(IMG ins, void *v);

#endif
