#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ch.h"
#include "hal.h"
#include "memory_protection.h"
#include <usbcfg.h>
#include <main.h>
#include <motors.h>
#include <camera/po8030.h>
#include <chprintf.h>
#include <process_image.h>
#include <motormove.h>
#include <mainthread.h>
#include <tof.h>
#include "sensors/VL53L0X/VL53L0X.h"
#include <audio/audio_thread.h>
#include <audio/play_melody.h>

#define STACK_CHK_GUARD 0xe2dee396
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;


int main(void){

    halInit();
    chSysInit();
    mpu_init();
    
    
    dcmi_start(); //starts the camera
	po8030_start();
	
	motors_init(); //inits the motors
	
	VL53L0X_start(); //init distance sensor

	process_image_start();

	dac_start(); //intits the music
	playMelodyStart();

	mainthread_start(); //inits our main thread

    /* Infinite loop. */
    while (1) {
    	//waits 1 second
    	chThdSleepMilliseconds(1000);
    }
}


void __stack_chk_fail(void){
   chSysHalt("Stack smashing detected");
}

