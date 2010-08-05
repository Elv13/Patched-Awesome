--- awesome wibox API
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2008-2009 Julien Danjou
module("wibox")

--- Wibox object.
-- @field screen Screen number.
-- @field client The client attached to (titlebar only).
-- @field border_width Border width.
-- @field border_color Border color.
-- @field align The alignment (titlebar only).
-- @field fg Foreground color.
-- @field bg Background color.
-- @field bg_image Background image.
-- @field position The position (titlebar only).
-- @field ontop On top of other windows.
-- @field cursor The mouse cursor.
-- @field visible Visibility.
-- @field orientation The drawing orientation: east, north or south.
-- @field widgets A table with all widgets drawn on this wibox.
-- @field opacity The opacity of the wibox, between 0 and 1.
-- @field x The x coordinates.
-- @field y The y coordinates.
-- @field width The width of the wibox.
-- @field height The height of the wibox.
-- @field shape_bounding Image describing the window's border shape.
-- @field shape_clip Image describing the window's content shape.
-- @class table
-- @name wibox

--- Get or set mouse buttons bindings to a wibox.
-- @param buttons_table A table of buttons objects, or nothing.
-- @name buttons
-- @class function

--- Get or set wibox struts.
-- @param strut A table with new strut, or nothing
-- @return The wibox strut in a table.
-- @name struts
-- @class function

--- Get or set wibox geometry. That's the same as accessing or setting the x, y, width or height
-- properties of a wibox.
-- @param A table with coordinates to modify.
-- @return A table with wibox coordinates and geometry.
-- @name geometry
-- @class function

--- Add a signal.
-- @param name A signal name.
-- @param func A function to call when the signal is emitted.
-- @name add_signal
-- @class function

--- Remove a signal.
-- @param name A signal name.
-- @param func A function to remove.
-- @name remove_signal
-- @class function

--- Emit a signal.
-- @param name A signal name.
-- @param ... Various arguments, optional.
-- @name emit_signal
-- @class function
