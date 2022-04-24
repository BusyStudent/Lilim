#pragma once

#ifndef FONS_NAMESPACE 
    #define FONS_NAMESPACE Fons
#endif

#ifndef FONS_NDEBUG
    #ifdef  NDEBUG
        #define FONS_NDEBUG
    #endif
#endif

#ifdef FONS_STATIC
    #define FONS_NS_BEGIN \
        namespace { \
        namespace FONS_NAMESPACE {
    #define FONS_NS_END \
        } \
        }
#else
    #define FONS_NS_BEGIN \
        namespace FONS_NAMESPACE {
    #define FONS_NS_END \
        }
#endif

#ifndef FONS_MAX_CACHED_GLYPHS
    #define FONS_MAX_CACHED_GLYPHS (1024 * 4)
#endif

#include <unordered_map>
#include <string>
#include <vector>
#include <stack>
#include <map>
#include <set>
#include "lilim.hpp"

#define FONS_INVALID -1

enum {
	// Horizontal align
	FONS_ALIGN_LEFT 	= 1<<0,	// Default
	FONS_ALIGN_CENTER 	= 1<<1,
	FONS_ALIGN_RIGHT 	= 1<<2,
	// Vertical align
	FONS_ALIGN_TOP 		= 1<<3,
	FONS_ALIGN_MIDDLE	= 1<<4,
	FONS_ALIGN_BOTTOM	= 1<<5,
	FONS_ALIGN_BASELINE	= 1<<6, // Default
};

enum {
	FONS_GLYPH_BITMAP_OPTIONAL = 0,
	FONS_GLYPH_BITMAP_REQUIRED = 1,
};

FONS_NS_BEGIN

using namespace LILIM_NAMESPACE;
//Interface for nanovg
class Context;
class Fontstash;
class FontParams {
    public:
        Context *context;//< Belongs to
        char32_t codepoint;// < Codepoint
        short spacing;
        short blur;
        short size;
};
class Glyph : public GlyphMetrics ,public FontParams {
    public:
        int x = -1;//< In bitmap position
        int y = -1;
};

class Font: public Refable<Font> {
    public:
        Font(const Font &) = delete;
        ~Font();

        Ref<Font>   clone();
        Glyph      *get_glyph(FontParams param,int req_bitmap);
        /**
         * @brief Get metrics of font with size
         * 
         * @param size 
         * @return FaceMetrics 
         */
        FaceMetrics metrics_of(float size){
            face->set_size(size);
            return face->metrics();
        }
        Uint kerning(char32_t prev,char32_t cur){
            return face->kerning(face->glyph_index(prev),face->glyph_index(cur));
        }
        void set_name(const char *name){
            this->name = name;
        }
        void reset_fallbacks(){
            fallbacks.clear();
        }
        void add_fallback(Ref<Font> f){
            if(f.get() == this){
                return;
            }
            fallbacks.emplace_back(std::move(f));
        }
        int get_id(){
            return id;
        }
    private:
        Font();

        std::unordered_multimap<char32_t,Glyph> glyphs;
        std::vector<Ref<Font>>     fallbacks;
        std::string                name;
        Fontstash                 *stash;
        Ref<Face>                  face;
        int                        id;
    friend class Fontstash;
};
/**
 * @brief For managing fonts 
 * 
 */
class Fontstash {
    public:
        Fontstash(Manager &manager);
        ~Fontstash();

        int   add_font(Ref<Face> font);
        Font *get_font(const char *name);
        Font *get_font(int id);
    private:
        std::map<int,Ref<Font>> fonts; 
        Manager              *manager;
};
/**
 * @brief For managing bitmaps and atlas
 * 
 */
class Context {
    public:
        Context(Fontstash &,int width = 512,int height = 512);
        Context(const Context &) = delete;
        ~Context();

        void push_state(){
            if(states.empty()){
                states.push({});
            }
            else{
                states.push(states.top());
            }
        }
        void pop_state(){
            states.pop();
        }
        void clear_state(){
            states.top() = {};
        }

        void set_font(int id){
            states.top().font = id;
        }
        void set_size(float size){
            states.top().size = size;
        }
        void set_spacing(float spacing){
            states.top().spacing = spacing;
        }
        void set_blur(float blur){
            states.top().blur = blur;
        }
        void set_align(int align){
            states.top().align = align;
        }

        void expand_atlas(int width,int height);
        void reset_atlas(int width,int height);
        void get_atlas_size(int *width,int *height);

        void *get_data(int *w,int *h){
            if(w != nullptr){
                *w = bitmap_w;
            }
            if(h != nullptr){
                *h = bitmap_h;
            }
            return bitmap.data();
        }
        bool  validate(int *dirty);

        Size  measure_text(const char *text,const char *end = nullptr);
        //NVG Interface
        float text_bounds(float x, float y, const char* str, const char* end, float* bounds);
        void  line_bounds(float y, float* miny, float* maxy);
        void  vert_metrics(float* ascender, float* descender,float* lineh);
        //Get Fontstash
        Fontstash *fontstash() const noexcept{
            return stash;
        }
        /**
         * @brief Register a font in context
         * 
         * @param id 
         * @return int 
         */
        void  add_font(int id);
        void  remove_font(int id);
        void  dump_info(FILE *f = stdout);
    private:
        struct State {
            int align = FONS_ALIGN_LEFT | FONS_ALIGN_BASELINE;
            int font = 0;
            int blur = 0;
            int spacing = 0;
            float size = 12;
        };
        // Atlas based on Skyline Bin Packer by Jukka Jylänki
        // From nanovg
        struct AtlasNode {
            short x = 0;
            short y = 0;
            short width = 0;
        };
        struct Atlas {
            Atlas(int w,int h);
            ~Atlas();

            int width, height;
            AtlasNode* nodes;
            int nnodes;
            int cnodes;

            void reset(int width,int height);
            void expend(int width,int height);
            void remove_node(int index);
            bool insert_node(int index,int x,int width,int height);
            bool add_skyline(int index,int x,int y,int w,int h);
            int  rect_fits(int i,int w,int h);
            bool add_rect(int x,int y,int *w,int *h);
        };
        std::set<Ref<Font>>     fonts;
        std::vector<uint8_t>    bitmap;
        std::stack<State>       states;
        Atlas                   atlas;
        Fontstash              *stash;
        int                     bitmap_w;
        int                     bitmap_h;
        int                     dirty_rect[4];
    friend class TextIter;
    friend class Font;
};

class Quad {
    public:
        float x0,y0,s0,t0;
        float x1,y1,s1,t1;

};
class TextIter {
    public:
        float x, y, nextx, nexty, scale, spacing;
        unsigned int codepoint;
        
        short isize, iblur;

        Font *font;
        int prevGlyphIndex;
        const char* str;
        const char* next;
        const char* end;
        unsigned int utf8state;
        int bitmapOption;

        //Text Height / Width
        float width;
        float height;

        Context *context;
        FaceMetrics metrics;

        bool init(Context *ctxt,float x,float y,const char* str,const char* end,int bitmapOption);
        bool to_next(Quad* quad);
};

//Util
//Transform From Point to
//Point(X,Y)-----------------
//|     TEXT                |
//|--------------------------
void TransformByAlign(
    float *x,float *y,
    int align,
    Size size,
    FaceMetrics m
);

FONS_NS_END

//Inline Wrap for C

struct FONSparams {
	int width, height;
	unsigned char flags;
	void* userPtr;
	int (*renderCreate)(void* uptr, int width, int height);
	int (*renderResize)(void* uptr, int width, int height);
	void (*renderUpdate)(void* uptr, int* rect, const unsigned char* data);
	void (*renderDraw)(void* uptr, const float* verts, const float* tcoords, const unsigned int* colors, int nverts);
	void (*renderDelete)(void* uptr);
};
using FONStextIter = FONS_NAMESPACE::TextIter;
using FONScontext = FONS_NAMESPACE::Context;
using FONSquad    = FONS_NAMESPACE::Quad;


#ifndef fonsGetStash
    extern FONS_NAMESPACE::Fontstash *__fons_stash;
    #define fonsGetStash() _fons_stash
#endif
//Create / Destroy
#define fonsCreateInternal(Params) (new Fons::Context(*fonsGetStash(),(Params)->width,(Params)->height))
#define fonsDeleteInternal(X) delete X

// State Setting
#define fonsSetSize(X,SIZE) X->set_size(SIZE)
#define fonsSetFont(X,ID) X->set_font(ID)
#define fonsSetBlur(X,BLUR) X->set_blur(BLUR)
#define fonsSetSpacing(X,SPACING) X->set_spacing(SPACING)
#define fonsSetAlign(X,ALIGN) X->set_align(ALIGN)

//Font Set / Add
#define fonsAddFont(X,NAME,PATH,FONTINDEX) (FONS_INVALID)
#define fonsAddFontMem(X,NAME,DATA,SIZE,FREE,FONTINDEX) (FONS_INVALID)
#define fonsGetFontByName(X,NAME) (FONS_INVALID)
#define fonsAddFallbackFont(X,NAME,FONTINDEX) (FONS_INVALID)
#define fonsResetFallbackFont(X,ID) (FONS_INVALID)

// State handling
#define fonsPushState(X) X->push_state()
#define fonsPopState(X) X->pop_state()
#define fonsClearState(X) X->clear_state()

//Measure Text
#define fonsTextBounds(S,X,Y,STR,END,BOUNDS) S->text_bounds(X,Y,STR,END,BOUNDS)
#define fonsLineBounds(S,Y,MINY,MAXY) S->line_bounds(Y,MINY,MAXY)
#define fonsVertMetrics(S,ASC,DESC,LINEH) S->vert_metrics(ASC,DESC,LINEH)

//Iterate Text
#define fonsTextIterInit(S,ITER,X,Y,STR,END,BITMAPOPTION) (ITER)->init(S,X,Y,STR,END,BITMAPOPTION)
#define fonsTextIterNext(S,ITER,QUAD) (ITER)->to_next(QUAD)

//Pull Data
#define fonsGetTextureData(S,W,H) (const uint8_t*)S->get_data(W,H)
#define fonsValidateTexture(S,D) S->validate(D)

//Atlas
#define fonsExpandAtlas(S,W,H) S->expand_atlas(W,H)
