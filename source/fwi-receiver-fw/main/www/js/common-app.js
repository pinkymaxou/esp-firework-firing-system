const EOutputState =
{
    Idle: 0,
    Enabled: 1,
    Fired: 2,
    Connected: 3
};

const EGeneralState =
{
    Idle: 0,
    FiringMasterSwitchWrongStateError: 1,
    FiringUnknownError: 2,
    Firing: 3,
    FiringOK: 4,
    ArmingSystemOK: 7,
    CheckingConnection: 8,
    CheckingConnectionOK: 9,
    CheckingConnectionError: 10,
    LiveCheckContinuity: 11,
    DisarmedMasterSwitchOff: 12
};

let g_ws = null;
let g_wsMessageHandler = null;
let g_wsOpenHandler = null;
let g_wsCloseHandler = null;

function connectWebSocket()
{
    const url = `ws://${window.location.host}/ws`;
    g_ws = new WebSocket(url);

    g_ws.onopen = () =>
    {
        console.log("WebSocket connected");
        if (g_wsOpenHandler)
            g_wsOpenHandler();
    };

    g_ws.onmessage = (event) =>
    {
        try
        {
            const msg = JSON.parse(event.data);
            if (g_wsMessageHandler)
                g_wsMessageHandler(msg);
        }
        catch (e)
        {
            console.error("WS message parse error", e);
        }
    };

    g_ws.onerror = (err) =>
    {
        console.error("WebSocket error", err);
    };

    g_ws.onclose = () =>
    {
        console.log("WebSocket closed, reconnecting in 3s");
        g_ws = null;
        if (g_wsCloseHandler)
            g_wsCloseHandler();
        setTimeout(connectWebSocket, 3000);
    };
}

function sendWS(data)
{
    if (null !== g_ws && WebSocket.OPEN === g_ws.readyState)
    {
        g_ws.send(JSON.stringify(data));
        return true;
    }
    return false;
}
