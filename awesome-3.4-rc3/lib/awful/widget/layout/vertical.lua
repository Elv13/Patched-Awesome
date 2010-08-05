-------------------------------------------------
-- @author Gregor Best <farhaven@googlemail.com>
-- @copyright 2009 Gregor Best
-- @release v3.4-rc3
-------------------------------------------------

-- Grab environment
local ipairs = ipairs
local type = type
local table = table
local math = math
local util = require("awful.util")
local default = require("awful.widget.layout.default")
local margins = awful.widget.layout.margins

--- Vertical widget layout
module("awful.widget.layout.vertical")

local function vertical(direction, bounds, widgets, screen)
    local geometries = { }
    local y = 0

    -- we are only interested in tables and widgets
    local keys = util.table.keys_filter(widgets, "table", "widget")

    for _, k in ipairs(keys) do
        local v = widgets[k]
        if type(v) == "table" then
	  v.height = 50
            local layout = v.layout or default
            if margins[v] then
                bounds.height = 50
                bounds.width = bounds.width - (margins[v].left or 0) - (margins[v].right or 0)
            end
            local g = layout(bounds, v, screen)
            if margins[v] then
                y = y + (margins[v].top or 0)
            end
            for _, v in ipairs(g) do
                v.y = v.y + y
                v.x = v.x + (margins[v] and (margins[v].left and margins[v].left or 0) or 0)
                table.insert(geometries, v)
            end
            bounds = g.free
            if margins[v] then
                y = y + g.free.y + (margins[v].bottom or 0)
                bounds.height = 50
            else
                y = y + g.free.y
            end
	    
        elseif type(v) == "widget" then
            local g
            if v.visible then
                g = v:extents(screen)
                if margins[v] then
                    g.height = g.height + (margins[v].top or 0) + (margins[v].bottom or 0)
                    g.width = g.width + (margins[v].left or 0) + (margins[v].right or 0)
                end
            else
                g = {
                    height  = 0,
                    width = 0,
                }
            end

            if v.resize and g.height > 0 and g.width > 0 then
                local ratio = g.height / g.width
                g.height = math.floor(bounds.width * ratio)
                g.width = bounds.width
            end

            if g.height > bounds.height then
                g.height = bounds.height
            end
            g.width = bounds.width

            if margins[v] then
                g.x = (margins[v].left or 0)
            else
                g.x = 0
            end

            if direction == "topbottom" then
                if margins[v] then
                    g.y = y + (margins[v].top or 0)
                else
                    g.y = y
                end
                y = y + g.height
            else
                if margins[v] then
                    g.y = y + bounds.height - g.height + (margins[v].top or 0)
                else
                    g.y = y + bounds.height - g.height
                end
            end
            bounds.height = bounds.height - g.height

            table.insert(geometries, g)
        end
    end

    geometries.free = util.table.clone(bounds)
    geometries.free.y = y
    geometries.free.x = 0

    return geometries
end

function flex(bounds, widgets, screen)
    local geometries = {
        free = util.table.clone(bounds)
    }

    local y = 0

    -- we are only interested in tables and widgets
    local keys = util.table.keys_filter(widgets, "table", "widget")
    local nelements = 0
    for _, k in ipairs(keys) do
        local v = widgets[k]
        if type(v) == "table" then
            nelements = nelements + 1
        else
            local e = v:extents()
            if v.visible and e.width > 0 and e.height > 0 then
                nelements = nelements + 1
            end
        end
    end
    if nelements == 0 then return geometries end
    local height = math.floor(bounds.height / nelements)

    for _, k in ipairs(keys) do
        local v = widgets[k]
        if type(v) == "table" then
            local layout = v.layout or default
            -- we need to modify the height a bit because vertical layouts always span the
            -- whole height
            nbounds = util.table.clone(bounds)
            nbounds.height = height
            local g = layout(nbounds, v, screen)
            for _, w in ipairs(g) do
                w.y = w.y + y
                table.insert(geometries, w)
            end
            y = y + height
        elseif type(v) == "widget" then
            local g
            if v.visible then
                g = v:extents(screen)
            else
                g = {
                    ["width"] = 0,
                    ["height"] = 0
                }
            end

            g.ratio = 1
            if g.height > 0 and g.width > 0 then
                g.ratio = g.width / g.height
            end
            g.height = height
            if v.resize then
                g.width = g.height * g.ratio
            end
            g.width = math.min(g.width, bounds.width)
            geometries.free.x = math.max(geometries.free.x, g.width)

            g.x = 0
            g.y = y
            y = y + g.height
            bounds.height = bounds.height - g.height

            table.insert(geometries, g)
        end
    end

    local maxw = 0
    local maxx = 0
    for _, v in ipairs(geometries) do
        if v.width > maxw then maxw = v.width end
        if v.x > maxx then maxx = v.x end
    end

    geometries.free.width = geometries.free.width - maxw
    geometries.free.x = geometries.free.x + maxw

    geometries.free.height = nelements * height
    geometries.free.y = 0

    return geometries
end

function topbottom(...)
    return vertical("topbottom", ...)
end

function bottomtopt(...)
    return vertical("bottomtop", ...)
end
