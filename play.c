#include <assert.h>
#include <sys/ioctl.h>
#include <sys/kd.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <mmli.h>

#ifndef CLOCK_TICK_RATE
#define CLOCK_TICK_RATE 1193180
#endif

#define SLEEP_TIME 20000

//const char* str = "t136mno3l8ddgfe-dco2b-ago3d2.l12dddl8g4p4p2p2t236l6o2dddl2mlgo3ddmnl6co2bal2o3mlgddmnl6co2bamll2o3gddmnl6co2bo3cl2mlo2a1a4p4mnt236l6o2dddl2mlgo3ddmnl6co2bal2o3mlgddmnl6co2bamll2o3gddmnl6co2bo3cl2mlo2a1a4p4mnt136mno3l8p4mno2l8d4e4.eo3co2bagl12gabl8a8.e16f+4d8.de4.eo3co2bago3d8.o2a16mla4a4mnd4e4.eO3co2bagl12gaba8.e16f+4o3d8.d16l16g8.fe-8.dc8.o2b-a8.go3d2t236l6o2dddl2mlgo3ddmnl6co2bal2o3mlgddmnl6co2bamll2o3gddmnl6co2bo3cl2mlo2a1a4p4mnt236l6o2dddl2mlgo3ddmnl6co2bal2o3mlgddmnl6co2bamll2o3gddmnl6co2bo3cl2mlo2a1a4p4mnl6o3mndddmll1gggg4p4p4mnl12dddg2";
//const char* str = "T240<BB>CDDC<BAGGABB3A8A2T240<P8BB>CDDC<BAGGABA3G8G2T240<P8AABGAB8>C8<BGAB8>C8<BAGADT240<P4BB>CDDC<BAGGABA2G8G2T240<G2A2GB8A8G2>>D2";
const char* fgstr = "T133MFO3L24MLCCP24<AAP24>CCCDDCP24P24<EEEEFFFF#F#F#GGGP24P24P24GG<GGG>GGP24P24P24P24P24P24P24P24DDD#EED#EEGGGAGGGCCCP24P24P24P24P24P24<GGG#AAA-AA>CCCDCCC<AAAP24P24P24P24P24P24G#G#G#GGG-GG>GGP24P24<GGG-GG>GGP24P24<GGG-GG>GFFFGGGAAGP24P24DP24P24P24P24P24P24P24P24P24DDD#EEE-EEGGGAGGGA#A#A#P24P24P24P24P24P24GGG#AAA-AA>CCCDCCC<AAAP24P24P24P24P24P24G#G#G#GGG-GG<GP24P24P24>GGG-GG<GP24P24P24>EEP24EEP24CCCCCCP24P24P24P24P24P24P24P24P24P24P24P24P24P24P24P24P24P24";
const char* bgstr = "T133MBO2L24MLCCP24<AAP24>CCCDDCP24P24<EEEEFFFF#F#F#GGGP24P24G<GGG>GGGP24P24P24P24P24G<GGGGGGO1L24CCCCCCCCCCP24CP24P24P24P24P24P24P24P24P24EEEFFFFFFFFFFP24FP24P24P24P24P24P24P24P24P24FFFEEEEEEEEMSEMLEP24D#D#D#D#D#D#D#D#D#MSD#MLD#P24D#DDDP24P24P24DDP24P24P24GGGGP24P24P24P24P24P24<GGG>CCCCCCCCCCP24CP24P24P24P24P24P24P24P24P24EEEFFFFFFFFFFP24FP24P24P24P24P24P24P24P24P24F#F#F#GGMSGMLGG<GP24P24P24>GGMSGMLGG<GP24P24P24GGMSGMLGGMSGML>CCCCCCCCCCCCCCC";

int console_fd;

struct note_t {
  int channel;
  int rval;
  struct mmli_context ctx;
  int play;
  int rest;
  int freq;
};

void note_init(struct note_t* note, const char* str, int channel) {
  mmli_init (&note->ctx);
  mmli_set (&note->ctx, str);
  note->channel = channel;
  note->play = 0;
  note->rest = 0;
  note->freq = 0;
  note->rval = 0;
}

void note_next(struct note_t* note) {
  float play, rest, freq;
  if(note->ctx.tail == NULL) { note->play = 0; note->rest = 0; note->freq = 0; return; }
  note->rval = mmli_next (&note->ctx, &freq, &play, &rest, &note->channel);
  note->play = (int)(1e6 * play);
  note->rest = (int)(1e6 * rest);
  note->freq = (int) freq;
}

int note_playing(struct note_t* note) {
  return note->play > 0;
}

int note_is_current(struct note_t* note) {
  return note->play > 0 || note->rest > 0;
}

void note_play(struct note_t* note) {
  if(note->play > 0) {
    ioctl(console_fd, KIOCSOUND, note->freq != 0 ? (int)(CLOCK_TICK_RATE/note->freq) : note->freq);
  }
}

void note_tick(struct note_t* note) {
  if(note->play > 0) {
    note->play -= SLEEP_TIME;
  } else {
    note->rest -= SLEEP_TIME;
  }
}

void note_stop() {
  int freq = 0;
  ioctl(console_fd, KIOCSOUND, freq != 0 ? (int)(CLOCK_TICK_RATE/freq) : freq);
}

int music(const char* str1, const char* str2) {
  int channel = 0;
  struct note_t fg;
  struct note_t bg;
  note_init(&fg, str1, 0);
  note_init(&bg, str2, 1);

  while ( 1 ) {
    if(note_playing(&fg) && note_playing(&bg)) {
      if(channel == 0) {
        note_play(&fg);
      } else {
        note_play(&bg);
      }
    } else if(note_playing(&fg)) {
      note_play(&fg);
    } else if(note_playing(&bg)) {
      note_play(&bg);
    } else {
      note_stop();
      if(fg.ctx.tail == NULL && bg.ctx.tail == NULL) {
        return;
      }
    }

    note_tick(&fg);
    note_tick(&bg);

    if(!note_is_current(&fg)) {
      note_next(&fg);
    }

    if(!note_is_current(&bg)) {
      note_next(&bg);
    }
    usleep(SLEEP_TIME);
    channel = ( channel + 1 ) % 2;
  }
}

int main() {
  int i;
  console_fd = open("/dev/console", O_WRONLY);
  music(fgstr, bgstr);
}
