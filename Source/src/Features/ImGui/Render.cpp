#include "Render.h"
#include "Menu/CustomUI/CustomUI.h"
#include "../../Hooks/Direct3DDevice9.h"
#include <ImGui/imgui_impl_win32.h>
#include "Fonts/MaterialDesign/MaterialIcons.h"
#include "Fonts/MaterialDesign/IconDefinitions.h"
#include "Fonts/CascadiaMono/CascadiaMono.h"
#include "Fonts/Roboto/RobotoMedium.h"
#include "Fonts/Roboto/RobotoBlack.h"
#include "Menu/Menu.h"

void CRender::Render(IDirect3DDevice9* pDevice)
{
	static std::once_flag tFlag; std::call_once(tFlag, [&]
	{
		Initialize(pDevice);
	});

	LoadColors();
	{
		static float flStaticScale = Vars::Menu::Scale.Value;
		float flOldScale = flStaticScale;
		float flNewScale = flStaticScale = Vars::Menu::Scale.Value;
		if (flNewScale != flOldScale)
			Reload();
	}

	DWORD dwOldRGB; pDevice->GetRenderState(D3DRS_SRGBWRITEENABLE, &dwOldRGB);
	pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, false);
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	F::Menu.Render();
	//F::CustomUI.Render();
	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
	pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, dwOldRGB);
}

void CRender::LoadColors()
{
	auto fColorToVec = [](Color_t tColor) -> ImColor
	{
		return { tColor.r / 255.f, tColor.g / 255.f, tColor.b / 255.f, tColor.a / 255.f };
	};
	Black = ImVec4(0.f, 0.f, 0.f, 1.f);
	Red = fColorToVec(Vars::Colors::IndicatorTextBad.Value);
	Yellow = fColorToVec(Vars::Colors::IndicatorTextMid.Value);
	Green = fColorToVec(Vars::Colors::IndicatorTextGood.Value);
	Accent = fColorToVec(Vars::Menu::Theme::Accent.Value);
	AccentDark = fColorToVec(Vars::Menu::Theme::Accent.Value.Lerp({ 0, 0, 0 }, 3.f / 9, LerpEnum::NoAlpha));
	Background0 = fColorToVec(Vars::Menu::Theme::Background.Value);
	Background0p5 = fColorToVec(Vars::Menu::Theme::Background.Value.Lerp({ 127, 127, 127 }, 0.5f / 9, LerpEnum::NoAlpha));
	Background1 = fColorToVec(Vars::Menu::Theme::Background.Value.Lerp({ 127, 127, 127 }, 1.f / 9, LerpEnum::NoAlpha));
	Background1p5 = fColorToVec(Vars::Menu::Theme::Background.Value.Lerp({ 127, 127, 127 }, 1.5f / 9, LerpEnum::NoAlpha));
	Background1p5L = { Background1p5.Value.x * 1.1f, Background1p5.Value.y * 1.1f, Background1p5.Value.z * 1.1f, Background1p5.Value.w };
	Background2 = fColorToVec(Vars::Menu::Theme::Background.Value.Lerp({ 127, 127, 127 }, 2.f / 9, LerpEnum::NoAlpha));
	Inactive = fColorToVec(Vars::Menu::Theme::Inactive.Value);
	Active = fColorToVec(Vars::Menu::Theme::Active.Value);

	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Border] = Black;
	colors[ImGuiCol_Button] = {};
	colors[ImGuiCol_ButtonHovered] = {};
	colors[ImGuiCol_ButtonActive] = {};
	colors[ImGuiCol_FrameBg] = Background1p5;
	colors[ImGuiCol_FrameBgHovered] = Background1p5L;
	colors[ImGuiCol_FrameBgActive] = Background1p5;
	colors[ImGuiCol_Header] = {};
	colors[ImGuiCol_HeaderHovered] = { Background1p5L.Value.x * 1.1f, Background1p5L.Value.y * 1.1f, Background1p5L.Value.z * 1.1f, Background1p5.Value.w }; // divd by 1.1
	colors[ImGuiCol_HeaderActive] = Background1p5;
	colors[ImGuiCol_ModalWindowDimBg] = { Background0.Value.x, Background0.Value.y, Background0.Value.z, 0.4f };
	colors[ImGuiCol_PopupBg] = Background1p5L;
	colors[ImGuiCol_ResizeGrip] = {};
	colors[ImGuiCol_ResizeGripActive] = {};
	colors[ImGuiCol_ResizeGripHovered] = {};
	colors[ImGuiCol_ScrollbarBg] = {};
	colors[ImGuiCol_Text] = Active;
	colors[ImGuiCol_WindowBg] = {};
}

void CRender::LoadFonts()
{
	auto& io = ImGui::GetIO();

	if (static bool bLoaded = false; !bLoaded)
		bLoaded = true;
	else
		io.Fonts->Clear();

	ImFontConfig tFontConfig;
	tFontConfig.OversampleH = 1;

	FontSmall = io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\TAHOMABD.TTF)", H::Draw.Scale(13), &tFontConfig);
	FontRegular = io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\TAHOMABD.TTF)", H::Draw.Scale(14), &tFontConfig);
	FontBold = io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\TAHOMABD.TTF)", H::Draw.Scale(14), &tFontConfig);
	FontLarge = io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\TAHOMABD.TTF)", H::Draw.Scale(15), &tFontConfig);
	FontMono = io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\cour.ttf)", H::Draw.Scale(16), &tFontConfig); // windows mono font installed by default
	CritIndicator = io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\SEGOEUI.TTF)", H::Draw.Scale(16), &tFontConfig);
	DTIndicator = io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\SEGOEUI.TTF)", H::Draw.Scale(20), &tFontConfig);


	ImFontConfig tIconConfig;
	tIconConfig.PixelSnapH = true;
	IconFont = io.Fonts->AddFontFromMemoryCompressedTTF(MaterialIcons_compressed_data, MaterialIcons_compressed_size, H::Draw.Scale(16), &tIconConfig);

	io.Fonts->Build();
	io.ConfigDebugHighlightIdConflicts = false;
}

void CRender::LoadStyle()
{
	auto& style = ImGui::GetStyle();
	style.ButtonTextAlign = { 0.5f, 0.5f };
	style.CellPadding = { H::Draw.Scale(4), 0 };
	style.ChildBorderSize = 0.f;
	style.ChildRounding = H::Draw.Scale(0);
	style.FrameBorderSize = 0.f;
	style.FramePadding = { 0, 0 };
	style.FrameRounding = H::Draw.Scale(0);
	style.ItemInnerSpacing = { 0, 0 };
	style.ItemSpacing = { H::Draw.Scale(8), H::Draw.Scale(8) };
	style.PopupBorderSize = 0.f;
	style.PopupRounding = H::Draw.Scale(0);
	style.ScrollbarSize = 6.f + H::Draw.Scale(3);
	style.ScrollbarRounding = 0.f;
	style.WindowBorderSize = 0.f;
	style.WindowPadding = { 0, 0 };
	style.WindowRounding = H::Draw.Scale(0);
	style.ScrollbarSize = H::Draw.Scale(2.0f);
}

void CRender::Initialize(IDirect3DDevice9* pDevice)
{
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(WndProc::hwWindow);
	ImGui_ImplDX9_Init(pDevice);

	auto& io = ImGui::GetIO();
	//io.IniFilename = nullptr;
	io.LogFilename = nullptr;

	LoadFonts();
	LoadStyle();

	m_bLoaded = true;
}

void CRender::Reload()
{
	m_bLoaded = false;

	LoadFonts();
	LoadStyle();

	m_bLoaded = true;
}