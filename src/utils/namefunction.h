#ifndef NAMEFUNCTION_H
#define NAMEFUNCTION_H

typedef struct Model_struct Model;

typedef struct NameFunction_struct
{
    const char name[32];
    void (*parserFunc)();
}
NameFunction;

#endif