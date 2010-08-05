--- awesome screen API
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Julien Danjou
module("screen")

--- Screen is a table where indexes are screen number. You can use screen[1]
-- to get access to the first screen, etc. Each screen has a set of properties.
-- @field geometry The screen coordinates. Immutable.
-- @field workarea The screen workarea.
-- @field index The screen number.
-- @class table
-- @name screen

--- Get the number of screen.
-- @param -
-- @return The screen count, at least 1.
-- @name count
-- @class function

--- Add a signal to a screen.
-- @param name A signal name.
-- @param func A function to call when the signal is emitted.
-- @name add_signal
-- @class function

--- Remove a signal to a screen.
-- @param name A signal name.
-- @param func A function to remove
-- @name remove_signal
-- @class function

--- Emit a signal to a screen.
-- @param name A signal name.
-- @param ... Various arguments, optional.
-- @name emit_signal
-- @class function

--- Get or set screen tags.
-- @param tags_table None or a table of tags to set to the screen.
-- The table must contains at least one tag.
-- @name tags
-- @class function
