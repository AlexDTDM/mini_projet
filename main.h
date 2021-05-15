#ifndef MAIN_H
#define MAIN_H

#ifdef __cplusplus
extern "C"{
	#endif

	#include "camera/dcmi_camera.h"
	#include "msgbus/messagebus.h"
	#include "parameter/parameter.h"

	int status(void);

	#define IMAGE_BUFFER_SIZE		640
	#define WIDTH_SLOPE				5
	#define MIN_LINE_WIDTH			40
	#define PXTOCM					1570.0f //experimental value
	#define GOAL_DISTANCE 			10.0f
	#define MAX_DISTANCE 			25.0f

	/** Robot wide IPC bus. */
	extern messagebus_t bus; //UILE ??

	extern parameter_namespace_t parameter_root; // UTILE ??

	void SendUint8ToComputer(uint8_t* data, uint16_t size); // UTILE ??

	#ifdef __cplusplus
}
#endif

#endif
