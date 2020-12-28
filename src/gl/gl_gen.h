#ifndef _GL_GEN_H
#define _GL_GEN_H

#define BUILD_GLFW

#if defined(BUILD_GTK)
    #include <epoxy/gl.h>
#endif

#if defined(BUILD_GLFW)
    #include <glad/glad.h>
    #define GLFW_INCLUDE_NONE
    #include <GLFW/glfw3.h>
#endif

#if defined(BUILD_LINUX)
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>
    #include <GL/gl.h>
    #include <GL/glx.h>
#endif

#endif