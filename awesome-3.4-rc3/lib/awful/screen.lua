---------------------------------------------------------------------------
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008 Julien Danjou
-- @release v3.4-rc3
---------------------------------------------------------------------------

-- Grab environment we need
local capi =
{
    mouse = mouse,
    screen = screen,
    client = client
}
local util = require("awful.util")
local client = require("awful.client")

--- Screen module for awful
module("awful.screen")

local data = {}
data.padding = {}

--- Give the focus to a screen, and move pointer.
-- @param screen Screen number.
function focus(screen)
    if screen > capi.screen.count() then screen = capi.mouse.screen end
    local c = client.focus.history.get(screen, 0)
    if c then capi.client.focus = c end
    -- Move the mouse on the screen
    capi.mouse.screen = screen
end

--- Give the focus to a screen, and move pointer, but relative to the current
-- focused screen.
-- @param i Value to add to the current focused screen index. 1 will focus next
-- screen, -1 would focus the previous one.
function focus_relative(i)
    return focus(util.cycle(capi.screen.count(), capi.mouse.screen + i))
end

--- Get or set the screen padding.
-- @param i The screen number.
-- @param padding The padding, an table with 'top', 'left', 'right' and/or
-- 'bottom'. Can be nil if you only want to retrieve padding
function padding(i, padding)
    if padding then
        data.padding[i] = padding
        capi.screen[i]:emit_signal("padding")
    end
    return data.padding[i]
end

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
