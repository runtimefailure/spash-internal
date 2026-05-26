#include <filesystem>
#include <fstream>
#include "rbx/update/globals.hpp"
#include <environment/libs/closures.hpp>
#include <execution/execution.hpp>
#include <shared.hpp>

namespace file {
    std::string get_workspace() {
        if (!std::filesystem::exists(exploit::workspace)) {
			std::filesystem::create_directories(exploit::workspace);
        }
        return exploit::workspace.string() + "\\";
    };

    std::string get_autoexecute() {
        if (!std::filesystem::exists(exploit::autoexec)) {
            std::filesystem::create_directories(exploit::autoexec);
        }
        return exploit::autoexec.string() + "\\";
    }

    static void auto_execute() {
        std::filesystem::path folder = get_autoexecute();

        for (const auto& entry : std::filesystem::directory_iterator(folder)) {
            if (!entry.is_regular_file()) continue;

            std::string ext = entry.path().extension().string();
            if (ext == ".luau" || ext == ".txt" || ext == ".lua") {
                std::ifstream file(entry.path());
                if (!file.is_open()) continue;

                std::string script((std::istreambuf_iterator<char>(file)),
                    std::istreambuf_iterator<char>());
                execution::execute(shared::LocalState,"if not game:IsLoaded() then game.Loaded:Wait() end wait(1) \n" + script);
            }
        }
    }

    void _SplitString(std::string Str, std::string By, std::vector<std::string>& Tokens)
    {
        Tokens.push_back(Str);
        const auto splitLen = By.size();
        while (true)
        {
            auto frag = Tokens.back();
            const auto splitAt = frag.find(By);
            if (splitAt == std::string::npos)
                break;
            Tokens.back() = frag.substr(0, splitAt);
            Tokens.push_back(frag.substr(splitAt + splitLen, frag.size() - (splitAt + splitLen)));
        }
    }

    int makefolder(lua_State* L) {
        luaL_checktype(L, 1, LUA_TSTRING);

        std::string Path = luaL_checklstring(L, 1, 0);

        std::replace(Path.begin(), Path.end(), '\\', '/');
        std::vector<std::string> Tokens;
        _SplitString(Path, "/", Tokens);

        std::string CurrentPath = get_workspace();
        std::replace(CurrentPath.begin(), CurrentPath.end(), '\\', '/');

        for (const auto& Token : Tokens) {
            CurrentPath += Token + "/";

            if (!std::filesystem::is_directory(CurrentPath))
                std::filesystem::create_directory(CurrentPath);
        }

        return 0;
    };

    int isfile(lua_State* L) {
        luaL_checktype(L, 1, LUA_TSTRING);

        std::string Path = luaL_checklstring(L, 1, 0);

        std::string path = get_workspace() + Path;
        std::replace(path.begin(), path.end(), '\\', '/');

        lua_pushboolean(L, std::filesystem::is_regular_file(path));
        return 1;
    };

    int readfile(lua_State* L) {
        luaL_checktype(L, 1, LUA_TSTRING);

        std::string Path = luaL_checklstring(L, 1, 0);

        std::string path = get_workspace() + Path;
        std::replace(path.begin(), path.end(), '\\', '/');

        if (std::to_string(std::filesystem::is_regular_file(path)) == "1")
        {
            std::ifstream File(path, std::ios::binary);
            if (!File)
                luaL_error(L, "Failed to open file: %s", path.c_str());

            std::string content((std::istreambuf_iterator<char>(File)), std::istreambuf_iterator<char>());
            File.close();

            lua_pushlstring(L, content.data(), content.size());
            return 1;
        }
        else
            luaL_error(L, "Failed to open file: %s", path.c_str());

        return 0;
    }


    int writefile(lua_State* L) {
        luaL_checktype(L, 1, LUA_TSTRING);
        luaL_checktype(L, 2, LUA_TSTRING);

        size_t size = 0;
        std::string Path = luaL_checklstring(L, 1, 0);
        const auto content = luaL_checklstring(L, 2, &size);

        std::replace(Path.begin(), Path.end(), '\\', '/');

        std::vector<std::string> blacklisted =
        {
            ".exe", ".com", ".bat", ".cmd", ".vbs", ".vbe", ".js", ".jse", ".wsf", ".wsh",
            ".ps1", ".ps1_sys", ".ps2", ".ps2_sys", ".ps3", ".ps3_sys", ".ps4", ".ps4_sys",
            ".ps5", ".ps5_sys", ".ps6", ".ps6_sys", ".ps7", ".ps7_sys", ".ps8", ".ps8_sys",
            ".psm1", ".psm1_sys", ".psd1", ".psd1_sys", ".psh1", ".psh1_sys", ".msc", ".msc_sys",
            ".msh", ".msh_sys", ".msh1", ".msh1_sys", ".msh2", ".msh2_sys", ".mshxml", ".mshxml_sys",
            ".vshost", ".vshost_sys", ".vbscript", ".vbscript_sys", ".wsh1", ".wsh1_sys", ".wsh2",
            ".wsh2_sys", ".wshxml", ".wshxmlsys", ".scf", ".dll", ".msi", ".sh", ".py", ".scr",
            ".pif", ".cpl", ".hta", ".psdl", ".reg", ".inf", ".rgs", ".sct", ".lnk", ".zip", ".rar", ".7z", ".cab",
            ".iso", ".img", ".dmg", ".toast", ".vcd", ".vhd", ".vhdx", ".ps1xml", ".ps2xml", ".psc1", ".psc2",
            ".jar", ".tar", ".gz", ".bz2", ".xz", ".deb", ".rpm", ".apk", ".bin", ".cue", ".xml", ".xaml", ".cs"
        };

        for (std::string extension : blacklisted) {
            if (Path.find(extension) != std::string::npos) {
                luaL_error(L, ("forbidden file extension"));
            }
        }

        std::string path = get_workspace() + Path;
        std::replace(path.begin(), path.end(), '\\', '/');
        std::ofstream file(path, std::ios::beg | std::ios::binary);
        file.write(content, size);
        file.close();

        return 0;
    };

    int listfiles(lua_State* L) {
        luaL_checktype(L, 1, LUA_TSTRING);

        std::string Path = luaL_checklstring(L, 1, 0);

        std::string path = get_workspace() + Path;
        std::replace(path.begin(), path.end(), '\\', '/');
        std::string halfPath = get_workspace();
        std::string workspace = ("\\Workspace\\");

        if (!std::filesystem::is_directory(path))
            luaL_error(L, ("folder does not exist"));

        lua_createtable(L, 0, 0);
        int i = 1;
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            std::string path = entry.path().string().substr(halfPath.length());

            lua_pushinteger(L, i);
            lua_pushstring(L, path.c_str());
            lua_settable(L, -3);
            i++;
        }

        return 1;
    };

    int isfolder(lua_State* L) {
        luaL_checktype(L, 1, LUA_TSTRING);

        std::string Path = luaL_checklstring(L, 1, 0);

        std::string path = get_workspace() + Path;
        std::replace(path.begin(), path.end(), '\\', '/');

        lua_pushboolean(L, std::filesystem::is_directory(path));

        return 1;
    };

    int delfolder(lua_State* L) {
        luaL_checktype(L, 1, LUA_TSTRING);

        std::string Path = luaL_checklstring(L, 1, 0);

        std::string path = get_workspace() + Path;
        std::replace(path.begin(), path.end(), '\\', '/');

        if (!std::filesystem::remove_all(path))
            luaL_error(L, ("folder does not exist"));

        return 0;
    };

    int delfile(lua_State* L) {
        luaL_checktype(L, 1, LUA_TSTRING);

        std::string Path = luaL_checklstring(L, 1, 0);

        std::string path = get_workspace() + Path;
        std::replace(path.begin(), path.end(), '\\', '/');

        if (!std::filesystem::remove(path))
            luaL_error(L, ("file does not exist"));

        return 0;
    };

    int loadfile(lua_State* L) {
        luaL_checktype(L, 1, LUA_TSTRING);

        std::string Path = luaL_checklstring(L, 1, 0);

        std::string path = get_workspace() + Path;
        std::replace(path.begin(), path.end(), '\\', '/');

        if (!std::filesystem::is_regular_file(path))
            luaL_error(L, ("file does not exist"));

        std::ifstream File(path);
        std::string content((std::istreambuf_iterator<char>(File)), std::istreambuf_iterator<char>());
        File.close();

        lua_pop(L, lua_gettop(L));

        lua_pushlstring(L, content.data(), content.size());

        return closures::loadstring(L);
    };

    int appendfile(lua_State* L) {
        luaL_checktype(L, 1, LUA_TSTRING);
        luaL_checktype(L, 2, LUA_TSTRING);

        size_t size = 0;
        std::string Path = luaL_checklstring(L, 1, 0);
        const auto content = luaL_checklstring(L, 2, &size);

        std::replace(Path.begin(), Path.end(), '\\', '/');

        std::string path = get_workspace() + Path;
        std::replace(path.begin(), path.end(), '\\', '/');

        std::ofstream file(path, std::ios::binary | std::ios::app);
        file << content;
        file.close();

        return 0;
    };

    int getcustomasset(lua_State* L) {
        luaL_checktype(L, 1, LUA_TSTRING);

        std::string assetPath = lua_tostring(L, 1);
        std::string pathStr = get_workspace() + assetPath;
        std::replace(pathStr.begin(), pathStr.end(), '\\', '/');
        std::filesystem::path path = pathStr;

        if (!std::filesystem::is_regular_file(path)) {
            luaL_error(L, "Failed to find local asset!");
            return 0;
        }

        std::filesystem::path custom_assets = std::filesystem::current_path()
            / "Extracontent"
            / "diegosploit";

        std::filesystem::path custom_asset = custom_assets / path.filename();

        try {
            if (!std::filesystem::exists(custom_assets))
                std::filesystem::create_directories(custom_assets);

            std::filesystem::copy_file(path, custom_asset, std::filesystem::copy_options::update_existing);
        }
        catch (const std::exception& e) {
            luaL_error(L, std::format("Failed to copy asset: {}", e.what()).c_str());

            return 0;
        }

        std::string Final = "rbxasset://" + std::string("diegosploit") + "/" + custom_asset.filename().string();
        lua_pushlstring(L, Final.c_str(), Final.size());
        return 1;
    }

    void initialize(lua_State* L)
    {
        Function(L, "makefolder", file::makefolder);
        Function(L, "isfile", file::isfile);
        Function(L, "readfile", file::readfile);
        Function(L, "writefile", file::writefile);
        Function(L, "listfiles", file::listfiles);
        Function(L, "isfolder", file::isfolder);
        Function(L, "delfolder", file::delfolder);
        Function(L, "delfile", file::delfile);
        Function(L, "loadfile", file::loadfile);
        Function(L, "appendfile", file::appendfile);
        Function(L, "getcustomasset", file::getcustomasset);
    }
}