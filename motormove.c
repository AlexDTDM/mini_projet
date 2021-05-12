/*
 * motormove.c
 *
 *  Created on: 22 avr. 2021
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
#include <mainthread.h>
#include <tof.h>

#define NSTEP_ONE_TURN 1000 //number of step for 1 turn of the motor
#define WHEEL_PERIMETER 130 //mm
#define OUTSIDE_ARENE_POS 50 //mm
#define RAYON_ARENE 400 //mm
#define THRESHOLD 100 //mm

void attack_return(void) {

	    bool attack_or_return;

    	attack_or_return = red_blue();

    	static int16_t position_to_reach_insteps = 0; //in steps
    	float position_to_reach;


    	//init counter steps at 0
    	left_motor_set_pos(0);
    	right_motor_set_pos(0);

    	int32_t current_pos = 0;
    	uint16_t distance_objet = 0;

    	distance_objet = getdistance_mm();
    	chprintf((BaseSequentialStream *)&SDU1, "[distance_objet = %d]", distance_objet);




    	if (attack_or_return == true) { //attack mode

    		position_to_reach = RAYON_ARENE - distance_objet + THRESHOLD + OUTSIDE_ARENE_POS;

    		position_to_reach_insteps = position_to_reach * NSTEP_ONE_TURN / WHEEL_PERIMETER; //in steps
    		//chprintf((BaseSequentialStream *)&SDU1, "[position_to_reach_insteps = %d]", position_to_reach_insteps);





			while (current_pos < position_to_reach_insteps) {

				right_motor_set_speed(1000);
			    left_motor_set_speed(1000);

			    //for(uint32_t i = 0 ; i < 21000000 ; i++){
			    				//__asm__ volatile ("nop");
			    								//}

			    current_pos = right_motor_get_pos();
			    //chprintf((BaseSequentialStream *)&SDU1, "[current_pos = %d]", current_pos);





			}
			right_motor_set_speed(0);
			left_motor_set_speed(0);

			//parti reviens sur place

			left_motor_set_pos(0);
			right_motor_set_pos(0);

			position_to_reach = RAYON_ARENE + OUTSIDE_ARENE_POS;
			position_to_reach_insteps = - position_to_reach * NSTEP_ONE_TURN / WHEEL_PERIMETER;

			while (current_pos > position_to_reach_insteps) {

				right_motor_set_speed(-1000);
			    left_motor_set_speed(-1000);

			    current_pos = right_motor_get_pos(); //je suppose que le signe est toujours positif de right_motor_get_pos qu'importe le sens du robot
/*
			    for(uint32_t i = 0 ; i < 21000000 ; i++){
			    			__asm__ volatile ("nop");
			    			}
			  chprintf((BaseSequentialStream *)&SDU1, "[current_posBACKRED = %d]", current_pos);
*/

			}

			left_motor_set_speed(0);
		    right_motor_set_speed(0);


		    for(uint32_t i = 0 ; i < 5000000 ; i++){
		    	             __asm__ volatile ("nop");
		    				}


			left_motor_set_speed(500);
			right_motor_set_speed(-5000);

			for(uint32_t i = 0 ; i < 5000000 ; i++){
			                __asm__ volatile ("nop");
						    }
			left_motor_set_speed(0);
			right_motor_set_speed(0);


    	}


       	if (attack_or_return == false) {


			position_to_reach = distance_objet - THRESHOLD;
			position_to_reach_insteps = - position_to_reach * NSTEP_ONE_TURN / WHEEL_PERIMETER;

			while (current_pos > position_to_reach_insteps) {

				right_motor_set_speed(-1000);
			    left_motor_set_speed(-1000);

			    current_pos = right_motor_get_pos(); //je suppose que le signe est toujours positif de right_motor_get_pos qu'importe le sens du robot


/*
			    for(uint32_t i = 0 ; i < 21000000 ; i++){
			    	        __asm__ volatile ("nop");
			    			}
			    chprintf((BaseSequentialStream *)&SDU1, "[current_posBACKBLUE = %d]", current_pos);
*/

			}

			left_motor_set_speed(0);
			right_motor_set_speed(0);


			for(uint32_t i = 0 ; i < 5000000 ; i++){
					    	 __asm__ volatile ("nop");
					    }


			left_motor_set_speed(500);
			right_motor_set_speed(-5000);

			for(uint32_t i = 0 ; i < 5000000 ; i++){
						 __asm__ volatile ("nop");
						}
			left_motor_set_speed(0);
			right_motor_set_speed(0);

    	}

}




