add_rules("mode.debug", "mode.release")

set_languages("c++20")
add_requires("sfml 3.0.1")

if is_plat("windows") then
    add_syslinks("ole32") 
end

target("CommonNet")
    set_kind("static")
    on_install(function (target) end)
    
    add_headerfiles("src/CommonNet/**.h")
    add_includedirs("src/CommonNet", {public = true})
    
    if is_plat("windows", "mingw") then
        add_syslinks("ws2_32", {public = true}) 
    end

-- Le Serveur
target("Server")
    set_kind("binary")
    add_deps("CommonNet")
    add_files("src/Server/private/**.cpp")
    add_headerfiles("src/Server/public/**.h")
    add_filegroups("", {rootdir = "src/Server", files = {"src/Server/**"}})

-- Le Client
target("Client")
    set_kind("binary")
    add_deps("CommonNet")
    add_packages("sfml")
    add_files("src/Client/**.cpp")
    add_headerfiles("src/Client/**.h")
    
    if is_plat("windows") then
        add_ldflags("/SUBSYSTEM:WINDOWS", "/ENTRY:mainCRTStartup", {force = true})
    end
    
    after_build(function (target)
        local dest_dir = path.join(target:targetdir(), "assets")
        if not os.exists(dest_dir) then
            os.mkdir(dest_dir)
        end
                
        os.cp("resources/*", dest_dir)
    end)

    add_installfiles("resources/**", {prefixdir = "assets"})