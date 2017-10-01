local bitmapfile = io.open(arg[1], "r")
if not bitmapfile then error ("Unable to open ".. arg[1]) end

local lineIter = bitmapfile:lines()

print ([[
#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <avr/pgmspace.h>
#include <stdint.h>
]])

repeat
  -- Read in symbol names
  
  local symbolName = lineIter()
  
  if not symbolName then break end
  
  print ("extern const uint8_t PROGMEM Bitmap_"..symbolName.."[];")
  
  local thisDefinition = { }
  -- Read 8 lines for the declaration and ditch them.
  for lineNo=1,8 do
    local line = lineIter()
  end
  
until false 
print ""  
print("#endif")
