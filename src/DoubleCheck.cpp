#include "../include/DoubleCheck.h"
#include "../include/Globals.h"
#include "../include/BarterMenuEx.h"

void Main::SetupLog()
{
	auto logsFolder = SKSE::log::log_directory();
	if (!logsFolder)
	{
		SKSE::stl::report_and_fail("SKSE log_directory not provided, logs disabled.");
	}

	auto pluginName = SKSE::PluginDeclaration::GetSingleton()->GetName();
	auto logFilePath = *logsFolder / std::format("{}.log", pluginName);

	auto fileLoggerPtr = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath.string(), true);
	g_Logger = std::make_shared<spdlog::logger>("log", std::move(fileLoggerPtr));
	spdlog::set_default_logger(g_Logger);
	spdlog::set_level(spdlog::level::trace);
	spdlog::flush_on(spdlog::level::trace);
}

void Main::LoadINI()
{
	CSimpleIniA ini;
	ini.SetUnicode(false);
	ini.LoadFile(L"Data\\SKSE\\Plugins\\DoubleCheckBeforeSelling.ini");

	const char* sectionFeatures = "Features";

	EnableCheckForEquipped = ini.GetBoolValue(sectionFeatures, "EnableCheckForEquipped");
	EnableCheckForFavourited = ini.GetBoolValue(sectionFeatures, "EnableCheckForFavourited");
	EnableCheckForUnique = ini.GetBoolValue(sectionFeatures, "EnableCheckForUnique");

	DEBUG_LOG(g_Logger, "EnableCheckForEquipped: {} - EnableCheckForFavourited: {} - EnableCheckForUnique: {}", EnableCheckForEquipped, EnableCheckForFavourited, EnableCheckForUnique);
}

void MessageListener(SKSE::MessagingInterface::Message* message)
{
	switch (message->type)
	{
	case SKSE::MessagingInterface::kDataLoaded:
	{
		SKSE::Translation::ParseTranslation(Plugin::NAME.data());
		CheckBeforeSelling::BarterMenuEx::InstallHook();
	}
	break;

	default:
		break;

	}
}

void Main::Setup()
{
	SetupLog();
	LoadINI();
	SKSE::GetMessagingInterface()->RegisterListener(MessageListener);

	g_Logger->info("{} v{} loaded", Plugin::NAME, Plugin::VERSION);
}

SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
	SKSE::Init(skse);
	Main plugin;
	plugin.Setup();

	return true;
}

#define DLLEXPORT __declspec(dllexport)

extern "C" DLLEXPORT const auto SKSEPlugin_Version = []() noexcept {
	SKSE::PluginVersionData v;
	v.PluginName(Plugin::NAME.data());
	v.PluginVersion(Plugin::VERSION);
	v.UsesAddressLibrary(true);
	v.HasNoStructUse();
	return v;
	}
();

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface*, SKSE::PluginInfo * pluginInfo)
{
	pluginInfo->name = SKSEPlugin_Version.pluginName;
	pluginInfo->infoVersion = SKSE::PluginInfo::kVersion;
	pluginInfo->version = SKSEPlugin_Version.pluginVersion;
	return true;
}