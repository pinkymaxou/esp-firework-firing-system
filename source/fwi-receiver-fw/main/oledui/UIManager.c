#include "UIManager.h"
#include "UIHome.h"
#include "UIArmed.h"
#include "UISetting.h"
#include "UIErrorPleaseDisarm.h"
#include "UIMenu.h"
#include "UITestConn.h"

static const UICORE_SLifeCycle m_sUIHomes[] =
{
    [UIMANAGER_EMENU_Home] = UIHOME_INITLIFECYCLE,
    [UIMANAGER_EMENU_ArmedReady] = UIARMED_INITLIFECYCLE,
    [UIMANAGER_EMENU_ErrorPleaseDisarm] = UIERRORPLEASEDISARM_INITLIFECYCLE,
    [UIMANAGER_EMENU_Menu] = UIMENU_INITLIFECYCLE,
    [UIMANAGER_EMENU_Setting] = UISETTING_INITLIFECYCLE,
    [UIMANAGER_EMENU_TestConn] = UITESTCONN_INITLIFECYCLE,
};
static_assert(UIMANAGER_EMENU_Count == (sizeof(m_sUIHomes)/sizeof(m_sUIHomes[0])), "Life cycle doesn't fit");

static UIMANAGER_EMENU m_eCurrentMenu = -1;

static const UICORE_SLifeCycle* GetMenuLC(UIMANAGER_EMENU eMenu);

void UIMANAGER_Goto(UIMANAGER_EMENU eMenu)
{
    const UICORE_SLifeCycle* pLC = GetMenuLC(eMenu);
    if (pLC == NULL)
        return;
    if (m_eCurrentMenu == eMenu) // Already there
        return;
    const UICORE_SLifeCycle* pOldLC = GetMenuLC(m_eCurrentMenu);
    if (pOldLC != NULL)
    {
        pOldLC->FnExit();
        m_eCurrentMenu = -1;
    }
    m_eCurrentMenu = eMenu;
    pLC->FnEnter();
}

void UIMANAGER_RunTick()
{
    const UICORE_SLifeCycle* pLC = GetMenuLC(m_eCurrentMenu);
    if (pLC == NULL)
        return;
    pLC->FnTick();
}

void UIMANAGER_EncoderMove(UICORE_EBTNEVENT eBtnEvent, int32_t s32ClickCount)
{
    const UICORE_SLifeCycle* pLC = GetMenuLC(m_eCurrentMenu);
    if (pLC == NULL)
        return;
    pLC->FnEncoderMove(eBtnEvent, s32ClickCount);
}

static const UICORE_SLifeCycle* GetMenuLC(UIMANAGER_EMENU eMenu)
{
    if ((int32_t)eMenu < 0 || (int32_t)eMenu >= UIMANAGER_EMENU_Count)
        return NULL;
    return &m_sUIHomes[(int32_t)eMenu];
}