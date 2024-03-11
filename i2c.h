// https://github.com/cs107e/cs107e.github.io/blob/master/lectures/Sensors/code/accel/i2c.h
// by Julie Zelenski
#pragma once

void i2c_init(void);
void i2c_write(unsigned char device_id, unsigned char *data, int data_length);
void i2c_read(unsigned char device_id, unsigned char *data, int data_length);