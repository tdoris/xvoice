/**
 * keys.h
 *
 * Description: key definitions.
 *
 * Copyright (c) 2000, Brian Craft.
 * See the LICENSE file. All rights not granted therein are reserved.
 *
 * @author Brian Craft
 * $Revision: 1.5 $
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

#ifndef _KEYS_H
#define _KEYS_H

/*
 * These are X keysyms for keys we want to be able to fake. It
 * may seem odd to use keysyms here, since Events are supposed to
 * be independent of X. But we have to choose *some* representation,
 * and may as well choose one that gives us a direct mapping for
 * some target type, e.g. X targets.
 */

/*
 * Latin keysyms
 */
#define KEYS \
    "<!ENTITY BackSpace    \"\\FF08\" >\n"\
    "<!ENTITY Tab      \"\\FF09\" >\n"\
    "<!ENTITY Linefeed    \"\\FF0A\" >\n"\
    "<!ENTITY Clear    \"\\FF0B\" >\n"\
    "<!ENTITY Return    \"\\FF0D\" >\n"\
    "<!ENTITY Pause    \"\\FF13\" >\n"\
    "<!ENTITY Scroll_Lock    \"\\FF14\" >\n"\
    "<!ENTITY Sys_Req    \"\\FF15\" >\n"\
    "<!ENTITY Escape    \"\\FF1B\" >\n"\
    "<!ENTITY Delete    \"\\FFFF\" >\n"\
    "\n"\
    "<!ENTITY Home      \"\\FF50\" >\n"\
    "<!ENTITY Left      \"\\FF51\" >\n"\
    "<!ENTITY Up      \"\\FF52\" >\n"\
    "<!ENTITY Right    \"\\FF53\" >\n"\
    "<!ENTITY Down      \"\\FF54\" >\n"\
    "<!ENTITY Prior    \"\\FF55\" >\n"\
    "<!ENTITY Page_Up    \"\\FF55\" >\n"\
    "<!ENTITY Next      \"\\FF56\" >\n"\
    "<!ENTITY Page_Down    \"\\FF56\" >\n"\
    "<!ENTITY End      \"\\FF57\" >\n"\
    "<!ENTITY Begin    \"\\FF58\" >\n"\
    "\n"\
    "<!ENTITY Select    \"\\FF60\" >\n"\
    "<!ENTITY Print    \"\\FF61\" >\n"\
    "<!ENTITY Execute    \"\\FF62\" >\n"\
    "<!ENTITY Insert    \"\\FF63\" >\n"\
    "<!ENTITY Undo      \"\\FF65\" >\n"\
    "<!ENTITY Redo      \"\\FF66\" >\n"\
    "<!ENTITY Menu      \"\\FF67\" >\n"\
    "<!ENTITY Find      \"\\FF68\" >\n"\
    "<!ENTITY Cancel    \"\\FF69\" >\n"\
    "<!ENTITY Help      \"\\FF6A\" >\n"\
    "<!ENTITY Break    \"\\FF6B\" >\n"\
    "<!ENTITY Mode_switch    \"\\FF7E\" >\n"\
    "<!ENTITY script_switch        \"\\FF7E\" >\n"\
    "<!ENTITY Num_Lock    \"\\FF7F\" >\n"\
    "\n"\
    "<!ENTITY KP_Space    \"\\FF80\" >\n"\
    "<!ENTITY KP_Tab    \"\\FF89\" >\n"\
    "<!ENTITY KP_Enter    \"\\FF8D\" >\n"\
    "<!ENTITY KP_F1    \"\\FF91\" >\n"\
    "<!ENTITY KP_F2    \"\\FF92\" >\n"\
    "<!ENTITY KP_F3    \"\\FF93\" >\n"\
    "<!ENTITY KP_F4    \"\\FF94\" >\n"\
    "<!ENTITY KP_Home    \"\\FF95\" >\n"\
    "<!ENTITY KP_Left    \"\\FF96\" >\n"\
    "<!ENTITY KP_Up    \"\\FF97\" >\n"\
    "<!ENTITY KP_Right    \"\\FF98\" >\n"\
    "<!ENTITY KP_Down    \"\\FF99\" >\n"\
    "<!ENTITY KP_Prior    \"\\FF9A\" >\n"\
    "<!ENTITY KP_Page_Up    \"\\FF9A\" >\n"\
    "<!ENTITY KP_Next    \"\\FF9B\" >\n"\
    "<!ENTITY KP_Page_Down    \"\\FF9B\" >\n"\
    "<!ENTITY KP_End    \"\\FF9C\" >\n"\
    "<!ENTITY KP_Begin    \"\\FF9D\" >\n"\
    "<!ENTITY KP_Insert    \"\\FF9E\" >\n"\
    "<!ENTITY KP_Delete    \"\\FF9F\" >\n"\
    "<!ENTITY KP_Equal    \"\\FFBD\" >\n"\
    "<!ENTITY KP_Multiply    \"\\FFAA\" >\n"\
    "<!ENTITY KP_Add    \"\\FFAB\" >\n"\
    "<!ENTITY KP_Separator    \"\\FFAC\" >\n"\
    "<!ENTITY KP_Subtract    \"\\FFAD\" >\n"\
    "<!ENTITY KP_Decimal    \"\\FFAE\" >\n"\
    "<!ENTITY KP_Divide    \"\\FFAF\" >\n"\
    "\n"\
    "<!ENTITY KP_0      \"\\FFB0\" >\n"\
    "<!ENTITY KP_1      \"\\FFB1\" >\n"\
    "<!ENTITY KP_2      \"\\FFB2\" >\n"\
    "<!ENTITY KP_3      \"\\FFB3\" >\n"\
    "<!ENTITY KP_4      \"\\FFB4\" >\n"\
    "<!ENTITY KP_5      \"\\FFB5\" >\n"\
    "<!ENTITY KP_6      \"\\FFB6\" >\n"\
    "<!ENTITY KP_7      \"\\FFB7\" >\n"\
    "<!ENTITY KP_8      \"\\FFB8\" >\n"\
    "<!ENTITY KP_9      \"\\FFB9\" >\n"\
    "\n"\
    "<!ENTITY F1      \"\\FFBE\" >\n"\
    "<!ENTITY F2      \"\\FFBF\" >\n"\
    "<!ENTITY F3      \"\\FFC0\" >\n"\
    "<!ENTITY F4      \"\\FFC1\" >\n"\
    "<!ENTITY F5      \"\\FFC2\" >\n"\
    "<!ENTITY F6      \"\\FFC3\" >\n"\
    "<!ENTITY F7      \"\\FFC4\" >\n"\
    "<!ENTITY F8      \"\\FFC5\" >\n"\
    "<!ENTITY F9      \"\\FFC6\" >\n"\
    "<!ENTITY F10      \"\\FFC7\" >\n"\
    "<!ENTITY F11      \"\\FFC8\" >\n"\
    "<!ENTITY L1      \"\\FFC8\" >\n"\
    "<!ENTITY F12      \"\\FFC9\" >\n"\
    "<!ENTITY L2      \"\\FFC9\" >\n"\
    "<!ENTITY F13      \"\\FFCA\" >\n"\
    "<!ENTITY L3      \"\\FFCA\" >\n"\
    "<!ENTITY F14      \"\\FFCB\" >\n"\
    "<!ENTITY L4      \"\\FFCB\" >\n"\
    "<!ENTITY F15      \"\\FFCC\" >\n"\
    "<!ENTITY L5      \"\\FFCC\" >\n"\
    "<!ENTITY F16      \"\\FFCD\" >\n"\
    "<!ENTITY L6      \"\\FFCD\" >\n"\
    "<!ENTITY F17      \"\\FFCE\" >\n"\
    "<!ENTITY L7      \"\\FFCE\" >\n"\
    "<!ENTITY F18      \"\\FFCF\" >\n"\
    "<!ENTITY L8      \"\\FFCF\" >\n"\
    "<!ENTITY F19      \"\\FFD0\" >\n"\
    "<!ENTITY L9      \"\\FFD0\" >\n"\
    "<!ENTITY F20      \"\\FFD1\" >\n"\
    "<!ENTITY L10      \"\\FFD1\" >\n"\
    "<!ENTITY F21      \"\\FFD2\" >\n"\
    "<!ENTITY R1      \"\\FFD2\" >\n"\
    "<!ENTITY F22      \"\\FFD3\" >\n"\
    "<!ENTITY R2      \"\\FFD3\" >\n"\
    "<!ENTITY F23      \"\\FFD4\" >\n"\
    "<!ENTITY R3      \"\\FFD4\" >\n"\
    "<!ENTITY F24      \"\\FFD5\" >\n"\
    "<!ENTITY R4      \"\\FFD5\" >\n"\
    "<!ENTITY F25      \"\\FFD6\" >\n"\
    "<!ENTITY R5      \"\\FFD6\" >\n"\
    "<!ENTITY F26      \"\\FFD7\" >\n"\
    "<!ENTITY R6      \"\\FFD7\" >\n"\
    "<!ENTITY F27      \"\\FFD8\" >\n"\
    "<!ENTITY R7      \"\\FFD8\" >\n"\
    "<!ENTITY F28      \"\\FFD9\" >\n"\
    "<!ENTITY R8      \"\\FFD9\" >\n"\
    "<!ENTITY F29      \"\\FFDA\" >\n"\
    "<!ENTITY R9      \"\\FFDA\" >\n"\
    "<!ENTITY F30      \"\\FFDB\" >\n"\
    "<!ENTITY R10      \"\\FFDB\" >\n"\
    "<!ENTITY F31      \"\\FFDC\" >\n"\
    "<!ENTITY R11      \"\\FFDC\" >\n"\
    "<!ENTITY F32      \"\\FFDD\" >\n"\
    "<!ENTITY R12      \"\\FFDD\" >\n"\
    "<!ENTITY F33      \"\\FFDE\" >\n"\
    "<!ENTITY R13      \"\\FFDE\" >\n"\
    "<!ENTITY F34      \"\\FFDF\" >\n"\
    "<!ENTITY R14      \"\\FFDF\" >\n"\
    "<!ENTITY F35      \"\\FFE0\" >\n"\
    "<!ENTITY R15      \"\\FFE0\" >\n"\
    "\n"\
    "<!ENTITY Shift_L    \"\\FFE1\" >\n"\
    "<!ENTITY Shift_R    \"\\FFE2\" >\n"\
    "<!ENTITY Control_L    \"\\FFE3\" >\n"\
    "<!ENTITY Control_R    \"\\FFE4\" >\n"\
    "<!ENTITY Caps_Lock    \"\\FFE5\" >\n"\
    "<!ENTITY Shift_Lock    \"\\FFE6\" >\n"\
    "\n"\
    "<!ENTITY Meta_L    \"\\FFE7\" >\n"\
    "<!ENTITY Meta_R    \"\\FFE8\" >\n"\
    "<!ENTITY Alt_L    \"\\FFE9\" >\n"\
    "<!ENTITY Alt_R    \"\\FFEA\" >\n"\
    "<!ENTITY Super_L    \"\\FFEB\" >\n"\
    "<!ENTITY Super_R    \"\\FFEC\" >\n"\
    "<!ENTITY Hyper_L    \"\\FFED\" >\n"\
    "<!ENTITY Hyper_R    \"\\FFEE\" >\n"\
    "\n"\
    "<!ENTITY  dead_grave          \"\\FE50\" >\n"\
    "<!ENTITY  dead_acute          \"\\FE51\" >\n"\
    "<!ENTITY  dead_circumflex        \"\\FE52\" >\n"\
    "<!ENTITY  dead_tilde          \"\\FE53\" >\n"\
    "<!ENTITY  dead_macron          \"\\FE54\" >\n"\
    "<!ENTITY  dead_breve          \"\\FE55\" >\n"\
    "<!ENTITY  dead_abovedot        \"\\FE56\" >\n"\
    "<!ENTITY  dead_diaeresis        \"\\FE57\" >\n"\
    "<!ENTITY  dead_abovering        \"\\FE58\" >\n"\
    "<!ENTITY  dead_doubleacute        \"\\FE59\" >\n"\
    "<!ENTITY  dead_caron          \"\\FE5A\" >\n"\
    "<!ENTITY  dead_cedilla          \"\\FE5B\" >\n"\
    "<!ENTITY  dead_ogonek          \"\\FE5C\" >\n"\
    "<!ENTITY  dead_iota          \"\\FE5D\" >\n"\
    "<!ENTITY  dead_voiced_sound        \"\\FE5E\" >\n"\
    "<!ENTITY  dead_semivoiced_sound      \"\\FE5F\" >\n"\
    "<!ENTITY  dead_belowdot        \"\\FE60\" >\n"\
    "\n"\
    "<!ENTITY Aogonek             \"\\1a1\" >\n"\
    "<!ENTITY breve               \"\\1a2\" >\n"\
    "<!ENTITY Lstroke             \"\\1a3\" >\n"\
    "<!ENTITY Lcaron              \"\\1a5\" >\n"\
    "<!ENTITY Sacute              \"\\1a6\" >\n"\
    "<!ENTITY Scaron              \"\\1a9\" >\n"\
    "<!ENTITY Scedilla            \"\\1aa\" >\n"\
    "<!ENTITY Tcaron              \"\\1ab\" >\n"\
    "<!ENTITY Zacute              \"\\1ac\" >\n"\
    "<!ENTITY Zcaron              \"\\1ae\" >\n"\
    "<!ENTITY Zabovedot           \"\\1af\" >\n"\
    "<!ENTITY aogonek             \"\\1b1\" >\n"\
    "<!ENTITY ogonek              \"\\1b2\" >\n"\
    "<!ENTITY lstroke             \"\\1b3\" >\n"\
    "<!ENTITY lcaron              \"\\1b5\" >\n"\
    "<!ENTITY sacute              \"\\1b6\" >\n"\
    "<!ENTITY caron               \"\\1b7\" >\n"\
    "<!ENTITY scaron              \"\\1b9\" >\n"\
    "<!ENTITY scedilla            \"\\1ba\" >\n"\
    "<!ENTITY tcaron              \"\\1bb\" >\n"\
    "<!ENTITY zacute              \"\\1bc\" >\n"\
    "<!ENTITY doubleacute         \"\\1bd\" >\n"\
    "<!ENTITY zcaron              \"\\1be\" >\n"\
    "<!ENTITY zabovedot           \"\\1bf\" >\n"\
    "<!ENTITY Racute              \"\\1c0\" >\n"\
    "<!ENTITY Abreve              \"\\1c3\" >\n"\
    "<!ENTITY Lacute              \"\\1c5\" >\n"\
    "<!ENTITY Cacute              \"\\1c6\" >\n"\
    "<!ENTITY Ccaron              \"\\1c8\" >\n"\
    "<!ENTITY Eogonek             \"\\1ca\" >\n"\
    "<!ENTITY Ecaron              \"\\1cc\" >\n"\
    "<!ENTITY Dcaron              \"\\1cf\" >\n"\
    "<!ENTITY Dstroke             \"\\1d0\" >\n"\
    "<!ENTITY Nacute              \"\\1d1\" >\n"\
    "<!ENTITY Ncaron              \"\\1d2\" >\n"\
    "<!ENTITY Odoubleacute        \"\\1d5\" >\n"\
    "<!ENTITY Rcaron              \"\\1d8\" >\n"\
    "<!ENTITY Uring               \"\\1d9\" >\n"\
    "<!ENTITY Udoubleacute        \"\\1db\" >\n"\
    "<!ENTITY Tcedilla            \"\\1de\" >\n"\
    "<!ENTITY racute              \"\\1e0\" >\n"\
    "<!ENTITY abreve              \"\\1e3\" >\n"\
    "<!ENTITY lacute              \"\\1e5\" >\n"\
    "<!ENTITY cacute              \"\\1e6\" >\n"\
    "<!ENTITY ccaron              \"\\1e8\" >\n"\
    "<!ENTITY eogonek             \"\\1ea\" >\n"\
    "<!ENTITY ecaron              \"\\1ec\" >\n"\
    "<!ENTITY dcaron              \"\\1ef\" >\n"\
    "<!ENTITY dstroke             \"\\1f0\" >\n"\
    "<!ENTITY nacute              \"\\1f1\" >\n"\
    "<!ENTITY ncaron              \"\\1f2\" >\n"\
    "<!ENTITY odoubleacute        \"\\1f5\" >\n"\
    "<!ENTITY udoubleacute        \"\\1fb\" >\n"\
    "<!ENTITY rcaron              \"\\1f8\" >\n"\
    "<!ENTITY uring               \"\\1f9\" >\n"\
    "<!ENTITY tcedilla            \"\\1fe\" >\n"\
    "<!ENTITY abovedot            \"\\1ff\" >\n"\
    "\n"\
    "<!ENTITY Hstroke             \"\\2a1\" >\n"\
    "<!ENTITY Hcircumflex         \"\\2a6\" >\n"\
    "<!ENTITY Iabovedot           \"\\2a9\" >\n"\
    "<!ENTITY Gbreve              \"\\2ab\" >\n"\
    "<!ENTITY Jcircumflex         \"\\2ac\" >\n"\
    "<!ENTITY hstroke             \"\\2b1\" >\n"\
    "<!ENTITY hcircumflex         \"\\2b6\" >\n"\
    "<!ENTITY idotless            \"\\2b9\" >\n"\
    "<!ENTITY gbreve              \"\\2bb\" >\n"\
    "<!ENTITY jcircumflex         \"\\2bc\" >\n"\
    "<!ENTITY Cabovedot           \"\\2c5\" >\n"\
    "<!ENTITY Ccircumflex         \"\\2c6\" >\n"\
    "<!ENTITY Gabovedot           \"\\2d5\" >\n"\
    "<!ENTITY Gcircumflex         \"\\2d8\" >\n"\
    "<!ENTITY Ubreve              \"\\2dd\" >\n"\
    "<!ENTITY Scircumflex         \"\\2de\" >\n"\
    "<!ENTITY cabovedot           \"\\2e5\" >\n"\
    "<!ENTITY ccircumflex         \"\\2e6\" >\n"\
    "<!ENTITY gabovedot           \"\\2f5\" >\n"\
    "<!ENTITY gcircumflex         \"\\2f8\" >\n"\
    "<!ENTITY ubreve              \"\\2fd\" >\n"\
    "<!ENTITY scircumflex         \"\\2fe\" >\n"\
    "\n"\
    "<!ENTITY kra                 \"\\3a2\" >\n"\
    "<!ENTITY kappa               \"\\3a2\" >\n"\
    "<!ENTITY Rcedilla            \"\\3a3\" >\n"\
    "<!ENTITY Itilde              \"\\3a5\" >\n"\
    "<!ENTITY Lcedilla            \"\\3a6\" >\n"\
    "<!ENTITY Emacron             \"\\3aa\" >\n"\
    "<!ENTITY Gcedilla            \"\\3ab\" >\n"\
    "<!ENTITY Tslash              \"\\3ac\" >\n"\
    "<!ENTITY rcedilla            \"\\3b3\" >\n"\
    "<!ENTITY itilde              \"\\3b5\" >\n"\
    "<!ENTITY lcedilla            \"\\3b6\" >\n"\
    "<!ENTITY emacron             \"\\3ba\" >\n"\
    "<!ENTITY gcedilla            \"\\3bb\" >\n"\
    "<!ENTITY tslash              \"\\3bc\" >\n"\
    "<!ENTITY ENG                 \"\\3bd\" >\n"\
    "<!ENTITY eng                 \"\\3bf\" >\n"\
    "<!ENTITY Amacron             \"\\3c0\" >\n"\
    "<!ENTITY Iogonek             \"\\3c7\" >\n"\
    "<!ENTITY Eabovedot           \"\\3cc\" >\n"\
    "<!ENTITY Imacron             \"\\3cf\" >\n"\
    "<!ENTITY Ncedilla            \"\\3d1\" >\n"\
    "<!ENTITY Omacron             \"\\3d2\" >\n"\
    "<!ENTITY Kcedilla            \"\\3d3\" >\n"\
    "<!ENTITY Uogonek             \"\\3d9\" >\n"\
    "<!ENTITY Utilde              \"\\3dd\" >\n"\
    "<!ENTITY Umacron             \"\\3de\" >\n"\
    "<!ENTITY amacron             \"\\3e0\" >\n"\
    "<!ENTITY iogonek             \"\\3e7\" >\n"\
    "<!ENTITY eabovedot           \"\\3ec\" >\n"\
    "<!ENTITY imacron             \"\\3ef\" >\n"\
    "<!ENTITY ncedilla            \"\\3f1\" >\n"\
    "<!ENTITY omacron             \"\\3f2\" >\n"\
    "<!ENTITY kcedilla            \"\\3f3\" >\n"\
    "<!ENTITY uogonek             \"\\3f9\" >\n"\
    "<!ENTITY utilde              \"\\3fd\" >\n"\
    "<!ENTITY umacron             \"\\3fe\" >\n"\
    "\n"\
    "<!ENTITY OE                  \"\\13bc\" >\n"\
    "<!ENTITY oe                  \"\\13bd\" >\n"\
    "<!ENTITY Ydiaeresis          \"\\13be\" >\n"

#endif /* _KEYS_H */
