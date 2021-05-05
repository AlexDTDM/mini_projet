#include "ch.h"
#include "hal.h"
#include <chprintf.h>
#include <usbcfg.h>

#include <main.h>
#include <camera/po8030.h>

#include <process_image.h>
#include <motors.h>
#include <mainthread.h>

#define EPSILON 10
#define THRESHOLD 10

//#define CASE_SCOOT 0
//#define CASE_ADVANCE 1
static float distance_cm = 0;
static uint16_t line_position = IMAGE_BUFFER_SIZE/2;	//middle
bool red_or_blue = true; //int 0 pas pret 1 pret 2 pret
//semaphore
static BSEMAPHORE_DECL(image_ready_sem, TRUE);

//static BSEMAPHORE_DECL(color_found_sem, TRUE);

/*
 *  Returns the line's width extracted from the image buffer given
 *  Returns 0 if line not found
 */
uint16_t extract_line_width(uint8_t *buffer){

	uint16_t i = 0, begin = 0, end = 0, width = 0;
	uint8_t stop = 0, wrong_line = 0, line_not_found = 0;
	uint32_t mean = 0;
	static uint16_t last_width = PXTOCM/GOAL_DISTANCE;

	//performs an average
	for(uint16_t i = 0 ; i < IMAGE_BUFFER_SIZE ; i++){
		mean += buffer[i];
	}
	mean /= IMAGE_BUFFER_SIZE;

	do{
		wrong_line = 0;
		//search for a begin
		while(stop == 0 && i < (IMAGE_BUFFER_SIZE - WIDTH_SLOPE))
		{
			//the slope must at least be WIDTH_SLOPE wide and is compared
		    //to the mean of the image
		    if(buffer[i] > mean && buffer[i+WIDTH_SLOPE] < mean)
		    {
		        begin = i;
		        stop = 1;
		    }
		    i++;
		}
		//if a begin was found, search for an end
		if (i < (IMAGE_BUFFER_SIZE - WIDTH_SLOPE) && begin)
		{
		    stop = 0;

		    while(stop == 0 && i < IMAGE_BUFFER_SIZE)
		    {
		        if(buffer[i] > mean && buffer[i-WIDTH_SLOPE] < mean)
		        {
		            end = i;
		            stop = 1;
		        }
		        i++;
		    }
		    //if an end was not found
		    if (i > IMAGE_BUFFER_SIZE || !end)
		    {
		        line_not_found = 1;
		    }
		}
		else//if no begin was found
		{
		    line_not_found = 1;
		}

		//if a line too small has been detected, continues the search
		if(!line_not_found && (end-begin) < MIN_LINE_WIDTH){
			i = end;
			begin = 0;
			end = 0;
			stop = 0;
			wrong_line = 1;
		}
	}while(wrong_line);

	if(line_not_found){
		begin = 0;
		end = 0;
		width = last_width;
	}else{
		last_width = width = (end - begin);
		line_position = (begin + end)/2; //gives the line position.
	}

	//sets a maximum width or returns the measured width
	if((PXTOCM/width) > MAX_DISTANCE){
		return PXTOCM/MAX_DISTANCE;
	}else{
		return width;
	}
}


float get_distance_cm(void){
	return distance_cm;
}


uint16_t get_line_position(void){
	return line_position;
}


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



static THD_WORKING_AREA(waProcessImage, 1536);  //1024
static THD_FUNCTION(ProcessImage, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;

	uint8_t *img_buff_ptr;
	uint8_t image[IMAGE_BUFFER_SIZE] = {0};
    uint8_t image_blue[IMAGE_BUFFER_SIZE] = {0};
	//uint8_t image_red = 0;
	//uint8_t image_blue = 0;
	//uint8_t image_red[2*EPSILON] = {0};
	//uint8_t image_blue[2*EPSILON] = {0};

	//uint8_t image_blue_moy = 0;
	//uint8_t image_red_moy = 0;
	uint32_t image_blue_moy = 0;
	uint32_t image_red_moy = 0;
	int state = 0;



	uint16_t lineWidth = 0;
	//uint16_t center_line = IMAGE_BUFFER_SIZE/2;

    //palClearPad(GPIOD, GPIOD_LED_FRONT); //if red
	//palClearPad(GPIOB, GPIOB_LED_BODY); //if blue
	//palClearPad(GPIOD, GPIOD_LED7); // if stranger


    //bool friend_or_enemi = true; // true = friends //bleu = ami, rouge = emenie.
    //bool total_stranger = false;


	//bool send_to_computer = true;

    while(1){

    	state = status();

    	if (state == COLOR) {

    	//waits until an image has been captured
        chBSemWait(&image_ready_sem);

		//gets the pointer to the array filled with the last image in RGB565
		img_buff_ptr = dcmi_get_last_image_ptr();


		for(uint16_t i = 0 ; i < (2 * IMAGE_BUFFER_SIZE) ; i+=2){
			//extracts first 5bits of the first byte
			//takes nothing from the second byte

			image[i/2] = (((uint8_t)img_buff_ptr[i])>>3)&0x1F;  //va reperer bleu jsp pourquoi
            //image_red_moy = image_red_moy + image[i/2];


            image_blue[i/2] = ((uint8_t)img_buff_ptr[i+1])&0x1F;
            //image_blue_moy = image_blue_moy + image_blue[i/2];





			//image[i/2] = ((uint8_t)img_buff_ptr[i+1])&0x1F; //va reperer le rouge trop bizarre

			//image_red_moy = image_red_moy + image[i/2];

			}


		for(uint16_t i = 2*(IMAGE_BUFFER_SIZE/2 - THRESHOLD) ; i < (2 * (IMAGE_BUFFER_SIZE/2 + THRESHOLD)) ; i+=2){

			image_blue_moy = image_blue_moy + image_blue[i/2];
			image_red_moy = image_red_moy + image[i/2];
		}


		image_red_moy = image_red_moy/(2*THRESHOLD);
		image_blue_moy = image_blue_moy/(2*THRESHOLD);

		//chprintf((BaseSequentialStream *)&SDU1, "[REDl = %d]", image_red_moy);
		//chprintf((BaseSequentialStream *)&SDU1, "[BLUEl = %d]", image_blue_moy);






		if (image_red_moy > image_blue_moy) {

			lineWidth = extract_line_width(image_blue);
			if (lineWidth > 0) {

				//chprintf((BaseSequentialStream *)&SDU1, "[RED]");
				//right_motor_set_speed(1000);
			    //left_motor_set_speed(1000);
				red_or_blue = true;
				//chBSemWait(&image_ready_sem);
				//chprintf((BaseSequentialStream *)&SDU1, "[REDl = %d]", red_or_blue);
				chThdSleepMilliseconds(1000);

				//for(uint32_t i = 0 ; i < 21000000 ; i++){
										// __asm__ volatile ("nop");
								//}

			}
		}

		if (image_blue_moy > image_red_moy) {

			lineWidth = extract_line_width(image);
			if (lineWidth > 0) {

				//chprintf((BaseSequentialStream *)&SDU1, "[BLUE]");
				//right_motor_set_speed(0);
				//left_motor_set_speed(0);
				red_or_blue = false;
				//chprintf((BaseSequentialStream *)&SDU1, "[bool = %d]", red_or_blue);
				//chBSemWait(&image_ready_sem);
				chThdSleepMilliseconds(1000);

				//for(uint32_t i = 0 ; i < 21000000 ; i++){
										//__asm__ volatile ("nop");
								//}
			}
		}



    }

    	else {
    		chThdSleepMilliseconds(1000);
    	}

  }


}



void process_image_start(void){
	chThdCreateStatic(waProcessImage, sizeof(waProcessImage), NORMALPRIO, ProcessImage, NULL);
	chThdCreateStatic(waCaptureImage, sizeof(waCaptureImage), NORMALPRIO, CaptureImage, NULL);
}
