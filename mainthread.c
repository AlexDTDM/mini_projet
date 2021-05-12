/*
 * mainthread.c
 *
 *  Created on: 24 avr. 2021
 *      Author: alexandredemontleau
 */

#include "ch.h"
#include "hal.h"
#include <chprintf.h>
#include <usbcfg.h>

#include <main.h>
#include <camera/po8030.h>

#include <process_image.h>
#include <motors.h>
#include <tof.h>
#include <mainthread.h>
#include <motormove.h>

#define RESEARCH 0
#define COLOR 1
#define ACTION 2

int state = RESEARCH;

int status(void){
	return state;
}


static THD_WORKING_AREA(waMainThread, 1024);
static THD_FUNCTION(MainThread, arg) {

	chRegSetThreadName(__FUNCTION__);
	(void)arg;
	//bool changement_state = 0 ;

	//uint16_t det = 0;

	 while(1){
		 chThdSleepMilliseconds(1000);
		 switch(state)
		    	{
		       case RESEARCH:
		       detection_objet();
		       state = COLOR;
		       chprintf((BaseSequentialStream *)&SDU1, "[STATE = %d]", state);
		    	break;

		    	case COLOR: //pas encore de cas si trouve pas la couleur.
		    	chThdSleepMilliseconds(1000);
		    	state = ACTION;
		    	 chprintf((BaseSequentialStream *)&SDU1, "[STATE = %d]", state);
		    	break;

		    	case ACTION:
		        attack_return();
		    	state = RESEARCH;
		    	break;
		    }
	 }

}


void mainthread_start(void){
	chThdCreateStatic(waMainThread, sizeof(waMainThread), NORMALPRIO, MainThread, NULL);
}





