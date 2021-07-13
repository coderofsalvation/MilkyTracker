#pragma once

#ifndef CONVOLVE_H
#define CONVOLVE_H

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef float real;
typedef struct { real Re; real Im; } complex;

#ifndef PI
#define PI 3.14159265358979323846264338327950288
#endif

// filter scratch vars
extern float lastOutput;
extern float lastInput;
extern float momentum;

extern void print_vector(const char* title, complex* x, int n);
extern complex complex_mult(complex a, complex b);
extern void fft(complex* v, int n, complex* tmp);
extern void ifft(complex* v, int n, complex* tmp);
extern int convolve(float* x, float* h, int lenX, int lenH, float** output);
extern float filter(float input, float cutoff, float reso);


class Convolve
{
};
#endif