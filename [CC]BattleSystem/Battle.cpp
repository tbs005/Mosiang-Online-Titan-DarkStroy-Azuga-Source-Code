// Battle.cpp: implementation of the CBattle class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Battle.h"

#include "Player.h"
#include "BattleTeam.h"
#include "BattleObject.h"
#include "WantedManager.h"
#include "PartyManager.h"
#include "Party.h"
#include "SurvivalModeManager.h"

#include "PKManager.h"

#ifdef _MAPSERVER_
#include "Monster.h"
#include "ItemDrop.h"
#include "ItemManager.h"
#include "SkillManager_Server.h"
#include "ChannelSystem.h"
#include "..\[CC]Quest\QuestEvent.h"
#include "QuestManager.h"
#include "EventMapMgr.h"
#include "GuildFieldWarMgr.h"
#include "PartyWarMgr.h"
#include "GuildManager.h"
#include "GuildUnionManager.h"
#else
#include "ObjectManager.h"
#include "ChatManager.h"
#include "EventMapInfo.h"
#include "GuildFieldWar.h"
#include "PartyWar.h"
#include "ItemManager.h"
#include "MHMap.h"
#include "GuildManager.h"
#include "GuildUnion.h"
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBattle::CBattle()
{
	m_BattleFlag = BATTLE_FLAG_NONE;
	m_bDestroyFlag = FALSE;
	
	ZeroMemory( m_TeamArray, sizeof(m_TeamArray) );
}

CBattle::~CBattle()
{
}
void CBattle::Initialize(BATTLE_INFO_BASE* pCreateInfo, CBattleTeam* pTeam1, CBattleTeam* pTeam2)
{	
	OnCreate(pCreateInfo,pTeam1,pTeam2);
	
	m_BattleInfo = *pCreateInfo;
	m_Team1 = pTeam1;
	m_Team2 = pTeam2;
	m_TeamObserver = NULL;

	m_bDestroyFlag = FALSE;

	m_WinnerTeam = eBattleTeam_Max;
	m_BattleFlag = BATTLE_FLAG_NONE;
	
}

void CBattle::StartBattle()
{
	OnFightStart();

#ifdef _MAPSERVER_

	MSG_DWORD msg;
	msg.Category = MP_BATTLE;
	msg.Protocol = MP_BATTLE_START_NOTIFY;
	msg.dwData = GetBattleID();

	for(int n=0;n<eBattleTeam_Max;++n)
	{
		if( m_TeamArray[n] )
			m_TeamArray[n]->SendTeamMsg(&msg,sizeof(msg));
	}

#endif
}


BOOL CBattle::IsEnemy(CObject* pOperator,CObject* pTarget)
{
	if(pOperator->GetObjectKind() & eObjectKind_Monster)
	{	// Operator �� �����ϰ��
		if(pTarget->GetObjectKind() & eObjectKind_Monster)	// ���ʹ� ���� �ƴ�
			return FALSE;
#ifdef _MAPSERVER_
		if( pTarget->GetState() == eObjectState_Immortal )	//�������´� ���� �ƴϴ�.
			return FALSE;
#endif
		
	}
	else if(pOperator->GetObjectKind() == eObjectKind_Player)
	{	// Operator �� �÷��̾��ϰ��
		if( pTarget->GetObjectKind() & eObjectKind_Monster )	// ���ʹ� ���̴�
		{
#ifdef _JAPAN_LOCAL_
 			WORD wMonsterKind = ((CMonster*)pTarget)->GetMonsterKind();
			if( wMonsterKind == 166 || wMonsterKind == 167 ||
 				wMonsterKind == 168 || wMonsterKind == 169 )
 				return FALSE;		//��/����/���� ���� �ƴϴ�.
#endif

			/*
#ifdef _MAPSERVER_
			if( ((CPlayer*)pOperator)->InTitan() && g_pServerSystem->GetMapNum() < TITANMAP_START_INDEX )
#elif  defined _TESTCLIENT_
			return TRUE;
#else
			if( ((CPlayer*)pOperator)->InTitan() && !MAP->IsTitanMap() )
#endif
			
			{
				return FALSE;
			}
			*/
			return TRUE;
		}
		else if( pTarget->GetObjectKind() == eObjectKind_Player )
		{
			// 2005 ũ�������� �̺�Ʈ �ڵ�
			// �̺�Ʈ ���� ����ÿ��� ��� ���� �����ϴ�
			if( ((CPlayer*)pOperator)->GetWeaponEquipType() == WP_EVENT )
				return TRUE;
			else if( ((CPlayer*)pOperator)->GetWeaponEquipType() == WP_EVENT_HAMMER )
				return TRUE;

			//SW061019 �����̹����
#ifdef _MAPSERVER_
			//if( g_pServerSystem->CheckMapKindIs(eSurvival) )
			if( g_pServerSystem->GetMap()->IsMapKind(eSurvivalMap) )
#else
			//if( MAP->CheckMapKindIs(eSurvival) )
			if( MAP->IsMapKind(eSurvivalMap) )
#endif
			{
				if( pOperator->GetID() == pTarget->GetID() )
					return FALSE;

				if( SVVMODEMGR->GetCurModeState() == eSVVMode_Fight )
					return TRUE;
				else
					return FALSE;
			}

			
#ifdef _MAPSERVER_
			DWORD dwChannel = ((CPlayer*)pOperator)->GetChannelID();
			//if( g_pServerSystem->GetMapNum() == RUNNINGMAP )
			if( g_pServerSystem->GetMap()->IsMapSame(eRunningMap) )
				return FALSE;

			if( pTarget->GetState() == eObjectState_Immortal )	//�������´� ���� �ƴϴ�.
				return FALSE;

			if( ((CPlayer*)pOperator)->GetPartyIdx() != 0 )
			{
				CParty* pParty = PARTYMGR->GetParty( ((CPlayer*)pOperator)->GetPartyIdx() );
				if( pParty )
				if( pParty->IsPartyMember( pTarget->GetID() ) )
					return FALSE;	//��Ƽ���� ���� �ƴϴ�.

				// partywar
				if( PARTYWARMGR->IsEnemy( (CPlayer*)pOperator, (CPlayer*)pTarget ) )
					return TRUE;
			}
			// Pk�� ���ǰų� �������ֹ����� ������̸�
			if( PKMGR->IsPKAllow(dwChannel) || ((CPlayer*)pOperator)->GetShopItemManager()->GetUsingItemInfo( eIncantation_Tracking_Jin ) )
			if( WANTEDMGR->IsOwner((CPlayer*)pOperator, ((CPlayer*)pTarget)->GetWantedIdx()) == TRUE )
				return TRUE;	//ô�� ����ڴ� ���̴�.

			if( EVENTMAPMGR->IsEnemy( pOperator, pTarget, this->GetBattleID() ) )
				return TRUE;
			
			if( GUILDFIELDWARMGR->IsEnemyInField( (CPlayer*)pOperator, (CPlayer*)pTarget ) == TRUE )
				return TRUE;

			//SW080515 ä�κ� ����	//��Ʋä�ο����� ���ݰ���.
			if( CHANNELSYSTEM->IsBattleChannel( ((CPlayer*)pOperator)->GetChannelID() ) )
			{
				//���Ŀ� �ƴϰ�	// ������ üũ pParty->IsPartyMember(..
				//���Ŀ� �ƴϰ�
				if( GUILDMGR->IsSameGuild( (CPlayer*)pOperator, (CPlayer*)pTarget) )
					return FALSE;
				//���Ϳ� �ƴϸ�
				if( GUILDUNIONMGR->IsSameUnion( (CPlayer*)pOperator, (CPlayer*)pTarget ) )
					return FALSE;

				return TRUE;	// �� �ܿ� ��� ���� ����.
			}
#else
			//if( MAP->GetMapNum() == RUNNINGMAP )
			if( MAP->IsMapSame(eRunningMap) )
				return FALSE;

			if( pOperator == HERO )
			{
				if( PARTYMGR->IsPartyMember( pTarget->GetID() ) )
				{
					return FALSE;	//��Ƽ���� ���� �ƴϴ�.
				}
				if( PARTYWAR->IsEnemy( (CPlayer*)pTarget ) )
					return TRUE;

                // �������ֹ����� ������̸�
				if( PKMGR->IsPKAllow() || ITEMMGR->GetUsedItemInfo( eIncantation_Tracking_Jin ) )
				if( WANTEDMGR->IsChrIDInList( pTarget->GetID() ) )
					return TRUE;	//ô�� ����ڴ� ���̴�.

				if( EVENTMAP->IsEnemy( pTarget ) )
					return TRUE;

				if( GUILDFIELDWAR->IsEnemy( (CPlayer*)pTarget ) == TRUE )
					return TRUE;
			}
			//SW080515 ä�κ� ����	//��Ʋä�ο����� ���ݰ���.
			if( IsBattleChannel[gChannelNum] )
			{
				//���Ŀ� �ƴϰ�	// ������ üũ PARTYMGR->IsPartyMember(..
				//���Ŀ� �ƴϰ�
				if( GUILDMGR->IsSameGuild( (CPlayer*)pOperator, (CPlayer*)pTarget) )
					return FALSE;
				//���Ϳ� �ƴϸ�
				if( GUILDUNION->IsSameUnion( (CPlayer*)pOperator, (CPlayer*)pTarget ) )
					return FALSE;

				return TRUE;	// �� �ܿ� ��� ���� ����.
			}

#endif

#ifdef _MAPSERVER_
			if( PKMGR->IsPKAllow(dwChannel) )
#else
			if( PKMGR->IsPKAllow() )
#endif
			{
				if( ((CPlayer*)pOperator)->IsPKMode() )				
				{					
					if(pOperator == pTarget )
						return FALSE;
					else
						return TRUE;	//�ڽ��� PK����̸� �ڽ��� �� �������� ���̴�.
				}
				if( ((CPlayer*)pTarget)->IsPKMode() )
					return TRUE;		//��밡 PK����̸� ���̴�.
			}

			return FALSE;
		}
		else if(pTarget->GetObjectKind() == eObjectKind_SkillObject)
		{
			// YH2DO ������ ���� �ȶ����� �Ǵµ�... -_-a;
			CSkillObject* pSObj = (CSkillObject*)pTarget;
			return TRUE;
		}
		else	//�������� ���� �ƴϴ�.
		{
			return FALSE;
		}
	}

	return TRUE;						// ������ ��� ��
}
BOOL CBattle::IsFriend(CObject* pOperator,CObject* pTarget)
{
	if(pOperator->GetObjectKind() & eObjectKind_Monster)
	{	// Operator �� �����ϰ��
		if((pTarget->GetObjectKind() & eObjectKind_Monster) == FALSE)		// ���Ͱ� �ƴϸ� teammate�� �ƴϴ�.
			return FALSE;
	}
	else if(pOperator->GetObjectKind() == eObjectKind_Player)
	{	// Operator �� �÷��̾��ϰ��
		DWORD PartyIdx = 0;
#ifdef _MAPSERVER_
		PartyIdx = ((CPlayer*)pOperator)->GetPartyIdx(); // ��Ƽ�� ������ ���� ����
		if(PartyIdx == 0)
			return FALSE;
		if(((CPlayer*)pTarget)->GetPartyIdx() != PartyIdx)		// ��Ƽ�� �ٸ��� FALSE
			return FALSE;
#endif
		if(pTarget->GetObjectKind() != eObjectKind_Player)		// �÷��̾ �ƴϸ� FALSE
			return FALSE;
	}

	return TRUE;						// ������ ������Ʈ
}

DWORD CBattle::GetBattleTeamID( CObject* pObject )
{
	for(int n=0; n<eBattleTeam_Max; ++n)
	{
		if( m_TeamArray[n] )
		if( m_TeamArray[n]->IsTeamMember( pObject ) )
			return n+1;
	}

	return 0;
}

#ifdef _MAPSERVER_
BOOL CBattle::AddObjectToBattle(CObject* pObject)
{
	for(int n=0;n<eBattleTeam_Max;++n)
	{
		if( m_TeamArray[n] )
		if( m_TeamArray[n]->IsAddableTeamMember(pObject))
		{
			pObject->SetBattle(GetBattleID(),n);

			BOOL rt = m_TeamArray[n]->AddTeamMember(pObject);
			ASSERT(rt);
			
			MSG_BATTLE_TEAMMEMBER_ADDDELETE msg;
			SetProtocol(&msg,MP_BATTLE,MP_BATTLE_TEAMMEMBER_ADD_NOTIFY);
			msg.Member.set(pObject->GetID(),pObject->GetObjectName());
			msg.Team = n;
			ToEachTeam(pTeam)
				pTeam->SendTeamMsg(&msg,sizeof(msg));
			EndToEachTeam

			OnTeamMemberAdd(n,pObject->GetID(),pObject->GetObjectName());

			if(pObject->GetObjectKind() == eObjectKind_Player)
				SendBattleInfo((CPlayer*)pObject);
			
			return TRUE;
		}
	}

	return FALSE;
}

BOOL CBattle::IsAddableBattle(CObject* pObject)
{
	for(int n=0;n<eBattleTeam_Max;++n)
	{
		if(m_TeamArray[n])
		if(m_TeamArray[n]->IsAddableTeamMember(pObject))
		{
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CBattle::DeleteObjectFromBattle(CObject* pObject)
{
	ToEachTeam(pTeam)
		if(pTeam->DeleteTeamMember(pObject) == TRUE)
		{
			pObject->SetBattle(pObject->GetGridID(),0);
			
			if(OnTeamMemberDelete(TeamNumber,pObject->GetID(),pObject->GetObjectName()) == TRUE)
			{				
				MSG_BATTLE_TEAMMEMBER_ADDDELETE msg;
				SetProtocol(&msg,MP_BATTLE,MP_BATTLE_TEAMMEMBER_DELETE_NOTIFY);
				msg.Member.set(pObject->GetID(),pObject->GetObjectName());
				msg.Team = TeamNumber;
				ToEachTeam(pTeam)
					pTeam->SendTeamMsg(&msg,sizeof(msg));
				EndToEachTeam
			}			
			
			return TRUE;
		}
	EndToEachTeam

	return FALSE;
}

void CBattle::SendBattleInfo(CPlayer* pPlayer)
{
	static MSG_BATTLE_INFO msg;
	SetProtocol(&msg,MP_BATTLE,MP_BATTLE_INFO);
	GetBattleInfo(msg.info,&msg.msglen);	
	pPlayer->SendMsg(&msg,msg.GetMsgLength());
}


void CBattle::GetBattleInfo(char* pInfo,WORD* size)
{
	memcpy(pInfo,&m_BattleInfo,sizeof(BATTLE_INFO_BASE));
	*size = sizeof(BATTLE_INFO_BASE);
}

DWORD CBattle::GetTeamMemberNum(int i)
{
	if( m_TeamArray[i] ) 
		return m_TeamArray[i]->GetMemberNum();

	return 0;
}
#endif




// event func
void CBattle::OnCreate(BATTLE_INFO_BASE* pCreateInfo, CBattleTeam* pTeam1, CBattleTeam* pTeam2)
{
#ifdef _MAPSERVER_
	pCreateInfo->BattleTime = gCurTime;
#else
//	pCreateInfo->BattleTime = gCurTime - pCreateInfo->BattleTime;
	pCreateInfo->BattleTime = gCurTime;
#endif

	pCreateInfo->BattleState = eBATTLE_STATE_READY;
}
void CBattle::OnFightStart()
{
	m_BattleInfo.BattleTime = gCurTime;
	m_BattleInfo.BattleState = eBATTLE_STATE_FIGHT;
}
void CBattle::OnDestroy()
{
#ifdef _MAPSERVER_
	MSGBASE msg;
	SetProtocol(&msg,MP_BATTLE,MP_BATTLE_DESTROY_NOTIFY);
	ToEachTeam(pTeam)
		pTeam->SendTeamMsg(&msg,sizeof(msg));
		YHTPOSITION pos = pTeam->GetPositionHead();
		while(CObject* pObject = pTeam->GetNextTeamMember(&pos))
		{
			pTeam->DeleteTeamMember(pObject);
		}
	EndToEachTeam
#endif
}
void CBattle::OnTeamMemberAdd(int Team,DWORD MemberID,char* Name)
{
}
BOOL CBattle::OnTeamMemberDelete(int Team,DWORD MemberID,char* Name)
{
	// return TRUE�� �ϸ� MP_BATTLE_TEAMMEMBER_DELETE_NOTIFY �޼����� �Ѹ��� �ȴ�.
	return FALSE;
}
BOOL CBattle::OnTeamMemberDie(int Team,DWORD VictimMemberID,DWORD KillerID)
{
	return FALSE;
}
void CBattle::OnTeamObjectCreate(int Team,CBattleObject* pBattleObject)
{
}
void CBattle::OnTeamObjectDestroy(int Team,CBattleObject* pBattleObject)
{
}
void CBattle::OnTeamMemberRevive(int Team,CObject* pReviver)
{
}

void CBattle::OnMonsterDistribute(CMonster* pMonster,CObject* pKiller)
{
#ifdef _MAPSERVER_
	pMonster->Drop();
	
	if(pKiller->GetObjectKind() == eObjectKind_Player)
	{
		// jsd quest
		// ���⼭ monsterkill event�߻�	
		CQuestEvent qe;
		qe.SetValue( eQuestEvent_Hunt, pMonster->m_MonsterInfo.MonsterKind, 0 );
		QUESTMGR->AddQuestEvent( (CPlayer*)pKiller, &qe );
		//
		((CPlayer*)pKiller)->SetKillMonsterLevel( pMonster->GetLevel() );
	}
#endif
}

void CBattle::OnTick()
{
	ySWITCH(m_BattleInfo.BattleState)
		yCASE(eBATTLE_STATE_READY)

		yCASE(eBATTLE_STATE_FIGHT)

		yCASE(eBATTLE_STATE_RESULT)
	
	yENDSWITCH
}

// ���� ����
BOOL CBattle::Judge()
{
	return FALSE;	
}
void CBattle::Victory(int WinnerTeamNum,int LoserTeamNum)
{
#ifdef _MAPSERVER_
	MSG_DWORD2 msg;
	msg.Category = MP_BATTLE;
	msg.Protocol = MP_BATTLE_VICTORY_NOTIFY;
	msg.dwData1 = WinnerTeamNum;
	msg.dwData2 = LoserTeamNum;
	
	ToEachTeam(pTeam)
		pTeam->SendTeamMsg(&msg,sizeof(msg));
	EndToEachTeam

#endif

	m_WinnerTeam = WinnerTeamNum;
	m_BattleInfo.BattleState = eBATTLE_STATE_RESULT;
	m_BattleInfo.BattleTime = gCurTime;
}
void CBattle::Draw()
{
#ifdef _MAPSERVER_
	MSGBASE msg;
	msg.Category = MP_BATTLE;
	msg.Protocol = MP_BATTLE_DRAW_NOTIFY;
	
	ToEachTeam(pTeam)
		pTeam->SendTeamMsg(&msg,sizeof(msg));
	EndToEachTeam
#endif
		
	m_WinnerTeam = eBattleTeam_Max;
	m_BattleInfo.BattleState = eBATTLE_STATE_RESULT;
	m_BattleInfo.BattleTime = gCurTime;
}


// Render
#ifdef _MHCLIENT_

#include "interface/cFont.h"
void CBattle::Render()
{
//	RECT rect;
//	SetRect(&rect,30,100,500,150);
//	char temp[256];
//	sprintf(temp,"Default Battle");
//	CFONT_OBJ->RenderFont(0,temp,strlen(temp),&rect,0xffffffff);
}

#endif