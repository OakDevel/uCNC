/*
        Name: kinematic_delta.c
        Description: Implements all kinematics math equations to translate the motion of a delta machine.
                Also implements the homing motion for this type of machine.

        Copyright: Copyright (c) João Martins
        Author: João Martins
        Date: 19/01/2022

        µCNC is free software: you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation, either version 3 of the License, or
        (at your option) any later version. Please see <http://www.gnu.org/licenses/>

        µCNC is distributed WITHOUT ANY WARRANTY;
        Also without the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
        See the	GNU General Public License for more details.
*/

#include "../../cnc.h"

#if (KINEMATIC == KINEMATIC_DELTA)
#include <stdio.h>
#include <math.h>

#define STEPPER0_FACTX (cos(STEPPER0_ANGLE * M_PI / 180.0f));
#define STEPPER0_FACTY (sin(STEPPER0_ANGLE * M_PI / 180.0f));
#define STEPPER1_FACTX (cos(STEPPER1_ANGLE * M_PI / 180.0f));
#define STEPPER1_FACTY (sin(STEPPER1_ANGLE * M_PI / 180.0f));
#define STEPPER2_FACTX (cos(STEPPER2_ANGLE * M_PI / 180.0f));
#define STEPPER2_FACTY (sin(STEPPER2_ANGLE * M_PI / 180.0f));

static float delta_arm_sqr;
static float delta_base_height;
static float delta_x[3];
static float delta_y[3];

void kinematics_init(void)
{
        float delta_triang_base = (g_settings.delta_armbase_radius /* - g_settings.delta_efector_radius*/);
        delta_arm_sqr = g_settings.delta_arm_length * g_settings.delta_arm_length;
        delta_x[0] = delta_triang_base * STEPPER0_FACTX;
        delta_x[1] = delta_triang_base * STEPPER1_FACTX;
        delta_x[2] = delta_triang_base * STEPPER2_FACTX;
        delta_y[0] = delta_triang_base * STEPPER0_FACTY;
        delta_y[1] = delta_triang_base * STEPPER1_FACTY;
        delta_y[2] = delta_triang_base * STEPPER2_FACTY;
        delta_base_height = sqrtf(delta_arm_sqr - delta_x[0] * delta_x[0] - delta_y[0] * delta_y[0]);
}

void kinematics_apply_inverse(float *axis, int32_t *steps)
{
        float x = delta_x[0] - axis[AXIS_X];
        float y = delta_y[0] - axis[AXIS_Y];
        float z = axis[AXIS_Z] - delta_base_height;
        float steps_mm = sqrt(delta_arm_sqr - (x * x) - (y * y)) + z;
        steps[0] = (int32_t)lroundf(g_settings.step_per_mm[0] * steps_mm);
        x = delta_x[1] - axis[AXIS_X];
        y = delta_y[1] - axis[AXIS_Y];
        steps_mm = sqrt(delta_arm_sqr - (x * x) - (y * y)) + z;
        steps[1] = (int32_t)lroundf(g_settings.step_per_mm[1] * steps_mm);
        x = delta_x[2] - axis[AXIS_X];
        y = delta_y[2] - axis[AXIS_Y];
        steps_mm = sqrt(delta_arm_sqr - (x * x) - (y * y)) + z;
        steps[2] = (int32_t)lroundf(g_settings.step_per_mm[2] * steps_mm);
}

void kinematics_apply_forward(int32_t *steps, float *axis)
{
        // using trialteration (similar to marlin)
        float z0 = (steps[0] / g_settings.step_per_mm[0]);
        float z1 = (steps[1] / g_settings.step_per_mm[1]);
        float z2 = (steps[2] / g_settings.step_per_mm[2]);
        float p01[3] = {delta_x[1] - delta_x[0], delta_y[1] - delta_y[0], z1 - z0};

        // Get the reciprocal of Magnitude of vector.
        float d2 = (p01[0] * p01[0]) + (p01[1] * p01[1]) + (p01[2] * p01[2]);
        float inv_d = 1.0f / sqrtf(d2);

        // Create unit vector by multiplying by the inverse of the magnitude.
        float ex[3] = {p01[0] * inv_d, p01[1] * inv_d, p01[2] * inv_d};

        // Get the vector from the origin of the new system to the third point.
        float p02[3] = {delta_x[2] - delta_x[0], delta_y[2] - delta_y[0], z2 - z0};

        // Use the dot product to find the component of this vector on the X axis.
        float i = ex[0] * p02[0] + ex[1] * p02[1] + ex[2] * p02[2];

        // Create a vector along the x axis that represents the x component of p02.
        float iex[3] = {ex[0] * i, ex[1] * i, ex[2] * i};

        // Subtract the X component from the original vector leaving only Y. We use the
        // variable that will be the unit vector after we scale it.
        float ey[3] = {p02[0] - iex[0], p02[1] - iex[1], p02[2] - iex[2]};

        // The magnitude and the inverse of the magnitude of Y component
        float j2 = (ey[0] * ey[0]) + (ey[1] * ey[1]) + (ey[2] * ey[2]);
        float inv_j = 1.0f / sqrtf(j2);

        // Convert to a unit vector
        ey[0] *= inv_j;
        ey[1] *= inv_j;
        ey[2] *= inv_j;

        // The cross product of the unit x and y is the unit z
        // float[] ez = vectorCrossProd(ex, ey);
        float ez[3] = {
            ex[1] * ey[2] - ex[2] * ey[1],
            ex[2] * ey[0] - ex[0] * ey[2],
            ex[0] * ey[1] - ex[1] * ey[0]};

        // We now have the d, i and j values defined in Wikipedia.
        // Plug them into the equations defined in Wikipedia for Xnew, Ynew and Znew
        float Xnew = d2 * inv_d * 0.5;
        float Ynew = ((i * i + j2) * 0.5 - i * Xnew) * inv_j;
        float Znew = sqrtf(delta_arm_sqr - (Xnew * Xnew + Ynew * Ynew));

        // Start from the origin of the old coordinates and add vectors in the
        // old coords that represent the Xnew, Ynew and Znew to find the point
        // in the old system.
        axis[0] = delta_x[0] + ex[0] * Xnew + ey[0] * Ynew - ez[0] * Znew;
        axis[1] = delta_y[0] + ex[1] * Xnew + ey[1] * Ynew - ez[1] * Znew;
        axis[2] = z0 + ex[2] * Xnew + ey[2] * Ynew - ez[2] * Znew + delta_base_height;
}

uint8_t kinematics_home(void)
{
        uint8_t result = 0;

        result = mc_home_axis(AXIS_Z, LIMITS_DELTA_MASK);
        if (result != 0)
        {
                return result;
        }

        // unlocks the machine to go to offset
        cnc_unlock(true);

        float target[AXIS_COUNT];
        motion_data_t block_data = {0};
        mc_get_position(target);
        target[AXIS_Z] += ((g_settings.homing_dir_invert_mask & (1 << AXIS_Z)) ? -g_settings.homing_offset : g_settings.homing_offset);
        block_data.feed = g_settings.homing_fast_feed_rate;
        block_data.spindle = 0;
        block_data.dwell = 0;
        // starts offset and waits to finnish
        mc_line(target, &block_data);
        itp_sync();

        target[AXIS_X] = 0.5f*g_settings.max_distance[AXIS_X];
        target[AXIS_Y] = 0.5f*g_settings.max_distance[AXIS_Y];
        target[AXIS_Z] = g_settings.max_distance[AXIS_Z];
        // reset position
        itp_reset_rt_position(target);
		cnc_clear_exec_state(EXEC_HOMING);
        return STATUS_OK;
}

void kinematics_apply_transform(float *axis)
{
}

void kinematics_apply_reverse_transform(float *axis)
{
}

#endif
