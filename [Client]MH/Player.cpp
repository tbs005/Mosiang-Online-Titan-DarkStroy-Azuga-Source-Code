// Player.cpp: implementation of the CPlayer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Player.h"
#include "GameResourceManager.h"
#include "MoveManager.h "
#include "./interface/cWindowManager.h"
#include "AppearanceManager.h"
#include "PartyManager.h"
#include "ItemManager.h"
#include "ObjectManager.h"
#include "KyungGongManager.h"
#include "StreetStallTitleTip.h"
#include "StreetStallManager.h"
#include "GameIn.h"
#include "PeaceWarModeManager.h"
#include "ObjectBalloon.h"
#include "MonsterGuageDlg.h"
#include "AbilityManager.h"

#include "MainGame.h"
#include "WindowIdEnum.h"
#include "MapChange.h"
#include "cMsgBox.h"

#include "ObjectStateManager.h"
#include "PKManager.h"
#include "ChatManager.h"
#include "GuildFieldWar.h"

#include "OptionManager.h"

#include "Pet.h"
#include "PetBase.h"
#include "UngijosikManager.h"

#include "CharacterDialog.h"
#include "WeatherManager.h"

#include "Titan.h"
#include "TitanManager.h"

#include "cScriptManager.h"

//#include "QuestGroup.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPlayer::CPlayer()
{
//	m_pQuestGroup = NULL;
	m_ItemUseEffect = -1;
	m_DecorationAddress = NULL;	// magi82(44) - ������ ������Ʈ
	InitSkinDelayTime();
}

CPlayer::~CPlayer()
{
//	if( m_pQuestGroup )
//		delete m_pQuestGroup;
}

void CPlayer::InitPlayer(CHARACTER_TOTALINFO* pTotalInfo)
{
	memcpy(&m_CharacterInfo,pTotalInfo,sizeof(CHARACTER_TOTALINFO));
	memset(&m_MoveInfo,0,sizeof(MOVE_INFO));
	memset(&m_ShopItemOption, 0, sizeof(SHOPITEMOPTION));
	memset(&m_AvatarOption, 0, sizeof(AVATARITEMOPTION));	

	//////////////////////////////////////////////////////////////////////////
	// 06. 06. 2�� ���� - �̿���
	// ���� ��ȯ �߰�
	memset(&m_SkillStatsOption, 0, sizeof(SKILLSTATSOPTION));
	//////////////////////////////////////////////////////////////////////////

	for(int i=eAvatar_Weared_Hair; i<eAvatar_Max; i++)	
		m_ShopItemOption.Avatar[i] = 1;
	
	m_pPet = NULL;
	m_pTitan = NULL;
	m_bInTitan = FALSE;
	m_TitanMoveSpeed = 0;
	m_bTitanPreView = FALSE;

	SetBaseMotion();

	m_dwSetItemEffectID = 0;

	memset(&m_TitanAppearanceInfo,0,sizeof(TITAN_APPEARANCEINFO));
	memset(&m_TitanPreViewAppInfo,0,sizeof(TITAN_APPEARANCEINFO));

	OBJECTMGR->RemoveDecoration(this);	// magi82(44) - ������ ������Ʈ
}

/*void CPlayer::AddChat(char * chatMsg)
{

	m_pObjectName->GetObjectChatTip()->SetText(chatMsg);
	m_pObjectName->GetObjectChatTip()->SetDisappearTime(5000);
	if(!m_pObjectName->IsActive())
	{
		DropObjectName();
		m_pObjectName->GetObjectChatTip()->SetActive(TRUE);
		WINDOWMGR->AddChatTooltip(m_pObjectName);
	}	
}*/

void CPlayer::GetCharacterTotalInfo(CHARACTER_TOTALINFO* pRtInfo)
{
	memcpy(pRtInfo,&m_CharacterInfo,sizeof(CHARACTER_TOTALINFO));
}

#define DecorationTime	1500

void CPlayer::Process()
{
	if( GetState() == eObjectState_Die )
	if( GAMEIN->GetMonsterGuageDlg() )
	{
		CObject * pSelectedObj = GAMEIN->GetMonsterGuageDlg()->GetCurMonster();
		if( pSelectedObj == this )
		{
//			OBJECTMGR->SetSelectedObject(NULL);
			GAMEIN->GetMonsterGuageDlg()->SetActive(FALSE);
		}
	}

	CObject::Process();

	// magi82(44) - ������ �ٹ̱� ������Ʈ
	// ������ �ٹ̱� ��ü�� ������ ������Ƿ� �÷��̾��� ���μ������� ���İ��� ������ ���ִٰ� �����ش�.
	CObject* pObject = GetDecorationAddress();
	if(pObject &&  m_dwDecorationStartTime > 0)
	{
		float alp = GetAlphaValue(DecorationTime, m_dwDecorationStartTime, m_bDecorationFlag);
		
		if(pObject)
            pObject->GetEngineObject()->SetAlpha(alp);

		if( gCurTime - m_dwDecorationStartTime > DecorationTime && m_bDecorationFlag == FALSE )	// m_bDecorationFlag �� FALSE �̸� ���ڷ��̼� ����
		{
			SetDecorationInfo(0, FALSE);
			alp = 0.0f;
			OBJECTMGR->RemoveDecoration(this);
		}
	}

	//2008. 1. 21. CBH - ��Ų ������ ó�� �߰�
	if(m_bSkinDelayResult == TRUE)
	{
		DWORD dwCurrentTime = gCurTime;
		if(dwCurrentTime > m_dwSkinDelayTime)
		{
			InitSkinDelayTime();
		}
	}
}


void CPlayer::SetWearedItemIdx(DWORD WearedPosition,WORD ItemIdx)
{
	if(WearedPosition == eWearedItem_Weapon)
	{
		// ���⸦ �ٲٸ� ������ �������.
		GetSpecialStateParam1(eSpecialState_AmplifiedPowerPhy);
	}

	m_CharacterInfo.WearedItemIdx[WearedPosition] = ItemIdx;
	SetBaseMotion();			// ���Ⱑ �ٲ�� �⺻������ �ٲ��
}


WORD CPlayer::GetWeaponEquipType()
{
	WORD WeaponItemIdx = GetWearedItemIdx(eWearedItem_Weapon);
	if(WeaponItemIdx == 0)
		return WP_GWUN;
	ITEM_INFO * pItemInfo = ITEMMGR->GetItemInfo(WeaponItemIdx);
	ASSERT(pItemInfo);
	if(pItemInfo && pItemInfo->WeaponType != 0)
		return pItemInfo->WeaponType;
	else
		return 0;
}

WORD CPlayer::GetTitanWeaponEquipType()
{
	WORD WeaponItemIdx = 0;
	// magi82 - Titan(070912) Ÿ��ź ����������Ʈ
//	if( InTitan() )
		WeaponItemIdx = m_TitanAppearanceInfo.WearedItemIdx[eTitanWearedItem_Weapon];
//	else if( IsTitanPreView() )
//		WeaponItemIdx = m_TitanPreViewAppInfo.WearedItemIdx[eTitanWearedItem_Weapon];

	if(WeaponItemIdx == 0)
		return WP_GWUN;
	ITEM_INFO * pItemInfo = ITEMMGR->GetItemInfo(WeaponItemIdx);
	ASSERT(pItemInfo);
	if(pItemInfo && pItemInfo->WeaponType != 0)
		return pItemInfo->WeaponType;
	else
		return 0;
}

WORD CPlayer::GetTitanWearedItemIdx(DWORD WearedPosition)
{	
	if( InTitan() )
		return m_TitanAppearanceInfo.WearedItemIdx[WearedPosition];
	else if( IsTitanPreView() )
		return m_TitanPreViewAppInfo.WearedItemIdx[WearedPosition];

	return 0;
}

void CPlayer::SetBaseMotion()
{
	WORD WeaponEquipType = 0;
	if( InTitan() )
		WeaponEquipType = GetTitanWeaponEquipType();
	else
		WeaponEquipType = GetWeaponEquipType();
	
	//	2005 ũ�������� �̺�Ʈ �ڵ�
	//////////////////////////////////////////////////////////////////////////
	// �̺�Ʈ���� Ÿ���� ��� �尩! �� ���� ��� ���
	if(WeaponEquipType == WP_EVENT)
		WeaponEquipType = WP_GWUN;
	else if( WeaponEquipType == WP_EVENT_HAMMER )
		WeaponEquipType = WP_GUM;
	//////////////////////////////////////////////////////////////////////////
	
	if(m_MoveInfo.KyungGongIdx)
	{	// ������̸�
		// ��� ������ ���� ������� 1,2,3�� �����Ѵ�.
		CKyungGongInfo* pKGInfo = KYUNGGONGMGR->GetKyungGongInfo(m_MoveInfo.KyungGongIdx);
		ASSERT(pKGInfo);
		int MoveType;
		if(pKGInfo != NULL)
			MoveType = pKGInfo->GetMoveType() - 1;		// -1 �� 1���� �����ϱ� ������ 0���� ������ ����
		else
			MoveType = 0;
		
		// magi82(9) - Titan(071024) Ÿ��ź �϶� �ι�° ����� ���۵����� �����ʴ´�.
		if(MoveType == 1 && this->InTitan() == FALSE)	// �ι�° ����� ���۵��� �� by SSAMI
			m_Move_Start_Motion = eMotion_KyungGong1_Move_NoWeapon + MoveType*3;
		else
			m_Move_Start_Motion = eMotion_KyungGong1_Start_NoWeapon + MoveType*3;
		m_Move_Ing_Motion = eMotion_KyungGong1_Move_NoWeapon + MoveType*3;

		// RaMa - 05.09.05  - ������������ ����
		if( GetAvatarOption()->bKyungGong || GetShopItemStats()->bKyungGong )
			m_Move_End_Motion = m_Move_End_Motion;
		// 2005 ũ�������� �̺�Ʈ �ڵ�
		else if( (WEATHERMGR->GetWeatherState() == eWS_Snow ) &&
				 ((GetShopItemStats()->Avatar[eAvatar_Dress] == EVENT_SHOPITEM_SNOWMAN_DRESS && GetShopItemStats()->Avatar[eAvatar_Hat] == EVENT_SHOPITEM_SNOWMAN_HAT) ||
				  (GetShopItemStats()->Avatar[eAvatar_Dress] ==  EVENT_SHOPITEM_RUDOLP_DRESS && GetShopItemStats()->Avatar[eAvatar_Hat] ==  EVENT_SHOPITEM_RUDOLP_HAT) ||
				  (GetShopItemStats()->Avatar[eAvatar_Dress] == EVENT_SHOPITEM_SNOWMAN_DRESS2 && GetShopItemStats()->Avatar[eAvatar_Hat] == EVENT_SHOPITEM_SNOWMAN_HAT2) ||
				  (GetShopItemStats()->Avatar[eAvatar_Dress] ==  EVENT_SHOPITEM_RUDOLP_DRESS2 && GetShopItemStats()->Avatar[eAvatar_Hat] ==  EVENT_SHOPITEM_RUDOLP_HAT2) || 
				  (GetShopItemStats()->Avatar[eAvatar_Dress] == EVENT_SHOPITEM_SNOWMAN_DRESS && GetShopItemStats()->Avatar[eAvatar_Hat] == EVENT_SHOPITEM_SNOWMAN_HAT2) ||
				  (GetShopItemStats()->Avatar[eAvatar_Dress] ==  EVENT_SHOPITEM_RUDOLP_DRESS && GetShopItemStats()->Avatar[eAvatar_Hat] ==  EVENT_SHOPITEM_RUDOLP_HAT2) ||
				  (GetShopItemStats()->Avatar[eAvatar_Dress] == EVENT_SHOPITEM_SNOWMAN_DRESS2 && GetShopItemStats()->Avatar[eAvatar_Hat] == EVENT_SHOPITEM_SNOWMAN_HAT) ||
				  (GetShopItemStats()->Avatar[eAvatar_Dress] ==  EVENT_SHOPITEM_RUDOLP_DRESS2 && GetShopItemStats()->Avatar[eAvatar_Hat] ==  EVENT_SHOPITEM_RUDOLP_HAT) ||
				  GetShopItemStats()->Avatar[eAvatar_Dress] ==  EVENT_SHOPITEM_SNOWMAN1_HK || GetShopItemStats()->Avatar[eAvatar_Dress] ==  EVENT_SHOPITEM_SNOWMAN2_HK ||
				  GetShopItemStats()->Avatar[eAvatar_Dress] ==  EVENT_SHOPITEM_SNOWMAN3_HK || GetShopItemStats()->Avatar[eAvatar_Dress] ==  EVENT_SHOPITEM_RUDOLP1_HK ||
				  GetShopItemStats()->Avatar[eAvatar_Dress] ==  EVENT_SHOPITEM_RUDOLP2_HK || GetShopItemStats()->Avatar[eAvatar_Dress] ==  EVENT_SHOPITEM_RUDOLP3_HK ))
					m_Move_End_Motion = m_Move_End_Motion;
		else
		{
			#ifdef TAIWAN_LOCAL
				m_Move_End_Motion = m_Move_End_Motion;
			#else
				m_Move_End_Motion = eMotion_KyungGong1_End_NoWeapon;// + MoveType*3;
			#endif
		}
		
		if(m_CharacterInfo.bPeace == TRUE)		// ��ȭ��� �̳�
		{	// ��ȭ���
			if(WeaponEquipType == WP_DO)
				m_StandardMotion = eMotion_Peace_Standard_Do;
			else if(WeaponEquipType == WP_CHANG)
				m_StandardMotion = eMotion_Peace_Standard_Change;
			else
				m_StandardMotion = eMotion_Peace_Standard;
		}
		else
		{
			WORD WeaponEquipType = GetWeaponEquipType();

			//////////////////////////////////////////////////////////////////////////
			// �̺�Ʈ���� Ÿ���� ��� �尩! �� ���� ��� ���
			if(WeaponEquipType == WP_EVENT)
				WeaponEquipType = WP_GWUN;
			else if( WeaponEquipType == WP_EVENT_HAMMER )
				WeaponEquipType = WP_GUM;
			//////////////////////////////////////////////////////////////////////////
			
			m_StandardMotion = eMotion_Battle_Gum_Standard + WeaponEquipType -1;			
		}
		return;
	}

	if(m_CharacterInfo.bPeace == TRUE)		// ��ȭ��� �̳�
	{	// ��ȭ���
		if(WeaponEquipType == WP_DO)
			m_StandardMotion = eMotion_Peace_Standard_Do;
		else if(WeaponEquipType == WP_CHANG)
			m_StandardMotion = eMotion_Peace_Standard_Change;
		else
			m_StandardMotion = eMotion_Peace_Standard;

		if(m_MoveInfo.MoveMode == eMoveMode_Walk)
		{	// �ȱ�
			if(WeaponEquipType == WP_DO)
				m_Move_Start_Motion = eMotion_Peace_Walk_Do;
			else if(WeaponEquipType == WP_CHANG)
				m_Move_Start_Motion = eMotion_Peace_Walk_Change;
			else
				m_Move_Start_Motion = eMotion_Peace_Walk;
			m_Move_Ing_Motion = m_Move_Start_Motion;
			m_Move_End_Motion = -1;
		}
		else
		{	// �ٱ�	
			if(WeaponEquipType == WP_DO)
				m_Move_Start_Motion = eMotion_Peace_Run_Do;
			else if(WeaponEquipType == WP_CHANG)
				m_Move_Start_Motion = eMotion_Peace_Run_Change;
			else
				m_Move_Start_Motion = eMotion_Peace_Run;
			m_Move_Ing_Motion = m_Move_Start_Motion;
			m_Move_End_Motion = -1;
		}
	}
	else
	{	// �������
		m_StandardMotion = eMotion_Battle_Gum_Standard + WeaponEquipType -1;
		
		if(m_MoveInfo.MoveMode == eMoveMode_Walk)
		{	// �ȱ�
			m_Move_Start_Motion = eMotion_Battle_Gum_Walk + WeaponEquipType -1;
			m_Move_Ing_Motion = eMotion_Battle_Gum_Walk + WeaponEquipType -1;
			m_Move_End_Motion = -1;
		}
		else
		{	// �ٱ�	
			m_Move_Start_Motion = eMotion_Battle_Gum_Run + WeaponEquipType -1;
			m_Move_Ing_Motion = eMotion_Battle_Gum_Run + WeaponEquipType -1;
			m_Move_End_Motion = -1;
		}
	}
}

BOOL CPlayer::StartSocietyAct( int nStartMotion, int nIngMotion, int nEndMotion, BOOL bHideWeapon )
{
	//��� ���°� �ƴϸ� �ȵȴ�.
	if( GetState() != eObjectState_None && GetState() != eObjectState_Society
		&& GetState() != eObjectState_Immortal ) return FALSE;

	OBJECTSTATEMGR->EndObjectState( this, GetState(), 0 );	//���� ���¸� ������.

	m_ObjectState.State_Start_Motion	= (short int)nStartMotion;
	m_ObjectState.State_Ing_Motion		= (short int)nIngMotion;
	m_ObjectState.State_End_Motion		= (short int)nEndMotion;

	if( nEndMotion != -1 )
		m_ObjectState.State_End_MotionTime	= GetEngineObject()->GetAnimationTime(nEndMotion);
	else
		m_ObjectState.State_End_MotionTime = 0;


	if(m_ObjectState.State_Start_Motion != -1)
	{
		ChangeMotion( m_ObjectState.State_Start_Motion, FALSE );

		if( m_ObjectState.State_Ing_Motion != -1 )
			ChangeBaseMotion( m_ObjectState.State_Ing_Motion );
	}

	if(m_ObjectState.State_End_MotionTime)
	m_ObjectState.BeforeState		= m_BaseObjectInfo.ObjectState;
	m_BaseObjectInfo.ObjectState	= eObjectState_Society;
	m_ObjectState.State_Start_Time	= gCurTime;
	m_ObjectState.bEndState			= FALSE;

	if( bHideWeapon )
		APPEARANCEMGR->HideWeapon( this );

	m_ObjectState.BeforeState		= m_BaseObjectInfo.ObjectState;
	m_BaseObjectInfo.ObjectState	= eObjectState_Society;
	m_ObjectState.State_Start_Time	= gCurTime;
	m_ObjectState.bEndState			= FALSE;
	
	if( m_ObjectState.State_Ing_Motion == -1 )
		OBJECTSTATEMGR->EndObjectState( this, eObjectState_Society, GetEngineObject()->GetAnimationTime( m_ObjectState.State_Start_Motion ) );

	return TRUE;
}


void CPlayer::SetMotionInState(BYTE State)
{
	switch(State)
	{
	case eObjectState_StreetStall_Owner:
		{
			m_ObjectState.State_Start_Motion = eMotion_StreetStall_Start;
			m_ObjectState.State_Ing_Motion = eMotion_StreetStall_Ing;
			m_ObjectState.State_End_Motion = eMotion_StreetStall_End;
			m_ObjectState.State_End_MotionTime = GetEngineObject()->GetAnimationTime(eMotion_StreetStall_End);
		}
		break;
	case eObjectState_Move:
		{
			m_ObjectState.State_Start_Motion = m_Move_Start_Motion;
			m_ObjectState.State_Ing_Motion = m_Move_Ing_Motion;
			m_ObjectState.State_End_Motion = m_Move_End_Motion;
			m_ObjectState.State_End_MotionTime = GetEngineObject()->GetAnimationTime(m_Move_End_Motion);
		}
		break;
	case eObjectState_None:
	case eObjectState_Exchange:
	case eObjectState_Deal:
	case eObjectState_Munpa:
	case eObjectState_PrivateWarehouse:
	case eObjectState_StreetStall_Guest:
		{
			m_ObjectState.State_Start_Motion = m_StandardMotion;
			
			//YH2DO
			if( GetState() == eObjectState_SkillSyn ||
				GetState() == eObjectState_SkillUsing )
				m_ObjectState.State_Start_Motion = -1;

			m_ObjectState.State_Ing_Motion = m_StandardMotion;
			m_ObjectState.State_End_Motion = -1;
			m_ObjectState.State_End_MotionTime = 0;
		}
		break;
	case eObjectState_Ungijosik:
		{
			m_ObjectState.State_Start_Motion = eMotion_Ungijosik_Start;
			m_ObjectState.State_Ing_Motion = eMotion_Ungijosik_Ing;
			m_ObjectState.State_End_Motion = eMotion_Ungijosik_End;
			m_ObjectState.State_End_MotionTime = GetEngineObject()->GetAnimationTime(eMotion_Ungijosik_End);
		}
		break;
		
	case eObjectState_Tactic:
		{
			m_ObjectState.State_Start_Motion = eMotion_Tactic_Start;
			m_ObjectState.State_Ing_Motion = eMotion_Tactic_Ing;
			m_ObjectState.State_End_Motion = -1;
			m_ObjectState.State_End_MotionTime = 0;
		}
		break;
	case eObjectState_Engrave:
		{
			m_ObjectState.State_Start_Motion = eMotion_Engrave_End;
			m_ObjectState.State_Ing_Motion = eMotion_Engrave_Ing;
			m_ObjectState.State_End_Motion = -1;
			m_ObjectState.State_End_MotionTime = GetEngineObject()->GetAnimationTime(eMotion_Ungijosik_End);
		}
		break;
	case eObjectState_TitanRecall:
		{
			m_ObjectState.State_Start_Motion = eMotion_Engrave_Start;
			m_ObjectState.State_Ing_Motion = eMotion_Engrave_Ing;
			m_ObjectState.State_End_Motion = -1;
			m_ObjectState.State_End_MotionTime = 0;
		}
		break;
	default:
		{
			m_ObjectState.State_Start_Motion = -1;
			m_ObjectState.State_Ing_Motion = -1;
			m_ObjectState.State_End_Motion = -1;
			m_ObjectState.State_End_MotionTime = 0;
		}
		break;
	}
}

BYTE CPlayer::GetLifePercent()
{
	return (BYTE)((m_CharacterInfo.Life*100) / GetMaxLife());
}

BYTE CPlayer::GetShieldPercent()
{
	return (BYTE)((m_CharacterInfo.Shield*100) / GetMaxShield());
}

//////////////////////////////////////////////////////////////////////////
// ObjectStateManager���� State�� ���۰� ������ ȣ�����ִ� �Լ���
void CPlayer::OnStartObjectState(BYTE State)
{
	switch(State)
	{
	case eObjectState_Move:
		{
			// magi82(4) - Titan(071023) Ÿ��ź�϶� ����Ʈ ó��(�ȱ� & �ٱ�)
			// magi82(11) - Effect(071025) ��� ����Ʈ ID �ο�
			if( this == HERO )
			{
				WORD KyungGongIdx = ((CObject*)this)->GetBaseMoveInfo()->KyungGongIdx;

				if( KyungGongIdx > 0 )
				{
					KyungGongIdx = KyungGongIdx - 2600;	// ����� 2600�����̹Ƿ� ���� ZeroBase ȭ ���Ѿ���
					
					switch(KyungGongIdx)
					{
					case 0:	// ����麯
						{
							if(this->InTitan() == TRUE)
							{
								if( GetAvatarOption()->bKyungGong || GetShopItemStats()->bKyungGong )
								{
									OBJECTEFFECTDESC desc(eEffect_Titan_Shop_KyungGong1);
									AddObjectEffect(KYUNGGONG_EFFECTID,&desc,1);				
								}
								else
								{
									OBJECTEFFECTDESC desc(eEffect_Titan_KyungGong1);
									AddObjectEffect(KYUNGGONG_EFFECTID,&desc,1);				
								}
							}
							else
							{
								OBJECTEFFECTDESC desc(eEffect_KyungGong1);
								AddObjectEffect(KYUNGGONG_EFFECTID,&desc,1);
							}
						}
						break;
					case 2:	// �ʻ��
						{
							if(this->InTitan() == TRUE)
							{
 								if( GetAvatarOption()->bKyungGong || GetShopItemStats()->bKyungGong )
								{
									OBJECTEFFECTDESC desc(eEffect_Titan_Shop_KyungGong2);
									AddObjectEffect(KYUNGGONG_EFFECTID,&desc,1);				
								}
								else
								{
									OBJECTEFFECTDESC desc(eEffect_Titan_KyungGong2);
									AddObjectEffect(KYUNGGONG_EFFECTID,&desc,1);				
								}
							}
							else
							{
								OBJECTEFFECTDESC desc(eEffect_KyungGong2);
								AddObjectEffect(KYUNGGONG_EFFECTID,&desc,1);
							}
						}
						break;
					case 4:	// �ɰ��㵵
						{
							if(this->InTitan() == TRUE)
							{
								if( GetAvatarOption()->bKyungGong || GetShopItemStats()->bKyungGong )
								{
									OBJECTEFFECTDESC desc(eEffect_Titan_Shop_KyungGong3);
									AddObjectEffect(KYUNGGONG_EFFECTID,&desc,1);				
								}
								else
								{
									OBJECTEFFECTDESC desc(eEffect_Titan_KyungGong3);
									AddObjectEffect(KYUNGGONG_EFFECTID,&desc,1);				
								}
							}
							else
							{
								OBJECTEFFECTDESC desc(eEffect_KyungGong3);
								AddObjectEffect(KYUNGGONG_EFFECTID,&desc,1);
							}
						}
						break;
					}
				}
				else
				{
					if( ((CObject*)this)->GetBaseMoveInfo()->MoveMode == eMoveMode_Run )
					{
						if(this->InTitan() == TRUE)
						{
							OBJECTEFFECTDESC desc(FindEffectNum("t_ba_004.beff"));
							AddObjectEffect(KYUNGGONG_EFFECTID,&desc,1);
						}
						else
						{
							OBJECTEFFECTDESC desc(eEffect_Run);
							AddObjectEffect(KYUNGGONG_EFFECTID,&desc,1);
						}
					}
					else if( ((CObject*)this)->GetBaseMoveInfo()->MoveMode == eMoveMode_Walk )
					{
						if(this->InTitan() == TRUE)
						{
							OBJECTEFFECTDESC desc(FindEffectNum("t_ba_003.beff"));
							AddObjectEffect(KYUNGGONG_EFFECTID,&desc,1);
						}
						else
						{
							OBJECTEFFECTDESC desc(eEffect_Walk);
							AddObjectEffect(KYUNGGONG_EFFECTID,&desc,1);
						}
					}
				}
			}
		}
		break;
	case eObjectState_Exit:
		{
			PEACEWARMGR->Peace_WarMode( this, TRUE );

			// magi82 - UniqueItem(071010) ���� �ƴҶ��� ���⸦ �����.
			if(GetWeaponEquipType() != WP_GWUN)
				APPEARANCEMGR->HideWeapon(this);
		}
		break;
	case eObjectState_Ungijosik:
		{
			// magi82 - UniqueItem(071010) ���� �ƴҶ��� ���⸦ �����.
			if(GetWeaponEquipType() != WP_GWUN)
				APPEARANCEMGR->HideWeapon(this);

			OBJECTEFFECTDESC desc(eEffect_Ungijosik);
			AddObjectEffect(STATE_EFFECT_ID,&desc,1);
		}
		break;
	case eObjectState_Tactic:
		{
		}
		break;
	case eObjectState_StreetStall_Owner:
		{
			// magi82 - UniqueItem(071010) ���� �ƴҶ��� ���⸦ �����.
			if(GetWeaponEquipType() != WP_GWUN)
				APPEARANCEMGR->HideWeapon(this);
		}
		break;
	case eObjectState_ItemUse:
		{
			if( m_ItemUseEffect == -1 )
				return;

			EFFECTMGR->StartPlayerEffectProcess( HERO, m_ItemUseEffect );
			m_ItemUseEffect = -1;
		}
		break;
	case eObjectState_TitanRecall:
		{
			OBJECTEFFECTDESC desc(FindEffectNum("m_Summon01.beff"));			
			AddObjectEffect( TITAN_RECALL_EFFECTID, &desc, 1, this );
		}
		break;
	}
}

void CPlayer::OnEndObjectState(BYTE State)
{
	switch(State)
	{
	case eObjectState_StreetStall_Owner:
	case eObjectState_Ungijosik:
	case eObjectState_Society:
		{
			//2008. 6. 30. CBH - ����ǥ�� ���� �� �ϱ� ���̰� ����
			WORD weaponType = GetWeaponEquipType();
			/*
			if(weaponType == WP_AMGI)
			{
				if(PEACEWARMGR->IsPeaceMode(this) == FALSE)
					APPEARANCEMGR->ShowWeapon(this);
			}
			//else if(weaponType == WP_GWUN)
			//{
			//	WORD WeaponItemIdx = GetWearedItemIdx(eWearedItem_Weapon);
			//	ITEM_INFO* pInfo = ITEMMGR->GetItemInfo(WeaponItemIdx);

			//	if(pInfo->ItemKind == eEQUIP_ITEM_UNIQUE)
			//	{
			//		APPEARANCEMGR->ShowWeapon(this);
			//	}
			//}
			else if(weaponType != WP_GWUN)
			{
				APPEARANCEMGR->ShowWeapon(this);
			}
			*/
			if(weaponType != WP_GWUN)
			{
				APPEARANCEMGR->ShowWeapon(this);
			}
		}
		break;
	case eObjectState_Move:
		{
			// magi82(11) - Effect(071025) ��� ����Ʈ ID �ο�
			RemoveObjectEffect(KYUNGGONG_EFFECTID);
		}
		break;
	case eObjectState_Die:
		{
			if(GetID() != HEROID)
				GetEngineObject()->EnablePick();
		}
		break;
	case eObjectState_Tactic:
		{
		}
		break;
	case eObjectState_ItemUse:
		{
			if( m_ItemUseMotion==eMotion_Item_ChangeHair_1 || m_ItemUseMotion==eMotion_Item_ChangeHair_2 || m_ItemUseMotion==eMotion_Item_ChangeHair_3 )
			{
				APPEARANCEMGR->InitAppearance( this );
			}

			//
			if(GetID() != HEROID )
				goto MotionReset;
			if( m_ItemUseMotion==eMotion_Item_Teleport_1 || m_ItemUseMotion==eMotion_Item_Teleport_2 || m_ItemUseMotion==eMotion_Item_Teleport_3 )
			{
				DWORD map = GAMEIN->GetMoveMap();
				WINDOWMGR->MsgBox( MBI_NOBTNMSGBOX, MBT_NOBTN, CHATMGR->GetChatMsg( 160 ) );
				MAPCHANGE->SetGameInInitKind(eGameInInitKind_MovePos);				
				MAINGAME->SetGameState(eGAMESTATE_MAPCHANGE,&map,4);
			}

MotionReset:
			m_ItemUseMotion = -1;
			m_ItemUseEffect = -1;
		}
		break;
	case eObjectState_TitanRecall:
		{
			RemoveObjectEffect( TITAN_RECALL_EFFECTID );
		}
		break;
	}
	
	RemoveObjectEffect(STATE_EFFECT_ID);	//????
}
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// �ൿ �Լ���..
void CPlayer::Die(CObject* pAttacker,BOOL bFatalDamage,BOOL bCritical, BOOL bDecisive) 
{	
	if(pAttacker)
	{
		MOVEMGR->SetLookatPos(this,&pAttacker->GetCurPosition(),0,gCurTime);

		if( HERO )
		if( pAttacker == HERO && HERO->IsPKMode() )
//		if( GUILDFIELDWAR->GetEnemyGuildIdx() != 0 )
		{
			PKMGR->AddPKContinueTime( 60000 );
			CHATMGR->AddMsg( CTC_SYSMSG, CHATMGR->GetChatMsg(698), 60 );
			//msg �������� ��� ���ӽð��� �߰��� 60�ʸ�ŭ �þ���ϴ�.
		}
	}
	
	int DieEffect;
	if(bFatalDamage)
		DieEffect = eEffect_Die2;	// ���󰡼� �ױ�
	else
		DieEffect = eEffect_Die1;	// �׳� �ױ�
		
	TARGETSET set;
	set.pTarget = this;
	set.ResultInfo.bCritical = bCritical;
	set.ResultInfo.bDecisive = bDecisive;
	
	EFFECTMGR->StartEffectProcess(DieEffect,pAttacker,&set,1,GetID(),
							EFFECT_FLAG_DIE|EFFECT_FLAG_GENDERBASISTARGET);

	CObject::Die(pAttacker,bFatalDamage,bCritical,bDecisive);
}

void CPlayer::Damage(CObject* pAttacker,BYTE DamageKind,DWORD Damage,DWORD ShieldDamage,BOOL bCritical, BOOL bDecisive, DWORD titanObserbDamage)
{
	// 2005 ũ�������� �̺�Ʈ
	BOOL bEvent = FALSE;

	if( pAttacker )
	{
        if( pAttacker->GetObjectKind() == eObjectKind_Player )
        if( ((CPlayer*)pAttacker)->GetWeaponEquipType() == WP_EVENT ||
			((CPlayer*)pAttacker)->GetWeaponEquipType() == WP_EVENT_HAMMER )
			bEvent = TRUE;
	}

	if(Damage != 0 || bEvent)
	{
		if( GetState() == eObjectState_None )	
		{
			if(pAttacker)
				MOVEMGR->SetLookatPos(this,&pAttacker->GetCurPosition(),0,gCurTime);
			
			//SW050826 ������� �ڵ���ȯ
			if( this == HERO )
			{
//				if( PEACEWARMGR->IsPeaceMode(this) )
//				{
//					PEACEWARMGR->ToggleHeroPeace_WarMode();
//				}
//				else if( !PEACEWARMGR->IsPeaceMode(this) )
//				{
//					PEACEWARMGR->SetCheckTime( gCurTime );
//				}
				if( m_CharacterInfo.bPeace )
				{
					PEACEWARMGR->ToggleHeroPeace_WarMode();
				}
				else
				{
					PEACEWARMGR->SetCheckTime( gCurTime );
				}
			}
		}
	
		if( GetState() == eObjectState_Ungijosik && m_ObjectState.bEndState == FALSE &&
			GetEngineObject()->GetCurMotion() != eMotion_Ungijosik_Start)
		{	// ��������̸� ��������� ������ ����
			//ChangeMotion(1,FALSE);
			ChangeMotion(eMotion_Ungijosik_Damage,FALSE);
			
#ifdef _JAPAN_LOCAL_
			if( HERO == this )
				UNGIMGR->ToggleHero_UngiMode();
#endif
		}
		else if(GetState() == eObjectState_None)
		{	// �Ϲ� ������ ����	
			//ChangeMotion(1,FALSE);
			switch(DamageKind) {
			case eDamageKind_Front:
				ChangeMotion(eMotion_Damage_Front,FALSE);
				break;
			case eDamageKind_Left:
				ChangeMotion(eMotion_Damage_Left,FALSE);
				break;
			case eDamageKind_Right:
				ChangeMotion(eMotion_Damage_Right,FALSE);
				break;
			}
		}
		
		int DamageEffect = -1;
		switch(DamageKind) {
		case eDamageKind_Front:
		case eDamageKind_Counter:
		case eDamageKind_ContinueDamage:
			{
				// magi82(4) - Titan(071023) Ÿ��ź�϶� ����Ʈ ó��(������1)
				if( InTitan() == TRUE )
                    DamageEffect = eEffect_Titan_Damage1;
				else
					DamageEffect = eEffect_Damage1;
			}
			break;
		case eDamageKind_Left:
			{
				// magi82(4) - Titan(071023) Ÿ��ź�϶� ����Ʈ ó��(������2)
				if( InTitan() == TRUE )
					DamageEffect = eEffect_Titan_Damage2;
				else
					DamageEffect = eEffect_Damage2;
			}
			break;
		case eDamageKind_Right:
			{
				// magi82(4) - Titan(071023) Ÿ��ź�϶� ����Ʈ ó��(������3)
				if( InTitan() == TRUE )
					DamageEffect = eEffect_Titan_Damage3;
				else
					DamageEffect = eEffect_Damage3;
			}
			break;
		}
		if(DamageEffect != -1)
		{
			TARGETSET set;
			set.pTarget = this;
			set.ResultInfo.bCritical = bCritical;
			set.ResultInfo.bDecisive = bDecisive;
			
			EFFECTMGR->StartEffectProcess(DamageEffect,pAttacker,&set,1,GetID(),
											EFFECT_FLAG_GENDERBASISTARGET);
		}
	}
	
	if( pAttacker == HERO || this == HERO )	//�ڽ��� ������ ��ȣ�� ǥ�� //KES 040801
	{
		//SW070127 Ÿ��ź	//Ÿ��ź ž�½� Ÿ��ź �������� ǥ��
		if( this->InTitan() && titanObserbDamage )
			EFFECTMGR->AddDamageNumber(titanObserbDamage,pAttacker,this,DamageKind,bCritical,bDecisive);
		else
			EFFECTMGR->AddDamageNumber(Damage,pAttacker,this,DamageKind,bCritical,bDecisive);
	}
	else if( pAttacker )
	{
		if( OPTIONMGR->IsShowMemberDamage() )
		if( PARTYMGR->IsPartyMember( pAttacker->GetID() ) )
		{
			EFFECTMGR->AddDamageNumber(Damage,pAttacker,this,DamageKind,bCritical,bDecisive);
		}
	}	

	// Life ����
	DWORD life = GetLife();
	if(life < Damage)
		life = 0;
	else
		life = life - Damage;
	SetLife(life);
	
	// Shield ����
	DWORD Shield = GetShield();
	if(Shield < ShieldDamage)
		Shield = 0;
	else
		Shield = Shield - ShieldDamage;
	SetShield(Shield,FALSE);

	//SW070127 Ÿ��ź
	if( this == HERO && InTitan() )
	{
		if( titanObserbDamage )
		{
			DWORD fuel = GetCurTitan()->GetLife();
			if( fuel < titanObserbDamage )
				fuel = 0;
			else
				fuel = fuel - titanObserbDamage;

			GetCurTitan()->SetLife(fuel);
		}
	}
}
void CPlayer::SetLife(DWORD life, BYTE type)
{
	m_CharacterInfo.Life = life; 

	
	if(GetState() == eObjectState_Die)
		life = 0;

	if( GAMEIN->GetMonsterGuageDlg())
	{
		if( GAMEIN->GetMonsterGuageDlg()->GetCurMonster() == (CObject*)this )
		{
			GAMEIN->GetMonsterGuageDlg()->SetMonsterLife( life, GetMaxLife() );
		}
	}
}
void CPlayer::SetShield(DWORD Shield, BYTE type)
{ 
	m_CharacterInfo.Shield = Shield; 

	
	if(GetState() == eObjectState_Die)
		Shield = 0;

	if( GAMEIN->GetMonsterGuageDlg() )
	{
		if( GAMEIN->GetMonsterGuageDlg()->GetCurMonster() == (CObject*)this )
		{
			GAMEIN->GetMonsterGuageDlg()->SetMonsterShield( Shield, GetMaxShield());
		}
	}
}

void CPlayer::Heal(CObject* pHealer,BYTE HealKind,DWORD HealVal)
{
	DWORD life = GetLife() + HealVal;
	SetLife(life);
}

void CPlayer::Recharge(CObject* pHealer,BYTE RechargeKind,DWORD RechargeVal)
{
	DWORD mana = GetNaeRyuk() + RechargeVal;
	SetNaeRyuk(mana);
}

void CPlayer::ShowStreetStallTitle( BOOL bShow, char* strTitle, DWORD dwColor )
{
	if( !m_pObjectBalloon )									return;
	if( !(m_bObjectBalloonBits & ObjectBalloon_Title ) )	return;

	m_pObjectBalloon->ShowStreeStallTitle( bShow, strTitle, dwColor );
}

void CPlayer::SetStreetStallTitle( char* strTitle )
{
	if( m_pObjectBalloon )
	if( m_pObjectBalloon->GetSSTitleTip() )
		m_pObjectBalloon->GetSSTitleTip()->SetTitle( strTitle );
	
}

void CPlayer::ShowStreetBuyStallTitle( BOOL bShow, char* strTitle, DWORD dwColor )
{
	if( !m_pObjectBalloon )									return;
	if( !(m_bObjectBalloonBits & ObjectBalloon_Title ) )	return;
	
	m_pObjectBalloon->ShowStreeBuyStallTitle( bShow, strTitle, dwColor );
}

void CPlayer::SetStreetBuyStallTitle( char* strTitle )
{
	if( m_pObjectBalloon )
		if( m_pObjectBalloon->GetSBSTitleTip() )
			m_pObjectBalloon->GetSBSTitleTip()->SetTitle( strTitle );
		
}

float CPlayer::DoGetMoveSpeed()
{
	float Speed = 0;
	/*
	if(m_MoveInfo.KyungGongIdx)
	{
		if( this == HERO )
		{
			Speed = MOVEMGR->GetKyungGongSpeed(m_MoveInfo.KyungGongIdx) 
				  + GetAbilityStats()->Kyunggong;
		}
		else
		{
			Speed = MOVEMGR->GetKyungGongSpeed(m_MoveInfo.KyungGongIdx) 
				  + ABILITYMGR->GetAbilityKyungGongSpeed( m_MoveInfo.AbilityKyungGongLevel );
		}
		
		// �ƹ�Ÿ ������ ��� ���ǵ� 
		Speed += GetAvatarOption()->KyunggongSpeed;
		// ���� ��� �ӵ� ���
		Speed += GetShopItemStats()->KyungGongSpeed;
	}
	else
	{
		if(m_MoveInfo.MoveMode == eMoveMode_Run)
			Speed = RUNSPEED;
		else
			Speed = WALKSPEED;
	}*/
	//SW070127 Ÿ��ź
	if( InTitan() )
	{
		if( m_MoveInfo.KyungGongIdx )
		{
			TITANINFO_GRADE* pGradeInfo = GAMERESRCMNGR->GetTitanGradeInfo(m_TitanAppearanceInfo.TitanKind, m_TitanAppearanceInfo.TitanGrade);
			if( !pGradeInfo )
				return TITAN_WALKSPEED;

			WORD klev = 0;		// ����麯
			if( m_MoveInfo.KyungGongIdx == 2602 )
				klev = 1;		// �ʻ��
			else if( m_MoveInfo.KyungGongIdx == 2604 )
				klev = 2;		// 
			Speed = pGradeInfo->KyungGongSpeed[klev];

			// �ƹ�Ÿ ������ ��� ���ǵ� 
			Speed += GetAvatarOption()->KyunggongSpeed;
			// ���� ��� �ӵ� ���
			Speed += GetShopItemStats()->KyungGongSpeed;
		}
		else
		{
			if(m_MoveInfo.MoveMode == eMoveMode_Run)
				Speed = GetTitanMoveSpeed();
			else
				Speed =TITAN_WALKSPEED;
		}
	}

	else
	{
		if(m_MoveInfo.KyungGongIdx)
		{
			if( this == HERO )
			{
				Speed = MOVEMGR->GetKyungGongSpeed(m_MoveInfo.KyungGongIdx) 
					+ GetAbilityStats()->Kyunggong;
			}
			else
			{
				Speed = MOVEMGR->GetKyungGongSpeed(m_MoveInfo.KyungGongIdx) 
					+ ABILITYMGR->GetAbilityKyungGongSpeed( m_MoveInfo.AbilityKyungGongLevel );
			}

			// �ƹ�Ÿ ������ ��� ���ǵ� 
			Speed += GetAvatarOption()->KyunggongSpeed;

			//SW ���� �̺�Ʈ ���� �ӽ� �ϵ� �ڵ�
#define SHOPITEM_COS_CAT_HAT		56019
#define SHOPITEM_COS_CAT_DRESS		56020
#define SHOPITEM_COS_WEDDING_MAN	56021
#define SHOPITEM_COS_WEDDING_WOMAN	56022
			if( WEATHERMGR->GetWeatherState() == eWS_Snow )
			{
				if( (GetShopItemStats()->Avatar[eAvatar_Dress] ==  SHOPITEM_COS_CAT_DRESS) ||
					(GetShopItemStats()->Avatar[eAvatar_Dress] ==  SHOPITEM_COS_WEDDING_MAN) ||
					(GetShopItemStats()->Avatar[eAvatar_Dress] ==  SHOPITEM_COS_WEDDING_WOMAN) )
					Speed += 50;
			}
			// ���� ��� �ӵ� ���
			Speed += GetShopItemStats()->KyungGongSpeed;
		}
		else
		{
			if(m_MoveInfo.MoveMode == eMoveMode_Run)
				Speed = RUNSPEED;
			else
				Speed = WALKSPEED;
		}
	}
	return Speed;
}

void CPlayer::SetPKMode( BOOL bPKMode )
{
	m_CharacterInfo.bPKMode = bPKMode ? true : false;	
}

DWORD CPlayer::DoGetMaxLife()
{
	return m_CharacterInfo.MaxLife;
}

DWORD CPlayer::DoGetMaxShield()
{
	return m_CharacterInfo.MaxShield;
}

void CPlayer::SetBadFame(FAMETYPE val)
{
	m_CharacterInfo.BadFame = val;
}

void CPlayer::SetShopItemOptionInfo(SHOPITEMOPTION* pInfo)
{
	if(!pInfo)			return;
	memcpy(&m_ShopItemOption, pInfo, sizeof(SHOPITEMOPTION));
}

void CPlayer::SetGuildIdxRank(DWORD GuildIdx, BYTE Rank)
{ 
	m_CharacterInfo.MunpaID = GuildIdx;
	m_CharacterInfo.PositionInMunpa = Rank;
}

void CPlayer::SetGuildMarkName(MARKNAMETYPE MarkName)
{
	m_CharacterInfo.MarkName = MarkName;
}

void CPlayer::SetNickName(char* pNickName)
{
	SafeStrCpy(m_CharacterInfo.NickName, pNickName, MAX_GUILD_NICKNAME+1);
}

MARKNAMETYPE CPlayer::GetGuildMarkName()
{
	return m_CharacterInfo.MarkName;
}

void CPlayer::SetGuildIdx(DWORD GuildIdx)
{
	m_CharacterInfo.MunpaID = GuildIdx;
}

void CPlayer::ClearGuildInfo()
{
	m_CharacterInfo.MunpaID = 0;
	m_CharacterInfo.PositionInMunpa = GUILD_NOTMEMBER;
	m_CharacterInfo.MarkName = 0;
	SafeStrCpy(m_CharacterInfo.NickName, "", MAX_GUILD_NICKNAME+1);
	SafeStrCpy(m_CharacterInfo.GuildName, "", MAX_GUILD_NAME+1);
}

char* CPlayer::GetNickName()
{
	return m_CharacterInfo.NickName;
}

void CPlayer::SetGuildName(char* GuildName)
{
	SafeStrCpy(m_CharacterInfo.GuildName, GuildName, MAX_GUILD_NAME+1);
}

char* CPlayer::GetGuildName()
{
	return m_CharacterInfo.GuildName;
}

void CPlayer::SetPet( CPet* pet )
{
	m_pPet = pet;

//	if(!pet)		return;
//	pet->SetMaster(this);
}

void CPlayer::SetStage( BYTE Stage )
{
	m_CharacterInfo.Stage = Stage;
}


void CPlayer::SetCharChangeInfo( CHARACTERCHANGE_INFO* pInfo )
{
	m_CharacterInfo.Gender = pInfo->Gender;
	m_CharacterInfo.HairType = pInfo->HairType;
	m_CharacterInfo.FaceType = pInfo->FaceType;
	m_CharacterInfo.Height = pInfo->Height;
	m_CharacterInfo.Width = pInfo->Width;

	APPEARANCEMGR->AddCharacterPartChange( GetID() );
}

void CPlayer::SetSiegeName( DWORD NameType )
{
	if( m_pObjectBalloon )
		m_pObjectBalloon->SetSiegeName( NameType, strlen(GetObjectName()) );
}

void CPlayer::CalcAvatarOption()
{
	memset( &m_AvatarOption, 0, sizeof(AVATARITEMOPTION) );

	for(int i=0; i<eAvatar_Max; i++)
	{
		if( m_ShopItemOption.Avatar[i] < 2 )					continue;

		ITEM_INFO* pItemInfo = ITEMMGR->GetItemInfo( m_ShopItemOption.Avatar[i] );
		if( !pItemInfo )		continue;
		
		// �ٰ�
		if( pItemInfo->GenGol > 0 )
			m_AvatarOption.Gengol += pItemInfo->GenGol;
		// ��ø
		if( pItemInfo->MinChub > 0 )
			m_AvatarOption.Minchub += pItemInfo->MinChub;
		// ü��
		if( pItemInfo->CheRyuk > 0 )
			m_AvatarOption.Cheryuk += pItemInfo->CheRyuk;
		// �ɸ�
		if( pItemInfo->SimMek > 0 )
			m_AvatarOption.Simmek += pItemInfo->SimMek;
		// �ִ������
		if( pItemInfo->Life > 0 )
			m_AvatarOption.Life += (WORD)pItemInfo->Life;
		// �ִ�ȣ�Ű���
		if( pItemInfo->Shield > 0 )
			m_AvatarOption.Shield += (WORD)pItemInfo->Shield;
		// �ִ볻��
		if( pItemInfo->NaeRyuk > 0 )
			m_AvatarOption.Naeruyk += pItemInfo->NaeRyuk;
		// ������ݷ�
		if( pItemInfo->MeleeAttackMin > 0 )
			m_AvatarOption.Attack += pItemInfo->MeleeAttackMin;
		// ũ��Ƽ�ü�ġ
		if( pItemInfo->CriticalPercent > 0 )
			m_AvatarOption.Critical += pItemInfo->CriticalPercent;
		// �ݰ�Ȯ��
		if( pItemInfo->Plus_MugongIdx > 0 )
			m_AvatarOption.CounterPercent += pItemInfo->Plus_MugongIdx;
		// �ݰݽ� ��������
		if( pItemInfo->Plus_Value > 0 )
			m_AvatarOption.CounterDamage += pItemInfo->Plus_Value;
		// ���������
		if( pItemInfo->AllPlus_Kind == 1 )
			m_AvatarOption.bKyungGong = 1;
		// ���¼Ҹ𰨼�
		if( pItemInfo->LimitCheRyuk > 0 )
			m_AvatarOption.NeaRyukSpend += pItemInfo->LimitCheRyuk;
		// ���� ������
		if( pItemInfo->LimitJob > 0 )
			m_AvatarOption.NeagongDamage += pItemInfo->LimitJob;
		// �ܰ� ������
		if( pItemInfo->LimitGender > 0 )
			m_AvatarOption.WoigongDamage += pItemInfo->LimitGender;		
		// ���� �������� ����
		if( pItemInfo->LimitLevel > 0 )
			m_AvatarOption.TargetPhyDefDown += pItemInfo->LimitLevel;
		// ���� �Ӽ����׷� ����
		if( pItemInfo->LimitGenGol > 0 )
			m_AvatarOption.TargetAttrDefDown += pItemInfo->LimitGenGol;
		// ���� ���ݷ� ����
		if( pItemInfo->LimitMinChub > 0 )
			m_AvatarOption.TargetAtkDown += pItemInfo->LimitMinChub;
		// ȸ���� �߰� ����ġ
		if( pItemInfo->LimitSimMek )
			m_AvatarOption.RecoverRate += pItemInfo->LimitSimMek;
		// ����ӵ� ���
		if( pItemInfo->ItemGrade )
			m_AvatarOption.KyunggongSpeed += pItemInfo->ItemGrade;
		// ��ȥ������ �����ӵ�
		if( pItemInfo->RangeType )
			m_AvatarOption.MussangCharge += pItemInfo->RangeType;
		// ������� ���¼Ҹ𿩺�
		if( pItemInfo->EquipKind == 1 )
			m_AvatarOption.NaeruykspendbyKG = 1;
		//////////////////////////////////////////////////////
		// 06. 07 ���� ����(�ϰ�) - �̿���
		if( pItemInfo->NaeRyukRecover )
			m_AvatarOption.Decisive += pItemInfo->NaeRyukRecover;
		// ��ȥ���ߵ��� ����������
        if( pItemInfo->RangeAttackMax > 0 )
			m_AvatarOption.MussangDamage += pItemInfo->RangeAttackMax;
	}

	// Character Dialog
	if( GetID() == HEROID )
		GAMEIN->GetCharacterDialog()->RefreshInfo();
}

#ifdef _JAPAN_LOCAL_
int CPlayer::WhatIsAttrRelation( int nOtherMainAttr )
{
	if( m_CharacterInfo.nMainAttr == eCA_None || nOtherMainAttr == eCA_None )	return eCAR_None;

	int nRelation = nOtherMainAttr - GetMainCharAttr();
	if( nRelation < 0 )	nRelation += 5;

	return nRelation;
}
#endif // _JAPAN_LOCAL_

//////////////////////////////////////////////////////////////////////////
// 06. 06. 2�� ���� - �̿���
// ���� ��ȯ �߰�
void CPlayer::AddSkillStatsOption(SKILLOPTION *pSkillOption)
{
	m_SkillStatsOption.Life += pSkillOption->Life;
	m_SkillStatsOption.NaeRyuk += pSkillOption->NaeRyuk;
	m_SkillStatsOption.Shield += pSkillOption->Shield;
	m_SkillStatsOption.PhyDef += pSkillOption->PhyDef;
	m_SkillStatsOption.AttDef += pSkillOption->AttDef;

	m_SkillStatsOption.Range += pSkillOption->Range;
	m_SkillStatsOption.ReduceNaeRyuk += pSkillOption->ReduceNaeRyuk;
	m_SkillStatsOption.PhyAtk += pSkillOption->PhyAtk;
	m_SkillStatsOption.BaseAtk += pSkillOption->BaseAtk;
	m_SkillStatsOption.AttAtk += pSkillOption->AttAtk;
}

void CPlayer::RemoveSkillStatsOption(SKILLOPTION *pSkillOption)
{
	m_SkillStatsOption.Life -= pSkillOption->Life;
	m_SkillStatsOption.NaeRyuk -= pSkillOption->NaeRyuk;
	m_SkillStatsOption.Shield -= pSkillOption->Shield;
	m_SkillStatsOption.PhyDef -= pSkillOption->PhyDef;
	m_SkillStatsOption.AttDef -= pSkillOption->AttDef;

	m_SkillStatsOption.Range -= pSkillOption->Range;
	m_SkillStatsOption.ReduceNaeRyuk -= pSkillOption->ReduceNaeRyuk;
	m_SkillStatsOption.PhyAtk -= pSkillOption->PhyAtk;
	m_SkillStatsOption.BaseAtk -= pSkillOption->BaseAtk;
	m_SkillStatsOption.AttAtk -= pSkillOption->AttAtk;
}
//////////////////////////////////////////////////////////////////////////

//SW070127 Ÿ��ź
void CPlayer::SetTitanAppearanceInfo( TITAN_APPEARANCEINFO* pInfo )
{
	if(pInfo)
	{
		m_TitanAppearanceInfo = *pInfo;

		SetTitanMoveSpeed();
	}
	else
	{
		memset(&m_TitanAppearanceInfo,0,sizeof(TITAN_APPEARANCEINFO));
		memset(&m_TitanPreViewAppInfo,0,sizeof(TITAN_APPEARANCEINFO));		
	}
}

void CPlayer::SetTitanMoveSpeed()
{
	m_TitanMoveSpeed = 0;

	// Ÿ��ź �̵� �ӵ� ����
	TITAN_APPEARANCEINFO* pAprInfo = NULL;
	if( InTitan() )
		pAprInfo = GetTitanAppearInfo();	//pAprInfo not NULL.member
	else if( IsTitanPreView() )
		pAprInfo = GetTitanPreViewInfo();	//pAprInfo not NULL.member

	if( !pAprInfo )
		return;

	TITANINFO_GRADE* pGradeInfo = GAMERESRCMNGR->GetTitanGradeInfo(pAprInfo->TitanKind, pAprInfo->TitanGrade);
	if(pGradeInfo)
        m_TitanMoveSpeed = (float)pGradeInfo->Speed;

	// ������ �ɼǺκ�
	for( int i = 0; i < eTitanWearedItem_Max; ++i )
	{
		if(pAprInfo->WearedItemIdx[i])
		{
			ITEM_INFO* pInfo = ITEMMGR->GetItemInfo(pAprInfo->WearedItemIdx[i]);
			if(pInfo)	// magi82 - Titan(070705)
			{
				m_TitanMoveSpeed += pInfo->AllPlus_Value;	//	���� ������ ����+ ����
			}
		}
	}
}

void CPlayer::SetCurTitan( CTitan* pTitan )
{
	if(this == HERO)
	{
		TITANMGR->SetCurRidingTitan(pTitan);
	}
	
	m_pTitan = pTitan;

	if( pTitan )
		RidingTitan(TRUE);
	else
		RidingTitan(FALSE);
}

void CPlayer::SetTitanPreView( TITAN_APPEARANCEINFO* pInfo )
{
	m_bTitanPreView = TRUE;

	m_TitanPreViewAppInfo = *pInfo;
	APPEARANCEMGR->InitAppearance( this );
}

void CPlayer::ReleaseTitanPreView()
{
	if( m_bTitanPreView )
	{
		m_bTitanPreView = FALSE;
		APPEARANCEMGR->InitAppearance( this );
	}
}

//2007. 8. 8. CBH - ��Ʈ������ ����Ʈ ���� �߰�
DWORD CPlayer::GetSetItemEffectID()
{
	return m_dwSetItemEffectID;
}
void CPlayer::SetSetItemEffectID(DWORD dwEffectID)
{
	m_dwSetItemEffectID = dwEffectID;
}

//2008. 1. 21. CBH - ��Ų ������ ���� �Լ�
void CPlayer::InitSkinDelayTime()
{
	m_dwSkinDelayTime = 0;	//������ �� �ð� ����
	m_bSkinDelayResult = FALSE;	//������������ üũ�ϴ� ����
}

void CPlayer::StartSkinDelayTime()
{
	m_dwSkinDelayTime = gCurTime + 10000;
	m_bSkinDelayResult = TRUE;
}

BOOL CPlayer::CheckSkinDelay()
{
	//������ ó��
	if(m_bSkinDelayResult == TRUE)
	{
		DWORD dwTime = m_dwSkinDelayTime - gCurTime;
		if(gCurTime > m_dwSkinDelayTime)
		{
			dwTime = 0;
		}		
		CHATMGR->AddMsg( CTC_SYSMSG, CHATMGR->GetChatMsg(1355), dwTime/1000+1);

		return FALSE;
	}

	return TRUE;
}
//////////////////////////////////////////////////


// magi82(47)
void CPlayer::SetStreetStallBalloonImage( DWORD eKind, BOOL bFlag )
{
	CStreetStallTitleTip* pStallTitle = NULL;

	cImage left1;
	cImage left2;
	cImage left3;
	cImage center1;
	cImage center2;
	cImage center3;
	cImage right1;
	cImage right2;
	cImage right3;

	if( eSK_SELL == eKind )
		pStallTitle = m_pObjectBalloon->GetSSTitleTip();
	else if( eSK_BUY == eKind )
		pStallTitle = m_pObjectBalloon->GetSBSTitleTip();

	if( TRUE == bFlag )
	{
		SCRIPTMGR->GetImage(138, &left1);
		SCRIPTMGR->GetImage(139, &center1);
		SCRIPTMGR->GetImage(140, &right1);
		SCRIPTMGR->GetImage(141, &left2);
		SCRIPTMGR->GetImage(142, &center2);
		SCRIPTMGR->GetImage(143, &right2);
		SCRIPTMGR->GetImage(144, &left3);
		SCRIPTMGR->GetImage(145, &center3);
		SCRIPTMGR->GetImage(146, &right3);	
	}
	else
	{
		if( eSK_SELL == eKind )
		{
			SCRIPTMGR->GetImage(117, &left1);
			SCRIPTMGR->GetImage(118, &center1);
			SCRIPTMGR->GetImage(119, &right1);
			SCRIPTMGR->GetImage(120, &left2);
			SCRIPTMGR->GetImage(121, &center2);
			SCRIPTMGR->GetImage(122, &right2);
			SCRIPTMGR->GetImage(123, &left3);
			SCRIPTMGR->GetImage(124, &center3);
			SCRIPTMGR->GetImage(125, &right3);

		}
		else if( eSK_BUY == eKind )
		{
			SCRIPTMGR->GetImage(128, &left1);
			SCRIPTMGR->GetImage(129, &center1);
			SCRIPTMGR->GetImage(130, &right1);
			SCRIPTMGR->GetImage(131, &left2);
			SCRIPTMGR->GetImage(132, &center2);
			SCRIPTMGR->GetImage(133, &right2);
			SCRIPTMGR->GetImage(134, &left3);
			SCRIPTMGR->GetImage(135, &center3);
			SCRIPTMGR->GetImage(136, &right3);
		}
	}

	pStallTitle->InitBalloonForChat(&left1, &center1, &right1, &left2, &center2, &right2, &left3, &center3, &right3, 8);
}