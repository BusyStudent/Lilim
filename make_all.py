def read_file(name : str) -> str:
    with open(name, 'r') as f:
        return f.read()

text = ""

# Add header
text += "\n#ifndef _LILIM_ALL_HPP_"
text += "\n#define _LILIM_ALL_HPP_\n"

# Detect ft2build.h
text += "#ifdef __has_include\n"
text += "#if !__has_include(\"ft2build.h\")\n"
text += "#define LILIM_STBTRUETYPE\n"
text += "#endif\n"
text += "#endif\n"

text += "#ifdef LILIM_FREETYPE\n"
text += "#undef LILIM_STBTRUETYPE\n"
text += "#endif\n"



text += read_file("lilim.hpp")
text += "\n"
text += read_file("fontstash.hpp")

# Add fons_backend
text += read_file('fons_backend.hpp')
text += "\n"

text += "\n#ifdef LILIM_IMPLEMENTATION\n"

text += read_file('lilim.cpp')
text += "\n"
text += read_file('fontstash.cpp')

text = text.replace('#include "lilim.hpp"', "")
text = text.replace('#include "fontstash.hpp"', "")

# Inject stb_truetype
text = text.replace('#include LILIM_STBTRUETYPE_H',read_file('stb_truetype.h'))

text += "\n#endif\n"


text += "\n#endif\n"

text = text.replace('#pragma once', "")

with open('lilim_all.hpp','w') as f:
    f.write(text)