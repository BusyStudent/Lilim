add_rules("mode.debug", "mode.release")

add_requires("freetype","libsdl")
add_packages("freetype","libsdl")

option("memcheck")
    set_default(false)
    set_showmenu(true)
    set_category("debug")
    set_description("Enable memory checker")
option_end()

if has_config("memcheck") then
    add_cxxflags("-fsanitize=address")
    add_cxxflags("-fno-omit-frame-pointer")
    add_links("asan")
end

if is_plat("windows") then 
    --Let it use UTF-8
    add_cxxflags("/utf-8")
    add_cxxflags("/Zc:__cplusplus")
    add_cflags("/utf-8")    
end

target("Lilim")
    set_kind("static")
    add_files("lilim.cpp")
target("test")
    set_kind("binary")
    add_files("test.cpp")
target("test_cleartype")
    set_kind("binary")
    add_files("test_cleartype.cpp")

--
-- If you want to known more usage about xmake, please see https://xmake.io
--
-- ## FAQ
--
-- You can enter the project directory firstly before building project.
--
--   $ cd projectdir
--
-- 1. How to build project?
--
--   $ xmake
--
-- 2. How to configure project?
--
--   $ xmake f -p [macosx|linux|iphoneos ..] -a [x86_64|i386|arm64 ..] -m [debug|release]
--
-- 3. Where is the build output directory?
--
--   The default output directory is `./build` and you can configure the output directory.
--
--   $ xmake f -o outputdir
--   $ xmake
--
-- 4. How to run and debug target after building project?
--
--   $ xmake run [targetname]
--   $ xmake run -d [targetname]
--
-- 5. How to install target to the system directory or other output directory?
--
--   $ xmake install
--   $ xmake install -o installdir
--
-- 6. Add some frequently-used compilation flags in xmake.lua
--
-- @code
--    -- add debug and release modes
--    add_rules("mode.debug", "mode.release")
--
--    -- add macro defination
--    add_defines("NDEBUG", "_GNU_SOURCE=1")
--
--    -- set warning all as error
--    set_warnings("all", "error")
--
--    -- set language: c99, c++11
--    set_languages("c99", "c++11")
--
--    -- set optimization: none, faster, fastest, smallest
--    set_optimize("fastest")
--
--    -- add include search directories
--    add_includedirs("/usr/include", "/usr/local/include")
--
--    -- add link libraries and search directories
--    add_links("tbox")
--    add_linkdirs("/usr/local/lib", "/usr/lib")
--
--    -- add system link libraries
--    add_syslinks("z", "pthread")
--
--    -- add compilation and link flags
--    add_cxflags("-stdnolib", "-fno-strict-aliasing")
--    add_ldflags("-L/usr/local/lib", "-lpthread", {force = true})
--
-- @endcode
--

