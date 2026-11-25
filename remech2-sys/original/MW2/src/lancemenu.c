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
int32_t g_lanceOrders[8] = {7, 7, 7, 7, 7, 0, 0, 0};

// Fills the page with an item per AI slot (from template 5) and a last one (template 6), marking
// the slots whose player is in AI state 12. With no slot, or every slot marked, the page has only
// templates 7 and 6.
int32_t PrepareCommandComputerPage(MenuDefinition* p_menu, MenuPage* p_page)
{
	int32_t marked;
	int32_t slot;
	int32_t count;
	int32_t i;
	int32_t index;
	int32_t src;

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
int32_t PrepareCommandPointPage(MenuDefinition* p_menu, MenuPage* p_page)
{
	int32_t count;
	int32_t slot;
	int32_t index;
	int32_t src;
	uint32_t unk0x08;
	int32_t valid;

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

int32_t GetLanceOrder(int32_t p_index)
{
	int32_t state;

	state = 7;
	if (p_index < 8) {
		state = g_lanceOrders[p_index];
	}

	return state;
}

int32_t GetFormation(int32_t p_arg)
{
	return GetTeamFormation(g_localStar);
}

void SelectFormation(int32_t p_formation, int32_t p_value)
{
	g_lanceOrders[0] = 0;
	SetTeamFormation(g_localStar, p_formation);
	RequestMenuClose(1);
}

// Returns the AI state of the player in AI slot p_index, plus one (0: none).
int32_t GetSlotAiState(int32_t p_index)
{
	Player* player;
	PlayerAi* ai;
	int32_t index;
	int32_t state;

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
void InstallGoalSuffix(MenuPage* p_page, MenuControl* p_control)
{
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

	choices->m_suffix = GetSlotGoalName;
}

// The orders' actions: each records the order for the menu and gives it to the slot's player
// (OrderStarSlot).
void OrderAttack(int32_t p_index, int32_t p_value)
{
	if (p_index < 8) {
		g_lanceOrders[p_index] = 2;
		OrderStarSlot(p_index, 2);
	}

	RequestMenuClose(1);
}

void OrderEngageAtWill(int32_t p_index, int32_t p_value)
{
	if (p_index < 8) {
		g_lanceOrders[p_index] = 1;
		OrderStarSlot(p_index, 3);
	}

	RequestMenuClose(1);
}

void OrderJoinFormation(int32_t p_index, int32_t p_value)
{
	if (p_index < 8) {
		g_lanceOrders[p_index] = 3;
		OrderStarSlot(p_index, 5);
	}

	RequestMenuClose(1);
}

void OrderDefend(int32_t p_index, int32_t p_value)
{
	if (p_index < 8) {
		g_lanceOrders[p_index] = 4;
		OrderStarSlot(p_index, 7);
	}

	RequestMenuClose(1);
}

void OrderDisengage(int32_t p_index, int32_t p_value)
{
	if (p_index < 8) {
		g_lanceOrders[p_index] = 5;
		OrderStarSlot(p_index, 8);
	}

	RequestMenuClose(1);
}

void OrderShutdown(int32_t p_index, int32_t p_value)
{
	if (p_index < 8) {
		g_lanceOrders[p_index] = 6;
		OrderStarSlot(p_index, 0xb);
	}

	RequestMenuClose(1);
}

// Returns the name of the goal of the control's AI player: a nav, a player or a game thing.
char* GetSlotGoalName(MenuDefinition* p_menu, MenuControl* p_control, int32_t p_index, Point p_pos, MenuPage* p_page)
{
	int16_t target;
	Player* player;
	int32_t index;
	int16_t goal;

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
