local fontfile = io.open(arg[1], "r")
if not fontfile then error ("Unable to open ".. arg[1]) end

local font = { }
local charWidth = 5

for line in fontfile:lines() do
  local nrChars = math.floor (#line / charWidth)
  
  for charNumber = 1, nrChars do
    local char = font[charNumber]
    
    if not char then
      char = { }
      font[charNumber] = char
    end

    local begin = (charNumber - 1) * charWidth + 1
    
    local thisdef = 0 
   
    for i = begin, begin+4 do
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
print ("// two rows per byte, lower nybble first")
print ("uint8_t font[10][4] = {")

for _, c in pairs(font) do
  local str = {}
  for i = 1, #c,2 do
    table.insert(str, "0x"..c[i+1]..c[i])
  end
  print ("  { " .. table.concat(str, ", ") .. "}, ")
end

print ("};")
