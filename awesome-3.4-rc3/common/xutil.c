/*
 * common/xutil.c - X-related useful functions
 *
 * Copyright © 2007-2009 Julien Danjou <julien@danjou.info>
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

/* XCB doesn't provide keysyms definition */
#include <X11/keysym.h>

#include "common/util.h"

#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_icccm.h>

#include "common/xutil.h"
#include "common/tokenize.h"

/** Get the lock masks (shiftlock, numlock, capslock, modeswitch).
 * \param connection The X connection.
 * \param cookie The cookie of the request.
 * \param keysyms Key symbols.
 * \param numlockmask Numlock mask.
 * \param shiftlockmask Shiftlock mask.
 * \param capslockmask Capslock mask.
 * \todo Split this.
 */
void
xutil_lock_mask_get(xcb_connection_t *connection,
                    xcb_get_modifier_mapping_cookie_t cookie,
                    xcb_key_symbols_t *keysyms,
                    uint16_t *numlockmask,
                    uint16_t *shiftlockmask,
                    uint16_t *capslockmask,
                    uint16_t *modeswitchmask)
{
    xcb_get_modifier_mapping_reply_t *modmap_r;
    xcb_keycode_t *modmap, kc;
    xcb_keycode_t *numlockcodes = xcb_key_symbols_get_keycode(keysyms, XK_Num_Lock);
    xcb_keycode_t *shiftlockcodes = xcb_key_symbols_get_keycode(keysyms, XK_Shift_Lock);
    xcb_keycode_t *capslockcodes = xcb_key_symbols_get_keycode(keysyms, XK_Caps_Lock);
    xcb_keycode_t *modeswitchcodes = xcb_key_symbols_get_keycode(keysyms, XK_Mode_switch);

    modmap_r = xcb_get_modifier_mapping_reply(connection, cookie, NULL);
    modmap = xcb_get_modifier_mapping_keycodes(modmap_r);

    /* reset */
    *numlockmask = *shiftlockmask = *capslockmask = *modeswitchmask = 0;

    int i;
    for(i = 0; i < 8; i++)
        for(int j = 0; j < modmap_r->keycodes_per_modifier; j++)
        {
            kc = modmap[i * modmap_r->keycodes_per_modifier + j];

#define LOOK_FOR(mask, codes) \
            if(*mask == 0 && codes) \
                for(xcb_keycode_t *ktest = codes; *ktest; ktest++) \
                    if(*ktest == kc) \
                    { \
                        *mask = (1 << i); \
                        break; \
                    }

            LOOK_FOR(numlockmask, numlockcodes)
            LOOK_FOR(shiftlockmask, shiftlockcodes)
            LOOK_FOR(capslockmask, capslockcodes)
            LOOK_FOR(modeswitchmask, modeswitchcodes)
#undef LOOK_FOR
        }
    p_delete(&numlockcodes);
    p_delete(&shiftlockcodes);
    p_delete(&capslockcodes);
    p_delete(&modeswitchcodes);
    p_delete(&modmap_r);
}

/* Number of different errors */
#define ERRORS_NBR 256

void
xutil_error_handler_catch_all_set(xcb_event_handlers_t *evenths,
                                  xcb_generic_error_handler_t handler,
                                  void *data)
{
    int err_num;
    for(err_num = 0; err_num < ERRORS_NBR; err_num++)
	xcb_event_set_error_handler(evenths, err_num, handler, data);
}

uint16_t
xutil_key_mask_fromstr(const char *keyname, size_t len)
{
    switch(a_tokenize(keyname, len))
    {
      case A_TK_SHIFT:   return XCB_MOD_MASK_SHIFT;
      case A_TK_LOCK:    return XCB_MOD_MASK_LOCK;
      case A_TK_CTRL:
      case A_TK_CONTROL: return XCB_MOD_MASK_CONTROL;
      case A_TK_MOD1:    return XCB_MOD_MASK_1;
      case A_TK_MOD2:    return XCB_MOD_MASK_2;
      case A_TK_MOD3:    return XCB_MOD_MASK_3;
      case A_TK_MOD4:    return XCB_MOD_MASK_4;
      case A_TK_MOD5:    return XCB_MOD_MASK_5;
      /* this is misnamed but correct */
      case A_TK_ANY:     return XCB_BUTTON_MASK_ANY;
      default:           return XCB_NO_SYMBOL;
    }
}

void
xutil_key_mask_tostr(uint16_t mask, const char **name, size_t *len)
{
    switch(mask)
    {
#define SET_RESULT(res) \
        *name = #res; \
        *len = sizeof(#res) - 1; \
        return;
      case XCB_MOD_MASK_SHIFT:   SET_RESULT(Shift)
      case XCB_MOD_MASK_LOCK:    SET_RESULT(Lock)
      case XCB_MOD_MASK_CONTROL: SET_RESULT(Control)
      case XCB_MOD_MASK_1:       SET_RESULT(Mod1)
      case XCB_MOD_MASK_2:       SET_RESULT(Mod2)
      case XCB_MOD_MASK_3:       SET_RESULT(Mod3)
      case XCB_MOD_MASK_4:       SET_RESULT(Mod4)
      case XCB_MOD_MASK_5:       SET_RESULT(Mod5)
      case XCB_BUTTON_MASK_ANY:  SET_RESULT(Any)
      default:                   SET_RESULT(Unknown)
#undef SET_RESULT
    }
}

/** Convert a root window a physical screen ID.
 * \param connection The connection to the X server.
 * \param root Root window.
 * \return A physical screen number.
 */
int
xutil_root2screen(xcb_connection_t *connection, xcb_window_t root)
{
    xcb_screen_iterator_t iter;
    int phys_screen = 0;

    for(iter = xcb_setup_roots_iterator(xcb_get_setup(connection));
        iter.rem && iter.data->root != root; xcb_screen_next(&iter), ++phys_screen);

    return phys_screen;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
