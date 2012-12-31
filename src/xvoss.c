/*
 * This file is part of xvoice, a voice control app for the linux
 * desktop.
 * Copyright (C) 2002 Brian Craft (bcboy@thecraftstudio.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <esd.h>
#include <sys/times.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

/*
 * ViaVoice constants which aren't defined for us.
 */

#define AUDIO_BIG_ENDIAN           1
#define AUDIO_LITTLE_ENDIAN        2

#define BLOCK_HEADER_SIZE          26

#define BLOCK_DATA_SIZE_8KHZ       2020
#define BLOCK_DATA_SIZE_11KHZ      3030
#define BLOCK_DATA_SIZE_22KHZ      6102

#define HEADER_OFFSET_BYTE_ORDER   0
#define HEADER_OFFSET_FORMAT       1
#define HEADER_OFFSET_SAMPLES      2
#define HEADER_OFFSET_VOLUME       3
#define HEADER_OFFSET_ONE          4
#define HEADER_OFFSET_BLOCK_INDEX  5
#define HEADER_OFFSET_END_OF_PCM   6
#define HEADER_OFFSET_PCM_LENGTH  11
#define HEADER_OFFSET_DATA_LENGTH 12

static struct {
  int playsock;
  int recsock;
  int rate;
  int buflen;
  int blocklen; /* includes header */
  int format;
  char *device;
} audio;

static int bufsize (int rate)
{
  switch (rate) {
    case 8000:
      return BLOCK_DATA_SIZE_8KHZ;
    case 11025:
      return BLOCK_DATA_SIZE_11KHZ;
    default:
      return BLOCK_DATA_SIZE_22KHZ;
  }
}

static int stop_record;

int AudioCreate (const char *file, FILE *log, int log_level)
{
  return 0;
}

void AudioDestroy (void)
{
}

int AudioConnect (const char *init_string, const char *client,
    int *handle, int rate, int format, int *data_size,
    int *byte_order, const char **signature, char *cepslan_data)
{
  static char temp[20];

  audio.rate = rate;
  audio.buflen = bufsize(rate);
  audio.blocklen = audio.buflen + BLOCK_HEADER_SIZE;
  audio.recsock = -1;
  audio.playsock = -1;
  audio.device = strdup(init_string);

  *data_size = audio.buflen;
  *byte_order = AUDIO_LITTLE_ENDIAN;
  sprintf (temp, "PCM%02d   ", (rate/1000));
  *signature = temp;
  *handle = -1;

  audio.format = ESD_BITS16 | ESD_MONO | ESD_STREAM | ESD_RECORD;

  return 0;
}


int AudioDisconnect (void)
{
  if (audio.device) free(audio.device);
  audio.device = 0;
  return 0;
}

int AudioGetHandle (void)
{
  return audio.recsock;
}


int AudioSetSource (short len, void *value)
{
  return 0;
}


int AudioQuerySource (short *len, void **value)
{
  return 0;
}

#define FRAGMENT_BITS 10

int AudioStartRecording (unsigned long *time_zero, int report_errors)
{
  struct tms now;
  int i;
  int data;

  if (audio.recsock == -1) {
    audio.recsock = open(audio.device, O_RDONLY);
    if (audio.recsock < 0) {
        return -1;
    }

    data = AFMT_S16_LE;
    i = ioctl(audio.recsock, SNDCTL_DSP_SETFMT, &data);

    data = 0;
    i |= ioctl(audio.recsock, SNDCTL_DSP_STEREO, &data);

    data = audio.rate;
    i |= ioctl(audio.recsock, SNDCTL_DSP_SPEED, &data);

    data = 0x7fff0000 | FRAGMENT_BITS;
    i |= ioctl(audio.recsock, SNDCTL_DSP_SETFRAGMENT, &data);

    i |= fcntl(audio.recsock, F_SETFL, O_NONBLOCK);

    if (i != 0) {
        return -1;
    }
  }
  *time_zero = 10*times(&now);
  stop_record = 0;
  return 0;
}

int AudioGetPCM (char *block_buffer, long max_bytes, long *new_bytes, 
        int *end_of_pcm)
{
  int i;
  short max;
  static int block_num = 0;
  static char *buf = NULL;
  static int n = 0;
  int ret;

  *new_bytes = 0;
  *end_of_pcm = stop_record;

  if (buf == NULL) {
    buf = malloc(audio.blocklen);
    if (buf == NULL) {
      ret = -1;
      goto done;
    }
  }

  i = read(audio.recsock, buf+BLOCK_HEADER_SIZE+n, audio.buflen-n);
  if (i < 0) {
    if (errno == EAGAIN) {
      ret = 0;
    } else {
      ret = -1;
    }
    goto done;
  }

  n += i;

  if (n == audio.buflen) {
    short *hdr = (short*)buf;
    short *data = (short*)(buf+BLOCK_HEADER_SIZE);
    memset (hdr, '\0', BLOCK_HEADER_SIZE);
    hdr[HEADER_OFFSET_BYTE_ORDER ] = 0x0102;
    hdr[HEADER_OFFSET_FORMAT     ] = 2;
    hdr[HEADER_OFFSET_SAMPLES    ] = audio.buflen / 2;
    hdr[HEADER_OFFSET_ONE        ] = 1;
    hdr[HEADER_OFFSET_PCM_LENGTH ] = audio.buflen;
    hdr[HEADER_OFFSET_DATA_LENGTH] = audio.buflen;
    hdr[HEADER_OFFSET_END_OF_PCM ] = stop_record;

    max = 0;
    for (i = 0 ; i < audio.buflen/2; i++) {
      if (abs(data[i]) > max) {
        max = abs(data[i]);
      }
    }

    hdr[HEADER_OFFSET_VOLUME     ] = max;
    hdr[HEADER_OFFSET_BLOCK_INDEX] = block_num++;

    memcpy(block_buffer, buf, audio.blocklen);
    *new_bytes = audio.blocklen;
    n = 0;
  }

done:
  if (stop_record) {
    close(audio.recsock);
    audio.recsock = -1;
  }

  return 0;
}

int AudioStopRecording (void)
{
  stop_record = 1;
  return 0;
}

int AudioStartPlayback (void)
{
  if (audio.playsock == -1) {
    audio.recsock = open(audio.device, O_RDONLY);
    if (audio.playsock < 0) {
        return -1;
    }
  }
  return 0;
}

int AudioPutPCM (char *buf, int len)
{
  short *header;
  int n;
  char *ptr;

  ptr = buf;
  while (ptr < buf + len) {
    header = (short*)ptr;
    len = header[HEADER_OFFSET_PCM_LENGTH];
    n = write(audio.playsock, ptr+BLOCK_HEADER_SIZE, len);
    if (n < 0 || header[HEADER_OFFSET_END_OF_PCM])
      break;
    ptr += audio.blocklen;
  }
  return 0;
}

int AudioStopPlayback (int flush_output)
{
  if (audio.playsock != -1) {
    esd_close(audio.playsock);
    audio.playsock = -1;
  }
  return 0;
}

int AudioQueryDevices (long *devices)
{
  return -1;
}

int AudioSetDevice (long device)
{
  return -1;
}

int AudioQueryConfig (long *configuration)
{
  return -1;
}

int AudioSetInput (long input_device)
{
  return -1;
}

int AudioSetInputGain (long input_gain)
{
  return -1;
}

int AudioSetOutput (long output_gain)
{
  return -1;
}

int AudioSetOutputGain (long output_gain)
{
  return -1;
}
