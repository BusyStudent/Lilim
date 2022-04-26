#define FONS_CLEARTYPE

#include "fontstash.cpp"
#include "lilim.cpp"

#ifdef _WIN32
    #define FONT_FILE "C:/Windows/Fonts/msyh.ttc"
#else
    #define FONT_FILE "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"
#endif

int main(){
    Lilim::Manager manager;
    Fons::Fontstash stash(manager);
    Fons::Context context(stash);

    auto face = manager.new_face(FONT_FILE,0);
    int id = stash.add_font(face);

    context.set_font(id);
    context.set_align(FONS_ALIGN_LEFT | FONS_ALIGN_TOP);
    context.set_size(12);

    Fons::TextIter iter;

    iter.init(&context,0,0,"Hello Cleartype",nullptr,FONS_GLYPH_BITMAP_REQUIRED);

    context.dump_info();
}