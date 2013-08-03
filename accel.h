/*
 * accel.h
 *
 * Created: 8/3/2013 5:22:28 PM
 *  Author: andre_000
 */ 


#ifndef ACCEL_H_
#define ACCEL_H_

//As we discussed before a #define is used as a shorthand for the compiler. For example, "#define F_CPU 2000000" is telling the compiler to substitute F_CPU anywhere it finds it on the code to the number 20000000.

#define SDA_ACCEL_WRITE 0b00110010

#define SDA_ACCEL_READ 0b00110011

#define X_AXIS_OUT_LOW 0b0101000

#define X_AXIS_OUT_HIGH 0b0101001


#endif /* ACCEL_H_ */