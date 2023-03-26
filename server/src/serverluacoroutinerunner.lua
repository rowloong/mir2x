--, u8R"###(
--

function _RSVD_NAME_luaCoroutineRunner_main(code)
    assertType(code, 'string')
    return (load(code))()
end

local function _RSVD_NAME_waitRemoteCallResult()
    local seqID = getTLSTable().threadKey
    while true do
        local resList = {_RSVD_NAME_pollRemoteCallResult(seqID)}
        if next(resList) == nil then
            coroutine.yield()
        else
            local from  = resList[1]
            local event = resList[2]

            assertType(from, 'integer')
            assertType(event, 'string')

            return table.unpack(resList)
        end
    end
end

-- send lua code to uid to execute
-- used to support complicated logic through actor message
local function _RSVD_NAME_uidExecute(uid, code)
    assertType(uid, 'integer')
    assertType(code, 'string')
    _RSVD_NAME_sendRemoteCall(getTLSTable().uid, uid, code, false)

    local resList = {_RSVD_NAME_waitRemoteCallResult()}
    if resList[1] ~= uid then
        fatalPrintf('Send lua code to uid %s but get response from %d', uid, resList[1])
    end

    if resList[2] ~= SYS_EXECDONE then
        fatalPrintf('Wait event as SYS_EXECDONE but get %s', resList[2])
    end

    return table.unpack(resList, 3)
end

function uidExecute(uid, code, ...)
    return _RSVD_NAME_uidExecute(uid, code:format(...))
end

--
-- )###"