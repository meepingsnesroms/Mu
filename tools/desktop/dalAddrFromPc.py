# jumps to DAL address when given execution address

import ida_kernwin


DAL_START_IN_ROM = 0x0009B130
DAL_END_IN_ROM = 0x000C7EEB


def dalAddrFromPc():
   pc = ida_kernwin.ask_addr(0x00000000, "PC:")
   
   if type(pc) is not int:
      warning("Invalid input!")
      return
   
   if (pc & 0xF0000000) == 0x20000000:
      #MMU mapped
      pc -= 0x20000000
      
      if pc >= DAL_START_IN_ROM:
         pc -= DAL_START_IN_ROM
         
      if pc > DAL_END_IN_ROM:
         warning("Not in DAL!")
         return
         
   ida_kernwin.jumpto(pc)
   

dalAddrFromPc()