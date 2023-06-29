function main()
    uidExecute(getNPCharUID('比奇县_0', '王大人_1'),
    [[
        local questUID  = %d
        local questName = '%s'
        local questPath = {SYS_EPQST, questName}

        return setQuestHandler(questName,
        {
            [SYS_CHECKACTIVE] = function(uid)
                return uidExecute(uid, [=[ return getQuestState('初出江湖') ]=]) == SYS_EXIT
            end,

            [SYS_ENTER] = function(uid, value)
                local level = uidExecute(uid, [=[ return getLevel() ]=])
                if level < 9 then
                    uidPostXML(uid, questPath,
                    [=[
                        <layout>
                            <par>我要托付你帮我办点事情，但是现在的你好象还有点应付不了。再去好好修炼一下，等级达到9级的时候才能够得到我的信任让我把这件事情交给你去做。</par>
                            <par></par>
                            <par><event id="%%s">退出</event></par>
                        </layout>
                    ]=], SYS_EXIT)
                else
                    uidPostXML(uid, questPath,
                    [=[
                        <layout>
                            <par>噢！你来啦！你要是不来我正好要派人去叫你呢？其实我是有事情要拜托你做！是这样的！最近以我为会长的比奇商会和以那个姓崔的贪得无厌的家伙为首的传奇商会正在互相争夺势力。</par>
                            <par>啊！当然不是真刀真枪的动武啦！是为了争夺商权而展开的势力之争。不管怎么样，胜者为王败者寇，在这场斗争中失败者将被排挤出比奇省。因此想要求你帮我办点事情，为了增强我们比奇商会的势力，请你去说服图书管理人和施药商药剂师从传奇商会中退出，来加入我们比奇商会。</par>
                            <par></par>
                            <par><event id="npc_decide" arg="true" >好的！</event></par>
                            <par><event id="npc_decide" arg="false">我还有别的事情要做。</event></par>
                        </layout>
                    ]=])
                end
            end,

            npc_decide = function(uid, value)
                uidExecute(questUID, [=[ setUIDQuestState(%%d, SYS_ENTER, %%s) ]=], uid, value)
            end,
        })
    ]], getUID(), getQuestName())

    setQuestFSMTable(
    {
        [SYS_ENTER] = function(uid, value)
            uidExecute(getNPCharUID('比奇县_0', '王大人_1'),
            [[
                local playerUID = %d
                local accepted  = %s
                local questName = '%s'
                local questPath = {SYS_EPQST, questName}

                if accepted then
                    uidPostXML(playerUID, questPath,
                    [=[
                        <layout>
                            <par>妈耶！你来辣！</par>
                            <par></par>
                            <par><event id="%%s">结束</event></par>
                        </layout>
                    ]=], SYS_EXIT)
                else
                    uidPostXML(playerUID, questPath,
                    [=[
                        <layout>
                            <par>不行？</par>
                            <par>那么我只好再去找其他人了。</par>
                            <par></par>
                            <par><event id="%%s">结束</event></par>
                        </layout>
                    ]=], SYS_EXIT)
                end
            ]], uid, value, getQuestName())
        end,
    })
end
