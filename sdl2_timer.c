//
// sdl2_timer.h
// sdl2-sparkling
//
// Created by Arpad Goretity
// on 02/04/2015
//
// Licensed under the 2-clause BSD License
//

#include "sdl2_timer.h"
#include "sdl2_sparkling.h"

typedef struct spn_SDL_Timer {
	SpnObject base;
	SDL_TimerID ID;
	SpnContext *ctx;
	SpnFunction *callback;
} spn_SDL_Timer;

// If a timer object goes out of scope, stop it
static void spn_SDL_Timer_dtor(void *obj)
{
	spn_SDL_Timer *timer = obj;

	if (timer->ID) {
		SDL_RemoveTimer(timer->ID);
		timer->ID = 0;
	}
}

static const SpnClass spn_SDL_Timer_class = {
	sizeof(spn_SDL_Timer),
	SPN_SDL_CLASS_UID_TIMER,
	NULL,
	NULL,
	NULL,
	spn_SDL_Timer_dtor
};

static Uint32 native_timer_callback(Uint32 delay, void *data)
{
	spn_SDL_Timer *timer = data;

	// if the user supplied a callback function, call it
	// otherwise, just generate a timer event
	if (timer->callback) {
		// XXX: TODO: how can we do error checking here...!?
		spn_ctx_callfunc(timer->ctx, timer->callback, NULL, 0, NULL);
	} else {
		SDL_Event event;
		SDL_zero(event);
		event.type = SDL_USEREVENT;
		event.user.data1 = timer;
		SDL_PushEvent(&event);
	}

	return delay;
}

// Initiate a timer and return the corresponding descriptor object.
// Times are recurring: you must explicitly stop them if you don't
// want them to fire continuously.
// Also, when a timer descriptr object is deallocated (goes out of
// scope), its corresponding timer will be stopped automatically.
// Consequently, in order to keep the timer alive, you need to
// hold a reference to the returned timer descriptor (e. g. store
// it in a variable or a data structure).
// The first argument is the timeout in seconds (may be fractional),
// the second argument is an optional callback function which will
// be called in the current context. If omitted, the timer will
// generate SDL_Events of type "timer" instead.
int spnlib_SDL_StartTimer(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, number);

	if (argc > 1) {
		CHECK_ARG_RETURN_ON_ERROR(1, func);
	}

	double dt = NUMARG(0);
	SpnFunction *callback = argc > 1 ? FUNCARG(1) : NULL;

	spn_SDL_Timer *timer = spn_object_new(&spn_SDL_Timer_class);
	SDL_TimerID timerID = SDL_AddTimer(dt * 1000, native_timer_callback, timer);

	timer->ID = timerID;
	timer->ctx = ctx;
	timer->callback = callback;

	*ret = spn_makestrguserinfo(timer);
	return 0;
}

// Stop a timer object.
int spnlib_SDL_StopTimer(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	CHECK_ARG_RETURN_ON_ERROR(0, strguserinfo);
	spn_SDL_Timer *timer = OBJARG(0);

	if (timer->base.isa->UID != SPN_SDL_CLASS_UID_TIMER) {
		spn_ctx_runtime_error(ctx, "argument is not a timer object", NULL);
		return -1;
	}

	if (timer->ID) {
		SDL_RemoveTimer(timer->ID);
		timer->ID = 0;
	}

	return 0;
}
