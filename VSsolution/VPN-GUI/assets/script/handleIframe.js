function isValidIPAddress(ipAddress) {
    var ipRegex = /^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$/;

    return ipRegex.test(ipAddress)
}

const iframe = document.getElementById('content')
let side = undefined
let setupSettings = {}

iframe.addEventListener('load', () => {
    side = iframe.name

    if (iframe.contentDocument.title == 'setup') {
        if (localStorage.getItem(side) != null) {
            setupSettings = JSON.parse(localStorage.getItem(side))
            let parsed = JSON.parse(localStorage.getItem(side))
            iframe.contentDocument.getElementById('primary').value = parsed.primary
            iframe.contentDocument.getElementById('port').value = parsed.port
            iframe.contentDocument.getElementById('secondary').value = parsed.secondary
        }
        iframe.contentDocument.getElementById('confirm')
            .addEventListener('click', confirmBtn);
    } else if (iframe.contentDocument.title == 'status') {
        localStorage.clear()
        checkLocalStorage()
    } else {
        return
    }

    window.electronAPI.recvData(side, (event, msg) => {
        console.log(msg)
    })
})

function confirmBtn(event) {
    const ids = ['key', 'cert', 'encryption', 'primary', 'port', 'secondary'];
    const contentDocument = iframe.contentDocument;

    ids.forEach(id => {
        const element = contentDocument.getElementById(id);
        if (element) {
            const value = id === 'port' ? Number(element.value) : element.value;
            setupSettings[id] = id.startsWith('port') ? value || setupSettings[id] : value || setupSettings[id];
        }
    });

    localStorage.setItem(side, JSON.stringify(setupSettings));
    console.log(setupSettings);
}

function checkLocalStorage() {
    if (localStorage.getItem(side) == null) {
        iframe.contentDocument.getElementById('status').innerHTML = "WARNING missing values!"
    } else {
        iframe.contentDocument.getElementById('status').innerHTML = "WARNING some values are missing!"
    }
}

function startTunnel(event) {
    const primaryIP = iframe.contentDocument.getElementById('primary')
    const port = iframe.contentDocument.getElementById('port')
    const secondaryIP = iframe.contentDocument.getElementById('secondary')

    const primaryCheck = (primaryIP.value.length < 7 || !isValidIPAddress(primaryIP.value))
    const portCheck = (Number(port.value) <= 0 || Number(port.value) > 65535)
    const secondaryCheck = (secondaryIP.value.length < 7 || !isValidIPAddress(secondaryIP.value))

    primaryIP.classList.toggle("error", primaryCheck);
    port.classList.toggle("error", portCheck);
    secondaryIP.classList.toggle("error", secondaryCheck);

    if (!(primaryCheck || portCheck || secondaryCheck)) {
        window.electronAPI.sendData('server', {
            action: 'start',
            args: [
                event.target.name,
                primaryIP.value,
                port.value,
                secondaryIP.value
            ]
        })
    }
}