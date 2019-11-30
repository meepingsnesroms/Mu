# get PC address for DAL function

import ida_kernwin


DAL_START_IN_ROM = 0x0009B130
DAL_END_IN_ROM = 0x000C7EEB


def pcFromDalAddr():
   dalAddr = ida_kernwin.ask_addr(0x00000000, "DAL ADDR:")
   ida_kernwin.info("PC:" + "0x{0:0{1}X}".format(0x20000000 + DAL_START_IN_ROM + dalAddr, 8))
   

pcFromDalAddr()