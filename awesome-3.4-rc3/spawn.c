/*
 * spawn.c - Lua configuration management
 *
 * Copyright © 2009 Julien Danjou <julien@danjou.info>
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

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <glib/gspawn.h>

#include "spawn.h"
#include "screen.h"
#include "luaa.h"
#include "event.h"

/** 20 seconds timeout */
#define AWESOME_SPAWN_TIMEOUT 20.0

/** Wrapper for unrefing startup sequence.
 */
static inline void
a_sn_startup_sequence_unref(SnStartupSequence **sss)
{
    return sn_startup_sequence_unref(*sss);
}

DO_ARRAY(SnStartupSequence *, SnStartupSequence, a_sn_startup_sequence_unref)

/** The array of startup sequence running */
SnStartupSequence_array_t sn_waits;

/** Remove a SnStartupSequence pointer from an array and forget about it.
 * \param s The startup sequence to found, remove and unref.
 * \return True if found and removed.
 */
static inline bool
spawn_sequence_remove(SnStartupSequence *s)
{
    for(int i = 0; i < sn_waits.len; i++)
        if(sn_waits.tab[i] == s)
        {
            SnStartupSequence_array_take(&sn_waits, i);
            sn_startup_sequence_unref(s);
            return true;
        }
    return false;
}

static void
spawn_monitor_timeout(struct ev_loop *loop, ev_timer *w, int revents)
{
    if(spawn_sequence_remove(w->data))
    {
         signal_t *sig = signal_array_getbyid(&global_signals,
                                              a_strhash((const unsigned char *) "spawn::timeout"));
         if(sig)
         {
             /* send a timeout signal */
             lua_createtable(globalconf.L, 0, 2);
             lua_pushstring(globalconf.L, sn_startup_sequence_get_id(w->data));
             lua_setfield(globalconf.L, -2, "id");
             foreach(func, sig->sigfuncs)
             {
                 lua_pushvalue(globalconf.L, -1);
                 luaA_object_push(globalconf.L, (void *) *func);
                 luaA_dofunction(globalconf.L, 1, 0);
             }
             lua_pop(globalconf.L, 1);
         }
    }
    sn_startup_sequence_unref(w->data);
    p_delete(&w);
}

static void
spawn_monitor_event(SnMonitorEvent *event, void *data)
{
    SnStartupSequence *sequence = sn_monitor_event_get_startup_sequence(event);
    SnMonitorEventType event_type = sn_monitor_event_get_type(event);

    lua_createtable(globalconf.L, 0, 2);
    lua_pushstring(globalconf.L, sn_startup_sequence_get_id(sequence));
    lua_setfield(globalconf.L, -2, "id");

    const char *event_type_str = NULL;

    switch(event_type)
    {
      case SN_MONITOR_EVENT_INITIATED:
        /* ref the sequence for the array */
        sn_startup_sequence_ref(sequence);
        SnStartupSequence_array_append(&sn_waits, sequence);
        event_type_str = "spawn::initiated";

        /* Add a timeout function so we do not wait for this event to complete
         * for ever */
        struct ev_timer *ev_timeout = p_new(struct ev_timer, 1);
        ev_timer_init(ev_timeout, spawn_monitor_timeout, AWESOME_SPAWN_TIMEOUT, 0.);
        /* ref the sequence for the callback event */
        sn_startup_sequence_ref(sequence);
        ev_timeout->data = sequence;
        ev_timer_start(globalconf.loop, ev_timeout);
        break;
      case SN_MONITOR_EVENT_CHANGED:
        event_type_str = "spawn::change";
        break;
      case SN_MONITOR_EVENT_COMPLETED:
        event_type_str = "spawn::completed";
        break;
      case SN_MONITOR_EVENT_CANCELED:
        event_type_str = "spawn::canceled";
        break;
    }

    /* common actions */
    switch(event_type)
    {
      case SN_MONITOR_EVENT_INITIATED:
      case SN_MONITOR_EVENT_CHANGED:
        {
            const char *s = sn_startup_sequence_get_name(sequence);
            if(s)
            {
                lua_pushstring(globalconf.L, s);
                lua_setfield(globalconf.L, -2, "name");
            }

            if((s = sn_startup_sequence_get_description(sequence)))
            {
                lua_pushstring(globalconf.L, s);
                lua_setfield(globalconf.L, -2, "description");
            }

            lua_pushnumber(globalconf.L, sn_startup_sequence_get_workspace(sequence));
            lua_setfield(globalconf.L, -2, "workspace");

            if((s = sn_startup_sequence_get_binary_name(sequence)))
            {
                lua_pushstring(globalconf.L, s);
                lua_setfield(globalconf.L, -2, "binary_name");
            }

            if((s = sn_startup_sequence_get_icon_name(sequence)))
            {
                lua_pushstring(globalconf.L, s);
                lua_setfield(globalconf.L, -2, "icon_name");
            }

            if((s = sn_startup_sequence_get_wmclass(sequence)))
            {
                lua_pushstring(globalconf.L, s);
                lua_setfield(globalconf.L, -2, "wmclass");
            }
        }
        break;
      case SN_MONITOR_EVENT_COMPLETED:
      case SN_MONITOR_EVENT_CANCELED:
        spawn_sequence_remove(sequence);
        break;
    }

    /* send the signal */
    signal_t *sig = signal_array_getbyid(&global_signals,
                                         a_strhash((const unsigned char *) event_type_str));

    if(sig)
    {
        foreach(func, sig->sigfuncs)
        {
            lua_pushvalue(globalconf.L, -1);
            luaA_object_push(globalconf.L, (void *) *func);
            luaA_dofunction(globalconf.L, 1, 0);
        }
        lua_pop(globalconf.L, 1);
    }
}

/** Tell the spawn module that an app has been started.
 * \param c The client that just started.
 * \param startup_id The startup id of the started application.
 */
void
spawn_start_notify(client_t *c, const char * startup_id)
{
    foreach(_seq, sn_waits)
    {
        SnStartupSequence *seq = *_seq;
        bool found = false;
        const char *seqid = sn_startup_sequence_get_id(seq);

        if(!a_strcmp(seqid, startup_id))
            found = true;
        else
        {
            const char *seqclass = sn_startup_sequence_get_wmclass(seq);
            if(!a_strcmp(seqclass, c->class) || !a_strcmp(seqclass, c->instance))
                found = true;
            else
            {
                const char *seqbin = sn_startup_sequence_get_binary_name(seq);
                if(!a_strcasecmp(seqbin, c->class) || !a_strcasecmp(seqbin, c->instance))
                    found = true;
            }
        }

        if(found)
        {
            sn_startup_sequence_complete(seq);
            break;
        }
    }
}

/** Initialize program spawner.
 */
void
spawn_init(void)
{
    globalconf.sndisplay = sn_xcb_display_new(globalconf.connection, NULL, NULL);

    const int screen_max = xcb_setup_roots_length(xcb_get_setup(globalconf.connection));

    for(int screen = 0; screen < screen_max; screen++)
        globalconf.screens.tab[screen].snmonitor = sn_monitor_context_new(globalconf.sndisplay,
                                                                          screen,
                                                                          spawn_monitor_event,
                                                                          NULL, NULL);
}

static void
spawn_launchee_timeout(struct ev_loop *loop, ev_timer *w, int revents)
{
    sn_launcher_context_complete(w->data);
    sn_launcher_context_unref(w->data);
    p_delete(&w);
}

/** This callback is executed in the child process after fork(). It clears the
 * signal mask. Without this, child processes would be started with e.g.
 * SIGINT blocked if we are compiled against libev 3.8 or newer.
 * BEWARE: Pending signals are inherited by child processes. This might mean
 * that we receive a signal that was meant for awesome after clearing the signal
 * mask! Sadly, we can't do anything about this.
 * \param user_data Not used.
 */
static void
spawn_child_callback(gpointer user_data)
{
    sigset_t empty;

    sigemptyset(&empty);
    sigprocmask(SIG_SETMASK, &empty, NULL);
}

/** Spawn a command.
 * \param command_line The command line to launch.
 * \param error A error pointer to fill with the possible error from
 * g_spawn_async.
 * \return g_spawn_async value.
 */
static gboolean
spawn_command(const gchar *command_line, GError **error)
{
    gboolean retval;
    gchar **argv = 0;

    if(!g_shell_parse_argv(command_line, NULL, &argv, error))
        return FALSE;

    retval = g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH,
                           spawn_child_callback, NULL, NULL, error);
    g_strfreev (argv);

    return retval;
}

/** Spawn a program.
 * This function is multi-head (Zaphod) aware and will set display to
 * the right screen according to mouse position.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack
 * \luastack
 * \lparam The command to launch.
 * \lparam Use startup-notification, true or false, default to true.
 * \lparam The optional screen number to spawn the command on.
 * \lreturn Nothing if launch was successful, an error string otherwise.
 */
int
luaA_spawn(lua_State *L)
{
    char *host, newdisplay[128];
    const char *cmd;
    bool use_sn = true;
    int screen = 0, screenp, displayp;

    if(lua_gettop(L) >= 2)
        use_sn = luaA_checkboolean(L, 2);

    if(lua_gettop(L) == 3)
    {
        screen = luaL_checknumber(L, 3) - 1;
        luaA_checkscreen(screen);
    }

    cmd = luaL_checkstring(L, 1);

    if(!globalconf.xinerama_is_active)
    {
        xcb_parse_display(NULL, &host, &displayp, &screenp);
        snprintf(newdisplay, sizeof(newdisplay), "%s:%d.%d", host, displayp, screen);
        setenv("DISPLAY", newdisplay, 1);
        p_delete(&host);
    }

    SnLauncherContext *context = NULL;
    if(use_sn)
    {
        char *cmdname, *space;
        const char *first_no_space_char = cmd;
        /* Look for the first char which is not space */
        while(*first_no_space_char && *first_no_space_char == ' ')
            first_no_space_char++;
        /* Look for space in the string to get the command name. */
        if((space = strchr(first_no_space_char, ' ')))
            cmdname = a_strndup(cmd, space - cmd);
        else
            cmdname = a_strdup(cmd);

        context = sn_launcher_context_new(globalconf.sndisplay, screen_virttophys(screen));
        sn_launcher_context_set_name(context, "awesome");
        sn_launcher_context_set_description(context, "awesome spawn");
        sn_launcher_context_set_binary_name(context, cmdname);
        sn_launcher_context_initiate(context, "awesome", cmdname, XCB_CURRENT_TIME);
        p_delete(&cmdname);

        /* app will have AWESOME_SPAWN_TIMEOUT seconds to complete,
         * or the timeout function will terminate the launch sequence anyway */
        struct ev_timer *ev_timeout = p_new(struct ev_timer, 1);
        ev_timer_init(ev_timeout, spawn_launchee_timeout, AWESOME_SPAWN_TIMEOUT, 0.);
        ev_timeout->data = context;
        ev_timer_start(globalconf.loop, ev_timeout);
        sn_launcher_context_setup_child_process(context);
    }

    GError *error = NULL;
    if(!spawn_command(cmd, &error))
    {
        /* push error on stack */
        lua_pushstring(L, error->message);
        g_error_free(error);
        if(context)
            sn_launcher_context_complete(context);
        return 1;
    }

    return 0;
}

/** Spawn a program. This works exactly as system() does, but clears the signal mask.
 * \param arg The shell command to execute.
 * \return The return status of the program.
 */
int
spawn_system(const char *arg)
{
    GError *error;
    gboolean retval;
    gint status;
    static const char *shell = NULL;
    if(!shell && !(shell = getenv("SHELL")))
        shell = "/bin/sh";
    char *argv[] = { (char *) shell, (char *) "-c", (char *) arg, NULL };

    retval = g_spawn_sync(NULL, argv, NULL, 0, spawn_child_callback,
                          NULL, NULL, NULL, &status, &error);

    if (retval)
        return status;

    g_error_free(error);
    return -1;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
