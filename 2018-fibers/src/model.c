#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include "model.h"
#include "calqueue.h"
#include "numerical.h"

// Prepare a new event to schedule
static msg_t *pack_event(unsigned int cell, int type, double stamp, int ch) {
		fprintf(stderr,"1pe\n");

	msg_t *new_evt;
	
	new_evt = malloc(sizeof(msg_t));
	new_evt->sender = cell;
	new_evt->receiver = cell;
	new_evt->type = type;
	new_evt->timestamp = stamp;
	new_evt->channel = ch;
		fprintf(stderr,"2pe\n");
	
	return new_evt;
}

// Entry point for the model, all events are passed to this handler
long long ProcessEvent(msg_t *msg, lp_state_type *state, calqueue *q) {
	long long ret = 0;
	unsigned int me = msg->receiver;
	double now = msg->timestamp;
	double timestamp;
	unsigned int i;

	if(state != NULL) {
		state->lvt = now;
		state->executed_events++;
		//printf("%d executing %d at %f\n", me, msg->type, now);
		fflush(stdout);
	}
			fprintf(stderr,"1p\n");
	
	switch(msg->type) {

		case INIT:
			fprintf(stderr,"2p\n");

			// Initialize the GSM cell state
			state = malloc(sizeof(lp_state_type));
			if (state == NULL){
				printf("Out of memory!\n");
				exit(EXIT_FAILURE);
			}
			fprintf(stderr,"3p\n");

			bzero(state, sizeof(lp_state_type));
			state->channel_counter = CHANNELS_PER_CELL;
			state->ref_ta = state->ta = TA;
			state->ta_duration = TA_DURATION;
			state->ta_change = TA_CHANGE;
			state->channels_per_cell = CHANNELS_PER_CELL;
			state->channel_counter = state->channels_per_cell;
			fprintf(stderr,"4p\n");

			// Setup channel state
			state->channel_state = malloc(sizeof(unsigned int) * 2 * (CHANNELS_PER_CELL / BITS + 1));
			for (i = 0; i < state->channel_counter / (sizeof(int) * 8) + 1; i++)
				state->channel_state[i] = 0;
			fprintf(stderr,"5p\n");

			// Start the simulation
			//ERR
			timestamp = 20 * Random();
			fprintf(stderr,"5.5p\n");

			calqueue_put(q, timestamp, pack_event(me, START_CALL, timestamp, -1));
						fprintf(stderr,"6p\n");

			ret = (long long)state;
			break;

		case START_CALL:
			fprintf(stderr,"7p\n");

			state->arriving_calls++;

			if (state->channel_counter == 0) {
				state->blocked_on_setup++;
			} else {
				state->channel_counter--;
				timestamp = now + Expent(state->ta_duration);
				calqueue_put(q, timestamp, pack_event(me, END_CALL, timestamp, allocation(state)));
			}
			fprintf(stderr,"8p\n");

			state->ta = recompute_ta(state->ref_ta, now);
			timestamp = now + Expent(state->ta);
			calqueue_put(q, timestamp, pack_event(me, START_CALL, timestamp, -1));
			fprintf(stderr,"9p\n");

			break;

		case END_CALL:
			fprintf(stderr,"10p\n");

			state->channel_counter++;
			state->complete_calls++;
			deallocation(me, state, msg->channel, now);
			fprintf(stderr,"11p\n");

			if(state->complete_calls == COMPLETE_CALLS)
				ret = 1;

			break;

		default:
			fprintf(stdout, "PCS: Unknown event type! (me = %d - event type = %d)\n", me, msg->type);
			abort();
	}
	
	return ret;
}
