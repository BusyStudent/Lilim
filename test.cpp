#include <SDL2/SDL.h>
#define LILIM_STATIC
#define FONS_STATIC
#include "lilim.hpp"
#include "lilim.cpp"
#include "fontstash.cpp"

#include <iostream>
int main(){
    Lilim::Manager manager;
    Lilim::FaceSize size;

    Fons::Fontstash stash(manager);
    Fons::Context context(stash);

    // auto face = manager.new_face("/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",0);
    auto face = manager.new_face("./msyh.ttc",0);
    auto f2 = face;
    auto f3 = face;
    
    // size.width = 16;
    // size.height = 0;
    // size.xdpi = 96;
    // size.ydpi = 96;
    // face->set_size(size);
    // auto m = face->metrics();
    face->set_dpi(96,96);


    // std::cout << face.empty() << std::endl;
    // std::cout << m.ascender << std::endl;
    // std::cout << m.descender << std::endl;
    // std::cout << m.height << std::endl;
    // std::cout << m.max_advance << std::endl;

    int id = stash.add_font(face);
    context.add_font(id);
    context.set_align(FONS_ALIGN_LEFT | FONS_ALIGN_TOP);
    context.set_size(12);
    context.set_font(id);

    // auto ssize = context.measure_text("你好世界");
    // auto fsize = face->measure_text("你好世界");
    // std::cout << "w:" << ssize.width << " h:" << ssize.height << std::endl;
    // std::cout << "w:" << fsize.width << " h:" << fsize.height << std::endl;

    Fons::TextIter iter;
    Fons::Quad quad;
    iter.init(&context,0,0,"你A好世にほん界asdsds啊啊啊哈哈哈",nullptr,FONS_GLYPH_BITMAP_REQUIRED);

    int w,h;
    uint8_t *tex = (uint8_t*)context.get_data(&w,&h);

    int surf_w = iter.width;
    int surf_h = iter.height;

    std::cout << "w:" << surf_w << " h:" << surf_h << std::endl;

    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(
        0,
        surf_w,
        surf_h,
        32,
        SDL_PIXELFORMAT_RGBA32
    );
    Uint32 *pixel = (Uint32*)surf->pixels;

    while(iter.to_next(&quad)){
        //Output quad
        printf("Dst(%f,%f,%f,%f)\n",quad.x0,quad.y0,quad.x1,quad.y1);
        printf("InTex(%f,%f,%f,%f)\n\n",quad.s0,quad.t0,quad.s1,quad.t1);

        //Print them in tex
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

        printf("Glyph(%d,%d)\n\n",int(d_w),int(d_h));

        // for(int y = 0;y < d_h; y++){
        //     for(int x = 0;x < d_w; x++){
        //         std::cout << (tex[int((y + d_y) * w + (x + d_x))] ? '#' : ' '); 
        //     }
        //     std::cout << std::endl;
        // }
        for(int y = 0;y < d_h; y++){
            for(int x = 0;x < d_w; x++){
                Uint8 pix = tex[int((y + d_y) * w + (x + d_x))];

                pixel[(y + to_y) * surf->w + (x + to_x)] = SDL_MapRGBA(
                    surf->format,
                    0,
                    0,
                    0,
                    pix
                );
            }
        }
    }

    context.dump_info();

    int dirty[4];
    if(context.validate(dirty)){
        printf("Dirty (%d,%d,%d,%d)\n",dirty[0],dirty[1],dirty[2],dirty[3]);
    }



    // for(int y = 0; y < h; y++){
    //     for(int x = 0; x < w; x++){
    //         std::cout << (tex[y * w + x] ? '#' : ' ');
    //     }
    //     std::cout << std::endl;
    // }

//     auto bitmap = face->render_text("你好H世E界LM啊A啊S啦A啦S");
    

//     //Print bitmap
//     for(int y = 0; y < bitmap.height; y++){
//         for(int x = 0; x < bitmap.width; x++){
//             std::cout << (bitmap->as<uint8_t>()[y * bitmap.width + x] ? '#' : ' ');
//         }
//         std::cout << std::endl;
//     }

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Hello World",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        640,
        480,
        SDL_WINDOW_SHOWN
    );
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
//     SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(
//         0,
//         bitmap.width,
//         bitmap.height,
//         32,
//         SDL_PIXELFORMAT_RGBA32
//     );
//     //Copy bitmap to surface
//     Uint32 *pixels = static_cast<Uint32*>(surface->pixels);
//     for(int y = 0; y < bitmap.height; y++){
//         for(int x = 0; x < bitmap.width; x++){
//             pixels[y * bitmap.width + x] = SDL_MapRGBA(
//                 surface->format,
//                 bitmap->as<uint8_t>()[y * bitmap.width + x],
//                 bitmap->as<uint8_t>()[y * bitmap.width + x],
//                 bitmap->as<uint8_t>()[y * bitmap.width + x],
//                 255
//             );
//         }
//     }

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
        SDL_SetRenderDrawColor(renderer,255,255,255,255);
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