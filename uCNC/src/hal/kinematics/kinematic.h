/*
	Name: kinematic.h
	Description: Declares the functions needed to implement the machine kinematics and homing motion.
		This defines an opac interface that allows to adapt µCNC to different architectures/mechanics of
		different machines.

	Copyright: Copyright (c) João Martins
	Author: João Martins
	Date: 26/09/2019

	µCNC is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version. Please see <http://www.gnu.org/licenses/>

	µCNC is distributed WITHOUT ANY WARRANTY;
	Also without the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
	See the	GNU General Public License for more details.
*/
#ifndef KINEMATIC_H
#define KINEMATIC_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

	void kinematics_init(void);

	/*
	Converts from machine absolute coordinates to step position.
	This is done after computing position relative to the active coordinate system
*/
	void kinematics_apply_inverse(float *axis, int32_t *steps);

	/*
	Converts from step position to machine absolute coordinates
	This is done after computing position relative to the active coordinate system
*/
	void kinematics_apply_forward(int32_t *steps, float *axis);

	/*
	Executes the homing motion for the machine
	The homing motion for each axis is defined in the motion control
	In the kinematics home function the axis order of the homing motion and other custom motions can be defined
*/
	uint8_t kinematics_home(void);

	/*
	Aplies a transformation to the position sent to planner.
	This is aplied only on normal and jog moves. Homing motions go directly to planner.
*/
	void kinematics_apply_transform(float *axis);

	/*
	Aplies a reverse transformation to the position returned from the planner.
	This is aplied only on normal and jog moves. Homing motions go directly to planner.
*/
	void kinematics_apply_reverse_transform(float *axis);

#ifdef __cplusplus
}
#endif

#endif
