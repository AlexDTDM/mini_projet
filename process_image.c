#include "ch.h"
#include "hal.h"
#include <chprintf.h>
#include <usbcfg.h>
#include <main.h>
#include <camera/po8030.h>
#include <process_image.h>
#include <motors.h>
#include <mainthread.h>

#define THRESHOLD 10
#define INIT 0
#define COLOR 1
#define DOUBLE_INCREMENT 2
bool red_or_blue = true;
#define MASK 0x1F


//semaphore
static BSEMAPHORE_DECL(image_ready_sem, true);


bool red_blue(void){
	return red_or_blue;
}



static THD_WORKING_AREA(waCaptureImage, 256);
static THD_FUNCTION(CaptureImage, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;

	//Takes pixels 0 to IMAGE_BUFFER_SIZE of the line 10 + 11 (minimum 2 lines because reasons)
	po8030_advanced_config(FORMAT_RGB565, 0, 10, IMAGE_BUFFER_SIZE, 2, SUBSAMPLING_X1, SUBSAMPLING_X1); //subsampling_x4
	dcmi_enable_double_buffering();
	dcmi_set_capture_mode(CAPTURE_ONE_SHOT);
	dcmi_prepare();

    while(1){
        //starts a capture
		dcmi_capture_start();
		//waits for the capture to be done
		wait_image_ready();
		//signals an image has been captured
		chBSemSignal(&image_ready_sem);
    }
}



static THD_WORKING_AREA(waProcessImage, 1536);
static THD_FUNCTION(ProcessImage, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;

	uint8_t *img_buff_ptr;
	uint8_t image[IMAGE_BUFFER_SIZE] = {INIT};
    uint8_t image_blue[IMAGE_BUFFER_SIZE] = {INIT};
	uint32_t image_blue_moy = INIT; //uint_16
	uint32_t image_red_moy = INIT; //uint16

    while(1){

    	state = status(); //Ce state permet de mettre  thread en sleep dans tout les autres cas sauf le case COLOR

    	if(state == COLOR){

    		//waits until an image has been captured
	        chBSemWait(&image_ready_sem); 

			//gets the pointer to the array filled with the last image in RGB565
			img_buff_ptr = dcmi_get_last_image_ptr();

			for(uint16_t i = 0 ; i < (2 * IMAGE_BUFFER_SIZE) ; i+=DOUBLE_INCREMENT){

				image[i/2] = (((uint8_t)img_buff_ptr[i])>>3)&MASK; //extrait les bits qui correspondent au rouge
	            image_blue[i/2] = ((uint8_t)img_buff_ptr[i+1])&MASK;  //extrait les bits qui correspondent au bleu

			}

			for(uint16_t i = 2*(IMAGE_BUFFER_SIZE/2 - THRESHOLD) ; i < (2 * (IMAGE_BUFFER_SIZE/2 + THRESHOLD)) ; i+=DOUBLE_INCREMENT){

				image_blue_moy = image_blue_moy + image_blue[i/2];
				image_red_moy = image_red_moy + image[i/2];
			}

			image_red_moy = image_red_moy/(2*THRESHOLD);
			image_blue_moy = image_blue_moy/(2*THRESHOLD);

			if(image_red_moy > image_blue_moy){

					red_or_blue = true;
					chThdSleepMilliseconds(1000);  // Ce sleep permet de laisser la la main thread de reprendre la main
			}

			if(image_blue_moy > image_red_moy){

					red_or_blue = false;
					chThdSleepMilliseconds(1000);  //Ce sleep permet de laisser la la main thread de reprendre la main
			}
    	}	
    	else{
    		chThdSleepMilliseconds(1000);
    	}
	}
}


void process_image_start(void){
	chThdCreateStatic(waProcessImage, sizeof(waProcessImage), NORMALPRIO, ProcessImage, NULL);
	chThdCreateStatic(waCaptureImage, sizeof(waCaptureImage), NORMALPRIO, CaptureImage, NULL);
}
