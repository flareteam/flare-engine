### Attribute description
Attribute descriptions are in the following format: section.attributename, `valuetype`, short attribute description

#### Valuetype syntax

`[val1, ...]`, Multiple values between a set of square brackets denotes possible types/values for a single field.

`"..."`, Anything between double quotes is a string literal. This is mostly used in conjunction with the above square brackets syntax.

`list(...)`, Lists are a series of values that can repeat on a single line, separated by commas/semicolons. Example of list(int, bool): "1,false;2,true"

`repeatable(...)`, Repeatable keys can be used for multiple lines. An example of this would be an item with multiple "bonus" lines.

#### List of available valuetypes

`bool`, a string value of `true` or `false`

`int`, a signed integer value

`string`, a text string value

`float`, a floating point number

`item_id`, specifies an integer item identifer (greater than 0)

`power_id`, specifies an integer power identifer (greater than 0)

`icon_id`, specifies an integer icon identifer (greater than or equal to 0)

`point`, defined as: `int, int : X, Y`

`rectangle`, defined as: `int, int, int, int : X, Y, Width, Height`

`filename`, a string path to a file relative to the base of the mod directory (e.g. "animations/hero.txt")

`color`, defined as: `int, int, int : Red, Green, Blue`

`predefined_string`, same as a string, but uses a value defined elsewhere

`alignment`, defined as: `["topleft", "top", "topright", "left", "center", "right", "bottomleft", "bottom", "bottomright"]`

`direction`, defined as: `["N", "NE", "E", "SE", "S", "SW", "W", "NW", int]`. If defined as an integer, the value must be between 0-7 inclusive.

`duration`, durations can be specified in seconds and milliseconds with integer suffix s, ms (eg. 20s, 20000ms)

`label`, defined as: `"hidden"` **or** `int, int, ["left", "right", "center"], ["top", "center", "bottom"], string : X, Y, Justify, Vertical Align, Font style`. The font style can be any style defined in engine/font\_settings.txt.

`loot`, defined as: `filename` **or** `["currency", item_id], ["fixed", float], int, int : Item, Drop chance, Min quantity, Max quantity`. There is a limitation when defining as part of a list(...): filenames can only be used in the first list element.

`version`, defined as: a string of three numbers, separated by dots (e.g. "1.2.03")

`raw`, This is plain text, including line breaks. It is used only for map layer data.

