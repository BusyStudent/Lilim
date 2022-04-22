#include <algorithm>
#include "lilim.hpp"

//Platform-specific includes
#ifdef _WIN32
    #define  NOMINMAX
    #include <windows.h>
#endif

LILIM_NS_BEGIN

//Manager
Manager::Manager(){
    if(FT_Init_FreeType(&library)){
        //TODO: Error handling
        std::abort();
    }
}
Manager::~Manager(){
    FT_Done_FreeType(library);
}
Ref<Face> Manager::new_face(Ref<Blob> blob,int index){
    FT_Bytes bytes = static_cast<FT_Bytes>(blob->data());
    FT_Long  size  = static_cast<FT_Long>(blob->size());
    FT_Face  face;
    if(FT_New_Memory_Face(library,bytes,size,index,&face)){
        //Failed to load font
        return {};
    }
    Ref<Face> ret = new Face();
    ret->manager = this;
    ret->blob    = blob;
    ret->face    = face;
    return ret;
}
Ref<Face> Manager::new_face(const char *path,int index){
    FT_Face face;
    if(FT_New_Face(library,path,index,&face)){
        //Failed to load font
        return {};
    }
    Ref<Face> ret = new Face();
    ret->manager = this;
    ret->face    = face;
    return ret;
}
Ref<Blob> Manager::alloc_blob(size_t size){
    void *ptr = std::malloc(size);
    if(ptr == nullptr){
        return {};
    }
    Ref<Blob> ret = new Blob(ptr,size);
    ret->set_finalizer([](void *ptr,size_t,void*){
        std::free(ptr);
    },nullptr);
    return ret;
}

//Face
Face::Face(){
    flags = FT_LOAD_DEFAULT;
    manager = nullptr;
    face    = nullptr;
}
Face::~Face(){
    FT_Done_Face(face);
}
void  Face::set_size(FaceSize size){
    FT_Set_Char_Size(
        face,
        size.width * 64,
        size.height * 64,
        size.xdpi,
        size.ydpi
    );
}
Uint  Face::glyph_index(char32_t codepoint){
    return FT_Get_Char_Index(face,codepoint);
}
Uint  Face::kerning(Uint prev,Uint cur){
    FT_Vector delta;
    FT_Get_Kerning(face,prev,cur,FT_KERNING_DEFAULT,&delta);
    return delta.x >> 6;
}
auto  Face::metrics() -> FaceMetrics{
    FaceMetrics metrics;
    metrics.ascender    = face->size->metrics.ascender >> 6;
    metrics.descender   = face->size->metrics.descender >> 6;
    metrics.height      = face->size->metrics.height >> 6;
    metrics.max_advance = face->size->metrics.max_advance >> 6;
    metrics.underline_position = face->underline_position >> 6;
    metrics.underline_thickness = face->underline_thickness >> 6;
    return metrics;
}
auto  Face::build_glyph(Uint code) -> GlyphMetrics{
    FT_Load_Glyph(face,code,flags);
    FT_GlyphSlot slot = face->glyph;
    GlyphMetrics ret;

    //Get metrics
    ret.width  = slot->bitmap.width;
    ret.height = slot->bitmap.rows;
    ret.bitmap_left = slot->bitmap_left;
    ret.bitmap_top  = slot->bitmap_top;
    ret.advance_x   = slot->advance.x >> 6;
    return ret;
}
void  Face::render_glyph(Uint code,void *b,int pitch,int px,int py){
    FT_Load_Glyph(face,code,FT_LOAD_RENDER | flags);
    FT_GlyphSlot slot = face->glyph;
    //Begin rendering
    //TODO : Handle pitch and Format
    for(int y = 0;y < slot->bitmap.rows;y++){
        for(int x = 0;x < slot->bitmap.width;x++){
            int p = (py + y) * pitch + (px + x);
            int c = slot->bitmap.buffer[y * slot->bitmap.width + x];

            static_cast<uint8_t*>(b)[p] = c;
        }
    }
}
auto  Face::measure_text(const char *text,size_t len) -> Size{
    int w = 0;
    int h = 0;

    const char *cur = text;
    const char *end = text + len;
    GlyphMetrics glyph;
    FaceMetrics  m = metrics();

    Uint prev = 0;

    while(cur != end){
        char32_t ch = Utf8Decode(cur);
        //TODO: Handle invalid utf8
        Uint idx = glyph_index(ch);
        glyph = build_glyph(idx);

        int y_offset = m.ascender - glyph.bitmap_top;

        w += glyph.advance_x;
        h = std::max(h,glyph.height);
        h = std::max(h,y_offset + glyph.height);

        //Add kerning
        if(prev != 0){
            w += kerning(prev,idx);
        }
        prev = idx;
    }
    return {w,h};
}
auto  Face::render_text(const char *text,size_t len) -> Ref<Blob>{
    Size size = measure_text(text,len);
    auto blob = manager->alloc_blob(size.width * size.height);
    auto m    = metrics();

    uint8_t *data = static_cast<uint8_t*>(blob->data());
    //Begin rendering
    int pitch = size.width;
    int pen_x = 0;
    int pen_y = 0;

    const char *cur = text;
    const char *end = text + len;

    Uint prev = 0;

    std::memset(data,0,size.width * size.height);

    while(cur != end){
        char32_t ch = Utf8Decode(cur);
        Uint idx = glyph_index(ch);
        GlyphMetrics glyph = build_glyph(idx);

        int pen_y = m.ascender - glyph.bitmap_top;

        render_glyph(idx,data,pitch,pen_x,pen_y);

        //Move pen
        pen_x += glyph.advance_x;

        //Add kerning
        if(prev != 0){
            pen_x += kerning(prev,idx);
        }
        prev = idx;
    }
    return blob;
}
Ref<Face> Face::clone(){
    if(FT_Reference_Face(face)){
        //Error
        return {};
    }
    Ref<Face> ret = new Face();
    ret->manager = manager;
    ret->blob    = blob;
    ret->face    = face;
    return ret;
}

//Utility functions    

char32_t  Utf8Decode(const char *&str){
    //Algorithm from utf8 for cpp
    auto mask = [](char ch) -> uint8_t{
        return ch & 0xff;
    };
    //Get length first
    int     len = 0;
    uint8_t lead = mask(*str);
    if(lead < 0x80){
        len = 1;
    }
    else if((lead >> 5) == 0x6){
        len = 2;
    }
    else if((lead >> 4) == 0xe){
        len = 3;
    }
    else if((lead >> 3) == 0x1e){
        len = 4;
    }
    else{
        //Invalid
        std::abort();
    }
    //Start
    char32_t ret = mask(*str);
    switch (len) {
        case 1:
            break;
        case 2:
            ++str;
            ret = ((ret << 6) & 0x7ff) + ((*str) & 0x3f);
            break;
        case 3:
            ++str; 
            ret = ((ret << 12) & 0xffff) + ((mask(*str) << 6) & 0xfff);
            ++str;
            ret += (*str) & 0x3f;
            break;
        case 4:
            ++str;
            ret = ((ret << 18) & 0x1fffff) + ((mask(*str) << 12) & 0x3ffff);                
            ++str;
            ret += (mask(*str) << 6) & 0xfff;
            ++str;
            ret += (*str) & 0x3f; 
            break;
    }
    ++str;
    return ret;
}

Ref<Blob> MapFile(const char *file){
    //Convert to wide string
    size_t len = std::strlen(file);
    wchar_t *wfile = new wchar_t[len + 1];
    MultiByteToWideChar(CP_UTF8, 0, file, -1, wfile, len + 1);
    wfile[len] = 0;

    //Open file
    HANDLE fhandle = CreateFileW(
        wfile,
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    delete[] wfile;
    if(fhandle == INVALID_HANDLE_VALUE){
        return {};
    }
    //Map file
    DWORD size = GetFileSize(fhandle, nullptr);
    HANDLE mhandle = CreateFileMapping(
        fhandle, 
        nullptr, 
        PAGE_READONLY, 
        0, 
        size, 
        nullptr
    );
    CloseHandle(fhandle);
    if(mhandle == nullptr){
        return {};
    }
    void *data = MapViewOfFile(mhandle, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(mhandle);
    if(data == nullptr){
        return {};
    }
    //Create blob
    Ref<Blob> blob = new Blob(data, size);
    blob->set_finalizer([](void *data, size_t size, void *user){
        UnmapViewOfFile(data);
    }, nullptr);
    return blob;
}

LILIM_NS_END