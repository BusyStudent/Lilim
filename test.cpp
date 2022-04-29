#include <SDL2/SDL.h>
#define LILIM_STATIC
#define FONS_STATIC
#include "lilim.cpp"
#include "fontstash.cpp"

#include <iostream>

#ifdef _WIN32
    #define FONT_FILE "C:/Windows/Fonts/msyh.ttc"
#else
    #define FONT_FILE "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"
#endif


#define FONS_SDL_RENDERER
#include "fons_backend.hpp"

int main(){
    Lilim::Manager manager;
    Lilim::FaceSize size;

    Fons::Fontstash stash(manager);

    // auto face = manager.new_face("/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",0);
    auto face = manager.new_face(FONT_FILE,0);
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

    // auto ssize = context.measure_text("你好世界");
    // auto fsize = face->measure_text("你好世界");
    // std::cout << "w:" << ssize.width << " h:" << ssize.height << std::endl;
    // std::cout << "w:" << fsize.width << " h:" << fsize.height << std::endl;

    //Create a window / renderer
    SDL_Window *window = SDL_CreateWindow("Hello World!", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_RESIZABLE);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    auto context = new Fons::SDLTextRenderer(renderer,stash,640,480);

    //FIXME : When Font size is 27 or biger the text is not correct in windows 

    // context->add_font(id);
    float cur_size = 12;
    float cur_spacing = 0;

    context->set_size(cur_size);
    context->set_font(id);
    context->set_spacing(cur_spacing);


    SDL_Event event;
    float x_offset = 0;
    float y_offset = 0;

    while(SDL_WaitEvent(&event)){
        if(event.type == SDL_QUIT){
            break;
        }
        if(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_EXPOSED){
            redraw:
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);

            context->set_spacing(cur_spacing);
            context->set_size(cur_size);
            context->set_align(FONS_ALIGN_BOTTOM | FONS_ALIGN_CENTER);  // Align to bottom and center

            context->set_color(0xFFFF00FF);
            context->draw_text(0 + x_offset,0 + y_offset,"The Quick Brown Fox Jumps Over The Lazy Dog");

            context->set_color(0xFF0000FF);
            context->draw_text(0 + x_offset,40 + y_offset,"欲穷千里目，更上一层楼");

            context->set_color(0x00FF00FF);
            context->draw_text(0 + x_offset,80 + y_offset,"貧しくなりたいなら、次のレベルに進んでください");

            context->set_color(0x00FFFFFF);
            context->draw_text(0 + x_offset,120 + y_offset,"Если хочешь быть бедным, переходи на следующий уровень");

            context->flush();
            //Draw Detail info
            SDL_Rect detail_rect = {
                0,0,100,50
            };
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderFillRect(renderer,&detail_rect);
            //DRAW Info
            context->set_align(FONS_ALIGN_TOP | FONS_ALIGN_LEFT);
            context->set_color(0x000000FF);
            context->set_spacing(0);
            context->set_size(12);

            context->draw_vtext(0,0,"size:%d",int(cur_size));
            context->draw_vtext(0,20,"spacing:%d",int(cur_spacing));

            context->flush();

            SDL_RenderPresent(renderer);
            continue;
        }
        if(event.type == SDL_MOUSEBUTTONDOWN){
            x_offset = event.button.x;
            y_offset = event.button.y;

            goto redraw;
        }
        if(event.type == SDL_KEYDOWN){
            if(event.key.keysym.sym == SDLK_UP){
                cur_size += 1;
                goto redraw;
            }
            if(event.key.keysym.sym == SDLK_DOWN){
                cur_size -= 1;
                goto redraw;
            }
            if(event.key.keysym.sym == SDLK_LEFT){
                cur_spacing -= 1;
                goto redraw;
            }
            if(event.key.keysym.sym == SDLK_RIGHT){
                cur_spacing += 1;
                goto redraw;
            }

        }
        if(event.type == SDL_MOUSEWHEEL){
            if(event.wheel.y > 0){
                cur_size += 1;
                goto redraw;
            }
            if(event.wheel.y < 0){
                cur_size -= 1;
                goto redraw;
            }
        }
    }

    delete context;

    //CLeanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}