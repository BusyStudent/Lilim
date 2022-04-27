#define FONS_CLEARTYPE

#include <SDL2/SDL.h>

#include <iostream>
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
    // face->set_flags(FT_LOAD_TARGET_LCD);
    // face->set_dpi(96,96);
    // face->set_size(12);

    // auto metrics = face->metrics();
    // auto glyph = face->glyph_index('A');

    // std::cout << "ascender: " << metrics.ascender << std::endl;
    // std::cout << "descender: " << metrics.descender << std::endl;
    // std::cout << "height: " << metrics.height << std::endl;
    // std::cout << "max_advance: " << metrics.max_advance << std::endl;
    // std::cout << "underline_position: " << metrics.underline_position << std::endl;
    // std::cout << "underline_thickness: " << metrics.underline_thickness << std::endl;

    // auto gm = face->build_glyph(glyph);
    // std::cout << "Glyph metrics:" << std::endl;
    // std::cout << "width: " << gm.width << std::endl;
    // std::cout << "height: " << gm.height << std::endl;
    // std::cout << "left: " << gm.bitmap_left << std::endl;
    // std::cout << "top: " << gm.bitmap_top << std::endl;

    // std::vector<uint32_t> bitmap(gm.width * gm.height);
    // face->render_glyph(glyph,bitmap.data(),gm.width * 4,0,0);

    // //Print the bitmap
    // for(int i = 0; i < gm.height; i++){
    //     for(int j = 0; j < gm.width; j++){
    //         std::cout << (bitmap[i * gm.width + j] ? '#' : ' ');
    //     }
    //     std::cout << std::endl;
    // }

    // SDL_Surface *surf = SDL_CreateRGBSurfaceFrom(
    //     bitmap.data(),
    //     gm.width,
    //     gm.height,
    //     32,
    //     gm.width * 4,
    //     0xFF000000,
    //     0x00FF0000,
    //     0x0000FF00,
    //     0x00000000
    // );
    // SDL_SaveBMP(surf,"test.bmp");
    // SDL_FreeSurface(surf);
    face->set_dpi(96,96);

    int id = stash.add_font(face);
    context.set_align(FONS_ALIGN_LEFT | FONS_ALIGN_TOP);
    context.set_font(id);
    context.set_size(12);

    Fons::TextIter iter;
    Fons::Quad     quad;
    iter.init(&context,0,0,"Hello Cleartype 你好亚像素渲染",nullptr,FONS_GLYPH_BITMAP_REQUIRED);

    int surf_w = iter.width;
    int surf_h = iter.height;

    SDL_Surface *surf = SDL_CreateRGBSurface(
        0,
        surf_w,
        surf_h,
        32,
        0xFF000000,
        0x00FF0000,
        0x0000FF00,
        0x00000000
    );

    int w,h;
    uint32_t *tex = (uint32_t*)context.get_data(&w,&h);
    uint32_t *pixel = (uint32_t*)surf->pixels;

    while(iter.to_next(&quad)){
        float d_w = quad.s1 - quad.s0;
        float d_h = quad.t1 - quad.t0;
        float d_x = quad.s0;
        float d_y = quad.t0;

        d_w *= w;
        d_h *= h;
        d_x *= w;
        d_y *= h;

        int to_x = quad.x0;
        int to_y = quad.y0;

        for(int y = 0;y < d_h; y++){
            for(int x = 0;x < d_w; x++){
                Uint32 pix = tex[int((y + d_y) * w + (x + d_x))];

                pixel[(y + to_y) * surf->w + (x + to_x)] = pix;
            }
        }
    }

    //Create a window and put the surface in it

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Hello World",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        640,
        480,
        SDL_WINDOW_SHOWN
    );
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer,surf);

    SDL_Rect dst = {
        0,
        0,
        surf->w,
        surf->h
    };

    SDL_Event event;
    while(SDL_WaitEvent(&event)){
        if(event.type == SDL_QUIT){
            break;
        }
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, &dst);
        SDL_RenderPresent(renderer);
    }


    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surf);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}