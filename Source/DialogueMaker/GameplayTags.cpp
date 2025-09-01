#include "GameplayTags.h"

namespace GameplayTags
{
	// Chapter 기준
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Chapter_01_Complete, "Chapter.01.Complete", "Player clear scenario chapter 01. Ongoing chapter 02.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Chapter_02_Complete, "Chapter.02.Complete", "Player clear scenario chapter 01. Ongoing chapter 02.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Chapter_03_Complete, "Chapter.03.Complete", "Player clear scenario chapter 01. Ongoing chapter 02.");

	// Quest 진행 기준
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Quest_Chapter_01_VisitHome_Active, "Quest.Chapter.01.VisitHome.Active", "Player clear scenario chapter 01. Ongoing chapter 02.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Quest_Chapter_01_VisitHome_Complete, "Quest.Chapter.01.VisitHome.Complete", "Player clear scenario chapter 01. Ongoing chapter 02.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Quest_Chapter_02_VisitHome_Active, "Quest.Chapter.02.VisitHome.Active", "Player clear scenario chapter 01. Ongoing chapter 02.");

	// Player Tier 기준
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player_Tier_F, "Player.Tier.F", "Player clear scenario chapter 01. Ongoing chapter 02.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player_Tier_E, "Player.Tier.E", "Player clear scenario chapter 01. Ongoing chapter 02.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player_Tier_D, "Player.Tier.D", "Player clear scenario chapter 01. Ongoing chapter 02.");

	// Item 소지 여부 기준
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Weapon_LegendarySword, "Item.Weapon.LegendarySword", "Player clear scenario chapter 01. Ongoing chapter 02.");


	// TagContainer to Registry용 Tag
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Required_All_Tags, "RequiredAllTags", "Tag for Container to Registry Prefix. Match All Tags");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Required_Any_Tags, "RequiredAnyTags", "Tag for Container to Registry Prefix. Match Any Tags");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Blocked_Any_Tags, "BlockedAnyTags", "Tag for Container to Registry Prefix. Match Any Tags");

}
