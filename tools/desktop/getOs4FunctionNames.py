import sys
import idaapi
import idautils
import idc


def makeString(location):
   string = ''
   count = 0
   
   while True:
      if count >= 50:
         break
      
      byte = ida_bytes.get_byte(location + count)
      if byte == 0:
         break

      string += chr(byte)
      count += 1
      
   return string

#scan from ROM start to ROM end
for func in idautils.Functions(0x10000000, 0x103D7C00):
   str = makeString(idc.get_func_attr(func, FUNCATTR_END) + 1) + ('_%X' % idc.get_func_attr(func, FUNCATTR_START))
   idc.set_name(idc.get_func_attr(func, FUNCATTR_START), str, SN_NOCHECK)
   print str
