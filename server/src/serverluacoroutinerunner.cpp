#include <memory>
#include "luaf.hpp"
#include "uidf.hpp"
#include "totype.hpp"
#include "fflerror.hpp"
#include "actorpod.hpp"
#include "serdesmsg.hpp"
#include "monoserver.hpp"
#include "serverluacoroutinerunner.hpp"

extern MonoServer *g_monoServer;
ServerLuaCoroutineRunner::ServerLuaCoroutineRunner(ActorPod *podPtr, std::function<void(ServerLuaModule *)> fnBindExtraFuncs)
    : ServerLuaModule()
    , m_actorPod([podPtr]()
      {
          fflassert(podPtr); return podPtr;
      }())
{
    if(fnBindExtraFuncs){
        fnBindExtraFuncs(this);
    }

    bindFunction("_RSVD_NAME_sendRemoteCall", [this](uint64_t threadKey, uint64_t uid, std::string code)
    {
        if(!m_runnerList.contains(threadKey)){
            throw fflerror("calling sendRemoteCall(%llu, %llu, %s) outside of ServerLuaCoroutineRunner", to_llu(threadKey), to_llu(uid), to_cstr(code));
        }

        m_actorPod->forward(uid, {AM_REMOTECALL, cerealf::serialize(SDRemoteCall
        {
            .code = code,
        })},

        [threadKey, uid, code /* not ref */, this](const ActorMsgPack &mpk)
        {
            if(uid != mpk.from()){
                throw fflerror("lua code sent to uid %llu but get response from %llu", to_llu(uid), to_llu(mpk.from()));
            }

            if(mpk.seqID()){
                throw fflerror("remote call responder expects response");
            }

            auto p = m_runnerList.find(threadKey);
            if(p == m_runnerList.end()){
                // can not find runner
                // runner can be cancelled already, or just an error

                // but hard to tell if this is an error
                // because we didn't record keys of cancelled runners
                return;
            }

            if(!p->second->callback){
                throw fflerror("coroutine is not callable");
            }

            p->second->clearEvent();
            p->second->from = mpk.from();

            switch(mpk.type()){
                case AM_SDBUFFER:
                    {
                        // setup the call event/value as result
                        // event/value get consumed in _RSVD_NAME_pollRemoteCallResult()

                        p->second->event = SYS_EXECDONE;
                        p->second->value = std::string(reinterpret_cast<const char *>(mpk.data()), mpk.size());
                        break;
                    }
                default:
                    {
                        // any message other than AM_SDBUFFER means bad uid
                        // still need to inform current runner

                        p->second->event = SYS_BADUID;
                        p->second->value.clear();
                        break;
                    }
            }

            // comsume the event
            // call the coroutine to make it fail or stuck at next _RSVD_NAME_pollRemoteCallResult()
            resumeRunner(p->second.get());
        });
    });

    bindFunction("_RSVD_NAME_pollRemoteCallResult", [this](uint64_t threadKey, sol::this_state s)
    {
        sol::state_view sv(s);
        return sol::as_returns([threadKey, &sv, this]() -> std::vector<sol::object>
        {
            if(auto p = m_runnerList.find(threadKey); p != m_runnerList.end()){
                const auto from  = p->second->from;
                const auto event = std::move(p->second->event);
                const auto value = std::move(p->second->value);

                p->second->clearEvent();
                if(!from){
                    return {};
                }

                fflassert(str_haschar(event));
                std::vector<sol::object> eventStack
                {
                    sol::object(sv, sol::in_place_type<uint64_t>, from),
                    sol::object(sv, sol::in_place_type<std::string>, event),
                };

                if(event == SYS_BADUID){
                    // shouldn't throw here, only assert value's zero-length
                    // because uid can turn invalid after validation, lua script need to handle this
                    fflassert(value.empty(), value);
                }
                else if(event == SYS_EXECDONE){
                    const auto sdLCR = cerealf::deserialize<SDRemoteCallResult>(value);
                    if(sdLCR.error.empty()){
                        for(const auto &s: sdLCR.serVarList){
                            eventStack.push_back(luaf::buildLuaObj(sv, s));
                        }
                    }
                    else{
                        // lua script doesn't need to handle remote call error
                        // _RSVD_NAME_pollRemoteCallResult always return valid result from remote peer if not throw
                        fflassert(sdLCR.serVarList.empty(), sdLCR.error, sdLCR.serVarList);
                        for(const auto &line: sdLCR.error){
                            g_monoServer->addLog(LOGTYPE_WARNING, "%s", to_cstr(line));
                        }
                        throw fflerror("lua call failed in %s", to_cstr(uidf::getUIDString(from)));
                    }
                }
                else{
                    throw fflerror("invalid remote call event: %s", to_cstr(event));
                }
                return eventStack;
            }
            throw fflerror("can't find key: %llu", to_llu(threadKey));
        }());
    });

    pfrCheck(execRawString(BEGIN_LUAINC(char)
#include "serverluacoroutinerunner.lua"
    END_LUAINC()));
}

uint64_t ServerLuaCoroutineRunner::spawn(uint64_t key, uint64_t reqUID, uint64_t msgSeqID, const char *code)
{
    fflassert(key);
    fflassert(code);

    if(reqUID) fflassert( msgSeqID, reqUID, msgSeqID);
    else       fflassert(!msgSeqID, reqUID, msgSeqID);

    const auto [p, added] = m_runnerList.insert_or_assign(key, std::make_unique<_CoroutineRunner>(*this, key, m_seqID++, reqUID, msgSeqID));
    const auto currSeqID = p->second->seqID;

    resumeRunner(p->second.get(), str_printf(
        R"###( getTLSTable().threadKey = %llu            )###""\n"
        R"###( getTLSTable().startTime = getNanoTstamp() )###""\n"
        R"###( do                                        )###""\n"
        R"###(    %s                                     )###""\n"
        R"###( end                                       )###""\n", to_llu(key), code));

    return currSeqID;
}

void ServerLuaCoroutineRunner::resumeRunner(ServerLuaCoroutineRunner::_CoroutineRunner *runnerPtr, std::optional<std::string> codeOpt)
{
    // resume current runnerPtr
    // this function can invalidate runnerPtr if it's done

    std::vector<std::string> error;
    const auto fnDrainError = [&error](const std::string &s)
    {
        error.push_back(s);
    };

    fflassert(runnerPtr->callback);
    const auto fnOnThreadDone = [runnerPtr, this](std::vector<std::string> error, std::vector<std::string> serVarList)
    {
        if(!error.empty()){
            fflassert(serVarList.empty(), error, serVarList);
        }

        if(runnerPtr->reqUID){
            m_actorPod->forward(runnerPtr->reqUID, {AM_SDBUFFER, cerealf::serialize(SDRemoteCallResult
            {
                .error = std::move(error),
                .serVarList = std::move(serVarList),
            })},

            runnerPtr->msgSeqID);
        }
        else{
            // script issued by self
            // like ServerMap use it to run its peroiodic update script
            for(const auto &line: error){
                g_monoServer->addLog(LOGTYPE_WARNING, "%s", to_cstr(line));
            }

            // drop result
            // no good way to dump result for now, should I warning that result is dropped?
        }

        close(runnerPtr->key);
    };

    if(const auto pfr = codeOpt.has_value() ? runnerPtr->callback(codeOpt.value()) : runnerPtr->callback(); pfrCheck(pfr, fnDrainError)){
        // trigger the coroutine only *one* time
        // in principle the script runs in synchronized model, so here we can trigger aribitary time
        if(runnerPtr->callback){
            // still not done yet, wait for next trigger
            // script is ill-formed if there is no scheduled trigger for next
        }
        else{
            // initial run succeeds and coroutine finished
            // simple cases like: uidExecute(uid, [[ return getName() ]])
            //
            // for this case there is no need to uses coroutine
            // but we can not predict script from NPC is synchronized call or not
            //
            // for cases like spaceMove() we can use quasi-function from NPC side
            // but it prevents NPC side to execute commands like:
            //
            //     uidExecute(uid, [[
            //         spaceMove(12, 22, 12) -- impossible if using quasi-function
            //         return getLevel()
            //     ]])
            //
            // for the quasi-function solution player side has no spaceMove() function avaiable
            // player side can only support like:
            //
            //     'SPACEMOVE 12, 22, 12'
            //
            // this is because spaveMove is a async operation, it needs callbacks
            // this limits the script for NPC side, put everything into lua coroutine is a solution for it, but with cost
            //
            // for how quasi-function was implemented
            // check commit: 30981cc539a05b41309330eaa04fbf3042c9d826
            //
            // starting/running an coroutine with a light function in Lua costs ~280 ns
            // alternatively call a light function directly takes ~30 ns, best try is every time let uidExecute() do as much as possible
            //
            // light cases, no yield in script
            // directly return the result with save the runner
            fnOnThreadDone(std::move(error), luaf::pfrBuildBlobList(pfr));
        }
    }
    else{
        if(error.empty()){
            error.push_back(str_printf("unknown error for runner: key = %llu", to_llu(runnerPtr->key)));
        }
        fnOnThreadDone(std::move(error), {});
    }
}
