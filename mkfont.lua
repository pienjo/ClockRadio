local fontfile = io.open(arg[1], "r")
if not fontfile then error ("Unable to open ".. arg[1]) end

local font = { }
local labels = { }
local charWidth = 5
local lineIter = fontfile:lines()

-- Read in description labels
local firstLine = lineIter()
local nrChars = math.ceil (#firstLine / charWidth)

for label in string.gmatch(firstLine, "%g") do
  table.insert(labels, label)
end

for line in fontfile:lines() do
  
  for charNumber = 1, nrChars do
    local char = font[charNumber]
    
    if not char then
      char = { }
      font[charNumber] = char
    end

    local begin = (charNumber - 1) * charWidth + 1
    
    local thisdef = 0 
   
    for i = begin+4, begin+1,-1 do
      thisdef = thisdef * 2
      if line:byte(i) == 88 then
	thisdef = thisdef + 1
      end
    end
    table.insert(char, string.format("%x",thisdef))
  end
end

print ("#include <font.h>")
print ""
print ("// two rows per byte, lower nybble first, 4 bytes per char")
print ("const uint8_t PROGMEM font[] = {")

for _, c in pairs(font) do
  local str = {}
  for i = 1, #c,2 do
    table.insert(str, "0x"..c[i+1]..c[i])
  end
  local label = labels[_]

  print ("  " .. table.concat(str, ", ") .. ", // [".. (_ - 1).."] " .. (label or ""))
end

print ("};")
