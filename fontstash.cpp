// nanovg interface
//
// Copyright (c) 2009-2013 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
#include <algorithm>
#include <cstring>

#include "fontstash.hpp"

FONS_NS_BEGIN

Font::Font(){

}
Font::~Font(){

}

Ref<Font> Font::clone(){
    int id = stash->add_font(
        face->clone()
    );
    return stash->get_font(id);
}

Glyph *Font::get_glyph(FontParams param,int req_bitmap){
    //Clear cache if too big
    if(glyphs.size() > FONS_MAX_CACHED_GLYPHS){
        glyphs.clear();
    }

    Glyph *g = nullptr;
    Uint idx = face->glyph_index(param.codepoint);

    if(idx == 0){
        //No glyph, try fallbacks
        for(auto &f:fallbacks){
            idx = f->face->glyph_index(param.codepoint);
            if(idx != 0){
                return f->get_glyph(param,req_bitmap);
            }
        }
        //Oh,no.just keep it
    }

    //find existing glyph
    auto iter = glyphs.find(param.codepoint);
    auto num  = glyphs.count(param.codepoint);

    while(num){
        if(iter->second.spacing == param.spacing && iter->second.size == param.size){
            g = &iter->second;
            break;
        }
        num--;
        iter++;
    }

    if(g == nullptr){
        //Get metrics
        face->set_size(param.size);

        Glyph glyph;
        auto m = face->build_glyph(idx);

        static_cast<FontParams&>(glyph) = param;
        static_cast<GlyphMetrics&>(glyph) = m;
        //Insert
        iter = glyphs.insert(std::make_pair(param.codepoint,glyph));
        g = &iter->second;
    }
    if(req_bitmap && g->x < 0 && g->y < 0){
        //do render
        face->set_size(param.size);

        int x,y;
        auto m = face->build_glyph(idx);
        //Alloc space
        bool v = param.context->atlas.add_rect(
            m.width,
            m.height,
            &x,
            &y
        );
        //Atlas full
        if(!v){
            return nullptr;
        }

        g->x = x;
        g->y = y;
        //Rasterize
        face->render_glyph(
            idx,
            param.context->bitmap.data(),
            param.context->bitmap_w,
            x,
            y
        );
        //Update dirty
        int *dirty = param.context->dirty_rect;
        dirty[0] = std::min(dirty[0],x);
        dirty[1] = std::min(dirty[1],y);
        dirty[2] = std::max(dirty[2],x + g->width);
        dirty[3] = std::max(dirty[3],y + g->height);
    }
    return g;
}

Fontstash::Fontstash(Manager &m){
    manager = &m;
}
Fontstash::~Fontstash(){

}

int   Fontstash::add_font(Ref<Face> face){
    int id;
    do{
        id = std::rand();
        if(id < 0){
            id = -id;
        }
    }
    while(fonts.find(id) != fonts.end());
    Ref<Font> f = new Font();
    f->stash = this;
    f->face = face;
    f->id = id;
    fonts[id] = f;
    return id;
}
Font *Fontstash::get_font(int id){
    auto it = fonts.find(id);
    if(it == fonts.end()){
        return nullptr;
    }
    return it->second.get();
}

//Context operations
Context::Context(Fontstash &m,int w,int h):atlas(w,h){
    stash = &m;
    bitmap_w = w;
    bitmap_h = h;
    bitmap.resize(bitmap_w * bitmap_h);

    //Push one state
    push_state();

    //Init dirty
    dirty_rect[0] = w;
    dirty_rect[1] = h;
    dirty_rect[2] = 0;
    dirty_rect[3] = 0;
}
Context::~Context(){

}

Size Context::measure_text(const char *str,const char *end){
    Font *font = stash->get_font(states.top().font);
    if(font == nullptr){
        return {0,0};
    }
    if(end == nullptr){
        end = str + std::strlen(str);
    }
    Size size = {0,0};
    auto m = font->metrics_of(states.top().size);

    Uint prev = 0;

    while(str < end){
        char32_t c = Utf8Decode(str);
        FontParams param;
        param.context = this;
        param.codepoint = c;
        param.spacing = states.top().spacing;
        param.size = states.top().size;
        param.blur = states.top().blur;

        //Send to fond
        Glyph *g = font->get_glyph(param,FONS_GLYPH_BITMAP_REQUIRED);
        if(g == nullptr){
            //No Glyph?
            break;
        }
        int yoffset = m.ascender - g->bitmap_top;

        size.height = std::max(size.height,g->height);
        size.height = std::max(size.height,g->height + yoffset);
        size.width += g->advance_x;

        if(prev != 0){
            size.width += font->kerning(prev,c);
        }
        prev = c;
    }
    return size;
}

void Context::vert_metrics(float* ascender, float* descender, float* lineh){
    Font *font = stash->get_font(states.top().font);
    if(font == nullptr){
        return;
    }
    auto m = font->metrics_of(states.top().size);
    
    if(ascender != nullptr){
        *ascender = m.ascender;
    }
    if(descender != nullptr){
        *descender = m.descender;
    }
    if(lineh != nullptr){
        *lineh = m.height;
    }
}
void Context::line_bounds(float y, float* miny, float* maxy){
    Font *font = stash->get_font(states.top().font);
    if(font == nullptr){
        return;
    }
    auto m = font->metrics_of(states.top().size);
    //Align To Top
    //Dst()-------
    //|
    //|
    int align = states.top().align;
    if(align & FONS_ALIGN_TOP){
        //Nothing
    }
    else if(align & FONS_ALIGN_MIDDLE){
        y -= m.height / 2;
    }
    else if(align & FONS_ALIGN_BASELINE){
        y -= m.ascender;
    }
    else if(align & FONS_ALIGN_BOTTOM){
        y -= m.height;
    }
    //Done
    if(miny != nullptr){
        *miny = y;
    }
    if(maxy != nullptr){
        *maxy = y + m.height;
    }
}
float Context::text_bounds(float x, float y, const char* str, const char* end, float* bounds){
    if(end == nullptr){
        end = str + strlen(str);
    }
    Font *font = stash->get_font(states.top().font);
    auto metrics = font->metrics_of(states.top().size);
    auto align = states.top().align;

    auto size = measure_text(str,end);
    float width = size.width;
    float height = size.height;

    //Begin Transform
    TransformByAlign(&x,&y,align,size,metrics);

    if(bounds != nullptr){
        bounds[0] = x;
        bounds[1] = y;
        bounds[2] = x + width;
        bounds[3] = y + height;
    }
    //Return width
    return width;
}
bool Context::validate(int *dirty){
    if (dirty_rect[0] < dirty_rect[2] && dirty_rect[1] < dirty_rect[3]) {
		dirty[0] = dirty_rect[0];
		dirty[1] = dirty_rect[1];
		dirty[2] = dirty_rect[2];
		dirty[3] = dirty_rect[3];
		// Reset dirty rect
		dirty_rect[0] = bitmap_w;
		dirty_rect[1] = bitmap_h;
		dirty_rect[2] = 0;
		dirty_rect[3] = 0;
		return 1;
	}
	return 0;
}
//Manage owned font
void Context::add_font(int id){
    if(id < 0){
        return;
    }
    Font *f = stash->get_font(id);
    if(f == nullptr){
        return;
    }
    fonts.emplace(f);
}
void Context::remove_font(int id){
    if(id < 0){
        return;
    }
    Font *f = stash->get_font(id);
    if(f == nullptr){
        return;
    }
    fonts.erase(f);
}
void Context::dump_info(FILE *fp){
    #ifndef FONS_NDEBUG
    if(fp == nullptr){
        return;
    }
    fprintf(fp,"Context at %p\n",this);
    fprintf(fp,"    width %d height %d\n",bitmap_w,bitmap_h);
    fprintf(fp,"    Dirty in:\n");
    fprintf(fp,"        minx [%d]\n",dirty_rect[0]);
    fprintf(fp,"        miny [%d]\n",dirty_rect[1]);
    fprintf(fp,"        maxx [%d]\n",dirty_rect[2]);
    fprintf(fp,"        maxy [%d]\n",dirty_rect[3]);
    fprintf(fp,"    Fonts:\n");
    for(auto &f : fonts){
        fprintf(fp,"        id %d => %p\n",f.get()->get_id(),f.get());
    }
    fprintf(fp,"    States:\n");
    fprintf(fp,"        font => %d\n",states.top().font);
    fprintf(fp,"        size => %f\n",states.top().size);
    fprintf(fp,"        blur => %f\n",states.top().blur);
    fprintf(fp,"        align => %d\n",states.top().align);
    fprintf(fp,"        spacing => %f\n",states.top().spacing);
    //If has dirty rect dump it
    if(dirty_rect[0] < dirty_rect[2] && dirty_rect[1] < dirty_rect[3]){
        fprintf(fp,"    Dirty in:\n");
        fprintf(fp,"        minx [%d]\n",dirty_rect[0]);
        fprintf(fp,"        miny [%d]\n",dirty_rect[1]);
        fprintf(fp,"        maxx [%d]\n",dirty_rect[2]);
        fprintf(fp,"        maxy [%d]\n",dirty_rect[3]);
        //Print texture data in dirty rect
        fprintf(fp,"    Texture data:\n");
        for(int y = dirty_rect[1]; y < dirty_rect[3]; y++){
            fprintf(fp,"        ");
            for(int x = dirty_rect[0]; x < dirty_rect[2]; x++){
                fprintf(fp,"%c",bitmap[y * bitmap_w + x] ? '#' : ' ');
            }
            fprintf(fp,"\n");
        }
    }
    else{
        //Dirty rect is empty
        fprintf(fp,"    DirtyRect empty:\n");
    }
    #endif
}

//Atlas from nanovg
Context::Atlas::Atlas(int w,int h){
	// Allocate memory for the font stash.
    int n = 255;
	std::memset(this, 0, sizeof(Atlas));

	width = w;
	height = h;

	// Allocate space for skyline nodes
	nodes = (AtlasNode*)std::malloc(sizeof(AtlasNode) * n);
	memset(nodes, 0, sizeof(AtlasNode) * n);
	nnodes = 0;
	cnodes = n;

	// Init root node.
	nodes[0].x = 0;
	nodes[0].y = 0;
	nodes[0].width = (short)w;
	nnodes++;
}
Context::Atlas::~Atlas(){
    std::free(nodes);
}
bool Context::Atlas::insert_node(int idx,int x,int y,int w){
    int i;
	// Insert node
	if (nnodes+1 > cnodes) {
		cnodes = cnodes == 0 ? 8 : cnodes * 2;
		nodes = (AtlasNode*)std::realloc(nodes, sizeof(AtlasNode) * cnodes);
		if (nodes == nullptr)
			return false;
	}
	for (i = nnodes; i > idx; i--)
		nodes[i] = nodes[i-1];
	nodes[idx].x = (short)x;
	nodes[idx].y = (short)y;
	nodes[idx].width = (short)w;
	nnodes++;
    return true;
}
void Context::Atlas::remove_node(int idx){
    int i;
	if (nnodes == 0) return;
	for (i = idx; i < nnodes-1; i++)
		nodes[i] = nodes[i+1];
	nnodes--;
}
void Context::Atlas::expend(int w,int h){
	// Insert node for empty space
	if (w > width)
		insert_node(nnodes, width, 0, w - width);
	width = w;
	height = h;
}
void Context::Atlas::reset(int w,int h){
	width = w;
	height = h;
	nnodes = 0;

	// Init root node.
	nodes[0].x = 0;
	nodes[0].y = 0;
	nodes[0].width = (short)w;
	nnodes++;
}
bool Context::Atlas::add_skyline(int idx, int x, int y, int w, int h){
	int i;

	// Insert new node
	if (insert_node(idx, x, y+h, w) == 0)
		return false;

	// Delete skyline segments that fall under the shadow of the new segment.
	for (i = idx+1; i < nnodes; i++) {
		if (nodes[i].x < nodes[i-1].x + nodes[i-1].width) {
			int shrink = nodes[i-1].x + nodes[i-1].width - nodes[i].x;
			nodes[i].x += (short)shrink;
			nodes[i].width -= (short)shrink;
			if (nodes[i].width <= 0) {
				remove_node(i);
				i--;
			} else {
				break;
			}
		} else {
			break;
		}
	}

	// Merge same height skyline segments that are next to each other.
	for (i = 0; i < nnodes-1; i++) {
		if (nodes[i].y == nodes[i+1].y) {
			nodes[i].width += nodes[i+1].width;
			remove_node(i+1);
			i--;
		}
	}

	return true;
}
int  Context::Atlas::rect_fits(int i, int w, int h){
	// Checks if there is enough space at the location of skyline span 'i',
	// and return the max height of all skyline spans under that at that location,
	// (think tetris block being dropped at that position). Or -1 if no space found.
	int x = nodes[i].x;
	int y = nodes[i].y;
	int spaceLeft;
	if (x + w > width)
		return -1;
	spaceLeft = w;
	while (spaceLeft > 0) {
		if (i == nnodes) return -1;
		y = std::max<int>(y, nodes[i].y);
		if (y + h > height) return -1;
		spaceLeft -= nodes[i].width;
		++i;
	}
	return y;
}
bool Context::Atlas::add_rect(int rw, int rh, int* rx, int* ry){

    int besth = height, bestw = width, besti = -1;
	int bestx = -1, besty = -1, i;

	// Bottom left fit heuristic.
	for (i = 0; i < nnodes; i++) {
		int y = rect_fits(i, rw, rh);
		if (y != -1) {
			if (y + rh < besth || (y + rh == besth && nodes[i].width < bestw)) {
				besti = i;
				bestw = nodes[i].width;
				besth = y + rh;
				bestx = nodes[i].x;
				besty = y;
			}
		}
	}

	if (besti == -1)
		return false;

	// Perform the actual packing.
	if (add_skyline(besti, bestx, besty, rw, rh) == 0)
		return false;

	*rx = bestx;
	*ry = besty;

	return 1;
}

void Context::get_atlas_size(int *w,int *h){
    *w = bitmap_w;
    *h = bitmap_h;
}
void Context::expand_atlas(int w,int h){
    w = std::max(w,bitmap_w);
    h = std::max(h,bitmap_h);

    if(w == bitmap_w && h == bitmap_h){
        return;
    }
    
    std::vector<uint8_t> new_map(w * h);
    //Copy old map to new map
    for(int y = 0;y < bitmap_h;y++){
        for(int x = 0;x < bitmap_w;x++){
            new_map[y * w + x] = bitmap[y * bitmap_w + x];
        }
    }
    //Increase atlas size
    atlas.expend(w,h);

    //Mark as dirty
    short maxy = 0;
    for(int i = 0;i < atlas.nnodes;i++){
        maxy = std::max(maxy,atlas.nodes[i].y);
    }

    dirty_rect[0] = 0;
    dirty_rect[1] = 0;
    dirty_rect[2] = bitmap_w;
    dirty_rect[3] = maxy;

    bitmap = std::move(new_map);
    bitmap_w = w;
    bitmap_h = h;
}
void Context::reset_atlas(int w,int h){
    bitmap.resize(w * h);
    atlas.reset(w,h);
    bitmap_w = w;
    bitmap_h = h;
    dirty_rect[0] = w;
    dirty_rect[1] = h;
    dirty_rect[2] = 0;
    dirty_rect[3] = 0;

    //TODO Reset Glyph in owned font

}

void TransformByAlign(
    float *x,float *y,
    int align,
    Size size,
    FaceMetrics m
){
    //Align To Top
    //Dst()-------
    //|
    //|
    float ix = *x;
    float iy = *y;
    if(align & FONS_ALIGN_TOP){
        //Nothing
    }
    else if(align & FONS_ALIGN_MIDDLE){
        iy -= size.height / 2;
    }
    else if(align & FONS_ALIGN_BASELINE){
        iy -= m.ascender;
    }
    else if(align & FONS_ALIGN_BOTTOM){
        iy -= size.height;
    }

    //X
    if(align & FONS_ALIGN_LEFT){
        //Nothing
    }
    else if(align & FONS_ALIGN_CENTER){
        ix -= size.width / 2;
    }
    else if(align & FONS_ALIGN_RIGHT){
        ix -= size.width;
    }
    *x = ix;
    *y = iy;
}
//Text Iterator
bool TextIter::init(Context *ctxt,float x,float y,const char* str,const char* end,int bitmapOption){
    //Zero first
    std::memset(this,0,sizeof(TextIter));

    if(end == nullptr){
        end = str + strlen(str);
    }
    auto &state = ctxt->states.top();

    //Measure
    this->font = ctxt->stash->get_font(
        state.font
    );
    if(this->font == nullptr){
        return false;
    }
    auto size = ctxt->measure_text(str,end);

    this->width = size.width;
    this->height = size.height;
    //Transform back
    this->metrics = font->metrics_of(state.size);
    TransformByAlign(&x,&y,state.align,size,metrics);


	this->x = this->nextx = x;
	this->y = this->nexty = y;
	this->spacing = state.spacing;
	this->str = str;
	this->next = str;
	this->end = end;
	this->codepoint = 0;
	this->prevGlyphIndex = -1;
	this->bitmapOption = bitmapOption;
    this->context = ctxt;

    this->iblur = state.blur;
    this->isize = state.size;
    this->spacing = state.spacing;

    #ifndef NDEBUG
    printf("ascender %f\n",metrics.ascender);
    #endif

    return true;
}

bool TextIter::to_next(Quad *quad){
    Glyph *glyph;
    if(str == end){
        return false;
    }
    char32_t ch = Utf8Decode(str);
    x = nextx;
    y = nexty;

    FontParams params;
    params.codepoint = ch;
    params.blur = iblur;
    params.size = isize;
    params.context = context;
    params.spacing = spacing;

    glyph = font->get_glyph(params,bitmapOption);
    if(glyph == nullptr){
        return false;
    }

    //Generate quad
    float itw = 1.0f / context->bitmap_w;
    float ith = 1.0f / context->bitmap_h;
    //Mark glyph output
    float y_offset = metrics.ascender - glyph->bitmap_top;
    float act_x = x;
    float act_y = y + y_offset;

    quad->x0 = act_x;
    quad->x1 = act_x + glyph->width;
    quad->y0 = act_y;
    quad->y1 = act_y + glyph->height;

    //Mark glyph source
    quad->s0 = glyph->x * itw;
    quad->t0 = glyph->y * ith;
    quad->s1 = (glyph->x + glyph->width) * itw;
    quad->t1 = (glyph->y + glyph->height)* ith;

    //Debug print
    #ifndef FONS_NDEBUG
    printf("glyph pen_y %f bitmap_top %d height %d width %d\n",y_offset,glyph->bitmap_top,glyph->height,glyph->width);
    #endif
    //Move forward
    nextx += glyph->advance_x;

    if(prevGlyphIndex != -1){
        nextx  += font->kerning(prevGlyphIndex,ch);
    }
    prevGlyphIndex = ch;

    return true;
}

FONS_NS_END