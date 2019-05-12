#!/usr/bin/env python3

env=Environment(CPPPATH=['#external/dynamic','#external/static'],CXXFLAGS="-std=c++17")

Export('env')

#0 is the obj_list
#1 is the src_list for formatting
#2 is the hdr_list for formatting
file_list = SConscript('source/SConscript')

program = env.Program('devoured', file_list[0])
