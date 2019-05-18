#!/usr/bin/env python3

#Local
import thirdparty.methods

env=Environment(CPPPATH=['#modules'],CXXFLAGS=['-std=c++17','-Wall','-Wextra','-Werror'],LIBS=['stdc++fs'])

env.__class__.add_source_files = methods.add_source_files
env.__class__.add_library = methods.add_library

Export('env')

#0 is the obj_list
#1 is the src_list for formatting
#2 is the hdr_list for formatting
SConscript('modules/SConscript')
SConscript('core/SConscript')

env.Program('#bin/devoured', ['main.cpp',env.modules_sources, env.core_sources])
