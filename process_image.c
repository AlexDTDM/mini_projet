#include "ch.h"
#include "hal.h"
#include <chprintf.h>
#include <usbcfg.h>
//#include <inttypes.h>
//#include "VL53L0X.h"
//#include "vl53l0x_api_calibration.h"
#include "sensors/VL53L0X/VL53L0X.h"

#include <main.h>
#include <camera/po8030.h>

#include <process_image.h>


#define EPSILON 10
#define THRESHOLD 10
static float distance_cm = 0;
static uint16_t line_position = IMAGE_BUFFER_SIZE/2;	//middle

//semaphore
static BSEMAPHORE_DECL(image_ready_sem, TRUE);

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

static THD_WORKING_AREA(waCaptureImage, 256);
static THD_FUNCTION(CaptureImage, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;

	//Takes pixels 0 to IMAGE_BUFFER_SIZE of the line 10 + 11 (minimum 2 lines because reasons)
	po8030_advanced_config(FORMAT_RGB565, 0, 10, IMAGE_BUFFER_SIZE, 2, SUBSAMPLING_X1, SUBSAMPLING_X1);
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



	uint16_t lineWidth = 0;
	uint16_t center_line = IMAGE_BUFFER_SIZE/2;

    //palClearPad(GPIOD, GPIOD_LED_FRONT); //if red
	//palClearPad(GPIOB, GPIOB_LED_BODY); //if blue
	//palClearPad(GPIOD, GPIOD_LED7); // if stranger


    //bool friend_or_enemi = true; // true = friends //bleu = ami, rouge = emenie.
    //bool total_stranger = false;


	//bool send_to_computer = true;

    while(1){

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

		//ces 2 valeurs sont bcp trop proches


		//chprintf((BaseSequentialStream *)&SDU1, "[RED_MOY = %d]", image_red_moy);




		//chprintf((BaseSequentialStream *)&SDU1, "[BLUE_MOY = %d]", image_blue_moy);


		if (image_red_moy > image_blue_moy) {

			lineWidth = extract_line_width(image_blue);
			if (lineWidth > 0) {

				chprintf((BaseSequentialStream *)&SDU1, "[RED]");
				right_motor_set_speed(1000);
			    left_motor_set_speed(1000);



			}
		}

		if (image_blue_moy > image_red_moy) {

			lineWidth = extract_line_width(image);
			if (lineWidth > 0) {

				chprintf((BaseSequentialStream *)&SDU1, "[BLUE]");
				right_motor_set_speed(0);
				left_motor_set_speed(0);
			}
		}

		//else {

			//chprintf((BaseSequentialStream *)&SDU1, "NOLINE");
		//}


		//lineWidth = extract_line_width(image);

		//chprintf((BaseSequentialStream *)&SDU1, "[RED_MOY = %d]", image_red_moy);

		//for(uint32_t i = 0 ; i < 21000000 ; i++){
								// __asm__ volatile ("nop");
						//}


		//center_line = line_position;


		//chprintf((BaseSequentialStream *)&SDU1, "[center_line = %d]", center_line);

		//if(lineWidth > 0 && center_line > 0) {

			//chprintf((BaseSequentialStream *)&SDU1, "[BLUE = %d]", image_red_moy);



		//}

		//else {

			//chprintf((BaseSequentialStream *)&SDU1, "NOP" );

		//}

		//chprintf((BaseSequentialStream *)&SDU1, "[CENTER_LINE = %d]", center_line);

		//chprintf((BaseSequentialStream *)&SDU1, "[lineWidth = %d]", lineWidth);

		//image_red = ((uint8_t)img_buff_ptr[i])&0xF8;
		//image_blue = (uint8_t)img_buff_ptr[i+1]&0X1F


		//if(send_to_computer){
			    			//sends to the computer the image
			    	//	SendUint8ToComputer(image, IMAGE_BUFFER_SIZE);
			    		//}

			 //  send_to_computer = !send_to_computer;


		//if(lineWidth) {

/*

			for(uint16_t i = 0 ; i < (4*EPSILON) ; i+=2){  //for(uint16_t i = 2*(center_line - EPSILON) ; i < (2*(center_line + EPSILON)) ; i+=2){

				image_red[i/2] =  (((uint8_t)img_buff_ptr[2*(center_line - EPSILON) + i])>>3)&0x1F; //normalement :image_red[i/2] =  ((uint8_t)img_buff_ptr[i]>>3)&0x1F;

				image_blue[i/2] = (uint8_t)img_buff_ptr[2*(center_line - EPSILON) + i + 1]&0X1F;  //normalement :image_blue[i/2] =  ((uint8_t)img_buff_ptr[i+1)&0x1F;

				image_blue_moy = image_blue_moy + image_blue[i/2];
				image_red_moy = image_red_moy + image_red[i/2];

			}

			image_red_moy = image_red_moy/(2*EPSILON);
			image_blue_moy = image_blue_moy/(2*EPSILON);



			if ( image_red_moy < image_moy ) { //blue repere


				chprintf((BaseSequentialStream *)&SDU1, "[BLUE = %d]", image_blue_moy);

			}

			if ( image_blue_moy < image_moy ) { //red repere

				chprintf((BaseSequentialStream *)&SDU1, "[RED = %d]", image_blue_moy);
			}

			else {
				chprintf((BaseSequentialStream *)&SDU1, "[NOP]");

			}



			//for(uint32_t i = 0 ; i < 21000000 ; i++){
									// __asm__ volatile ("nop");
							//}

			//chprintf((BaseSequentialStream *)&SDU1, "[RED = %d]", image_red_moy);

			//for(uint32_t i = 0 ; i < 21000000 ; i++){
									// __asm__ volatile ("nop");
							//}


			//chprintf((BaseSequentialStream *)&SDU1, "[BLUE = %d]", image_blue_moy);


		//}



*/


/*


			if (image_red_moy > image_blue_moy) {



				chprintf((BaseSequentialStream *)&SDU1, "[RED = %d]", image_red_moy);

				//right_motor_set_speed(1000);
				//left_motor_set_speed(1000);

				for(uint32_t i = 0 ; i < 21000000 ; i++){
						 __asm__ volatile ("nop");
				}

				}

			if(image_blue_moy > image_red_moy ) {

				chprintf((BaseSequentialStream *)&SDU1, "[BLUE = %d]", image_blue_moy);


				//right_motor_set_speed(0);
				//left_motor_set_speed(0);

				for(uint32_t i = 0 ; i < 21000000 ; i++){
							__asm__ volatile ("nop");
				}

			}

			else {

				//right_motor_set_speed(0);
			    //left_motor_set_speed(0);

				//total_stranger = true;
				chprintf((BaseSequentialStream *)&SDU1, "NOP");

			    for(uint32_t i = 0 ; i < 21000000 ; i++){
						    __asm__ volatile ("nop");
							}

}



*/

/*
commentaire: peut etre mon erreur est que je compare un registre pour 1 pixel et pas plusieurs !
			//center_line = get_line_position();

			image_red = (((uint8_t)img_buff_ptr[2*center_line])>>3)&0x1F;
			image_blue = ((uint8_t)img_buff_ptr[2*center_line+1])&0X1F;

			//chprintf((BaseSequentialStream *)&SDU1, "[RED = %d]", image_red);

			//chprintf((BaseSequentialStream *)&SDU1, "[BLUE = %d]", image_blue);







		if (image_red - image_blue > EPSILON/2) {

			chprintf((BaseSequentialStream *)&SDU1, "[RED = %d]", image_red);

			right_motor_set_speed(1000);
			left_motor_set_speed(1000);

			for(uint32_t i = 0 ; i < 21000000 ; i++){
					 __asm__ volatile ("nop");
			}


			//friend_or_enemi = false;
			//lineWidth = extract_line_width(image_red);
			//chprintf((BaseSequentialStream *)&SDU1, "RED");
			//palTogglePad(GPIOD, GPIOD_LED_FRONT);

			}

		if(image_blue - image_red > EPSILON) {

			chprintf((BaseSequentialStream *)&SDU1, "[BLUE = %d]", image_blue);


			right_motor_set_speed(0);
			left_motor_set_speed(0);

			for(uint32_t i = 0 ; i < 21000000 ; i++){
						__asm__ volatile ("nop");
			}
			//friend_or_enemi = true;
		    //lineWidth = extract_line_width(image_blue);
			//palTogglePad(GPIOB, GPIOB_LED_BODY); //if blue
		    //chprintf((BaseSequentialStream *)&SDU1, "BLUE");


		}

		else {

			right_motor_set_speed(0);
		    left_motor_set_speed(0);

			//total_stranger = true;
			chprintf((BaseSequentialStream *)&SDU1, "NOP");

			//for(uint32_t i = 0 ; i < 21000000 ; i++){
					     //__asm__ volatile ("nop");
						//}

		}

		//for(uint32_t i = 0 ; i < 21000000 ; i++){
								     // __asm__ volatile ("nop");
								//}
*/



  }

}

void process_image_start(void){
	chThdCreateStatic(waProcessImage, sizeof(waProcessImage), NORMALPRIO, ProcessImage, NULL);
	chThdCreateStatic(waCaptureImage, sizeof(waCaptureImage), NORMALPRIO, CaptureImage, NULL);
}


/*
 --- ET SI ON ESSAYAIT LES THREADS ? ---
static THD_WORKING_AREA(waCaptureImage, 256);
static THD_FUNCTION(TOF, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;

    //chprintf((BaseSequentialStream *)&SDU1, " entrée dans TOF");
    	uint8_t variable_ret ;
    	variable_ret = VL53L0X_init(dev_chosen); // Init the VL53L0X_Dev_t structure and the sensor.
    	uint16_t measured_distance ;
    	//variable_ret = VL53L0X_configAccuracy(dev_chosen, VL53L0X_DEFAULT_MODE); // Configure the accuracy of the sensor (range).
    	//variable_ret = VL53L0X_startMeasure(dev_chosen, VL53L0X_DEVICEMODE_SINGLE_RANGING); // Begin the meausurement process with the specified mode
    	//variable_ret = VL53L0X_getLastMeasure(dev_chosen); //  Get the last valid measure and lpace it in the sensor structure given.
    	//VL53L0X_start(); // Init a thread which uses the distance sensor to continuoulsy measure the distance.
    	//VL53L0X_stop(); // Stop the distance measurement
    	//measured_distance = VL53L0X_get_dist_mm(); // Last distance measured in mm
    	//chprintf((BaseSequentialStream *)&SDU1, "la distance mesurée est :", measured_distance);


}
*/

/*
 * définitions des types : (définis dans vl53l0_def.h)
 *
 * VL53L0X_Error :  unsigned int 8 bits
 * typedef uint8_t VL53L0X_AccuracyMode ; // 1.2m, 30ms
#define VL53L0X_DEFAULT_MODE    ((VL53L0X_AccuracyMode) 0) // 1.2m, 200ms
#define VL53L0X_HIGH_ACCURACY   ((VL53L0X_AccuracyMode) 1) // 2m, 33ms
#define VL53L0X_LONG_RANGE      ((VL53L0X_AccuracyMode) 2) // 1.2m, 20ms
#define VL53L0X_HIGH_SPEED      ((VL53L0X_AccuracyMode) 3)
 *
 * typedef uint8_t VL53L0X_DeviceModes;

#define VL53L0X_DEVICEMODE_SINGLE_RANGING	((VL53L0X_DeviceModes)  0)
#define VL53L0X_DEVICEMODE_CONTINUOUS_RANGING	((VL53L0X_DeviceModes)  1)
#define VL53L0X_DEVICEMODE_SINGLE_HISTOGRAM	((VL53L0X_DeviceModes)  2)
#define VL53L0X_DEVICEMODE_CONTINUOUS_TIMED_RANGING ((VL53L0X_DeviceModes) 3)
#define VL53L0X_DEVICEMODE_SINGLE_ALS		((VL53L0X_DeviceModes) 10)
#define VL53L0X_DEVICEMODE_GPIO_DRIVE		((VL53L0X_DeviceModes) 20)
#define VL53L0X_DEVICEMODE_GPIO_OSC		((VL53L0X_DeviceModes) 21)
*/

void test_TOF(void)
{
	VL53L0X_start(); // // Init a thread which uses the distance sensor to continuoulsy measure the distance.
	//chprintf((BaseSequentialStream *)&SDU1, " entrée dans TOF");
	//uint8_t variable_ret ;
	//variable_ret = VL53L0X_init(dev_chosen); // Init the VL53L0X_Dev_t structure and the sensor.
	uint16_t measured_distance ;
	chThdSleepMilliseconds(1000);
	measured_distance = VL53L0X_get_dist_mm(); // Last distance measured in mm
	chprintf((BaseSequentialStream * ) & SDU1, "mesure = %d \n" , measured_distance);

	//variable_ret = VL53L0X_configAccuracy(dev_chosen, VL53L0X_DEFAULT_MODE); // Configure the accuracy of the sensor (range).
	//variable_ret = VL53L0X_startMeasure(dev_chosen, VL53L0X_DEVICEMODE_SINGLE_RANGING); // Begin the meausurement process with the specified mode
	//variable_ret = VL53L0X_getLastMeasure(dev_chosen); //  Get the last valid measure and lpace it in the sensor structure given.

	//VL53L0X_stop(); // Stop the distance measurement
}

