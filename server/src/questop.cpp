#include "quest.hpp"
#include "totype.hpp"

void Quest::on_AM_METRONOME(const ActorMsgPack &)
{
}

void Quest::on_AM_REMOTECALL(const ActorMsgPack &mpk)
{
    const auto sdRC = mpk.deserialize<SDRemoteCall>();
    m_luaRunner->spawn(m_threadKey++, mpk.fromAddr(), sdRC.code);
}

void Quest::on_AM_RUNQUESTTRIGGER(const ActorMsgPack &mpk)
{
    std::visit(SDQuestTriggerDispatcher
    {
        [&mpk, this](const SDQuestTriggerLevelUp &sdQTLU)
        {
            m_luaRunner->spawn(m_threadKey++, str_printf("_RSVD_NAME_trigger(SYS_ON_LEVELUP, %llu, %d, %d)", to_llu(mpk.from()), sdQTLU.oldLevel, sdQTLU.newLevel));
        },

        [&mpk, this](const SDQuestTriggerKill &sdQTK)
        {
            m_luaRunner->spawn(m_threadKey++, str_printf("_RSVD_NAME_trigger(SYS_ON_KILL, %llu, %llu)", to_llu(mpk.from()), to_llu(sdQTK.monsterID)));
        },

        [&mpk, this](const SDQuestTriggerGainExp &sdQTGE)
        {
            m_luaRunner->spawn(m_threadKey++, str_printf("_RSVD_NAME_trigger(SYS_ON_GAINEXP, %llu, %llu)", to_llu(mpk.from()), to_llu(sdQTGE.addedExp)));
        },

        [&mpk, this](const SDQuestTriggerGainGold &sdQTGG)
        {
            m_luaRunner->spawn(m_threadKey++, str_printf("_RSVD_NAME_trigger(SYS_ON_GAINGOLD, %llu, %llu)", to_llu(mpk.from()), to_llu(sdQTGG.addedGold)));
        },

        [&mpk, this](const SDQuestTriggerGainItem &sdQTGI)
        {
            m_luaRunner->spawn(m_threadKey++, str_printf("_RSVD_NAME_trigger(SYS_ON_GAINITEM, %llu, %llu)", to_llu(mpk.from()), to_llu(sdQTGI.itemID)));
        },

        [](auto)
        {
            throw fflerror("invalid quest trigger");
        },
    }, mpk.deserialize<SDQuestTriggerVar>());
}

void Quest::on_AM_SENDNOTIFY(const ActorMsgPack &mpk)
{
    /* */ auto sdQN = mpk.deserialize<SDSendNotify>();
    const auto notifyNeeded = m_luaRunner->needNotify(sdQN.key, sdQN.seqID);

    m_luaRunner->addNotify(sdQN.key, sdQN.seqID, std::move(sdQN.varList));

    if(mpk.seqID()){
        if(sdQN.waitConsume){
            if(notifyNeeded.value_or(false)){
                m_luaRunner->resume(sdQN.key, sdQN.seqID);
            }
            m_actorPod->forward(mpk.fromAddr(), AM_OK);
        }
        else{
            m_actorPod->forward(mpk.fromAddr(), AM_OK);
            if(notifyNeeded.value_or(false)){
                m_luaRunner->resume(sdQN.key, sdQN.seqID);
            }
        }
    }
    else{
        m_luaRunner->resume(sdQN.key, sdQN.seqID);
    }
}
