local bitmapfile = io.open(arg[1], "r")
if not bitmapfile then error ("Unable to open ".. arg[1]) end

local lineIter = bitmapfile:lines()

print ("#include <bitmap.h>")

repeat
  -- Read in symbol names
  
  local symbolName = lineIter()
  
  if not symbolName then break end
  print ""
  print ("const uint8_t PROGMEM Bitmap_"..symbolName.."[] = { ")
  
  local thisDefinition = { }
  -- Read 8 lines for the declaration
  for lineNo=1,8 do
    local line = lineIter()
    local characterIndex = 1
    
    for byteIdx = 1,3 do
      local bitValue = 1 
      local byteValue = 0
      for bitIdx = 8,1,-1 do
	if line:byte(characterIndex) == 88 then
	  byteValue = byteValue + bitValue
	end
	bitValue = bitValue * 2
	characterIndex = characterIndex + 1
      end
      table.insert(thisDefinition, string.format("0x%02x", byteValue))
    end
  end
  
  print ("  ", table.concat(thisDefinition, ", "))
  print ("};")
until false 
