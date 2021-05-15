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
#define OUTSIDE_VILLAGE_POS 40 //mm
#define RAYON_VILLAGE 230 //mm
#define THRESHOLD 120 //mm
#define INIT 0
#define STOP_MOVING 0
#define FAST_SPEED 1000
#define SLOW_SPEED 500
#define STOP_TIME 5000000

void attack_return(void){

	bool attack_or_return;
    attack_or_return = red_blue();
    static int16_t position_to_reach_insteps = INIT; //in steps
    float position_to_reach; 

    //init counter steps at 0
	left_motor_set_pos(INIT);
	right_motor_set_pos(INIT);
	int16_t current_pos = INIT;
	uint16_t distance_objet = INIT;

	distance_objet = getdistance_mm();
 
    if(attack_or_return == true){ //attack mode

    	position_to_reach = RAYON_VILLAGE - distance_objet + THRESHOLD + OUTSIDE_VILLAGE_POS; // in mm
    	position_to_reach_insteps = position_to_reach * NSTEP_ONE_TURN / WHEEL_PERIMETER; //in steps

		while(current_pos < position_to_reach_insteps){

			right_motor_set_speed(FAST_SPEED);
		    left_motor_set_speed(FAST_SPEED);
		    current_pos = right_motor_get_pos();
		}

		right_motor_set_speed(STOP_MOVING); // robot parti revient à sa place
		left_motor_set_speed(STOP_MOVING);
		left_motor_set_pos(INIT);
		right_motor_set_pos(INIT);

		position_to_reach = RAYON_VILLAGE + OUTSIDE_VILLAGE_POS;
		position_to_reach_insteps = - position_to_reach * NSTEP_ONE_TURN / WHEEL_PERIMETER;

		while (current_pos > position_to_reach_insteps){

			right_motor_set_speed(-FAST_SPEED);
		    left_motor_set_speed(-FAST_SPEED);
		    current_pos = right_motor_get_pos(); //NB : right_motor_get_pos>0 qu'importe le sens du robot
		}

		left_motor_set_speed(STOP_MOVING);
	    right_motor_set_speed(STOP_MOVING);

		for(uint32_t i = 0 ; i < STOP_TIME ; i++){ __asm__ volatile ("nop");}

		left_motor_set_speed(SLOW_SPEED);
		right_motor_set_speed(-SLOW_SPEED);

		for(uint32_t i = 0 ; i < STOP_TIME ; i++){ __asm__ volatile ("nop");}
		
		left_motor_set_speed(STOP_MOVING);
		right_motor_set_speed(STOP_MOVING);
    }

    if(attack_or_return == false){ // return mode

		position_to_reach = distance_objet - THRESHOLD;
		position_to_reach_insteps = - position_to_reach * NSTEP_ONE_TURN / WHEEL_PERIMETER;

		while (current_pos > position_to_reach_insteps) {

			right_motor_set_speed(-FAST_SPEED);
			left_motor_set_speed(-FAST_SPEED);
			current_pos = right_motor_get_pos();

		}

		left_motor_set_speed(STOP_MOVING);
		right_motor_set_speed(STOP_MOVING);

		for(uint32_t i = 0 ; i < STOP_TIME ; i++){ __asm__ volatile ("nop");}

		left_motor_set_speed(SLOW_SPEED);
		right_motor_set_speed(-SLOW_SPEED);

		for(uint32_t i = 0 ; i < STOP_TIME ; i++){ __asm__ volatile ("nop");}

		left_motor_set_speed(STOP_MOVING);
		right_motor_set_speed(STOP_MOVING);
    }
}




