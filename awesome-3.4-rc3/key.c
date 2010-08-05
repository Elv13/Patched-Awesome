/*
 * key.c - Key bindings configuration management
 *
 * Copyright © 2008-2009 Julien Danjou <julien@danjou.info>
 * Copyright © 2008 Pierre Habouzit <madcoder@debian.org>
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

/* XStringToKeysym() and XKeysymToString */
#include <X11/Xlib.h>

/* XCB doesn't provide keysyms definition */
#include <X11/keysym.h>
#include <X11/XF86keysym.h>

#include "globalconf.h"
#include "luaa.h"
#include "common/xutil.h"
#include "common/luaobject.h"

/** XCB equivalent of XLookupString which translate the keycode given
 * by PressEvent to a KeySym and a string
 * \todo use XKB!
 */
static unsigned short const
keysym_to_unicode_1a1_1ff[] =
{
            0x0104, 0x02d8, 0x0141, 0x0000, 0x013d, 0x015a, 0x0000, /* 0x01a0-0x01a7 */
    0x0000, 0x0160, 0x015e, 0x0164, 0x0179, 0x0000, 0x017d, 0x017b, /* 0x01a8-0x01af */
    0x0000, 0x0105, 0x02db, 0x0142, 0x0000, 0x013e, 0x015b, 0x02c7, /* 0x01b0-0x01b7 */
    0x0000, 0x0161, 0x015f, 0x0165, 0x017a, 0x02dd, 0x017e, 0x017c, /* 0x01b8-0x01bf */
    0x0154, 0x0000, 0x0000, 0x0102, 0x0000, 0x0139, 0x0106, 0x0000, /* 0x01c0-0x01c7 */
    0x010c, 0x0000, 0x0118, 0x0000, 0x011a, 0x0000, 0x0000, 0x010e, /* 0x01c8-0x01cf */
    0x0110, 0x0143, 0x0147, 0x0000, 0x0000, 0x0150, 0x0000, 0x0000, /* 0x01d0-0x01d7 */
    0x0158, 0x016e, 0x0000, 0x0170, 0x0000, 0x0000, 0x0162, 0x0000, /* 0x01d8-0x01df */
    0x0155, 0x0000, 0x0000, 0x0103, 0x0000, 0x013a, 0x0107, 0x0000, /* 0x01e0-0x01e7 */
    0x010d, 0x0000, 0x0119, 0x0000, 0x011b, 0x0000, 0x0000, 0x010f, /* 0x01e8-0x01ef */
    0x0111, 0x0144, 0x0148, 0x0000, 0x0000, 0x0151, 0x0000, 0x0000, /* 0x01f0-0x01f7 */
    0x0159, 0x016f, 0x0000, 0x0171, 0x0000, 0x0000, 0x0163, 0x02d9  /* 0x01f8-0x01ff */
};

static unsigned short const
keysym_to_unicode_2a1_2fe[] =
{
            0x0126, 0x0000, 0x0000, 0x0000, 0x0000, 0x0124, 0x0000, /* 0x02a0-0x02a7 */
    0x0000, 0x0130, 0x0000, 0x011e, 0x0134, 0x0000, 0x0000, 0x0000, /* 0x02a8-0x02af */
    0x0000, 0x0127, 0x0000, 0x0000, 0x0000, 0x0000, 0x0125, 0x0000, /* 0x02b0-0x02b7 */
    0x0000, 0x0131, 0x0000, 0x011f, 0x0135, 0x0000, 0x0000, 0x0000, /* 0x02b8-0x02bf */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x010a, 0x0108, 0x0000, /* 0x02c0-0x02c7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x02c8-0x02cf */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0120, 0x0000, 0x0000, /* 0x02d0-0x02d7 */
    0x011c, 0x0000, 0x0000, 0x0000, 0x0000, 0x016c, 0x015c, 0x0000, /* 0x02d8-0x02df */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x010b, 0x0109, 0x0000, /* 0x02e0-0x02e7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x02e8-0x02ef */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0121, 0x0000, 0x0000, /* 0x02f0-0x02f7 */
    0x011d, 0x0000, 0x0000, 0x0000, 0x0000, 0x016d, 0x015d          /* 0x02f8-0x02ff */
};

static unsigned short const
keysym_to_unicode_3a2_3fe[] =
{
                    0x0138, 0x0156, 0x0000, 0x0128, 0x013b, 0x0000, /* 0x03a0-0x03a7 */
    0x0000, 0x0000, 0x0112, 0x0122, 0x0166, 0x0000, 0x0000, 0x0000, /* 0x03a8-0x03af */
    0x0000, 0x0000, 0x0000, 0x0157, 0x0000, 0x0129, 0x013c, 0x0000, /* 0x03b0-0x03b7 */
    0x0000, 0x0000, 0x0113, 0x0123, 0x0167, 0x014a, 0x0000, 0x014b, /* 0x03b8-0x03bf */
    0x0100, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x012e, /* 0x03c0-0x03c7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0116, 0x0000, 0x0000, 0x012a, /* 0x03c8-0x03cf */
    0x0000, 0x0145, 0x014c, 0x0136, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x03d0-0x03d7 */
    0x0000, 0x0172, 0x0000, 0x0000, 0x0000, 0x0168, 0x016a, 0x0000, /* 0x03d8-0x03df */
    0x0101, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x012f, /* 0x03e0-0x03e7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0117, 0x0000, 0x0000, 0x012b, /* 0x03e8-0x03ef */
    0x0000, 0x0146, 0x014d, 0x0137, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x03f0-0x03f7 */
    0x0000, 0x0173, 0x0000, 0x0000, 0x0000, 0x0169, 0x016b          /* 0x03f8-0x03ff */
};

static unsigned short const
keysym_to_unicode_4a1_4df[] =
{
            0x3002, 0x3008, 0x3009, 0x3001, 0x30fb, 0x30f2, 0x30a1, /* 0x04a0-0x04a7 */
    0x30a3, 0x30a5, 0x30a7, 0x30a9, 0x30e3, 0x30e5, 0x30e7, 0x30c3, /* 0x04a8-0x04af */
    0x30fc, 0x30a2, 0x30a4, 0x30a6, 0x30a8, 0x30aa, 0x30ab, 0x30ad, /* 0x04b0-0x04b7 */
    0x30af, 0x30b1, 0x30b3, 0x30b5, 0x30b7, 0x30b9, 0x30bb, 0x30bd, /* 0x04b8-0x04bf */
    0x30bf, 0x30c1, 0x30c4, 0x30c6, 0x30c8, 0x30ca, 0x30cb, 0x30cc, /* 0x04c0-0x04c7 */
    0x30cd, 0x30ce, 0x30cf, 0x30d2, 0x30d5, 0x30d8, 0x30db, 0x30de, /* 0x04c8-0x04cf */
    0x30df, 0x30e0, 0x30e1, 0x30e2, 0x30e4, 0x30e6, 0x30e8, 0x30e9, /* 0x04d0-0x04d7 */
    0x30ea, 0x30eb, 0x30ec, 0x30ed, 0x30ef, 0x30f3, 0x309b, 0x309c  /* 0x04d8-0x04df */
};

static unsigned short const
keysym_to_unicode_590_5fe[] =
{
    0x06f0, 0x06f1, 0x06f2, 0x06f3, 0x06f4, 0x06f5, 0x06f6, 0x06f7, /* 0x0590-0x0597 */
    0x06f8, 0x06f9, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x0598-0x059f */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x066a, 0x0670, 0x0679, /* 0x05a0-0x05a7 */

    0x067e, 0x0686, 0x0688, 0x0691, 0x060c, 0x0000, 0x06d4, 0x0000, /* 0x05ac-0x05af */
    0x0660, 0x0661, 0x0662, 0x0663, 0x0664, 0x0665, 0x0666, 0x0667, /* 0x05b0-0x05b7 */
    0x0668, 0x0669, 0x0000, 0x061b, 0x0000, 0x0000, 0x0000, 0x061f, /* 0x05b8-0x05bf */
    0x0000, 0x0621, 0x0622, 0x0623, 0x0624, 0x0625, 0x0626, 0x0627, /* 0x05c0-0x05c7 */
    0x0628, 0x0629, 0x062a, 0x062b, 0x062c, 0x062d, 0x062e, 0x062f, /* 0x05c8-0x05cf */
    0x0630, 0x0631, 0x0632, 0x0633, 0x0634, 0x0635, 0x0636, 0x0637, /* 0x05d0-0x05d7 */
    0x0638, 0x0639, 0x063a, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x05d8-0x05df */
    0x0640, 0x0641, 0x0642, 0x0643, 0x0644, 0x0645, 0x0646, 0x0647, /* 0x05e0-0x05e7 */
    0x0648, 0x0649, 0x064a, 0x064b, 0x064c, 0x064d, 0x064e, 0x064f, /* 0x05e8-0x05ef */
    0x0650, 0x0651, 0x0652, 0x0653, 0x0654, 0x0655, 0x0698, 0x06a4, /* 0x05f0-0x05f7 */
    0x06a9, 0x06af, 0x06ba, 0x06be, 0x06cc, 0x06d2, 0x06c1          /* 0x05f8-0x05fe */
};

static unsigned short const
keysym_to_unicode_680_6ff[] =
{
    0x0492, 0x0496, 0x049a, 0x049c, 0x04a2, 0x04ae, 0x04b0, 0x04b2, /* 0x0680-0x0687 */
    0x04b6, 0x04b8, 0x04ba, 0x0000, 0x04d8, 0x04e2, 0x04e8, 0x04ee, /* 0x0688-0x068f */
    0x0493, 0x0497, 0x049b, 0x049d, 0x04a3, 0x04af, 0x04b1, 0x04b3, /* 0x0690-0x0697 */
    0x04b7, 0x04b9, 0x04bb, 0x0000, 0x04d9, 0x04e3, 0x04e9, 0x04ef, /* 0x0698-0x069f */
    0x0000, 0x0452, 0x0453, 0x0451, 0x0454, 0x0455, 0x0456, 0x0457, /* 0x06a0-0x06a7 */
    0x0458, 0x0459, 0x045a, 0x045b, 0x045c, 0x0491, 0x045e, 0x045f, /* 0x06a8-0x06af */
    0x2116, 0x0402, 0x0403, 0x0401, 0x0404, 0x0405, 0x0406, 0x0407, /* 0x06b0-0x06b7 */
    0x0408, 0x0409, 0x040a, 0x040b, 0x040c, 0x0490, 0x040e, 0x040f, /* 0x06b8-0x06bf */
    0x044e, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433, /* 0x06c0-0x06c7 */
    0x0445, 0x0438, 0x0439, 0x043a, 0x043b, 0x043c, 0x043d, 0x043e, /* 0x06c8-0x06cf */
    0x043f, 0x044f, 0x0440, 0x0441, 0x0442, 0x0443, 0x0436, 0x0432, /* 0x06d0-0x06d7 */
    0x044c, 0x044b, 0x0437, 0x0448, 0x044d, 0x0449, 0x0447, 0x044a, /* 0x06d8-0x06df */
    0x042e, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413, /* 0x06e0-0x06e7 */
    0x0425, 0x0418, 0x0419, 0x041a, 0x041b, 0x041c, 0x041d, 0x041e, /* 0x06e8-0x06ef */
    0x041f, 0x042f, 0x0420, 0x0421, 0x0422, 0x0423, 0x0416, 0x0412, /* 0x06f0-0x06f7 */
    0x042c, 0x042b, 0x0417, 0x0428, 0x042d, 0x0429, 0x0427, 0x042a  /* 0x06f8-0x06ff */
};

static unsigned short const
keysym_to_unicode_7a1_7f9[] =
{
            0x0386, 0x0388, 0x0389, 0x038a, 0x03aa, 0x0000, 0x038c, /* 0x07a0-0x07a7 */
    0x038e, 0x03ab, 0x0000, 0x038f, 0x0000, 0x0000, 0x0385, 0x2015, /* 0x07a8-0x07af */
    0x0000, 0x03ac, 0x03ad, 0x03ae, 0x03af, 0x03ca, 0x0390, 0x03cc, /* 0x07b0-0x07b7 */
    0x03cd, 0x03cb, 0x03b0, 0x03ce, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x07b8-0x07bf */
    0x0000, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397, /* 0x07c0-0x07c7 */
    0x0398, 0x0399, 0x039a, 0x039b, 0x039c, 0x039d, 0x039e, 0x039f, /* 0x07c8-0x07cf */
    0x03a0, 0x03a1, 0x03a3, 0x0000, 0x03a4, 0x03a5, 0x03a6, 0x03a7, /* 0x07d0-0x07d7 */
    0x03a8, 0x03a9, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x07d8-0x07df */
    0x0000, 0x03b1, 0x03b2, 0x03b3, 0x03b4, 0x03b5, 0x03b6, 0x03b7, /* 0x07e0-0x07e7 */
    0x03b8, 0x03b9, 0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03be, 0x03bf, /* 0x07e8-0x07ef */
    0x03c0, 0x03c1, 0x03c3, 0x03c2, 0x03c4, 0x03c5, 0x03c6, 0x03c7, /* 0x07f0-0x07f7 */
    0x03c8, 0x03c9                                                  /* 0x07f8-0x07ff */
};

static unsigned short const
keysym_to_unicode_8a4_8fe[] =
{
                                    0x2320, 0x2321, 0x0000, 0x231c, /* 0x08a0-0x08a7 */
    0x231d, 0x231e, 0x231f, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x08a8-0x08af */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x08b0-0x08b7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x2264, 0x2260, 0x2265, 0x222b, /* 0x08b8-0x08bf */
    0x2234, 0x0000, 0x221e, 0x0000, 0x0000, 0x2207, 0x0000, 0x0000, /* 0x08c0-0x08c7 */
    0x2245, 0x2246, 0x0000, 0x0000, 0x0000, 0x0000, 0x22a2, 0x0000, /* 0x08c8-0x08cf */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x221a, 0x0000, /* 0x08d0-0x08d7 */
    0x0000, 0x0000, 0x2282, 0x2283, 0x2229, 0x222a, 0x2227, 0x2228, /* 0x08d8-0x08df */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x08e0-0x08e7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x08e8-0x08ef */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0192, 0x0000, /* 0x08f0-0x08f7 */
    0x0000, 0x0000, 0x0000, 0x2190, 0x2191, 0x2192, 0x2193          /* 0x08f8-0x08ff */
};

static unsigned short const
keysym_to_unicode_9df_9f8[] =
{
                                                            0x2422, /* 0x09d8-0x09df */
    0x2666, 0x25a6, 0x2409, 0x240c, 0x240d, 0x240a, 0x0000, 0x0000, /* 0x09e0-0x09e7 */
    0x240a, 0x240b, 0x2518, 0x2510, 0x250c, 0x2514, 0x253c, 0x2500, /* 0x09e8-0x09ef */
    0x0000, 0x0000, 0x0000, 0x0000, 0x251c, 0x2524, 0x2534, 0x252c, /* 0x09f0-0x09f7 */
    0x2502                                                          /* 0x09f8-0x09ff */
};

static unsigned short const
keysym_to_unicode_aa1_afe[] =
{
            0x2003, 0x2002, 0x2004, 0x2005, 0x2007, 0x2008, 0x2009, /* 0x0aa0-0x0aa7 */
    0x200a, 0x2014, 0x2013, 0x0000, 0x0000, 0x0000, 0x2026, 0x2025, /* 0x0aa8-0x0aaf */
    0x2153, 0x2154, 0x2155, 0x2156, 0x2157, 0x2158, 0x2159, 0x215a, /* 0x0ab0-0x0ab7 */
    0x2105, 0x0000, 0x0000, 0x2012, 0x2039, 0x2024, 0x203a, 0x0000, /* 0x0ab8-0x0abf */
    0x0000, 0x0000, 0x0000, 0x215b, 0x215c, 0x215d, 0x215e, 0x0000, /* 0x0ac0-0x0ac7 */
    0x0000, 0x2122, 0x2120, 0x0000, 0x25c1, 0x25b7, 0x25cb, 0x25ad, /* 0x0ac8-0x0acf */
    0x2018, 0x2019, 0x201c, 0x201d, 0x211e, 0x0000, 0x2032, 0x2033, /* 0x0ad0-0x0ad7 */
    0x0000, 0x271d, 0x0000, 0x220e, 0x25c2, 0x2023, 0x25cf, 0x25ac, /* 0x0ad8-0x0adf */
    0x25e6, 0x25ab, 0x25ae, 0x25b5, 0x25bf, 0x2606, 0x2022, 0x25aa, /* 0x0ae0-0x0ae7 */
    0x25b4, 0x25be, 0x261a, 0x261b, 0x2663, 0x2666, 0x2665, 0x0000, /* 0x0ae8-0x0aef */
    0x2720, 0x2020, 0x2021, 0x2713, 0x2612, 0x266f, 0x266d, 0x2642, /* 0x0af0-0x0af7 */
    0x2640, 0x2121, 0x2315, 0x2117, 0x2038, 0x201a, 0x201e          /* 0x0af8-0x0aff */
};

/* none of the APL keysyms match the Unicode characters */
static unsigned short const
keysym_to_unicode_cdf_cfa[] =
{
                                                            0x2017, /* 0x0cd8-0x0cdf */
    0x05d0, 0x05d1, 0x05d2, 0x05d3, 0x05d4, 0x05d5, 0x05d6, 0x05d7, /* 0x0ce0-0x0ce7 */
    0x05d8, 0x05d9, 0x05da, 0x05db, 0x05dc, 0x05dd, 0x05de, 0x05df, /* 0x0ce8-0x0cef */
    0x05e0, 0x05e1, 0x05e2, 0x05e3, 0x05e4, 0x05e5, 0x05e6, 0x05e7, /* 0x0cf0-0x0cf7 */
    0x05e8, 0x05e9, 0x05ea                                          /* 0x0cf8-0x0cff */
};

static unsigned short const
keysym_to_unicode_da1_df9[] =
{
            0x0e01, 0x0e02, 0x0e03, 0x0e04, 0x0e05, 0x0e06, 0x0e07, /* 0x0da0-0x0da7 */
    0x0e08, 0x0e09, 0x0e0a, 0x0e0b, 0x0e0c, 0x0e0d, 0x0e0e, 0x0e0f, /* 0x0da8-0x0daf */
    0x0e10, 0x0e11, 0x0e12, 0x0e13, 0x0e14, 0x0e15, 0x0e16, 0x0e17, /* 0x0db0-0x0db7 */
    0x0e18, 0x0e19, 0x0e1a, 0x0e1b, 0x0e1c, 0x0e1d, 0x0e1e, 0x0e1f, /* 0x0db8-0x0dbf */
    0x0e20, 0x0e21, 0x0e22, 0x0e23, 0x0e24, 0x0e25, 0x0e26, 0x0e27, /* 0x0dc0-0x0dc7 */
    0x0e28, 0x0e29, 0x0e2a, 0x0e2b, 0x0e2c, 0x0e2d, 0x0e2e, 0x0e2f, /* 0x0dc8-0x0dcf */
    0x0e30, 0x0e31, 0x0e32, 0x0e33, 0x0e34, 0x0e35, 0x0e36, 0x0e37, /* 0x0dd0-0x0dd7 */
    0x0e38, 0x0e39, 0x0e3a, 0x0000, 0x0000, 0x0000, 0x0e3e, 0x0e3f, /* 0x0dd8-0x0ddf */
    0x0e40, 0x0e41, 0x0e42, 0x0e43, 0x0e44, 0x0e45, 0x0e46, 0x0e47, /* 0x0de0-0x0de7 */
    0x0e48, 0x0e49, 0x0e4a, 0x0e4b, 0x0e4c, 0x0e4d, 0x0000, 0x0000, /* 0x0de8-0x0def */
    0x0e50, 0x0e51, 0x0e52, 0x0e53, 0x0e54, 0x0e55, 0x0e56, 0x0e57, /* 0x0df0-0x0df7 */
    0x0e58, 0x0e59                                                  /* 0x0df8-0x0dff */
};

static unsigned short const
keysym_to_unicode_ea0_eff[] =
{
    0x0000, 0x1101, 0x1101, 0x11aa, 0x1102, 0x11ac, 0x11ad, 0x1103, /* 0x0ea0-0x0ea7 */
    0x1104, 0x1105, 0x11b0, 0x11b1, 0x11b2, 0x11b3, 0x11b4, 0x11b5, /* 0x0ea8-0x0eaf */
    0x11b6, 0x1106, 0x1107, 0x1108, 0x11b9, 0x1109, 0x110a, 0x110b, /* 0x0eb0-0x0eb7 */
    0x110c, 0x110d, 0x110e, 0x110f, 0x1110, 0x1111, 0x1112, 0x1161, /* 0x0eb8-0x0ebf */
    0x1162, 0x1163, 0x1164, 0x1165, 0x1166, 0x1167, 0x1168, 0x1169, /* 0x0ec0-0x0ec7 */
    0x116a, 0x116b, 0x116c, 0x116d, 0x116e, 0x116f, 0x1170, 0x1171, /* 0x0ec8-0x0ecf */
    0x1172, 0x1173, 0x1174, 0x1175, 0x11a8, 0x11a9, 0x11aa, 0x11ab, /* 0x0ed0-0x0ed7 */
    0x11ac, 0x11ad, 0x11ae, 0x11af, 0x11b0, 0x11b1, 0x11b2, 0x11b3, /* 0x0ed8-0x0edf */
    0x11b4, 0x11b5, 0x11b6, 0x11b7, 0x11b8, 0x11b9, 0x11ba, 0x11bb, /* 0x0ee0-0x0ee7 */
    0x11bc, 0x11bd, 0x11be, 0x11bf, 0x11c0, 0x11c1, 0x11c2, 0x0000, /* 0x0ee8-0x0eef */
    0x0000, 0x0000, 0x1140, 0x0000, 0x0000, 0x1159, 0x119e, 0x0000, /* 0x0ef0-0x0ef7 */
    0x11eb, 0x0000, 0x11f9, 0x0000, 0x0000, 0x0000, 0x0000, 0x20a9, /* 0x0ef8-0x0eff */
};

static unsigned short const
keysym_to_unicode_12a1_12fe[] =
{
            0x1e02, 0x1e03, 0x0000, 0x0000, 0x0000, 0x1e0a, 0x0000, /* 0x12a0-0x12a7 */
    0x1e80, 0x0000, 0x1e82, 0x1e0b, 0x1ef2, 0x0000, 0x0000, 0x0000, /* 0x12a8-0x12af */
    0x1e1e, 0x1e1f, 0x0000, 0x0000, 0x1e40, 0x1e41, 0x0000, 0x1e56, /* 0x12b0-0x12b7 */
    0x1e81, 0x1e57, 0x1e83, 0x1e60, 0x1ef3, 0x1e84, 0x1e85, 0x1e61, /* 0x12b8-0x12bf */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x12c0-0x12c7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x12c8-0x12cf */
    0x0174, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1e6a, /* 0x12d0-0x12d7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0176, 0x0000, /* 0x12d8-0x12df */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x12e0-0x12e7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x12e8-0x12ef */
    0x0175, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1e6b, /* 0x12f0-0x12f7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0177          /* 0x12f0-0x12ff */
};

static unsigned short const
keysym_to_unicode_13bc_13be[] =
{
                                    0x0152, 0x0153, 0x0178          /* 0x13b8-0x13bf */
};

static unsigned short const
keysym_to_unicode_14a1_14ff[] =
{
            0x2741, 0x00a7, 0x0589, 0x0029, 0x0028, 0x00bb, 0x00ab, /* 0x14a0-0x14a7 */
    0x2014, 0x002e, 0x055d, 0x002c, 0x2013, 0x058a, 0x2026, 0x055c, /* 0x14a8-0x14af */
    0x055b, 0x055e, 0x0531, 0x0561, 0x0532, 0x0562, 0x0533, 0x0563, /* 0x14b0-0x14b7 */
    0x0534, 0x0564, 0x0535, 0x0565, 0x0536, 0x0566, 0x0537, 0x0567, /* 0x14b8-0x14bf */
    0x0538, 0x0568, 0x0539, 0x0569, 0x053a, 0x056a, 0x053b, 0x056b, /* 0x14c0-0x14c7 */
    0x053c, 0x056c, 0x053d, 0x056d, 0x053e, 0x056e, 0x053f, 0x056f, /* 0x14c8-0x14cf */
    0x0540, 0x0570, 0x0541, 0x0571, 0x0542, 0x0572, 0x0543, 0x0573, /* 0x14d0-0x14d7 */
    0x0544, 0x0574, 0x0545, 0x0575, 0x0546, 0x0576, 0x0547, 0x0577, /* 0x14d8-0x14df */
    0x0548, 0x0578, 0x0549, 0x0579, 0x054a, 0x057a, 0x054b, 0x057b, /* 0x14e0-0x14e7 */
    0x054c, 0x057c, 0x054d, 0x057d, 0x054e, 0x057e, 0x054f, 0x057f, /* 0x14e8-0x14ef */
    0x0550, 0x0580, 0x0551, 0x0581, 0x0552, 0x0582, 0x0553, 0x0583, /* 0x14f0-0x14f7 */
    0x0554, 0x0584, 0x0555, 0x0585, 0x0556, 0x0586, 0x2019, 0x0027, /* 0x14f8-0x14ff */
};

static unsigned short const
keysym_to_unicode_15d0_15f6[] =
{
    0x10d0, 0x10d1, 0x10d2, 0x10d3, 0x10d4, 0x10d5, 0x10d6, 0x10d7, /* 0x15d0-0x15d7 */
    0x10d8, 0x10d9, 0x10da, 0x10db, 0x10dc, 0x10dd, 0x10de, 0x10df, /* 0x15d8-0x15df */
    0x10e0, 0x10e1, 0x10e2, 0x10e3, 0x10e4, 0x10e5, 0x10e6, 0x10e7, /* 0x15e0-0x15e7 */
    0x10e8, 0x10e9, 0x10ea, 0x10eb, 0x10ec, 0x10ed, 0x10ee, 0x10ef, /* 0x15e8-0x15ef */
    0x10f0, 0x10f1, 0x10f2, 0x10f3, 0x10f4, 0x10f5, 0x10f6          /* 0x15f0-0x15f7 */
};

static unsigned short const
keysym_to_unicode_16a0_16f6[] =
{
    0x0000, 0x0000, 0xf0a2, 0x1e8a, 0x0000, 0xf0a5, 0x012c, 0xf0a7, /* 0x16a0-0x16a7 */
    0xf0a8, 0x01b5, 0x01e6, 0x0000, 0x0000, 0x0000, 0x0000, 0x019f, /* 0x16a8-0x16af */
    0x0000, 0x0000, 0xf0b2, 0x1e8b, 0x01d1, 0xf0b5, 0x012d, 0xf0b7, /* 0x16b0-0x16b7 */
    0xf0b8, 0x01b6, 0x01e7, 0x0000, 0x0000, 0x01d2, 0x0000, 0x0275, /* 0x16b8-0x16bf */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x018f, 0x0000, /* 0x16c0-0x16c7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x16c8-0x16cf */
    0x0000, 0x1e36, 0xf0d2, 0xf0d3, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x16d0-0x16d7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x16d8-0x16df */
    0x0000, 0x1e37, 0xf0e2, 0xf0e3, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x16e0-0x16e7 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x16e8-0x16ef */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0259          /* 0x16f0-0x16f6 */
};

static unsigned short const
keysym_to_unicode_1e9f_1eff[] =
{
                                                            0x0303,
    0x1ea0, 0x1ea1, 0x1ea2, 0x1ea3, 0x1ea4, 0x1ea5, 0x1ea6, 0x1ea7, /* 0x1ea0-0x1ea7 */
    0x1ea8, 0x1ea9, 0x1eaa, 0x1eab, 0x1eac, 0x1ead, 0x1eae, 0x1eaf, /* 0x1ea8-0x1eaf */
    0x1eb0, 0x1eb1, 0x1eb2, 0x1eb3, 0x1eb4, 0x1eb5, 0x1eb6, 0x1eb7, /* 0x1eb0-0x1eb7 */
    0x1eb8, 0x1eb9, 0x1eba, 0x1ebb, 0x1ebc, 0x1ebd, 0x1ebe, 0x1ebf, /* 0x1eb8-0x1ebf */
    0x1ec0, 0x1ec1, 0x1ec2, 0x1ec3, 0x1ec4, 0x1ec5, 0x1ec6, 0x1ec7, /* 0x1ec0-0x1ec7 */
    0x1ec8, 0x1ec9, 0x1eca, 0x1ecb, 0x1ecc, 0x1ecd, 0x1ece, 0x1ecf, /* 0x1ec8-0x1ecf */
    0x1ed0, 0x1ed1, 0x1ed2, 0x1ed3, 0x1ed4, 0x1ed5, 0x1ed6, 0x1ed7, /* 0x1ed0-0x1ed7 */
    0x1ed8, 0x1ed9, 0x1eda, 0x1edb, 0x1edc, 0x1edd, 0x1ede, 0x1edf, /* 0x1ed8-0x1edf */
    0x1ee0, 0x1ee1, 0x1ee2, 0x1ee3, 0x1ee4, 0x1ee5, 0x1ee6, 0x1ee7, /* 0x1ee0-0x1ee7 */
    0x1ee8, 0x1ee9, 0x1eea, 0x1eeb, 0x1eec, 0x1eed, 0x1eee, 0x1eef, /* 0x1ee8-0x1eef */
    0x1ef0, 0x1ef1, 0x0300, 0x0301, 0x1ef4, 0x1ef5, 0x1ef6, 0x1ef7, /* 0x1ef0-0x1ef7 */
    0x1ef8, 0x1ef9, 0x01a0, 0x01a1, 0x01af, 0x01b0, 0x0309, 0x0323  /* 0x1ef8-0x1eff */
};

static unsigned short const
keysym_to_unicode_20a0_20ac[] =
{
    0x20a0, 0x20a1, 0x20a2, 0x20a3, 0x20a4, 0x20a5, 0x20a6, 0x20a7, /* 0x20a0-0x20a7 */
    0x20a8, 0x20a9, 0x20aa, 0x20ab, 0x20ac                          /* 0x20a8-0x20af */
};

static uint8_t const __utf8_mark[7] = {
    0x00, 0x00, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc
};

static int8_t const __utf32_clz_to_len[32] = {
    7,                  /* 0x80000000 */
    6, 6, 6, 6, 6,      /* 0x04000000 */
    5, 5, 5, 5, 5,      /* 0x00200000 */
    4, 4, 4, 4, 4,      /* 0x00010000 */
    3, 3, 3, 3, 3,      /* 0x00000800 */
    2, 2, 2, 2,         /* 0x00000080 */
    1, 1, 1, 1, 1, 1, 1 /* 0x00000001 */
};
#define utf8clen(c) __utf32_clz_to_len[__builtin_clz((uint32_t)(c) | 1)]

static bool
keysym_to_utf8(char *buf, const xcb_keysym_t ksym)
{
    unsigned int ksym_conv;
    int count;

    /* Unicode keysym */
    if((ksym & 0xff000000) == 0x01000000)
        ksym_conv = ksym & 0x00ffffff;
    else if(ksym > 0 && ksym < 0x100)
	ksym_conv = ksym;
    else if(ksym > 0x1a0 && ksym < 0x200)
	ksym_conv = keysym_to_unicode_1a1_1ff[ksym - 0x1a1];
    else if(ksym > 0x2a0 && ksym < 0x2ff)
	ksym_conv = keysym_to_unicode_2a1_2fe[ksym - 0x2a1];
    else if(ksym > 0x3a1 && ksym < 0x3ff)
	ksym_conv = keysym_to_unicode_3a2_3fe[ksym - 0x3a2];
    else if(ksym > 0x4a0 && ksym < 0x4e0)
	ksym_conv = keysym_to_unicode_4a1_4df[ksym - 0x4a1];
    else if(ksym > 0x589 && ksym < 0x5ff)
	ksym_conv = keysym_to_unicode_590_5fe[ksym - 0x590];
    else if(ksym > 0x67f && ksym < 0x700)
	ksym_conv = keysym_to_unicode_680_6ff[ksym - 0x680];
    else if(ksym > 0x7a0 && ksym < 0x7fa)
	ksym_conv = keysym_to_unicode_7a1_7f9[ksym - 0x7a1];
    else if(ksym > 0x8a3 && ksym < 0x8ff)
	ksym_conv = keysym_to_unicode_8a4_8fe[ksym - 0x8a4];
    else if(ksym > 0x9de && ksym < 0x9f9)
	ksym_conv = keysym_to_unicode_9df_9f8[ksym - 0x9df];
    else if(ksym > 0xaa0 && ksym < 0xaff)
	ksym_conv = keysym_to_unicode_aa1_afe[ksym - 0xaa1];
    else if(ksym > 0xcde && ksym < 0xcfb)
	ksym_conv = keysym_to_unicode_cdf_cfa[ksym - 0xcdf];
    else if(ksym > 0xda0 && ksym < 0xdfa)
	ksym_conv = keysym_to_unicode_da1_df9[ksym - 0xda1];
    else if(ksym > 0xe9f && ksym < 0xf00)
	ksym_conv = keysym_to_unicode_ea0_eff[ksym - 0xea0];
    else if(ksym > 0x12a0 && ksym < 0x12ff)
	ksym_conv = keysym_to_unicode_12a1_12fe[ksym - 0x12a1];
    else if(ksym > 0x13bb && ksym < 0x13bf)
	ksym_conv = keysym_to_unicode_13bc_13be[ksym - 0x13bc];
    else if(ksym > 0x14a0 && ksym < 0x1500)
        ksym_conv = keysym_to_unicode_14a1_14ff[ksym - 0x14a1];
    else if(ksym > 0x15cf && ksym < 0x15f7)
	ksym_conv = keysym_to_unicode_15d0_15f6[ksym - 0x15d0];
    else if(ksym > 0x169f && ksym < 0x16f7)
	ksym_conv = keysym_to_unicode_16a0_16f6[ksym - 0x16a0];
    else if(ksym > 0x1e9e && ksym < 0x1f00)
	ksym_conv = keysym_to_unicode_1e9f_1eff[ksym - 0x1e9f];
    else if(ksym > 0x209f && ksym < 0x20ad)
	ksym_conv = keysym_to_unicode_20a0_20ac[ksym - 0x20a0];
    else
        return false;

    count = utf8clen(ksym_conv);
    switch(count)
    {
      case 7: return false;
      case 6: buf[5] = (ksym_conv | 0x80) & 0xbf; ksym_conv >>= 6;
      case 5: buf[4] = (ksym_conv | 0x80) & 0xbf; ksym_conv >>= 6;
      case 4: buf[3] = (ksym_conv | 0x80) & 0xbf; ksym_conv >>= 6;
      case 3: buf[2] = (ksym_conv | 0x80) & 0xbf; ksym_conv >>= 6;
      case 2: buf[1] = (ksym_conv | 0x80) & 0xbf; ksym_conv >>= 6;
      case 1: buf[0] = (ksym_conv | __utf8_mark[count]);
    }
    buf[count] = '\0';
    return true;
}

static bool
keysym_to_str(char *buf, ssize_t len, const xcb_keysym_t ksym)
{
    switch(ksym)
    {
#define CASE(k)  case XK_##k: a_strcpy(buf, len, #k); return true
        CASE(BackSpace);
        CASE(Tab);
        CASE(Clear);
        CASE(Return);
        CASE(Pause);
        CASE(Scroll_Lock);
        CASE(Sys_Req);
        CASE(Escape);
        CASE(Delete);

        CASE(Home);
        CASE(Left);
        CASE(Up);
        CASE(Right);
        CASE(Down);
        CASE(Page_Up);
        CASE(Page_Down);
        CASE(End);
        CASE(Begin);

        CASE(Select);
        CASE(Print);
        CASE(Execute);
        CASE(Insert);
        CASE(Undo);
        CASE(Redo);
        CASE(Menu);
        CASE(Find);
        CASE(Cancel);
        CASE(Help);
        CASE(Break);
        CASE(Mode_switch);
        CASE(Num_Lock);

      case XK_KP_Space:
        /* Patch encoding botch */
        buf[0] = XK_space & 0x7F;
        break;
        CASE(KP_Tab);
        CASE(KP_Enter);
        CASE(KP_F1);
        CASE(KP_F2);
        CASE(KP_F3);
        CASE(KP_F4);
        CASE(KP_Home);
        CASE(KP_Left);
        CASE(KP_Up);
        CASE(KP_Right);
        CASE(KP_Down);
        CASE(KP_Page_Up);
        CASE(KP_Page_Down);
        CASE(KP_End);
        CASE(KP_Begin);
        CASE(KP_Insert);
        CASE(KP_Delete);
        CASE(KP_Separator);

        CASE(F1);  CASE(F2);  CASE(F3);  CASE(F4);  CASE(F5);  CASE(F6);
        CASE(F7);  CASE(F8);  CASE(F9);  CASE(F10); CASE(F11); CASE(F12);
        CASE(F13); CASE(F14); CASE(F15); CASE(F16); CASE(F17); CASE(F18);
        CASE(F19); CASE(F20); CASE(F21); CASE(F22); CASE(F23); CASE(F24);
        CASE(F25); CASE(F26); CASE(F27); CASE(F28); CASE(F29); CASE(F30);
        CASE(F31); CASE(F32); CASE(F33); CASE(F34); CASE(F35);

        CASE(Shift_L);
        CASE(Shift_R);
        CASE(Control_L);
        CASE(Control_R);
        CASE(Caps_Lock);
        CASE(Shift_Lock);

        CASE(Meta_L);
        CASE(Meta_R);
        CASE(Alt_L);
        CASE(Alt_R);
        CASE(Super_L);
        CASE(Super_R);
        CASE(Hyper_L);
        CASE(Hyper_R);
      default:
        buf[0] = ksym & 0x7F;
        break;
    }

    buf[1] = '\0';
    return true;
}

static bool
keysym_to_xkb(char *buf, ssize_t len, const xcb_keysym_t ksym)
{
    switch(ksym)
    {
        CASE(ISO_Lock);
        CASE(ISO_Level2_Latch);
        CASE(ISO_Level3_Shift);
        CASE(ISO_Level3_Latch);
        CASE(ISO_Level3_Lock);
        CASE(ISO_Level5_Shift);
        CASE(ISO_Level5_Latch);
        CASE(ISO_Level5_Lock);
        CASE(ISO_Group_Shift);
        CASE(ISO_Group_Latch);
        CASE(ISO_Group_Lock);
        CASE(ISO_Next_Group);
        CASE(ISO_Next_Group_Lock);
        CASE(ISO_Prev_Group);
        CASE(ISO_Prev_Group_Lock);
        CASE(ISO_First_Group);
        CASE(ISO_First_Group_Lock);
        CASE(ISO_Last_Group);
        CASE(ISO_Last_Group_Lock);
        CASE(ISO_Left_Tab);
        CASE(ISO_Move_Line_Up);
        CASE(ISO_Move_Line_Down);
        CASE(ISO_Partial_Line_Up);
        CASE(ISO_Partial_Line_Down);
        CASE(ISO_Partial_Space_Left);
        CASE(ISO_Partial_Space_Right);
        CASE(ISO_Set_Margin_Left);
        CASE(ISO_Set_Margin_Right);
        CASE(ISO_Release_Margin_Left);
        CASE(ISO_Release_Margin_Right);
        CASE(ISO_Release_Both_Margins);
        CASE(ISO_Fast_Cursor_Left);
        CASE(ISO_Fast_Cursor_Right);
        CASE(ISO_Fast_Cursor_Up);
        CASE(ISO_Fast_Cursor_Down);
        CASE(ISO_Continuous_Underline);
        CASE(ISO_Discontinuous_Underline);
        CASE(ISO_Emphasize);
        CASE(ISO_Center_Object);
        CASE(ISO_Enter);
        CASE(dead_grave);
        CASE(dead_acute);
        CASE(dead_circumflex);
        CASE(dead_tilde);
        CASE(dead_macron);
        CASE(dead_breve);
        CASE(dead_abovedot);
        CASE(dead_diaeresis);
        CASE(dead_abovering);
        CASE(dead_doubleacute);
        CASE(dead_caron);
        CASE(dead_cedilla);
        CASE(dead_ogonek);
        CASE(dead_iota);
        CASE(dead_voiced_sound);
        CASE(dead_semivoiced_sound);
        CASE(dead_belowdot);
        CASE(dead_hook);
        CASE(dead_horn);
        CASE(dead_stroke);
        CASE(dead_abovecomma);
        CASE(dead_abovereversedcomma);
        CASE(dead_doublegrave);
        CASE(dead_belowring);
        CASE(dead_belowmacron);
        CASE(dead_belowcircumflex);
        CASE(dead_belowtilde);
        CASE(dead_belowbreve);
        CASE(dead_belowdiaeresis);
        CASE(dead_invertedbreve);
        CASE(dead_belowcomma);
        CASE(dead_currency);
        CASE(dead_a);
        CASE(dead_A);
        CASE(dead_e);
        CASE(dead_E);
        CASE(dead_i);
        CASE(dead_I);
        CASE(dead_o);
        CASE(dead_O);
        CASE(dead_u);
        CASE(dead_U);
        CASE(dead_small_schwa);
        CASE(dead_capital_schwa);
        CASE(First_Virtual_Screen);
        CASE(Prev_Virtual_Screen);
        CASE(Next_Virtual_Screen);
        CASE(Last_Virtual_Screen);
        CASE(Terminate_Server);
        CASE(AccessX_Enable);
        CASE(AccessX_Feedback_Enable);
        CASE(RepeatKeys_Enable);
        CASE(SlowKeys_Enable);
        CASE(BounceKeys_Enable);
        CASE(StickyKeys_Enable);
        CASE(MouseKeys_Enable);
        CASE(MouseKeys_Accel_Enable);
        CASE(Overlay1_Enable);
        CASE(Overlay2_Enable);
        CASE(AudibleBell_Enable);
        CASE(Pointer_Left);
        CASE(Pointer_Right);
        CASE(Pointer_Up);
        CASE(Pointer_Down);
        CASE(Pointer_UpLeft);
        CASE(Pointer_UpRight);
        CASE(Pointer_DownLeft);
        CASE(Pointer_DownRight);
        CASE(Pointer_Button_Dflt);
        CASE(Pointer_Button1);
        CASE(Pointer_Button2);
        CASE(Pointer_Button3);
        CASE(Pointer_Button4);
        CASE(Pointer_Button5);
        CASE(Pointer_DblClick_Dflt);
        CASE(Pointer_DblClick1);
        CASE(Pointer_DblClick2);
        CASE(Pointer_DblClick3);
        CASE(Pointer_DblClick4);
        CASE(Pointer_DblClick5);
        CASE(Pointer_Drag_Dflt);
        CASE(Pointer_Drag1);
        CASE(Pointer_Drag2);
        CASE(Pointer_Drag3);
        CASE(Pointer_Drag4);
        CASE(Pointer_Drag5);
        CASE(Pointer_EnableKeys);
        CASE(Pointer_Accelerate);
        CASE(Pointer_DfltBtnNext);
        CASE(Pointer_DfltBtnPrev);
      default:
        return false;
    }

    return true;
}
#undef CASE

static bool
keysym_to_xf86(char *buf, ssize_t len, const xcb_keysym_t ksym)
{
    switch(ksym)
    {
#define CASE(k)  case XF86XK_##k: a_strcpy(buf, len, "XF86"); a_strcat(buf, len, #k); return true
        CASE(ModeLock);
        CASE(MonBrightnessUp);
        CASE(MonBrightnessDown);
        CASE(KbdLightOnOff);
        CASE(KbdBrightnessUp);
        CASE(KbdBrightnessDown);
        CASE(Standby);
        CASE(AudioLowerVolume);
        CASE(AudioMute);
        CASE(AudioRaiseVolume);
        CASE(AudioPlay);
        CASE(AudioStop);
        CASE(AudioPrev);
        CASE(AudioNext);
        CASE(HomePage);
        CASE(Mail);
        CASE(Start);
        CASE(Search);
        CASE(AudioRecord);
        CASE(Calculator);
        CASE(Memo);
        CASE(ToDoList);
        CASE(Calendar);
        CASE(PowerDown);
        CASE(ContrastAdjust);
        CASE(RockerUp);
        CASE(RockerDown);
        CASE(RockerEnter);
        CASE(Back);
        CASE(Forward);
        CASE(Stop);
        CASE(Refresh);
        CASE(PowerOff);
        CASE(WakeUp);
        CASE(Eject);
        CASE(ScreenSaver);
        CASE(WWW);
        CASE(Sleep);
        CASE(Favorites);
        CASE(AudioPause);
        CASE(AudioMedia);
        CASE(MyComputer);
        CASE(VendorHome);
        CASE(LightBulb);
        CASE(Shop);
        CASE(History);
        CASE(OpenURL);
        CASE(AddFavorite);
        CASE(HotLinks);
        CASE(BrightnessAdjust);
        CASE(Finance);
        CASE(Community);
        CASE(AudioRewind);
        CASE(BackForward);
        CASE(Launch0);
        CASE(Launch1);
        CASE(Launch2);
        CASE(Launch3);
        CASE(Launch4);
        CASE(Launch5);
        CASE(Launch6);
        CASE(Launch7);
        CASE(Launch8);
        CASE(Launch9);
        CASE(LaunchA);
        CASE(LaunchB);
        CASE(LaunchC);
        CASE(LaunchD);
        CASE(LaunchE);
        CASE(LaunchF);
        CASE(ApplicationLeft);
        CASE(ApplicationRight);
        CASE(Book);
        CASE(CD);
        CASE(Calculater);
        CASE(Clear);
        CASE(Close);
        CASE(Copy);
        CASE(Cut);
        CASE(Display);
        CASE(DOS);
        CASE(Documents);
        CASE(Excel);
        CASE(Explorer);
        CASE(Game);
        CASE(Go);
        CASE(iTouch);
        CASE(LogOff);
        CASE(Market);
        CASE(Meeting);
        CASE(MenuKB);
        CASE(MenuPB);
        CASE(MySites);
        CASE(New);
        CASE(News);
        CASE(OfficeHome);
        CASE(Open);
        CASE(Option);
        CASE(Paste);
        CASE(Phone);
        CASE(Q);
        CASE(Reply);
        CASE(Reload);
        CASE(RotateWindows);
        CASE(RotationPB);
        CASE(RotationKB);
        CASE(Save);
        CASE(ScrollUp);
        CASE(ScrollDown);
        CASE(ScrollClick);
        CASE(Send);
        CASE(Spell);
        CASE(SplitScreen);
        CASE(Support);
        CASE(TaskPane);
        CASE(Terminal);
        CASE(Tools);
        CASE(Travel);
        CASE(UserPB);
        CASE(User1KB);
        CASE(User2KB);
        CASE(Video);
        CASE(WheelButton);
        CASE(Word);
        CASE(Xfer);
        CASE(ZoomIn);
        CASE(ZoomOut);
        CASE(Away);
        CASE(Messenger);
        CASE(WebCam);
        CASE(MailForward);
        CASE(Pictures);
        CASE(Music);
        CASE(Battery);
        CASE(Bluetooth);
        CASE(WLAN);
        CASE(UWB);
        CASE(AudioForward);
        CASE(AudioRepeat);
        CASE(AudioRandomPlay);
        CASE(Subtitle);
        CASE(AudioCycleTrack);
        CASE(CycleAngle);
        CASE(FrameBack);
        CASE(FrameForward);
        CASE(Time);
        CASE(Select);
        CASE(View);
        CASE(TopMenu);
        CASE(Red);
        CASE(Green);
        CASE(Yellow);
        CASE(Blue);
        CASE(Suspend);
        CASE(Hibernate);
        CASE(Switch_VT_1);
        CASE(Switch_VT_2);
        CASE(Switch_VT_3);
        CASE(Switch_VT_4);
        CASE(Switch_VT_5);
        CASE(Switch_VT_6);
        CASE(Switch_VT_7);
        CASE(Switch_VT_8);
        CASE(Switch_VT_9);
        CASE(Switch_VT_10);
        CASE(Switch_VT_11);
        CASE(Switch_VT_12);
        CASE(Ungrab);
        CASE(ClearGrab);
        CASE(Next_VMode);
        CASE(Prev_VMode);
      default:
        return false;
    }

    return true;
}
#undef CASE

bool
key_press_lookup_string(xcb_keysym_t ksym,
                        char *buf, ssize_t buf_len)
{
    /* Handle special KeySym (Tab, Newline...) */
    if((ksym & 0xffffff00) == 0xff00)
        return keysym_to_str(buf, buf_len, ksym);
    else if((ksym & 0xfffffe00) == 0xfe00)
        return keysym_to_xkb(buf, buf_len, ksym);
    else if((ksym & 0x1008F000) == 0x1008F000)
        return keysym_to_xf86(buf, buf_len, ksym);

    /* Handle other KeySym (like unicode...) */
    return keysym_to_utf8(buf, ksym);
}

/** Return the keysym from keycode.
 * \param detail The keycode received.
 * \param state The modifier state.
 * \return A keysym.
 */
xcb_keysym_t
key_getkeysym(xcb_keycode_t detail, uint16_t state)
{
    xcb_keysym_t k0, k1;

    /* 'col' (third parameter) is used to get the proper KeySym
     * according to modifier (XCB doesn't provide an equivalent to
     * XLookupString()).
     *
     * If Mode_Switch is ON we look into second group.
     */
    if(state & globalconf.modeswitchmask)
    {
        k0 = xcb_key_symbols_get_keysym(globalconf.keysyms, detail, 4);
        k1 = xcb_key_symbols_get_keysym(globalconf.keysyms, detail, 5);
    }
    else
    {
        k0 = xcb_key_symbols_get_keysym(globalconf.keysyms, detail, 0);
        k1 = xcb_key_symbols_get_keysym(globalconf.keysyms, detail, 1);
    }

    /* If the second column does not exists use the first one. */
    if(k1 == XCB_NO_SYMBOL)
        k1 = k0;

    /* The numlock modifier is on and the second KeySym is a keypad
     * KeySym */
    if((state & globalconf.numlockmask) && xcb_is_keypad_key(k1))
    {
        /* The Shift modifier is on, or if the Lock modifier is on and
         * is interpreted as ShiftLock, use the first KeySym */
        if((state & XCB_MOD_MASK_SHIFT)
           || (state & XCB_MOD_MASK_LOCK && (state & globalconf.shiftlockmask)))
            return k0;
        else
            return k1;
    }

    /* The Shift and Lock modifers are both off, use the first
     * KeySym */
    else if(!(state & XCB_MOD_MASK_SHIFT) && !(state & XCB_MOD_MASK_LOCK))
        return k0;

    /* The Shift modifier is off and the Lock modifier is on and is
     * interpreted as CapsLock */
    else if(!(state & XCB_MOD_MASK_SHIFT)
            && (state & XCB_MOD_MASK_LOCK && (state & globalconf.capslockmask)))
        /* The first Keysym is used but if that KeySym is lowercase
         * alphabetic, then the corresponding uppercase KeySym is used
         * instead */
        return k1;

    /* The Shift modifier is on, and the Lock modifier is on and is
     * interpreted as CapsLock */
    else if((state & XCB_MOD_MASK_SHIFT)
            && (state & XCB_MOD_MASK_LOCK && (state & globalconf.capslockmask)))
        /* The second Keysym is used but if that KeySym is lowercase
         * alphabetic, then the corresponding uppercase KeySym is used
         * instead */
        return k1;

    /* The Shift modifier is on, or the Lock modifier is on and is
     * interpreted as ShiftLock, or both */
    else if((state & XCB_MOD_MASK_SHIFT)
            || (state & XCB_MOD_MASK_LOCK && (state & globalconf.shiftlockmask)))
        return k1;

    return XCB_NO_SYMBOL;
}

static void
luaA_keystore(lua_State *L, int ud, const char *str, ssize_t len)
{
    keyb_t *key = luaA_checkudata(L, ud, &key_class);
    if(len)
    {
        if(*str != '#')
        {
            key->keysym = XStringToKeysym(str);
            if(!key->keysym)
            {
                if(len == 1)
                    key->keysym = *str;
                else
                    warn("there's no keysym named \"%s\"", str);
            }
            key->keycode = 0;
        }
        else
        {
            key->keycode = atoi(str + 1);
            key->keysym = 0;
        }
        luaA_object_emit_signal(L, ud, "property::key", 0);
    }
}

/** Create a new key object.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_key_new(lua_State *L)
{
    size_t len;
    keyb_t *k;
    const char *key;

    /* compat code */
    if(lua_istable(L, 2) && lua_isstring(L, 3))
    {
        luaA_deprecate(L, "new syntax");

        /* be sure there's 5 arguments */
        lua_settop(L, 5);

        /* arg 2 is key mod table */
        luaA_checktable(L, 2);
        /* arg 3 is key */
        key = luaL_checklstring(L, 3, &len);

        if(!lua_isnil(L, 4))
            luaA_checkfunction(L, 4);

        if(!lua_isnil(L, 5))
            luaA_checkfunction(L, 5);

        k = key_new(L);
        luaA_keystore(L, -1, key, len);
        k->modifiers = luaA_tomodifiers(L, 2);

        if(!lua_isnil(L, 4))
        {
            lua_pushvalue(L, 4);
            luaA_object_add_signal(L, -2, "press", -1);
        }

        if(!lua_isnil(L, 5))
        {
            lua_pushvalue(L, 5);
            luaA_object_add_signal(L, -2, "release", -1);
        }

        return 1;
    }

    return luaA_class_new(L, &key_class);
}

/** Set a key array with a Lua table.
 * \param L The Lua VM state.
 * \param oidx The index of the object to store items into.
 * \param idx The index of the Lua table.
 * \param keys The array key to fill.
 */
void
luaA_key_array_set(lua_State *L, int oidx, int idx, key_array_t *keys)
{
    luaA_checktable(L, idx);

    foreach(key, *keys)
        luaA_object_unref_item(L, oidx, *key);

    key_array_wipe(keys);
    key_array_init(keys);

    lua_pushnil(L);
    while(lua_next(L, idx))
        if(luaA_toudata(L, -1, &key_class))
            key_array_append(keys, luaA_object_ref_item(L, oidx, -1));
        else
            lua_pop(L, 1);
}

/** Push an array of key as an Lua table onto the stack.
 * \param L The Lua VM state.
 * \param oidx The index of the object to get items from.
 * \param keys The key array to push.
 * \return The number of elements pushed on stack.
 */
int
luaA_key_array_get(lua_State *L, int oidx, key_array_t *keys)
{
    lua_createtable(L, keys->len, 0);
    for(int i = 0; i < keys->len; i++)
    {
        luaA_object_push_item(L, oidx, keys->tab[i]);
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

/** Push a modifier set to a Lua table.
 * \param L The Lua VM state.
 * \param modifiers The modifier.
 * \return The number of elements pushed on stack.
 */
int
luaA_pushmodifiers(lua_State *L, uint16_t modifiers)
{
    lua_newtable(L);
    {
        int i = 1;
        for(uint32_t maski = XCB_MOD_MASK_SHIFT; maski <= XCB_BUTTON_MASK_ANY; maski <<= 1)
            if(maski & modifiers)
            {
                const char *mod;
                size_t slen;
                xutil_key_mask_tostr(maski, &mod, &slen);
                lua_pushlstring(L, mod, slen);
                lua_rawseti(L, -2, i++);
            }
    }
    return 1;
}

/** Take a modifier table from the stack and return modifiers mask.
 * \param L The Lua VM state.
 * \param ud The index of the table.
 * \return The mask value.
 */
uint16_t
luaA_tomodifiers(lua_State *L, int ud)
{
    luaA_checktable(L, ud);
    ssize_t len = lua_objlen(L, ud);
    uint16_t mod = XCB_NONE;
    for(int i = 1; i <= len; i++)
    {
        lua_rawgeti(L, ud, i);
        size_t blen;
        const char *key = luaL_checklstring(L, -1, &blen);
        mod |= xutil_key_mask_fromstr(key, blen);
        lua_pop(L, 1);
    }
    return mod;
}

static int
luaA_key_set_modifiers(lua_State *L, keyb_t *k)
{
    k->modifiers = luaA_tomodifiers(L, -1);
    luaA_object_emit_signal(L, -3, "property::modifiers", 0);
    return 0;
}

LUA_OBJECT_EXPORT_PROPERTY(key, keyb_t, modifiers, luaA_pushmodifiers)

static int
luaA_key_get_key(lua_State *L, keyb_t *k)
{
    if(k->keycode)
    {
        char buf[12];
        int slen = snprintf(buf, sizeof(buf), "#%u", k->keycode);
        lua_pushlstring(L, buf, slen);
    }
    else
    {
        char buf[MAX(MB_LEN_MAX, 32)];
        if(!key_press_lookup_string(k->keysym, buf, countof(buf)))
            return 0;

        lua_pushstring(L, buf);
    }
    return 1;
}

static int
luaA_key_get_keysym(lua_State *L, keyb_t *k)
{
    lua_pushstring(L, XKeysymToString(k->keysym));
    return 1;
}

static int
luaA_key_set_key(lua_State *L, keyb_t *k)
{
    size_t klen;
    const char *key = luaL_checklstring(L, -1, &klen);
    luaA_keystore(L, -3, key, klen);
    return 0;
}

void
key_class_setup(lua_State *L)
{
    static const struct luaL_reg key_methods[] =
    {
        LUA_CLASS_METHODS(key)
        { "__call", luaA_key_new },
        { NULL, NULL }
    };

    static const struct luaL_reg key_meta[] =
    {
        LUA_OBJECT_META(key)
        LUA_CLASS_META
        { "__gc", luaA_object_gc },
        { NULL, NULL },
    };

    luaA_class_setup(L, &key_class, "key", (lua_class_allocator_t) key_new,
                     luaA_class_index_miss_property, luaA_class_newindex_miss_property,
                     key_methods, key_meta);
    luaA_class_add_property(&key_class, A_TK_KEY,
                            (lua_class_propfunc_t) luaA_key_set_key,
                            (lua_class_propfunc_t) luaA_key_get_key,
                            (lua_class_propfunc_t) luaA_key_set_key);
    luaA_class_add_property(&key_class, A_TK_KEYSYM,
                            NULL,
                            (lua_class_propfunc_t) luaA_key_get_keysym,
                            NULL);
    luaA_class_add_property(&key_class, A_TK_MODIFIERS,
                            (lua_class_propfunc_t) luaA_key_set_modifiers,
                            (lua_class_propfunc_t) luaA_key_get_modifiers,
                            (lua_class_propfunc_t) luaA_key_set_modifiers);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
