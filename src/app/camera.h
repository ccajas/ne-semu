#ifndef CAMERA_H
#define CAMERA_H

#include "inputstates.h"

typedef struct Camera_struct
{
	vec3  pos, front, target, up;
	float fov;
	float pitch, yaw;
	float sensitivity;
}
Camera;

void renderer_update_camera (Camera *camera, float targetDist)
{
	/* Set camera position and view from pitch, yaw, and distance */
	vec3 camDirection = {
		-sin(camera->yaw) * cos(camera->pitch),
		-sin(camera->pitch),
		-cos(camera->yaw) * cos(camera->pitch)
	};

	memcpy(&camera->front, camDirection, sizeof(vec3));
	vec3_scale (camera->front, camera->front, targetDist);
	vec3_add (camera->pos, camera->target, camera->front);
}

void camera_update (Camera *camera, MouseState mouseState, MouseState lastMouseState, float targetDist)
{
	if (camera->sensitivity < 1.0) {
		camera->sensitivity = 1.0;
	}
	
	float step = mouseState.scrollY / -20.0f;
	vec3 advance;
	vec3_scale (advance, camera->front, step);
	vec3_add (camera->target, camera->target, advance);
	vec3_add (camera->pos,    camera->pos,    advance);

    int pan = 
        (input_mouse (&mouseState, 0)) ? 0 :
        (input_mouse (&mouseState, 1)) ? 1 : -1;

	if (pan == 0) 
	{
		camera->yaw   += (lastMouseState.x - mouseState.x) / camera->sensitivity;
		camera->pitch += (lastMouseState.y - mouseState.y) / camera->sensitivity;
		if (camera->pitch >  89.8) { camera->pitch =  89.8; }
		if (camera->pitch < -89.8) { camera->pitch = -89.8; }

		renderer_update_camera(camera, targetDist);
	}
	else if (pan == 1) 
	{
		vec3 stepX, stepY, camFrameX, camFrameY;
		vec3_mul_cross(camFrameX, camera->up, camera->front);
		vec3_mul_cross(camFrameY, camera->front, camFrameX);

        vec3_scale (stepY, vec3_norm_r(camFrameY), (mouseState.y - lastMouseState.y) / camera->sensitivity);
		vec3_scale (stepX, vec3_norm_r(camFrameX), (lastMouseState.x - mouseState.x) / camera->sensitivity);

		vec3_add (camera->target, camera->target, stepX);
		vec3_add (camera->target, camera->target, stepY);
		vec3_add (camera->pos,    camera->target, camera->front);
	}
}

#endif