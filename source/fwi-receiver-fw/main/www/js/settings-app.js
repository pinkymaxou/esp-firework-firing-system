var g_current_app = new Vue(
{
    el: '#app',
    data:
    {
        sysinfos: [],
        setting_entries: [],
        edited_values: {},
        save_status: ''
    },
    methods:
    {
        idBtnUploadClick()
        {
            const firmware_file = document.getElementById("idFile").files[0];
            if (!firmware_file)
                return;

            const xhr = new XMLHttpRequest();
            xhr.open('POST', '/ota/upload', true);

            xhr.upload.onprogress = (e) => { console.log("upload progress", e); };
            xhr.upload.onerror    = (e) => { console.error("upload error", e); };
            xhr.upload.onabort    = (e) => { console.warn("upload aborted", e); };

            xhr.send(firmware_file);
        },
        idBtnSaveClick()
        {
            const entries = this.setting_entries.map(e => ({
                key: e.key,
                value: this.edited_values[e.key]
            }));
            sendWS({ cmd: "setsettings", entries: entries });
            this.save_status = 'Saved';
            setTimeout(() => { this.save_status = ''; }, 2000);
        },
        needsReboot()
        {
            return this.setting_entries.some(e =>
                e.info.flag_reboot &&
                String(this.edited_values[e.key]) !== String(e.value)
            );
        }
    }
});

function appLoaded()
{
    g_wsMessageHandler = (msg) =>
    {
        if ("settings" === msg.type)
        {
            g_current_app.setting_entries = msg.entries || [];
            const ev = {};
            (msg.entries || []).forEach(e => { ev[e.key] = e.value; });
            g_current_app.edited_values = ev;
        }
        else if ("sysinfo" === msg.type)
        {
            g_current_app.sysinfos = msg.infos || [];
        }
    };

    g_wsOpenHandler = () =>
    {
        sendWS({ cmd: "getsettings" });
        sendWS({ cmd: "getsysinfo" });
    };

    connectWebSocket();
}
