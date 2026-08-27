#include "Notifications.h"

void CNotifications::Add(const std::string& sText, Color_t tColor, float flLifeTime, float flPanTime)
{
    m_vNotifications.emplace_back(sText, float(SDK::PlatFloatTime()), flLifeTime, flPanTime, tColor);
    while (m_vNotifications.size() > m_iMaxNotifySize)
        m_vNotifications.pop_front();
}

