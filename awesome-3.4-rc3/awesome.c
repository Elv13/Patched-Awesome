/*
 * awesome.c - awesome main functions
 *
 * Copyright © 2007-2008 Julien Danjou <julien@danjou.info>
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

#include <getopt.h>

#include <locale.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include <xcb/xtest.h>
#include <xcb/xcb_event.h>

#include "awesome.h"
#include "spawn.h"
#include "client.h"
#include "window.h"
#include "ewmh.h"
#include "dbus.h"
#include "systray.h"
#include "event.h"
#include "property.h"
#include "screen.h"
#include "titlebar.h"
#include "luaa.h"
#include "common/version.h"
#include "common/atoms.h"
#include "common/xcursor.h"
#include "common/xutil.h"
#include "common/backtrace.h"

awesome_t globalconf;

typedef struct
{
    xcb_window_t id;
    xcb_query_tree_cookie_t tree_cookie;
} root_win_t;

/** Call before exiting.
 */
void
awesome_atexit(void)
{
    int screen_nbr, nscreens;

    if(globalconf.hooks.exit != LUA_REFNIL)
        luaA_dofunction_from_registry(globalconf.L, globalconf.hooks.exit, 0, 0);

    signal_object_emit(globalconf.L, &global_signals, "exit", 0);

    a_dbus_cleanup();

    /* reparent systray windows, otherwise they may die with their master */
    for(int i = 0; i < globalconf.embedded.len; i++)
    {
        xcb_screen_t *s = xutil_screen_get(globalconf.connection,
                                           globalconf.embedded.tab[i].phys_screen);
        xembed_window_unembed(globalconf.connection, globalconf.embedded.tab[i].win, s->root);
    }

    /* do this only for real screen */
    const xcb_setup_t *setup = xcb_get_setup(globalconf.connection);
    nscreens = setup ? xcb_setup_roots_length(setup) : -1;
    for(screen_nbr = 0;
        screen_nbr < nscreens;
        screen_nbr++)
        systray_cleanup(screen_nbr);

    /* remap all clients since some WM won't handle them otherwise */
    foreach(c, globalconf.clients)
    {
        client_unban(*c);
        titlebar_client_detach(*c);
    }

    /* Close Lua */
    lua_close(globalconf.L);

    xcb_flush(globalconf.connection);

    /* Disconnect *after* closing lua */
    xcb_disconnect(globalconf.connection);

    ev_default_destroy();
}

/** Scan X to find windows to manage.
 */
static void
scan(void)
{
    int i, phys_screen, tree_c_len;
    const int screen_max = xcb_setup_roots_length(xcb_get_setup(globalconf.connection));
    root_win_t root_wins[screen_max];
    xcb_query_tree_reply_t *tree_r;
    xcb_window_t *wins = NULL;
    xcb_get_window_attributes_reply_t *attr_r;
    xcb_get_geometry_reply_t *geom_r;
    long state;

    for(phys_screen = 0; phys_screen < screen_max; phys_screen++)
    {
        /* Get the root window ID associated to this screen */
        root_wins[phys_screen].id = xutil_screen_get(globalconf.connection, phys_screen)->root;

        /* Get the window tree associated to this screen */
        root_wins[phys_screen].tree_cookie = xcb_query_tree_unchecked(globalconf.connection,
                                                                      root_wins[phys_screen].id);
    }

    for(phys_screen = 0; phys_screen < screen_max; phys_screen++)
    {
        tree_r = xcb_query_tree_reply(globalconf.connection,
                                      root_wins[phys_screen].tree_cookie,
                                      NULL);

        if(!tree_r)
            continue;

        /* Get the tree of the children windows of the current root window */
        if(!(wins = xcb_query_tree_children(tree_r)))
            fatal("cannot get tree children");

        tree_c_len = xcb_query_tree_children_length(tree_r);
        xcb_get_window_attributes_cookie_t attr_wins[tree_c_len];
        xcb_get_property_cookie_t state_wins[tree_c_len];

        for(i = 0; i < tree_c_len; i++)
        {
            attr_wins[i] = xcb_get_window_attributes_unchecked(globalconf.connection,
                                                               wins[i]);

            state_wins[i] = window_state_get_unchecked(wins[i]);
        }

        xcb_get_geometry_cookie_t *geom_wins[tree_c_len];

        for(i = 0; i < tree_c_len; i++)
        {
            attr_r = xcb_get_window_attributes_reply(globalconf.connection,
                                                     attr_wins[i],
                                                     NULL);

            state = window_state_get_reply(state_wins[i]);

            if(!attr_r || attr_r->override_redirect
               || attr_r->map_state == XCB_MAP_STATE_UNVIEWABLE
               || state == XCB_WM_STATE_WITHDRAWN)
            {
                geom_wins[i] = NULL;
                p_delete(&attr_r);
                continue;
            }

            p_delete(&attr_r);

            /* Get the geometry of the current window */
            geom_wins[i] = p_alloca(xcb_get_geometry_cookie_t, 1);
            *(geom_wins[i]) = xcb_get_geometry_unchecked(globalconf.connection, wins[i]);
        }

        for(i = 0; i < tree_c_len; i++)
        {
            if(!geom_wins[i] || !(geom_r = xcb_get_geometry_reply(globalconf.connection,
                                                                  *(geom_wins[i]), NULL)))
                continue;

            /* The window can be mapped, so force it to be undrawn for startup */
            xcb_unmap_window(globalconf.connection, wins[i]);

            client_manage(wins[i], geom_r, phys_screen, true);

            p_delete(&geom_r);
        }

        p_delete(&tree_r);
    }
}

static void
a_refresh_cb(EV_P_ ev_prepare *w, int revents)
{
    awesome_refresh();
}

static void
a_xcb_check_cb(EV_P_ ev_check *w, int revents)
{
    xcb_generic_event_t *mouse = NULL, *event;

    while((event = xcb_poll_for_event(globalconf.connection)))
    {
        /* We will treat mouse events later.
         * We cannot afford to treat all mouse motion events,
         * because that would be too much CPU intensive, so we just
         * take the last we get after a bunch of events. */
        if(XCB_EVENT_RESPONSE_TYPE(event) == XCB_MOTION_NOTIFY)
        {
            p_delete(&mouse);
            mouse = event;
        }
        else
        {
            xcb_event_handle(&globalconf.evenths, event);
            p_delete(&event);
        }
    }

    if(mouse)
    {
        xcb_event_handle(&globalconf.evenths, mouse);
        p_delete(&mouse);
    }
}

static void
a_xcb_io_cb(EV_P_ ev_io *w, int revents)
{
    /* empty */
}

/** Startup Error handler to check if another window manager
 * is already running.
 * \param data Additional optional parameters data.
 * \param c X connection.
 * \param error Error event.
 */
static int __attribute__ ((noreturn))
xerrorstart(void * data __attribute__ ((unused)),
            xcb_connection_t * c  __attribute__ ((unused)),
            xcb_generic_error_t * error __attribute__ ((unused)))
{
    fatal("another window manager is already running");
}

static void
signal_fatal(int signum)
{
    buffer_t buf;
    backtrace_get(&buf);
    fatal("dumping backtrace\n%s", buf.s);
}

/** Function to exit on some signals.
 * \param w the signal received, unused
 * \param revents currently unused
 */
static void
exit_on_signal(EV_P_ ev_signal *w, int revents)
{
    ev_unloop(EV_A_ 1);
}

void
awesome_restart(void)
{
    awesome_atexit();
    a_exec(globalconf.argv);
}

/** Function to restart awesome on some signals.
 * \param w the signal received, unused
 * \param revents Currently unused
 */
static void
restart_on_signal(EV_P_ ev_signal *w, int revents)
{
    awesome_restart();
}

/** \brief awesome xerror function.
 * There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's).  Other types of errors call Xlibs
 * default error handler, which may call exit.
 * \param data Currently unused.
 * \param c The connection to the X server.
 * \param e The error event.
 * \return 0 if no error, or xerror's xlib return status.
 */
static int
xerror(void *data __attribute__ ((unused)),
       xcb_connection_t *c __attribute__ ((unused)),
       xcb_generic_error_t *e)
{
    /* ignore this */
    if(e->error_code == XCB_EVENT_ERROR_BAD_WINDOW
       || (e->error_code == XCB_EVENT_ERROR_BAD_MATCH
           && e->major_code == XCB_SET_INPUT_FOCUS)
       || (e->error_code == XCB_EVENT_ERROR_BAD_VALUE
           && e->major_code == XCB_KILL_CLIENT)
       || (e->major_code == XCB_CONFIGURE_WINDOW
           && e->error_code == XCB_EVENT_ERROR_BAD_MATCH))
        return 0;

    warn("X error: request=%s, error=%s",
         xcb_event_get_request_label(e->major_code),
         xcb_event_get_error_label(e->error_code));

    return 0;
}

/** Print help and exit(2) with given exit_code.
 * \param exit_code The exit code.
 */
static void __attribute__ ((noreturn))
exit_help(int exit_code)
{
    FILE *outfile = (exit_code == EXIT_SUCCESS) ? stdout : stderr;
    fprintf(outfile,
"Usage: awesome [OPTION]\n\
  -h, --help             show help\n\
  -v, --version          show version\n\
  -c, --config FILE      configuration file to use\n\
  -k, --check            check configuration file syntax\n");
    exit(exit_code);
}

/** Hello, this is main.
 * \param argc Who knows.
 * \param argv Who knows.
 * \return EXIT_SUCCESS I hope.
 */
int
main(int argc, char **argv)
{
    char *confpath = NULL;
    int xfd, i, screen_nbr, opt, colors_nbr;
    xcolor_init_request_t colors_reqs[2];
    ssize_t cmdlen = 1;
    xdgHandle xdg;
    static struct option long_options[] =
    {
        { "help",    0, NULL, 'h' },
        { "version", 0, NULL, 'v' },
        { "config",  1, NULL, 'c' },
        { "check",   0, NULL, 'k' },
        { NULL,      0, NULL, 0 }
    };

    /* event loop watchers */
    ev_io xio    = { .fd = -1 };
    ev_check xcheck;
    ev_prepare a_refresh;
    ev_signal sigint;
    ev_signal sigterm;
    ev_signal sighup;

    /* clear the globalconf structure */
    p_clear(&globalconf, 1);
    globalconf.keygrabber = LUA_REFNIL;
    globalconf.mousegrabber = LUA_REFNIL;

    /* save argv */
    for(i = 0; i < argc; i++)
        cmdlen += a_strlen(argv[i]) + 1;

    globalconf.argv = p_new(char, cmdlen);
    a_strcpy(globalconf.argv, cmdlen, argv[0]);

    for(i = 1; i < argc; i++)
    {
        a_strcat(globalconf.argv, cmdlen, " ");
        a_strcat(globalconf.argv, cmdlen, argv[i]);
    }

    /* Text won't be printed correctly otherwise */
    setlocale(LC_CTYPE, "");

    /* Get XDG basedir data */
    xdgInitHandle(&xdg);

    /* init lua */
    luaA_init(&xdg);

    /* check args */
    while((opt = getopt_long(argc, argv, "vhkc:",
                             long_options, NULL)) != -1)
        switch(opt)
        {
          case 'v':
            eprint_version();
            break;
          case 'h':
            exit_help(EXIT_SUCCESS);
            break;
          case 'k':
            if(!luaA_parserc(&xdg, confpath, false))
            {
                fprintf(stderr, "✘ Configuration file syntax error.\n");
                return EXIT_FAILURE;
            }
            else
            {
                fprintf(stderr, "✔ Configuration file syntax OK.\n");
                return EXIT_SUCCESS;
            }
          case 'c':
            if(a_strlen(optarg))
                confpath = a_strdup(optarg);
            else
                fatal("-c option requires a file name");
            break;
        }

    globalconf.loop = ev_default_loop(0);
    ev_timer_init(&globalconf.timer, &luaA_on_timer, 0., 0.);

    /* register function for signals */
    ev_signal_init(&sigint, exit_on_signal, SIGINT);
    ev_signal_init(&sigterm, exit_on_signal, SIGTERM);
    ev_signal_init(&sighup, restart_on_signal, SIGHUP);
    ev_signal_start(globalconf.loop, &sigint);
    ev_signal_start(globalconf.loop, &sigterm);
    ev_signal_start(globalconf.loop, &sighup);
    ev_unref(globalconf.loop);
    ev_unref(globalconf.loop);
    ev_unref(globalconf.loop);

    struct sigaction sa = { .sa_handler = signal_fatal, .sa_flags = 0 };
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, 0);

    /* X stuff */
    globalconf.connection = xcb_connect(NULL, &globalconf.default_screen);
    if(xcb_connection_has_error(globalconf.connection))
        fatal("cannot open display");

    /* check for xtest extension */
    const xcb_query_extension_reply_t *xtest_query;
    xtest_query = xcb_get_extension_data(globalconf.connection, &xcb_test_id);
    globalconf.have_xtest = xtest_query->present;

    /* initialize dbus */
    a_dbus_init();

    /* Grab server */
    xcb_grab_server(globalconf.connection);
    xcb_flush(globalconf.connection);

    /* Get the file descriptor corresponding to the X connection */
    xfd = xcb_get_file_descriptor(globalconf.connection);
    ev_io_init(&xio, &a_xcb_io_cb, xfd, EV_READ);
    ev_io_start(globalconf.loop, &xio);
    ev_check_init(&xcheck, &a_xcb_check_cb);
    ev_check_start(globalconf.loop, &xcheck);
    ev_unref(globalconf.loop);
    ev_prepare_init(&a_refresh, &a_refresh_cb);
    ev_prepare_start(globalconf.loop, &a_refresh);
    ev_unref(globalconf.loop);

    /* Allocate a handler which will holds all errors and events */
    xcb_event_handlers_init(globalconf.connection, &globalconf.evenths);
    xutil_error_handler_catch_all_set(&globalconf.evenths, xerrorstart, NULL);

    for(screen_nbr = 0;
        screen_nbr < xcb_setup_roots_length(xcb_get_setup(globalconf.connection));
        screen_nbr++)
    {
        const uint32_t select_input_val = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT;

        /* This causes an error if some other window manager is running */
        xcb_change_window_attributes(globalconf.connection,
                                     xutil_screen_get(globalconf.connection, screen_nbr)->root,
                                     XCB_CW_EVENT_MASK, &select_input_val);
    }

    /* Need to xcb_flush to validate error handler */
    xcb_aux_sync(globalconf.connection);

    /* Process all errors in the queue if any */
    xcb_event_poll_for_event_loop(&globalconf.evenths);

    /* Set the default xerror handler */
    xutil_error_handler_catch_all_set(&globalconf.evenths, xerror, NULL);

    /* Allocate the key symbols */
    globalconf.keysyms = xcb_key_symbols_alloc(globalconf.connection);
    xcb_get_modifier_mapping_cookie_t xmapping_cookie =
        xcb_get_modifier_mapping_unchecked(globalconf.connection);

    /* init atom cache */
    atoms_init(globalconf.connection);

    /* init screens information */
    screen_scan();

    /* init default font and colors */
    colors_reqs[0] = xcolor_init_unchecked(&globalconf.colors.fg,
                                           "black", sizeof("black") - 1);

    colors_reqs[1] = xcolor_init_unchecked(&globalconf.colors.bg,
                                           "white", sizeof("white") - 1);

    globalconf.font = draw_font_new("sans 8");

    for(colors_nbr = 0; colors_nbr < 2; colors_nbr++)
        xcolor_init_reply(colors_reqs[colors_nbr]);

    xutil_lock_mask_get(globalconf.connection, xmapping_cookie,
                        globalconf.keysyms, &globalconf.numlockmask,
                        &globalconf.shiftlockmask, &globalconf.capslockmask,
                        &globalconf.modeswitchmask);

    /* do this only for real screen */
    for(screen_nbr = 0;
        screen_nbr < xcb_setup_roots_length(xcb_get_setup(globalconf.connection));
        screen_nbr++)
    {
        /* select for events */
        const uint32_t change_win_vals[] =
        {
            XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
                | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW
                | XCB_EVENT_MASK_STRUCTURE_NOTIFY
                | XCB_EVENT_MASK_PROPERTY_CHANGE
                | XCB_EVENT_MASK_BUTTON_PRESS
                | XCB_EVENT_MASK_BUTTON_RELEASE
                | XCB_EVENT_MASK_FOCUS_CHANGE
        };

        xcb_change_window_attributes(globalconf.connection,
                                     xutil_screen_get(globalconf.connection, screen_nbr)->root,
                                     XCB_CW_EVENT_MASK,
                                     change_win_vals);
        ewmh_init(screen_nbr);
        systray_init(screen_nbr);
    }

    /* init spawn (sn) */
    spawn_init();

    /* Parse and run configuration file */
    if (!luaA_parserc(&xdg, confpath, true))
        fatal("couldn't find any rc file");

    p_delete(&confpath);

    xdgWipeHandle(&xdg);

    /* scan existing windows */
    scan();

    /* process all errors in the queue if any */
    xcb_event_poll_for_event_loop(&globalconf.evenths);
    a_xcb_set_event_handlers();
    a_xcb_set_property_handlers();

    /* we will receive events, stop grabbing server */
    xcb_ungrab_server(globalconf.connection);
    xcb_flush(globalconf.connection);

    /* main event loop */
    ev_loop(globalconf.loop, 0);

    /* cleanup event loop */
    ev_ref(globalconf.loop);
    ev_check_stop(globalconf.loop, &xcheck);
    ev_ref(globalconf.loop);
    ev_prepare_stop(globalconf.loop, &a_refresh);
    ev_ref(globalconf.loop);
    ev_io_stop(globalconf.loop, &xio);

    awesome_atexit();

    return EXIT_SUCCESS;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
