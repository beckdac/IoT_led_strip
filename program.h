#ifndef _PROGRAM_H_
#define _PROGRAM_H_

#define PROGRAM_PREAMBLE 8576
#define PROGRAM_PREAMBLE_LOCATION 0
#define PROGRAM_START 2

#define PROGRAM_STEP_LOCATION(step) (PROGRAM_START + sizeof(uint16_t) + (step * ((sizeof(uint8_t) * 3) + sizeof(uint16_t))))

typedef enum { PROGRAM_STOP = 0, PROGRAM_RUN, PROGRAM_PROGRAMMING } program_state_e;

void program_init(void);
void program_setup_default(void);
void program_run(void);

#endif /* _PROGRAM_H_ */
