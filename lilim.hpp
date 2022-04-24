#pragma once

#ifndef LILIM_NAMESPACE
    #define LILIM_NAMESPACE Lilim
#endif

#ifdef LILIM_STATIC
    #define LILIM_NS_BEGIN \
        namespace { \
        namespace LILIM_NAMESPACE {
    #define LILIM_NS_END \
        } \
        }
#else
    #define LILIM_NS_BEGIN \
        namespace LILIM_NAMESPACE {
    #define LILIM_NS_END \
        }
#endif

#ifndef LILIM_STBTRUETYPE
    #include <ft2build.h>
    #include FT_FREETYPE_H
#else
    #ifdef LILIM_STBTRUETYPE_H
        #include LILIM_STBTRUETYPE_H
    #endif

    #define FT_Face stbtt_fontinfo
#endif

#include <cstdlib>
#include <cstdint>
#include <cstdio>


LILIM_NS_BEGIN

using Uint = unsigned int;

class Manager;
class Face;
// Bultin refcounter
template<class T>
class Refable {
    public:
        Refable() = default;
        Refable(const Refable &) = delete;
        ~Refable() = default;

        void ref() noexcept{
            ++refcount;
        }
        void unref() noexcept{
            if(--refcount == 0){
                delete static_cast<T*>(this);
            }
        }
    private:
        int refcount = 0;
};
// Managed refcounter
template<class T>
class Ref {
    public:
        Ref() = default;
        Ref(T *p){
            ptr = p;
            ptr->ref();
        }
        Ref(const Ref &r){
            ptr = r.ptr;
            if(ptr != nullptr){
                ptr->ref();
            }
        }
        Ref(Ref &&r){
            ptr = r.ptr;
            r.ptr = nullptr;
        }
        ~Ref(){
            release();
        }

        Ref &operator=(const Ref &r){
            if(this != &r){
                release();
                ptr = r.ptr;
                if(ptr != nullptr){
                    ptr->ref();
                }
            }
            return *this;
        }
        Ref &operator=(Ref &&r){
            if(this != &r){
                release();
                ptr = r.ptr;
                r.ptr = nullptr;
            }
            return *this;
        }
        T *get() const{
            return ptr;
        }
        T *operator->(){
            return ptr;
        }
        T &operator *(){
            return *ptr;
        }

        void release(){
            if(ptr != nullptr){
                ptr->unref();
                ptr = nullptr;
            }
        }
        bool empty() const noexcept {
            return ptr == nullptr;
        }
        operator bool() const noexcept {
            return ptr != nullptr;
        }
    private:
        T *ptr = nullptr;
};


typedef void (*BlobFinalizer)(void *data,size_t size,void *user);

class Blob: public Refable<Blob> {
    public:
        Blob() = default;
        Blob(void *data,size_t size): _data(data), _size(size){}
        Blob(const Blob &) = delete;
        ~Blob(){
            if(_finalizer != nullptr){
                _finalizer(_data,_size,_user);
            }
        }
        void set_finalizer(BlobFinalizer f,void *user){
            _finalizer = f;
            _user = user;
        }

        void *data() const{
            return _data;
        }
        size_t size() const{
            return _size;
        }
        template<class T>
        T   *as() const{
            return static_cast<T*>(_data);
        }
    private:
        BlobFinalizer _finalizer = nullptr;
        void *_user = nullptr;
        void *_data = nullptr;
        size_t _size = 0;
};

typedef void *(*ReallocFunc)(void *ptr  ,size_t size,void *user);
typedef void *(*MallocFunc )(size_t size,void *user);
typedef void  (*FreeFunc   )(void *ptr  ,void *user);

/**
 * @brief Memory Allocator
 * 
 */
class MemHandler {
    public:
        ReallocFunc realloc = nullptr;
        MallocFunc  malloc = nullptr;
        FreeFunc    free = nullptr;
        void       *user = nullptr;
};

/**
 * @brief Resource manager
 * 
 */
class Manager {
    public:
        Manager();
        Manager(const Manager &) = delete;
        ~Manager();

        Ref<Face> new_face(Ref<Blob> blob,int index);
        Ref<Face> new_face(const char *file,int index);
        Ref<Blob> alloc_blob(size_t size);
        Ref<Blob> realloc_blob(Ref<Blob> blob,size_t size);

        void     *malloc (size_t size);
        void     *realloc(void *ptr,size_t size);
        void      free   (void *ptr);
    private:
        #ifndef LILIM_STBTRUETYPE
        FT_Library library;
        #endif
        
        MemHandler memory;
};


class GlyphMetrics {
    public:
        int width;
        int height;
        int bitmap_left;
        int bitmap_top;
        int advance_x;
};
class FaceSize {
    public:
        float height = 0;
        float width = 0;
        float ydpi = 0;
        float xdpi = 0;
};
class Size {
    public:
        int width = 0;
        int height = 0;
};
class FaceMetrics {
    public:
        float ascender = 0;
        float descender = 0;
        float height = 0;
        float max_advance = 0;
        float underline_position = 0;
        float underline_thickness = 0;
};

class Bitmap : public Size {
    public:
        Ref<Blob> data;
        Blob *operator ->() const{
            return data.get();
        }
};

class Face: public Refable<Face> {
    public:
        Face(const Face &) = delete;
        ~Face();

        Ref<Face> clone();
        FT_Face   native_handle() const{
            return face;
        }

        void  set_dpi    (Uint xdpi,Uint ydpi);
        void  set_size   (FaceSize size);
        void  set_size   (Uint     size);
        void  set_flags  (Uint     flags); 
        Uint  kerning    (Uint left,Uint right);
        Uint  glyph_index(char32_t codepoint);
        auto  metrics()              -> FaceMetrics;
        auto  build_glyph(Uint code) -> GlyphMetrics;
        auto  render_glyph(
            Uint code,
            void *buffer,
            int pitch,
            int pen_x,
            int pen_y
        ) -> void;

        auto measure_text(const char *text,const char *end = nullptr) -> Size;
        auto render_text(const char *text,const char *end = nullptr) -> Bitmap;
    private:
        Face();

        Manager  *manager;
        Ref<Blob> blob;
        FT_Face   face;
        Uint      flags; // FT_LOAD_XXX
        Uint      xdpi; // DPI in set_size(Uint)
        Uint      ydpi; // DPI in set_size(Uint)
        Uint      idx; // Face Index 
    friend class Manager;
};

//Math Utility
inline uint32_t  FixedFloor(uint32_t x);
inline uint32_t  FixedCeil(uint32_t x);
//Utility
extern char32_t  Utf8Decode(const char *&str);
extern Ref<Blob> MapFile(const char *path);

//Inline implement
inline uint32_t FixedFloor(uint32_t x){
    return ((x & -64) / 64);
}
inline uint32_t FixedCeil(uint32_t x){
    return (((x + 63) & -64) / 64);
}

//Manager
inline void *Manager::malloc(size_t byte){
    return memory.malloc(byte,memory.user);
}
inline void *Manager::realloc(void *ptr,size_t byte){
    return memory.realloc(ptr,byte,memory.user);
}
inline void  Manager::free(void *ptr){
    return memory.free(ptr,memory.user);
}

//Face
inline void Face::set_flags(Uint flags){
    this->flags = flags;
}
inline void Face::set_dpi(Uint xdpi,Uint ydpi){
    this->xdpi = xdpi;
    this->ydpi = ydpi;
}

LILIM_NS_END

//C-style wrapper

#ifdef LILIM_CSTATIC
    #define LILIM_CAPI(X) static     X
#else
    #define LILIM_CAPI(X) extern "C" X 
#endif

#ifndef _LILIM_SOURCE_
    #define LILIM_HANDLE(X) typedef struct _Lilim_##X *Lilim_##X
#else
    #define LILIM_HANDLE(X) typedef LILIM_NAMESPACE::X Lilim_##X
#endif

LILIM_HANDLE (Manager);
LILIM_HANDLE (Blob);
LILIM_HANDLE (Face);

//Manager
LILIM_CAPI(Lilim_Manager) Lilim_NewManager();
LILIM_CAPI(void         ) Lilim_DestroyManager(Lilim_Manager manager);

//Fac(
LILIM_CAPI(Lilim_Face   ) Lilim_NewFace(Lilim_Manager manager,Lilim_Blob blob,int index);
LILIM_CAPI(Lilim_Face   ) Lilim_CloneFace(Lilim_Face face);
LILIM_CAPI(void         ) Lilim_CloseFace(Lilim_Face face);

//Blob
LILIM_CAPI(Lilim_Blob   ) Lilim_MapData(const void *data,size_t size);
LILIM_CAPI(Lilim_Blob   ) Lilim_MapFile(const char *path);
LILIM_CAPI(void         ) Lilim_FreeBlob(Lilim_Blob blob);

//Cleanup
#undef FT_Face