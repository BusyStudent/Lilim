# Lilim  

nanovg fontstash reimplementation in C++.  

## Examples

```cpp

#include "fontstash.hpp"
#include "fons_backend.hpp"

int main(){
    //Setup
    Llilim::Manager manager;
    auto face = manager.new_face(FACE_FILE,FACE_IDX);
    //Set DPI if you want
    face->set_dpi(96,96);
    
    Fons::Fontstash stash(manager);
    Fons::Context context(stash);

    int id = stash.add_font(face);

    //Using orginal C fontstash api
    FONStextIter iter;
    FONSquad     quad;
    fonsSetAlign(&context,FONS_ALIGN_LEFT | FONS_ALIGN_TOP);
    fonsSetSize(&context,12);
    fonsSetFont(&context,id);
    fonsTextIterInit(&context,&iter,0,0,"Hello",nullptr,FONS_GLYPH_BITMAP_REQUIRED);
    fonsTextIterNext(&context,&iter,&quad);
    //etc...
    //Or C++ style fontstash api
    context.set_font(id);
    context.set_size(12);

    //etc...

    //Or new style api 
    Fons::SDLTextRenderer renderer(sdl_render,stash);
    renderer.draw_text(0,0,"Hello World");
    renderer.flush();
    //Add your backend by inheriting Fons::TextRenderer

    //End
}

```

## Todo List

- [ ] Add harfbuzz support.
- [ ] Add Embedded bitmap font support.
- [ ] Add fallback support.  
- [ ] Add stb_truetype support.
