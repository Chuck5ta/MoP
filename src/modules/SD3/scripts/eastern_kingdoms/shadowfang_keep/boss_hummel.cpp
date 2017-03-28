/* Copyright (C) 2006 - 2013 ScriptDev2 <http://www.scriptdev2.com/>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

/* ScriptData
SDName: boss_hummel
SD%Complete: 50
SDComment: The bosses are handled in eventAI; The event needs more research; only the basics are implemented; Check crazed apothecary timer
SDCategory: Shadowfang Keep
EndScriptData */

#if defined (WOTLK) || defined (CATA)
#include "precompiled.h"
#include "shadowfang_keep.h"

enum
{
    SAY_INTRO_1                     = -1033020,
    SAY_INTRO_2                     = -1033021,
    SAY_INTRO_3                     = -1033022,
    SAY_CALL_BAXTER                 = -1033023,
    SAY_CALL_FRYE                   = -1033024,

    SPELL_SUMMON_VALENTINE_ADD      = 68610,        // summons the crazy apothecary

    // It's unk who is handling the summoning of the crazed apothecary. This may get the following npcs involved: 36212
    NCP_CRAZED_APOTHECARY           = 36568,        // casts spell 68957 on range check

    QUEST_BEEN_SERVED               = 14488,

    FACTION_HOSTILE                 = 14,
};

static const DialogueEntry aIntroDialogue[] =
{
    {QUEST_BEEN_SERVED, 0,          1000},
    {SAY_INTRO_1,       NPC_HUMMEL, 4000},
    {SAY_INTRO_2,       NPC_HUMMEL, 4000},
    {SAY_INTRO_3,       NPC_HUMMEL, 3000},
    {NPC_HUMMEL,        0,          8000},
    {NPC_BAXTER,        0,          8000},
    {NPC_FRYE,          0,          0},
    {0, 0, 0},
};

struct npc_valentine_boss_manager : public CreatureScript
{
    npc_valentine_boss_manager() : CreatureScript("npc_valentine_boss_manager") {}

    struct  npc_valentine_boss_managerAI : public ScriptedAI, private DialogueHelper
    {
        npc_valentine_boss_managerAI(Creature* pCreature) : ScriptedAI(pCreature),
        DialogueHelper(aIntroDialogue)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
            InitializeDialogueHelper(m_pInstance);
        }

        ScriptedInstance* m_pInstance;

        uint32 m_uiCrazedApothecaryTimer;

        ObjectGuid m_EventStarterGuid;

        void Reset() override
        {
            m_uiCrazedApothecaryTimer = 30000;
        }

        void JustDidDialogueStep(int32 iEntry) override
        {
            if (!m_pInstance)
                return;

            Creature* pHummel = m_pInstance->GetSingleCreatureFromStorage(NPC_HUMMEL);
            switch (iEntry)
            {
            case NPC_HUMMEL:
                if (pHummel)
                {
                    // WARNING: workaround -> faction should be set on event start
                    pHummel->SetFactionTemporary(FACTION_HOSTILE, TEMPFACTION_RESTORE_REACH_HOME | TEMPFACTION_RESTORE_RESPAWN);
                    pHummel->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PASSIVE | UNIT_FLAG_NON_ATTACKABLE);

                    if (Player* pPlayer = m_creature->GetMap()->GetPlayer(m_EventStarterGuid))
                        pHummel->AI()->AttackStart(pPlayer);
                }

                m_pInstance->SetData(TYPE_APOTHECARY, IN_PROGRESS);
                break;
            case NPC_BAXTER:
                if (!pHummel)
                    return;

                if (Creature* pBaxter = m_pInstance->GetSingleCreatureFromStorage(NPC_BAXTER))
                {
                    DoScriptText(SAY_CALL_BAXTER, pHummel);

                    // WARNING: workaround -> faction should be set on event start
                    pBaxter->SetFactionTemporary(FACTION_HOSTILE, TEMPFACTION_RESTORE_REACH_HOME | TEMPFACTION_RESTORE_RESPAWN);
                    pBaxter->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PASSIVE | UNIT_FLAG_NON_ATTACKABLE);
                    if (pHummel->getVictim())
                        pBaxter->AI()->AttackStart(pHummel->getVictim());
                break;
            }
            case NPC_FRYE:
                if (!pHummel)
                    return;

                if (Creature* pFrye = m_pInstance->GetSingleCreatureFromStorage(NPC_FRYE))
                {
                    DoScriptText(SAY_CALL_FRYE, pHummel);

                    // WARNING: workaround -> faction should be set on event start
                    pFrye->SetFactionTemporary(FACTION_HOSTILE, TEMPFACTION_RESTORE_REACH_HOME | TEMPFACTION_RESTORE_RESPAWN);
                    pFrye->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PASSIVE | UNIT_FLAG_NON_ATTACKABLE);
                    if (pHummel->getVictim())
                        pFrye->AI()->AttackStart(pHummel->getVictim());
                }
                break;
            }
        }

        void JustSummoned(Creature* pSummoned) override
        {
            if (pSummoned->GetEntry() == NCP_CRAZED_APOTHECARY)
            {
                if (!m_pInstance)
                    return;

                // Make it attack a random Target
                Creature* pBoss = m_pInstance->GetSingleCreatureFromStorage(NPC_HUMMEL);
                if (!pBoss || !pBoss->IsAlive())
                    pBoss = m_pInstance->GetSingleCreatureFromStorage(NPC_BAXTER);
                if (!pBoss || !pBoss->IsAlive())
                    pBoss = m_pInstance->GetSingleCreatureFromStorage(NPC_FRYE);
                if (!pBoss || !pBoss->IsAlive())
                    return;

                // Attack a random target
                if (Unit* pTarget = pBoss->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                    pSummoned->AI()->AttackStart(pTarget);
            }
        }

        void ReceiveAIEvent(AIEventType type, Creature* sender, Unit* invoker, uint32 /*data*/)
        {
            if (type == AI_EVENT_CUSTOM_A && invoker && sender == m_creature)
                DoStartValentineEvent(invoker->GetObjectGuid());
        }

        // Wrapper to get the event started
        void DoStartValentineEvent(ObjectGuid starterGuid)
        {
            if (!m_pInstance)
                return;

            m_EventStarterGuid = starterGuid;

            if (Creature* pHummel = m_pInstance->GetSingleCreatureFromStorage(NPC_HUMMEL))
            {
                // I'm not sure if this unit flag should be used, but it's clear that the NPC shouldn't be attacked until the dialogue is finished
                pHummel->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                // WARNING: workaround -> faction should be set here - FIX THIS after the aura bug in core is fixed
                // pHummel->SetFactionTemporary(FACTION_HOSTILE, TEMPFACTION_RESTORE_REACH_HOME | TEMPFACTION_RESTORE_RESPAWN);
            }

            StartNextDialogueText(QUEST_BEEN_SERVED);

            // Move Baxter to position
            if (Creature* pBaxter = m_pInstance->GetSingleCreatureFromStorage(NPC_BAXTER))
            {
                pBaxter->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                // WARNING: workaround -> faction should be set here - FIX THIS after the aura bug in core is fixed
                // pBaxter->SetFactionTemporary(FACTION_HOSTILE, TEMPFACTION_RESTORE_REACH_HOME | TEMPFACTION_RESTORE_RESPAWN);

                if (GameObject* pVials = m_pInstance->GetSingleGameObjectFromStorage(GO_APOTHECARE_VIALS))
                {
                    float fX, fY, fZ;
                    pVials->GetContactPoint(pBaxter, fX, fY, fZ, CONTACT_DISTANCE);
                    pBaxter->SetWalk(false);
                    pBaxter->GetMotionMaster()->MovePoint(1, fX, fY, fZ);
                }
                else
                    script_error_log("Gameobject %u couldn't be found or something really bad happened.", GO_APOTHECARE_VIALS);
            }

            // Move Frye to position
            if (Creature* pFrye = m_pInstance->GetSingleCreatureFromStorage(NPC_FRYE))
            {
                pFrye->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                // WARNING: workaround -> faction should be set here - FIX THIS after the aura bug in core is fixed
                // pFrye->SetFactionTemporary(FACTION_HOSTILE, TEMPFACTION_RESTORE_REACH_HOME | TEMPFACTION_RESTORE_RESPAWN);

                if (GameObject* pChemistry = m_pInstance->GetSingleGameObjectFromStorage(GO_CHEMISTRY_SET))
                {
                    float fX, fY, fZ;
                    pChemistry->GetContactPoint(pFrye, fX, fY, fZ, CONTACT_DISTANCE);
                    pFrye->SetWalk(false);
                    pFrye->GetMotionMaster()->MovePoint(1, fX, fY, fZ);
                }
                else
                    script_error_log("Gameobject %u couldn't be found or something really bad happened.", GO_CHEMISTRY_SET);
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            DialogueUpdate(uiDiff);

            if (!m_pInstance)
                return;

            if (m_pInstance->GetData(TYPE_APOTHECARY) != IN_PROGRESS)
                return;

            if (m_uiCrazedApothecaryTimer < uiDiff)
            {
                if (Creature* pGenerator = m_pInstance->GetSingleCreatureFromStorage(NPC_APOTHECARY_GENERATOR))
                    pGenerator->CastSpell(pGenerator, SPELL_SUMMON_VALENTINE_ADD, true, NULL, NULL, m_creature->GetObjectGuid());

                m_uiCrazedApothecaryTimer = 30000;
            }
            else
                m_uiCrazedApothecaryTimer -= uiDiff;
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_valentine_boss_managerAI(pCreature);
    }
};

struct npc_apothecary_hummel : public CreatureScript
{
    npc_apothecary_hummel() : CreatureScript("npc_apothecary_hummel") {}

    bool OnQuestRewarded(Player* pPlayer, Creature* pCreature, Quest const* pQuest) override
    {
        if (pQuest->GetQuestId() == QUEST_BEEN_SERVED)
        {
            if (ScriptedInstance* pInstance = (ScriptedInstance*)pCreature->GetInstanceData())
            {
                if (Creature* pValentineMgr = pInstance->GetSingleCreatureFromStorage(NPC_VALENTINE_BOSS_MGR))
                {
                    if (CreatureAI* pManagerAI = pValentineMgr->AI())
                        pManagerAI->SendAIEvent(AI_EVENT_CUSTOM_A, pPlayer, pValentineMgr);
                }
            }
        }

        return true;
    }
};

void AddSC_boss_hummel()
{
    Script* s;
    s = new npc_valentine_boss_manager();
    s->RegisterSelf();
    s = new npc_apothecary_hummel();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_valentine_boss_manager";
    //pNewScript->GetAI = GetAI_npc_valentine_boss_manager;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_apothecary_hummel";
    //pNewScript->pQuestRewardedNPC = &QuestRewarded_npc_apothecary_hummel;
    //pNewScript->RegisterSelf();
}
#endif