/*
 * Module for remote control for Tetris game
 * Author: Aditi (aditijb@stanford.edu)
 */

#include "gpio.h"
#include "timer.h"
#include "servo.h"
#include "gpio_interrupt.h"
#include "interrupts.h"
#include "uart.h"
#include "LSD6DS33.h"
#include "i2c.h"
#include "ringbuffer.h"
#include "remote.h"
#include <stddef.h>
#include "music.h"
#include "passive_buzz_intr.h"

static remote_t remote ;

// 'handle_button'
// handles a button press 
static void handle_button(uintptr_t pc, void *aux_data) {
    gpio_interrupt_clear(remote.button) ;

    remote_t *rem = (remote_t *)aux_data ;
    // don't check whether it was added; it's trivial and unlikely that the queue overfills and branching takes time
    rb_enqueue(rem->rb, 1) ; 
}

// 'remote_is_button_press'
// checks if there are presses in the queue
bool remote_is_button_press(void) {
    int k = 0 ;
    if (!(rb_empty(remote.rb))) {
        servo_vibrate_milli_sec(100) ;
        rb_dequeue(remote.rb, &k) ;
        return true ;
    }
    return false ;
}

// 'remote_init'
// initializes button, servo, i2c, accelerometer, and interrupts for button
void remote_init(gpio_id_t servo_id, gpio_id_t button_id, gpio_id_t buzzer_id, int music_tempo) {
    
    remote.button = button_id ;
    gpio_set_input(button_id) ;

    remote.servo = servo_id ;    
    servo_init(servo_id) ;

    remote.rb = rb_new() ;

    // accelerometer init
    i2c_init();
	lsm6ds33_init();

    remote.buzzer = buzzer_id ;    
    buzzer_intr_init(buzzer_id, music_tempo) ; // this uses both timer0 and timer1 for the pwm and note-change :)

    gpio_interrupt_init() ;
    gpio_interrupt_config(remote.button, GPIO_INTERRUPT_POSITIVE_EDGE, true) ; // if pressed
    gpio_interrupt_register_handler(remote.button, handle_button, &remote) ;
    gpio_interrupt_enable(remote.button) ;

}

// 'remote_vibrate'
// only vibrates the most recently configured servo
void remote_vibrate(int duration_sec) {
    servo_vibrate(duration_sec) ;
}

// 'remote_get_x_y_status'
// returns int enum "left/right/home" ... enum defined in lsd6ds33.h
void remote_get_x_y_status(int *x_mod, int *y_mod) {
    short x=0; short y=0; 
    lsm6ds33_read_durable_pos(&x, &y, x_mod, y_mod) ; // read and print avged positions
}

