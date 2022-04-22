
//Interface for nanovg
class FontParams {
    Context *context;//< Belongs to
    Uint  index;// < Glyph index
    short spacing;
    short blur;
};
class Glyph : public GlyphMetrics {
    int x = -1;//< In bitmap position
    int y = -1;
};

class Font: public Refable<Font> {
    public:
        Font(const Font &) = delete;
        ~Font();

        Ref<Font> clone();
        Glyph    *get_glyph(FontParams paran);
        void      render_glyph(Context *ctxt,Glyph *glyph);
    private:
        Font();

        std::map<FontParams,Glyph> glyphs;
        std::string                name;
        Ref<Face>                  face;
        int                        id;
    friend class Fontstash;
};
/**
 * @brief For managing fonts 
 * 
 */
class Fontstash{
    public:
        Fontstash(Manager &manager);
        ~Fontstash();

        int   add_font(Ref<Face> font);
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

        void push_state();
        void pop_state();

        void set_font(int id){
            states.top().font = id;
        }
        void set_size(float size){
            states.top().size = size;
        }

        void expand_atlas(int width,int height);
        void reset_atlas(int width,int height);
        void get_atlas_size(int *width,int *height);

        void *get_data(int *w,int *h);
        bool  validate(int *dirty);
    private:
        struct State{
            int font = 0;
            int blur = 0;
            int spacing = 0;
            float size;
        };
        // Atlas based on Skyline Bin Packer by Jukka Jylänki
        // From nanovg
        struct AtlasNode{
            short x = 0;
            short y = 0;
            short width = 0;
        };
        struct Atlas{
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

        std::vector<uint8_t>    bitmap;
        std::stack<State>       states;
        Atlas                   atlas;
        Fontstash              *stash;
        int                     bitmap_w;
        int                     bitmap_h;
        int                     dirty_rect[4];
    friend class Font;
};



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
Font::Font(){

}
Font::~Font(){

}

Context::Context(Fontstash &m,int w,int h):atlas(w,h){
    stash = &m;
    bitmap_w = w;
    bitmap_h = h;
    bitmap.resize(bitmap_w * bitmap_h);
}
Context::~Context(){

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
		if (nodes == NULL)
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
}

