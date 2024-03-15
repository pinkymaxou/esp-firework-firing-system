#include "UIManager.hpp"

UIManager g_uiMgr;

void UIManager::Goto(EMenu eMenu)
{
    UIBase* pLC = GetMenuLC(eMenu);
    if (pLC == NULL)
        return;
    if (m_eCurrentMenu == eMenu) // Already there
        return;
    UIBase* pOldLC = GetMenuLC(m_eCurrentMenu);
    if (pOldLC != NULL)
    {
        pOldLC->OnExit();
        m_eCurrentMenu = EMenu::None;
    }
    m_eCurrentMenu = eMenu;
    pLC->OnEnter();
}

void UIManager::RunTick()
{
    UIBase* pLC = GetMenuLC(m_eCurrentMenu);
    if (pLC == NULL)
        return;
    pLC->OnTick();
}

void UIManager::EncoderMove(UIBase::BTEvent eBtnEvent, int32_t s32ClickCount)
{
    UIBase* pLC = GetMenuLC(m_eCurrentMenu);
    if (pLC == NULL)
        return;
    pLC->OnEncoderMove(eBtnEvent, s32ClickCount);
}

UIBase* UIManager::GetMenuLC(EMenu eMenu)
{
    if ((int32_t)eMenu < 0 || (int32_t)eMenu >= (int32_t)UIManager::EMenu::Count)
        return NULL;
    return m_psUIHomes[(int32_t)eMenu];
}