#include "../SDK/SDK.h"
#include "../Features/Visuals/Notifications/Notifications.h"
#include "../Features/Visuals/ESP/ESP.h"
#include "../Features/Visuals/OffscreenArrows/OffscreenArrows.h"
#include "../Features/Visuals/CameraWindow/CameraWindow.h"
#include "../Features/Visuals/Visuals.h"
#include "../Features/Ticks/Ticks.h"
#include "../Features/CritHack/CritHack.h"
#include "../Features/Visuals/SpectatorList/SpectatorList.h"
#include "../Features/Backtrack/Backtrack.h"
#include "../Features/Visuals/PlayerConditions/PlayerConditions.h"
#include "../Features/NoSpread/NoSpreadHitscan/NoSpreadHitscan.h"
#include "../Features/Resolver/Resolver.h"
#include "../Features/Aimbot/Aimbot.h"
#include "../Features/PacketManip/AntiAim/AntiAim.h"
#include "../Features/Aimbot/AutoHeal/AutoHeal.h"
#include "../Core/Core.h"
#define PI 3.14159265358979323846f
MAKE_HOOK(IEngineVGui_Paint, U::Memory.GetVirtual(I::EngineVGui, 14), void,
	void* rcx, int iMode)
{
	DEBUG_RETURN(IEngineVGui_Paint, rcx, iMode);

	if (G::Unload)
		return CALL_ORIGINAL(rcx, iMode);

	if (iMode & PAINT_INGAMEPANELS && !SDK::CleanScreenshot())
	{
		H::Draw.UpdateScreenSize();
		H::Draw.UpdateW2SMatrix();

		H::Draw.Start(true);
		
		if (auto pLocal = H::Entities.GetLocal())
		{
			F::CameraWindow.Draw();

			F::AntiAim.Draw(pLocal);
			F::Visuals.DrawPickupTimers();
			F::ESP.Draw();
			F::OffscreenArrows.Draw(pLocal);
			F::Aimbot.Draw(pLocal);


			F::AutoHeal.Draw(pLocal);
			F::NoSpreadHitscan.Draw(pLocal);
			F::Resolver.Draw();
			F::PlayerConditions.Draw(pLocal);
			//F::Backtrack.Draw(pLocal);
			F::SpectatorList.Draw(pLocal);
			F::CritHack.Draw(pLocal);
			F::Visuals.Draw_Ticks(pLocal);
			F::Visuals.DrawFeatureWindow(pLocal);
			F::Visuals.DrawSpotify(pLocal);
			F::Visuals.DrawDebugInfo(pLocal);

			if (G::HasSavedPos)
			{
				constexpr float flRadius = 35.f;          // diameter 16 → radius 8
				constexpr int   iSegments = 24;          // higher = smoother

				Vector vCenter = G::SavedPos;

				// Optional: make the circle face the player so it looks better
				Vector vEye = H::Entities.GetLocal()->GetShootPos();
				Vector vDir = (vEye - vCenter).Normalized();

				// Build two perpendicular vectors for the circle plane
				Vector vRight, vUp;
				Math::AngleVectors(vDir, &vRight, &vUp);  // or your own AngleVectors + CrossProduct

				std::vector<Vector> vPoints;
				vPoints.reserve(iSegments);

				for (int i = 0; i < iSegments; i++)
				{
					float flAngle = (i / float(iSegments)) * 2.f * PI;
					Vector vOffset = (vRight * cosf(flAngle) + vUp * sinf(flAngle)) * flRadius;
					vPoints.push_back(vCenter + vOffset);
				}

				// Project & draw the circle
				for (int i = 0; i < iSegments; i++)
				{
					Vector s1, s2;
					if (SDK::W2S(vPoints[i], s1) && SDK::W2S(vPoints[(i + 1) % iSegments], s2))
					{
						H::Draw.Line(s1.x, s1.y, s2.x, s2.y, Vars::Menu::Theme::Accent.Value);
					//	H::Draw.LineCircle(s1.x, s1.y, 2.f, 24, Vars::Menu::Theme::Accent.Value);
					//	H::Draw.FillCircle(s1.x, s1.y, 2.f, 24, Vars::Menu::Theme::Accent2.Value);
					}
				}
				
			}
		}
		H::Draw.End();
	}

	CALL_ORIGINAL(rcx, iMode);

	if (iMode & PAINT_UIPANELS && !SDK::CleanScreenshot())
	{
		H::Draw.UpdateScreenSize();
		H::Draw.UpdateKeyStrings();

		H::Draw.Start();
		{
			//F::Notifications.Draw();
			U::Core.SplashScreen();
			
		}
		H::Draw.End();
	}
}
