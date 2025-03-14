/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file league_cmd.cpp Handling of league tables. */

#include "stdafx.h"
#include "league_cmd.h"
#include "league_base.h"
#include "command_type.h"
#include "command_func.h"
#include "industry.h"
#include "story_base.h"
#include "town.h"
#include "window_func.h"
#include "core/pool_func.hpp"

#include "safeguards.h"

LeagueTableElementPool _league_table_element_pool("LeagueTableElement");
INSTANTIATE_POOL_METHODS(LeagueTableElement)

LeagueTablePool _league_table_pool("LeagueTable");
INSTANTIATE_POOL_METHODS(LeagueTable)

/**
 * Checks whether a link is valid, i.e. has a valid target.
 * @param link the link to check
 * @return true iff the link is valid
 */
bool IsValidLink(Link link)
{
	switch (link.type) {
		case LT_NONE: return (link.target == 0);
		case LT_TILE: return IsValidTile(link.target);
		case LT_INDUSTRY: return Industry::IsValidID(link.target);
		case LT_TOWN: return Town::IsValidID(link.target);
		case LT_COMPANY: return Company::IsValidID(link.target);
		case LT_STORY_PAGE: return StoryPage::IsValidID(link.target);
		default: return false;
	}
	return false;
}

/**
 * Create a new league table.
 * @param flags type of operation
 * @param title Title of the league table
 * @param header Text to show above the table
 * @param footer Text to show below the table
 * @return the cost of this operation or an error
 */
std::tuple<CommandCost, LeagueTableID> CmdCreateLeagueTable(DoCommandFlags flags, const EncodedString &title, const EncodedString &header, const EncodedString &footer)
{
	if (_current_company != OWNER_DEITY) return { CMD_ERROR, LeagueTableID::Invalid() };
	if (!LeagueTable::CanAllocateItem()) return { CMD_ERROR, LeagueTableID::Invalid() };
	if (title.empty()) return { CMD_ERROR, LeagueTableID::Invalid() };

	if (flags.Test(DoCommandFlag::Execute)) {
		LeagueTable *lt = new LeagueTable(title, header, footer);
		return { CommandCost(), lt->index };
	}

	return { CommandCost(), LeagueTableID::Invalid() };
}


/**
 * Create a new element in a league table.
 * @param flags type of operation
 * @param table Id of the league table this element belongs to
 * @param rating Value that elements are ordered by
 * @param company Company to show the color blob for or CompanyID::Invalid()
 * @param text Text of the element
 * @param score String representation of the score associated with the element
 * @param link_type Type of the referenced object
 * @param link_target Id of the referenced object
 * @return the cost of this operation or an error
 */
std::tuple<CommandCost, LeagueTableElementID> CmdCreateLeagueTableElement(DoCommandFlags flags, LeagueTableID table, int64_t rating, CompanyID company, const EncodedString &text, const EncodedString &score, LinkType link_type, LinkTargetID link_target)
{
	if (_current_company != OWNER_DEITY) return { CMD_ERROR, LeagueTableElementID::Invalid() };
	if (!LeagueTableElement::CanAllocateItem()) return { CMD_ERROR, LeagueTableElementID::Invalid() };
	Link link{link_type, link_target};
	if (!IsValidLink(link)) return { CMD_ERROR, LeagueTableElementID::Invalid() };
	if (company != CompanyID::Invalid() && !Company::IsValidID(company)) return { CMD_ERROR, LeagueTableElementID::Invalid() };

	if (flags.Test(DoCommandFlag::Execute)) {
		LeagueTableElement *lte = new LeagueTableElement(table, rating, company, text, score, link);
		InvalidateWindowData(WC_COMPANY_LEAGUE, table);
		return { CommandCost(), lte->index };
	}
	return { CommandCost(), LeagueTableElementID::Invalid() };
}

/**
 * Update the attributes of a league table element.
 * @param flags type of operation
 * @param element Id of the element to update
 * @param company Company to show the color blob for or CompanyID::Invalid()
 * @param text Text of the element
 * @param link_type Type of the referenced object
 * @param link_target Id of the referenced object
 * @return the cost of this operation or an error
 */
CommandCost CmdUpdateLeagueTableElementData(DoCommandFlags flags, LeagueTableElementID element, CompanyID company, const EncodedString &text, LinkType link_type, LinkTargetID link_target)
{
	if (_current_company != OWNER_DEITY) return CMD_ERROR;
	auto lte = LeagueTableElement::GetIfValid(element);
	if (lte == nullptr) return CMD_ERROR;
	if (company != CompanyID::Invalid() && !Company::IsValidID(company)) return CMD_ERROR;
	Link link{link_type, link_target};
	if (!IsValidLink(link)) return CMD_ERROR;

	if (flags.Test(DoCommandFlag::Execute)) {
		lte->company = company;
		lte->text = text;
		lte->link = link;
		InvalidateWindowData(WC_COMPANY_LEAGUE, lte->table);
	}
	return CommandCost();
}

/**
 * Update the score of a league table element.
 * @param flags type of operation
 * @param element Id of the element to update
 * @param rating Value that elements are ordered by
 * @param score String representation of the score associated with the element
 * @return the cost of this operation or an error
 */
CommandCost CmdUpdateLeagueTableElementScore(DoCommandFlags flags, LeagueTableElementID element, int64_t rating, const EncodedString &score)
{
	if (_current_company != OWNER_DEITY) return CMD_ERROR;
	auto lte = LeagueTableElement::GetIfValid(element);
	if (lte == nullptr) return CMD_ERROR;

	if (flags.Test(DoCommandFlag::Execute)) {
		lte->rating = rating;
		lte->score = score;
		InvalidateWindowData(WC_COMPANY_LEAGUE, lte->table);
	}
	return CommandCost();
}

/**
 * Remove a league table element.
 * @param flags type of operation
 * @param element Id of the element to update
 * @return the cost of this operation or an error
 */
CommandCost CmdRemoveLeagueTableElement(DoCommandFlags flags, LeagueTableElementID element)
{
	if (_current_company != OWNER_DEITY) return CMD_ERROR;
	auto lte = LeagueTableElement::GetIfValid(element);
	if (lte == nullptr) return CMD_ERROR;

	if (flags.Test(DoCommandFlag::Execute)) {
		auto table = lte->table;
		delete lte;
		InvalidateWindowData(WC_COMPANY_LEAGUE, table);
	}
	return CommandCost();
}
