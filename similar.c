#include <stdio.h>
#include <sndfile.h>
#include <string.h>
#include <stdlib.h>

static SNDFILE *sound;
static SF_INFO soundinfo;
static uint8_t sync = 0xab;
static char eom[] = "NNNN";
static short bitbuf[120];

void encode(SNDFILE *sound, uint8_t *buf, size_t len) {
    size_t curbyte = 0;
    uint8_t curbit = 0;
    while (curbyte < len) {
        uint8_t levelperiod = ((buf[curbyte]>>curbit)&1)?15:20;
        curbit = (curbit+1)&7;
        if (curbit == 0) curbyte++;
        short sample = INT16_MAX>>1;
        for (uint32_t isample=0;isample<120;isample++) {
            if (isample % levelperiod == 0) {
                sample ^= 0x8000;
            }
            bitbuf[isample] = sample;
        }
        sf_write_short(sound, bitbuf, 120);
    }
}

void silence(SNDFILE *sound, uint32_t samples) {
    short *buf = calloc(samples, sizeof(short));
    if (!buf) return; //lol
    sf_write_short(sound, buf, samples);
    free(buf);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("usage: %s \"output filename\" \"text to encode\"\n", argv[0]);
        exit(1);
    }

    soundinfo.channels = 1;
    soundinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16 | SF_ENDIAN_LITTLE;
    soundinfo.samplerate = 62500;

    sound = sf_open(argv[1], SFM_WRITE, &soundinfo);
    if (!sound) {
        printf("couldn't open file for writing\n");
        exit(1);
    }

    //leader
    silence(sound, soundinfo.samplerate);

    //payload
    for (uint32_t headers=0;headers<3;headers++) {
        for (uint32_t i=0;i<16;i++) encode(sound, &sync, 1);
        encode(sound, (uint8_t *)argv[2], strlen(argv[2]));
        silence(sound, soundinfo.samplerate);
    }

    //eom
    for (uint32_t eoms=0;eoms<3;eoms++) {
        for (uint32_t i=0;i<16;i++) encode(sound, &sync, 1);
        encode(sound, (uint8_t *)eom, strlen(eom));
        silence(sound, soundinfo.samplerate);
    }

    sf_close(sound);
    exit(0);
}