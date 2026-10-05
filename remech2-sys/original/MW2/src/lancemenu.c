#include "lancemenu.h"

#include "ai.h"
#include "decomp.h"
#include "menu.h"
#include "menupage.h"
#include "navpoint.h"
#include "players.h"
#include "targeting.h"
#include "team.h"
#include "types.h"

// The order each AI slot was given last, an index into g_orderChoices (7: none); slot 0 is the
// whole star's.
// GLOBAL: MW2 0x100acaf0
MechS32 g_lanceOrders[8] = {7, 7, 7, 7, 7, 0, 0, 0};

// Fills the page with an item per AI slot (from template 5) and a last one (template 6), marking
// the slots whose player is in AI state 12. With no slot, or every slot marked, the page has only
// templates 7 and 6.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x10065f50
MechS32 PrepareCommandComputerPage(MenuDefinition* p_menu, MenuPage* p_page)
{
	MechS32 marked;
	MechS32 slot;
	MechS32 count;
	MechS32 i;
	MechS32 index;
	MechS32 src;

	marked = 0;
	if (!p_page) {
		return FALSE;
	}

	count = GetLocalStarSize() - 1;
	if (count) {
		src = 5;
		slot = count + 1;
		p_page->m_items[slot] = p_page->m_items[src];
		src = 6;
		slot++;
		p_page->m_items[slot] = p_page->m_items[src];
		p_page->m_itemCount = slot + 1;
		for (i = 0; i < count; i++) {
			index = FindStarSlotPlayer(i + 1);
			if (index < g_playerCount && g_players[index]->m_ai.m_state == 12) {
				p_page->m_items[i + 1].m_type = 1;
				marked++;
			}
		}
	}

	if (!count || marked == count) {
		p_page->m_items[0] = p_page->m_items[7];
		p_page->m_items[1] = p_page->m_items[6];
		p_page->m_itemCount = 2;
	}

	return TRUE;
}

// Unless the page's AI slot holds a player outside AI state 12, keeps only the page's last two
// items.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x100660c2
MechS32 PrepareCommandPointPage(MenuDefinition* p_menu, MenuPage* p_page)
{
	MechS32 count;
	MechS32 slot;
	MechS32 index;
	MechS32 src;
	MechU32 unk0x08;
	MechS32 valid;

	if (!p_page) {
		return FALSE;
	}

	unk0x08 = p_page->m_aiSlot;
	count = GetLocalStarSize() - 1;
	valid = unk0x08 <= count;
	if (valid) {
		index = FindStarSlotPlayer(unk0x08);
		valid = index < g_playerCount && g_players[index]->m_ai.m_state != 12;
	}

	if (!valid) {
		src = p_page->m_itemCount;
		slot = 0;
		p_page->m_items[slot] = p_page->m_items[src];
		src = p_page->m_itemCount - 1;
		slot++;
		p_page->m_items[slot] = p_page->m_items[src];
		p_page->m_itemCount = slot + 1;
	}

	return TRUE;
}

// FUNCTION: MW2 0x100661ef
MechS32 GetLanceOrder(MechS32 p_index)
{
	MechS32 state;

	state = 7;
	if (p_index < 8) {
		state = g_lanceOrders[p_index];
	}

	return state;
}

// FUNCTION: MW2 0x10066223
MechS32 GetFormation(MechS32 p_arg)
{
	return GetTeamFormation(g_localStar);
}

// FUNCTION: MW2 0x10066241
void SelectFormation(MechS32 p_formation, MechS32 p_value)
{
	g_lanceOrders[0] = 0;
	SetTeamFormation(g_localStar, p_formation);
	RequestMenuClose(1);
}

// Returns the AI state of the player in AI slot p_index, plus one (0: none).
// Stack-slot permutation: player, ai, index and state.
// FUNCTION: MW2 0x10066272
MechS32 GetSlotAiState(MechS32 p_index)
{
	Player* player;
	PlayerAi* ai;
	MechS32 index;
	MechS32 state;

	state = -1;
	if (p_index < 8) {
		index = FindStarSlotPlayer(p_index);
		if (index != -1) {
			player = g_players[index];
			ai = &player->m_ai;
			if (ai) {
				state = ai->m_state;
			}
		}
	}

	return state + 1;
}

// Gives the control the page's AI slot.
// FUNCTION: MW2 0x100662df
void SetControlSlot(MenuPage* p_page, MenuControl* p_control)
{
	if (!p_page) {
		return;
	}

	if (!p_control) {
		return;
	}

	p_control->m_arg = p_page->m_aiSlot;
}

// Installs GetSlotGoalName as the suffix of the control's choices.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x10066314
void InstallGoalSuffix(MenuPage* p_page, MenuControl* p_control)
{
	MenuChoicesSuffixFn old;
	MenuChoices* choices;

	if (!p_page) {
		return;
	}

	if (!p_control) {
		return;
	}

	choices = p_control->m_data;
	if (!choices) {
		return;
	}

	old = choices->m_suffix;
	choices->m_suffix = GetSlotGoalName;
}

// The orders' actions: each records the order for the menu and gives it to the slot's player
// (OrderStarSlot).
// FUNCTION: MW2 0x10066369
void OrderAttack(MechS32 p_index, MechS32 p_value)
{
	if (p_index < 8) {
		g_lanceOrders[p_index] = 2;
		OrderStarSlot(p_index, 2);
	}

	RequestMenuClose(1);
}

// FUNCTION: MW2 0x100663a4
void OrderEngageAtWill(MechS32 p_index, MechS32 p_value)
{
	if (p_index < 8) {
		g_lanceOrders[p_index] = 1;
		OrderStarSlot(p_index, 3);
	}

	RequestMenuClose(1);
}

// FUNCTION: MW2 0x100663df
void OrderJoinFormation(MechS32 p_index, MechS32 p_value)
{
	if (p_index < 8) {
		g_lanceOrders[p_index] = 3;
		OrderStarSlot(p_index, 5);
	}

	RequestMenuClose(1);
}

// FUNCTION: MW2 0x1006641a
void OrderDefend(MechS32 p_index, MechS32 p_value)
{
	if (p_index < 8) {
		g_lanceOrders[p_index] = 4;
		OrderStarSlot(p_index, 7);
	}

	RequestMenuClose(1);
}

// FUNCTION: MW2 0x10066455
void OrderDisengage(MechS32 p_index, MechS32 p_value)
{
	if (p_index < 8) {
		g_lanceOrders[p_index] = 5;
		OrderStarSlot(p_index, 8);
	}

	RequestMenuClose(1);
}

// FUNCTION: MW2 0x10066490
void OrderShutdown(MechS32 p_index, MechS32 p_value)
{
	if (p_index < 8) {
		g_lanceOrders[p_index] = 6;
		OrderStarSlot(p_index, 0xb);
	}

	RequestMenuClose(1);
}

// Returns the name of the goal of the control's AI player: a nav, a player or a game thing.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x100664cb
MechChar* GetSlotGoalName(
	MenuDefinition* p_menu,
	MenuControl* p_control,
	MechS32 p_index,
	Point p_pos,
	MenuPage* p_page
)
{
	MechS16 target;
	Player* player;
	MechS32 index;
	MechS16 goal;

	index = FindStarSlotPlayer(p_control->m_arg);
	if (index >= g_playerCount) {
		return NULL;
	}

	player = g_players[index];
	if (player->m_ai.m_state == 5) {
		return NULL;
	}

	goal = player->m_ai.m_goal;
	target = goal & 0xff;
	switch (goal & 0xf00) {
	case 0x100:
		return g_navTable[target].m_name;
		break;
	case 0x400:
		return g_gameThings[target].m_name;
		break;
	case 0x200:
		return g_players[target]->m_name;
		break;
	default:
		return NULL;
		break;
	}
}
