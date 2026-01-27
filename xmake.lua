add_rules("mode.debug", "mode.release")

target("CommonNet")
    set_kind("phony")
    if is_plat("windows", "mingw") then
        add_syslinks("ws2_32")
    end

-- Le Serveur
target("Server")
    set_kind("binary")
    add_deps("CommonNet")
    add_files("src/Server/*.cpp")

-- Le Client
target("Client")
    set_kind("binary")
    add_deps("CommonNet")
    add_files("src/Client/*.cpp")