#include "Fonts.h"

#include "../../Definitions/Interfaces/IMatSystemSurface.h"
#include <ranges>

void CFonts::Reload(float flDPI, bool bOutline)
{
	int iFlags = !bOutline ? FONTFLAG_ANTIALIAS : FONTFLAG_ANTIALIAS | FONTFLAG_DROPSHADOW;

	m_mFonts[FONT_ESP] = { "Verdana", int(12.f * flDPI), FONTFLAG_NONE, 600 };
	m_mFonts[FONT_INDICATORS] = { "Small Fonts", int(11.f * flDPI), iFlags, 600 };
	m_mFonts[FONT_CRITHACK] = { "Verdana", int(12.f * flDPI), iFlags, 400 };
	m_mFonts[FONT_ESP_SMALL] = { "Small Fonts", int(9.f * flDPI), iFlags, 0 };
	for (auto& fFont : m_mFonts | std::views::values)
	{
		if (fFont.m_dwFont = I::MatSystemSurface->CreateFont())
			I::MatSystemSurface->SetFontGlyphSet(fFont.m_dwFont, fFont.m_szName, fFont.m_nTall, fFont.m_nWeight, 0, 0, fFont.m_nFlags);
	}
}

const Font_t& CFonts::GetFont(EFonts eFont)
{
	return m_mFonts[eFont];
}