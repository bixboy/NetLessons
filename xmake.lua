add_rules("mode.debug", "mode.release")

set_languages("c++17")
add_requires("sfml 3.0.1")

target("CommonNet")
    set_kind("static")
    add_headerfiles("src/CommonNet/**.h")
    add_includedirs("src/CommonNet", {public = true})
    
    if is_plat("windows", "mingw") then
        add_syslinks("ws2_32", {public = true}) 
    end

-- Le Serveur
target("Server")
    set_kind("binary")
    add_deps("CommonNet")
    add_files("src/Server/**.cpp")
    add_headerfiles("src/Server/**.h")

-- Le Client
target("Client")
    set_kind("binary")
    add_deps("CommonNet")
    add_packages("sfml")
    add_files("src/Client/**.cpp")
    add_headerfiles("src/Client/**.h")
    
    if is_plat("windows") then
        add_ldflags("/SUBSYSTEM:WINDOWS", "/ENTRY:mainCRTStartup")
        add_syslinks("ole32")
    end
    
    after_build(function (target)
        os.cp("resources/**", target:targetdir())
    end)