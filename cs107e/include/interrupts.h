#ifndef INTERRUPTS_H
#define INTERRUPTS_H

/*
 * Module to configure interrupts for Mango Pi.
 *
 * Author: Julie Zelenski <zelenski@cs.stanford.edu>
 */

#include <stdint.h>

typedef enum _interrupt_sources interrupt_source_t;


/*
 * `interrupts_init`: Required initialization for interrupts
 *
 * Initialize interrupts module and configure to a clean state.
 * After init, the state of the interrupt system will be:
 *
 *    - top-level trap handler installed and active
 *    - all interrupt sources are disabled
 *    - interrupts are globally disabled
 *
 * This module init should be called once (and only once)
 * before any calls to other functions in the interrupts module.
 * Calling the init function a second time will raise an error.
 * Without more specific initialization semantics and structure,
 * this is the safe approach that avoids having to debug why a
 * source suddenly stopped receiving interrupts after a re-init
 * silently wiped the settings from previous configuration.
 */
void interrupts_init(void);

/*
 * `interrupts_global_enable`
 *
 * Turn on interrupts system-wide. An interrupt generated on an
 * interrupt source that is enabled will call the registered handler.
 */
void interrupts_global_enable(void);

/*
 * `interrupts_global_disable`
 *
 * Turn off interrupts system-wide. No interrupts will be generated.
 * Does not remove registered handlers or disable interrupt sources,
 * only temporarily suspends interrupt generation. To resume
 * generating interrupts, call `interrupts_global_enable`.
 */
void interrupts_global_disable(void);

/*
 * `interrupts_enable_source`
 *
 * Enable a particular interrupt source. The source itself must
 * be configured to generate interrupts (and global interrupts must be
 * enabled) for a registered handler to be called.
 *
 * @param source    which interrupt source (see enumeration values below)
 *
 * An error is raised if `source` is not valid.
 */
void interrupts_enable_source(interrupt_source_t source);

/*
 * `interrupts_disable_source`
 *
 * Disable a particular interrupt source. Interrupts for this source
 * will not trigger a handler and will remain pending (until cleared).
 *
 * @param source    which interrupt source (see enumeration values below)
 *
 * An error is raised if `source` is not valid.
 */
void interrupts_disable_source(interrupt_source_t source);

/*
 * `handlerfn_t`
 *
 * This typedef gives a nickname to the type of function pointer used as
 * a handler callback. A handler is registered to an interrupt source. When
 * an interrupt is generated by that source, the handler is called to
 * process it. A handler takes two arguments. The first argument is the
 * value of the interrupted pc, the second is the client's
 * auxiliary data pointer (can be NULL if not used).
 */
typedef void (*handlerfn_t)(uintptr_t, void *);

/*
 * `interrupts_register_handler`
 *
 * Register the handler function for a given interrupt source. Each interrupt
 * source can have one handler: further dispatch should be managed by
 * the handler itself. Registering a handler does not enable the source:
 * this must be done separately through `interrupts_enable_source`.
 * These are separate because otherwise there can be impossible-to-solve
 * technical challenges such as
 *   - having an interrupt handled before `interrupts_register_handler` returns,
 *   - handling interrupts that were pending from a different use of the source,
 *   - changing the handler as one part of a larger atomic action.
 *
 * @param source    which interrupt source (see enumeration values below)
 * @param fn        handler function to call when interrupt generated on source
 * @param aux_data  client's data pointer to be passed as second argument
 *                  when calling handler function
 *
 * An error is raised if `source` is not valid. `aux_data` can be NULL if
 * handler function has no need for auxiliary data. If `fn` is NULL, this
 * removes any handler previously registered for `source`.
 */
void interrupts_register_handler(interrupt_source_t source, handlerfn_t fn, void *aux_data);

/*
 * 'interrupt_source_t` enumeration
 *
 * Below are interrupt sources for which this module can enable, disable,
 * and register a handler. Interrupt source numbers are assigned in
 * table 3-9 p.204-210 of the D1-H User Manual.
 */
enum _interrupt_sources {
    INTERRUPT_SOURCE_UART0 = 18,
    INTERRUPT_SOURCE_UART1 = 19,
    INTERRUPT_SOURCE_UART2 = 20,
    INTERRUPT_SOURCE_UART3 = 21,
    INTERRUPT_SOURCE_UART4 = 22,
    INTERRUPT_SOURCE_UART5 = 23,
    INTERRUPT_SOURCE_TWI0 = 25,
    INTERRUPT_SOURCE_TWI1 = 26,
    INTERRUPT_SOURCE_TWI2 = 27,
    INTERRUPT_SOURCE_TWI3 = 28,
    INTERRUPT_SOURCE_SPI0 = 31,
    INTERRUPT_SOURCE_SPI1 = 32,
    INTERRUPT_SOURCE_HSTIMER0 = 71,
    INTERRUPT_SOURCE_HSTIMER1 = 72,
    INTERRUPT_SOURCE_GPIOB = 85,
    INTERRUPT_SOURCE_GPIOC = 87,
    INTERRUPT_SOURCE_GPIOD = 89,
    INTERRUPT_SOURCE_GPIOE = 91,
    INTERRUPT_SOURCE_GPIOF = 93,
    INTERRUPT_SOURCE_GPIOG = 95,
};

#endif
