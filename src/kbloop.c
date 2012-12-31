/*
 * Copyright (C) 2001,2002 Brian Craft
 * Portions Copyright (c) 1999-2001 Vojtech Pavlik
 *
 * Event char device which allows write access in 2.2 and
 * 2.4 kernels w/o needing usb devices.
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#define MODULE
#define __KERNEL__

#include <linux/autoconf.h>
#if defined(CONFIG_MODVERSIONS) && !defined(MODVERSIONS)
#   define MODVERSIONS
#endif

#ifdef MODVERSIONS
#   include <linux/modversions.h>
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/input.h>
#include <asm/uaccess.h>

#define KBLOOP_MAJOR 0
#define NAME "kbloop"

#define BREAK_FLAG 0x80

#define BUFSIZE 128

static int kbloop_major = KBLOOP_MAJOR;

/*
 * The following code is lifted from keybdev.c, with non-x86
 * code snipped (vv only does x86).
 */ 

static int x86_sysrq_alt = 0;

static unsigned short x86_keycodes[256] =
{ 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
  32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
  48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
  64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
  80, 81, 82, 83, 43, 85, 86, 87, 88,115,119,120,121,375,123, 90,
  284,285,309,298,312, 91,327,328,329,331,333,335,336,337,338,339,
  367,294,293,286,350, 92,334,512,116,377,109,111,373,347,348,349,
  360, 93, 94, 95, 98,376,100,101,357,316,354,304,289,102,351,355,
  103,104,105,275,281,272,306,106,274,107,288,364,358,363,362,361,
  291,108,381,290,287,292,279,305,280, 99,112,257,258,113,270,114,
  118,117,125,374,379,259,260,261,262,263,264,265,266,267,268,269,
  271,273,276,277,278,282,283,295,296,297,299,300,301,302,303,307,
  308,310,313,314,315,317,318,319,320,321,322,323,324,325,326,330,
  332,340,341,342,343,344,345,346,356,359,365,368,369,370,371,372 };

static int emulate_raw(unsigned int keycode, int down)
{
  if (keycode > 255 || !x86_keycodes[keycode])
    return -1; 

  if (keycode == KEY_PAUSE) {
    handle_scancode(0xe1, 1);
    handle_scancode(0x1d, down);
    handle_scancode(0x45, down);
    return 0;
  } 

  if (keycode == KEY_SYSRQ && x86_sysrq_alt) {
    handle_scancode(0x54, down);

    return 0;
  }

  if (x86_keycodes[keycode] & 0x100)
    handle_scancode(0xe0, 1);

  handle_scancode(x86_keycodes[keycode] & 0x7f, down);

  if (keycode == KEY_SYSRQ) {
    handle_scancode(0xe0, 1);
    handle_scancode(0x37, down);
  }

  if (keycode == KEY_LEFTALT || keycode == KEY_RIGHTALT)
    x86_sysrq_alt = down;

  return 0;
}

/*
 * End of code from keybdev.c
 */

/*
 * handle_key() and kbloop_write() are based on code in evdev.c
 * and input.c.
 *
 */

static void handle_key(unsigned int code, int value)
{
  if (code > KEY_MAX) return;

  emulate_raw(code, value);
}

static ssize_t kbloop_write(struct file * file, const char * buffer, 
    size_t count, loff_t *ppos)
{
  struct input_event event;
  int retval = 0;

  while (retval < count) {

    if (copy_from_user(&event, buffer + retval, sizeof(struct input_event)))
      return -EFAULT;

    if (event.type == EV_KEY)
      handle_key(event.code, event.value);
    retval += sizeof(struct input_event);
  }

  return retval;
}

static int kbloop_open (struct inode *inode, struct file *file)
{
  MOD_INC_USE_COUNT;
  return 0;
}

static int kbloop_release (struct inode *inode, struct file *file)
{
  MOD_DEC_USE_COUNT;
  return 0;
}

static struct file_operations kbloop_fops = 
{
    write:      kbloop_write,
    open:       kbloop_open,
    release:    kbloop_release,
};

int init_module(void)
{ 
  int ret;

  printk("<1>kbloop device loaded\n"); 
  SET_MODULE_OWNER(&kbloop_fops);
  ret = register_chrdev(kbloop_major, NAME, &kbloop_fops);
  if (ret < 0) {
    printk(KERN_WARNING "kbloop: can't get major %d\n", kbloop_major);
    return ret;
  }

  if (kbloop_major == 0) kbloop_major = ret;
  return 0; 
}

void cleanup_module(void)
{
  printk("<1>kbloop device unloaded\n");
  unregister_chrdev(kbloop_major, NAME);
}
