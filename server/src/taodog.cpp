/*
 * =====================================================================================
 *
 *       Filename: taodog.cpp
 *        Created: 04/10/2016 02:32:45
 *    Description:
 *
 *        Version: 1.0
 *       Revision: none
 *       Compiler: gcc
 *
 *         Author: ANHONG
 *          Email: anhonghe@gmail.com
 *   Organization: USTC
 *
 * =====================================================================================
 */

#include "taodog.hpp"
#include "raiitimer.hpp"

corof::long_jmper TaoDog::updateCoroFunc()
{
    std::optional<uint64_t> idleTime;
    while(HP() > 0){
        if(const uint64_t targetUID = co_await coro_pickTarget()){
            idleTime.reset();
            co_await coro_trackAttackUID(targetUID);
        }

        else{
            if(!idleTime.has_value()){
                idleTime = hres_tstamp().to_nsec();
            }
            else if(hres_tstamp().to_nsec() - idleTime.value() > 30ULL * 1000000000ULL /* n x 1 sec*/){
                setStandMode(false);
            }

            if(m_actorPod->checkUIDValid(masterUID())){
                co_await coro_followMaster();
            }
            else{
                co_await corof::async_wait(200);
            }
        }
    }

    goDie();
    co_return true;
}

void TaoDog::onAMAttack(const ActorMsgPack &mpk)
{
    const auto amAK = mpk.conv<AMAttack>();
    if(m_dead.get()){
        notifyDead(amAK.UID);
    }
    else{
        if(mathf::LDistance2(X(), Y(), amAK.X, amAK.Y) > 2){
            switch(uidf::getUIDType(amAK.UID)){
                case UID_MON:
                case UID_PLY:
                    {
                        AMMiss amM;
                        std::memset(&amM, 0, sizeof(amM));

                        amM.UID = amAK.UID;
                        m_actorPod->forward(amAK.UID, {AM_MISS, amM});
                        return;
                    }
                default:
                    {
                        return;
                    }
            }
        }

        if(amAK.UID != masterUID()){
            setStandMode(true);
            addOffenderDamage(amAK.UID, amAK.Damage);
        }

        dispatchAction(ActionHitted
        {
            .x = X(),
            .y = Y(),
            .direction = Direction(),
            .extParam
            {
                .dog
                {
                    .standMode = m_standMode,
                }
            }
        });
        struckDamage({amAK.UID, amAK.Type, amAK.Damage, amAK.Element, amAK.Effect});
    }
}