/*** mmli.h --- Music Macro Language interpreter (C header)  -*- C -*- */

/*** Ivan Shmakov, 2012 */
/** This code is in the public domain. */

/*** Code: */

struct mmli_context {
  const char *tail;     /* the rest of the string */
  int   octave;         /* current octave */
  float whole;          /* whole-note time, in seconds */
  float dmul;           /* duration multiplier */
  float fill;           /* note spacing (1 for legato) */
  int   last_pitch;     /* latest returned pitch, or -1 */
  /* octave tracking variables: */
  int   oct_track_p;
  int   oct_override_p; /* set after _init (), or an O-prefix */
};

void  mmli_init  (struct mmli_context *c);

int   mmli_ctl   (struct mmli_context *c, const char *s,
                  const char **tailp);
void  mmli_set   (struct mmli_context *c, const char *s);

const char *mmli_tail  (const struct mmli_context *c);

float mmli_fill     (const struct mmli_context *c);
int   mmli_next     (struct mmli_context *c,
                     float *freqp, float *durationp, float *restp);
void  mmli_octave   (const struct mmli_context *c, int *octavep,
                     int *track_pp, int *over_pp);
void  mmli_whole    (const struct mmli_context *c, float *wholep);

/*** Emacs trailer */
/** Local variables: */
/** coding:              us-ascii */
/** mode:                outline-minor */
/** fill-column:         72 */
/** indent-tabs-mode:    nil */
/** outline-regexp:      "[*][*][*]" */
/** End: */
/*** mmli.h ends here */
