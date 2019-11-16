# jumps to DAL address when given execution address

import idc
import ida_bytes
import ctypes


DAL_JUMP_TABLE = 0x00000024


def getBranchOffset(opcode):
   offset = opcode & 0x007FFFFF;
   
   if opcode & 0x00800000:
      offset |= 0xFF800000;
      
   offset = ctypes.c_int(offset).value;
   
   offset *= 4
   offset += 8 # account for PC weirdness
   
   return ctypes.c_int(offset).value

def writeAddrName(addr, name):
   idc.set_name(addr, name + "Entrypoint", SN_NOCHECK)
   idc.set_name(addr + getBranchOffset(ida_bytes.get_dword(addr)), name, SN_NOCHECK)
   print hex(addr + getBranchOffset(ida_bytes.get_dword(addr)))

def applyHalTable():
   with open("C:/Users/Emily/Desktop/Projects/PalmOSEmulator/halTable.txt", 'r') as tblFile:
      halTable = tblFile.read().split()
   
   for x in range(0, len(halTable) / 3):
      writeAddrName(DAL_JUMP_TABLE + int(halTable[x * 3 + 1], 16), halTable[x * 3 + 2])
   

applyHalTable()