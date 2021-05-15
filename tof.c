/*
 * TOF.c
 *
 *  Created on: 23 avr. 2021
 *      Author: alexandredemontleau
 */

#include "ch.h"
#include "hal.h"
#include "sensors/VL53L0X/VL53L0X.h"
#include <math.h>
#include <usbcfg.h>
#include <chprintf.h>
#include <main.h>
#include <motors.h>
#include <process_image.h>
#include <mainthread.h>
#include <tof.h>

#define RAYON_VILLAGE 230				// in mm
#define DIST_MIN_DEPLACEMENT 130 		// in mm
#define WHEEL_PERIMETER_MM 130			// in mm
#define NSTEP_ONE_TURN 1000				// number of step for 1 turn of the motor
#define EPSILON 0.001
#define THRESHOLD 120 					//distance à laquelle le robot s arrete pour checker sa couleur
#define STOP_MOVING 0
#define VITESSE_ROTATION 300
#define VITESSE_CHECK 1000
#define INIT 0
#define MEASURE_TAKEN 3000


static uint16_t distance_mm;
static uint16_t distance_objet;

uint16_t getdistance_mm (void){
	return distance_objet;
}


bool avancer_check(void){

	bool passage_case_suivant = false;
	uint16_t position_to_reach_insteps; // sur 16 bits
	int16_t current_pos = INIT;

	//init counter steps at 0
	left_motor_set_pos(INIT);
	right_motor_set_pos(INIT);

	//distance_steps = distance_mm*NSTEP_ONE_TURN/WHEEL_PERIMETER_MM;
	position_to_reach_insteps = (distance_mm - THRESHOLD)*NSTEP_ONE_TURN/WHEEL_PERIMETER_MM; // conversion distmm -> diststeps
    distance_objet = distance_mm;

	while(current_pos < position_to_reach_insteps){

		right_motor_set_speed(VITESSE_CHECK);
	    left_motor_set_speed(VITESSE_CHECK);
	    current_pos = right_motor_get_pos();
	}

	right_motor_set_speed(STOP_MOVING);
	left_motor_set_speed(STOP_MOVING);
	passage_case_suivant = true ;
	distance_objet = distance_mm;

	// réinitialisation de la distance mesurée pour les appels suivants de detection_objet
	distance_mm = INIT;
	return passage_case_suivant;
}


bool detection_objet(void){

	bool obstacle_detected = false;
	distance_mm = MEASURE_TAKEN; //distance jamais atteinte donc condition valable pour entrer dans le while

	while(distance_mm > RAYON_VILLAGE || distance_mm < DIST_MIN_DEPLACEMENT){

		//robot tourne sur lui-même
		left_motor_set_speed(VITESSE_ROTATION); 
		right_motor_set_speed(-VITESSE_ROTATION);
		distance_mm = appel_TOF(); // appel de la fonction qui mesure les distances
	}

	obstacle_detected = true;
	return obstacle_detected;
}


uint16_t appel_TOF(void){

	uint16_t distance_objet_detected;
	distance_objet_detected = VL53L0X_get_dist_mm(); // Last distance measured in mm
	return distance_objet_detected;
}
