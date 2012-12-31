/*
 * This file is part of xvoice, a voice control app for the linux
 * desktop.
 * Copyright (C) 2001 Brian Craft (bcboy@thecraftstudio.com)
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
#include <kde/artsc/artsc.h>
#include <sys/times.h>

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
  arts_stream_t playsock;
  arts_stream_t recsock;
  int rate;
  int buflen;
  int blocklen; /* includes header */
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
  audio.recsock = NULL;
  audio.playsock = NULL;

  *data_size = audio.buflen;
  *byte_order = AUDIO_LITTLE_ENDIAN;
  sprintf (temp, "PCM%02d   ", (rate/1000));
  *signature = temp;
  *handle = -1;

  return 0;
}


int AudioDisconnect (void)
{
  return 0;
}

int AudioGetHandle (void)
{
  return (int)audio.recsock; /* XXX if anyone uses this we're toast */
}


int AudioSetSource (short len, void *value)
{
  return 0;
}


int AudioQuerySource (short *len, void **value)
{
  return 0;
}

int AudioStartRecording (unsigned long *time_zero, int report_errors)
{
  struct tms now;

  if (audio.recsock == NULL) {
    audio.recsock = arts_record_stream(audio.rate, 16, 1, "xvoice mic");
    if (audio.recsock == NULL) return -1;
  }
  *time_zero = 10*times(&now);
  return 0;
}

int AudioGetPCM (char *buf, long len, long *ret, int *eof)
{
  int i;
  short header[BLOCK_HEADER_SIZE/sizeof(short)];
  short max;
  short *sp;
  int total;
  int n;
  char *ptr;
  int block_num;

  /*
   * We have to replicate an identical header throughout the
   * input buffer?
   */

  memset (header, '\0', BLOCK_HEADER_SIZE);
  header[HEADER_OFFSET_BYTE_ORDER ] = 0x0102;
  header[HEADER_OFFSET_FORMAT     ] = 2;
  header[HEADER_OFFSET_SAMPLES    ] = audio.buflen / 2;
  header[HEADER_OFFSET_ONE        ] = 1;
  header[HEADER_OFFSET_PCM_LENGTH ] = audio.buflen;
  header[HEADER_OFFSET_DATA_LENGTH] = audio.buflen;

  block_num = 0;
  total = 0;
  ptr = buf;
  while (total + audio.blocklen < len) {
    n = arts_read(audio.recsock, ptr+BLOCK_HEADER_SIZE, audio.buflen);

    if (n < 0) {
      if (ptr > buf) { /* we've read at least one block */
        sp = (short*) (ptr - audio.blocklen);
        ((short*)ptr)[HEADER_OFFSET_END_OF_PCM ] = 1;
      }
      *eof = 1;
      break;
    }

    max = 0;
    sp = (short *)ptr+BLOCK_HEADER_SIZE;
    for (i = 0 ; i < n; i++) {
      if (abs(sp[i]) > max) {
        max = abs(sp[i]);
      }
    }

    memcpy(ptr, header, BLOCK_HEADER_SIZE);
    sp = (short*)ptr;
    sp[HEADER_OFFSET_BLOCK_INDEX] = block_num++;
    sp[HEADER_OFFSET_VOLUME     ] = max;

    total += n;
    ptr += audio.blocklen;
  }

  *ret = total;

  return (0);
}

int AudioStopRecording (void)
{
  if (audio.recsock != NULL) {
    arts_close_stream(audio.recsock);
    audio.recsock = NULL;
  }
  return 0;
}

int AudioStartPlayback (void)
{
  if (audio.playsock == NULL) {
    audio.playsock = arts_play_stream(audio.rate, 16, 1, "xvoice speaker");
    if (audio.playsock == NULL) return -1;
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
    n = arts_write(audio.playsock, ptr+BLOCK_HEADER_SIZE, len);
    if (n < 0 || header[HEADER_OFFSET_END_OF_PCM])
      break;
    ptr += audio.blocklen;
  }
  return 0;
}

int AudioStopPlayback (int flush_output)
{
  if (audio.playsock != NULL) {
    arts_close_stream(audio.playsock);
    audio.playsock = NULL;
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
