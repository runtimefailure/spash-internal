
#include <lapi.h>
#include <lualib.h>
#include <shared.hpp>
#include "env.hpp"

#include <libs/http.hpp>
#include <libs/closures.hpp>
#include <libs/misc.hpp>
#include <libs/cache.hpp>
#include "libs/file.hpp"
#include "libs/metatable.hpp"
#include "libs/lz4.hpp"
#include "libs/crypt.hpp"
#include "libs/console.hpp"

lua_CFunction OriginalIndex;
lua_CFunction OriginalNamecall;

std::vector<const char*> blacklisted_funcs = {
    "TestService.Run", "TestService", "Run",
    "OpenVideosFolder", "OpenScreenshotsFolder", "GetRobuxBalance", "PerformPurchase",
    "PromptBundlePurchase", "PromptNativePurchase", "PromptProductPurchase", "PromptPurchase",
    "PromptThirdPartyPurchase", "Publish", "GetMessageId", "OpenBrowserWindow", "RequestInternal",
    "ExecuteJavaScript", "ToggleRecording", "TakeScreenshot", "HttpRequestAsync", "GetLast",
    "SendCommand", "GetAsync", "GetAsyncFullUrl", "RequestAsync", "MakeRequest",
    "AddCoreScriptLocal", "SaveScriptProfilingData", "GetUserSubscriptionDetailsInternalAsync",
    "GetUserSubscriptionStatusAsync", "PerformBulkPurchase", "PerformCancelSubscription",
    "PerformPurchaseV2", "PerformSubscriptionPurchase", "PerformSubscriptionPurchaseV2",
    "PrepareCollectiblesPurchase", "PromptBulkPurchase", "PromptCancelSubscription",
    "PromptCollectiblesPurchase", "PromptGamePassPurchase", "PromptNativePurchaseWithLocalPlayer",
    "PromptPremiumPurchase", "PromptRobloxPurchase", "PromptSubscriptionPurchase",
    "ReportAbuse", "ReportAbuseV3", "ReturnToJavaScript", "OpenNativeOverlay",
    "OpenWeChatAuthWindow", "EmitHybridEvent", "OpenUrl", "PostAsync", "PostAsyncFullUrl",
    "RequestLimitedAsync", "Load", "CaptureScreenshot", "CreatePostAsync", "DeleteCapture",
    "DeleteCapturesAsync", "GetCaptureFilePathAsync", "SaveCaptureToExternalStorage",
    "SaveCapturesToExternalStorageAsync", "GetCaptureUploadDataAsync", "RetrieveCaptures",
    "SaveScreenshotCapture", "Call", "GetProtocolMethodRequestMessageId",
    "GetProtocolMethodResponseMessageId", "PublishProtocolMethodRequest",
    "PublishProtocolMethodResponse", "Subscribe", "SubscribeToProtocolMethodRequest",
    "SubscribeToProtocolMethodResponse", "GetDeviceIntegrityToken", "GetDeviceIntegrityTokenYield",
    "NoPromptCreateOutfit", "NoPromptDeleteOutfit", "NoPromptRenameOutfit", "NoPromptSaveAvatar",
    "NoPromptSaveAvatarThumbnailCustomization", "NoPromptSetFavorite", "NoPromptUpdateOutfit",
    "PerformCreateOutfitWithDescription", "PerformRenameOutfit", "PerformSaveAvatarWithDescription",
    "PerformSetFavorite", "PerformUpdateOutfit", "PromptCreateOutfit", "PromptDeleteOutfit",
    "PromptRenameOutfit", "PromptSaveAvatar", "PromptSetFavorite", "PromptUpdateOutfit"
};

int index_hook(lua_State* L)
{
    if (static_cast<RobloxExtraSpace*>(L->userdata)->Capabilities == MaxCapabilities)
    {
        std::string Key = lua_isstring(L, 2) ? lua_tostring(L, 2) : "";
        for (const char* func : blacklisted_funcs)
        {
            if (Key == func)
            {
                luaL_error(L, "Function '%s' has been disabled for security reasons", func);
                return 0;
            }
        }

        if (static_cast<RobloxExtraSpace*>(L->userdata)->Script.expired())
        {
            if (Key == "HttpGet" || Key == "HttpGetAsync")
            {
                lua_pushcclosure(L, http::httpget, "httpget", 0);
                return 1;
            }
            else if (Key == "GetObjects")
            {
                lua_pushcclosure(L, misc::getobjects, "getobjects", 0);
                return 1;
            }
        }
    }

    return OriginalIndex(L);
};

int namecall_hooks(lua_State* L)
{
    if (static_cast<RobloxExtraSpace*>(L->userdata)->Capabilities == MaxCapabilities)
    {
        std::string Key = L->namecall->data;
        for (const char* func : blacklisted_funcs)
        {
            if (Key == func)
            {
                luaL_error(L, "Function '%s' has been disabled for security reasons", func);
                return 0;
            }
        }

        if (static_cast<RobloxExtraSpace*>(L->userdata)->Script.expired())
        {
            if (Key == "HttpGet" || Key == "HttpGetAsync")
            {
                return http::httpget(L);
            }
            else if (Key == "GetObjects")
            {
                return misc::getobjects(L);
            }
        }
    }

    return OriginalNamecall(L);
};

void setup_hooks(lua_State* L)
{
    int OriginalTop = lua_gettop(L);

    lua_getglobal(L, "game");
    luaL_getmetafield(L, -1, "__index");
    if (lua_type(L, -1) == LUA_TFUNCTION || lua_type(L, -1) == LUA_TLIGHTUSERDATA)
    {
        Closure* ClosureIndex = clvalue(luaA_toobject(L, -1));
        OriginalIndex = ClosureIndex->c.f;
        ClosureIndex->c.f = index_hook;
    }
    lua_pop(L, 1);

    luaL_getmetafield(L, -1, "__namecall");
    if (lua_type(L, -1) == LUA_TFUNCTION || lua_type(L, -1) == LUA_TLIGHTUSERDATA)
    {
        Closure* NamecallClosure = clvalue(luaA_toobject(L, -1));
        OriginalNamecall = NamecallClosure->c.f;
        NamecallClosure->c.f = namecall_hooks;
    }
    lua_pop(L, 1);

    lua_settop(L, OriginalTop);
}


void env::setup(lua_State* L)
{
    cache::initialize(L);
    closures::initialize(L);
    file::initialize(L);
    lz4::initialize(L);
    metatable::initialize(L);
    misc::initialize(L);
    http::initialize(L);
    crypt::initialize(L);
	console::initialize(L);
    lua_newtable(L);
    lua_setglobal(L, "_G");

    lua_newtable(L);
    lua_setglobal(L, "shared");
    setup_hooks(L);
}