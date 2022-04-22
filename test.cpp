#include <SDL2/SDL.h>
#include "lilim.hpp"

#include <iostream>
int main(){
    Lilim::Manager manager;
    Lilim::FaceSize size;

    auto face = manager.new_face("C:/Windows/Fonts/msyh.ttc",0);
    auto f2 = face;
    auto f3 = face;
    
    size.width = 12;
    size.height = 12;
    face->set_size(size);
    auto m = face->metrics();


    std::cout << face.empty() << std::endl;
    std::cout << m.ascender << std::endl;
    std::cout << m.descender << std::endl;
    std::cout << m.height << std::endl;
    std::cout << m.max_advance << std::endl;
    
    auto fsize   = face->measure_text("你好",strlen("你好"));
    auto bitmap = face->render_text("你好",strlen("你好"));

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Hello World",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        640,
        480,
        SDL_WINDOW_SHOWN
    );
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Texture *texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        fsize.width,
        fsize.height
    );
    //Update the texture
    SDL_PixelFormat *format = SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888);
    void *pix = nullptr;
    int pitch = 0;
    SDL_LockTexture(texture, nullptr, &pix, &pitch);

    Uint32 *pixel = static_cast<Uint32*>(pix);

    for(int i = 0;i < size.height;i++){
        for(int l = 0;l < size.width;l++){
            Uint8 alpha = bitmap->as<uint8_t>()[i * fsize.width + l];

            pixel[i * fsize.width + l] = SDL_MapRGBA(
                format,
                255,
                255,
                255,
                alpha
            );
        }
    }
    SDL_UnlockTexture(texture);

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
    SDL_Delay(5000);
}