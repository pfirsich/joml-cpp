project('joml-cpp', 'cpp', default_options : ['warning_level=3', 'cpp_std=c++17'])

joml_cpp_inc = include_directories('include')
joml_cpp_lib = library('joml-cpp', 'src/joml.cpp', include_directories : joml_cpp_inc)
joml_cpp_dep = declare_dependency(link_with: joml_cpp_lib, include_directories : joml_cpp_inc)

if not meson.is_subproject()
  executable('joml2json', 'src/joml2json.cpp', dependencies : joml_cpp_dep)
endif
