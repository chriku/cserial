set_policy("check.auto_ignore_flags", false)
set_languages("cxx20")
add_includedirs("include","json/include/")
--add_cxflags("-fconcepts-diagnostics-depth=3",{force=true})
add_cxflags("-std=c++2a","-ftemplate-backtrace-limit=0",{force=true})
add_cxflags("-g")
add_ldflags("-g")
target("test")
  set_kind("binary")
  add_files("tests/**.cpp")
  add_includedirs("doctest/doctest")
target("develop")
  set_kind("binary")
  add_files("develop/patch.cpp")

  --add_defines("MAKE_LIB")
  --add_includedirs("lua")
  --add_files("lua/onelua.c")
