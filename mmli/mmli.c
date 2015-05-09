/*** mmli.c --- Music Macro Language interpreter  -*- C -*- */

/*** Ivan Shmakov, 2012 */

/** This code is in the public domain.
 **
 ** While this code is modelled after the spkr.c (Revision: 1.80) driver
 ** from FreeBSD, it contains no actual code from there.
 */

/*** Code: */

#include <ctype.h>              /* for isdigit (), tolower () */
#include <errno.h>              /* for EINVAL, errno */
#include <stddef.h>             /* for ptrdiff_t */
#include <stdlib.h>             /* for strtol () */
#include <limits.h>             /* for INT_MIN, LONG_MIN */

#include <mmli.h>

/*** Music Macro Language interpreter */

/** Music Macro Language (AKA PLAY string) interpretation, as
 ** implemented here, is modelled after the spkr.c (Revision: 1.80)
 ** driver from FreeBSD, which was itself modelled after (to quote the
 ** comment in the code) "the IBM BASIC 2.0's PLAY statement."  In
 ** particular, M[LNS] (legato, normal and staccato) are supported; the
 ** ~ synonym to P, the _ slur mark, and the octave-tracking facility
 ** are implemented.
 */

#define SEMITONES   (12)
#define A_SHIFT     (-3)

#define DFL_OCTAVE  (4)
#define DFL_TEMPO   (120)

#define TEMPO_WHOLE(tempo) (60 / (.25 * (tempo)))

static const int notes[] = {
  /* a  ais  b   c cis d  dis  e   f  fis  g  gis */
  -3,       -1,  0,    2,      4,  5,      7
};

/** This variant of the table was made with the following Emacs Lisp
 ** code:
 **
 ** (let ((a "")
 **       (p -33)
 **       (i 0))
 **   (while (< i 84)
 **     (let ((f (* 440 (expt 2 (/ (+ p i) 12.0)))))
 **       (setq a (format "%s%10.5f,%s"
 **                       a f (if (> 3 (mod i 4)) " " "\n"))
 **             i (+ 1 i))))
 **   a)
 */
static const float pitches[] = {
  /* O0 */
    65.40639,   69.29566,   73.41619,   77.78175,
    82.40689,   87.30706,   92.49861,   97.99886,
   103.82617,  110.00000,  116.54094,  123.47083,
  /* O1 */
   130.81278,  138.59132,  146.83238,  155.56349,
   164.81378,  174.61412,  184.99721,  195.99772,
   207.65235,  220.00000,  233.08188,  246.94165,
  /* O2 */
   261.62557,  277.18263,  293.66477,  311.12698,
   329.62756,  349.22823,  369.99442,  391.99544,
   415.30470,  440.00000,  466.16376,  493.88330,
  /* O3 */
   523.25113,  554.36526,  587.32954,  622.25397,
   659.25511,  698.45646,  739.98885,  783.99087,
   830.60940,  880.00000,  932.32752,  987.76660,
  /* O4 */
  1046.50226, 1108.73052, 1174.65907, 1244.50793,
  1318.51023, 1396.91293, 1479.97769, 1567.98174,
  1661.21879, 1760.00000, 1864.65505, 1975.53321,
  /* O5 */
  2093.00452, 2217.46105, 2349.31814, 2489.01587,
  2637.02046, 2793.82585, 2959.95538, 3135.96349,
  3322.43758, 3520.00000, 3729.31009, 3951.06641,
  /* O6 */
  4186.00904, 4434.92210, 4698.63629, 4978.03174,
  5274.04091, 5587.65170, 5919.91076, 6271.92698,
  6644.87516, 7040.00000, 7458.62018, 7902.13282,
};

static const int pitch_max
  = (-1 + sizeof (pitches) / sizeof (*pitches));
static const int octave_max
  = (-1 + sizeof (pitches) / sizeof (*pitches)) / SEMITONES;

static int
interp_note (struct mmli_context *x,
             float *freqp, float *durationp, float *restp,
             const char *s, const char **tailp)
{
  const char *p;

  for (p = s; isspace (*p); p++)
    ;
  if (*p == '\0') {
    /* FIXME: a specific indication for this condition? */
    if (tailp     != 0) { *tailp      = 0; }
    if (freqp     != 0) { *freqp      = 0; }
    if (durationp != 0) { *durationp  = 0; }
    if (restp     != 0) { *restp      = 0; }
    /* . */
    return 0;
  }

  const char c
    = tolower (*p);
  char *t
    = 1 + p;
  long pitch
    = LONG_MIN;

  switch (c) {
  case 'n':
    {
      char *newt;
      pitch = strtol (t, &newt, 10);
      t = (newt == t ? p : newt);
    }
    break;
  case 'p':
  case '~':
    /* NB: all ok, duration handled later */
    break;
  case 'a': case 'b': case 'c': case 'd':
  case 'e': case 'f': case 'g':
    pitch
      = notes[c - 'a'];
    /* NB: for octave no-tracking mode to work */
    if (pitch < 0) { pitch += SEMITONES; }
    pitch += SEMITONES * x->octave;
    if (*t == '#' || *t == '+') {
      ++pitch;
      ++t;
    } else if (*t == '-') {
      --pitch;
      ++t;
    }
    break;
  default:
    t = p;
    break;
  }

  /* fail early */
  if (t == p) {
    errno = EINVAL;
    if (tailp != 0) { *tailp = s; }
    /* . */
    return -1;
  }

  long octave
    = x->octave;

  if (c != 'n'
      && pitch != LONG_MIN
      && x->last_pitch != INT_MIN
      && x->oct_track_p && ! x->oct_override_p) {
    const int leap
      = SEMITONES / 2;
    const int l
      = x->last_pitch;
    /* -6 -5 -4 -3 -2 -1  0  1  2  3  4  5  6 */
    /* gis a ais b  c cis d dis e  f fis g gis */
    /* FIXME: check if it's (>=, >), and not (>, >=) */
    if (pitch >= l + leap) {
      pitch -= SEMITONES;
      octave--;
    } else if   (l - leap >  pitch) {
      pitch += SEMITONES;
      octave++;
    }
  }

  /* check if pitch is within the range */
  if (pitch != LONG_MIN
      && (pitch < 0 || pitch >= pitch_max)) {
    /* . */
    errno = EINVAL;
    if (tailp != 0) { *tailp = s; }
    return -1;
  }

  float dmul
    = x->dmul;

  if (c != 'n'
      && isdigit (*t)) {
    char *newt;
    long ddiv
      = strtol (t, &newt, 10);
    t = (newt == t ? p : newt);
    if (t != p) {
      dmul
        = (float)1 / ddiv;
    }
  }

  float fill
    = x->fill;

  /* fail here (the final check) */
  if (t == p) {
    errno = EINVAL;
    if (tailp != 0) { *tailp = s; }
    /* . */
    return -1;
  }

  /* check for sustain dots */
  for (; *t != '\0' && *t == '.'; t++) {
    dmul *= 1.5;
  }

  /* check for a slur mark */
  if (*t == '_') {
    fill = 1;
    t++;
  }

  /* update the context */
  /* (->octave, ->last_pitch, ->oct_override_p) */
  if (c != 'n'
      && x->oct_track_p && ! x->oct_override_p) {
    x->octave = octave;
  }
  x->oct_override_p = 0;
  if (pitch != LONG_MIN) {
    x->last_pitch = pitch;
  }

  /* record the current tone */
  if (freqp == 0 && durationp == 0 && restp == 0) {
    /* do nothing */
    /* obviously, a kind of a quiet mode */
  } else if (pitch == LONG_MIN) {
    if (freqp     != 0) { *freqp      = 0; }
    if (durationp != 0) { *durationp  = 0; }
    if (restp     != 0) { *restp      = x->whole * dmul; }
  } else {
    const float d
      = x->whole * dmul;
    if (freqp     != 0) { *freqp      = pitches[pitch]; }
    if (durationp != 0) { *durationp  = d *      fill;  }
    if (restp     != 0) { *restp      = d * (1 - fill); }
  }

  if (tailp != 0) { *tailp = t; }

  /* . */
  return 0;
}

static int
interp_ctl (struct mmli_context *x,
            const char *s, const char **tailp)
{
  const char *p;

  for (p = s; *p != '\0'; ) {
    /* skip over the whitespace */
    if (isspace (*p)) {
      p++;
      continue;
    }

    //printf("%c", p[0]);

    const char c
      = tolower (*p);
    ptrdiff_t shift
      = 1;

    int inval_p
      = 0;
    long new_ddiv
      = -1;
    long new_octave
      = x->octave;
    long new_tempo
      = -1;

    switch (c) {
    case 'o':
      if (isdigit (p[1])) {
        char *t;
        new_octave = strtol (p + 1, &t, 10);
        shift = (t - p);
      } else switch (tolower (p[1])) {
      case 'n':
        x->oct_override_p = x->oct_track_p = 0;
        shift = 2;
        break;
      case 'l':
        x->oct_track_p = 1;
        shift = 2;
        break;
      default:
        new_octave = -1;
        break;
      }
      break;
    case '<':
      new_octave--;
      break;
    case '>':
      new_octave++;
      break;
    case 'l':
    case 't':
      {
        long *vp
          = (c   == 'l' ? &new_ddiv : &new_tempo);
        if (isdigit (p[1])) {
          char *t;
          *vp = strtol (p + 1, &t, 10);
          shift = (t - p);
        } else {
          *vp = 0;
        }
      }
      break;
    case 'm':
      switch (tolower (p[1])) {
      case 's': x->fill = 6. / 8;   shift = 2; break;
      case 'n': x->fill = 7. / 8;   shift = 2; break;
      case 'l': x->fill = 1;        shift = 2; break;
      default:
        inval_p = 1;
        break;
      }
      break;
    default:
      inval_p = 1;
      break;
    }

    msnl_done:

    //printf("%d", inval_p);

    if (new_octave < 0 || new_octave > octave_max
        || new_ddiv   == 0
        || new_tempo  == 0
        || inval_p) {
      errno = EINVAL;
      if (tailp != 0) { *tailp = p; }
      /* . */
      return -1;
    }

    x->octave
      = new_octave;
    if (new_ddiv > 0) {
      x->dmul
        = (float)1 / new_ddiv;
    }
    if (new_tempo > 0) {
      x->whole
        = TEMPO_WHOLE (new_tempo);
    }

    p += shift;
  }

  if (tailp != 0) { *tailp = p; }
  /* . */
  return 0;
}

/*** Public interface */

void
mmli_init  (struct mmli_context *c)
{
  c->tail       = 0;
  c->octave     = DFL_OCTAVE;
  c->whole      = TEMPO_WHOLE (DFL_TEMPO);
  c->dmul       = .25;          /* quarters */
  c->fill       = .125 * 7;     /* 7/8 (normal) */
  c->last_pitch = INT_MIN;
  c->oct_track_p    = 1;
  c->oct_override_p = 1;

  /* . */
}

int
mmli_ctl   (struct mmli_context *c, const char *s,
            const char **tailp)
{
  /* . */
  return
    interp_ctl (c, s, tailp);
}

void
mmli_set   (struct mmli_context *c, const char *s)
{
  c->tail = s;
  /* . */
}

const char *
mmli_tail (const struct mmli_context *c)
{
  /* . */
  return c->tail;
}

float
mmli_fill (const struct mmli_context *c)
{
  /* . */
  return c->fill;
}

int
mmli_next     (struct mmli_context *c,
               float *freqp, float *durationp, float *restp)
{
  const char *t;

  /* interpret all the controls first */
  int rc
    = interp_ctl (c, c->tail, &t);

#if 0
  /* NB: will be done by interp_note () later, anyway */

  if (*t == '\0') {
    /* finished */
    c->tail = t;

    /* FIXME: a specific indication for this condition? */
    if (freqp     != 0) { *freqp      = 0; }
    if (durationp != 0) { *durationp  = 0; }
    if (restp     != 0) { *restp      = 0; }

    /* . */
    return rc;
  }

  /* NB: assuming rc < 0 (though it hardly matters) */
#endif

  /* try to interpret the notes, if any */
  int rn
    = interp_note (c, freqp, durationp, restp,
                   c->tail = t, &t);

  if (rn < 0) {
    /* NB: assuming t == c->tail; *freqp, etc. untouched */
    /* . */
    return rn;
  }

  c->tail = t;

  /* . */
  return rn;
}

void
mmli_octave   (const struct mmli_context *c, int *octavep,
               int *track_pp, int *over_pp)
{
  if (octavep  != 0) { *octavep   = c->octave; }
  if (track_pp != 0) { *track_pp  = c->oct_track_p; }
  if (over_pp  != 0) { *over_pp   = c->oct_override_p; }
  /* . */
}

void
mmli_whole    (const struct mmli_context *c, float *wholep)
{
  if (wholep != 0) { *wholep = c->whole; }
  /* . */
}

/*** Emacs trailer */
/** Local variables: */
/** coding:              us-ascii */
/** mode:                outline-minor */
/** fill-column:         64 */
/** indent-tabs-mode:    nil */
/** outline-regexp:      "[*][*][*]" */
/** End: */
/*** mmli.c ends here */
