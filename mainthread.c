
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
#include <audio/audio_thread.h>
#include<audio/play_melody.h>

enum states{RESEARCH, COLOR, ACTION, SLEEP};

#define NB_MAX_OBSTACLES 4
#define PARAMETRES_MUSIC 0
#define STOP_MOVING 0
#define SIMPLE_INCREMENT 1
#define INIT 0

int state = RESEARCH;
static bool stranger;

int status(void){
	return state;
}


static THD_WORKING_AREA(waMainThread, 1024); //thread qui gère le fonctionnement général du programme
static THD_FUNCTION(MainThread, arg) {

	chRegSetThreadName(__FUNCTION__);
	(void)arg;
	stranger = false;
	uint8_t compteur_obstacles = INIT;

	while(1){
		
		chThdSleepMilliseconds(1000);
		
		switch(state){

		 	case RESEARCH:

		 		stranger = false;

		 		while(stranger == false){

					 stranger = detection_objet();
		 		}

		 		if(avancer_check()){ // s'approche de l'obstacle

		 			state = COLOR;
		 		}
		 		break;

		 	case COLOR: // regarde la couleur de l'obstacle

		 		chThdSleepMilliseconds(1000); //sleep laisse la place à la thread pour checker la couleur
		 		state = ACTION;
		 		break;

		 	case ACTION: //agit en fonction de la nature de l'obstacle

		 		attack_return();
		 		if(compteur_obstacles< NB_MAX_OBSTACLES){ //si on a déjà détecté NB_MAX_OBSTACLES obstacles, on passe au case sleep
		 			compteur_obstacles +=SIMPLE_INCREMENT;
		 			state = RESEARCH;
		 			break;

		 				 		}
		 		else state = SLEEP; //si on a déjà détecté NB_MAX_OBSTACLES obstacles, on sleep
		 		break;


		 	case SLEEP: //s'arrête de tourner et célèbre 

		 		right_motor_set_speed(STOP_MOVING); //arrête de tourner 
		 		left_motor_set_speed(STOP_MOVING);
		 		playMelody(WE_ARE_THE_CHAMPIONS, PARAMETRES_MUSIC, PARAMETRES_MUSIC); //joue de la musique
		 		waitMelodyHasFinished();
		 		stopCurrentMelody();
		 		break;
		}
	}
}


void mainthread_start(void){
	chThdCreateStatic(waMainThread, sizeof(waMainThread), NORMALPRIO, MainThread, NULL);
}
