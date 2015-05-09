/*** mmlitest.c --- Music Macro Language interpreter: a test  -*- C -*- */

/*** Ivan Shmakov, 2012 */
/** This code is in the public domain. */

/*** Code: */

#include <assert.h>		/* for assert () */
#include <stdio.h>		/* for printf () */

#include <mmli.h>

int
main (int argc, const char *argv[])
{
  /* Beethoven's Fifth */
  const char *dfl_song
    = ("T180 o2 P2 P8 L8 GGG L2 E-"
       "P24 P8 L8 FFF L2 D");
  const char *s
    = (-1 + argc == 1 ? argv[1] : dfl_song);
  struct mmli_context x;

  mmli_init (&x);

  mmli_set (&x, s);

  int done_p;
  float t;
  for (done_p = 0, t = 0; ! done_p; ) {
    float freq, dur, rest;
    int r
      = mmli_next (&x, &freq, &dur, &rest);
#if 1
    printf ("-n -f %.3f -l %.0f -D %.0f\n",
	    (freq < 1 ? 1 : freq), 1e3 * dur, 1e3 * rest);
#else
    printf (("t: %8.3f\tf: %8.3f\td: %8.3f\tr: %8.3f"
             "\t(p: %2d)\tS: %s\n"),
	    t, freq, dur, rest,
            /* FIXME: call an interface function */
            x.last_pitch,
            mmli_tail (&x));
#endif
    assert (r >= 0);
    if (dur == 0 && rest == 0) {
      /* NB: finished */
      assert (freq == 0);
      done_p = 1;
      continue;
    }
    t += dur + rest;
  }

  /* . */
  return 0;
}

/*** Emacs trailer */
/** Local variables: */
/** coding:              us-ascii */
/** mode:                outline-minor */
/** fill-column:         64 */
/** indent-tabs-mode:    nil */
/** outline-regexp:      "[*][*][*]" */
/** End: */
/*** mmlitest.c ends here */
