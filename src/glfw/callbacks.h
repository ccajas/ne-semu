/* This file should be included only in one place */

#ifndef _GLFW_CALLBACKS_H
#define _GLFW_CALLBACKS_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

/* Forward declarations */

typedef struct App_struct App;

extern void app_capture_mouse_scroll (App * const app, double xOffset, double yOffset);
extern void app_capture_drop         (App * const app, char * paths[]);

void glfw_cb_error  (int error, const char* description);
void glfw_cb_scroll (GLFWwindow * window, double xoffset, double yoffset);

void glfw_cb_drop        (GLFWwindow * window, int count, char * paths[]);
void glfw_cb_window_size (GLFWwindow * window, int width, int height);

GLFWwindow * glfw_new_window(int width, int height, const char *title, GLFWwindow * shared);

#endif