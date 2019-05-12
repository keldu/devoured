#!/usr/bin/env python3

env=Environment(CPPPATH=['#modules'],CXXFLAGS="-std=c++17")

Export('env')

#0 is the obj_list
#1 is the src_list for formatting
#2 is the hdr_list for formatting
core_list = SConscript('core/SConscript')
modules_list = SConscript('modules/SConscript')

program = env.Program('devoured', core_list[0])
