
var currentApp = new Vue({
  el: '#app',
  data:
  {
    sysinfos: [],
	setting_entries: []
  },
  methods: {
	idBtnReboot_Click(event)
	{
		SendAction("/action/reboot");
	},
	idBtnCheckIgnition_Click(event)
	{
		SendAction("/action/checkignition");
	},
	idBtnDoorTest_Click(event)
	{
		SendAction("/action/doortest");
	},
	idBtnDisarm_Click(event)
	{
		SendAction("/action/disarm");
	}
  }
})

function AppLoaded()
{
	fetch('./api/getsysinfo')
		.then((response) => response.json())
		.then((data) =>
		{
			currentApp.sysinfos = data.infos;
		});
}
