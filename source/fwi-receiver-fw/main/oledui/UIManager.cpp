#include "UIManager.hpp"

UIManager g_uiMgr;

void UIManager::Goto(EMenu eMenu)
{
    UIBase* pLC = GetMenuLC(eMenu);
    if (NULL == pLC)
        return;
    if (m_eCurrentMenu == eMenu) // Already there
        return;
    UIBase* pOldLC = GetMenuLC(m_eCurrentMenu);
    if (NULL != pOldLC)
    {
        pOldLC->onExit();
        m_eCurrentMenu = EMenu::None;
    }
    m_eCurrentMenu = eMenu;
    pLC->onEnter();
}

void UIManager::RunTick()
{
    UIBase* pLC = GetMenuLC(m_eCurrentMenu);
    if (NULL == pLC)
        return;
    pLC->onTick();
}

void UIManager::EncoderMove(UIBase::BTEvent btn_event, int32_t click_count)
{
    UIBase* pLC = GetMenuLC(m_eCurrentMenu);
    if (NULL == pLC)
        return;
    pLC->onEncoderMove(btn_event, click_count);
}

UIBase* UIManager::GetMenuLC(EMenu eMenu)
{
    if (0 > (int32_t)eMenu || (int32_t)eMenu >= (int32_t)UIManager::EMenu::Count)
        return NULL;
    return m_psUIHomes[(int32_t)eMenu];
}
