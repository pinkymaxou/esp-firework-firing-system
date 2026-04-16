let g_current_data =
{
    is_connected: false,
    status:
    {
        req: 0,
        is_armed: false,
        general_state: 0,
        outputs: []
    }
};

var g_current_app = new Vue(
{
    el: '#app',
    data: g_current_data,
    methods:
    {
        translateGeneralState(value)
        {
            switch (value)
            {
                case EGeneralState.Idle:
                    return "Idle";
                case EGeneralState.FiringMasterSwitchWrongStateError:
                    return "Firing master switch in the wrong position";
                case EGeneralState.FiringUnknownError:
                    return "Unknown firing error";
                case EGeneralState.Firing:
                    return "Firing";
                case EGeneralState.FiringOK:
                    return "Firing OK";
                case EGeneralState.ArmingSystemOK:
                    return "System armed and ready";
                case EGeneralState.CheckingConnection:
                    return "Checking connections";
                case EGeneralState.CheckingConnectionOK:
                    return "Check connections OK";
                case EGeneralState.CheckingConnectionError:
                    return "Check connections error";
                case EGeneralState.LiveCheckContinuity:
                    return "Live check continuity";
                case EGeneralState.DisarmedMasterSwitchOff:
                    return "Disarmed (automatic, wrong master switch state)";
            }
            return 'Unknown #' + String(value);
        },
        idBtnCheckIgnitionClick()
        {
            sendWS({ cmd: "checkconnections" });
        },
        idBtnFireClick(ix)
        {
            sendWS({ cmd: "fire", index: ix });
        },
        getOutputClass(output)
        {
            switch (output.s)
            {
                case EOutputState.Enabled:   return 'button-enabled';
                case EOutputState.Fired:     return 'button-fired';
                case EOutputState.Connected: return this.status.is_armed ? 'button-connected-armed' : 'button-connected';
                default:                     return 'button-idle';
            }
        }
    }
});

function appLoaded()
{
    console.log("page loaded");

    g_wsMessageHandler = (msg) =>
    {
        if ("status" === msg.type)
        {
            g_current_data.status = msg.status;
            g_current_data.is_connected = true;
        }
    };

    g_wsOpenHandler = () =>
    {
        g_current_data.is_connected = true;
        sendWS({ cmd: "getstatus" });
    };

    g_wsCloseHandler = () =>
    {
        g_current_data.is_connected = false;
    };

    connectWebSocket();
    setInterval(() => { sendWS({ cmd: "getstatus" }); }, 250);
}
